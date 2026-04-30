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
    // Clamp to [100, 2000] — matches the Power.qml slider range.
    const int clamped = qBound(100, ms, 2000);
    if (m_graceMs == clamped) return;
    m_graceMs = clamped;
    emit graceMsChanged();
    // If a grace window is currently active, restart with the new interval so the
    // user-visible effect of changing the slider is immediate.
    if (m_graceTimer.isActive()) {
        m_graceTimer.start(m_graceMs);
    }
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
        // Wake event detected — arm the grace window. If the user presses something
        // within m_graceMs, onUserInput() cancels it. Otherwise forceLowPower() fires.
        m_graceTimer.start(m_graceMs);
        emit armedChanged();
        qCDebug(lcHw()) << "PhantomWakeSuppressor armed:" << m_graceMs << "ms";
    } else if (toPowerMode != Power::PowerMode::Normal && m_graceTimer.isActive()) {
        // Defensive: firmware itself transitioned away from NORMAL during our grace.
        // Don't double-fire force-back; let the firmware's own transition stand.
        cancelGrace("powerMode left NORMAL during grace");
    }
}

void PhantomWakeSuppressor::onUserInput() {
    if (!m_graceTimer.isActive()) return;
    cancelGrace("user input");
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
    qCDebug(lcHw()) << "PhantomWakeSuppressor cancelled:" << reason;
}

void PhantomWakeSuppressor::forceLowPower() {
    qCInfo(lcHw()) << "PhantomWakeSuppressor: phantom wake -> forced LOW_POWER";
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
