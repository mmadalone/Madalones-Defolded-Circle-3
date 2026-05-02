// Copyright (c) 2026 madalone. ActivitySessionKeeper — suppresses firmware standby during active media sessions via periodic set_power_mode pings. Wires the orphan RequestTypes::set_power_mode (enums.h:96).
// SPDX-License-Identifier: GPL-3.0-or-later

#include "activitySessionKeeper.h"

#include <QCoreApplication>

#include "../logging.h"
#include "../ui/entity/mediaPlayer.h"

namespace uc {
namespace hw {

ActivitySessionKeeper* ActivitySessionKeeper::s_instance = nullptr;

ActivitySessionKeeper::ActivitySessionKeeper(core::Api* core, QObject* parent)
    : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    // Single-shot idle timer — fires when no curated button touch has happened for m_idleTimeoutSec.
    m_idleTimer.setSingleShot(true);
    QObject::connect(&m_idleTimer, &QTimer::timeout, this, [this] { evaluateSession(); });

    // Repeating ping — 270 s gives 30 s margin under the firmware's 300 s standby_timeout_sec.
    // The PUT only resets the timer to 300 on a LOW_POWER/IDLE → NORMAL transition; while we're
    // already in NORMAL the firmware countdown drains naturally, so cadence ≤ 300 s is required.
    m_pingTimer.setInterval(270000);
    QObject::connect(&m_pingTimer, &QTimer::timeout, this, [this] { ping(); });

    // Defensive: clear all session state on core disconnect — connection's down anyway,
    // and if it comes back the natural triggers (media playing / button activity) re-arm us.
    QObject::connect(m_core, &core::Api::disconnected, this, &ActivitySessionKeeper::onCoreDisconnected);

    // Clean shutdown — stop timers so we don't fire pings during process teardown.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        m_idleTimer.stop();
        m_pingTimer.stop();
    });
}

ActivitySessionKeeper::~ActivitySessionKeeper() { s_instance = nullptr; }

QObject* ActivitySessionKeeper::qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine) {
    Q_UNUSED(scriptEngine)
    QObject* obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
    return obj;
}

void ActivitySessionKeeper::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    emit enabledChanged();
    qCDebug(lcHw()) << "ActivitySessionKeeper enabled:" << enabled;
    evaluateSession();
}

void ActivitySessionKeeper::setIdleTimeoutSec(int seconds) {
    if (m_idleTimeoutSec == seconds) return;
    m_idleTimeoutSec = seconds;
    emit idleTimeoutSecChanged();
    if (m_idleTimer.isActive()) {
        m_idleTimer.start(m_idleTimeoutSec * 1000);  // restart with new interval
    }
}

void ActivitySessionKeeper::setRequireAcPower(bool require) {
    if (m_requireAcPower == require) return;
    m_requireAcPower = require;
    emit requireAcPowerChanged();
    evaluateSession();
}

void ActivitySessionKeeper::onMediaPlayerStateChanged(QString entityId, int newState) {
    if (newState == ui::entity::MediaPlayerStates::Playing) {
        m_activeMediaPlayers.insert(entityId);
    } else {
        m_activeMediaPlayers.remove(entityId);
    }
    evaluateSession();
}

void ActivitySessionKeeper::onEntityCommandIssued(QString entityId, QString command) {
    Q_UNUSED(entityId)
    // Curated allowlist — only the human-facing media-control commands count as "active session".
    // Avoids inhibit-on-poll-noise from state queries / capability fetches.
    static const QSet<QString> kKeepers = {
        "PLAY_PAUSE", "PLAY", "STOP", "PAUSE", "SEEK",
        "VOLUME", "VOLUME_UP", "VOLUME_DOWN", "MUTE_TOGGLE", "MUTE", "UNMUTE",
        "CURSOR_UP", "CURSOR_DOWN", "CURSOR_LEFT", "CURSOR_RIGHT", "CURSOR_ENTER",
        "CHANNEL_UP", "CHANNEL_DOWN",
        "NEXT", "PREVIOUS", "FAST_FORWARD", "REWIND",
    };
    if (!kKeepers.contains(command.toUpper())) return;
    m_idleTimer.start(m_idleTimeoutSec * 1000);
    evaluateSession();
}

