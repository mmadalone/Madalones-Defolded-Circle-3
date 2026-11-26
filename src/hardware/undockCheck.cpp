// Copyright (c) 2026 madalone. UndockCheck — the quick undock check (see undockCheck.h).
// SPDX-License-Identifier: GPL-3.0-or-later

#include "undockCheck.h"

#include "../logging.h"

namespace uc {
namespace hw {

namespace {

enum class Rounding { Down, Nearest, Up };

// A time in ms as seconds with two decimals. A lower bound is rounded down and an upper bound up, so that a line never
// says more than was read; a moment is rounded to the nearest. Every time handed in is the difference of two readings
// of one monotonic clock, the later minus the earlier: never negative.
QString seconds(qint64 ms, Rounding rounding) {
    const qint64 hundredths = (ms + (rounding == Rounding::Down ? 0 : rounding == Rounding::Nearest ? 5 : 9)) / 10;
    return QStringLiteral("%1.%2").arg(hundredths / 100).arg(hundredths % 100, 2, 10, QLatin1Char('0'));
}

}  // namespace

UndockCheck *UndockCheck::s_instance = nullptr;

UndockCheck::UndockCheck(core::Api *core, QObject *parent) : QObject(parent), m_core(core) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    m_clock.start();  // once: the log lines are made of readings of this one clock

    m_pollTimer.setInterval(ui::kUndockPollMs);
    m_pollTimer.setSingleShot(false);
    // precise: a coarse timer may fire up to 5 % early, and two answers would then be closer together than the Undock
    // delay counts on
    m_pollTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_pollTimer, &QTimer::timeout, this, &UndockCheck::poll);

    m_pendingTimer.setInterval(PENDING_MS);
    m_pendingTimer.setSingleShot(true);
    // precise too: a coarse timer may fire 5 % early; in the desktop simulator it ended this wait of 6 s after 5.70 s
    m_pendingTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_pendingTimer, &QTimer::timeout, this, &UndockCheck::pendingTimedOut);

    connect(m_core, &core::Api::respBatteryRest, this, &UndockCheck::onBatteryRest);
}

UndockCheck::~UndockCheck() {
    s_instance = nullptr;
}

QObject *UndockCheck::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);
    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
    return obj;
}

void UndockCheck::setScreensaverOpen(bool open) {
    if (open == m_open) {
        return;
    }
    m_open = open;
    if (open) {
        m_stoppedThisOpen = false;  // a new opening tries again
        m_runLines = 0;             // ... and may log its runs again
    }
    emit screensaverOpenChanged();
    evaluate();
}

void UndockCheck::setCloseOnWake(int choice) {
    m_choice = static_cast<ui::CloseOnWake>(qBound(0, choice, 2));
    evaluate();
}

void UndockCheck::setUndockCloseMs(int ms) {
    m_closeMs = ui::undockCloseMs(ms);
}

void UndockCheck::onPowerModeChanged(Power::PowerMode fromPowerMode, Power::PowerMode toPowerMode) {
    Q_UNUSED(fromPowerMode);
    const bool wasLit = m_lit;
    m_lit = toPowerMode == Power::Normal || toPowerMode == Power::Idle;
    evaluate();
    // The watch stops and starts with the display in evaluate(). The asking after a close does the same here: no
    // request and no timer tick while the display is off, and a first question at once when it comes back.
    if (m_following && !m_watching && m_lit != wasLit) {
        if (m_lit) {
            poll();
            m_pollTimer.start();
        } else {
            m_pollTimer.stop();
        }
    }
}

void UndockCheck::onPowerSupplyChanged(bool powerSupply) {
    m_onCharger = powerSupply;
    if (!powerSupply && m_pending) {
        // the core confirms the close; if the reading was "on the charger" again meanwhile, the line says so and when
        QString line =
            QStringLiteral("Close on wake: the core reported the charger gone %1 s after the quick undock check")
                .arg(seconds(m_clock.elapsed() - m_closedAt, Rounding::Nearest));
        if (m_backOnAt >= 0) {
            line += QStringLiteral("; the reading had said \"on the charger\" again %1 s after the close")
                        .arg(seconds(m_backOnAt - m_closedAt, Rounding::Nearest));
        }
        qCInfo(lcScreensaver).noquote() << line;
        setPending(false);
    }
    evaluate();
}

