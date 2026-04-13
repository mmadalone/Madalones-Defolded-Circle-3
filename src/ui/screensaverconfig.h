// Copyright (c) 2026 madalone. Screensaver config — owns QSettings storage for all screensaver
// properties. Single declaration per property via SCRN_* macros. No dependency on upstream Config.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QObject>
#include <QQmlEngine>
#include <QSettings>

#include "screensaverconfig_macros.h"

namespace uc {

// ---------------------------------------------------------------------------
// ScreensaverConfig — singleton owning all screensaver QSettings storage.
//
// Responsibilities:
//   1. Read/write all charging/* QSettings keys (own QSettings instance)
//   2. Apply value transforms (speed/50, density/100, fadeRate formula, trailLength)
//   3. Handle showBattery conditional (dockedOnly + Battery::powerSupply)
//
// QML usage:  import ScreensaverConfig 1.0
//             MatrixRain { speed: ScreensaverConfig.speed }
//             // Settings pages write: ScreensaverConfig.matrixSpeed = slider.value
// ---------------------------------------------------------------------------
class ScreensaverConfig : public QObject {
    Q_OBJECT

    // === Theme + overlays (shared across all themes) ===
    SCRN_STRING(theme,             "charging/theme",                  "matrix")
    SCRN_BOOL(showClock,           "charging/showClock",              false)
    SCRN_BOOL(clockDockedOnly,     "charging/clockDockedOnly",        false)
    SCRN_INT(clockSize,            "charging/clockSize",              48)
    SCRN_STRING(clockFont,         "charging/clockFont",              "primary")
    SCRN_STRING(clockColor,        "charging/clockColor",             "#ffffff")
    SCRN_BOOL(clockClock24h,       "charging/clockClock24h",          true)
    SCRN_INT(clockDateSize,        "charging/clockDateSize",          20)
    SCRN_BOOL(clockShowDate,       "charging/clockShowDate",          false)
    SCRN_STRING(clockDateColor,    "charging/clockDateColor",         "#d0d0d0")
    SCRN_STRING(clockPosition,     "charging/clockPosition",          "top")
    SCRN_BOOL(showBatteryEnabled,  "charging/showBattery",            true)
    SCRN_BOOL(batteryDockedOnly,   "charging/batteryDockedOnly",      true)
    SCRN_INT(batteryTextSize,      "charging/batteryTextSize",        24)

    // === Core appearance — raw ints for settings page sliders ===
    // These five properties back transformed getters (color, speed, density,
    // fadeRate, trailLength) declared below. Hand-written setters dual-emit
    // both the raw and the transformed NOTIFY signal so QML property
    // bindings on the transformed properties update correctly. See the
    // `Transformed read-only properties` block below for the paired signals.

    Q_PROPERTY(QString matrixColor READ matrixColor WRITE setMatrixColor NOTIFY matrixColorChanged)
public:
    QString matrixColor() const { return m_settings->value("charging/matrixColor", "#00ff41").toString(); }
    void setMatrixColor(const QString &value);
signals:
    void matrixColorChanged();
public:

    Q_PROPERTY(int matrixSpeed READ matrixSpeed WRITE setMatrixSpeed NOTIFY matrixSpeedChanged)
public:
    int matrixSpeed() const { return m_settings->value("charging/matrixSpeed", 50).toInt(); }
    void setMatrixSpeed(int value);
signals:
    void matrixSpeedChanged();
public:

    Q_PROPERTY(int matrixDensity READ matrixDensity WRITE setMatrixDensity NOTIFY matrixDensityChanged)
public:
    int matrixDensity() const { return m_settings->value("charging/matrixDensity", 70).toInt(); }
    void setMatrixDensity(int value);
signals:
    void matrixDensityChanged();
public:

    Q_PROPERTY(int matrixFade READ matrixFade WRITE setMatrixFade NOTIFY matrixFadeChanged)
public:
    int matrixFade() const { return m_settings->value("charging/matrixFade", 60).toInt(); }
    void setMatrixFade(int value);
signals:
    void matrixFadeChanged();
public:

    Q_PROPERTY(int matrixTrail READ matrixTrail WRITE setMatrixTrail NOTIFY matrixTrailChanged)
public:
    int matrixTrail() const { return m_settings->value("charging/matrixTrail", 50).toInt(); }
    void setMatrixTrail(int value);
signals:
    void matrixTrailChanged();
public:
    SCRN_STRING(colorMode,         "charging/matrixColorMode",        "green")
    SCRN_INT(fontSize,             "charging/matrixFontSize",         16)
    SCRN_STRING(charset,           "charging/matrixCharset",          "ascii")

