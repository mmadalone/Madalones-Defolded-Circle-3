// Copyright (c) 2026 madalone. ScreenOffSync — keeps the screensaver's screen-off animation in step with the core's
// display switch-off: when the dim phase started, how long it lasts at least, and switching the display off the moment
// the animation has ended instead of leaving a black but still lit screen until the core does it.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>

#include "../core/core.h"
#include "../system/power.h"

namespace uc {
namespace hw {

/// The core's schedule after the last activity, with T = Config.displayTimeout (display_off_sec), measured in the core
/// simulator for T = 5..60 s and on a remote at T = 30 s (2026-11-02, core 0.83.1): Normal -> Idle (display dimmed)
/// after max(T/2, 3) + 1 s, Idle -> Low_power (display off) after T + 1 or T + 2 s. The core ticks in whole seconds,
/// so the dim phase lasts T - max(T/2, 3) s, or one second more, and which one varies from cycle to cycle.
class ScreenOffSync : public QObject {
    Q_OBJECT

 public:
    /// The animation ends this long before the earliest switch-off (about 4x the measured jitter).
    static constexpr int END_MARGIN_MS = 250;

    /// Shortest dim phase for a display-off timeout of @p displayOffSec seconds (0 when the display never goes off).
    static int dimPhaseMinMsFor(int displayOffSec);

    /// Starts in Normal: Power reports the core's actual mode after the connection, through powerModeChanged.
    explicit ScreenOffSync(core::Api *core, QObject *parent = nullptr);
    ~ScreenOffSync() override;

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

    /// The display-off timeout in seconds (Config.displayTimeout, pushed by main.cpp).
    void setDisplayOffSec(int seconds) { m_displayOffSec = seconds; }
    int  displayOffSec() const { return m_displayOffSec; }

    /// Milliseconds since the core entered Idle, or -1 when it is not in Idle.
    Q_INVOKABLE int msSinceIdle() const;
    /// Shortest dim phase for the current display-off timeout.
    Q_INVOKABLE int dimPhaseMinMs() const;
    /// Delay from now that makes an animation of @p leadMs end END_MARGIN_MS before the earliest switch-off
    /// (0 when that moment has passed, -1 when the core is not in Idle).
    Q_INVOKABLE int startDelayMs(int leadMs) const;
    /// The screen-off animation starts now: input from here on vetoes requestDisplayOff().
    Q_INVOKABLE void markEffectStart();
    /// The animation has ended: switch the display off (set_power_mode LOW_POWER, the call the phantom-wake suppressor
    /// sends) if the core is still in Idle and nobody touched the remote since markEffectStart(). Returns whether the
    /// request was sent.
    Q_INVOKABLE bool requestDisplayOff();

 public slots:
    void onPowerModeChanged(uc::hw::Power::PowerMode fromPowerMode, uc::hw::Power::PowerMode toPowerMode);
    /// InputController::keyPressed / touchDetected, TouchSlider::touchPressed, EntityController::entityCommandIssued.
    void onUserInput();

 private:
    static ScreenOffSync *s_instance;

    core::Api *m_core;

    int              m_displayOffSec = 0;
    Power::PowerMode m_mode = Power::Normal;
    QElapsedTimer    m_idleTimer;            // valid while the core is in Idle (monotonic)
    quint64          m_inputSerial = 0;      // counts user input
    quint64          m_effectInputSerial = 0;
    bool             m_effectMarked = false;
    bool             m_requested = false;    // a request was sent during this dim phase
    qint64           m_requestedAtMs = -1;   // ms into the dim phase at the request

#ifdef UC_TEST_BUILD
    friend class ScreenOffSyncTest;
#endif
};

}  // namespace hw
}  // namespace uc
