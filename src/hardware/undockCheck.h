// Copyright (c) 2026 madalone. UndockCheck — the quick undock check: closes the screensaver soon after the remote is
// lifted off its charger, instead of at the core's next battery check, up to 5 s later (GitHub issue #3).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <QVariantMap>

#include "../core/core.h"
#include "../system/power.h"
#include "../ui/screensaverClosePolicy.h"

namespace uc {
namespace hw {

/**
 * The core reports the charger (battery_status, Battery.powerSupply) only at its battery check, every 5.004 s, so
 * Close on wake closed a lifted screensaver 0-5 s late. GET /api/system/power/battery says so sooner: on the
 * maintainer's Remote 3 on a Qi pad it said "off the charger" 1.4-4.5 s before the core's report on all 7 lifts with
 * the screensaver lit or dimmed (2026-11-15; the moments of the lifts were not recorded). While the screensaver is
 * open, lit or dimmed, on the charger, with a Close on wake choice that closes on charger loss, this asks it every
 * ui::kUndockPollMs (one request at a time, the first at once) and emits closeRequested() after "off the charger" came
 * back ui::undockReadingsNeeded() times in a row ("Undock delay"). The rules are in src/ui/screensaverClosePolicy.h.
 *
 * The delay is a whole delay. The poll timer is precise (a coarse one may fire 5 % early), and it starts again when
 * the first "off" answer of a run arrives: the next questions leave whole intervals after that answer, so the last
 * question of a run is sent at least the delay after its first answer arrived, however slow an answer is.
 *
 * What the log can say. The check sees the endpoint's reading only when it asks, so its lines give bounds, not the
 * length of a cut of the charging: "at least" from the arrival of a run's first "off" answer to the sending of the
 * question its last "off" answer came back to, "at most" from the sending of the last question answered "on the
 * charger" before the run to the arrival of the "on" answer after it (one clock, started with the check). What the
 * reading did between two questions it cannot tell: a cut that falls between two leaves no line. A cut shorter than
 * the Undock delay never closes the screensaver; one longer than the delay plus one interval and two answer times
 * (about 0.85 s at the default delay on the remote, whose answers take about 45 ms) always does; in between it
 * depends on the moment it begins: the first "off" answer can come up to an interval and an answer time after the
 * cut began, and the reading of the last one is taken up to an answer time after its question left.
 * A run of "off" answers that closed nothing leaves a line with its bounds, at most RUN_LINES_PER_OPENING per opening.
 *
 * It never writes into Battery: keep-awake, the phantom-wake check, the dock chime and the docked-only clock follow the
 * core's report as before. A short loss the core never sees (a bump on a Qi pad) would otherwise leave them believing
 * the remote undocked until the next real change.
 *
 * Safety net: it closes only after the endpoint has said "on the charger" once since the UI started, so an endpoint
 * that never says so (another model, another firmware) can't close every screensaver half a second after it opens.
 * A 404 turns it off until the UI restarts; a 401 / 403, or 3 failed answers in a row, until the next opening; one
 * warning per UI run, and the screensaver then closes on the core's report as before.
 *
 * undockPending is true from a close it asked for until the core reports the charger gone, PENDING_MS have passed or
 * the core disconnects. main.qml opens no screensaver on the charger while it is true ("Run after dismissal while
 * docked", a wake while charging, the dock chime's grace timer), so none can open in the hand of someone who just
 * lifted the remote: Battery still says "on the charger" then, and the re-arm's 5 s minimum is only about 0.55 s
 * longer than the core's slowest report (at the default Undock delay).
 * While it is true and the display is on, the check also goes on asking until the endpoint says "on the charger"
 * again, for the log only. Such an answer closes nothing and writes nothing into Battery. The line written when the
 * wait ends says what was read meanwhile and when: between which two moments the reading was "on the charger" again
 * (a bump that cut the charging for longer than the delay, or a lift and put back), or when the last "off" answer
 * came. Three failed answers in a row end that asking without the warning above, as nothing depends on those answers;
 * a 404 or a 401 / 403 is handled as while watching. While the display is off it asks nothing and its timer is
 * stopped: the bounds in that line are then wider, not wrong.
 */
class UndockCheck : public QObject {
    Q_OBJECT
    /// written by main.qml: the screensaver is open (its Popup's `opened`: loaded, neither opening nor closing)
    Q_PROPERTY(bool screensaverOpen READ screensaverOpen WRITE setScreensaverOpen NOTIFY screensaverOpenChanged)
    /// true from a close this asked for until the core reports the charger gone or disconnects, or PENDING_MS later
    Q_PROPERTY(bool undockPending READ undockPending NOTIFY undockPendingChanged)

 public:
    /// how long a close waits for the core's report: its battery check runs every 5.004 s
    static constexpr int PENDING_MS = 6000;
    /// failed answers in a row (no answer in 1 s, an HTTP error) that stop the check until the next opening; after a
    /// close they only end the asking for the log, without a warning
    static constexpr int FAILURES_TO_STOP = 3;
    /// log lines about a run of "off" answers that closed nothing, per opening of the screensaver: a charger contact
    /// that flickers must not fill the log. 20, so that a test of several bumps in one opening gets a line for each of
    /// them: one bump can give more than one run.
    static constexpr int RUN_LINES_PER_OPENING = 20;