void ActivitySessionKeeper::onPowerSupplyChanged(bool onAc) {
    if (m_onAc == onAc) return;
    m_onAc = onAc;
    evaluateSession();
}

void ActivitySessionKeeper::onCoreDisconnected() {
    m_activeMediaPlayers.clear();
    m_idleTimer.stop();
    m_pingTimer.stop();
    // M1: WS is down — DELETE would fail. Just clear local state. Orphan cleanup
    // on reconnect (onCoreConnected) handles the firmware-side inhibitor.
    m_inhibitorId.clear();
    m_inhibitorCreatePending = false;
    if (m_active) {
        m_active = false;
        emit activeChanged();
    }
}

// madalone (M1, 2026-05-03): orphan cleanup on (re)connect.
// Lists existing inhibitors, deletes any with who=="madalone.session-keeper" left
// behind by a prior process instance, then if currently m_active recreates fresh.
void ActivitySessionKeeper::onCoreConnected() {
    if (!m_useInhibitorApi) return;

    const int reqId = m_core->listStandbyInhibitors();
    auto* conn = new QMetaObject::Connection;
    *conn = QObject::connect(m_core, &core::Api::respListInhibitors, this,
        [this, reqId, conn](int respId, int httpCode, QList<core::Inhibitor> inhibitors, QString errorMessage) {
            if (respId != reqId) return;
            QObject::disconnect(*conn);
            delete conn;
            if (httpCode != 200) {
                qCWarning(lcHw()) << "ActivitySessionKeeper orphan-list GET failed:" << httpCode << errorMessage;
                return;
            }
            int orphans = 0;
            for (const auto& inh : inhibitors) {
                if (inh.who == QStringLiteral("madalone.session-keeper")) {
                    qCDebug(lcHw()) << "ActivitySessionKeeper orphan cleanup; deleting" << inh.id;
                    m_core->deleteStandbyInhibitor(inh.id);
                    ++orphans;
                }
            }
            // Local state was already cleared on disconnect; cleanup is firmware-side only.
            if (orphans > 0) {
                qCWarning(lcHw()) << "ActivitySessionKeeper orphan cleanup deleted" << orphans << "inhibitor(s)";
            }
            // Recreate fresh if we're currently in an active session.
            if (m_active) {
                activateInhibitor();
            }
        });
}

void ActivitySessionKeeper::evaluateSession() {
    const bool wasActive = m_active;
    m_active = m_enabled
            && (m_onAc || !m_requireAcPower)
            && (!m_activeMediaPlayers.isEmpty() || m_idleTimer.isActive());

    if (m_active != wasActive) {
        qCDebug(lcHw()) << "ActivitySessionKeeper active:" << m_active
                       << "(media=" << m_activeMediaPlayers.size() << "idle=" << m_idleTimer.isActive() << ")";
        emit activeChanged();
    }

    // M1: dispatch effector by toggle. WS ping path = legacy default; REST inhibitor = opt-in.
    if (m_active && !wasActive) {
        // inactive → active edge
        if (m_useInhibitorApi) {
            activateInhibitor();
        } else if (!m_pingTimer.isActive()) {
            ping();              // immediate first ping resets firmware timer to 300 s now
            m_pingTimer.start();
        }
    } else if (!m_active && wasActive) {
        // active → inactive edge
        if (m_useInhibitorApi) {
            deactivateInhibitor();
        } else if (m_pingTimer.isActive()) {
            m_pingTimer.stop();
        }
    }
}

