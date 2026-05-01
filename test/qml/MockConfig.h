// Copyright (c) 2026 madalone. Mock Config singleton for QML tests.
// Production Config (src/config/config.h) has 100+ Q_PROPERTYs. Mock has exactly the
// minimum the chips read. Expand only when a future test needs more — pre-mirroring
// the full surface buys nothing.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class MockConfig : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool showBatteryPercentage READ getShowBatteryPercentage WRITE setShowBatteryPercentage NOTIFY showBatteryPercentageChanged)

 public:
    explicit MockConfig(QObject *parent = nullptr) : QObject(parent) {}

    bool getShowBatteryPercentage() const { return m_showBatteryPercentage; }

    void setShowBatteryPercentage(bool v) {
        if (m_showBatteryPercentage == v) return;
        m_showBatteryPercentage = v;
        emit showBatteryPercentageChanged();
    }

    Q_INVOKABLE void resetDefaults() {
        setShowBatteryPercentage(false);
    }

 signals:
    void showBatteryPercentageChanged();

 private:
    bool m_showBatteryPercentage{false};
};
