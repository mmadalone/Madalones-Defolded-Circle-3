// Copyright (c) 2026 madalone. ScreensaverConfig — owns all screensaver QSettings storage.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screensaverconfig.h"

#include <QCoreApplication>
#include <QTimer>
#include <QtMath>

#include "../hardware/battery.h"

namespace uc {

ScreensaverConfig *ScreensaverConfig::s_instance = nullptr;

ScreensaverConfig::ScreensaverConfig(QObject *parent)
    : QObject(parent) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    // Own QSettings instance — same config.ini file, non-overlapping charging/* keys
    QString configPath = qgetenv("UC_CONFIG_HOME");
    if (configPath.isEmpty())
        configPath = QCoreApplication::applicationDirPath();
    m_settings = new QSettings(configPath + "/config.ini", QSettings::IniFormat, this);

    qmlRegisterSingletonType<ScreensaverConfig>(
        "ScreensaverConfig", 1, 0, "ScreensaverConfig",
        &ScreensaverConfig::qmlInstance);

    // Transformed-property forwarding is handled by dual-emission in each
    // raw setter (see set{MatrixColor,MatrixSpeed,MatrixDensity,MatrixFade,
    // MatrixTrail} below). Signal-to-signal connects for these five were
    // removed: the Qt 5.15 + QML binding engine only follows the NOTIFY
    // signal named in Q_PROPERTY directly — indirect signal chains don't
    // trigger QML re-evaluation, and the chain also failed to reach C++
    // consumers in this codebase due to a MOC edge case with macro-expanded
    // Q_SIGNALS blocks mixed with a separate manual `signals:` block.

    // showBattery depends on showBatteryEnabled + batteryDockedOnly + Battery power state
    connect(this, &ScreensaverConfig::showBatteryEnabledChanged, this, &ScreensaverConfig::showBatteryChanged);
    connect(this, &ScreensaverConfig::batteryDockedOnlyChanged,  this, &ScreensaverConfig::showBatteryChanged);

    // Battery may not exist yet at construction; connect when it appears.
    auto *batt = hw::Battery::instance();
    if (batt) {
        connect(batt, &hw::Battery::powerSupplyChanged, this, &ScreensaverConfig::showBatteryChanged);
        connect(batt, &hw::Battery::isChargingChanged,  this, &ScreensaverConfig::showBatteryChanged);
    } else {
        QTimer::singleShot(500, this, [this]() {
            auto *b = hw::Battery::instance();
            if (b) {
                connect(b, &hw::Battery::powerSupplyChanged, this, &ScreensaverConfig::showBatteryChanged);
                connect(b, &hw::Battery::isChargingChanged,  this, &ScreensaverConfig::showBatteryChanged);
                emit showBatteryChanged();
            }
        });
    }
}

QObject *ScreensaverConfig::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)
    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
    return obj;
}

// --- Transformed getters (read from own m_settings) ---

QColor ScreensaverConfig::color() const {
    return QColor(m_settings->value("charging/matrixColor", "#00ff41").toString());
}

qreal ScreensaverConfig::speed() const {
    return qBound(0.2, m_settings->value("charging/matrixSpeed", 50).toInt() / 50.0, 2.0);
}

qreal ScreensaverConfig::density() const {
    return qBound(0.2, m_settings->value("charging/matrixDensity", 70).toInt() / 100.0, 5.0);
}

qreal ScreensaverConfig::fadeRate() const {
    return qBound(0.76, 0.76 + m_settings->value("charging/matrixFade", 60).toInt() * 0.002, 0.96);
}

int ScreensaverConfig::trailLength() const {
    int v = qBound(5, m_settings->value("charging/matrixTrail", 50).toInt(), 100);
    return qMax(5, qRound(5.0 + 175.0 * (v - 10) / 90.0));
}

bool ScreensaverConfig::showBattery() const {
    if (!m_settings->value("charging/showBattery", true).toBool())
        return false;
    if (m_settings->value("charging/batteryDockedOnly", true).toBool()) {
        auto *batt = hw::Battery::instance();
        return batt ? batt->getPowerSupply() : false;
    }
    return true;
}

