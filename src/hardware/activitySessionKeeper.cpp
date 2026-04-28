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
    if (m_active) {
        m_active = false;
        emit activeChanged();
    }
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

    if (m_active && !m_pingTimer.isActive()) {
        ping();              // immediate first ping resets firmware timer to 300 s now
        m_pingTimer.start();
    } else if (!m_active && m_pingTimer.isActive()) {
        m_pingTimer.stop();
    }
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
