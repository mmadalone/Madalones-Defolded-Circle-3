// Copyright (c) 2024 madalone. ScreensaverConfig — bridges Config to MatrixRainItem.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screensaverconfig.h"

#include "../config/config.h"
#include "../hardware/battery.h"

#include <QtMath>

namespace uc {

ScreensaverConfig *ScreensaverConfig::s_instance = nullptr;

ScreensaverConfig::ScreensaverConfig(Config *config, QObject *parent)
    : QObject(parent), m_config(config) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    qmlRegisterSingletonType<ScreensaverConfig>(
        "ScreensaverConfig", 1, 0, "ScreensaverConfig",
        &ScreensaverConfig::qmlInstance);

    connectSignals();
}

QObject *ScreensaverConfig::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)
    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
    return obj;
}

// --- Transformed getters ---

QColor ScreensaverConfig::color() const {
    return QColor(m_config->getChargingMatrixColor());
}

qreal ScreensaverConfig::speed() const {
    return qBound(0.2, m_config->getChargingMatrixSpeed() / 50.0, 2.0);
}

qreal ScreensaverConfig::density() const {
    return qBound(0.2, m_config->getChargingMatrixDensity() / 100.0, 5.0);
}

qreal ScreensaverConfig::fadeRate() const {
    return qBound(0.76, 0.76 + m_config->getChargingMatrixFade() * 0.002, 0.96);
}

int ScreensaverConfig::trailLength() const {
    int v = qBound(10, m_config->getChargingMatrixTrail(), 100);
    return qMax(5, qRound(5.0 + 175.0 * (v - 10) / 90.0));
}

bool ScreensaverConfig::showBattery() const {
    if (!m_config->getChargingShowBattery())
        return false;
    if (m_config->getChargingBatteryDockedOnly()) {
        auto *batt = hw::Battery::instance();
        return batt ? batt->getPowerSupply() : false;
    }
    return true;
}

// --- Signal forwarding ---