// --- Raw setters with dual-emit of transformed signals ---
// These replace the SCRN_INT/SCRN_STRING macro-generated setters for the
// five properties that back transformed getters. Each fires BOTH the raw
// NOTIFY signal and the transformed NOTIFY signal, so C++ consumers and
// QML property bindings on either property update correctly.

void ScreensaverConfig::setMatrixColor(const QString &value) {
    if (m_settings->value("charging/matrixColor", "#00ff41").toString() == value) return;
    m_settings->setValue("charging/matrixColor", value);
    emit matrixColorChanged();
    emit colorChanged();
}

void ScreensaverConfig::setMatrixSpeed(int value) {
    if (m_settings->value("charging/matrixSpeed", 50).toInt() == value) return;
    m_settings->setValue("charging/matrixSpeed", value);
    emit matrixSpeedChanged();
    emit speedChanged();
}

void ScreensaverConfig::setMatrixDensity(int value) {
    if (m_settings->value("charging/matrixDensity", 70).toInt() == value) return;
    m_settings->setValue("charging/matrixDensity", value);
    emit matrixDensityChanged();
    emit densityChanged();
}

void ScreensaverConfig::setMatrixFade(int value) {
    if (m_settings->value("charging/matrixFade", 60).toInt() == value) return;
    m_settings->setValue("charging/matrixFade", value);
    emit matrixFadeChanged();
    emit fadeRateChanged();
}

void ScreensaverConfig::setMatrixTrail(int value) {
    if (m_settings->value("charging/matrixTrail", 50).toInt() == value) return;
    m_settings->setValue("charging/matrixTrail", value);
    emit matrixTrailChanged();
    emit trailLengthChanged();
}