void UndockCheck::onCoreDisconnected() {
    setPending(false);
    stop("the core disconnected");
}

// The core did not confirm the close within PENDING_MS. The line adds what the check read since the close, as bounds
// (see "What the log can say" in the header). Either between which two moments the reading was "on the charger" again
// and how long it was off in all: a bump that cut the charging for longer than the delay, or a lift and put back. Or
// when the last "off" answer came and how many there were: the core's report is late or was missed, or the asking
// ended early (the display went off, three answers failed). Or that neither an "off" nor an "on" answer came. It says
// what was read and when, not why.
void UndockCheck::pendingTimedOut() {
    QString what;
    if (m_backOnAt >= 0) {
        // back on between the sending of the last question answered "off" and the arrival of the first "on" answer
        const QString until = seconds(m_backOnAt - m_closedAt, Rounding::Up);
        what = m_offSinceClose > 0 ? QStringLiteral("on the charger again between %1 and %2 s after the close")
                                         .arg(seconds(m_closedCut.lastOffSentAt - m_closedAt, Rounding::Down), until)
                                   : QStringLiteral("on the charger again within %1 s after the close").arg(until);
        const QString atLeast = seconds(m_closedCut.atLeast(), Rounding::Down);
        what += m_closedCut.hasAtMost() ? QStringLiteral(", so off for %1 to %2 s in all")
                                              .arg(atLeast, seconds(m_closedCut.atMost(m_backOnAt), Rounding::Up))
                                        : QStringLiteral(", so off for at least %1 s in all").arg(atLeast);
    } else if (m_offSinceClose > 0) {
        what = QStringLiteral(
                   "still off the charger at the last %1, %2 s after the close "
                   "(%3 \"off\" %4 since the close, none \"on\")")
                   .arg(m_offAtLastAnswer ? QStringLiteral("answer") : QStringLiteral("\"off\" answer"),
                        seconds(m_lastOffAt - m_closedAt, Rounding::Nearest))
                   .arg(m_offSinceClose)
                   .arg(m_offSinceClose == 1 ? QStringLiteral("answer") : QStringLiteral("answers"));
    } else {
        what = QStringLiteral("no \"off\" or \"on\" answer since the close");
    }
    qCInfo(lcScreensaver).noquote().nospace() << "Close on wake: the core did not report the charger gone within "
                                              << PENDING_MS / 1000 << " s of the quick undock check: " << what;
    setPending(false);
}

void UndockCheck::evaluate() {
    const bool watch =
        !m_disabled && !m_stoppedThisOpen && ui::shouldWatchCharger(m_open, m_lit, m_onCharger, m_choice);
    if (watch == m_watching) {
        return;  // also while it only follows a close: that has its own ends (endFollowing() and stop())
    }
    if (!watch) {
        stop();
        return;
    }
    m_watching = true;
    m_inFlight = 0;  // an answer to a question asked while following a close is dropped: the watch asks its own
    m_run.reset();
    m_cut = Cut();  // this watch has had no "on the charger" answer yet: its first run has no "at most"
    m_failures = 0;
    qCDebug(lcScreensaver) << "quick undock check: watching, every" << ui::kUndockPollMs << "ms";
    poll();  // the first reading at once: a lift that woke a dark remote is caught right after the wake
    m_pollTimer.start();
}

void UndockCheck::stop(const char *why) {
    if (m_watching) {
        if (m_run.offInARow > 0) {
            logRunEnded(m_run.offInARow, why ? why : whyNotWatching(), false);
        }
        qCDebug(lcScreensaver) << "quick undock check: stopped";
    }
    m_watching = false;
    m_following = false;  // the poll timer is one for both
    m_pollTimer.stop();
    m_inFlight = 0;  // an answer still on its way is dropped (upstream ADR 0017)
    m_run.reset();
}

void UndockCheck::poll() {
    if (m_inFlight != 0) {
        return;  // one request at a time: a slow answer delays the next one
    }
    if (!m_watching && !(m_following && m_lit)) {
        return;  // following a close asks only while the display is on, like the watch
    }
    m_sentAt = m_clock.elapsed();  // before it leaves: the reading it brings back is taken after this
    m_inFlight = m_core->getBatteryRest();
}

