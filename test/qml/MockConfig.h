// Copyright (c) 2024 madalone. Mock Config singleton for QML tests.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>

class MockConfig : public QObject {
    Q_OBJECT

    // Charging screen properties — names match config.h Q_PROPERTY declarations exactly
    Q_PROPERTY(QString chargingTheme READ chargingTheme WRITE setChargingTheme NOTIFY chargingThemeChanged)
    Q_PROPERTY(bool chargingShowClock READ chargingShowClock WRITE setChargingShowClock NOTIFY chargingShowClockChanged)
    Q_PROPERTY(bool chargingShowBattery READ chargingShowBattery WRITE setChargingShowBattery NOTIFY chargingShowBatteryChanged)
    Q_PROPERTY(QString chargingMatrixColor READ chargingMatrixColor WRITE setChargingMatrixColor NOTIFY chargingMatrixColorChanged)
    Q_PROPERTY(int chargingMatrixSpeed READ chargingMatrixSpeed WRITE setChargingMatrixSpeed NOTIFY chargingMatrixSpeedChanged)
    Q_PROPERTY(int chargingMatrixDensity READ chargingMatrixDensity WRITE setChargingMatrixDensity NOTIFY chargingMatrixDensityChanged)
    Q_PROPERTY(QString chargingMatrixColorMode READ chargingMatrixColorMode WRITE setChargingMatrixColorMode NOTIFY chargingMatrixColorModeChanged)
    Q_PROPERTY(int chargingMatrixTrail READ chargingMatrixTrail WRITE setChargingMatrixTrail NOTIFY chargingMatrixTrailChanged)
    Q_PROPERTY(int chargingMatrixFontSize READ chargingMatrixFontSize WRITE setChargingMatrixFontSize NOTIFY chargingMatrixFontSizeChanged)
    Q_PROPERTY(QString chargingMatrixCharset READ chargingMatrixCharset WRITE setChargingMatrixCharset NOTIFY chargingMatrixCharsetChanged)
    Q_PROPERTY(bool chargingMatrixGlow READ chargingMatrixGlow WRITE setChargingMatrixGlow NOTIFY chargingMatrixGlowChanged)
    Q_PROPERTY(bool chargingMatrixGlitch READ chargingMatrixGlitch WRITE setChargingMatrixGlitch NOTIFY chargingMatrixGlitchChanged)
    Q_PROPERTY(int chargingMatrixGlitchRate READ chargingMatrixGlitchRate WRITE setChargingMatrixGlitchRate NOTIFY chargingMatrixGlitchRateChanged)
    Q_PROPERTY(bool chargingMatrixGlitchFlash READ chargingMatrixGlitchFlash WRITE setChargingMatrixGlitchFlash NOTIFY chargingMatrixGlitchFlashChanged)
    Q_PROPERTY(bool chargingMatrixGlitchStutter READ chargingMatrixGlitchStutter WRITE setChargingMatrixGlitchStutter NOTIFY chargingMatrixGlitchStutterChanged)
    Q_PROPERTY(bool chargingMatrixGlitchReverse READ chargingMatrixGlitchReverse WRITE setChargingMatrixGlitchReverse NOTIFY chargingMatrixGlitchReverseChanged)
    Q_PROPERTY(bool chargingMatrixGlitchDirection READ chargingMatrixGlitchDirection WRITE setChargingMatrixGlitchDirection NOTIFY chargingMatrixGlitchDirectionChanged)
    Q_PROPERTY(int chargingMatrixGlitchDirRate READ chargingMatrixGlitchDirRate WRITE setChargingMatrixGlitchDirRate NOTIFY chargingMatrixGlitchDirRateChanged)
    Q_PROPERTY(bool chargingMatrixGlitchDirCardinal READ chargingMatrixGlitchDirCardinal WRITE setChargingMatrixGlitchDirCardinal NOTIFY chargingMatrixGlitchDirCardinalChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaos READ chargingMatrixGlitchChaos WRITE setChargingMatrixGlitchChaos NOTIFY chargingMatrixGlitchChaosChanged)
    Q_PROPERTY(int chargingMatrixGlitchChaosFrequency READ chargingMatrixGlitchChaosFrequency WRITE setChargingMatrixGlitchChaosFrequency NOTIFY chargingMatrixGlitchChaosFrequencyChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosSurge READ chargingMatrixGlitchChaosSurge WRITE setChargingMatrixGlitchChaosSurge NOTIFY chargingMatrixGlitchChaosSurgeChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosScramble READ chargingMatrixGlitchChaosScramble WRITE setChargingMatrixGlitchChaosScramble NOTIFY chargingMatrixGlitchChaosScrambleChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosFreeze READ chargingMatrixGlitchChaosFreeze WRITE setChargingMatrixGlitchChaosFreeze NOTIFY chargingMatrixGlitchChaosFreezeChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosScatter READ chargingMatrixGlitchChaosScatter WRITE setChargingMatrixGlitchChaosScatter NOTIFY chargingMatrixGlitchChaosScatterChanged)
    Q_PROPERTY(int chargingMatrixGlitchDirLength READ chargingMatrixGlitchDirLength WRITE setChargingMatrixGlitchDirLength NOTIFY chargingMatrixGlitchDirLengthChanged)
    Q_PROPERTY(bool chargingMatrixGlitchRandomColor READ chargingMatrixGlitchRandomColor WRITE setChargingMatrixGlitchRandomColor NOTIFY chargingMatrixGlitchRandomColorChanged)
    Q_PROPERTY(int chargingMatrixGlitchChaosIntensity READ chargingMatrixGlitchChaosIntensity WRITE setChargingMatrixGlitchChaosIntensity NOTIFY chargingMatrixGlitchChaosIntensityChanged)
    Q_PROPERTY(int chargingMatrixGlitchChaosScatterRate READ chargingMatrixGlitchChaosScatterRate WRITE setChargingMatrixGlitchChaosScatterRate NOTIFY chargingMatrixGlitchChaosScatterRateChanged)
    Q_PROPERTY(int chargingMatrixGlitchChaosScatterLength READ chargingMatrixGlitchChaosScatterLength WRITE setChargingMatrixGlitchChaosScatterLength NOTIFY chargingMatrixGlitchChaosScatterLengthChanged)
    Q_PROPERTY(int chargingMatrixFade READ chargingMatrixFade WRITE setChargingMatrixFade NOTIFY chargingMatrixFadeChanged)
    Q_PROPERTY(QString chargingMatrixDirection READ chargingMatrixDirection WRITE setChargingMatrixDirection NOTIFY chargingMatrixDirectionChanged)
    Q_PROPERTY(bool chargingMatrixInvertTrail READ chargingMatrixInvertTrail WRITE setChargingMatrixInvertTrail NOTIFY chargingMatrixInvertTrailChanged)
    Q_PROPERTY(QString chargingMatrixMessages READ chargingMatrixMessages WRITE setChargingMatrixMessages NOTIFY chargingMatrixMessagesChanged)
    Q_PROPERTY(int chargingMatrixMessageInterval READ chargingMatrixMessageInterval WRITE setChargingMatrixMessageInterval NOTIFY chargingMatrixMessageIntervalChanged)
    Q_PROPERTY(bool chargingMatrixMessageRandom READ chargingMatrixMessageRandom WRITE setChargingMatrixMessageRandom NOTIFY chargingMatrixMessageRandomChanged)
    Q_PROPERTY(QString chargingMatrixMessageDirection READ chargingMatrixMessageDirection WRITE setChargingMatrixMessageDirection NOTIFY chargingMatrixMessageDirectionChanged)
    Q_PROPERTY(bool chargingMatrixMessageFlash READ chargingMatrixMessageFlash WRITE setChargingMatrixMessageFlash NOTIFY chargingMatrixMessageFlashChanged)
    Q_PROPERTY(bool chargingMatrixMessagePulse READ chargingMatrixMessagePulse WRITE setChargingMatrixMessagePulse NOTIFY chargingMatrixMessagePulseChanged)
    Q_PROPERTY(bool chargingMatrixTapRandomize READ chargingMatrixTapRandomize WRITE setChargingMatrixTapRandomize NOTIFY chargingMatrixTapRandomizeChanged)
    Q_PROPERTY(int chargingMatrixTapRandomizeChance READ chargingMatrixTapRandomizeChance WRITE setChargingMatrixTapRandomizeChance NOTIFY chargingMatrixTapRandomizeChanceChanged)
    Q_PROPERTY(bool chargingMatrixSubliminal READ chargingMatrixSubliminal WRITE setChargingMatrixSubliminal NOTIFY chargingMatrixSubliminalChanged)
    Q_PROPERTY(int chargingMatrixSubliminalInterval READ chargingMatrixSubliminalInterval WRITE setChargingMatrixSubliminalInterval NOTIFY chargingMatrixSubliminalIntervalChanged)
    Q_PROPERTY(int chargingMatrixSubliminalDuration READ chargingMatrixSubliminalDuration WRITE setChargingMatrixSubliminalDuration NOTIFY chargingMatrixSubliminalDurationChanged)
    Q_PROPERTY(bool chargingMatrixSubliminalStream READ chargingMatrixSubliminalStream WRITE setChargingMatrixSubliminalStream NOTIFY chargingMatrixSubliminalStreamChanged)
    Q_PROPERTY(bool chargingMatrixSubliminalOverlay READ chargingMatrixSubliminalOverlay WRITE setChargingMatrixSubliminalOverlay NOTIFY chargingMatrixSubliminalOverlayChanged)
    Q_PROPERTY(bool chargingMatrixSubliminalFlash READ chargingMatrixSubliminalFlash WRITE setChargingMatrixSubliminalFlash NOTIFY chargingMatrixSubliminalFlashChanged)
    Q_PROPERTY(bool chargingTapToClose READ chargingTapToClose WRITE setChargingTapToClose NOTIFY chargingTapToCloseChanged)
    Q_PROPERTY(bool chargingIdleEnabled READ chargingIdleEnabled WRITE setChargingIdleEnabled NOTIFY chargingIdleEnabledChanged)
    Q_PROPERTY(int chargingIdleTimeout READ chargingIdleTimeout WRITE setChargingIdleTimeout NOTIFY chargingIdleTimeoutChanged)
    Q_PROPERTY(bool chargingMotionToClose READ chargingMotionToClose WRITE setChargingMotionToClose NOTIFY chargingMotionToCloseChanged)
    Q_PROPERTY(bool chargingBatteryDockedOnly READ chargingBatteryDockedOnly WRITE setChargingBatteryDockedOnly NOTIFY chargingBatteryDockedOnlyChanged)

