// Copyright (c) 2026 madalone. PhantomWakeSuppressor (Mod 6, v1.4.20) — forces the device
// back to LOW_POWER when a LOW_POWER/SUSPEND -> NORMAL transition happens with no user
// input within a grace window. Inverse-symmetric to ActivitySessionKeeper (Mod 5).
// Kills phantom-WoWLAN-wake battery drain (~9%/hr observed) without sacrificing
// fast wake-press for real user wakes.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>

#include "../core/core.h"
#include "../system/power.h"

namespace uc {
namespace hw {

class PhantomWakeSuppressor : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool enabled         READ getEnabled         WRITE setEnabled         NOTIFY enabledChanged)
    Q_PROPERTY(int  graceMs         READ getGraceMs         WRITE setGraceMs         NOTIFY graceMsChanged)
    Q_PROPERTY(int  inputLookbackMs READ getInputLookbackMs WRITE setInputLookbackMs NOTIFY inputLookbackMsChanged)
    Q_PROPERTY(bool armed           READ getArmed                                    NOTIFY armedChanged)

 public:
    explicit PhantomWakeSuppressor(core::Api* core, QObject* parent = nullptr);
    ~PhantomWakeSuppressor();

    bool getEnabled() const         { return m_enabled; }
    int  getGraceMs() const         { return m_graceMs; }
    int  getInputLookbackMs() const { return m_inputLookbackMs; }
    bool getArmed() const           { return m_graceTimer.isActive(); }

    void setEnabled(bool enabled);
    void setGraceMs(int ms);
    void setInputLookbackMs(int ms);

    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 public slots:
    // Wake detection — connects to Power::powerModeChanged(from, to).
    void onPowerModeChanged(uc::hw::Power::PowerMode fromPowerMode, uc::hw::Power::PowerMode toPowerMode);
    // Single multi-source slot for InputController::keyPressed (QString arg auto-discarded),
    // InputController::touchDetected, TouchSlider::touchPressed.
    void onUserInput();
    // EntityController::entityCommandIssued — every entity command counts as user input
    // (v1.5.5; entity commands only come from user actions).
    // v1.4.22: added for the press that wakes the device. On firmware 2.9.1 one such press, out
    // of standby (PLAY, 2026-05-01), went straight to the integrations and did not reach
    // InputController::keyPressed in our UI process: it surfaced only at the entity-command
    // layer, and without this slot Mod 6 would have forced it back as a phantom wake. On firmware
    // 2.12.1 the journal shows activity 3-53 ms before the wake in 48 of 165 wakes; test build
    // 1.6.4-keys1 did not log of which kind. Since v1.6.5 the "skipping grace" line names the kind.
    void onEntityCommandIssued(QString entityId, QString command);
    // Battery::powerSupplyChanged — dock/undock counts as user activity.
    void onPowerSupplyChanged(bool onAc);
    // madalone (v1.6.5): UndockCheck::closeRequested — the quick undock check found the remote off its charger for
    // the Undock delay (GitHub issue #3): a lift, or a cut of the charging that long. The same fact as the charger
    // report above, known sooner (up to about 4.5 s at the default delay), and it counts as activity in the same way.
    // On a dark pick-up it keeps the display on only when the close comes before the grace window runs out: about
    // 0.6-0.85 s after the wake at the default Undock delay (the grace has to be about 0.4 s longer than the delay),
    // so not within the default 500 ms: the display then goes off and the screensaver closes on the core's report.
    void onQuickUndock();
    // core::Api::disconnected — defensive cleanup if connection drops mid-grace.
    void onCoreDisconnected();

 signals:
    void enabledChanged();
    void graceMsChanged();
    void inputLookbackMsChanged();
    void armedChanged();

 private:
    // madalone (v1.6.5): every kind of activity goes through here. It remembers when the activity came and what
    // it was (for the lookback and its log line) and cancels a running grace with that as the reason.
    void noteActivity(const char* what);
    void cancelGrace(const char* reason);
    void forceLowPower();

    static PhantomWakeSuppressor* s_instance;
    core::Api*                    m_core;

    QTimer        m_graceTimer;       // single-shot, fires forceLowPower if grace expires without user input
    QElapsedTimer m_lastInputTimer;   // v1.4.23: tracks "last user input" timestamp for skip-grace logic
    const char*   m_lastInputKind = "input";  // v1.6.5: what that input was (a string literal), for the skip line

    bool m_enabled         = false;
    int  m_graceMs         = 500;
    int  m_inputLookbackMs = 500;     // v1.4.23: skip arming grace if user input arrived within this many ms before wake
};

}  // namespace hw
}  // namespace uc
