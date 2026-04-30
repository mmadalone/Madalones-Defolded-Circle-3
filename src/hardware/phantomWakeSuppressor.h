// Copyright (c) 2026 madalone. PhantomWakeSuppressor (Mod 6, v1.4.20) — forces the device
// back to LOW_POWER when a LOW_POWER/SUSPEND -> NORMAL transition happens with no user
// input within a grace window. Inverse-symmetric to ActivitySessionKeeper (Mod 5).
// Kills phantom-WoWLAN-wake battery drain (~9%/hr observed) without sacrificing
// fast wake-press for real user wakes.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>

#include "../core/core.h"
#include "power.h"

namespace uc {
namespace hw {

class PhantomWakeSuppressor : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool enabled READ getEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int  graceMs READ getGraceMs WRITE setGraceMs NOTIFY graceMsChanged)
    Q_PROPERTY(bool armed   READ getArmed                    NOTIFY armedChanged)

 public:
    explicit PhantomWakeSuppressor(core::Api* core, QObject* parent = nullptr);
    ~PhantomWakeSuppressor();

    bool getEnabled() const { return m_enabled; }
    int  getGraceMs() const { return m_graceMs; }
    bool getArmed() const   { return m_graceTimer.isActive(); }

    void setEnabled(bool enabled);
    void setGraceMs(int ms);

    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 public slots:
    // Wake detection — connects to Power::powerModeChanged(from, to).
    void onPowerModeChanged(uc::hw::Power::PowerMode fromPowerMode, uc::hw::Power::PowerMode toPowerMode);
    // Single multi-source slot for InputController::keyPressed (QString arg auto-discarded),
    // InputController::touchDetected, TouchSlider::touchPressed.
    void onUserInput();
    // Battery::powerSupplyChanged — dock/undock counts as user activity.
    void onPowerSupplyChanged(bool onAc);
    // core::Api::disconnected — defensive cleanup if connection drops mid-grace.
    void onCoreDisconnected();

 signals:
    void enabledChanged();
    void graceMsChanged();
    void armedChanged();

 private:
    void cancelGrace(const char* reason);
    void forceLowPower();

    static PhantomWakeSuppressor* s_instance;
    core::Api*                    m_core;

    QTimer m_graceTimer;   // single-shot, fires forceLowPower if grace expires without user input

    bool m_enabled = false;
    int  m_graceMs = 500;
};

}  // namespace hw
}  // namespace uc