 public:
    explicit MockConfig(QObject *parent = nullptr) : QObject(parent) {}

    // --- Getters ---
    QString chargingTheme() const { return m_chargingTheme; }
    bool chargingShowClock() const { return m_chargingShowClock; }
    bool chargingShowBattery() const { return m_chargingShowBattery; }
    QString chargingMatrixColor() const { return m_chargingMatrixColor; }
    int chargingMatrixSpeed() const { return m_chargingMatrixSpeed; }
    int chargingMatrixDensity() const { return m_chargingMatrixDensity; }
    QString chargingMatrixColorMode() const { return m_chargingMatrixColorMode; }
    int chargingMatrixTrail() const { return m_chargingMatrixTrail; }
    int chargingMatrixFontSize() const { return m_chargingMatrixFontSize; }
    QString chargingMatrixCharset() const { return m_chargingMatrixCharset; }
    bool chargingMatrixGlow() const { return m_chargingMatrixGlow; }
    bool chargingMatrixGlitch() const { return m_chargingMatrixGlitch; }
    int chargingMatrixGlitchRate() const { return m_chargingMatrixGlitchRate; }
    bool chargingMatrixGlitchFlash() const { return m_chargingMatrixGlitchFlash; }
    bool chargingMatrixGlitchStutter() const { return m_chargingMatrixGlitchStutter; }
    bool chargingMatrixGlitchReverse() const { return m_chargingMatrixGlitchReverse; }
    bool chargingMatrixGlitchDirection() const { return m_chargingMatrixGlitchDirection; }
    int chargingMatrixGlitchDirRate() const { return m_chargingMatrixGlitchDirRate; }
    bool chargingMatrixGlitchDirCardinal() const { return m_chargingMatrixGlitchDirCardinal; }
    int chargingMatrixGlitchDirLength() const { return m_chargingMatrixGlitchDirLength; }
    bool chargingMatrixGlitchRandomColor() const { return m_chargingMatrixGlitchRandomColor; }
    bool chargingMatrixGlitchChaos() const { return m_chargingMatrixGlitchChaos; }
    int chargingMatrixGlitchChaosFrequency() const { return m_chargingMatrixGlitchChaosFrequency; }
    bool chargingMatrixGlitchChaosSurge() const { return m_chargingMatrixGlitchChaosSurge; }
    bool chargingMatrixGlitchChaosScramble() const { return m_chargingMatrixGlitchChaosScramble; }
    bool chargingMatrixGlitchChaosFreeze() const { return m_chargingMatrixGlitchChaosFreeze; }
    bool chargingMatrixGlitchChaosScatter() const { return m_chargingMatrixGlitchChaosScatter; }
    int chargingMatrixGlitchChaosIntensity() const { return m_chargingMatrixGlitchChaosIntensity; }
    int chargingMatrixGlitchChaosScatterRate() const { return m_chargingMatrixGlitchChaosScatterRate; }
    int chargingMatrixGlitchChaosScatterLength() const { return m_chargingMatrixGlitchChaosScatterLength; }
    int chargingMatrixFade() const { return m_chargingMatrixFade; }
    QString chargingMatrixDirection() const { return m_chargingMatrixDirection; }
    bool chargingMatrixInvertTrail() const { return m_chargingMatrixInvertTrail; }
    QString chargingMatrixMessages() const { return m_chargingMatrixMessages; }
    int chargingMatrixMessageInterval() const { return m_chargingMatrixMessageInterval; }
    bool chargingMatrixMessageRandom() const { return m_chargingMatrixMessageRandom; }
    QString chargingMatrixMessageDirection() const { return m_chargingMatrixMessageDirection; }
    bool chargingMatrixMessageFlash() const { return m_chargingMatrixMessageFlash; }
    bool chargingMatrixMessagePulse() const { return m_chargingMatrixMessagePulse; }
    bool chargingMatrixTapRandomize() const { return m_chargingMatrixTapRandomize; }
    int chargingMatrixTapRandomizeChance() const { return m_chargingMatrixTapRandomizeChance; }
    bool chargingMatrixSubliminal() const { return m_chargingMatrixSubliminal; }
    int chargingMatrixSubliminalInterval() const { return m_chargingMatrixSubliminalInterval; }
    int chargingMatrixSubliminalDuration() const { return m_chargingMatrixSubliminalDuration; }
    bool chargingMatrixSubliminalStream() const { return m_chargingMatrixSubliminalStream; }
    bool chargingMatrixSubliminalOverlay() const { return m_chargingMatrixSubliminalOverlay; }
    bool chargingMatrixSubliminalFlash() const { return m_chargingMatrixSubliminalFlash; }
    bool chargingTapToClose() const { return m_chargingTapToClose; }
    bool chargingIdleEnabled() const { return m_chargingIdleEnabled; }
    int chargingIdleTimeout() const { return m_chargingIdleTimeout; }
    bool chargingMotionToClose() const { return m_chargingMotionToClose; }
    bool chargingBatteryDockedOnly() const { return m_chargingBatteryDockedOnly; }

