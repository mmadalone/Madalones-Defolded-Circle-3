// Copyright (c) 2024 madalone. Screensaver config bridge — reads from Config singleton,
// applies value transforms, exposes domain-specific properties for MatrixRainItem binding.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QColor>
#include <QQmlEngine>

#include "../config/config.h"

namespace uc {

/// @name SC_BOOL / SC_INT / SC_STRING macros
/// Read-only Q_PROPERTY generator macros for ScreensaverConfig.
/// Each macro expands to: a Q_PROPERTY declaration, a public getter that reads
/// live from the Config singleton via the specified Getter method, and a signal.
/// Properties are read-only from QML -- Config writes happen through settings pages.
/// @{
#define SC_BOOL(Name, Getter)                                                  \
    Q_PROPERTY(bool Name READ Name NOTIFY Name##Changed FINAL)                 \
public:                                                                        \
    bool Name() const { return m_config->Getter(); }                           \
Q_SIGNALS:                                                                     \
    void Name##Changed();                                                      \
private:

#define SC_INT(Name, Getter)                                                   \
    Q_PROPERTY(int Name READ Name NOTIFY Name##Changed FINAL)                  \
public:                                                                        \
    int Name() const { return m_config->Getter(); }                            \
Q_SIGNALS:                                                                     \
    void Name##Changed();                                                      \
private:

#define SC_STRING(Name, Getter)                                                \
    Q_PROPERTY(QString Name READ Name NOTIFY Name##Changed FINAL)              \
public:                                                                        \
    QString Name() const { return m_config->Getter(); }                        \
Q_SIGNALS:                                                                     \
    void Name##Changed();                                                      \
private:
/// @}

// ---------------------------------------------------------------------------
// ScreensaverConfig — singleton bridge between Config and MatrixRainItem.
//
// Responsibilities:
//   1. Forward Config::chargingMatrix*Changed signals as domain-specific signals
//   2. Apply value transforms (speed/50, density/100, fadeRate formula, trailLength)
//   3. Handle showBattery conditional (dockedOnly + Battery::powerSupply)
//
// QML usage:  import ScreensaverConfig 1.0
//             MatrixRain { speed: ScreensaverConfig.speed }
// ---------------------------------------------------------------------------
/// @brief Singleton bridge between Config (QSettings) and MatrixRainItem.
///
/// Forwards Config change signals as domain-specific signals, applies value
/// transforms (speed/50, density/100, fadeRate formula, trailLength mapping),
/// and handles conditional logic (showBattery dockedOnly check).
class ScreensaverConfig : public QObject {
    Q_OBJECT

    // --- Theme + overlays (shared across all themes) ---
    SC_STRING(theme,      getChargingTheme)
    SC_BOOL(showClock,    getChargingShowClock)
    // showBattery has custom getter (dockedOnly logic) — declared manually below

    // --- Core appearance (transformed) ---
    // speed, density, fadeRate, trailLength have custom getters — declared manually below

    // --- Core appearance (direct pass-through) ---
    SC_STRING(colorMode,  getChargingMatrixColorMode)
    SC_INT(fontSize,      getChargingMatrixFontSize)
    SC_STRING(charset,    getChargingMatrixCharset)

    // --- Visual effects ---
    SC_BOOL(glow,         getChargingMatrixGlow)
    SC_BOOL(invertTrail,  getChargingMatrixInvertTrail)

    // --- Glitch ---
    SC_BOOL(glitch,              getChargingMatrixGlitch)
    SC_INT(glitchRate,           getChargingMatrixGlitchRate)
    SC_BOOL(glitchFlash,         getChargingMatrixGlitchFlash)
    SC_BOOL(glitchStutter,       getChargingMatrixGlitchStutter)
    SC_BOOL(glitchReverse,       getChargingMatrixGlitchReverse)
    SC_BOOL(glitchDirection,     getChargingMatrixGlitchDirection)
    SC_INT(glitchDirRate,        getChargingMatrixGlitchDirRate)
    SC_INT(glitchDirMask,        getChargingMatrixGlitchDirMask)
    SC_INT(glitchDirFade,        getChargingMatrixGlitchDirFade)
    SC_INT(glitchDirSpeed,       getChargingMatrixGlitchDirSpeed)
    SC_INT(glitchDirLength,      getChargingMatrixGlitchDirLength)
    SC_BOOL(glitchRandomColor,   getChargingMatrixGlitchRandomColor)
    SC_BOOL(glitchChaos,         getChargingMatrixGlitchChaos)
    SC_INT(glitchChaosFrequency, getChargingMatrixGlitchChaosFrequency)
    SC_BOOL(glitchChaosSurge,    getChargingMatrixGlitchChaosSurge)
    SC_BOOL(glitchChaosScramble, getChargingMatrixGlitchChaosScramble)
    SC_BOOL(glitchChaosFreeze,   getChargingMatrixGlitchChaosFreeze)
    SC_BOOL(glitchChaosScatter,  getChargingMatrixGlitchChaosScatter)
    SC_INT(glitchChaosIntensity, getChargingMatrixGlitchChaosIntensity)
    SC_INT(glitchChaosScatterRate,   getChargingMatrixGlitchChaosScatterRate)
    SC_INT(glitchChaosScatterLength, getChargingMatrixGlitchChaosScatterLength)

    // --- Direction / gravity ---
    SC_STRING(direction,      getChargingMatrixDirection)
    SC_BOOL(gravityMode,      getChargingMatrixGravity)
    SC_INT(autoRotateSpeed,   getChargingMatrixAutoRotateSpeed)
    SC_INT(autoRotateBend,    getChargingMatrixAutoRotateBend)

    // --- Messages ---
    SC_STRING(messages,         getChargingMatrixMessages)
    SC_INT(messageInterval,     getChargingMatrixMessageInterval)
    SC_BOOL(messageRandom,      getChargingMatrixMessageRandom)
    SC_STRING(messageDirection,  getChargingMatrixMessageDirection)
    SC_BOOL(messageFlash,       getChargingMatrixMessageFlash)
    SC_BOOL(messagePulse,       getChargingMatrixMessagePulse)

    // --- Subliminal ---
    SC_BOOL(subliminal,          getChargingMatrixSubliminal)
    SC_INT(subliminalInterval,   getChargingMatrixSubliminalInterval)
    SC_INT(subliminalDuration,   getChargingMatrixSubliminalDuration)
    SC_BOOL(subliminalStream,    getChargingMatrixSubliminalStream)
    SC_BOOL(subliminalOverlay,   getChargingMatrixSubliminalOverlay)
    SC_BOOL(subliminalFlash,     getChargingMatrixSubliminalFlash)

    // --- Tap interaction flags (read on-demand by ChargingScreen.qml tap handler) ---
    SC_BOOL(tapBurst,            getChargingMatrixTapBurst)
    SC_BOOL(tapFlash,            getChargingMatrixTapFlash)
    SC_BOOL(tapScramble,         getChargingMatrixTapScramble)
    SC_BOOL(tapSpawn,            getChargingMatrixTapSpawn)
    SC_BOOL(tapMessage,          getChargingMatrixTapMessage)
    SC_BOOL(tapRandomize,        getChargingMatrixTapRandomize)
    SC_INT(tapRandomizeChance,   getChargingMatrixTapRandomizeChance)

    // --- General behavior (used by ChargingScreen.qml) ---
    SC_BOOL(tapToClose,     getChargingTapToClose)
    SC_BOOL(motionToClose,  getChargingMotionToClose)
    SC_BOOL(idleEnabled,    getChargingIdleEnabled)
    SC_INT(idleTimeout,     getChargingIdleTimeout)
    SC_BOOL(dpadEnabled,    getChargingMatrixDpadEnabled)
    SC_STRING(lastDirection, getChargingMatrixLastDirection)

    // ---- Manually declared properties (custom getters with transforms) ----

    Q_PROPERTY(QColor color READ color NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal speed READ speed NOTIFY speedChanged FINAL)
    Q_PROPERTY(qreal density READ density NOTIFY densityChanged FINAL)
    Q_PROPERTY(qreal fadeRate READ fadeRate NOTIFY fadeRateChanged FINAL)
    Q_PROPERTY(int trailLength READ trailLength NOTIFY trailLengthChanged FINAL)
    Q_PROPERTY(bool showBattery READ showBattery NOTIFY showBatteryChanged FINAL)

public:
    explicit ScreensaverConfig(Config *config, QObject *parent = nullptr);

    /// @brief QML singleton factory -- registered via qmlRegisterSingletonType.
    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);
    /// @brief C++ singleton accessor. Returns null before construction.
    static ScreensaverConfig *instance() { return s_instance; }

    /// @name Transformed getters
    /// These apply value transforms to raw Config integers before exposing to QML.
    /// @{
    /// @brief Returns QColor parsed from Config's hex string.
    QColor color() const;
    /// @brief Returns Config speed / 50.0 (normalized to ~0.0-2.0 range).
    qreal  speed() const;
    /// @brief Returns Config density / 100.0 (normalized to 0.0-1.0 range).
    qreal  density() const;
    /// @brief Returns transformed fade rate: 0.80 + (Config value / 100.0) * 0.18.
    qreal  fadeRate() const;
    /// @brief Returns Config trail length mapped from percentage to cell count.
    int    trailLength() const;
    /// @brief Returns true if battery overlay should show (respects dockedOnly + power state).
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
    void connectSignals();

    static ScreensaverConfig *s_instance;
    Config *m_config;
};

// Clean up macros — not needed outside this header
#undef SC_BOOL
#undef SC_INT
#undef SC_STRING

}  // namespace uc
