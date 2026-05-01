// Copyright (c) 2026 madalone. Mock Battery singleton for QML tests.
// Mirrors src/hardware/battery.h Q_PROPERTY signatures + signal arities, but adds
// WRITE setters so QML tests can drive state. Production Battery's Q_PROPERTYs are
// READ-only; production code mutates via internal slots responding to core::Api
// signals. Tests need WRITE access — divergence is intentional.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class MockBattery : public QObject {
    Q_OBJECT

    Q_PROPERTY(int level READ getLevel WRITE setLevel NOTIFY levelChanged)
    Q_PROPERTY(bool low READ getLow WRITE setLow NOTIFY lowChanged)
    Q_PROPERTY(bool isCharging READ getIsCharging WRITE setIsCharging NOTIFY isChargingChanged)
    Q_PROPERTY(bool powerSupply READ getPowerSupply WRITE setPowerSupply NOTIFY powerSupplyChanged)

 public:
    explicit MockBattery(QObject *parent = nullptr) : QObject(parent) {}

    int  getLevel() const { return m_level; }
    bool getLow() const { return m_low; }
    bool getIsCharging() const { return m_isCharging; }
    bool getPowerSupply() const { return m_powerSupply; }

    void setLevel(int v) {
        if (m_level == v) return;
        m_level = v;
        emit levelChanged();
    }
    void setLow(bool v) {
        if (m_low == v) return;
        m_low = v;
        emit lowChanged(v);  // production signal arity: lowChanged(bool)
    }
    void setIsCharging(bool v) {
        if (m_isCharging == v) return;
        m_isCharging = v;
        emit isChargingChanged();
    }
    void setPowerSupply(bool v) {
        if (m_powerSupply == v) return;
        m_powerSupply = v;
        emit powerSupplyChanged(v);
    }

    Q_INVOKABLE void resetDefaults() {
        setLevel(80);
        setLow(false);
        setIsCharging(false);
        setPowerSupply(false);
    }

 signals:
    void levelChanged();
    void lowChanged(bool value);
    void isChargingChanged();
    void powerSupplyChanged(bool value);

 private:
    int  m_level{80};
    bool m_low{false};
    bool m_isCharging{false};
    bool m_powerSupply{false};
};