void UndockCheck::onBatteryRest(int reqId, int httpCode, const QVariantMap &battery, const QString &errorMessage) {
    if (reqId != m_inFlight || m_inFlight == 0) {
        qCDebug(lcScreensaver) << "quick undock check: dropped an answer that was no longer awaited" << reqId;
        return;
    }
    m_inFlight = 0;
    if (!m_watching && !m_following) {
        return;
    }
    const qint64 arrived = m_clock.elapsed();  // the reading was taken between m_sentAt and now
    const qint64 sent = m_sentAt;
    if (httpCode != 200) {
        if (m_following) {
            m_offAtLastAnswer = false;
        }
        if (m_run.offInARow > 0) {
            logRunEnded(m_run.offInARow, "a failed answer", true);
        }
        m_run.reset();
        failed(httpCode, errorMessage);
        return;
    }
    m_failures = 0;
    const ui::ChargerReading reading = ui::chargerReadingFrom(battery);
    if (m_following) {
        // after a close, for the log only: what the endpoint says, and when
        m_offAtLastAnswer = reading == ui::ChargerReading::OffCharger;
        if (reading == ui::ChargerReading::OnCharger) {
            m_backOnAt = arrived;
            endFollowing();
        } else if (reading == ui::ChargerReading::OffCharger) {
            ++m_offSinceClose;
            m_lastOffAt = arrived;
            m_closedCut.lastOffSentAt = sent;
        }
    }
    if (!m_watching) {
        return;  // an answer asked for after a close never reaches the count below: it can close nothing
    }
    if (reading == ui::ChargerReading::OnCharger) {
        m_trusted = true;
    }
    if (!m_trusted) {
        m_run.reset();  // not yet seen "on the charger" from this endpoint: its "off" can't be trusted
        return;
    }
    const int  before = m_run.offInARow;
    const bool complete = m_run.feed(reading, ui::undockReadingsNeeded(m_closeMs));
    if (reading == ui::ChargerReading::OffCharger) {
        if (before == 0) {
            // A run starts. The next questions leave whole intervals after this answer arrived, so the question that
            // completes the run is sent at least the Undock delay after it, however slow an answer is.
            m_cut.firstOffAt = arrived;
            m_pollTimer.start();
        }
        m_cut.lastOffSentAt = sent;
    } else {
        if (before > 0) {
            if (reading == ui::ChargerReading::OnCharger) {
                logRunEnded(before, "on the charger again", true, arrived);
            } else {
                logRunEnded(before, "an answer without power_supply", true);
            }
        }
        if (reading == ui::ChargerReading::OnCharger) {
            m_cut.lastOnSentAt = sent;  // after the line above, which wants the one before the run
        }
    }
    if (!complete) {
        return;
    }
    // "at least": by the rule above never below the Undock delay
    qCInfo(lcScreensaver).nospace() << "Close on wake (" << ui::closeOnWakeName(m_choice)
                                    << "): off the charger for at least " << m_cut.atLeast() << " ms, "
                                    << m_run.offInARow << " \"off\" answers in a row (quick undock check, Undock delay "
                                    << m_closeMs << " ms), closes the screensaver";
    m_run.reset();
    stop();
    // the record of this close; a second close while undockPending starts it, and the wait, anew
    m_closedAt = arrived;
    m_closedCut = m_cut;
    m_offSinceClose = 0;
    m_backOnAt = -1;
    // All state first, the two signals last. A slot on undockPendingChanged may end the wait at once, and main.qml
    // closes the screensaver inside closeRequested, which comes back here as setScreensaverOpen(false).
    m_pendingTimer.start();
    m_following = true;  // it goes on asking while undockPending, for the log only (see the header)
    m_pollTimer.start();
    setPending(true);
    emit closeRequested();
}