    // --- Setters (emit signal only when value changes) ---
    void setChargingTheme(const QString &v) { if (m_chargingTheme != v) { m_chargingTheme = v; emit chargingThemeChanged(); } }
    void setChargingShowClock(bool v) { if (m_chargingShowClock != v) { m_chargingShowClock = v; emit chargingShowClockChanged(); } }
    void setChargingShowBattery(bool v) { if (m_chargingShowBattery != v) { m_chargingShowBattery = v; emit chargingShowBatteryChanged(); } }
    void setChargingMatrixColor(const QString &v) { if (m_chargingMatrixColor != v) { m_chargingMatrixColor = v; emit chargingMatrixColorChanged(); } }
    void setChargingMatrixSpeed(int v) { if (m_chargingMatrixSpeed != v) { m_chargingMatrixSpeed = v; emit chargingMatrixSpeedChanged(); } }
    void setChargingMatrixDensity(int v) { if (m_chargingMatrixDensity != v) { m_chargingMatrixDensity = v; emit chargingMatrixDensityChanged(); } }
    void setChargingMatrixColorMode(const QString &v) { if (m_chargingMatrixColorMode != v) { m_chargingMatrixColorMode = v; emit chargingMatrixColorModeChanged(); } }
    void setChargingMatrixTrail(int v) { if (m_chargingMatrixTrail != v) { m_chargingMatrixTrail = v; emit chargingMatrixTrailChanged(); } }
    void setChargingMatrixFontSize(int v) { if (m_chargingMatrixFontSize != v) { m_chargingMatrixFontSize = v; emit chargingMatrixFontSizeChanged(); } }
    void setChargingMatrixCharset(const QString &v) { if (m_chargingMatrixCharset != v) { m_chargingMatrixCharset = v; emit chargingMatrixCharsetChanged(); } }
    void setChargingMatrixGlow(bool v) { if (m_chargingMatrixGlow != v) { m_chargingMatrixGlow = v; emit chargingMatrixGlowChanged(); } }
    void setChargingMatrixGlitch(bool v) { if (m_chargingMatrixGlitch != v) { m_chargingMatrixGlitch = v; emit chargingMatrixGlitchChanged(); } }
    void setChargingMatrixGlitchRate(int v) { if (m_chargingMatrixGlitchRate != v) { m_chargingMatrixGlitchRate = v; emit chargingMatrixGlitchRateChanged(); } }
    void setChargingMatrixGlitchFlash(bool v) { if (m_chargingMatrixGlitchFlash != v) { m_chargingMatrixGlitchFlash = v; emit chargingMatrixGlitchFlashChanged(); } }
    void setChargingMatrixGlitchStutter(bool v) { if (m_chargingMatrixGlitchStutter != v) { m_chargingMatrixGlitchStutter = v; emit chargingMatrixGlitchStutterChanged(); } }
    void setChargingMatrixGlitchReverse(bool v) { if (m_chargingMatrixGlitchReverse != v) { m_chargingMatrixGlitchReverse = v; emit chargingMatrixGlitchReverseChanged(); } }
    void setChargingMatrixGlitchDirection(bool v) { if (m_chargingMatrixGlitchDirection != v) { m_chargingMatrixGlitchDirection = v; emit chargingMatrixGlitchDirectionChanged(); } }
    void setChargingMatrixGlitchDirRate(int v) { if (m_chargingMatrixGlitchDirRate != v) { m_chargingMatrixGlitchDirRate = v; emit chargingMatrixGlitchDirRateChanged(); } }
    void setChargingMatrixGlitchDirCardinal(bool v) { if (m_chargingMatrixGlitchDirCardinal != v) { m_chargingMatrixGlitchDirCardinal = v; emit chargingMatrixGlitchDirCardinalChanged(); } }
    void setChargingMatrixGlitchDirLength(int v) { if (m_chargingMatrixGlitchDirLength != v) { m_chargingMatrixGlitchDirLength = v; emit chargingMatrixGlitchDirLengthChanged(); } }
    void setChargingMatrixGlitchRandomColor(bool v) { if (m_chargingMatrixGlitchRandomColor != v) { m_chargingMatrixGlitchRandomColor = v; emit chargingMatrixGlitchRandomColorChanged(); } }
    void setChargingMatrixGlitchChaos(bool v) { if (m_chargingMatrixGlitchChaos != v) { m_chargingMatrixGlitchChaos = v; emit chargingMatrixGlitchChaosChanged(); } }
    void setChargingMatrixGlitchChaosFrequency(int v) { if (m_chargingMatrixGlitchChaosFrequency != v) { m_chargingMatrixGlitchChaosFrequency = v; emit chargingMatrixGlitchChaosFrequencyChanged(); } }
    void setChargingMatrixGlitchChaosSurge(bool v) { if (m_chargingMatrixGlitchChaosSurge != v) { m_chargingMatrixGlitchChaosSurge = v; emit chargingMatrixGlitchChaosSurgeChanged(); } }
    void setChargingMatrixGlitchChaosScramble(bool v) { if (m_chargingMatrixGlitchChaosScramble != v) { m_chargingMatrixGlitchChaosScramble = v; emit chargingMatrixGlitchChaosScrambleChanged(); } }
    void setChargingMatrixGlitchChaosFreeze(bool v) { if (m_chargingMatrixGlitchChaosFreeze != v) { m_chargingMatrixGlitchChaosFreeze = v; emit chargingMatrixGlitchChaosFreezeChanged(); } }
    void setChargingMatrixGlitchChaosScatter(bool v) { if (m_chargingMatrixGlitchChaosScatter != v) { m_chargingMatrixGlitchChaosScatter = v; emit chargingMatrixGlitchChaosScatterChanged(); } }
    void setChargingMatrixGlitchChaosIntensity(int v) { if (m_chargingMatrixGlitchChaosIntensity != v) { m_chargingMatrixGlitchChaosIntensity = v; emit chargingMatrixGlitchChaosIntensityChanged(); } }
    void setChargingMatrixGlitchChaosScatterRate(int v) { if (m_chargingMatrixGlitchChaosScatterRate != v) { m_chargingMatrixGlitchChaosScatterRate = v; emit chargingMatrixGlitchChaosScatterRateChanged(); } }
    void setChargingMatrixGlitchChaosScatterLength(int v) { if (m_chargingMatrixGlitchChaosScatterLength != v) { m_chargingMatrixGlitchChaosScatterLength = v; emit chargingMatrixGlitchChaosScatterLengthChanged(); } }
    void setChargingMatrixFade(int v) { if (m_chargingMatrixFade != v) { m_chargingMatrixFade = v; emit chargingMatrixFadeChanged(); } }
    void setChargingMatrixDirection(const QString &v) { if (m_chargingMatrixDirection != v) { m_chargingMatrixDirection = v; emit chargingMatrixDirectionChanged(); } }
    void setChargingMatrixInvertTrail(bool v) { if (m_chargingMatrixInvertTrail != v) { m_chargingMatrixInvertTrail = v; emit chargingMatrixInvertTrailChanged(); } }
    void setChargingMatrixMessages(const QString &v) { if (m_chargingMatrixMessages != v) { m_chargingMatrixMessages = v; emit chargingMatrixMessagesChanged(); } }
    void setChargingMatrixMessageInterval(int v) { if (m_chargingMatrixMessageInterval != v) { m_chargingMatrixMessageInterval = v; emit chargingMatrixMessageIntervalChanged(); } }
    void setChargingMatrixMessageRandom(bool v) { if (m_chargingMatrixMessageRandom != v) { m_chargingMatrixMessageRandom = v; emit chargingMatrixMessageRandomChanged(); } }
    void setChargingMatrixMessageDirection(const QString &v) { if (m_chargingMatrixMessageDirection != v) { m_chargingMatrixMessageDirection = v; emit chargingMatrixMessageDirectionChanged(); } }
    void setChargingMatrixMessageFlash(bool v) { if (m_chargingMatrixMessageFlash != v) { m_chargingMatrixMessageFlash = v; emit chargingMatrixMessageFlashChanged(); } }
    void setChargingMatrixMessagePulse(bool v) { if (m_chargingMatrixMessagePulse != v) { m_chargingMatrixMessagePulse = v; emit chargingMatrixMessagePulseChanged(); } }
    void setChargingMatrixTapRandomize(bool v) { if (m_chargingMatrixTapRandomize != v) { m_chargingMatrixTapRandomize = v; emit chargingMatrixTapRandomizeChanged(); } }
    void setChargingMatrixTapRandomizeChance(int v) { if (m_chargingMatrixTapRandomizeChance != v) { m_chargingMatrixTapRandomizeChance = v; emit chargingMatrixTapRandomizeChanceChanged(); } }
    void setChargingMatrixSubliminal(bool v) { if (m_chargingMatrixSubliminal != v) { m_chargingMatrixSubliminal = v; emit chargingMatrixSubliminalChanged(); } }
    void setChargingMatrixSubliminalInterval(int v) { if (m_chargingMatrixSubliminalInterval != v) { m_chargingMatrixSubliminalInterval = v; emit chargingMatrixSubliminalIntervalChanged(); } }
    void setChargingMatrixSubliminalDuration(int v) { if (m_chargingMatrixSubliminalDuration != v) { m_chargingMatrixSubliminalDuration = v; emit chargingMatrixSubliminalDurationChanged(); } }
    void setChargingMatrixSubliminalStream(bool v) { if (m_chargingMatrixSubliminalStream != v) { m_chargingMatrixSubliminalStream = v; emit chargingMatrixSubliminalStreamChanged(); } }
    void setChargingMatrixSubliminalOverlay(bool v) { if (m_chargingMatrixSubliminalOverlay != v) { m_chargingMatrixSubliminalOverlay = v; emit chargingMatrixSubliminalOverlayChanged(); } }
    void setChargingMatrixSubliminalFlash(bool v) { if (m_chargingMatrixSubliminalFlash != v) { m_chargingMatrixSubliminalFlash = v; emit chargingMatrixSubliminalFlashChanged(); } }
    void setChargingTapToClose(bool v) { if (m_chargingTapToClose != v) { m_chargingTapToClose = v; emit chargingTapToCloseChanged(); } }
    void setChargingIdleEnabled(bool v) { if (m_chargingIdleEnabled != v) { m_chargingIdleEnabled = v; emit chargingIdleEnabledChanged(); } }
    void setChargingIdleTimeout(int v) { if (m_chargingIdleTimeout != v) { m_chargingIdleTimeout = v; emit chargingIdleTimeoutChanged(); } }
    void setChargingMotionToClose(bool v) { if (m_chargingMotionToClose != v) { m_chargingMotionToClose = v; emit chargingMotionToCloseChanged(); } }
    void setChargingBatteryDockedOnly(bool v) { if (m_chargingBatteryDockedOnly != v) { m_chargingBatteryDockedOnly = v; emit chargingBatteryDockedOnlyChanged(); } }

