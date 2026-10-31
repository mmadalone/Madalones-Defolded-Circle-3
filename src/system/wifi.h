// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. WiFi UX bundle: live diagnostics, reconnect, WoWLAN surfacing, periodic poll, displayOff gate, RSSI history + drop counter (W13).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <QVariantList>
#include <QVector>

#include "../core/core.h"

namespace uc {
namespace hw {

class SignalStrength {
    Q_GADGET

 public:
    enum Enum { NONE, WEAK, OK, GOOD, EXCELLENT };
    Q_ENUM(Enum)

    static Enum fromRssi(int rssi) {
        if (rssi == 0)
            return NONE;
        else if (rssi >= -60)
            return EXCELLENT;
        else if (rssi >= -68)
            return GOOD;
        else if (rssi >= -76)
            return OK;
        else if (rssi >= -84)
            return WEAK;
        else
            return NONE;
    }

 private:
    SignalStrength() {}
};

class Security {
    Q_GADGET

 public:
    /**
     * WiFi security types.
     *
     * OPEN, WPA_PSK, WPA2_WPA3 and WPA3_SAE can be requested when adding a network, AUTO leaves the
     * choice to the core. The remaining values are only used to classify an existing connection.
     */
    enum Enum { OPEN, WPA_PSK, WPA_EAP, WPA2_PSK, WPA2_EAP, WPA2_WPA3, WPA3_SAE, AUTO };
    Q_ENUM(Enum)

 private:
    Security() {}
};

class WifiNetwork : public QObject {
    Q_OBJECT

    Q_PROPERTY(int id READ getId NOTIFY idChanged)
    Q_PROPERTY(QString ssid READ getSsid CONSTANT)
    Q_PROPERTY(QString ssidHex READ getSsidHex CONSTANT)
    Q_PROPERTY(QString identifier READ getIdentifier CONSTANT)
    Q_PROPERTY(uc::hw::SignalStrength::Enum signalStrength READ getSignalStrength NOTIFY signalStrengthChanged)
    Q_PROPERTY(bool encrypted READ isEncrypted NOTIFY securityChanged)
    Q_PROPERTY(uc::hw::Security::Enum security READ getSecurity NOTIFY securityChanged)
    Q_PROPERTY(QString keyManagement READ getKeyManagement NOTIFY ciphersChanged)
    Q_PROPERTY(QString pairwiseCipher READ getPairwiseCipher NOTIFY ciphersChanged)
    Q_PROPERTY(QString groupCipher READ getGroupCipher NOTIFY ciphersChanged)
    Q_PROPERTY(int frequency READ getFrequency NOTIFY frequencyChanged)
    Q_PROPERTY(bool enabled READ getEnabled NOTIFY enabledChanged)

 public:
    explicit WifiNetwork(int id, const QString &ssid, const QString &ssidHex, Security::Enum security, int rssi, const QString keyManagement, const QString pairwiseCipher, const QString groupCipher, int frequency, bool enabled, QObject *parent = nullptr);
    ~WifiNetwork();

    int                  getId() const { return m_id; }
    QString              getSsid() const { return m_ssid; }
    QString              getSsidHex() const { return m_ssidHex; }

    /**
     * Identifies the network by its native SSID, see identifier().
     */
    QString getIdentifier() const { return identifier(m_ssid, m_ssidHex); }

    /**
     * Identifies a network by its native SSID, to compare networks from a scan result, a saved network and the
     * WiFi status.
     *
     * The friendly SSID name is a lossy conversion of the native SSID and is not unique: two networks can share
     * it. If the remote didn't report the native SSID it is derived from the friendly name, which is exact for
     * the valid UTF-8 names it can be derived from.
     */
    static QString identifier(const QString &ssid, const QString &ssidHex) {
        return ssidHex.isEmpty() ? QString::fromLatin1(ssid.toUtf8().toHex()) : ssidHex;
    }
    SignalStrength::Enum getSignalStrength() const { return m_signalStrenght; }
    bool                 isEncrypted() const { return m_security != Security::OPEN; }
    Security::Enum       getSecurity() const { return m_security; }
    QString              getKeyManagement() const { return m_keyManagement; }
    QString              getPairwiseCipher() const { return m_pairwiseCipher; }
    QString              getGroupCipher() const { return m_groupCipher; }
    int                  getFrequency() const { return m_frequency; }
    bool                 getEnabled() const { return m_enabled; }

