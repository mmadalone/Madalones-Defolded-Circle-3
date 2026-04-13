// Copyright (c) 2026 madalone. Mock ScreensaverConfig singleton for QML tests.
// Auto-generated from screensaverconfig.h SCRN_* macros.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QObject>
#include <QQmlEngine>

class MockScreensaverConfig : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool showClock READ showClock WRITE setShowClock NOTIFY showClockChanged)
    Q_PROPERTY(bool clockDockedOnly READ clockDockedOnly WRITE setClockDockedOnly NOTIFY clockDockedOnlyChanged)
    Q_PROPERTY(int clockSize READ clockSize WRITE setClockSize NOTIFY clockSizeChanged)
    Q_PROPERTY(QString clockFont READ clockFont WRITE setClockFont NOTIFY clockFontChanged)
    Q_PROPERTY(QString clockColor READ clockColor WRITE setClockColor NOTIFY clockColorChanged)
    Q_PROPERTY(bool clockClock24h READ clockClock24h WRITE setClockClock24h NOTIFY clockClock24hChanged)
    Q_PROPERTY(int clockDateSize READ clockDateSize WRITE setClockDateSize NOTIFY clockDateSizeChanged)
    Q_PROPERTY(bool clockShowDate READ clockShowDate WRITE setClockShowDate NOTIFY clockShowDateChanged)
    Q_PROPERTY(QString clockPosition READ clockPosition WRITE setClockPosition NOTIFY clockPositionChanged)
    Q_PROPERTY(bool showBatteryEnabled READ showBatteryEnabled WRITE setShowBatteryEnabled NOTIFY showBatteryEnabledChanged)
    Q_PROPERTY(bool batteryDockedOnly READ batteryDockedOnly WRITE setBatteryDockedOnly NOTIFY batteryDockedOnlyChanged)
    Q_PROPERTY(int batteryTextSize READ batteryTextSize WRITE setBatteryTextSize NOTIFY batteryTextSizeChanged)
    Q_PROPERTY(QString matrixColor READ matrixColor WRITE setMatrixColor NOTIFY matrixColorChanged)
    Q_PROPERTY(int matrixSpeed READ matrixSpeed WRITE setMatrixSpeed NOTIFY matrixSpeedChanged)
    Q_PROPERTY(int matrixDensity READ matrixDensity WRITE setMatrixDensity NOTIFY matrixDensityChanged)
    Q_PROPERTY(int matrixFade READ matrixFade WRITE setMatrixFade NOTIFY matrixFadeChanged)
    Q_PROPERTY(int matrixTrail READ matrixTrail WRITE setMatrixTrail NOTIFY matrixTrailChanged)
    Q_PROPERTY(QString colorMode READ colorMode WRITE setColorMode NOTIFY colorModeChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QString charset READ charset WRITE setCharset NOTIFY charsetChanged)
    Q_PROPERTY(bool glow READ glow WRITE setGlow NOTIFY glowChanged)
    Q_PROPERTY(int glowFade READ glowFade WRITE setGlowFade NOTIFY glowFadeChanged)
    Q_PROPERTY(bool depthGlow READ depthGlow WRITE setDepthGlow NOTIFY depthGlowChanged)
    Q_PROPERTY(int depthGlowMin READ depthGlowMin WRITE setDepthGlowMin NOTIFY depthGlowMinChanged)
    Q_PROPERTY(bool invertTrail READ invertTrail WRITE setInvertTrail NOTIFY invertTrailChanged)
    Q_PROPERTY(bool glitch READ glitch WRITE setGlitch NOTIFY glitchChanged)
    Q_PROPERTY(int glitchRate READ glitchRate WRITE setGlitchRate NOTIFY glitchRateChanged)
    Q_PROPERTY(bool glitchFlash READ glitchFlash WRITE setGlitchFlash NOTIFY glitchFlashChanged)
    Q_PROPERTY(bool glitchStutter READ glitchStutter WRITE setGlitchStutter NOTIFY glitchStutterChanged)
    Q_PROPERTY(bool glitchReverse READ glitchReverse WRITE setGlitchReverse NOTIFY glitchReverseChanged)
    Q_PROPERTY(bool glitchDirection READ glitchDirection WRITE setGlitchDirection NOTIFY glitchDirectionChanged)
    Q_PROPERTY(int glitchDirRate READ glitchDirRate WRITE setGlitchDirRate NOTIFY glitchDirRateChanged)
    Q_PROPERTY(int glitchDirMask READ glitchDirMask WRITE setGlitchDirMask NOTIFY glitchDirMaskChanged)
    Q_PROPERTY(int glitchDirFade READ glitchDirFade WRITE setGlitchDirFade NOTIFY glitchDirFadeChanged)
    Q_PROPERTY(int glitchDirSpeed READ glitchDirSpeed WRITE setGlitchDirSpeed NOTIFY glitchDirSpeedChanged)
    Q_PROPERTY(int glitchDirLength READ glitchDirLength WRITE setGlitchDirLength NOTIFY glitchDirLengthChanged)
    Q_PROPERTY(bool glitchRandomColor READ glitchRandomColor WRITE setGlitchRandomColor NOTIFY glitchRandomColorChanged)
    Q_PROPERTY(bool glitchChaos READ glitchChaos WRITE setGlitchChaos NOTIFY glitchChaosChanged)
    Q_PROPERTY(int glitchChaosFrequency READ glitchChaosFrequency WRITE setGlitchChaosFrequency NOTIFY glitchChaosFrequencyChanged)
    Q_PROPERTY(bool glitchChaosSurge READ glitchChaosSurge WRITE setGlitchChaosSurge NOTIFY glitchChaosSurgeChanged)
    Q_PROPERTY(bool glitchChaosScramble READ glitchChaosScramble WRITE setGlitchChaosScramble NOTIFY glitchChaosScrambleChanged)
    Q_PROPERTY(bool glitchChaosFreeze READ glitchChaosFreeze WRITE setGlitchChaosFreeze NOTIFY glitchChaosFreezeChanged)
    Q_PROPERTY(bool glitchChaosScatter READ glitchChaosScatter WRITE setGlitchChaosScatter NOTIFY glitchChaosScatterChanged)
    Q_PROPERTY(bool glitchChaosSquareBurst READ glitchChaosSquareBurst WRITE setGlitchChaosSquareBurst NOTIFY glitchChaosSquareBurstChanged)
    Q_PROPERTY(int glitchChaosSquareBurstSize READ glitchChaosSquareBurstSize WRITE setGlitchChaosSquareBurstSize NOTIFY glitchChaosSquareBurstSizeChanged)
    Q_PROPERTY(bool glitchChaosRipple READ glitchChaosRipple WRITE setGlitchChaosRipple NOTIFY glitchChaosRippleChanged)
    Q_PROPERTY(bool glitchChaosWipe READ glitchChaosWipe WRITE setGlitchChaosWipe NOTIFY glitchChaosWipeChanged)
    Q_PROPERTY(int glitchChaosIntensity READ glitchChaosIntensity WRITE setGlitchChaosIntensity NOTIFY glitchChaosIntensityChanged)
    Q_PROPERTY(int glitchChaosScatterRate READ glitchChaosScatterRate WRITE setGlitchChaosScatterRate NOTIFY glitchChaosScatterRateChanged)
    Q_PROPERTY(int glitchChaosScatterLength READ glitchChaosScatterLength WRITE setGlitchChaosScatterLength NOTIFY glitchChaosScatterLengthChanged)
    Q_PROPERTY(QString direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(bool gravityMode READ gravityMode WRITE setGravityMode NOTIFY gravityModeChanged)
    Q_PROPERTY(int autoRotateSpeed READ autoRotateSpeed WRITE setAutoRotateSpeed NOTIFY autoRotateSpeedChanged)
    Q_PROPERTY(int autoRotateBend READ autoRotateBend WRITE setAutoRotateBend NOTIFY autoRotateBendChanged)
    Q_PROPERTY(QString messages READ messages WRITE setMessages NOTIFY messagesChanged)
    Q_PROPERTY(bool messagesEnabled READ messagesEnabled WRITE setMessagesEnabled NOTIFY messagesEnabledChanged)
    Q_PROPERTY(int messageInterval READ messageInterval WRITE setMessageInterval NOTIFY messageIntervalChanged)
    Q_PROPERTY(bool messageRandom READ messageRandom WRITE setMessageRandom NOTIFY messageRandomChanged)
    Q_PROPERTY(QString messageDirection READ messageDirection WRITE setMessageDirection NOTIFY messageDirectionChanged)
    Q_PROPERTY(bool messageFlash READ messageFlash WRITE setMessageFlash NOTIFY messageFlashChanged)
    Q_PROPERTY(bool messagePulse READ messagePulse WRITE setMessagePulse NOTIFY messagePulseChanged)
    Q_PROPERTY(bool subliminal READ subliminal WRITE setSubliminal NOTIFY subliminalChanged)
    Q_PROPERTY(int subliminalInterval READ subliminalInterval WRITE setSubliminalInterval NOTIFY subliminalIntervalChanged)
    Q_PROPERTY(int subliminalDuration READ subliminalDuration WRITE setSubliminalDuration NOTIFY subliminalDurationChanged)
    Q_PROPERTY(bool subliminalStream READ subliminalStream WRITE setSubliminalStream NOTIFY subliminalStreamChanged)
    Q_PROPERTY(bool subliminalOverlay READ subliminalOverlay WRITE setSubliminalOverlay NOTIFY subliminalOverlayChanged)
    Q_PROPERTY(bool subliminalFlash READ subliminalFlash WRITE setSubliminalFlash NOTIFY subliminalFlashChanged)
    Q_PROPERTY(bool tapBurst READ tapBurst WRITE setTapBurst NOTIFY tapBurstChanged)
    Q_PROPERTY(int tapBurstCount READ tapBurstCount WRITE setTapBurstCount NOTIFY tapBurstCountChanged)
    Q_PROPERTY(int tapBurstLength READ tapBurstLength WRITE setTapBurstLength NOTIFY tapBurstLengthChanged)
    Q_PROPERTY(bool tapFlash READ tapFlash WRITE setTapFlash NOTIFY tapFlashChanged)
    Q_PROPERTY(bool tapScramble READ tapScramble WRITE setTapScramble NOTIFY tapScrambleChanged)
    Q_PROPERTY(bool tapSpawn READ tapSpawn WRITE setTapSpawn NOTIFY tapSpawnChanged)
    Q_PROPERTY(int tapSpawnCount READ tapSpawnCount WRITE setTapSpawnCount NOTIFY tapSpawnCountChanged)
    Q_PROPERTY(int tapSpawnLength READ tapSpawnLength WRITE setTapSpawnLength NOTIFY tapSpawnLengthChanged)
    Q_PROPERTY(bool tapMessage READ tapMessage WRITE setTapMessage NOTIFY tapMessageChanged)
    Q_PROPERTY(bool tapSquareBurst READ tapSquareBurst WRITE setTapSquareBurst NOTIFY tapSquareBurstChanged)
    Q_PROPERTY(int tapSquareBurstSize READ tapSquareBurstSize WRITE setTapSquareBurstSize NOTIFY tapSquareBurstSizeChanged)
    Q_PROPERTY(bool tapRipple READ tapRipple WRITE setTapRipple NOTIFY tapRippleChanged)
    Q_PROPERTY(bool tapWipe READ tapWipe WRITE setTapWipe NOTIFY tapWipeChanged)
    Q_PROPERTY(bool tapRandomize READ tapRandomize WRITE setTapRandomize NOTIFY tapRandomizeChanged)
    Q_PROPERTY(int tapRandomizeChance READ tapRandomizeChance WRITE setTapRandomizeChance NOTIFY tapRandomizeChanceChanged)
    Q_PROPERTY(bool tapToClose READ tapToClose WRITE setTapToClose NOTIFY tapToCloseChanged)
    Q_PROPERTY(bool motionToClose READ motionToClose WRITE setMotionToClose NOTIFY motionToCloseChanged)
    Q_PROPERTY(bool idleEnabled READ idleEnabled WRITE setIdleEnabled NOTIFY idleEnabledChanged)
    Q_PROPERTY(int idleTimeout READ idleTimeout WRITE setIdleTimeout NOTIFY idleTimeoutChanged)
    Q_PROPERTY(bool dpadEnabled READ dpadEnabled WRITE setDpadEnabled NOTIFY dpadEnabledChanged)
    Q_PROPERTY(bool dpadPersist READ dpadPersist WRITE setDpadPersist NOTIFY dpadPersistChanged)
    Q_PROPERTY(bool dpadTouchbarSpeed READ dpadTouchbarSpeed WRITE setDpadTouchbarSpeed NOTIFY dpadTouchbarSpeedChanged)
    Q_PROPERTY(bool tapDirection READ tapDirection WRITE setTapDirection NOTIFY tapDirectionChanged)
    Q_PROPERTY(bool tapSwipeSpeed READ tapSwipeSpeed WRITE setTapSwipeSpeed NOTIFY tapSwipeSpeedChanged)
    Q_PROPERTY(QString lastDirection READ lastDirection WRITE setLastDirection NOTIFY lastDirectionChanged)
    Q_PROPERTY(bool depthEnabled READ depthEnabled WRITE setDepthEnabled NOTIFY depthEnabledChanged)
    Q_PROPERTY(int depthIntensity READ depthIntensity WRITE setDepthIntensity NOTIFY depthIntensityChanged)
    Q_PROPERTY(bool depthOverlay READ depthOverlay WRITE setDepthOverlay NOTIFY depthOverlayChanged)
    Q_PROPERTY(bool layersEnabled READ layersEnabled WRITE setLayersEnabled NOTIFY layersEnabledChanged)
    Q_PROPERTY(int minimalClockSize READ minimalClockSize WRITE setMinimalClockSize NOTIFY minimalClockSizeChanged)
    Q_PROPERTY(int minimalDateSize READ minimalDateSize WRITE setMinimalDateSize NOTIFY minimalDateSizeChanged)
    Q_PROPERTY(QString minimalFont READ minimalFont WRITE setMinimalFont NOTIFY minimalFontChanged)
    Q_PROPERTY(bool minimalClock24h READ minimalClock24h WRITE setMinimalClock24h NOTIFY minimalClock24hChanged)
    Q_PROPERTY(QString minimalTimeColor READ minimalTimeColor WRITE setMinimalTimeColor NOTIFY minimalTimeColorChanged)
    Q_PROPERTY(QString minimalDateColor READ minimalDateColor WRITE setMinimalDateColor NOTIFY minimalDateColorChanged)
    Q_PROPERTY(int starfieldSpeed READ starfieldSpeed WRITE setStarfieldSpeed NOTIFY starfieldSpeedChanged)
    Q_PROPERTY(int starfieldDensity READ starfieldDensity WRITE setStarfieldDensity NOTIFY starfieldDensityChanged)
    Q_PROPERTY(QString starfieldColor READ starfieldColor WRITE setStarfieldColor NOTIFY starfieldColorChanged)
    Q_PROPERTY(int starfieldStarSize READ starfieldStarSize WRITE setStarfieldStarSize NOTIFY starfieldStarSizeChanged)
    Q_PROPERTY(int starfieldTrailLength READ starfieldTrailLength WRITE setStarfieldTrailLength NOTIFY starfieldTrailLengthChanged)
    Q_PROPERTY(QString analogShutoffHands READ analogShutoffHands WRITE setAnalogShutoffHands NOTIFY analogShutoffHandsChanged)
    Q_PROPERTY(int tvStaticIntensity READ tvStaticIntensity WRITE setTvStaticIntensity NOTIFY tvStaticIntensityChanged)
    Q_PROPERTY(int tvStaticSnowSize READ tvStaticSnowSize WRITE setTvStaticSnowSize NOTIFY tvStaticSnowSizeChanged)
    Q_PROPERTY(int tvStaticScanlineStrength READ tvStaticScanlineStrength WRITE setTvStaticScanlineStrength NOTIFY tvStaticScanlineStrengthChanged)
    Q_PROPERTY(int tvStaticScanlineSpeed READ tvStaticScanlineSpeed WRITE setTvStaticScanlineSpeed NOTIFY tvStaticScanlineSpeedChanged)
    Q_PROPERTY(int tvStaticChromaAmount READ tvStaticChromaAmount WRITE setTvStaticChromaAmount NOTIFY tvStaticChromaAmountChanged)
    Q_PROPERTY(bool tvStaticTrackingEnable READ tvStaticTrackingEnable WRITE setTvStaticTrackingEnable NOTIFY tvStaticTrackingEnableChanged)
    Q_PROPERTY(int tvStaticTrackingSpeed READ tvStaticTrackingSpeed WRITE setTvStaticTrackingSpeed NOTIFY tvStaticTrackingSpeedChanged)
    Q_PROPERTY(bool tvStaticFlashOnTap READ tvStaticFlashOnTap WRITE setTvStaticFlashOnTap NOTIFY tvStaticFlashOnTapChanged)
    Q_PROPERTY(bool tvStaticChannelFlashAuto READ tvStaticChannelFlashAuto WRITE setTvStaticChannelFlashAuto NOTIFY tvStaticChannelFlashAutoChanged)
    Q_PROPERTY(int tvStaticFlashInterval READ tvStaticFlashInterval WRITE setTvStaticFlashInterval NOTIFY tvStaticFlashIntervalChanged)
    Q_PROPERTY(int tvStaticFlashDuration READ tvStaticFlashDuration WRITE setTvStaticFlashDuration NOTIFY tvStaticFlashDurationChanged)
    Q_PROPERTY(int tvStaticFlashBrightness READ tvStaticFlashBrightness WRITE setTvStaticFlashBrightness NOTIFY tvStaticFlashBrightnessChanged)
    Q_PROPERTY(QString tvStaticTint READ tvStaticTint WRITE setTvStaticTint NOTIFY tvStaticTintChanged)
    Q_PROPERTY(QColor color READ color NOTIFY colorChanged)
    Q_PROPERTY(qreal speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(qreal density READ density NOTIFY densityChanged)
    Q_PROPERTY(qreal fadeRate READ fadeRate NOTIFY fadeRateChanged)
    Q_PROPERTY(int trailLength READ trailLength NOTIFY trailLengthChanged)
    Q_PROPERTY(bool showBattery READ showBattery NOTIFY showBatteryChanged)

 public:
    explicit MockScreensaverConfig(QObject *parent = nullptr) : QObject(parent) {}

    QString theme() const { return m_theme; }
    void setTheme(QString v) { if (m_theme == v) return; m_theme = v; emit themeChanged(); }
    bool showClock() const { return m_showClock; }
    void setShowClock(bool v) { if (m_showClock == v) return; m_showClock = v; emit showClockChanged(); }
    bool clockDockedOnly() const { return m_clockDockedOnly; }
    void setClockDockedOnly(bool v) { if (m_clockDockedOnly == v) return; m_clockDockedOnly = v; emit clockDockedOnlyChanged(); }
    int clockSize() const { return m_clockSize; }
    void setClockSize(int v) { if (m_clockSize == v) return; m_clockSize = v; emit clockSizeChanged(); }
    QString clockFont() const { return m_clockFont; }
    void setClockFont(QString v) { if (m_clockFont == v) return; m_clockFont = v; emit clockFontChanged(); }
    QString clockColor() const { return m_clockColor; }
    void setClockColor(QString v) { if (m_clockColor == v) return; m_clockColor = v; emit clockColorChanged(); }
    bool clockClock24h() const { return m_clockClock24h; }
    void setClockClock24h(bool v) { if (m_clockClock24h == v) return; m_clockClock24h = v; emit clockClock24hChanged(); }
    int clockDateSize() const { return m_clockDateSize; }
    void setClockDateSize(int v) { if (m_clockDateSize == v) return; m_clockDateSize = v; emit clockDateSizeChanged(); }
    bool clockShowDate() const { return m_clockShowDate; }
    void setClockShowDate(bool v) { if (m_clockShowDate == v) return; m_clockShowDate = v; emit clockShowDateChanged(); }
    QString clockPosition() const { return m_clockPosition; }
    void setClockPosition(QString v) { if (m_clockPosition == v) return; m_clockPosition = v; emit clockPositionChanged(); }
    bool showBatteryEnabled() const { return m_showBatteryEnabled; }
    void setShowBatteryEnabled(bool v) { if (m_showBatteryEnabled == v) return; m_showBatteryEnabled = v; emit showBatteryEnabledChanged(); }
    bool batteryDockedOnly() const { return m_batteryDockedOnly; }
    void setBatteryDockedOnly(bool v) { if (m_batteryDockedOnly == v) return; m_batteryDockedOnly = v; emit batteryDockedOnlyChanged(); }
    int batteryTextSize() const { return m_batteryTextSize; }
    void setBatteryTextSize(int v) { if (m_batteryTextSize == v) return; m_batteryTextSize = v; emit batteryTextSizeChanged(); }
    QString matrixColor() const { return m_matrixColor; }
    void setMatrixColor(QString v) { if (m_matrixColor == v) return; m_matrixColor = v; emit matrixColorChanged(); }
    int matrixSpeed() const { return m_matrixSpeed; }
    void setMatrixSpeed(int v) { if (m_matrixSpeed == v) return; m_matrixSpeed = v; emit matrixSpeedChanged(); }
    int matrixDensity() const { return m_matrixDensity; }
    void setMatrixDensity(int v) { if (m_matrixDensity == v) return; m_matrixDensity = v; emit matrixDensityChanged(); }
    int matrixFade() const { return m_matrixFade; }
    void setMatrixFade(int v) { if (m_matrixFade == v) return; m_matrixFade = v; emit matrixFadeChanged(); }
    int matrixTrail() const { return m_matrixTrail; }
    void setMatrixTrail(int v) { if (m_matrixTrail == v) return; m_matrixTrail = v; emit matrixTrailChanged(); }
    QString colorMode() const { return m_colorMode; }
    void setColorMode(QString v) { if (m_colorMode == v) return; m_colorMode = v; emit colorModeChanged(); }
    int fontSize() const { return m_fontSize; }
    void setFontSize(int v) { if (m_fontSize == v) return; m_fontSize = v; emit fontSizeChanged(); }
    QString charset() const { return m_charset; }
    void setCharset(QString v) { if (m_charset == v) return; m_charset = v; emit charsetChanged(); }
    bool glow() const { return m_glow; }
    void setGlow(bool v) { if (m_glow == v) return; m_glow = v; emit glowChanged(); }
    int glowFade() const { return m_glowFade; }
    void setGlowFade(int v) { if (m_glowFade == v) return; m_glowFade = v; emit glowFadeChanged(); }
    bool depthGlow() const { return m_depthGlow; }
    void setDepthGlow(bool v) { if (m_depthGlow == v) return; m_depthGlow = v; emit depthGlowChanged(); }
    int depthGlowMin() const { return m_depthGlowMin; }
    void setDepthGlowMin(int v) { if (m_depthGlowMin == v) return; m_depthGlowMin = v; emit depthGlowMinChanged(); }
    bool invertTrail() const { return m_invertTrail; }
    void setInvertTrail(bool v) { if (m_invertTrail == v) return; m_invertTrail = v; emit invertTrailChanged(); }
    bool glitch() const { return m_glitch; }
    void setGlitch(bool v) { if (m_glitch == v) return; m_glitch = v; emit glitchChanged(); }
    int glitchRate() const { return m_glitchRate; }
    void setGlitchRate(int v) { if (m_glitchRate == v) return; m_glitchRate = v; emit glitchRateChanged(); }
    bool glitchFlash() const { return m_glitchFlash; }
    void setGlitchFlash(bool v) { if (m_glitchFlash == v) return; m_glitchFlash = v; emit glitchFlashChanged(); }
    bool glitchStutter() const { return m_glitchStutter; }
    void setGlitchStutter(bool v) { if (m_glitchStutter == v) return; m_glitchStutter = v; emit glitchStutterChanged(); }
    bool glitchReverse() const { return m_glitchReverse; }
    void setGlitchReverse(bool v) { if (m_glitchReverse == v) return; m_glitchReverse = v; emit glitchReverseChanged(); }
    bool glitchDirection() const { return m_glitchDirection; }
    void setGlitchDirection(bool v) { if (m_glitchDirection == v) return; m_glitchDirection = v; emit glitchDirectionChanged(); }
    int glitchDirRate() const { return m_glitchDirRate; }
    void setGlitchDirRate(int v) { if (m_glitchDirRate == v) return; m_glitchDirRate = v; emit glitchDirRateChanged(); }
    int glitchDirMask() const { return m_glitchDirMask; }
    void setGlitchDirMask(int v) { if (m_glitchDirMask == v) return; m_glitchDirMask = v; emit glitchDirMaskChanged(); }
    int glitchDirFade() const { return m_glitchDirFade; }
    void setGlitchDirFade(int v) { if (m_glitchDirFade == v) return; m_glitchDirFade = v; emit glitchDirFadeChanged(); }
    int glitchDirSpeed() const { return m_glitchDirSpeed; }
    void setGlitchDirSpeed(int v) { if (m_glitchDirSpeed == v) return; m_glitchDirSpeed = v; emit glitchDirSpeedChanged(); }
    int glitchDirLength() const { return m_glitchDirLength; }
    void setGlitchDirLength(int v) { if (m_glitchDirLength == v) return; m_glitchDirLength = v; emit glitchDirLengthChanged(); }
    bool glitchRandomColor() const { return m_glitchRandomColor; }
    void setGlitchRandomColor(bool v) { if (m_glitchRandomColor == v) return; m_glitchRandomColor = v; emit glitchRandomColorChanged(); }
    bool glitchChaos() const { return m_glitchChaos; }
    void setGlitchChaos(bool v) { if (m_glitchChaos == v) return; m_glitchChaos = v; emit glitchChaosChanged(); }
    int glitchChaosFrequency() const { return m_glitchChaosFrequency; }
    void setGlitchChaosFrequency(int v) { if (m_glitchChaosFrequency == v) return; m_glitchChaosFrequency = v; emit glitchChaosFrequencyChanged(); }
    bool glitchChaosSurge() const { return m_glitchChaosSurge; }
    void setGlitchChaosSurge(bool v) { if (m_glitchChaosSurge == v) return; m_glitchChaosSurge = v; emit glitchChaosSurgeChanged(); }
    bool glitchChaosScramble() const { return m_glitchChaosScramble; }
    void setGlitchChaosScramble(bool v) { if (m_glitchChaosScramble == v) return; m_glitchChaosScramble = v; emit glitchChaosScrambleChanged(); }
    bool glitchChaosFreeze() const { return m_glitchChaosFreeze; }
    void setGlitchChaosFreeze(bool v) { if (m_glitchChaosFreeze == v) return; m_glitchChaosFreeze = v; emit glitchChaosFreezeChanged(); }
    bool glitchChaosScatter() const { return m_glitchChaosScatter; }
    void setGlitchChaosScatter(bool v) { if (m_glitchChaosScatter == v) return; m_glitchChaosScatter = v; emit glitchChaosScatterChanged(); }
    bool glitchChaosSquareBurst() const { return m_glitchChaosSquareBurst; }
    void setGlitchChaosSquareBurst(bool v) { if (m_glitchChaosSquareBurst == v) return; m_glitchChaosSquareBurst = v; emit glitchChaosSquareBurstChanged(); }
    int glitchChaosSquareBurstSize() const { return m_glitchChaosSquareBurstSize; }
    void setGlitchChaosSquareBurstSize(int v) { if (m_glitchChaosSquareBurstSize == v) return; m_glitchChaosSquareBurstSize = v; emit glitchChaosSquareBurstSizeChanged(); }
    bool glitchChaosRipple() const { return m_glitchChaosRipple; }
    void setGlitchChaosRipple(bool v) { if (m_glitchChaosRipple == v) return; m_glitchChaosRipple = v; emit glitchChaosRippleChanged(); }
    bool glitchChaosWipe() const { return m_glitchChaosWipe; }
    void setGlitchChaosWipe(bool v) { if (m_glitchChaosWipe == v) return; m_glitchChaosWipe = v; emit glitchChaosWipeChanged(); }
    int glitchChaosIntensity() const { return m_glitchChaosIntensity; }
    void setGlitchChaosIntensity(int v) { if (m_glitchChaosIntensity == v) return; m_glitchChaosIntensity = v; emit glitchChaosIntensityChanged(); }
    int glitchChaosScatterRate() const { return m_glitchChaosScatterRate; }
    void setGlitchChaosScatterRate(int v) { if (m_glitchChaosScatterRate == v) return; m_glitchChaosScatterRate = v; emit glitchChaosScatterRateChanged(); }
    int glitchChaosScatterLength() const { return m_glitchChaosScatterLength; }
    void setGlitchChaosScatterLength(int v) { if (m_glitchChaosScatterLength == v) return; m_glitchChaosScatterLength = v; emit glitchChaosScatterLengthChanged(); }
    QString direction() const { return m_direction; }
    void setDirection(QString v) { if (m_direction == v) return; m_direction = v; emit directionChanged(); }
    bool gravityMode() const { return m_gravityMode; }
    void setGravityMode(bool v) { if (m_gravityMode == v) return; m_gravityMode = v; emit gravityModeChanged(); }
    int autoRotateSpeed() const { return m_autoRotateSpeed; }
    void setAutoRotateSpeed(int v) { if (m_autoRotateSpeed == v) return; m_autoRotateSpeed = v; emit autoRotateSpeedChanged(); }
    int autoRotateBend() const { return m_autoRotateBend; }
    void setAutoRotateBend(int v) { if (m_autoRotateBend == v) return; m_autoRotateBend = v; emit autoRotateBendChanged(); }
    QString messages() const { return m_messages; }
    void setMessages(QString v) { if (m_messages == v) return; m_messages = v; emit messagesChanged(); }
    bool messagesEnabled() const { return m_messagesEnabled; }
    void setMessagesEnabled(bool v) { if (m_messagesEnabled == v) return; m_messagesEnabled = v; emit messagesEnabledChanged(); }
    int messageInterval() const { return m_messageInterval; }
    void setMessageInterval(int v) { if (m_messageInterval == v) return; m_messageInterval = v; emit messageIntervalChanged(); }
    bool messageRandom() const { return m_messageRandom; }
    void setMessageRandom(bool v) { if (m_messageRandom == v) return; m_messageRandom = v; emit messageRandomChanged(); }
    QString messageDirection() const { return m_messageDirection; }
    void setMessageDirection(QString v) { if (m_messageDirection == v) return; m_messageDirection = v; emit messageDirectionChanged(); }
    bool messageFlash() const { return m_messageFlash; }
    void setMessageFlash(bool v) { if (m_messageFlash == v) return; m_messageFlash = v; emit messageFlashChanged(); }
    bool messagePulse() const { return m_messagePulse; }
    void setMessagePulse(bool v) { if (m_messagePulse == v) return; m_messagePulse = v; emit messagePulseChanged(); }
    bool subliminal() const { return m_subliminal; }
    void setSubliminal(bool v) { if (m_subliminal == v) return; m_subliminal = v; emit subliminalChanged(); }
    int subliminalInterval() const { return m_subliminalInterval; }
    void setSubliminalInterval(int v) { if (m_subliminalInterval == v) return; m_subliminalInterval = v; emit subliminalIntervalChanged(); }
    int subliminalDuration() const { return m_subliminalDuration; }
    void setSubliminalDuration(int v) { if (m_subliminalDuration == v) return; m_subliminalDuration = v; emit subliminalDurationChanged(); }
    bool subliminalStream() const { return m_subliminalStream; }
    void setSubliminalStream(bool v) { if (m_subliminalStream == v) return; m_subliminalStream = v; emit subliminalStreamChanged(); }
    bool subliminalOverlay() const { return m_subliminalOverlay; }
    void setSubliminalOverlay(bool v) { if (m_subliminalOverlay == v) return; m_subliminalOverlay = v; emit subliminalOverlayChanged(); }
    bool subliminalFlash() const { return m_subliminalFlash; }
    void setSubliminalFlash(bool v) { if (m_subliminalFlash == v) return; m_subliminalFlash = v; emit subliminalFlashChanged(); }
    bool tapBurst() const { return m_tapBurst; }
    void setTapBurst(bool v) { if (m_tapBurst == v) return; m_tapBurst = v; emit tapBurstChanged(); }
    int tapBurstCount() const { return m_tapBurstCount; }
    void setTapBurstCount(int v) { if (m_tapBurstCount == v) return; m_tapBurstCount = v; emit tapBurstCountChanged(); }
    int tapBurstLength() const { return m_tapBurstLength; }
    void setTapBurstLength(int v) { if (m_tapBurstLength == v) return; m_tapBurstLength = v; emit tapBurstLengthChanged(); }
    bool tapFlash() const { return m_tapFlash; }
    void setTapFlash(bool v) { if (m_tapFlash == v) return; m_tapFlash = v; emit tapFlashChanged(); }
    bool tapScramble() const { return m_tapScramble; }
    void setTapScramble(bool v) { if (m_tapScramble == v) return; m_tapScramble = v; emit tapScrambleChanged(); }
    bool tapSpawn() const { return m_tapSpawn; }
    void setTapSpawn(bool v) { if (m_tapSpawn == v) return; m_tapSpawn = v; emit tapSpawnChanged(); }
    int tapSpawnCount() const { return m_tapSpawnCount; }
    void setTapSpawnCount(int v) { if (m_tapSpawnCount == v) return; m_tapSpawnCount = v; emit tapSpawnCountChanged(); }
    int tapSpawnLength() const { return m_tapSpawnLength; }
    void setTapSpawnLength(int v) { if (m_tapSpawnLength == v) return; m_tapSpawnLength = v; emit tapSpawnLengthChanged(); }
    bool tapMessage() const { return m_tapMessage; }
    void setTapMessage(bool v) { if (m_tapMessage == v) return; m_tapMessage = v; emit tapMessageChanged(); }
    bool tapSquareBurst() const { return m_tapSquareBurst; }
    void setTapSquareBurst(bool v) { if (m_tapSquareBurst == v) return; m_tapSquareBurst = v; emit tapSquareBurstChanged(); }
    int tapSquareBurstSize() const { return m_tapSquareBurstSize; }
    void setTapSquareBurstSize(int v) { if (m_tapSquareBurstSize == v) return; m_tapSquareBurstSize = v; emit tapSquareBurstSizeChanged(); }
    bool tapRipple() const { return m_tapRipple; }
    void setTapRipple(bool v) { if (m_tapRipple == v) return; m_tapRipple = v; emit tapRippleChanged(); }
    bool tapWipe() const { return m_tapWipe; }
    void setTapWipe(bool v) { if (m_tapWipe == v) return; m_tapWipe = v; emit tapWipeChanged(); }
    bool tapRandomize() const { return m_tapRandomize; }
    void setTapRandomize(bool v) { if (m_tapRandomize == v) return; m_tapRandomize = v; emit tapRandomizeChanged(); }
    int tapRandomizeChance() const { return m_tapRandomizeChance; }
    void setTapRandomizeChance(int v) { if (m_tapRandomizeChance == v) return; m_tapRandomizeChance = v; emit tapRandomizeChanceChanged(); }
    bool tapToClose() const { return m_tapToClose; }
    void setTapToClose(bool v) { if (m_tapToClose == v) return; m_tapToClose = v; emit tapToCloseChanged(); }
    bool motionToClose() const { return m_motionToClose; }
    void setMotionToClose(bool v) { if (m_motionToClose == v) return; m_motionToClose = v; emit motionToCloseChanged(); }
    bool idleEnabled() const { return m_idleEnabled; }
    void setIdleEnabled(bool v) { if (m_idleEnabled == v) return; m_idleEnabled = v; emit idleEnabledChanged(); }
    int idleTimeout() const { return m_idleTimeout; }
    void setIdleTimeout(int v) { if (m_idleTimeout == v) return; m_idleTimeout = v; emit idleTimeoutChanged(); }
    bool dpadEnabled() const { return m_dpadEnabled; }
    void setDpadEnabled(bool v) { if (m_dpadEnabled == v) return; m_dpadEnabled = v; emit dpadEnabledChanged(); }
    bool dpadPersist() const { return m_dpadPersist; }
    void setDpadPersist(bool v) { if (m_dpadPersist == v) return; m_dpadPersist = v; emit dpadPersistChanged(); }
    bool dpadTouchbarSpeed() const { return m_dpadTouchbarSpeed; }
    void setDpadTouchbarSpeed(bool v) { if (m_dpadTouchbarSpeed == v) return; m_dpadTouchbarSpeed = v; emit dpadTouchbarSpeedChanged(); }
    bool tapDirection() const { return m_tapDirection; }
    void setTapDirection(bool v) { if (m_tapDirection == v) return; m_tapDirection = v; emit tapDirectionChanged(); }
    bool tapSwipeSpeed() const { return m_tapSwipeSpeed; }
    void setTapSwipeSpeed(bool v) { if (m_tapSwipeSpeed == v) return; m_tapSwipeSpeed = v; emit tapSwipeSpeedChanged(); }
    QString lastDirection() const { return m_lastDirection; }
    void setLastDirection(QString v) { if (m_lastDirection == v) return; m_lastDirection = v; emit lastDirectionChanged(); }
    bool depthEnabled() const { return m_depthEnabled; }
    void setDepthEnabled(bool v) { if (m_depthEnabled == v) return; m_depthEnabled = v; emit depthEnabledChanged(); }
    int depthIntensity() const { return m_depthIntensity; }
    void setDepthIntensity(int v) { if (m_depthIntensity == v) return; m_depthIntensity = v; emit depthIntensityChanged(); }
    bool depthOverlay() const { return m_depthOverlay; }
    void setDepthOverlay(bool v) { if (m_depthOverlay == v) return; m_depthOverlay = v; emit depthOverlayChanged(); }
    bool layersEnabled() const { return m_layersEnabled; }
    void setLayersEnabled(bool v) { if (m_layersEnabled == v) return; m_layersEnabled = v; emit layersEnabledChanged(); }
    int minimalClockSize() const { return m_minimalClockSize; }
    void setMinimalClockSize(int v) { if (m_minimalClockSize == v) return; m_minimalClockSize = v; emit minimalClockSizeChanged(); }
    int minimalDateSize() const { return m_minimalDateSize; }
    void setMinimalDateSize(int v) { if (m_minimalDateSize == v) return; m_minimalDateSize = v; emit minimalDateSizeChanged(); }
    QString minimalFont() const { return m_minimalFont; }
    void setMinimalFont(QString v) { if (m_minimalFont == v) return; m_minimalFont = v; emit minimalFontChanged(); }
    bool minimalClock24h() const { return m_minimalClock24h; }
    void setMinimalClock24h(bool v) { if (m_minimalClock24h == v) return; m_minimalClock24h = v; emit minimalClock24hChanged(); }
    QString minimalTimeColor() const { return m_minimalTimeColor; }
    void setMinimalTimeColor(QString v) { if (m_minimalTimeColor == v) return; m_minimalTimeColor = v; emit minimalTimeColorChanged(); }
    QString minimalDateColor() const { return m_minimalDateColor; }
    void setMinimalDateColor(QString v) { if (m_minimalDateColor == v) return; m_minimalDateColor = v; emit minimalDateColorChanged(); }
    int starfieldSpeed() const { return m_starfieldSpeed; }
    void setStarfieldSpeed(int v) { if (m_starfieldSpeed == v) return; m_starfieldSpeed = v; emit starfieldSpeedChanged(); }
    int starfieldDensity() const { return m_starfieldDensity; }
    void setStarfieldDensity(int v) { if (m_starfieldDensity == v) return; m_starfieldDensity = v; emit starfieldDensityChanged(); }
    QString starfieldColor() const { return m_starfieldColor; }
    void setStarfieldColor(QString v) { if (m_starfieldColor == v) return; m_starfieldColor = v; emit starfieldColorChanged(); }
    int starfieldStarSize() const { return m_starfieldStarSize; }
    void setStarfieldStarSize(int v) { if (m_starfieldStarSize == v) return; m_starfieldStarSize = v; emit starfieldStarSizeChanged(); }
    int starfieldTrailLength() const { return m_starfieldTrailLength; }
    void setStarfieldTrailLength(int v) { if (m_starfieldTrailLength == v) return; m_starfieldTrailLength = v; emit starfieldTrailLengthChanged(); }
    QString analogShutoffHands() const { return m_analogShutoffHands; }
    void setAnalogShutoffHands(QString v) { if (m_analogShutoffHands == v) return; m_analogShutoffHands = v; emit analogShutoffHandsChanged(); }
    int tvStaticIntensity() const { return m_tvStaticIntensity; }
    void setTvStaticIntensity(int v) { if (m_tvStaticIntensity == v) return; m_tvStaticIntensity = v; emit tvStaticIntensityChanged(); }
    int tvStaticSnowSize() const { return m_tvStaticSnowSize; }
    void setTvStaticSnowSize(int v) { if (m_tvStaticSnowSize == v) return; m_tvStaticSnowSize = v; emit tvStaticSnowSizeChanged(); }
    int tvStaticScanlineStrength() const { return m_tvStaticScanlineStrength; }
    void setTvStaticScanlineStrength(int v) { if (m_tvStaticScanlineStrength == v) return; m_tvStaticScanlineStrength = v; emit tvStaticScanlineStrengthChanged(); }
    int tvStaticScanlineSpeed() const { return m_tvStaticScanlineSpeed; }
    void setTvStaticScanlineSpeed(int v) { if (m_tvStaticScanlineSpeed == v) return; m_tvStaticScanlineSpeed = v; emit tvStaticScanlineSpeedChanged(); }
    int tvStaticChromaAmount() const { return m_tvStaticChromaAmount; }
    void setTvStaticChromaAmount(int v) { if (m_tvStaticChromaAmount == v) return; m_tvStaticChromaAmount = v; emit tvStaticChromaAmountChanged(); }
    bool tvStaticTrackingEnable() const { return m_tvStaticTrackingEnable; }
    void setTvStaticTrackingEnable(bool v) { if (m_tvStaticTrackingEnable == v) return; m_tvStaticTrackingEnable = v; emit tvStaticTrackingEnableChanged(); }
    int tvStaticTrackingSpeed() const { return m_tvStaticTrackingSpeed; }
    void setTvStaticTrackingSpeed(int v) { if (m_tvStaticTrackingSpeed == v) return; m_tvStaticTrackingSpeed = v; emit tvStaticTrackingSpeedChanged(); }
    bool tvStaticFlashOnTap() const { return m_tvStaticFlashOnTap; }
    void setTvStaticFlashOnTap(bool v) { if (m_tvStaticFlashOnTap == v) return; m_tvStaticFlashOnTap = v; emit tvStaticFlashOnTapChanged(); }
    bool tvStaticChannelFlashAuto() const { return m_tvStaticChannelFlashAuto; }
    void setTvStaticChannelFlashAuto(bool v) { if (m_tvStaticChannelFlashAuto == v) return; m_tvStaticChannelFlashAuto = v; emit tvStaticChannelFlashAutoChanged(); }
    int tvStaticFlashInterval() const { return m_tvStaticFlashInterval; }
    void setTvStaticFlashInterval(int v) { if (m_tvStaticFlashInterval == v) return; m_tvStaticFlashInterval = v; emit tvStaticFlashIntervalChanged(); }
    int tvStaticFlashDuration() const { return m_tvStaticFlashDuration; }
    void setTvStaticFlashDuration(int v) { if (m_tvStaticFlashDuration == v) return; m_tvStaticFlashDuration = v; emit tvStaticFlashDurationChanged(); }
    int tvStaticFlashBrightness() const { return m_tvStaticFlashBrightness; }
    void setTvStaticFlashBrightness(int v) { if (m_tvStaticFlashBrightness == v) return; m_tvStaticFlashBrightness = v; emit tvStaticFlashBrightnessChanged(); }
    QString tvStaticTint() const { return m_tvStaticTint; }
    void setTvStaticTint(QString v) { if (m_tvStaticTint == v) return; m_tvStaticTint = v; emit tvStaticTintChanged(); }

    QColor color() const { return m_color; }
    qreal speed() const { return m_speed; }
    qreal density() const { return m_density; }
    qreal fadeRate() const { return m_fadeRate; }
    int trailLength() const { return m_trailLength; }
    bool showBattery() const { return m_showBattery; }

    Q_INVOKABLE void resetDefaults() {
        setTheme("matrix");
        setShowClock(false);
        setClockDockedOnly(false);
        setClockSize(48);
        setClockFont("primary");
        setClockColor("#ffffff");
        setClockClock24h(true);
        setClockDateSize(20);
        setClockShowDate(false);
        setClockPosition("top");
        setShowBatteryEnabled(true);
        setBatteryDockedOnly(true);
        setBatteryTextSize(24);
        setMatrixColor("#00ff41");
        setMatrixSpeed(50);
        setMatrixDensity(70);
        setMatrixFade(60);
        setMatrixTrail(50);
        setColorMode("green");
        setFontSize(16);
        setCharset("ascii");
        setGlow(true);
        setGlowFade(50);
        setDepthGlow(false);
        setDepthGlowMin(40);
        setInvertTrail(false);
        setGlitch(true);
        setGlitchRate(30);
        setGlitchFlash(true);
        setGlitchStutter(true);
        setGlitchReverse(true);
        setGlitchDirection(true);
        setGlitchDirRate(30);
        setGlitchDirMask(255);
        setGlitchDirFade(20);
        setGlitchDirSpeed(50);
        setGlitchDirLength(5);
        setGlitchRandomColor(false);
        setGlitchChaos(false);
        setGlitchChaosFrequency(50);
        setGlitchChaosSurge(true);
        setGlitchChaosScramble(true);
        setGlitchChaosFreeze(true);
        setGlitchChaosScatter(true);
        setGlitchChaosSquareBurst(true);
        setGlitchChaosSquareBurstSize(5);
        setGlitchChaosRipple(true);
        setGlitchChaosWipe(false);
        setGlitchChaosIntensity(50);
        setGlitchChaosScatterRate(50);
        setGlitchChaosScatterLength(8);
        setDirection("down");
        setGravityMode(false);
        setAutoRotateSpeed(50);
        setAutoRotateBend(50);
        setMessages("");
        setMessagesEnabled(true);
        setMessageInterval(10);
        setMessageRandom(true);
        setMessageDirection("horizontal-lr");
        setMessageFlash(true);
        setMessagePulse(true);
        setSubliminal(false);
        setSubliminalInterval(5);
        setSubliminalDuration(8);
        setSubliminalStream(true);
        setSubliminalOverlay(true);
        setSubliminalFlash(false);
        setTapBurst(true);
        setTapBurstCount(25);
        setTapBurstLength(6);
        setTapFlash(true);
        setTapScramble(true);
        setTapSpawn(true);
        setTapSpawnCount(6);
        setTapSpawnLength(10);
        setTapMessage(true);
        setTapSquareBurst(true);
        setTapSquareBurstSize(5);
        setTapRipple(true);
        setTapWipe(false);
        setTapRandomize(false);
        setTapRandomizeChance(50);
        setTapToClose(true);
        setMotionToClose(false);
        setIdleEnabled(false);
        setIdleTimeout(45);
        setDpadEnabled(true);
        setDpadPersist(true);
        setDpadTouchbarSpeed(true);
        setTapDirection(false);
        setTapSwipeSpeed(true);
        setLastDirection("");
        setDepthEnabled(false);
        setDepthIntensity(50);
        setDepthOverlay(false);
        setLayersEnabled(false);
        setMinimalClockSize(96);
        setMinimalDateSize(28);
        setMinimalFont("primary");
        setMinimalClock24h(true);
        setMinimalTimeColor("#d0d0d0");
        setMinimalDateColor("#666666");
        setStarfieldSpeed(50);
        setStarfieldDensity(50);
        setStarfieldColor("#ffffff");
        setStarfieldStarSize(50);
        setStarfieldTrailLength(50);
        setAnalogShutoffHands("all");
        setTvStaticIntensity(70);
        setTvStaticSnowSize(2);
        setTvStaticScanlineStrength(35);
        setTvStaticScanlineSpeed(0);
        setTvStaticChromaAmount(25);
        setTvStaticTrackingEnable(true);
        setTvStaticTrackingSpeed(40);
        setTvStaticFlashOnTap(true);
        setTvStaticChannelFlashAuto(true);
        setTvStaticFlashInterval(20);
        setTvStaticFlashDuration(400);
        setTvStaticFlashBrightness(100);
        setTvStaticTint("#ffffff");
    }

 signals:
    void themeChanged();
    void showClockChanged();
    void clockDockedOnlyChanged();
    void clockSizeChanged();
    void clockFontChanged();
    void clockColorChanged();
    void clockClock24hChanged();
    void clockDateSizeChanged();
    void clockShowDateChanged();
    void clockPositionChanged();
    void showBatteryEnabledChanged();
    void batteryDockedOnlyChanged();
    void batteryTextSizeChanged();
    void matrixColorChanged();
    void matrixSpeedChanged();
    void matrixDensityChanged();
    void matrixFadeChanged();
    void matrixTrailChanged();
    void colorModeChanged();
    void fontSizeChanged();
    void charsetChanged();
    void glowChanged();
    void glowFadeChanged();
    void depthGlowChanged();
    void depthGlowMinChanged();
    void invertTrailChanged();
    void glitchChanged();
    void glitchRateChanged();
    void glitchFlashChanged();
    void glitchStutterChanged();
    void glitchReverseChanged();
    void glitchDirectionChanged();
    void glitchDirRateChanged();
    void glitchDirMaskChanged();
    void glitchDirFadeChanged();
    void glitchDirSpeedChanged();
    void glitchDirLengthChanged();
    void glitchRandomColorChanged();
    void glitchChaosChanged();
    void glitchChaosFrequencyChanged();
    void glitchChaosSurgeChanged();
    void glitchChaosScrambleChanged();
    void glitchChaosFreezeChanged();
    void glitchChaosScatterChanged();
    void glitchChaosSquareBurstChanged();
    void glitchChaosSquareBurstSizeChanged();
    void glitchChaosRippleChanged();
    void glitchChaosWipeChanged();
    void glitchChaosIntensityChanged();
    void glitchChaosScatterRateChanged();
    void glitchChaosScatterLengthChanged();
    void directionChanged();
    void gravityModeChanged();
    void autoRotateSpeedChanged();
    void autoRotateBendChanged();
    void messagesChanged();
    void messagesEnabledChanged();
    void messageIntervalChanged();
    void messageRandomChanged();
    void messageDirectionChanged();
    void messageFlashChanged();
    void messagePulseChanged();
    void subliminalChanged();
    void subliminalIntervalChanged();
    void subliminalDurationChanged();
    void subliminalStreamChanged();
    void subliminalOverlayChanged();
    void subliminalFlashChanged();
    void tapBurstChanged();
    void tapBurstCountChanged();
    void tapBurstLengthChanged();
    void tapFlashChanged();
    void tapScrambleChanged();
    void tapSpawnChanged();
    void tapSpawnCountChanged();
    void tapSpawnLengthChanged();
    void tapMessageChanged();
    void tapSquareBurstChanged();
    void tapSquareBurstSizeChanged();
    void tapRippleChanged();
    void tapWipeChanged();
    void tapRandomizeChanged();
    void tapRandomizeChanceChanged();
    void tapToCloseChanged();
    void motionToCloseChanged();
    void idleEnabledChanged();
    void idleTimeoutChanged();
    void dpadEnabledChanged();
    void dpadPersistChanged();
    void dpadTouchbarSpeedChanged();
    void tapDirectionChanged();
    void tapSwipeSpeedChanged();
    void lastDirectionChanged();
    void depthEnabledChanged();
    void depthIntensityChanged();
    void depthOverlayChanged();
    void layersEnabledChanged();
    void minimalClockSizeChanged();
    void minimalDateSizeChanged();
    void minimalFontChanged();
    void minimalClock24hChanged();
    void minimalTimeColorChanged();
    void minimalDateColorChanged();
    void starfieldSpeedChanged();
    void starfieldDensityChanged();
    void starfieldColorChanged();
    void starfieldStarSizeChanged();
    void starfieldTrailLengthChanged();
    void analogShutoffHandsChanged();
    void tvStaticIntensityChanged();
    void tvStaticSnowSizeChanged();
    void tvStaticScanlineStrengthChanged();
    void tvStaticScanlineSpeedChanged();
    void tvStaticChromaAmountChanged();
    void tvStaticTrackingEnableChanged();
    void tvStaticTrackingSpeedChanged();
    void tvStaticFlashOnTapChanged();
    void tvStaticChannelFlashAutoChanged();
    void tvStaticFlashIntervalChanged();
    void tvStaticFlashDurationChanged();
    void tvStaticFlashBrightnessChanged();
    void tvStaticTintChanged();
    void colorChanged();
    void speedChanged();
    void densityChanged();
    void fadeRateChanged();
    void trailLengthChanged();
    void showBatteryChanged();

 private:
    QString m_theme{"matrix"};
    bool m_showClock{false};
    bool m_clockDockedOnly{false};
    int m_clockSize{48};
    QString m_clockFont{"primary"};
    QString m_clockColor{"#ffffff"};
    bool m_clockClock24h{true};
    int m_clockDateSize{20};
    bool m_clockShowDate{false};
    QString m_clockPosition{"top"};
    bool m_showBatteryEnabled{true};
    bool m_batteryDockedOnly{true};
    int m_batteryTextSize{24};
    QString m_matrixColor{"#00ff41"};
    int m_matrixSpeed{50};
    int m_matrixDensity{70};
    int m_matrixFade{60};
    int m_matrixTrail{50};
    QString m_colorMode{"green"};
    int m_fontSize{16};
    QString m_charset{"ascii"};
    bool m_glow{true};
    int m_glowFade{50};
    bool m_depthGlow{false};
    int m_depthGlowMin{40};
    bool m_invertTrail{false};
    bool m_glitch{true};
    int m_glitchRate{30};
    bool m_glitchFlash{true};
    bool m_glitchStutter{true};
    bool m_glitchReverse{true};
    bool m_glitchDirection{true};
    int m_glitchDirRate{30};
    int m_glitchDirMask{255};
    int m_glitchDirFade{20};
    int m_glitchDirSpeed{50};
    int m_glitchDirLength{5};
    bool m_glitchRandomColor{false};
    bool m_glitchChaos{false};
    int m_glitchChaosFrequency{50};
    bool m_glitchChaosSurge{true};
    bool m_glitchChaosScramble{true};
    bool m_glitchChaosFreeze{true};
    bool m_glitchChaosScatter{true};
    bool m_glitchChaosSquareBurst{true};
    int m_glitchChaosSquareBurstSize{5};
    bool m_glitchChaosRipple{true};
    bool m_glitchChaosWipe{false};
    int m_glitchChaosIntensity{50};
    int m_glitchChaosScatterRate{50};
    int m_glitchChaosScatterLength{8};
    QString m_direction{"down"};
    bool m_gravityMode{false};
    int m_autoRotateSpeed{50};
    int m_autoRotateBend{50};
    QString m_messages{""};
    bool m_messagesEnabled{true};
    int m_messageInterval{10};
    bool m_messageRandom{true};
    QString m_messageDirection{"horizontal-lr"};
    bool m_messageFlash{true};
    bool m_messagePulse{true};
    bool m_subliminal{false};
    int m_subliminalInterval{5};
    int m_subliminalDuration{8};
    bool m_subliminalStream{true};
    bool m_subliminalOverlay{true};
    bool m_subliminalFlash{false};
    bool m_tapBurst{true};
    int m_tapBurstCount{25};
    int m_tapBurstLength{6};
    bool m_tapFlash{true};
    bool m_tapScramble{true};
    bool m_tapSpawn{true};
    int m_tapSpawnCount{6};
    int m_tapSpawnLength{10};
    bool m_tapMessage{true};
    bool m_tapSquareBurst{true};
    int m_tapSquareBurstSize{5};
    bool m_tapRipple{true};
    bool m_tapWipe{false};
    bool m_tapRandomize{false};
    int m_tapRandomizeChance{50};
    bool m_tapToClose{true};
    bool m_motionToClose{false};
    bool m_idleEnabled{false};
    int m_idleTimeout{45};
    bool m_dpadEnabled{true};
    bool m_dpadPersist{true};
    bool m_dpadTouchbarSpeed{true};
    bool m_tapDirection{false};
    bool m_tapSwipeSpeed{true};
    QString m_lastDirection{""};
    bool m_depthEnabled{false};
    int m_depthIntensity{50};
    bool m_depthOverlay{false};
    bool m_layersEnabled{false};
    int m_minimalClockSize{96};
    int m_minimalDateSize{28};
    QString m_minimalFont{"primary"};
    bool m_minimalClock24h{true};
    QString m_minimalTimeColor{"#d0d0d0"};
    QString m_minimalDateColor{"#666666"};
    int m_starfieldSpeed{50};
    int m_starfieldDensity{50};
    QString m_starfieldColor{"#ffffff"};
    int m_starfieldStarSize{50};
    int m_starfieldTrailLength{50};
    QString m_analogShutoffHands{"all"};
    int m_tvStaticIntensity{70};
    int m_tvStaticSnowSize{2};
    int m_tvStaticScanlineStrength{35};
    int m_tvStaticScanlineSpeed{0};
    int m_tvStaticChromaAmount{25};
    bool m_tvStaticTrackingEnable{true};
    int m_tvStaticTrackingSpeed{40};
    bool m_tvStaticFlashOnTap{true};
    bool m_tvStaticChannelFlashAuto{true};
    int m_tvStaticFlashInterval{20};
    int m_tvStaticFlashDuration{400};
    int m_tvStaticFlashBrightness{100};
    QString m_tvStaticTint{"#ffffff"};
    QColor m_color{QColor("#00ff41")};
    qreal m_speed{1.0};
    qreal m_density{0.7};
    qreal m_fadeRate{0.88};
    int m_trailLength{50};
    bool m_showBattery{true};
};