    // Test helper — reset all properties to defaults
    Q_INVOKABLE void resetDefaults() {
        setChargingTheme("matrix");
        setChargingShowClock(false);
        setChargingShowBattery(true);
        setChargingMatrixColor("#00ff41");
        setChargingMatrixSpeed(50);
        setChargingMatrixDensity(70);
        setChargingMatrixColorMode("green");
        setChargingMatrixTrail(50);
        setChargingMatrixFontSize(16);
        setChargingMatrixCharset("ascii");
        setChargingMatrixGlow(true);
        setChargingMatrixGlitch(true);
        setChargingMatrixGlitchRate(30);
        setChargingMatrixGlitchFlash(true);
        setChargingMatrixGlitchStutter(true);
        setChargingMatrixGlitchReverse(true);
        setChargingMatrixGlitchDirection(true);
        setChargingMatrixGlitchDirRate(30);
        setChargingMatrixGlitchDirCardinal(false);
        setChargingMatrixGlitchDirLength(5);
        setChargingMatrixGlitchRandomColor(false);
        setChargingMatrixGlitchChaos(false);
        setChargingMatrixGlitchChaosFrequency(50);
        setChargingMatrixGlitchChaosSurge(true);
        setChargingMatrixGlitchChaosScramble(true);
        setChargingMatrixGlitchChaosFreeze(true);
        setChargingMatrixGlitchChaosScatter(true);
        setChargingMatrixGlitchChaosIntensity(50);
        setChargingMatrixGlitchChaosScatterRate(50);
        setChargingMatrixGlitchChaosScatterLength(8);
        setChargingMatrixFade(60);
        setChargingMatrixDirection("down");
        setChargingMatrixInvertTrail(false);
        setChargingMatrixMessages("");
        setChargingMatrixMessageInterval(10);
        setChargingMatrixMessageRandom(true);
        setChargingMatrixMessageDirection("horizontal-lr");
        setChargingMatrixMessageFlash(true);
        setChargingMatrixMessagePulse(true);
        setChargingMatrixTapRandomize(false);
        setChargingMatrixTapRandomizeChance(50);
        setChargingMatrixSubliminal(false);
        setChargingMatrixSubliminalInterval(5);
        setChargingMatrixSubliminalDuration(8);
        setChargingMatrixSubliminalStream(true);
        setChargingMatrixSubliminalOverlay(true);
        setChargingMatrixSubliminalFlash(false);
        setChargingTapToClose(true);
        setChargingIdleEnabled(false);
        setChargingIdleTimeout(45);
        setChargingMotionToClose(false);
        setChargingBatteryDockedOnly(true);
    }

