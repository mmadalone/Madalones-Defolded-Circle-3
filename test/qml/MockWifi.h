// Copyright (c) 2026 madalone. Mock Wifi + WifiNetwork for QML tests.
// Mirrors src/hardware/wifi.h:98-130 (Wifi) and lines 57-96 (WifiNetwork) Q_PROPERTYs,
// but with two deliberate divergences for testability:
//
// 1. WRITE setters added on Wifi.isConnected (production is READ-only — driven by
//    core::Api signals). Mock's QML tests need WRITE access.
//
// 2. WifiNetwork.signalStrength is NOT CONSTANT in this mock (production declares it
//    CONSTANT). The mock keeps a single persistent MockWifiNetwork parented to MockWifi,
//    and mutates its signalStrength + emits signalStrengthChanged. This is simpler than
//    production's pointer-swap approach (production swaps the WifiNetwork QObject* on
//    every state change), and equivalent for the chip's binding chain since the prop's
//    own NOTIFY triggers re-bind. Single-object lifetime keeps mock allocations bounded.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class MockWifiNetwork : public QObject {
    Q_OBJECT
    Q_PROPERTY(int signalStrength READ getSignalStrength WRITE setSignalStrength NOTIFY signalStrengthChanged)

 public:
    explicit MockWifiNetwork(QObject *parent = nullptr) : QObject(parent) {}

    int getSignalStrength() const { return m_signalStrength; }

    void setSignalStrength(int v) {
        if (m_signalStrength == v) return;
        m_signalStrength = v;
        emit signalStrengthChanged();
    }

 signals:
    void signalStrengthChanged();

 private:
    int m_signalStrength{3};  // GOOD = 3 (matches uc::hw::SignalStrength::Enum::GOOD)
};

class MockWifi : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isConnected READ getIsConnected WRITE setIsConnected NOTIFY isConnectedChanged)
    Q_PROPERTY(QObject *currentNetwork READ getCurrentNetwork NOTIFY currentNetworkChanged)

 public:
    explicit MockWifi(QObject *parent = nullptr)
        : QObject(parent), m_currentNetwork(new MockWifiNetwork(this)) {}

    bool getIsConnected() const { return m_isConnected; }
    QObject *getCurrentNetwork() const { return m_currentNetwork; }

    void setIsConnected(bool v) {
        if (m_isConnected == v) return;
        m_isConnected = v;
        emit isConnectedChanged();
    }

    Q_INVOKABLE void setSignalStrength(int s) {
        m_currentNetwork->setSignalStrength(s);
    }

    Q_INVOKABLE void resetDefaults() {
        setIsConnected(true);
        m_currentNetwork->setSignalStrength(3);  // GOOD
    }

 signals:
    void isConnectedChanged();
    void currentNetworkChanged();  // unused by mock (single-network lifetime) but kept for binding compatibility

 private:
    bool m_isConnected{true};
    MockWifiNetwork *m_currentNetwork;
};