    // === Visual effects ===
    SCRN_BOOL(glow,                "charging/matrixGlow",             true)
    SCRN_INT(glowFade,             "charging/matrixGlowFade",         50)
    SCRN_BOOL(depthGlow,           "charging/matrixDepthGlow",        false)
    SCRN_INT(depthGlowMin,         "charging/matrixDepthGlowMin",     40)
    SCRN_BOOL(invertTrail,         "charging/matrixInvertTrail",      false)

    // === Glitch ===
    SCRN_BOOL(glitch,              "charging/matrixGlitch",           true)
    SCRN_INT(glitchRate,           "charging/matrixGlitchRate",       30)
    SCRN_BOOL(glitchFlash,         "charging/matrixGlitchFlash",      true)
    SCRN_BOOL(glitchStutter,       "charging/matrixGlitchStutter",    true)
    SCRN_BOOL(glitchReverse,       "charging/matrixGlitchReverse",    true)
    SCRN_BOOL(glitchDirection,     "charging/matrixGlitchDirection",  true)
    SCRN_INT(glitchDirRate,        "charging/matrixGlitchDirRate",    30)
    SCRN_INT(glitchDirMask,        "charging/matrixGlitchDirMask",    255)
    SCRN_INT(glitchDirFade,        "charging/matrixGlitchDirFade",    20)
    SCRN_INT(glitchDirSpeed,       "charging/matrixGlitchDirSpeed",   50)
    SCRN_INT(glitchDirLength,      "charging/matrixGlitchDirLength",  5)
    SCRN_BOOL(glitchRandomColor,   "charging/matrixGlitchRandomColor", false)
    SCRN_BOOL(glitchChaos,         "charging/matrixGlitchChaos",      false)
    SCRN_INT(glitchChaosFrequency, "charging/matrixGlitchChaosFrequency", 50)
    SCRN_BOOL(glitchChaosSurge,    "charging/matrixGlitchChaosSurge", true)
    SCRN_BOOL(glitchChaosScramble, "charging/matrixGlitchChaosScramble", true)
    SCRN_BOOL(glitchChaosFreeze,   "charging/matrixGlitchChaosFreeze", true)
    SCRN_BOOL(glitchChaosScatter,  "charging/matrixGlitchChaosScatter", true)
    SCRN_BOOL(glitchChaosSquareBurst, "charging/matrixGlitchChaosSquareBurst", true)
    SCRN_INT(glitchChaosSquareBurstSize, "charging/matrixGlitchChaosSquareBurstSize", 5)
    SCRN_BOOL(glitchChaosRipple,   "charging/matrixGlitchChaosRipple", true)
    SCRN_BOOL(glitchChaosWipe,     "charging/matrixGlitchChaosWipe",  false)
    SCRN_INT(glitchChaosIntensity, "charging/matrixGlitchChaosIntensity", 50)
    SCRN_INT(glitchChaosScatterRate, "charging/matrixGlitchChaosScatterRate", 50)
    SCRN_INT(glitchChaosScatterLength, "charging/matrixGlitchChaosScatterLength", 8)

    // === Direction / gravity ===
    SCRN_STRING(direction,         "charging/matrixDirection",        "down")
    SCRN_BOOL(gravityMode,         "charging/matrixGravity",         false)
    SCRN_INT(autoRotateSpeed,      "charging/matrixAutoRotateSpeed",  50)
    SCRN_INT(autoRotateBend,       "charging/matrixAutoRotateBend",   50)

    // === Messages ===
    SCRN_STRING(messages,          "charging/matrixMessages",         "")
    SCRN_BOOL(messagesEnabled,     "charging/matrixMessagesEnabled",  true)
    SCRN_INT(messageInterval,      "charging/matrixMessageInterval",  10)
    SCRN_BOOL(messageRandom,       "charging/matrixMessageRandom",    true)
    SCRN_STRING(messageDirection,  "charging/matrixMessageDirection", "horizontal-lr")
    SCRN_BOOL(messageFlash,        "charging/matrixMessageFlash",     true)
    SCRN_BOOL(messagePulse,        "charging/matrixMessagePulse",     true)

    // === Subliminal ===
    SCRN_BOOL(subliminal,          "charging/matrixSubliminal",       false)
    SCRN_INT(subliminalInterval,   "charging/matrixSubliminalInterval", 5)
    SCRN_INT(subliminalDuration,   "charging/matrixSubliminalDuration", 8)
    SCRN_BOOL(subliminalStream,    "charging/matrixSubliminalStream", true)
    SCRN_BOOL(subliminalOverlay,   "charging/matrixSubliminalOverlay", true)
    SCRN_BOOL(subliminalFlash,     "charging/matrixSubliminalFlash",  false)

