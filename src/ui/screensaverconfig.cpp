// Copyright (c) 2026 madalone. ScreensaverConfig — owns all screensaver QSettings storage.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screensaverconfig.h"

#include "../hardware/battery.h"

#include <QCoreApplication>
#include <QTimer>
#include <QtMath>

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

    // Transformed property forwarding: raw int setter → transformed signal
    // (SCRN_INT setter emits matrixSpeedChanged; we also need speedChanged for QML bindings)
    connect(this, &ScreensaverConfig::matrixSpeedChanged,   this, &ScreensaverConfig::speedChanged);
    connect(this, &ScreensaverConfig::matrixDensityChanged,  this, &ScreensaverConfig::densityChanged);
    connect(this, &ScreensaverConfig::matrixFadeChanged,     this, &ScreensaverConfig::fadeRateChanged);
    connect(this, &ScreensaverConfig::matrixTrailChanged,    this, &ScreensaverConfig::trailLengthChanged);
    connect(this, &ScreensaverConfig::matrixColorChanged,    this, &ScreensaverConfig::colorChanged);

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

}  // namespace uc
