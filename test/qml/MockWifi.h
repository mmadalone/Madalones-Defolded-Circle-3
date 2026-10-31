// Copyright (c) 2026 madalone. Mock Wifi + WifiNetwork for QML tests.
// Mirrors src/system/wifi.h:98-130 (Wifi) and lines 57-96 (WifiNetwork) Q_PROPERTYs,
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
#include <QVariantList>

class MockWifiNetwork : public QObject {
    Q_OBJECT
    Q_PROPERTY(int signalStrength READ getSignalStrength WRITE setSignalStrength NOTIFY signalStrengthChanged)
    Q_PROPERTY(int frequency MEMBER m_frequency NOTIFY frequencyChanged)

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
    void frequencyChanged();

 private:
    int m_signalStrength{3};  // GOOD = 3 (matches uc::hw::SignalStrength::Enum::GOOD)
    int m_frequency{2437};    // channel 6
};

class MockWifi : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isConnected READ getIsConnected WRITE setIsConnected NOTIFY isConnectedChanged)
    Q_PROPERTY(QObject *currentNetwork READ getCurrentNetwork NOTIFY currentNetworkChanged)
    // W2 link details and W13 diagnostics (src/system/wifi.h), writable here so the tests can set them
    Q_PROPERTY(QString currentBssid MEMBER m_currentBssid NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentRssi MEMBER m_currentRssi NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentSnr MEMBER m_currentSnr NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentLinkSpeed MEMBER m_currentLinkSpeed NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentEstimatedThroughput MEMBER m_currentEstimatedThroughput NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(QVariantList rssiHistory MEMBER m_rssiHistory NOTIFY rssiHistoryChanged)
    Q_PROPERTY(int disconnectCount MEMBER m_disconnectCount NOTIFY disconnectCountChanged)
    Q_PROPERTY(qint64 currentSessionDurationSec MEMBER m_currentSessionDurationSec NOTIFY connectionStatsChanged)
    Q_PROPERTY(qint64 secondsSinceLastDisconnect MEMBER m_secondsSinceLastDisconnect NOTIFY connectionStatsChanged)
    Q_PROPERTY(int statusRequests MEMBER m_statusRequests NOTIFY statusRequestsChanged)

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
        m_currentNetwork->setProperty("frequency", 2437);
        m_currentBssid = QStringLiteral("aa:bb:cc:dd:ee:ff");
        m_currentRssi = -55;
        m_currentSnr = 0;
        m_currentLinkSpeed = 72;
        m_currentEstimatedThroughput = 0;
        emit currentLinkInfoChanged();
        m_rssiHistory.clear();
        emit rssiHistoryChanged();
        m_disconnectCount = 0;
        emit disconnectCountChanged();
        m_currentSessionDurationSec = 0;
        m_secondsSinceLastDisconnect = -1;
        emit connectionStatsChanged();
        m_statusRequests = 0;
        emit statusRequestsChanged();
    }

    // the real ones ask the core; here they only count / reset
    Q_INVOKABLE void getWifiStatus() {
        m_statusRequests++;
        emit statusRequestsChanged();
    }
    Q_INVOKABLE void resetDiagnosticCounters() {
        m_disconnectCount = 0;
        emit disconnectCountChanged();
        m_rssiHistory.clear();
        emit rssiHistoryChanged();
    }

 signals:
    void isConnectedChanged();
    void currentNetworkChanged();  // unused by mock (single-network lifetime) but kept for binding compatibility
    void currentLinkInfoChanged();
    void rssiHistoryChanged();
    void disconnectCountChanged();
    void connectionStatsChanged();
    void statusRequestsChanged();

 private:
    bool m_isConnected{true};
    MockWifiNetwork *m_currentNetwork;
    QString      m_currentBssid{QStringLiteral("aa:bb:cc:dd:ee:ff")};
    int          m_currentRssi{-55};
    int          m_currentSnr{0};
    int          m_currentLinkSpeed{72};
    int          m_currentEstimatedThroughput{0};
    QVariantList m_rssiHistory;
    int          m_disconnectCount{0};
    qint64       m_currentSessionDurationSec{0};
    qint64       m_secondsSinceLastDisconnect{-1};
    int          m_statusRequests{0};
};
