// Copyright (c) 2026 madalone. ScreenOffSync — screen-off animation in step with the core's display switch-off.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screenOffSync.h"

#include <climits>

#include "../logging.h"

namespace uc {
namespace hw {

ScreenOffSync *ScreenOffSync::s_instance = nullptr;

namespace {
// For the log lines (no meta-object lookup, so the unit test needs none of Power's own code)
const char *modeName(Power::PowerMode mode) {
    switch (mode) {
        case Power::Normal:
            return "Normal";
        case Power::Idle:
            return "Idle";
        case Power::Low_power:
            return "Low_power";
        case Power::Suspend:
            return "Suspend";
    }
    return "?";
}
}  // namespace

int ScreenOffSync::dimPhaseMinMsFor(int displayOffSec) {
    if (displayOffSec <= 0) return 0;  // behaves like 3 s in the core (dims at ~4 s, off up to 1 s later)
    return qMax(0, displayOffSec - qMax(displayOffSec / 2, 3)) * 1000;
}

ScreenOffSync::ScreenOffSync(core::Api *core, QObject *parent) : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

ScreenOffSync::~ScreenOffSync() { s_instance = nullptr; }

QObject *ScreenOffSync::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)
    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
    return obj;
}

int ScreenOffSync::msSinceIdle() const {
    if (m_mode != Power::Idle || !m_idleTimer.isValid()) return -1;
    return static_cast<int>(qMin<qint64>(m_idleTimer.elapsed(), INT_MAX));
}

int ScreenOffSync::dimPhaseMinMs() const {
    return dimPhaseMinMsFor(m_displayOffSec);
}

int ScreenOffSync::startDelayMs(int leadMs) const {
    const int since = msSinceIdle();
    if (since < 0) return -1;
    const int delay = qMax(0, dimPhaseMinMs() - END_MARGIN_MS - qMax(0, leadMs) - since);
    qCDebug(lcHw()) << "ScreenOffSync: a" << leadMs << "ms animation starts in" << delay << "ms (" << since
                    << "ms into the dim phase)";
    return delay;
}

int ScreenOffSync::fitLeadMs(int leadMs) const {
    const int since = msSinceIdle();
    if (since < 0) return leadMs;
    const int room = qMax(0, dimPhaseMinMs() - END_MARGIN_MS - since);
    return qMin(qMax(0, leadMs), room);
}

void ScreenOffSync::markEffectStart() {
    m_effectInputSerial = m_inputSerial;
    m_effectMarked = true;
    qCDebug(lcHw()) << "ScreenOffSync: screen-off animation started" << msSinceIdle() << "ms into the dim phase";
}

bool ScreenOffSync::requestDisplayOff() {
    const int since = msSinceIdle();
    if (since < 0) {
        qCInfo(lcHw()) << "ScreenOffSync: animation ended outside the dim phase (mode" << modeName(m_mode)
                       << "), display left to the core";
        return false;
    }
    if (!m_effectMarked || m_inputSerial != m_effectInputSerial) {
        qCInfo(lcHw()) << "ScreenOffSync: input during the screen-off animation, display left to the core";
        return false;
    }
    if (m_requested) return true;  // already sent in this dim phase

    const int id = m_core->setPowerMode(core::PowerEnums::PowerMode::LOW_POWER);
    if (id < 0) {
        qCWarning(lcHw()) << "ScreenOffSync: LOW_POWER request not sent (no core connection)";
        return false;
    }
    m_requested = true;
    m_requestedAtMs = since;
    qCInfo(lcHw()) << "ScreenOffSync: screen-off animation ended" << since << "ms into the dim phase (shortest"
                   << dimPhaseMinMs() << "ms): LOW_POWER requested";
    m_core->onResult(
        id,
        []() {  // success: the core announces Low_power with power_mode_change
        },
        [this](int code, QString message) {
            // The core switching off by itself at the same moment refuses the request: not a fault
            if (m_mode == Power::Low_power || m_mode == Power::Suspend) {
                qCInfo(lcHw()) << "ScreenOffSync: LOW_POWER request refused, the display is already off:" << code
                               << message;
            } else {
                qCWarning(lcHw()) << "ScreenOffSync: LOW_POWER request failed:" << code << message;
            }
        });
    return true;
}

void ScreenOffSync::onPowerModeChanged(Power::PowerMode fromPowerMode, Power::PowerMode toPowerMode) {
    if (fromPowerMode == Power::Idle && m_idleTimer.isValid()) {
        const qint64 dimMs = m_idleTimer.elapsed();
        if (toPowerMode == Power::Low_power || toPowerMode == Power::Suspend) {
            if (m_requested) {
                qCInfo(lcHw()) << "ScreenOffSync: display off" << (dimMs - m_requestedAtMs)
                               << "ms after the request";
            } else {
                // The core's own switch-off: its dim phase, against the rule (a change shows up here)
                qCInfo(lcHw()) << "ScreenOffSync: the core switched the display off after a dim phase of" << dimMs
                               << "ms (shortest expected" << dimPhaseMinMs() << "ms)";
            }
        }
    }
    m_mode = toPowerMode;
    m_requested = false;
    m_requestedAtMs = -1;
    m_effectMarked = false;
    if (toPowerMode == Power::Idle) {
        m_idleTimer.start();
    } else {
        m_idleTimer.invalidate();
    }
}

void ScreenOffSync::onUserInput() { ++m_inputSerial; }

}  // namespace hw
}  // namespace uc
