// Copyright (c) 2026 madalone. PhantomWakeSuppressor (Mod 6, v1.4.20).
// SPDX-License-Identifier: GPL-3.0-or-later

#include "phantomWakeSuppressor.h"

#include <QCoreApplication>

#include "../logging.h"

namespace uc {
namespace hw {

PhantomWakeSuppressor* PhantomWakeSuppressor::s_instance = nullptr;

PhantomWakeSuppressor::PhantomWakeSuppressor(core::Api* core, QObject* parent)
    : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    // Single-shot grace timer: fires forceLowPower() once if the grace window expires
    // without user input. Restarting (m_graceTimer.start()) resets the countdown — used
    // when a second wake event lands during an already-armed grace window.
    m_graceTimer.setSingleShot(true);
    QObject::connect(&m_graceTimer, &QTimer::timeout, this, &PhantomWakeSuppressor::forceLowPower);

    // Defensive: cancel any pending force on core disconnect — connection's down anyway.
    QObject::connect(m_core, &core::Api::disconnected, this, &PhantomWakeSuppressor::onCoreDisconnected);

    // Clean shutdown — stop the timer so we don't fire force-back during teardown.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, [this] { m_graceTimer.stop(); });
}

PhantomWakeSuppressor::~PhantomWakeSuppressor() { s_instance = nullptr; }

QObject* PhantomWakeSuppressor::qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine) {
    Q_UNUSED(scriptEngine)
    QObject* obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
    return obj;
}

void PhantomWakeSuppressor::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    emit enabledChanged();
    qCDebug(lcHw()) << "PhantomWakeSuppressor enabled:" << enabled;
    // If disabled mid-grace, cancel the pending force.
    if (!m_enabled && m_graceTimer.isActive()) {
        cancelGrace("disabled mid-grace");
    }
}

void PhantomWakeSuppressor::setGraceMs(int ms) {
    // Clamp to [100, 5000] — v1.4.23 expanded from 2000 (matches Power.qml slider range).
    const int clamped = qBound(100, ms, 5000);
    if (m_graceMs == clamped) return;
    m_graceMs = clamped;
    emit graceMsChanged();
    // If a grace window is currently active, restart with the new interval so the
    // user-visible effect of changing the slider is immediate.
    if (m_graceTimer.isActive()) {
        m_graceTimer.start(m_graceMs);
    }
}

// v1.4.23: recent-input lookback — handles firmware delivery ordering where
// keyPressed/entityCommandIssued can fire BEFORE powerModeChanged on wake-from-LOW_POWER.
void PhantomWakeSuppressor::setInputLookbackMs(int ms) {
    const int clamped = qBound(0, ms, 2000);  // 0 disables; matches Power.qml slider range
    if (m_inputLookbackMs == clamped) return;
    m_inputLookbackMs = clamped;
    emit inputLookbackMsChanged();
}

void PhantomWakeSuppressor::onPowerModeChanged(Power::PowerMode fromPowerMode, Power::PowerMode toPowerMode) {
    if (!m_enabled) return;

    // wasAsleep gate matches the v1.4.19 wake-replay HUD precedent at
    // entityController.cpp:761-762. Mod 5's setPowerMode(NORMAL) pings only fire while
    // the device is already in NORMAL/IDLE (Mod 5's m_active gate is false in LOW_POWER),
    // so they cause NORMAL->NORMAL or IDLE->NORMAL transitions — never LOW_POWER->NORMAL.
    // The wasAsleep check therefore cleanly distinguishes phantom wakes from Mod 5 pings.
    const bool wasAsleep = (fromPowerMode == Power::PowerMode::Low_power ||
                            fromPowerMode == Power::PowerMode::Suspend);

    if (toPowerMode == Power::PowerMode::Normal && wasAsleep) {
        // v1.4.23: skip-grace if user input arrived within m_inputLookbackMs of this wake.
        // Firmware delivers wake-from-LOW_POWER input events (keyPressed, entityCommandIssued)
        // BEFORE the powerModeChanged signal, so by the time we'd arm the grace timer the
        // input has already been processed. Tracking "last input" lets us recognize this
        // ordering and skip arming entirely. m_inputLookbackMs == 0 disables the skip.
        if (m_inputLookbackMs > 0 && m_lastInputTimer.isValid()
                && m_lastInputTimer.elapsed() < m_inputLookbackMs) {
            qCWarning(lcHw()) << "PhantomWakeSuppressor: skipping grace (input"
                              << m_lastInputTimer.elapsed() << "ms ago, lookback"
                              << m_inputLookbackMs << "ms)";
            return;
        }
        // Wake event detected — arm the grace window. If the user presses something
        // within m_graceMs, onUserInput() cancels it. Otherwise forceLowPower() fires.
        m_graceTimer.start(m_graceMs);
        emit armedChanged();
        // v1.4.22: promoted from qCDebug → qCWarning so this surfaces in
        // /api/system/logs (NOTICE+ filter) and is prominent in Logdy captures.
        // Diagnostic event, not a real warning — semantic stretch is intentional
        // because Qt 5.15 has no qCNotice and qCInfo is filtered out.
        qCWarning(lcHw()) << "PhantomWakeSuppressor armed:" << m_graceMs << "ms";
    } else if (toPowerMode != Power::PowerMode::Normal && m_graceTimer.isActive()) {
        // Defensive: firmware itself transitioned away from NORMAL during our grace.
        // Don't double-fire force-back; let the firmware's own transition stand.
        cancelGrace("powerMode left NORMAL during grace");
    }
}