    /**
     * Update the network in place from a new scan result or saved-network list, emitting only the change
     * signals of the properties that differ. Keeping the object lets the QML list keep its delegates - and the
     * keypad selection - instead of rebuilding them on every scan.
     */
    void update(int id, Security::Enum security, int rssi, const QString &keyManagement, const QString &pairwiseCipher, const QString &groupCipher, int frequency, bool enabled);

    /**
     * Ordering for the network lists shown in the UI: strongest signal first, then by name. A stable order keeps
     * the list from being reshuffled by the QHash iteration order every time it is handed to QML.
     */
    static bool lessThan(const WifiNetwork *a, const WifiNetwork *b);

 signals:
    void idChanged();
    void signalStrengthChanged();
    void securityChanged();
    void ciphersChanged();
    void frequencyChanged();
    void enabledChanged();

 private:
    int                  m_id;
    QString              m_ssid;
    QString              m_ssidHex;
    Security::Enum       m_security;
    SignalStrength::Enum m_signalStrenght;
    QString              m_keyManagement;
    QString              m_pairwiseCipher;
    QString              m_groupCipher;
    int                  m_frequency;
    bool                 m_enabled;
};

class Wifi : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isConnected READ getIsConnected NOTIFY isConnectedChanged)
    Q_PROPERTY(QObject *currentNetwork READ getCurrentNetwork NOTIFY currentNetworkChanged)
    Q_PROPERTY(QString ipAddress READ getIpAddress NOTIFY ipAddressChanged)
    Q_PROPERTY(QString macAddress READ getMac NOTIFY macAddressChanged)
    Q_PROPERTY(QList<WifiNetwork *> networkList READ getNetworkList NOTIFY networkListChanged)
    Q_PROPERTY(QList<WifiNetwork *> knownNetworkList READ getKnownNetworkList NOTIFY knownNetworkListChanged)
    Q_PROPERTY(bool scanActive READ getScanActive NOTIFY scanActiveChanged);
    Q_PROPERTY(bool wowlanEnabled READ isWowlan CONSTANT)
    Q_PROPERTY(QString currentBssid READ getCurrentBssid NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentRssi READ getCurrentRssi NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentAverageRssi READ getCurrentAverageRssi NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentNoise READ getCurrentNoise NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentSnr READ getCurrentSnr NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentLinkSpeed READ getCurrentLinkSpeed NOTIFY currentLinkInfoChanged)
    Q_PROPERTY(int currentEstimatedThroughput READ getCurrentEstimatedThroughput NOTIFY currentLinkInfoChanged)
    // madalone (W13, v1.4.17): diagnostics surface
    Q_PROPERTY(QVariantList rssiHistory READ getRssiHistory NOTIFY rssiHistoryChanged)
    Q_PROPERTY(int disconnectCount READ getDisconnectCount NOTIFY disconnectCountChanged)
    Q_PROPERTY(qint64 currentSessionDurationSec READ getCurrentSessionDurationSec NOTIFY connectionStatsChanged)
    Q_PROPERTY(qint64 secondsSinceLastDisconnect READ getSecondsSinceLastDisconnect NOTIFY connectionStatsChanged)

 public:
    explicit Wifi(core::Api *core, QObject *parent = nullptr);
    ~Wifi();

    // Q_PROPERTY methods
    bool                 getIsConnected() { return m_isConnected; }
    QObject             *getCurrentNetwork() { return m_currentNetwork; }
    QString              getIpAddress() { return m_ipAddress; }
    QString              getMac() { return m_mac; }
    QList<WifiNetwork *> getNetworkList();
    QList<WifiNetwork *> getKnownNetworkList();
    bool                 getScanActive() { return m_scanActive; }
    bool                 isWowlan() const { return m_wowlan; }
    QString              getCurrentBssid() const { return m_currentBssid; }
    int                  getCurrentRssi() const { return m_currentRssi; }
    int                  getCurrentAverageRssi() const { return m_currentAverageRssi; }
    int                  getCurrentNoise() const { return m_currentNoise; }
    int                  getCurrentSnr() const { return m_currentSnr; }
    int                  getCurrentLinkSpeed() const { return m_currentLinkSpeed; }
    int                  getCurrentEstimatedThroughput() const { return m_currentEstimatedThroughput; }
    QVariantList         getRssiHistory() const;
    int                  getDisconnectCount() const { return m_disconnectCount; }
    qint64               getCurrentSessionDurationSec() const;
    qint64               getSecondsSinceLastDisconnect() const;

