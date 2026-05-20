// Copyright (c) 2026 madalone. ActivitySessionKeeper — suppresses firmware standby during active media sessions via the firmware's POST /system/power/standby_inhibitors REST API (BLOCK mode).
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
    // madalone: orphan-cleanup hook. Wired to core::Api::connected in main.cpp.
    // Lists existing inhibitors filtered by who == "madalone.session-keeper",
    // deletes any orphans from a prior crashed session, then (if currently m_active)
    // creates a fresh inhibitor.
    void onCoreConnected();

 signals:
    void enabledChanged();
    void activeChanged();
    void idleTimeoutSecChanged();
    void requireAcPowerChanged();

 private:
    void evaluateSession();
    // madalone: activate creates a REST inhibitor on session start; deactivate
    // deletes the stored ID on session end. Hard-fail on REST auth flake — logged
    // loudly, no silent fallback (auto-revert handles the worst-case).
    void activateInhibitor();
    void deactivateInhibitor();

    static ActivitySessionKeeper* s_instance;
    core::Api*                    m_core;

    QSet<QString> m_activeMediaPlayers;
    QTimer        m_idleTimer;   // single-shot, fires when no recent button touch

    bool    m_enabled = false;
    bool    m_active = false;
    bool    m_onAc = false;
    int     m_idleTimeoutSec = 60;
    bool    m_requireAcPower = true;
    QString m_inhibitorId;                     // populated on POST 201, cleared on DELETE 200
    bool    m_inhibitorCreatePending = false;  // POST in flight, waiting for 201
};

}  // namespace hw
}  // namespace uc
