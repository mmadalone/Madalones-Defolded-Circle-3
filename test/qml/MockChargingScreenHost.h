// Copyright (c) 2026 madalone. Stand-ins for the singletons and the C++ item that the real ChargingScreen.qml (the
// screensaver popup) imports, so a QML test can load it: Power (+ Power.Modes), ScreenOffSync, SoundEffects and a
// MatrixRain stub. Registered in tst_qml_main.cpp under the production URIs.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQuickItem>
#include <QStringList>

// src/system/power.h: the power-mode enum (registered as Power.Modes / PowerModes) and the change signal
class MockPower : public QObject {
    Q_OBJECT

 public:
    enum PowerMode { Normal, Idle, Low_power, Suspend };
    Q_ENUM(PowerMode)

    explicit MockPower(QObject *parent = nullptr) : QObject(parent) {}

 signals:
    void powerModeChanged(int fromPowerMode, int toPowerMode);
};

// src/hardware/screenOffSync.h, outside the dim phase: startDelayMs() returns -1, so the screensaver never schedules
// a screen-off animation in a test
class MockScreenOffSync : public QObject {
    Q_OBJECT

 public:
    explicit MockScreenOffSync(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE int msSinceIdle() const { return -1; }
    Q_INVOKABLE int dimPhaseMinMs() const { return 0; }
    Q_INVOKABLE int startDelayMs(int) const { return -1; }
    Q_INVOKABLE int fitLeadMs(int leadMs) const { return leadMs; }
    Q_INVOKABLE void markEffectStart() {}
    Q_INVOKABLE bool requestDisplayOff() { return false; }
};

// src/ui/soundEffects.h: the effects the screensaver plays (the dock chime); play() does nothing
class MockSoundEffects : public QObject {
    Q_OBJECT

 public:
    enum SoundEffect { Click, ClickLow, Confirm, Error, BatteryCharge };
    Q_ENUM(SoundEffect)

    explicit MockSoundEffects(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void play(int) {}
};

// src/ui/matrixrain.h stand-in: draws nothing and records what ChargingScreen.qml and MatrixTheme.qml call. The real
// item takes its speed from ScreensaverConfig in C++ (src/ui/matrixrain/bindinghelper.cpp), not from QML, so a test
// reads ScreensaverConfig.matrixSpeed.
class StubMatrixRain : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(bool gravityMode MEMBER m_gravityMode NOTIFY gravityModeChanged)
    Q_PROPERTY(bool displayOff MEMBER m_displayOff NOTIFY displayOffChanged)
    Q_PROPERTY(bool running MEMBER m_running NOTIFY runningChanged)
    Q_PROPERTY(QString lastBuildSummary READ lastBuildSummary CONSTANT)
    Q_PROPERTY(QStringList calls READ calls NOTIFY callsChanged)

 public:
    explicit StubMatrixRain(QQuickItem *parent = nullptr) : QQuickItem(parent) {}

    QString     lastBuildSummary() const { return QString(); }
    QStringList calls() const { return m_calls; }

    Q_INVOKABLE void interactiveInput(const QString &action) { record(QStringLiteral("input:") + action); }
    Q_INVOKABLE void enterPressed() { record(QStringLiteral("enterPressed")); }
    Q_INVOKABLE void enterReleased() { record(QStringLiteral("enterReleased")); }
    Q_INVOKABLE void resetEnterState() {}
    Q_INVOKABLE void pauseTicks() { record(QStringLiteral("pauseTicks")); }
    Q_INVOKABLE void resetAfterScreenOff() {}

 signals:
    void gravityModeChanged();
    void displayOffChanged();
    void runningChanged();
    void callsChanged();
    void enterAction(QString action);

 private:
    void record(const QString &call) {
        m_calls << call;
        emit callsChanged();
    }

    bool        m_gravityMode = false;
    bool        m_displayOff = false;
    bool        m_running = false;
    QStringList m_calls;
};