// --- resetDefaults: wipe all charging/* keys, then re-broadcast NOTIFY signals so
// QML bindings re-read defaults via the SCRN_*-emitted getters and the hand-written
// transformed getters. Added v1.4.28 to match the API the QML test suite already
// relied on (was previously only on MockScreensaverConfig — see audit-v1.4.26-thorough.md
// Finding 12). Also useful as a user-facing "Reset screensaver to defaults" affordance.
void ScreensaverConfig::resetDefaults() {
    // QSettings::remove("groupName") with a path that doesn't end in '/' removes
    // the whole group — all keys whose name starts with "charging/" will be erased.
    m_settings->remove("charging");
    m_settings->sync();

    // Re-broadcast every NOTIFY signal so QML property bindings re-read defaults.
    // Order roughly matches header declaration order; cheap signals so emit-them-all
    // is cleaner than per-property selective emission. Signals named here must
    // match the NOTIFY identifiers in the matching Q_PROPERTY / SCRN_* declarations.
    emit themeChanged();
    emit showClockChanged();
    emit clockDockedOnlyChanged();
    emit clockSizeChanged();
    emit clockFontChanged();
    emit clockColorChanged();
    emit clockClock24hChanged();
    emit clockDateSizeChanged();
    emit clockShowDateChanged();
    emit clockDateColorChanged();
    emit clockPositionChanged();
    emit showBatteryEnabledChanged();
    emit batteryDockedOnlyChanged();
    emit batteryTextSizeChanged();

    emit matrixColorChanged();
    emit matrixSpeedChanged();
    emit matrixDensityChanged();
    emit matrixFadeChanged();
    emit matrixTrailChanged();
    emit colorModeChanged();
    emit fontSizeChanged();
    emit charsetChanged();

    emit glowChanged();
    emit glowFadeChanged();
    emit depthGlowChanged();
    emit depthGlowMinChanged();
    emit invertTrailChanged();

    emit glitchChanged();
    emit glitchRateChanged();
    emit glitchFlashChanged();
    emit glitchStutterChanged();
    emit glitchReverseChanged();
    emit glitchDirectionChanged();
    emit glitchDirRateChanged();
    emit glitchDirMaskChanged();
    emit glitchDirFadeChanged();
    emit glitchDirSpeedChanged();
    emit glitchDirLengthChanged();
    emit glitchRandomColorChanged();
    emit glitchChaosChanged();
    emit glitchChaosFrequencyChanged();
    emit glitchChaosSurgeChanged();
    emit glitchChaosScrambleChanged();
    emit glitchChaosFreezeChanged();
    emit glitchChaosScatterChanged();
    emit glitchChaosSquareBurstChanged();
    emit glitchChaosSquareBurstSizeChanged();
    emit glitchChaosRippleChanged();
    emit glitchChaosWipeChanged();
    emit glitchChaosIntensityChanged();
    emit glitchChaosScatterRateChanged();
    emit glitchChaosScatterLengthChanged();

    emit directionChanged();
    emit gravityModeChanged();
    emit autoRotateSpeedChanged();
    emit autoRotateBendChanged();

    emit messagesChanged();
    emit messagesEnabledChanged();
    emit messageIntervalChanged();
    emit messageRandomChanged();
    emit messageDirectionChanged();
    emit messageFlashChanged();
    emit messagePulseChanged();

    emit subliminalChanged();
    emit subliminalIntervalChanged();
    emit subliminalDurationChanged();
    emit subliminalStreamChanged();
    emit subliminalOverlayChanged();
    emit subliminalFlashChanged();

    emit tapEnabledChanged();
    emit tapBurstChanged();
    emit tapBurstCountChanged();
    emit tapBurstLengthChanged();
    emit tapFlashChanged();
    emit tapScrambleChanged();
    emit tapSpawnChanged();
    emit tapSpawnCountChanged();
    emit tapSpawnLengthChanged();
    emit tapMessageChanged();
    emit tapSquareBurstChanged();
    emit tapSquareBurstSizeChanged();
    emit tapRippleChanged();
    emit tapWipeChanged();
    emit tapRandomizeChanged();
    emit tapRandomizeChanceChanged();

    emit tapToCloseChanged();
    emit motionToCloseChanged();
    emit idleEnabledChanged();
    emit idleTimeoutChanged();
    emit reopenWhileDockedSecChanged();
    emit dpadEnabledChanged();
    emit dpadPersistChanged();
    emit dpadTouchbarSpeedChanged();
    emit tapDirectionChanged();
    emit tapSwipeSpeedChanged();
    emit lastDirectionChanged();

    emit depthEnabledChanged();
    emit depthIntensityChanged();
    emit depthOverlayChanged();
    emit layersEnabledChanged();

    emit minimalClockSizeChanged();
    emit minimalDateSizeChanged();
    emit minimalFontChanged();
    emit minimalClock24hChanged();
    emit minimalTimeColorChanged();
    emit minimalDateColorChanged();

    emit starfieldSpeedChanged();
    emit starfieldDensityChanged();
    emit starfieldColorChanged();
    emit starfieldStarSizeChanged();
    emit starfieldTrailLengthChanged();

    emit analogShutoffHandsChanged();

    emit screenOffEffectEnabledChanged();
    emit screenOffEffectUndockedChanged();
    emit screenOffEffectStyleChanged();
    emit measuredDimPhaseMsChanged();

    emit tvStaticIntensityChanged();
    emit tvStaticSnowSizeChanged();
    emit tvStaticScanlineStrengthChanged();
    emit tvStaticScanlineSpeedChanged();
    emit tvStaticChromaAmountChanged();
    emit tvStaticTrackingEnableChanged();
    emit tvStaticTrackingSpeedChanged();
    emit tvStaticFlashOnTapChanged();
    emit tvStaticChannelFlashAutoChanged();
    emit tvStaticFlashIntervalChanged();
    emit tvStaticFlashDurationChanged();
    emit tvStaticFlashBrightnessChanged();
    emit tvStaticTintChanged();

    emit debugAtlasOverlayChanged();

    // Transformed properties — these read through to the raw values, so they must
    // also re-broadcast even though the raw setters above already emit during
    // normal mutation. After a group-wide remove() the dual-emit path is bypassed.
    emit colorChanged();
    emit speedChanged();
    emit densityChanged();
    emit fadeRateChanged();
    emit trailLengthChanged();
    emit showBatteryChanged();
}

}  // namespace uc
