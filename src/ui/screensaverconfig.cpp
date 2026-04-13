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

}  // namespace uc