void UndockCheck::failed(int httpCode, const QString &errorMessage) {
    const QString what = httpCode == 0 ? QStringLiteral("no answer (%1)").arg(errorMessage)
                                       : QStringLiteral("HTTP %1 %2").arg(httpCode).arg(errorMessage.left(80));
    if (httpCode == 404) {
        m_disabled = true;
        warnOnce(QStringLiteral("not available (%1): off until the UI restarts").arg(what));
        stop();
        return;
    }
    if (httpCode == 401 || httpCode == 403) {
        m_stoppedThisOpen = true;
        warnOnce(QStringLiteral("refused (%1): off until the screensaver opens again").arg(what));
        stop();
        return;
    }
    if (++m_failures >= FAILURES_TO_STOP) {
        if (!m_watching) {
            // Only asking for the log after a close: nothing depends on these answers and no screensaver is open, so
            // the asking ends quietly. The one warning of the UI run is left to a failure of the watch itself.
            qCDebug(lcScreensaver).noquote()
                << "quick undock check:" << m_failures
                << "failed answers in a row after a close, it asks no more; the last:" << what;
            endFollowing();
            return;
        }
        m_stoppedThisOpen = true;
        warnOnce(QStringLiteral("%1 failed answers in a row, the last %2: off until the screensaver opens again")
                     .arg(m_failures)
                     .arg(what));
        stop();
        return;
    }
    qCDebug(lcScreensaver) << "quick undock check: failed answer" << what;
}

void UndockCheck::warnOnce(const QString &message) {
    if (m_warned) {
        qCDebug(lcScreensaver).noquote() << "quick undock check" << message;
        return;
    }
    m_warned = true;
    qCWarning(lcScreensaver).noquote() << "quick undock check" << message
                                       << "(the screensaver closes on the core's charger report, as before)";
}

void UndockCheck::setPending(bool pending) {
    if (pending == m_pending) {
        return;
    }
    m_pending = pending;
    if (!pending) {
        m_pendingTimer.stop();
        endFollowing();
    }
    emit undockPendingChanged();
}

// A run of "off the charger" answers that closed nothing, for the log, with the bounds the answers give (see "What the
// log can say" in the header). "At most" needs an "on the charger" answer on both sides of the run; "at least" needs
// two "off" answers, as the one question of a single answer was sent before that answer came. A bump of a Qi pad
// shows here, unless it falls between two questions.
void UndockCheck::logRunEnded(int answers, const char *then, bool stays, qint64 onAgainAt) {
    if (m_runLines >= RUN_LINES_PER_OPENING) {
        return;
    }
    ++m_runLines;
    const bool atMost = onAgainAt >= 0 && m_cut.hasAtMost();
    QString    text;
    if (answers == 1) {
        text = atMost
                   ? QStringLiteral("off the charger for at most %1 ms (1 \"off\" answer)").arg(m_cut.atMost(onAgainAt))
                   : QStringLiteral("off the charger at 1 answer");
    } else if (atMost) {
        text = QStringLiteral("off the charger for %1 to %2 ms (%3 \"off\" answers)")
                   .arg(m_cut.atLeast())
                   .arg(m_cut.atMost(onAgainAt))
                   .arg(answers);
    } else {
        text =
            QStringLiteral("off the charger for at least %1 ms (%2 \"off\" answers)").arg(m_cut.atLeast()).arg(answers);
    }
    text += QStringLiteral(", then ") + QLatin1String(then);
    text += stays ? QStringLiteral(": the screensaver stays") : QStringLiteral(": the check stopped");
    if (m_runLines == RUN_LINES_PER_OPENING) {
        text += QStringLiteral(" (no more such lines until the screensaver opens again)");
    }
    qCInfo(lcScreensaver).noquote() << "quick undock check:" << text;
}

void UndockCheck::endFollowing() {
    if (!m_following) {
        return;
    }
    m_following = false;
    if (!m_watching) {
        m_pollTimer.stop();
        m_inFlight = 0;  // an answer still on its way is dropped
    }
}

// What ended the watch, from the state evaluate() just looked at, for the line of a run that was cut short. In the
// app that line says "the screensaver closed" or "the display went off" (or "the core disconnected", handed to stop()).
// The others need something the app does not do today. "the core reported the charger gone": main.qml's handler of
// that report runs before this object's slot and has closed the screensaver by then, so the line says "the
// screensaver closed" (the unit test prints this one). "Close on wake was set to Never": the setting changed under an
// open screensaver, which covers the settings page. "it was switched off": a watch that stops for no reason the state
// shows.
const char *UndockCheck::whyNotWatching() const {
    if (!m_open) {
        return "the screensaver closed";
    }
    if (!m_lit) {
        return "the display went off";
    }
    if (!m_onCharger) {
        return "the core reported the charger gone";
    }
    if (m_choice == ui::CloseOnWake::Never) {
        return "Close on wake was set to Never";
    }
    return "it was switched off";
}

}  // namespace hw
}  // namespace uc