    /// Starts in Normal and off the charger: Power and Battery report the core's state after the connection.
    explicit UndockCheck(core::Api *core, QObject *parent = nullptr);
    ~UndockCheck() override;

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

    bool screensaverOpen() const { return m_open; }
    void setScreensaverOpen(bool open);
    bool undockPending() const { return m_pending; }

    /// "Close on wake" (ScreensaverConfig.closeOnWake, 0-2) and "Undock delay" (undockCloseMs), pushed by main.cpp
    void setCloseOnWake(int choice);
    void setUndockCloseMs(int ms);

 public slots:
    void onPowerModeChanged(uc::hw::Power::PowerMode fromPowerMode, uc::hw::Power::PowerMode toPowerMode);
    /// Battery::powerSupplyChanged: the core's report
    void onPowerSupplyChanged(bool powerSupply);
    void onCoreDisconnected();

 signals:
    void screensaverOpenChanged();
    void undockPendingChanged();
    /// The endpoint has said "off the charger" for the whole Undock delay (a lift, or a cut of the charging that
    /// long); emitted for nothing else. main.qml closes the screensaver, and the phantom-wake check counts it as
    /// activity (PhantomWakeSuppressor::onQuickUndock).
    void closeRequested();

 private:
    /// What the answers tell about one cut of the charging, as readings of m_clock (see "What the log can say").
    struct Cut {
        /// its first "off" answer arrived: the cut had begun by then
        qint64 firstOffAt = 0;
        /// the question its latest "off" answer came back to was sent: the cut still lasted after that
        qint64 lastOffSentAt = 0;
        /// the last question answered "on the charger" before it was sent: the cut began after that. -1: this watch
        /// has had no such answer yet
        qint64 lastOnSentAt = -1;

        /// how long it lasted at least; from its second "off" answer on (the question of the first was sent before
        /// that answer arrived)
        qint64 atLeast() const { return lastOffSentAt - firstOffAt; }
        bool   hasAtMost() const { return lastOnSentAt >= 0; }
        /// ... and at most. onAgainAt: the "on the charger" answer after it arrived, so the cut had ended by then
        qint64 atMost(qint64 onAgainAt) const { return onAgainAt - lastOnSentAt; }
    };

    void evaluate();
    void poll();
    /// why: what ended the watch, for the log line of a run it cut short (default: what the state says)
    void stop(const char *why = nullptr);
    void onBatteryRest(int reqId, int httpCode, const QVariantMap &battery, const QString &errorMessage);
    void failed(int httpCode, const QString &errorMessage);
    void warnOnce(const QString &message);
    void setPending(bool pending);
    void pendingTimedOut();
    /// logs a run of `answers` "off" answers that closed nothing. stays: an answer ended it, so the screensaver stays
    /// (a failed answer can stop the watch in the same call, with its warning); otherwise the watch itself stopped.
    /// onAgainAt: the arrival of the "on the charger" answer that ended it; -1 when something else did.
    void        logRunEnded(int answers, const char *then, bool stays, qint64 onAgainAt = -1);
    void        endFollowing();
    const char *whyNotWatching() const;

    static UndockCheck *s_instance;

    core::Api *m_core;

    QTimer        m_pollTimer;     // ui::kUndockPollMs, precise; runs while watching, or following a close when lit
    QTimer        m_pendingTimer;  // PENDING_MS, precise, single shot
    QElapsedTimer m_clock;         // started once, in the constructor: every time kept below is a reading of it, in ms

    bool            m_open = false;
    bool            m_lit = true;  // Normal or Idle (dimmed): the display is on
    bool            m_onCharger = false;
    ui::CloseOnWake m_choice = ui::CloseOnWake::Never;
    int             m_closeMs = ui::kUndockCloseMsDefault;

    bool          m_watching = false;
    int           m_inFlight = 0;  // the request id of the answer awaited; 0 = none
    qint64        m_sentAt = 0;    // when that question was sent
    ui::UndockRun m_run;
    Cut           m_cut;                      // the run under way; lastOnSentAt also between two runs
    int           m_runLines = 0;             // lines logged about runs that closed nothing, in this opening
    bool          m_trusted = false;          // the endpoint said "on the charger" once in this UI run
    bool          m_disabled = false;         // a 404: off until the UI restarts
    bool          m_stoppedThisOpen = false;  // a 401 / 403 or FAILURES_TO_STOP failures: off until the next opening
    int           m_failures = 0;
    bool          m_warned = false;  // one warning per UI run
    bool          m_pending = false;

    // After a close, while undockPending: it goes on asking, for the log only (m_following implies m_pending). The
    // record of that close, for the line written when the core's report comes, or doesn't:
    bool   m_following = false;
    qint64 m_closedAt = 0;             // the answer that completed the run arrived
    Cut    m_closedCut;                // that run; lastOffSentAt moves on with every "off" answer since the close
    int    m_offSinceClose = 0;        // "off" answers since the close
    qint64 m_lastOffAt = 0;            // the latest of them arrived
    bool   m_offAtLastAnswer = false;  // ... and no failed answer and none without power_supply came after it
    qint64 m_backOnAt = -1;            // the first "on the charger" answer since the close arrived; -1: none

#ifdef UC_TEST_BUILD
    friend class UndockCheckTest;
#endif
};

}  // namespace hw
}  // namespace uc