 signals:
    void chargingThemeChanged();
    void chargingShowClockChanged();
    void chargingShowBatteryChanged();
    void chargingMatrixColorChanged();
    void chargingMatrixSpeedChanged();
    void chargingMatrixDensityChanged();
    void chargingMatrixColorModeChanged();
    void chargingMatrixTrailChanged();
    void chargingMatrixFontSizeChanged();
    void chargingMatrixCharsetChanged();
    void chargingMatrixGlowChanged();
    void chargingMatrixGlitchChanged();
    void chargingMatrixGlitchRateChanged();
    void chargingMatrixGlitchFlashChanged();
    void chargingMatrixGlitchStutterChanged();
    void chargingMatrixGlitchReverseChanged();
    void chargingMatrixGlitchDirectionChanged();
    void chargingMatrixGlitchDirRateChanged();
    void chargingMatrixGlitchDirCardinalChanged();
    void chargingMatrixGlitchDirLengthChanged();
    void chargingMatrixGlitchRandomColorChanged();
    void chargingMatrixGlitchChaosChanged();
    void chargingMatrixGlitchChaosFrequencyChanged();
    void chargingMatrixGlitchChaosSurgeChanged();
    void chargingMatrixGlitchChaosScrambleChanged();
    void chargingMatrixGlitchChaosFreezeChanged();
    void chargingMatrixGlitchChaosScatterChanged();
    void chargingMatrixGlitchChaosIntensityChanged();
    void chargingMatrixGlitchChaosScatterRateChanged();
    void chargingMatrixGlitchChaosScatterLengthChanged();
    void chargingMatrixFadeChanged();
    void chargingMatrixDirectionChanged();
    void chargingMatrixInvertTrailChanged();
    void chargingMatrixMessagesChanged();
    void chargingMatrixMessageIntervalChanged();
    void chargingMatrixMessageRandomChanged();
    void chargingMatrixMessageDirectionChanged();
    void chargingMatrixMessageFlashChanged();
    void chargingMatrixMessagePulseChanged();
    void chargingMatrixTapRandomizeChanged();
    void chargingMatrixTapRandomizeChanceChanged();
    void chargingMatrixSubliminalChanged();
    void chargingMatrixSubliminalIntervalChanged();
    void chargingMatrixSubliminalDurationChanged();
    void chargingMatrixSubliminalStreamChanged();
    void chargingMatrixSubliminalOverlayChanged();
    void chargingMatrixSubliminalFlashChanged();
    void chargingTapToCloseChanged();
    void chargingIdleEnabledChanged();
    void chargingIdleTimeoutChanged();
    void chargingMotionToCloseChanged();
    void chargingBatteryDockedOnlyChanged();