    // === Tap interaction ===
    SCRN_BOOL(tapEnabled,          "charging/matrixTapEnabled",       true)
    SCRN_BOOL(tapBurst,            "charging/matrixTapBurst",         true)
    SCRN_INT(tapBurstCount,        "charging/matrixTapBurstCount",    25)
    SCRN_INT(tapBurstLength,       "charging/matrixTapBurstLength",   6)
    SCRN_BOOL(tapFlash,            "charging/matrixTapFlash",         true)
    SCRN_BOOL(tapScramble,         "charging/matrixTapScramble",      true)
    SCRN_BOOL(tapSpawn,            "charging/matrixTapSpawn",         true)
    SCRN_INT(tapSpawnCount,        "charging/matrixTapSpawnCount",    6)
    SCRN_INT(tapSpawnLength,       "charging/matrixTapSpawnLength",   10)
    SCRN_BOOL(tapMessage,          "charging/matrixTapMessage",       true)
    SCRN_BOOL(tapSquareBurst,      "charging/matrixTapSquareBurst",   true)
    SCRN_INT(tapSquareBurstSize,   "charging/matrixTapSquareBurstSize", 5)
    SCRN_BOOL(tapRipple,           "charging/matrixTapRipple",        true)
    SCRN_BOOL(tapWipe,             "charging/matrixTapWipe",          false)
    SCRN_BOOL(tapRandomize,        "charging/matrixTapRandomize",     false)
    SCRN_INT(tapRandomizeChance,   "charging/matrixTapRandomizeChance", 50)

    // === General behavior ===
    SCRN_BOOL(tapToClose,          "charging/tapToClose",             true)
    SCRN_BOOL(motionToClose,       "charging/motionToClose",          false)
    SCRN_BOOL(idleEnabled,         "charging/idleEnabled",            false)
    SCRN_INT(idleTimeout,          "charging/idleTimeout",            45)
    SCRN_BOOL(dpadEnabled,         "charging/matrixDpadEnabled",      true)
    SCRN_BOOL(dpadPersist,         "charging/matrixDpadPersist",      true)
    SCRN_BOOL(dpadTouchbarSpeed,   "charging/matrixDpadTouchbarSpeed", true)
    SCRN_BOOL(tapDirection,        "charging/matrixTapDirection",     false)
    SCRN_BOOL(tapSwipeSpeed,       "charging/matrixTapSwipeSpeed",    true)
    SCRN_STRING(lastDirection,     "charging/matrixLastDirection",    "")

    // === Color layers (per-vertex depth tinting) ===
    SCRN_BOOL(depthEnabled,        "charging/matrixDepthEnabled",     false)
    SCRN_INT(depthIntensity,       "charging/matrixDepthIntensity",   50)
    SCRN_BOOL(depthOverlay,        "charging/matrixDepthOverlay",     false)

    // === Rain layers (multi-grid depth) ===
    SCRN_BOOL(layersEnabled,       "charging/matrixLayersEnabled",    false)

    // === Minimal theme ===
    SCRN_INT(minimalClockSize,     "charging/minimalClockSize",       96)
    SCRN_INT(minimalDateSize,      "charging/minimalDateSize",        28)
    SCRN_STRING(minimalFont,       "charging/minimalFont",            "primary")
    SCRN_BOOL(minimalClock24h,     "charging/minimalClock24h",        true)
    SCRN_STRING(minimalTimeColor,  "charging/minimalTimeColor",       "#d0d0d0")
    SCRN_STRING(minimalDateColor,  "charging/minimalDateColor",       "#666666")

    // === Starfield theme ===
    SCRN_INT(starfieldSpeed,       "charging/starfieldSpeed",         50)
    SCRN_INT(starfieldDensity,     "charging/starfieldDensity",       50)
    SCRN_STRING(starfieldColor,    "charging/starfieldColor",         "#ffffff")
    SCRN_INT(starfieldStarSize,    "charging/starfieldStarSize",      50)
    SCRN_INT(starfieldTrailLength, "charging/starfieldTrailLength",   50)

    // === Analog theme ===
    // Which hands animate during the native screen-off shutdown sequence.
    // Values: "all" (sec+min+hour spin+fall together) | "main" (min+hour
    // animate; second hand fades opacity to 0 during the sweep phase).
    SCRN_STRING(analogShutoffHands, "charging/analogShutoffHands",    "all")

    // === Matrix shutdown ===
    // Style for the Matrix native screen-off animation. Pure QML — no
    // simulation modifications. The simulation is paused (running=false)
    // during the animation; matrixRain.opacity fades 1→0 over the lead
    // time, then Qt Quick's renderer culls the item entirely (zero GPU
    // work in the final state). Two presets differ only in the easing
    // curve of the opacity fade:
    //   "cascade" (default) — InCubic (slow start, accelerating fade)
    //   "drain"             — OutCubic (fast start, decelerating fade)
    SCRN_STRING(matrixShutoffStyle,    "charging/matrixShutoffStyle",    "cascade")
    // Total fade duration in ms. User-configurable via slider (800-2000).
    SCRN_INT   (matrixShutoffDuration, "charging/matrixShutoffDuration", 1300)

