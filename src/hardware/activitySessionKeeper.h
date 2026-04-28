// Copyright (c) 2026 madalone. ActivitySessionKeeper — suppresses firmware standby during active media sessions via periodic set_power_mode pings. Wires the orphan RequestTypes::set_power_mode (enums.h:96).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QSet>
#include <QString>
#include <QTimer>

#include "../core/core.h"

namespace uc {
namespace hw {

class ActivitySessionKeeper : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool enabled READ getEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool active READ getActive NOTIFY activeChanged)
    Q_PROPERTY(int  idleTimeoutSec READ getIdleTimeoutSec WRITE setIdleTimeoutSec NOTIFY idleTimeoutSecChanged)
    Q_PROPERTY(bool requireAcPower READ getRequireAcPower WRITE setRequireAcPower NOTIFY requireAcPowerChanged)

 public:
    explicit ActivitySessionKeeper(core::Api* core, QObject* parent = nullptr);
    ~ActivitySessionKeeper();

    bool getEnabled() const { return m_enabled; }
    bool getActive() const { return m_active; }
    int  getIdleTimeoutSec() const { return m_idleTimeoutSec; }
    bool getRequireAcPower() const { return m_requireAcPower; }

    void setEnabled(bool enabled);
    void setIdleTimeoutSec(int seconds);
    void setRequireAcPower(bool require);

    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);

 public slots:
    void onMediaPlayerStateChanged(QString entityId, int newState);
    void onEntityCommandIssued(QString entityId, QString command);
    void onPowerSupplyChanged(bool onAc);
    void onCoreDisconnected();

 signals:
    void enabledChanged();
    void activeChanged();
    void idleTimeoutSecChanged();
    void requireAcPowerChanged();

 private:
    void evaluateSession();
    void ping();

    static ActivitySessionKeeper* s_instance;
    core::Api*                    m_core;

    QSet<QString> m_activeMediaPlayers;
    QTimer        m_idleTimer;   // single-shot, fires when no recent button touch
    QTimer        m_pingTimer;   // 270 s repeating, fires set_power_mode(NORMAL) while m_active

    bool m_enabled = false;
    bool m_active = false;
    bool m_onAc = false;
    int  m_idleTimeoutSec = 60;
    bool m_requireAcPower = true;
};

}  // namespace hw
}  // namespace uc