void PhantomWakeSuppressor::onUserInput() {
    // v1.4.23: always update last-input timestamp (used by onPowerModeChanged for
    // skip-grace logic), regardless of whether a grace window is currently active.
    m_lastInputTimer.restart();
    if (!m_graceTimer.isActive()) return;
    cancelGrace("user input");
}

void PhantomWakeSuppressor::onEntityCommandIssued(QString entityId, QString command) {
    Q_UNUSED(entityId)
    // Curated allowlist mirrors ActivitySessionKeeper::onEntityCommandIssued — only
    // human-facing commands count as wake-press evidence. Avoids false-cancel on
    // poll-noise (state queries / capability fetches that some integrations fire
    // during the post-wake reconnect window).
    static const QSet<QString> kUserCommands = {
        "PLAY_PAUSE", "PLAY", "STOP", "PAUSE", "SEEK",
        "VOLUME", "VOLUME_UP", "VOLUME_DOWN", "MUTE_TOGGLE", "MUTE", "UNMUTE",
        "CURSOR_UP", "CURSOR_DOWN", "CURSOR_LEFT", "CURSOR_RIGHT", "CURSOR_ENTER",
        "CHANNEL_UP", "CHANNEL_DOWN",
        "NEXT", "PREVIOUS", "FAST_FORWARD", "REWIND",
    };
    if (!kUserCommands.contains(command.toUpper())) return;
    // v1.4.23: update last-input timestamp here too, so an entity command arriving
    // before the powerModeChanged signal still counts toward the skip-grace lookback.
    m_lastInputTimer.restart();
    if (!m_graceTimer.isActive()) return;
    cancelGrace("entity command");
}

void PhantomWakeSuppressor::onPowerSupplyChanged(bool onAc) {
    // Both directions count as user activity (placing on dock or removing from dock).
    Q_UNUSED(onAc)
    onUserInput();
}

void PhantomWakeSuppressor::onCoreDisconnected() {
    if (!m_graceTimer.isActive()) return;
    cancelGrace("core disconnected");
}

void PhantomWakeSuppressor::cancelGrace(const char* reason) {
    m_graceTimer.stop();
    emit armedChanged();
    // v1.4.22: promoted to qCWarning. The cancel reason is the load-bearing
    // diagnostic — distinguishes "real user input fired during grace" (Mod 6
    // working as designed) from "ghost touch defeated Mod 6" (mitigation needed).
    qCWarning(lcHw()) << "PhantomWakeSuppressor cancelled:" << reason;
}

void PhantomWakeSuppressor::forceLowPower() {
    // v1.4.22: was qCInfo, promoted to qCWarning to match cancelGrace and onPowerModeChanged.
    qCWarning(lcHw()) << "PhantomWakeSuppressor: phantom wake -> forced LOW_POWER";
    int id = m_core->setPowerMode(core::PowerEnums::PowerMode::LOW_POWER);
    m_core->onResult(
        id,
        []() {  // success — silent
        },
        [](int code, QString message) {
            qCWarning(lcHw()) << "PhantomWakeSuppressor force-back failed:" << code << message;
        });
    // Timer is no longer active; let consumers know the armed state cleared.
    emit armedChanged();
}

}  // namespace hw
}  // namespace uc