    // === Screen-off animation system (shared across all themes) ===
    // Master on/off + "fire when undocked" gate + shared-overlay style selector.
    // Style values: "fade", "flash", "vignette", "wipe", "theme-native".
    // "theme-native" defers to theme.startScreenOff()/cancelScreenOff()/finalizeScreenOff()
    // when the theme declares providesNativeScreenOff: true; otherwise behaves as "fade".
    SCRN_BOOL  (screenOffEffectEnabled,  "charging/screenOffEffectEnabled",  true)
    SCRN_BOOL  (screenOffEffectUndocked, "charging/screenOffEffectUndocked", false)
    SCRN_STRING(screenOffEffectStyle,    "charging/screenOffEffectStyle",    "theme-native")

    // Empirically-measured dim-phase duration (ms between Normal->Idle
    // and Idle->Low_power). Persisted to QSettings so the measurement
    // survives Popup destruction on undock/redock cycles (otherwise a
    // local Popup property would reset to the 3000 ms seed on every
    // docking, causing a visible "black but on" gap between animation
    // end and the core's actual display-off transition).
    SCRN_INT   (measuredDimPhaseMs,      "charging/measuredDimPhaseMs",      3000)

    // === TV Static theme ===
    SCRN_INT(tvStaticIntensity,         "charging/tvStaticIntensity",         70)
    SCRN_INT(tvStaticSnowSize,          "charging/tvStaticSnowSize",          2)   // 1..8 px per cell
    SCRN_INT(tvStaticScanlineStrength,  "charging/tvStaticScanlineStrength",  35)
    SCRN_INT(tvStaticScanlineSpeed,     "charging/tvStaticScanlineSpeed",     0)
    SCRN_INT(tvStaticChromaAmount,      "charging/tvStaticChromaAmount",      25)
    SCRN_BOOL(tvStaticTrackingEnable,   "charging/tvStaticTrackingEnable",    true)
    SCRN_INT(tvStaticTrackingSpeed,     "charging/tvStaticTrackingSpeed",     40)
    // --- Channel-flash effect ---
    SCRN_BOOL(tvStaticFlashOnTap,       "charging/tvStaticFlashOnTap",        true) // tap/touch fires flash
    SCRN_BOOL(tvStaticChannelFlashAuto, "charging/tvStaticChannelFlashAuto",  true) // periodic auto bursts
    SCRN_INT(tvStaticFlashInterval,     "charging/tvStaticFlashInterval",     20)   // seconds, ±50% jitter
    SCRN_INT(tvStaticFlashDuration,     "charging/tvStaticFlashDuration",     400)  // ms
    SCRN_INT(tvStaticFlashBrightness,   "charging/tvStaticFlashBrightness",   100)  // 0..100
    SCRN_STRING(tvStaticTint,           "charging/tvStaticTint",              "#ffffff")

    // === Transformed read-only properties (custom getters) ===
    Q_PROPERTY(QColor color READ color NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal speed READ speed NOTIFY speedChanged FINAL)
    Q_PROPERTY(qreal density READ density NOTIFY densityChanged FINAL)
    Q_PROPERTY(qreal fadeRate READ fadeRate NOTIFY fadeRateChanged FINAL)
    Q_PROPERTY(int trailLength READ trailLength NOTIFY trailLengthChanged FINAL)
    Q_PROPERTY(bool showBattery READ showBattery NOTIFY showBatteryChanged FINAL)

public:
    explicit ScreensaverConfig(QObject *parent = nullptr);

    /// @brief QML singleton factory -- registered via qmlRegisterSingletonType.
    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);
    /// @brief C++ singleton accessor. Returns null before construction.
    static ScreensaverConfig *instance() { return s_instance; }

    /// @name Transformed getters
    /// These apply value transforms to raw QSettings integers before exposing to QML.
    /// @{
    QColor color() const;
    qreal  speed() const;
    qreal  density() const;
    qreal  fadeRate() const;
    int    trailLength() const;
    bool   showBattery() const;
    /// @}

signals:
    void colorChanged();
    void speedChanged();
    void densityChanged();
    void fadeRateChanged();
    void trailLengthChanged();
    void showBatteryChanged();

private:
    static ScreensaverConfig *s_instance;
    QSettings *m_settings;
};

// Clean up macros — not needed outside this header
#undef SCRN_BOOL
#undef SCRN_INT
#undef SCRN_STRING

}  // namespace uc