 private:
    // Defaults match config.cpp QSettings defaults
    QString m_chargingTheme{"matrix"};
    bool m_chargingShowClock{false};
    bool m_chargingShowBattery{true};
    QString m_chargingMatrixColor{"#00ff41"};
    int m_chargingMatrixSpeed{50};
    int m_chargingMatrixDensity{70};
    QString m_chargingMatrixColorMode{"green"};
    int m_chargingMatrixTrail{50};
    int m_chargingMatrixFontSize{16};
    QString m_chargingMatrixCharset{"ascii"};
    bool m_chargingMatrixGlow{true};
    bool m_chargingMatrixGlitch{true};
    int m_chargingMatrixGlitchRate{30};
    bool m_chargingMatrixGlitchFlash{true};
    bool m_chargingMatrixGlitchStutter{true};
    bool m_chargingMatrixGlitchReverse{true};
    bool m_chargingMatrixGlitchDirection{true};
    int m_chargingMatrixGlitchDirRate{30};
    bool m_chargingMatrixGlitchDirCardinal{false};
    int m_chargingMatrixGlitchDirLength{5};
    bool m_chargingMatrixGlitchRandomColor{false};
    bool m_chargingMatrixGlitchChaos{false};
    int m_chargingMatrixGlitchChaosFrequency{50};
    bool m_chargingMatrixGlitchChaosSurge{true};
    bool m_chargingMatrixGlitchChaosScramble{true};
    bool m_chargingMatrixGlitchChaosFreeze{true};
    bool m_chargingMatrixGlitchChaosScatter{true};
    int m_chargingMatrixGlitchChaosIntensity{50};
    int m_chargingMatrixGlitchChaosScatterRate{50};
    int m_chargingMatrixGlitchChaosScatterLength{8};
    int m_chargingMatrixFade{60};
    QString m_chargingMatrixDirection{"down"};
    bool m_chargingMatrixInvertTrail{false};
    QString m_chargingMatrixMessages{""};
    int m_chargingMatrixMessageInterval{10};
    bool m_chargingMatrixMessageRandom{true};
    QString m_chargingMatrixMessageDirection{"horizontal-lr"};
    bool m_chargingMatrixMessageFlash{true};
    bool m_chargingMatrixMessagePulse{true};
    bool m_chargingMatrixTapRandomize{false};
    int m_chargingMatrixTapRandomizeChance{50};
    bool m_chargingMatrixSubliminal{false};
    int m_chargingMatrixSubliminalInterval{5};
    int m_chargingMatrixSubliminalDuration{8};
    bool m_chargingMatrixSubliminalStream{true};
    bool m_chargingMatrixSubliminalOverlay{true};
    bool m_chargingMatrixSubliminalFlash{false};
    bool m_chargingTapToClose{true};
    bool m_chargingIdleEnabled{false};
    int m_chargingIdleTimeout{45};
    bool m_chargingMotionToClose{false};
    bool m_chargingBatteryDockedOnly{true};
};