void ScreensaverConfig::connectSignals() {
    // Transformed properties — signal forwarding (getter recomputes on read)
    connect(m_config, &Config::chargingMatrixColorChanged,     this, &ScreensaverConfig::colorChanged);
    connect(m_config, &Config::chargingMatrixSpeedChanged,     this, &ScreensaverConfig::speedChanged);
    connect(m_config, &Config::chargingMatrixDensityChanged,   this, &ScreensaverConfig::densityChanged);
    connect(m_config, &Config::chargingMatrixFadeChanged,      this, &ScreensaverConfig::fadeRateChanged);
    connect(m_config, &Config::chargingMatrixTrailChanged,     this, &ScreensaverConfig::trailLengthChanged);

    // showBattery depends on 3 sources
    connect(m_config, &Config::chargingShowBatteryChanged,     this, &ScreensaverConfig::showBatteryChanged);
    connect(m_config, &Config::chargingBatteryDockedOnlyChanged, this, &ScreensaverConfig::showBatteryChanged);
    // Battery may not exist yet at construction; connect when it appears
    auto *batt = hw::Battery::instance();
    if (batt) {
        connect(batt, &hw::Battery::powerSupplyChanged, this, &ScreensaverConfig::showBatteryChanged);
    }

    // Theme + overlays
    connect(m_config, &Config::chargingThemeChanged,           this, &ScreensaverConfig::themeChanged);
    connect(m_config, &Config::chargingShowClockChanged,       this, &ScreensaverConfig::showClockChanged);

    // Core appearance (direct)
    connect(m_config, &Config::chargingMatrixColorModeChanged, this, &ScreensaverConfig::colorModeChanged);
    connect(m_config, &Config::chargingMatrixFontSizeChanged,  this, &ScreensaverConfig::fontSizeChanged);
    connect(m_config, &Config::chargingMatrixCharsetChanged,   this, &ScreensaverConfig::charsetChanged);

    // Visual effects
    connect(m_config, &Config::chargingMatrixGlowChanged,      this, &ScreensaverConfig::glowChanged);
    connect(m_config, &Config::chargingMatrixInvertTrailChanged, this, &ScreensaverConfig::invertTrailChanged);

    // Glitch
    connect(m_config, &Config::chargingMatrixGlitchChanged,              this, &ScreensaverConfig::glitchChanged);
    connect(m_config, &Config::chargingMatrixGlitchRateChanged,          this, &ScreensaverConfig::glitchRateChanged);
    connect(m_config, &Config::chargingMatrixGlitchFlashChanged,         this, &ScreensaverConfig::glitchFlashChanged);
    connect(m_config, &Config::chargingMatrixGlitchStutterChanged,       this, &ScreensaverConfig::glitchStutterChanged);
    connect(m_config, &Config::chargingMatrixGlitchReverseChanged,       this, &ScreensaverConfig::glitchReverseChanged);
    connect(m_config, &Config::chargingMatrixGlitchDirectionChanged,     this, &ScreensaverConfig::glitchDirectionChanged);
    connect(m_config, &Config::chargingMatrixGlitchDirRateChanged,       this, &ScreensaverConfig::glitchDirRateChanged);
    connect(m_config, &Config::chargingMatrixGlitchDirMaskChanged,       this, &ScreensaverConfig::glitchDirMaskChanged);
    connect(m_config, &Config::chargingMatrixGlitchDirFadeChanged,       this, &ScreensaverConfig::glitchDirFadeChanged);
    connect(m_config, &Config::chargingMatrixGlitchDirSpeedChanged,      this, &ScreensaverConfig::glitchDirSpeedChanged);
    connect(m_config, &Config::chargingMatrixGlitchDirLengthChanged,     this, &ScreensaverConfig::glitchDirLengthChanged);
    connect(m_config, &Config::chargingMatrixGlitchRandomColorChanged,   this, &ScreensaverConfig::glitchRandomColorChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosChanged,         this, &ScreensaverConfig::glitchChaosChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosFrequencyChanged, this, &ScreensaverConfig::glitchChaosFrequencyChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosSurgeChanged,    this, &ScreensaverConfig::glitchChaosSurgeChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosScrambleChanged, this, &ScreensaverConfig::glitchChaosScrambleChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosFreezeChanged,   this, &ScreensaverConfig::glitchChaosFreezeChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosScatterChanged,     this, &ScreensaverConfig::glitchChaosScatterChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosSquareBurstChanged,     this, &ScreensaverConfig::glitchChaosSquareBurstChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosSquareBurstSizeChanged, this, &ScreensaverConfig::glitchChaosSquareBurstSizeChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosRippleChanged,        this, &ScreensaverConfig::glitchChaosRippleChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosWipeChanged,          this, &ScreensaverConfig::glitchChaosWipeChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosIntensityChanged, this, &ScreensaverConfig::glitchChaosIntensityChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosScatterRateChanged,   this, &ScreensaverConfig::glitchChaosScatterRateChanged);
    connect(m_config, &Config::chargingMatrixGlitchChaosScatterLengthChanged, this, &ScreensaverConfig::glitchChaosScatterLengthChanged);

    // Direction / gravity
    connect(m_config, &Config::chargingMatrixDirectionChanged,      this, &ScreensaverConfig::directionChanged);
    connect(m_config, &Config::chargingMatrixGravityChanged,        this, &ScreensaverConfig::gravityModeChanged);
    connect(m_config, &Config::chargingMatrixAutoRotateSpeedChanged, this, &ScreensaverConfig::autoRotateSpeedChanged);
    connect(m_config, &Config::chargingMatrixAutoRotateBendChanged,  this, &ScreensaverConfig::autoRotateBendChanged);

    // Messages
    connect(m_config, &Config::chargingMatrixMessagesChanged,         this, &ScreensaverConfig::messagesChanged);
    connect(m_config, &Config::chargingMatrixMessageIntervalChanged,  this, &ScreensaverConfig::messageIntervalChanged);
    connect(m_config, &Config::chargingMatrixMessageRandomChanged,    this, &ScreensaverConfig::messageRandomChanged);
    connect(m_config, &Config::chargingMatrixMessageDirectionChanged, this, &ScreensaverConfig::messageDirectionChanged);
    connect(m_config, &Config::chargingMatrixMessageFlashChanged,     this, &ScreensaverConfig::messageFlashChanged);
    connect(m_config, &Config::chargingMatrixMessagePulseChanged,     this, &ScreensaverConfig::messagePulseChanged);

    // Subliminal
    connect(m_config, &Config::chargingMatrixSubliminalChanged,          this, &ScreensaverConfig::subliminalChanged);
    connect(m_config, &Config::chargingMatrixSubliminalIntervalChanged,  this, &ScreensaverConfig::subliminalIntervalChanged);
    connect(m_config, &Config::chargingMatrixSubliminalDurationChanged,  this, &ScreensaverConfig::subliminalDurationChanged);
    connect(m_config, &Config::chargingMatrixSubliminalStreamChanged,    this, &ScreensaverConfig::subliminalStreamChanged);
    connect(m_config, &Config::chargingMatrixSubliminalOverlayChanged,   this, &ScreensaverConfig::subliminalOverlayChanged);
    connect(m_config, &Config::chargingMatrixSubliminalFlashChanged,     this, &ScreensaverConfig::subliminalFlashChanged);

    // Tap interaction
    connect(m_config, &Config::chargingMatrixTapBurstChanged,            this, &ScreensaverConfig::tapBurstChanged);
    connect(m_config, &Config::chargingMatrixTapBurstCountChanged,     this, &ScreensaverConfig::tapBurstCountChanged);
    connect(m_config, &Config::chargingMatrixTapBurstLengthChanged,    this, &ScreensaverConfig::tapBurstLengthChanged);
    connect(m_config, &Config::chargingMatrixTapFlashChanged,            this, &ScreensaverConfig::tapFlashChanged);
    connect(m_config, &Config::chargingMatrixTapScrambleChanged,         this, &ScreensaverConfig::tapScrambleChanged);
    connect(m_config, &Config::chargingMatrixTapSpawnChanged,            this, &ScreensaverConfig::tapSpawnChanged);
    connect(m_config, &Config::chargingMatrixTapSpawnCountChanged,     this, &ScreensaverConfig::tapSpawnCountChanged);
    connect(m_config, &Config::chargingMatrixTapSpawnLengthChanged,    this, &ScreensaverConfig::tapSpawnLengthChanged);
    connect(m_config, &Config::chargingMatrixTapMessageChanged,          this, &ScreensaverConfig::tapMessageChanged);
    connect(m_config, &Config::chargingMatrixTapSquareBurstChanged,       this, &ScreensaverConfig::tapSquareBurstChanged);
    connect(m_config, &Config::chargingMatrixTapSquareBurstSizeChanged,   this, &ScreensaverConfig::tapSquareBurstSizeChanged);
    connect(m_config, &Config::chargingMatrixTapRippleChanged,           this, &ScreensaverConfig::tapRippleChanged);
    connect(m_config, &Config::chargingMatrixTapWipeChanged,             this, &ScreensaverConfig::tapWipeChanged);
    connect(m_config, &Config::chargingMatrixTapRandomizeChanged,        this, &ScreensaverConfig::tapRandomizeChanged);
    connect(m_config, &Config::chargingMatrixTapRandomizeChanceChanged,  this, &ScreensaverConfig::tapRandomizeChanceChanged);

    // General behavior
    connect(m_config, &Config::chargingTapToCloseChanged,    this, &ScreensaverConfig::tapToCloseChanged);
    connect(m_config, &Config::chargingMotionToCloseChanged, this, &ScreensaverConfig::motionToCloseChanged);
    connect(m_config, &Config::chargingIdleEnabledChanged,   this, &ScreensaverConfig::idleEnabledChanged);
    connect(m_config, &Config::chargingIdleTimeoutChanged,   this, &ScreensaverConfig::idleTimeoutChanged);
    connect(m_config, &Config::chargingMatrixDpadEnabledChanged,    this, &ScreensaverConfig::dpadEnabledChanged);
    connect(m_config, &Config::chargingMatrixTapDirectionChanged,  this, &ScreensaverConfig::tapDirectionChanged);
    connect(m_config, &Config::chargingMatrixLastDirectionChanged,  this, &ScreensaverConfig::lastDirectionChanged);
}

}  // namespace uc
