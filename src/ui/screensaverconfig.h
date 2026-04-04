// Copyright (c) 2024 madalone. Screensaver config bridge — reads from Config singleton,
// applies value transforms, exposes domain-specific properties for MatrixRainItem binding.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QColor>
#include <QQmlEngine>

namespace uc {

class Config;

// ---------------------------------------------------------------------------
// Read-only property macros — getter reads live from Config, signal forwarded.
// These properties are read-only from QML (Config writes via settings pages).
// ---------------------------------------------------------------------------
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

    // ---- Manually declared properties (custom getters with transforms) ----

    Q_PROPERTY(QColor color READ color NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal speed READ speed NOTIFY speedChanged FINAL)
    Q_PROPERTY(qreal density READ density NOTIFY densityChanged FINAL)
    Q_PROPERTY(qreal fadeRate READ fadeRate NOTIFY fadeRateChanged FINAL)
    Q_PROPERTY(int trailLength READ trailLength NOTIFY trailLengthChanged FINAL)
    Q_PROPERTY(bool showBattery READ showBattery NOTIFY showBatteryChanged FINAL)

public:
    explicit ScreensaverConfig(Config *config, QObject *parent = nullptr);

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);
    static ScreensaverConfig *instance() { return s_instance; }

    // Transformed getters
    QColor color() const;
    qreal  speed() const;
    qreal  density() const;
    qreal  fadeRate() const;
    int    trailLength() const;
    bool   showBattery() const;

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