    Q_INVOKABLE void turnOn();
    Q_INVOKABLE void turnOff();
    Q_INVOKABLE void connect(const QString &ssid, const QString &ssidHex, const QString &password,
                             uc::hw::Security::Enum security = uc::hw::Security::AUTO, bool hidden = false);
    Q_INVOKABLE void connectSavedNetwork(int id);
    Q_INVOKABLE void enableSavedNetwork(int id, bool enable);
    Q_INVOKABLE void disconnect();
    Q_INVOKABLE void reassociate();
    Q_INVOKABLE void setDisplayOff(bool off);
    Q_INVOKABLE void resetDiagnosticCounters();   // madalone (W13): zero counters + clear ring buffer

    Q_INVOKABLE void getWifiStatus();
    Q_INVOKABLE void startNetworkScan();
    Q_INVOKABLE void getWifiScanStatus();
    Q_INVOKABLE void stopNetworkScan();
    Q_INVOKABLE void clearNetworkList();

    Q_INVOKABLE void getAllWifiNetworks();
    Q_INVOKABLE void deleteSavedNetwork(const QString &identifier);
    Q_INVOKABLE void deleteAllNetworks();

    Q_INVOKABLE QString getLastConnectedSsid() { return m_lastConnectedSSid; }
    Q_INVOKABLE QString getLastConnectedPassword() { return m_lastConnectedPassword; }

    void addNetwork(const QString &ssid, const QString &ssidHex, const QString &password,
                    uc::hw::Security::Enum security = uc::hw::Security::AUTO, bool hidden = false);

    static core::WifiEnums::WifiSecurity toApiSecurity(uc::hw::Security::Enum security);
    /**
     * Security type of the current connection from the key management string of the wifi status
     * ("WPA2-PSK", "SAE", "NONE", ...). Never -1: an unknown string is encrypted, an empty one or NONE is open.
     */
    static uc::hw::Security::Enum        securityFromKeyManagement(const QString &keyManagement);
    static uc::hw::Security::Enum        fromApiSecurity(core::WifiEnums::WifiSecurity security);
    void updateNetworkList(bool scanActive, const QList<core::AccessPointScan> &scan);
    void wifiNetworkCommand(int networkId, core::WifiEnums::WifiNetworkCmd command);
    void wifiCommand(core::WifiEnums::WifiCmd command);
    void clearKnownNetworkList();

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void isConnectedChanged();
    void ipAddressChanged();
    void macAddressChanged();
    void currentNetworkChanged();
    void networkListChanged();
    void knownNetworkListChanged();
    void scanActiveChanged();
    void scanFailed();
    void connecting();
    void connected(bool success);
    void networkNotFound();
    void wrongKey();
    void currentLinkInfoChanged();
    // madalone (W13, v1.4.17)
    void rssiHistoryChanged();
    void disconnectCountChanged();
    void connectionStatsChanged();   // 1 Hz tick + on DISCONNECTED/CONNECTED transitions

 private:
    static Wifi *s_instance;
    core::Api   *m_core;

    bool         m_isConnected = true;
    WifiNetwork *m_currentNetwork = nullptr;
    QString      m_ipAddress;
    QString      m_mac;

    QHash<QString, WifiNetwork *> m_networkList;
    QHash<QString, WifiNetwork *> m_knownNetworkList;
    bool                          m_scanActive = true;
    bool                          m_wowlan = false;

    QString m_currentBssid;
    int     m_currentRssi = 0;
    int     m_currentAverageRssi = 0;
    int     m_currentNoise = 0;
    int     m_currentSnr = 0;
    int     m_currentLinkSpeed = 0;
    int     m_currentEstimatedThroughput = 0;

    QTimer  m_statusPollTimer;          // 30 s periodic getWifiStatus while connected + display awake
    bool    m_displayOff = false;       // mirrored from core::Api::powerModeChanged

    // madalone (W13, v1.4.17): diagnostics surface
    static const int kRssiHistoryMax = 60;   // 5 min @ 5 s polling = 60 samples; or 30 min @ 30 s background poll
    QVector<int> m_rssiHistory;              // ring of last N RSSI samples (-100..-30 dBm typical)
    int          m_disconnectCount = 0;
    qint64       m_currentSessionStartMs = 0;   // QDateTime::currentMSecsSinceEpoch when CONNECTED (or boot init)
    qint64       m_lastDisconnectMs = 0;        // 0 = none since boot
    QTimer       m_statsTickTimer;              // 1 Hz, fires connectionStatsChanged so QML uptime label re-evaluates

 private:
    QString m_lastConnectedSSid;
    QString m_lastConnectedPassword;

 private slots:
    void onWifiEventChanged(core::WifiEvent::Enum wifiEvent);
};

}  // namespace hw
}  // namespace uc
