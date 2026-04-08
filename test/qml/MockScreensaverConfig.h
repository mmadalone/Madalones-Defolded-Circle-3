// Copyright (c) 2024 madalone. Mock ScreensaverConfig singleton for QML tests.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>

class MockScreensaverConfig : public QObject {
    Q_OBJECT

    // Charging screen properties — names match screensaverconfig.h SCRN_* macro declarations exactly
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool showClock READ showClock WRITE setShowClock NOTIFY showClockChanged)
    Q_PROPERTY(bool showBatteryEnabled READ showBatteryEnabled WRITE setShowBatteryEnabled NOTIFY showBatteryEnabledChanged)
    Q_PROPERTY(QString matrixColor READ matrixColor WRITE setMatrixColor NOTIFY matrixColorChanged)
    Q_PROPERTY(int matrixSpeed READ matrixSpeed WRITE setMatrixSpeed NOTIFY matrixSpeedChanged)
    Q_PROPERTY(int matrixDensity READ matrixDensity WRITE setMatrixDensity NOTIFY matrixDensityChanged)
    Q_PROPERTY(QString colorMode READ colorMode WRITE setColorMode NOTIFY colorModeChanged)
    Q_PROPERTY(int matrixTrail READ matrixTrail WRITE setMatrixTrail NOTIFY matrixTrailChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QString charset READ charset WRITE setCharset NOTIFY charsetChanged)
    Q_PROPERTY(bool glow READ glow WRITE setGlow NOTIFY glowChanged)
    Q_PROPERTY(bool glitch READ glitch WRITE setGlitch NOTIFY glitchChanged)
    Q_PROPERTY(int glitchRate READ glitchRate WRITE setGlitchRate NOTIFY glitchRateChanged)
    Q_PROPERTY(bool glitchFlash READ glitchFlash WRITE setGlitchFlash NOTIFY glitchFlashChanged)
    Q_PROPERTY(bool glitchStutter READ glitchStutter WRITE setGlitchStutter NOTIFY glitchStutterChanged)
    Q_PROPERTY(bool glitchReverse READ glitchReverse WRITE setGlitchReverse NOTIFY glitchReverseChanged)
    Q_PROPERTY(bool glitchDirection READ glitchDirection WRITE setGlitchDirection NOTIFY glitchDirectionChanged)
    Q_PROPERTY(int glitchDirRate READ glitchDirRate WRITE setGlitchDirRate NOTIFY glitchDirRateChanged)
    Q_PROPERTY(bool glitchDirCardinal READ glitchDirCardinal WRITE setGlitchDirCardinal NOTIFY glitchDirCardinalChanged)
    Q_PROPERTY(bool glitchChaos READ glitchChaos WRITE setGlitchChaos NOTIFY glitchChaosChanged)
    Q_PROPERTY(int glitchChaosFrequency READ glitchChaosFrequency WRITE setGlitchChaosFrequency NOTIFY glitchChaosFrequencyChanged)
    Q_PROPERTY(bool glitchChaosSurge READ glitchChaosSurge WRITE setGlitchChaosSurge NOTIFY glitchChaosSurgeChanged)
    Q_PROPERTY(bool glitchChaosScramble READ glitchChaosScramble WRITE setGlitchChaosScramble NOTIFY glitchChaosScrambleChanged)
    Q_PROPERTY(bool glitchChaosFreeze READ glitchChaosFreeze WRITE setGlitchChaosFreeze NOTIFY glitchChaosFreezeChanged)
    Q_PROPERTY(bool glitchChaosScatter READ glitchChaosScatter WRITE setGlitchChaosScatter NOTIFY glitchChaosScatterChanged)
    Q_PROPERTY(int glitchDirLength READ glitchDirLength WRITE setGlitchDirLength NOTIFY glitchDirLengthChanged)
    Q_PROPERTY(bool glitchRandomColor READ glitchRandomColor WRITE setGlitchRandomColor NOTIFY glitchRandomColorChanged)
    Q_PROPERTY(int glitchChaosIntensity READ glitchChaosIntensity WRITE setGlitchChaosIntensity NOTIFY glitchChaosIntensityChanged)
    Q_PROPERTY(int glitchChaosScatterRate READ glitchChaosScatterRate WRITE setGlitchChaosScatterRate NOTIFY glitchChaosScatterRateChanged)
    Q_PROPERTY(int glitchChaosScatterLength READ glitchChaosScatterLength WRITE setGlitchChaosScatterLength NOTIFY glitchChaosScatterLengthChanged)
    Q_PROPERTY(int matrixFade READ matrixFade WRITE setMatrixFade NOTIFY matrixFadeChanged)
    Q_PROPERTY(QString direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(bool invertTrail READ invertTrail WRITE setInvertTrail NOTIFY invertTrailChanged)
    Q_PROPERTY(QString messages READ messages WRITE setMessages NOTIFY messagesChanged)
    Q_PROPERTY(int messageInterval READ messageInterval WRITE setMessageInterval NOTIFY messageIntervalChanged)
    Q_PROPERTY(bool messageRandom READ messageRandom WRITE setMessageRandom NOTIFY messageRandomChanged)
    Q_PROPERTY(QString messageDirection READ messageDirection WRITE setMessageDirection NOTIFY messageDirectionChanged)
    Q_PROPERTY(bool messageFlash READ messageFlash WRITE setMessageFlash NOTIFY messageFlashChanged)
    Q_PROPERTY(bool messagePulse READ messagePulse WRITE setMessagePulse NOTIFY messagePulseChanged)
    Q_PROPERTY(bool tapRandomize READ tapRandomize WRITE setTapRandomize NOTIFY tapRandomizeChanged)
    Q_PROPERTY(int tapRandomizeChance READ tapRandomizeChance WRITE setTapRandomizeChance NOTIFY tapRandomizeChanceChanged)
    Q_PROPERTY(bool subliminal READ subliminal WRITE setSubliminal NOTIFY subliminalChanged)
    Q_PROPERTY(int subliminalInterval READ subliminalInterval WRITE setSubliminalInterval NOTIFY subliminalIntervalChanged)
    Q_PROPERTY(int subliminalDuration READ subliminalDuration WRITE setSubliminalDuration NOTIFY subliminalDurationChanged)
    Q_PROPERTY(bool subliminalStream READ subliminalStream WRITE setSubliminalStream NOTIFY subliminalStreamChanged)
    Q_PROPERTY(bool subliminalOverlay READ subliminalOverlay WRITE setSubliminalOverlay NOTIFY subliminalOverlayChanged)
    Q_PROPERTY(bool subliminalFlash READ subliminalFlash WRITE setSubliminalFlash NOTIFY subliminalFlashChanged)
    Q_PROPERTY(bool tapToClose READ tapToClose WRITE setTapToClose NOTIFY tapToCloseChanged)
    Q_PROPERTY(bool idleEnabled READ idleEnabled WRITE setIdleEnabled NOTIFY idleEnabledChanged)
    Q_PROPERTY(int idleTimeout READ idleTimeout WRITE setIdleTimeout NOTIFY idleTimeoutChanged)
    Q_PROPERTY(bool motionToClose READ motionToClose WRITE setMotionToClose NOTIFY motionToCloseChanged)
    Q_PROPERTY(bool batteryDockedOnly READ batteryDockedOnly WRITE setBatteryDockedOnly NOTIFY batteryDockedOnlyChanged)

 public:
    explicit MockScreensaverConfig(QObject *parent = nullptr) : QObject(parent) {}

    // --- Getters ---
    QString theme() const { return m_theme; }
    bool showClock() const { return m_showClock; }
    bool showBatteryEnabled() const { return m_showBatteryEnabled; }
    QString matrixColor() const { return m_matrixColor; }
    int matrixSpeed() const { return m_matrixSpeed; }
    int matrixDensity() const { return m_matrixDensity; }
    QString colorMode() const { return m_colorMode; }
    int matrixTrail() const { return m_matrixTrail; }
    int fontSize() const { return m_fontSize; }
    QString charset() const { return m_charset; }
    bool glow() const { return m_glow; }
    bool glitch() const { return m_glitch; }
    int glitchRate() const { return m_glitchRate; }
    bool glitchFlash() const { return m_glitchFlash; }
    bool glitchStutter() const { return m_glitchStutter; }
    bool glitchReverse() const { return m_glitchReverse; }
    bool glitchDirection() const { return m_glitchDirection; }
    int glitchDirRate() const { return m_glitchDirRate; }
    bool glitchDirCardinal() const { return m_glitchDirCardinal; }
    int glitchDirLength() const { return m_glitchDirLength; }
    bool glitchRandomColor() const { return m_glitchRandomColor; }
    bool glitchChaos() const { return m_glitchChaos; }
    int glitchChaosFrequency() const { return m_glitchChaosFrequency; }
    bool glitchChaosSurge() const { return m_glitchChaosSurge; }
    bool glitchChaosScramble() const { return m_glitchChaosScramble; }
    bool glitchChaosFreeze() const { return m_glitchChaosFreeze; }
    bool glitchChaosScatter() const { return m_glitchChaosScatter; }
    int glitchChaosIntensity() const { return m_glitchChaosIntensity; }
    int glitchChaosScatterRate() const { return m_glitchChaosScatterRate; }
    int glitchChaosScatterLength() const { return m_glitchChaosScatterLength; }
    int matrixFade() const { return m_matrixFade; }
    QString direction() const { return m_direction; }
    bool invertTrail() const { return m_invertTrail; }
    QString messages() const { return m_messages; }
    int messageInterval() const { return m_messageInterval; }
    bool messageRandom() const { return m_messageRandom; }
    QString messageDirection() const { return m_messageDirection; }
    bool messageFlash() const { return m_messageFlash; }
    bool messagePulse() const { return m_messagePulse; }
    bool tapRandomize() const { return m_tapRandomize; }
    int tapRandomizeChance() const { return m_tapRandomizeChance; }
    bool subliminal() const { return m_subliminal; }
    int subliminalInterval() const { return m_subliminalInterval; }
    int subliminalDuration() const { return m_subliminalDuration; }
    bool subliminalStream() const { return m_subliminalStream; }
    bool subliminalOverlay() const { return m_subliminalOverlay; }
    bool subliminalFlash() const { return m_subliminalFlash; }
    bool tapToClose() const { return m_tapToClose; }
    bool idleEnabled() const { return m_idleEnabled; }
    int idleTimeout() const { return m_idleTimeout; }
    bool motionToClose() const { return m_motionToClose; }
    bool batteryDockedOnly() const { return m_batteryDockedOnly; }

    // --- Setters (emit signal only when value changes) ---
    void setTheme(const QString &v) { if (m_theme != v) { m_theme = v; emit themeChanged(); } }
    void setShowClock(bool v) { if (m_showClock != v) { m_showClock = v; emit showClockChanged(); } }
    void setShowBatteryEnabled(bool v) { if (m_showBatteryEnabled != v) { m_showBatteryEnabled = v; emit showBatteryEnabledChanged(); } }
    void setMatrixColor(const QString &v) { if (m_matrixColor != v) { m_matrixColor = v; emit matrixColorChanged(); } }
    void setMatrixSpeed(int v) { if (m_matrixSpeed != v) { m_matrixSpeed = v; emit matrixSpeedChanged(); } }
    void setMatrixDensity(int v) { if (m_matrixDensity != v) { m_matrixDensity = v; emit matrixDensityChanged(); } }
    void setColorMode(const QString &v) { if (m_colorMode != v) { m_colorMode = v; emit colorModeChanged(); } }
    void setMatrixTrail(int v) { if (m_matrixTrail != v) { m_matrixTrail = v; emit matrixTrailChanged(); } }
    void setFontSize(int v) { if (m_fontSize != v) { m_fontSize = v; emit fontSizeChanged(); } }
    void setCharset(const QString &v) { if (m_charset != v) { m_charset = v; emit charsetChanged(); } }
    void setGlow(bool v) { if (m_glow != v) { m_glow = v; emit glowChanged(); } }
    void setGlitch(bool v) { if (m_glitch != v) { m_glitch = v; emit glitchChanged(); } }
    void setGlitchRate(int v) { if (m_glitchRate != v) { m_glitchRate = v; emit glitchRateChanged(); } }
    void setGlitchFlash(bool v) { if (m_glitchFlash != v) { m_glitchFlash = v; emit glitchFlashChanged(); } }
    void setGlitchStutter(bool v) { if (m_glitchStutter != v) { m_glitchStutter = v; emit glitchStutterChanged(); } }
    void setGlitchReverse(bool v) { if (m_glitchReverse != v) { m_glitchReverse = v; emit glitchReverseChanged(); } }
    void setGlitchDirection(bool v) { if (m_glitchDirection != v) { m_glitchDirection = v; emit glitchDirectionChanged(); } }
    void setGlitchDirRate(int v) { if (m_glitchDirRate != v) { m_glitchDirRate = v; emit glitchDirRateChanged(); } }
    void setGlitchDirCardinal(bool v) { if (m_glitchDirCardinal != v) { m_glitchDirCardinal = v; emit glitchDirCardinalChanged(); } }
    void setGlitchDirLength(int v) { if (m_glitchDirLength != v) { m_glitchDirLength = v; emit glitchDirLengthChanged(); } }
    void setGlitchRandomColor(bool v) { if (m_glitchRandomColor != v) { m_glitchRandomColor = v; emit glitchRandomColorChanged(); } }
    void setGlitchChaos(bool v) { if (m_glitchChaos != v) { m_glitchChaos = v; emit glitchChaosChanged(); } }
    void setGlitchChaosFrequency(int v) { if (m_glitchChaosFrequency != v) { m_glitchChaosFrequency = v; emit glitchChaosFrequencyChanged(); } }
    void setGlitchChaosSurge(bool v) { if (m_glitchChaosSurge != v) { m_glitchChaosSurge = v; emit glitchChaosSurgeChanged(); } }
    void setGlitchChaosScramble(bool v) { if (m_glitchChaosScramble != v) { m_glitchChaosScramble = v; emit glitchChaosScrambleChanged(); } }
    void setGlitchChaosFreeze(bool v) { if (m_glitchChaosFreeze != v) { m_glitchChaosFreeze = v; emit glitchChaosFreezeChanged(); } }
    void setGlitchChaosScatter(bool v) { if (m_glitchChaosScatter != v) { m_glitchChaosScatter = v; emit glitchChaosScatterChanged(); } }
    void setGlitchChaosIntensity(int v) { if (m_glitchChaosIntensity != v) { m_glitchChaosIntensity = v; emit glitchChaosIntensityChanged(); } }
    void setGlitchChaosScatterRate(int v) { if (m_glitchChaosScatterRate != v) { m_glitchChaosScatterRate = v; emit glitchChaosScatterRateChanged(); } }
    void setGlitchChaosScatterLength(int v) { if (m_glitchChaosScatterLength != v) { m_glitchChaosScatterLength = v; emit glitchChaosScatterLengthChanged(); } }
    void setMatrixFade(int v) { if (m_matrixFade != v) { m_matrixFade = v; emit matrixFadeChanged(); } }
    void setDirection(const QString &v) { if (m_direction != v) { m_direction = v; emit directionChanged(); } }
    void setInvertTrail(bool v) { if (m_invertTrail != v) { m_invertTrail = v; emit invertTrailChanged(); } }
    void setMessages(const QString &v) { if (m_messages != v) { m_messages = v; emit messagesChanged(); } }
    void setMessageInterval(int v) { if (m_messageInterval != v) { m_messageInterval = v; emit messageIntervalChanged(); } }
    void setMessageRandom(bool v) { if (m_messageRandom != v) { m_messageRandom = v; emit messageRandomChanged(); } }
    void setMessageDirection(const QString &v) { if (m_messageDirection != v) { m_messageDirection = v; emit messageDirectionChanged(); } }
    void setMessageFlash(bool v) { if (m_messageFlash != v) { m_messageFlash = v; emit messageFlashChanged(); } }
    void setMessagePulse(bool v) { if (m_messagePulse != v) { m_messagePulse = v; emit messagePulseChanged(); } }
    void setTapRandomize(bool v) { if (m_tapRandomize != v) { m_tapRandomize = v; emit tapRandomizeChanged(); } }
    void setTapRandomizeChance(int v) { if (m_tapRandomizeChance != v) { m_tapRandomizeChance = v; emit tapRandomizeChanceChanged(); } }
    void setSubliminal(bool v) { if (m_subliminal != v) { m_subliminal = v; emit subliminalChanged(); } }
    void setSubliminalInterval(int v) { if (m_subliminalInterval != v) { m_subliminalInterval = v; emit subliminalIntervalChanged(); } }
    void setSubliminalDuration(int v) { if (m_subliminalDuration != v) { m_subliminalDuration = v; emit subliminalDurationChanged(); } }
    void setSubliminalStream(bool v) { if (m_subliminalStream != v) { m_subliminalStream = v; emit subliminalStreamChanged(); } }
    void setSubliminalOverlay(bool v) { if (m_subliminalOverlay != v) { m_subliminalOverlay = v; emit subliminalOverlayChanged(); } }
    void setSubliminalFlash(bool v) { if (m_subliminalFlash != v) { m_subliminalFlash = v; emit subliminalFlashChanged(); } }
    void setTapToClose(bool v) { if (m_tapToClose != v) { m_tapToClose = v; emit tapToCloseChanged(); } }
    void setIdleEnabled(bool v) { if (m_idleEnabled != v) { m_idleEnabled = v; emit idleEnabledChanged(); } }
    void setIdleTimeout(int v) { if (m_idleTimeout != v) { m_idleTimeout = v; emit idleTimeoutChanged(); } }
    void setMotionToClose(bool v) { if (m_motionToClose != v) { m_motionToClose = v; emit motionToCloseChanged(); } }
    void setBatteryDockedOnly(bool v) { if (m_batteryDockedOnly != v) { m_batteryDockedOnly = v; emit batteryDockedOnlyChanged(); } }

    // Test helper — reset all properties to defaults
    Q_INVOKABLE void resetDefaults() {
        setTheme("matrix");
        setShowClock(false);
        setShowBatteryEnabled(true);
        setMatrixColor("#00ff41");
        setMatrixSpeed(50);
        setMatrixDensity(70);
        setColorMode("green");
        setMatrixTrail(50);
        setFontSize(16);
        setCharset("ascii");
        setGlow(true);
        setGlitch(true);
        setGlitchRate(30);
        setGlitchFlash(true);
        setGlitchStutter(true);
        setGlitchReverse(true);
        setGlitchDirection(true);
        setGlitchDirRate(30);
        setGlitchDirCardinal(false);
        setGlitchDirLength(5);
        setGlitchRandomColor(false);
        setGlitchChaos(false);
        setGlitchChaosFrequency(50);
        setGlitchChaosSurge(true);
        setGlitchChaosScramble(true);
        setGlitchChaosFreeze(true);
        setGlitchChaosScatter(true);
        setGlitchChaosIntensity(50);
        setGlitchChaosScatterRate(50);
        setGlitchChaosScatterLength(8);
        setMatrixFade(60);
        setDirection("down");
        setInvertTrail(false);
        setMessages("");
        setMessageInterval(10);
        setMessageRandom(true);
        setMessageDirection("horizontal-lr");
        setMessageFlash(true);
        setMessagePulse(true);
        setTapRandomize(false);
        setTapRandomizeChance(50);
        setSubliminal(false);
        setSubliminalInterval(5);
        setSubliminalDuration(8);
        setSubliminalStream(true);
        setSubliminalOverlay(true);
        setSubliminalFlash(false);
        setTapToClose(true);
        setIdleEnabled(false);
        setIdleTimeout(45);
        setMotionToClose(false);
        setBatteryDockedOnly(true);
    }

 signals:
    void themeChanged();
    void showClockChanged();
    void showBatteryEnabledChanged();
    void matrixColorChanged();
    void matrixSpeedChanged();
    void matrixDensityChanged();
    void colorModeChanged();
    void matrixTrailChanged();
    void fontSizeChanged();
    void charsetChanged();
    void glowChanged();
    void glitchChanged();
    void glitchRateChanged();
    void glitchFlashChanged();
    void glitchStutterChanged();
    void glitchReverseChanged();
    void glitchDirectionChanged();
    void glitchDirRateChanged();
    void glitchDirCardinalChanged();
    void glitchDirLengthChanged();
    void glitchRandomColorChanged();
    void glitchChaosChanged();
    void glitchChaosFrequencyChanged();
    void glitchChaosSurgeChanged();
    void glitchChaosScrambleChanged();
    void glitchChaosFreezeChanged();
    void glitchChaosScatterChanged();
    void glitchChaosIntensityChanged();
    void glitchChaosScatterRateChanged();
    void glitchChaosScatterLengthChanged();
    void matrixFadeChanged();
    void directionChanged();
    void invertTrailChanged();
    void messagesChanged();
    void messageIntervalChanged();
    void messageRandomChanged();
    void messageDirectionChanged();
    void messageFlashChanged();
    void messagePulseChanged();
    void tapRandomizeChanged();
    void tapRandomizeChanceChanged();
    void subliminalChanged();
    void subliminalIntervalChanged();
    void subliminalDurationChanged();
    void subliminalStreamChanged();
    void subliminalOverlayChanged();
    void subliminalFlashChanged();
    void tapToCloseChanged();
    void idleEnabledChanged();
    void idleTimeoutChanged();
    void motionToCloseChanged();
    void batteryDockedOnlyChanged();

 private:
    // Defaults match config.cpp QSettings defaults
    QString m_theme{"matrix"};
    bool m_showClock{false};
    bool m_showBatteryEnabled{true};
    QString m_matrixColor{"#00ff41"};
    int m_matrixSpeed{50};
    int m_matrixDensity{70};
    QString m_colorMode{"green"};
    int m_matrixTrail{50};
    int m_fontSize{16};
    QString m_charset{"ascii"};
    bool m_glow{true};
    bool m_glitch{true};
    int m_glitchRate{30};
    bool m_glitchFlash{true};
    bool m_glitchStutter{true};
    bool m_glitchReverse{true};
    bool m_glitchDirection{true};
    int m_glitchDirRate{30};
    bool m_glitchDirCardinal{false};
    int m_glitchDirLength{5};
    bool m_glitchRandomColor{false};
    bool m_glitchChaos{false};
    int m_glitchChaosFrequency{50};
    bool m_glitchChaosSurge{true};
    bool m_glitchChaosScramble{true};
    bool m_glitchChaosFreeze{true};
    bool m_glitchChaosScatter{true};
    int m_glitchChaosIntensity{50};
    int m_glitchChaosScatterRate{50};
    int m_glitchChaosScatterLength{8};
    int m_matrixFade{60};
    QString m_direction{"down"};
    bool m_invertTrail{false};
    QString m_messages{""};
    int m_messageInterval{10};
    bool m_messageRandom{true};
    QString m_messageDirection{"horizontal-lr"};
    bool m_messageFlash{true};
    bool m_messagePulse{true};
    bool m_tapRandomize{false};
    int m_tapRandomizeChance{50};
    bool m_subliminal{false};
    int m_subliminalInterval{5};
    int m_subliminalDuration{8};
    bool m_subliminalStream{true};
    bool m_subliminalOverlay{true};
    bool m_subliminalFlash{false};
    bool m_tapToClose{true};
    bool m_idleEnabled{false};
    int m_idleTimeout{45};
    bool m_motionToClose{false};
    bool m_batteryDockedOnly{true};
};