// madalone (M1, 2026-05-03): mid-session toggle flip support.
// User flipping the experimental REST toggle while an active session is in flight:
// - WS → REST: stop ping timer, send POST.
// - REST → WS: send DELETE, restart ping timer + immediate ping.
// Toggle generally OFF in practice; this is the rare-but-correct path.
void ActivitySessionKeeper::setUseInhibitorApi(bool use) {
    if (m_useInhibitorApi == use) return;
    m_useInhibitorApi = use;
    emit useInhibitorApiChanged();
    qCDebug(lcHw()) << "ActivitySessionKeeper useInhibitorApi:" << use;

    if (!m_active) return;  // no session in flight — nothing to swap

    if (use) {
        // WS → REST
        m_pingTimer.stop();
        activateInhibitor();
    } else {
        // REST → WS
        deactivateInhibitor();
        ping();
        m_pingTimer.start();
    }
}

// madalone (M1): activate effector — POST a BLOCK inhibitor. Hard-fail on auth flake.
void ActivitySessionKeeper::activateInhibitor() {
    if (m_inhibitorCreatePending) return;  // POST already in flight
    if (!m_inhibitorId.isEmpty()) return;  // already have one

    m_inhibitorCreatePending = true;
    const int reqId = m_core->createStandbyInhibitor(
        QStringLiteral("madalone.session-keeper"),
        QStringLiteral("media-playing or recent-input"));

    auto* conn = new QMetaObject::Connection;
    *conn = QObject::connect(m_core, &core::Api::respCreateInhibitor, this,
        [this, reqId, conn](int respId, int httpCode, QString id, QString errorMessage) {
            if (respId != reqId) return;
            QObject::disconnect(*conn);
            delete conn;
            m_inhibitorCreatePending = false;
            if (httpCode == 201 && !id.isEmpty()) {
                m_inhibitorId = id;
                qCDebug(lcHw()) << "ActivitySessionKeeper inhibitor created; id =" << id;
                // Race: we may have gone inactive while POST was in flight.
                if (!m_active) {
                    qCDebug(lcHw()) << "ActivitySessionKeeper: session ended during POST; deleting immediately";
                    deactivateInhibitor();
                }
            } else {
                // Hard-fail per audit: log loudly, do NOT silently fall back to ping path.
                qCWarning(lcHw()) << "ActivitySessionKeeper REST POST failed:" << httpCode << errorMessage;
            }
        });
}

// madalone (M1): deactivate effector — DELETE the stored inhibitor. Optimistic clear.
void ActivitySessionKeeper::deactivateInhibitor() {
    if (m_inhibitorId.isEmpty()) return;
    const QString id = m_inhibitorId;
    m_inhibitorId.clear();  // optimistic — orphan cleanup catches a failed DELETE on next reconnect

    const int reqId = m_core->deleteStandbyInhibitor(id);
    auto* conn = new QMetaObject::Connection;
    *conn = QObject::connect(m_core, &core::Api::respDeleteInhibitor, this,
        [reqId, conn, id](int respId, int httpCode, QString errorMessage) {
            if (respId != reqId) return;
            QObject::disconnect(*conn);
            delete conn;
            if (httpCode == 200 || httpCode == 204) {
                qCDebug(lcHw()) << "ActivitySessionKeeper inhibitor deleted; id =" << id;
            } else {
                qCWarning(lcHw()) << "ActivitySessionKeeper REST DELETE failed:" << httpCode
                                  << "id =" << id << "msg =" << errorMessage;
            }
        });
}

void ActivitySessionKeeper::ping() {
    qCDebug(lcHw()) << "ActivitySessionKeeper ping → set_power_mode(NORMAL)";
    int id = m_core->setPowerMode(core::PowerEnums::PowerMode::NORMAL);
    m_core->onResult(
        id,
        []() {  // success — silent
        },
        [](int code, QString message) {
            qCWarning(lcHw()) << "ActivitySessionKeeper ping failed:" << code << message;
        });
}

}  // namespace hw
}  // namespace uc
