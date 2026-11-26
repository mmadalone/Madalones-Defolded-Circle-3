// Copyright (c) 2026 madalone. Unit tests for UndockCheck, the quick undock check (v1.6.5, GitHub issue #3): when it
// asks GET /api/system/power/battery, when it asks main.qml to close the screensaver, its safety net and its failures,
// what it writes to the log, and what it does after a close while it waits for the core's report.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The stub core::Api (../mock_core_api.cpp) answers getBatteryRest() only when the test says so
// (MockCoreRecorder::setBatteryRestManual(true)), so every reading is the test's. The poll timer is stopped after the
// check starts watching and the next poll is asked directly (friend access), except in the two tests with the real
// timer (pollsEveryQuarterSecond, aSlowFirstAnswerStillGivesTheWholeDelay). The first "off" answer of a run starts the
// timer again; its ticks ask nothing in the other tests, where a request is always in flight or the test has stopped
// the timer.
//
// The times in the log lines are checked in three ways, none of which can fail because the machine is slow:
// - What the check keeps (when a question left, when an answer came) is compared with the check's own clock read by
//   the test right before and after: the kept time must lie between the two. Real time passes between the steps there
//   (QTest::qWait, which waits at least that long), so that a time kept at the wrong moment falls outside.
// - What a line prints from those times is checked with times the test writes into the check itself (friend access):
//   every form of every line, to the digit.
// - With the real timer, only lower bounds: a precise timer never fires early, a loaded machine only makes it late.

#include <QCoreApplication>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QtTest>

#include "../../../src/core/core.h"
#include "../../../src/hardware/undockCheck.h"
#include "../../../src/system/power.h"
#include "../mock_core_api.h"

using uc::test::MockCoreRecorder;

namespace {
QVariantMap onCharger() { return {{QStringLiteral("power_supply"), true}, {QStringLiteral("capacity"), 40}}; }
QVariantMap offCharger() { return {{QStringLiteral("power_supply"), false}, {QStringLiteral("capacity"), 40}}; }
QVariantMap noField() { return {{QStringLiteral("capacity"), 40}}; }

// counts the quick check's warnings (oneWarningPerRun)
int  s_warnings = 0;
void countWarnings(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    Q_UNUSED(context);
    if (type == QtWarningMsg && message.contains(QStringLiteral("quick undock check"))) {
        ++s_warnings;
    }
}

// the same count while one of these exists, from zero; the handler installed before comes back when it goes
class WarningCount {
 public:
    WarningCount() : m_previous(qInstallMessageHandler(countWarnings)) { s_warnings = 0; }
    ~WarningCount() { qInstallMessageHandler(m_previous); }
    WarningCount(const WarningCount &) = delete;
    WarningCount &operator=(const WarningCount &) = delete;

    int count() const { return s_warnings; }

 private:
    QtMessageHandler m_previous;
};

// the info lines of uc.ui.screensaver while one of these exists, oldest first (they still reach QtTest's handler)
class ScreensaverLog {
 public:
    ScreensaverLog() {
        s_lines.clear();
        s_previous = qInstallMessageHandler(&ScreensaverLog::handle);
    }
    ~ScreensaverLog() { qInstallMessageHandler(s_previous); }
    ScreensaverLog(const ScreensaverLog &) = delete;
    ScreensaverLog &operator=(const ScreensaverLog &) = delete;

    // the lines that contain `part`
    QStringList matching(const QString &part) const { return s_lines.filter(part); }

 private:
    static void handle(QtMsgType type, const QMessageLogContext &context, const QString &message) {
        if (type == QtInfoMsg && context.category && qstrcmp(context.category, "uc.ui.screensaver") == 0) {
            s_lines.append(message);
        }
        if (s_previous) {
            s_previous(type, context, message);
        }
    }

    static QStringList      s_lines;
    static QtMessageHandler s_previous;
};

QStringList      ScreensaverLog::s_lines;
QtMessageHandler ScreensaverLog::s_previous = nullptr;

bool matches(const QString &line, const QString &pattern) {
    return QRegularExpression(pattern).match(line).hasMatch();
}
}  // namespace

namespace uc {
namespace hw {

class UndockCheckTest : public QObject {
    Q_OBJECT

 private:
    core::Api   *m_api = nullptr;
    UndockCheck *m_check = nullptr;

    using PowerMode = Power::PowerMode;

    // the check's own clock, which all the times it keeps are readings of
    qint64 now() const { return m_check->m_clock.elapsed(); }

    // about the answer the test gave last: when its question left, as the check noted it (ask() and startWatching()
    // check that note), and the two readings of the clock between which it arrived
    qint64 m_answeredSentAt = 0;
    qint64 m_arrivedLo = 0;
    qint64 m_arrivedHi = 0;

    bool arrivedThen(qint64 kept) const { return kept >= m_arrivedLo && kept <= m_arrivedHi; }

    // open, lit, on the charger, Close on wake as given (1 Undocked, 2 Always): the check starts and asks at once
    void startWatching(int closeOnWake = 1) {
        m_check->setCloseOnWake(closeOnWake);
        m_check->onPowerSupplyChanged(true);
        const qint64 lo = now();
        m_check->setScreensaverOpen(true);
        QVERIFY(m_check->m_watching);
        QVERIFY2(m_check->m_sentAt >= lo && m_check->m_sentAt <= now(),
                 "the first question's time is not when it left");
        m_check->m_pollTimer.stop();  // the test asks for each reading
    }

    // answers the request in flight and asks nothing: the test decides when the next question leaves (ask())
    void give(int httpCode, const QVariantMap &battery, const QString &error = QString()) {
        const int id = MockCoreRecorder::lastBatteryRestId();
        m_answeredSentAt = m_check->m_sentAt;
        m_arrivedLo = now();
        emit m_api->respBatteryRest(id, httpCode, battery, error);
        m_arrivedHi = now();
    }

    // lets the check ask. If a question leaves, the time the check keeps for it must be the time it left.
    void ask() {
        const int    asked = calls();
        const qint64 lo = now();
        m_check->poll();
        if (calls() != asked) {
            QVERIFY2(m_check->m_sentAt >= lo && m_check->m_sentAt <= now(), "a question's time is not when it left");
        }
    }

    // answers the request in flight, then lets the check ask again at once
    void answer(int httpCode, const QVariantMap &battery, const QString &error = QString()) {
        give(httpCode, battery, error);
        ask();
    }

    // For the tests of the times: an answer `gapMs` or more after its question left, and the next question `gapMs` or
    // more after that answer. The poll timer is stopped in between (the first "off" answer of a run starts it), so no
    // tick asks before the test does.
    void answerSlowly(int gapMs, int httpCode, const QVariantMap &battery, const QString &error = QString()) {
        QTest::qWait(gapMs);
        give(httpCode, battery, error);
        m_check->m_pollTimer.stop();
        QTest::qWait(gapMs);
        ask();
    }

    // "on the charger" once (the endpoint is trusted), then "off" until the check asks for the close: 3 answers at
    // the default delay
    void liftUntilClosed() {
        answer(200, onCharger());
        const int needed = ui::undockReadingsNeeded(m_check->m_closeMs);
        for (int i = 0; i < needed; ++i) {
            answer(200, offCharger());
        }
        QVERIFY(m_check->undockPending());
    }

    // seconds with two decimals as the lines print them: a lower bound rounded down, an upper bound up, a moment to
    // the nearest (the check formats with whole numbers; here it is done the other way round, with a double)
    static QString secondsDown(qint64 ms) { return QString::number(double(ms / 10) / 100.0, 'f', 2); }
    static QString secondsUp(qint64 ms) { return QString::number(double((ms + 9) / 10) / 100.0, 'f', 2); }
    static QString secondsNearest(qint64 ms) { return QString::number(double((ms + 5) / 10) / 100.0, 'f', 2); }

    // lets the wait for the core's report run out, with the same timer shortened
    void letThePendingWaitRunOut() {
        m_check->m_pendingTimer.setInterval(50);
        m_check->m_pendingTimer.start();
        QTRY_VERIFY_WITH_TIMEOUT(!m_check->undockPending(), 2000);
    }

    int calls() const { return MockCoreRecorder::batteryRestCallCount(); }

    // the figure of a close line, "off the charger for at least <N> ms": -1 unless exactly one such line was logged
    static int closeLineAtLeast(const ScreensaverLog &log, const QString &answers) {
        const QStringList lines = log.matching(QStringLiteral("closes the screensaver"));
        if (lines.count() != 1) {
            return -1;
        }
        const QRegularExpressionMatch m =
            QRegularExpression(
                QStringLiteral("^Close on wake \\(\\w+\\): off the charger for at least (\\d+) ms, %1 "
                               "\"off\" answers in a row \\(quick undock check, Undock delay \\d+ ms\\), "
                               "closes the screensaver$")
                    .arg(answers))
                .match(lines.first());
        return m.hasMatch() ? m.captured(1).toInt() : -1;
    }

 private slots:
    void init() {
        MockCoreRecorder::reset();
        MockCoreRecorder::setBatteryRestManual(true);
        m_api = new core::Api(QStringLiteral("ws://test"), this);
        m_check = new UndockCheck(m_api, this);
    }

    void cleanup() {
        delete m_check;
        m_check = nullptr;
        delete m_api;
        m_api = nullptr;
    }

    // it starts asking only while the screensaver is open, lit, on the charger and Close on wake isn't Never
    void watchesOnlyWhenEveryConditionHolds_data() {
        QTest::addColumn<bool>("open");
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("charger");
        QTest::addColumn<int>("choice");
        QTest::addColumn<bool>("asks");
        QTest::newRow("all, Undocked") << true << int(Power::Normal) << true << 1 << true;
        QTest::newRow("all, Always") << true << int(Power::Normal) << true << 2 << true;
        QTest::newRow("dimmed") << true << int(Power::Idle) << true << 1 << true;
        QTest::newRow("Never") << true << int(Power::Normal) << true << 0 << false;
        QTest::newRow("closed") << false << int(Power::Normal) << true << 1 << false;
        QTest::newRow("display off") << true << int(Power::Low_power) << true << 1 << false;
        QTest::newRow("suspend") << true << int(Power::Suspend) << true << 1 << false;
        QTest::newRow("off the charger") << true << int(Power::Normal) << false << 1 << false;
    }
    void watchesOnlyWhenEveryConditionHolds() {
        QFETCH(bool, open);
        QFETCH(int, mode);
        QFETCH(bool, charger);
        QFETCH(int, choice);
        QFETCH(bool, asks);
        m_check->onPowerModeChanged(Power::Normal, static_cast<PowerMode>(mode));
        m_check->setCloseOnWake(choice);
        m_check->onPowerSupplyChanged(charger);
        m_check->setScreensaverOpen(open);
        QCOMPARE(calls() > 0, asks);
        QCOMPARE(m_check->m_pollTimer.isActive(), asks);
    }

    // the first reading at once, and never a second request while one is awaited
    void firstRequestAtOnce_oneInFlight() {
        startWatching();
        QCOMPARE(calls(), 1);
        m_check->poll();
        m_check->poll();
        QCOMPARE(calls(), 1);
        answer(200, onCharger());
        QCOMPARE(calls(), 2);
    }

    // "off the charger" N times in a row closes: the first, then one for every 250 ms of the Undock delay (3 at
    // 500 ms, 9 at 2000 ms). After the close it goes on asking, for the log only.
    void closesAfterTheReadingsNeeded_data() {
        QTest::addColumn<int>("delayMs");
        QTest::addColumn<int>("needed");
        QTest::newRow("500 ms (default)") << 500 << 3;
        QTest::newRow("750 ms") << 750 << 4;
        QTest::newRow("1000 ms") << 1000 << 5;
        QTest::newRow("2000 ms") << 2000 << 9;
    }
    void closesAfterTheReadingsNeeded() {
        QFETCH(int, delayMs);
        QFETCH(int, needed);
        m_check->setUndockCloseMs(delayMs);
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        answer(200, onCharger());
        for (int i = 1; i < needed; ++i) {
            answer(200, offCharger());
        }
        QCOMPARE(closeSpy.count(), 0);
        const int before = calls();
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 1);
        QVERIFY(m_check->undockPending());
        QVERIFY(!m_check->m_watching);
        QVERIFY(m_check->m_following);
        QCOMPARE(calls(), before + 1);  // the one question answer() let it ask after the close
    }

    // two "off" answers, a quarter second apart on a remote, close nothing at the shortest delay: that closed the
    // screensaver on a bump of the pad with test build 1.6.4-keys1
    void twoOffAnswersCloseNothing() {
        QCOMPARE(m_check->m_closeMs, 500);
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        answer(200, onCharger());
        answer(200, offCharger());
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 0);
        QVERIFY(m_check->m_watching);
        QVERIFY(!m_check->undockPending());
        answer(200, onCharger());
        QCOMPARE(closeSpy.count(), 0);
    }

    // a reading "on the charger" between "off" readings starts the count again (a bump)
    void anOnReadingStartsTheCountAgain() {
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        answer(200, onCharger());
        answer(200, offCharger());
        answer(200, offCharger());
        answer(200, onCharger());
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 0);
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 0);
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 1);
    }

    // an answer without power_supply, or a failed one, starts the count again
    void unknownStartsTheCountAgain() {
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        answer(200, onCharger());
        answer(200, offCharger());
        answer(200, offCharger());
        answer(200, noField());
        answer(200, offCharger());
        answer(200, offCharger());
        answer(0, QVariantMap(), QStringLiteral("Operation canceled"));
        answer(200, offCharger());
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 0);
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 1);
    }

    // until the endpoint has said "on the charger" once, its "off" never closes the screensaver
    void untrustedEndpointNeverCloses() {
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        for (int i = 0; i < 10; ++i) {
            answer(200, offCharger());
        }
        QCOMPARE(closeSpy.count(), 0);
        answer(200, onCharger());  // now trusted, for the rest of the UI run
        answer(200, offCharger());
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 0);
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 1);
    }

    // trusted once, it also acts at a later opening whose first reading is already "off" (a lift that woke the remote)
    void trustCarriesToTheNextOpening() {
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        answer(200, onCharger());
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        m_check->m_pollTimer.stop();
        answer(200, offCharger());
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 0);
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 1);
    }

    // it stops when the screensaver goes dark or closes, when the core reports no charger, and for Never
    void stopsWhenDarkClosedUndockedOrNever() {
        startWatching();
        m_check->onPowerModeChanged(Power::Idle, Power::Low_power);
        QVERIFY(!m_check->m_watching);
        m_check->onPowerModeChanged(Power::Low_power, Power::Normal);
        QVERIFY(m_check->m_watching);
        m_check->setScreensaverOpen(false);
        QVERIFY(!m_check->m_watching);
        m_check->setScreensaverOpen(true);
        QVERIFY(m_check->m_watching);
        m_check->onPowerSupplyChanged(false);
        QVERIFY(!m_check->m_watching);
        m_check->onPowerSupplyChanged(true);
        QVERIFY(m_check->m_watching);
        m_check->setCloseOnWake(0);
        QVERIFY(!m_check->m_watching);
        QVERIFY(!m_check->m_pollTimer.isActive());
    }

    // an answer still on its way when it stopped is dropped, also once it watches again
    void lateAnswerAfterAStopIsDropped() {
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        answer(200, onCharger());
        answer(200, offCharger());
        const int late = MockCoreRecorder::lastBatteryRestId();
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        m_check->m_pollTimer.stop();
        emit m_api->respBatteryRest(late, 200, offCharger(), QString());
        QCOMPARE(closeSpy.count(), 0);
        QVERIFY(MockCoreRecorder::lastBatteryRestId() != late);
        answer(200, offCharger());
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 0);  // the count started again at the new opening
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 1);
    }

    // undockPending: set by the close, cleared by the core's report, which also ends the asking after the close
    void pendingClearedByTheCoreReport() {
        QSignalSpy pendingSpy(m_check, &UndockCheck::undockPendingChanged);
        startWatching();
        liftUntilClosed();
        QVERIFY(m_check->m_pendingTimer.isActive());
        QVERIFY(m_check->m_following);
        QVERIFY(m_check->m_pollTimer.isActive());
        m_check->onPowerSupplyChanged(false);
        QVERIFY(!m_check->undockPending());
        QVERIFY(!m_check->m_pendingTimer.isActive());
        QVERIFY(!m_check->m_following);
        QVERIFY(!m_check->m_pollTimer.isActive());
        QCOMPARE(pendingSpy.count(), 2);
    }

    // ... or PENDING_MS later, when the core never reported it (a bump it didn't see, a lift and put back)
    void pendingClearedAfterSixSeconds() {
        QCOMPARE(UndockCheck::PENDING_MS, 6000);
        startWatching();
        liftUntilClosed();
        letThePendingWaitRunOut();
        QVERIFY(!m_check->m_following);
        QVERIFY(!m_check->m_pollTimer.isActive());
    }

    // a 404 (no such endpoint) turns it off until the UI restarts
    void notFoundTurnsItOffForTheRun() {
        startWatching();
        answer(404, QVariantMap(), QStringLiteral("Not Found"));
        QVERIFY(!m_check->m_watching);
        const int before = calls();
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        QVERIFY(!m_check->m_watching);
        QCOMPARE(calls(), before);
    }

    // a 401 / 403 stops it until the screensaver opens again
    void refusedStopsItUntilTheNextOpening_data() {
        QTest::addColumn<int>("code");
        QTest::newRow("401") << 401;
        QTest::newRow("403") << 403;
    }
    void refusedStopsItUntilTheNextOpening() {
        QFETCH(int, code);
        startWatching();
        answer(code, QVariantMap(), QStringLiteral("Unauthorized"));
        QVERIFY(!m_check->m_watching);
        m_check->onPowerModeChanged(Power::Normal, Power::Low_power);
        m_check->onPowerModeChanged(Power::Low_power, Power::Normal);
        QVERIFY(!m_check->m_watching);  // a wake is not a new opening
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        QVERIFY(m_check->m_watching);
    }

    // FAILURES_TO_STOP failed answers in a row stop it until the next opening; one success in between resets the count
    void threeFailuresStopItUntilTheNextOpening() {
        QCOMPARE(UndockCheck::FAILURES_TO_STOP, 3);
        startWatching();
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        answer(500, QVariantMap(), QStringLiteral("Internal"));
        answer(200, onCharger());
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        QVERIFY(m_check->m_watching);
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        QVERIFY(!m_check->m_watching);
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        QVERIFY(m_check->m_watching);
    }

    // one warning per UI run, however often it fails
    void oneWarningPerRun() {
        s_warnings = 0;
        const QtMessageHandler previous = qInstallMessageHandler(countWarnings);
        startWatching();
        answer(401, QVariantMap(), QStringLiteral("Unauthorized"));
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        m_check->m_pollTimer.stop();
        answer(401, QVariantMap(), QStringLiteral("Unauthorized"));
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        m_check->m_pollTimer.stop();
        answer(404, QVariantMap(), QStringLiteral("Not Found"));
        qInstallMessageHandler(previous);
        QCOMPARE(s_warnings, 1);
    }

    // a coarse timer may fire 5 % early; the answers must never be closer together than the Undock delay counts on
    void pollTimerIsPrecise() {
        QCOMPARE(m_check->m_pollTimer.timerType(), Qt::PreciseTimer);
        QCOMPARE(m_check->m_pollTimer.interval(), uc::ui::kUndockPollMs);
    }

    // ... and the wait for the core's report is the 6 s the log line names: on a coarse timer it ended after 5.70 s in
    // the desktop simulator
    void pendingTimerIsPrecise() {
        QCOMPARE(m_check->m_pendingTimer.timerType(), Qt::PreciseTimer);
        QCOMPARE(m_check->m_pendingTimer.interval(), UndockCheck::PENDING_MS);
        QVERIFY(m_check->m_pendingTimer.isSingleShot());
    }

    // The first "off" answer of a run starts the poll timer again, so that the next questions leave whole intervals
    // after that answer arrived; the later answers of the run leave the timer alone. (startWatching() has stopped
    // the timer, so "running" here means "started again".)
    void theFirstOffAnswerOfARunStartsThePollTimerAgain() {
        startWatching();
        answer(200, onCharger());
        QVERIFY(!m_check->m_pollTimer.isActive());
        answer(200, offCharger());
        QVERIFY(m_check->m_pollTimer.isActive());
        m_check->m_pollTimer.stop();
        answer(200, offCharger());
        QVERIFY(!m_check->m_pollTimer.isActive());
        answer(200, onCharger());  // the run is over
        QVERIFY(!m_check->m_pollTimer.isActive());
        answer(200, offCharger());  // the first of the next run
        QVERIFY(m_check->m_pollTimer.isActive());
    }

    // with the real timer: a reading at once ("on"), then one every 250 ms ("off", "off", "off"). The close comes at
    // the fourth, at least 0.75 s after the start, and its line shows the guarantee: off the charger for at least the
    // Undock delay.
    void pollsEveryQuarterSecond() {
        QCOMPARE(uc::ui::kUndockPollMs, 250);
        ScreensaverLog log;
        MockCoreRecorder::setBatteryRestManual(false);
        MockCoreRecorder::pushNextBatteryResponse(200, onCharger());
        MockCoreRecorder::pushNextBatteryResponse(200, offCharger());
        MockCoreRecorder::pushNextBatteryResponse(200, offCharger());
        MockCoreRecorder::pushNextBatteryResponse(200, offCharger());
        QSignalSpy    closeSpy(m_check, &UndockCheck::closeRequested);
        QElapsedTimer t;
        int           callsAtClose = -1;
        qint64        msAtClose = -1;
        connect(m_check, &UndockCheck::closeRequested, this, [&]() {
            callsAtClose = calls();
            msAtClose = t.elapsed();
        });
        t.start();
        m_check->setCloseOnWake(1);
        m_check->onPowerSupplyChanged(true);
        m_check->setScreensaverOpen(true);
        QTRY_COMPARE_WITH_TIMEOUT(closeSpy.count(), 1, 5000);
        // the timer is precise: never early. Late is allowed (a loaded machine), up to the time-out above.
        QVERIFY2(msAtClose >= 750, qPrintable(QString::number(msAtClose)));
        QCOMPARE(callsAtClose, 4);
        // the last "off" question left at least the Undock delay after the first "off" answer came
        const int atLeast = closeLineAtLeast(log, QStringLiteral("3"));
        QVERIFY2(atLeast >= 500,
                 qPrintable(log.matching(QStringLiteral("closes the screensaver")).join(QLatin1Char('|'))));
        QCOMPARE(qint64(atLeast), m_check->m_closedCut.atLeast());
    }

    // with the real timer and a slow answer: the first "off" answer arrives late in its interval. The next question
    // still leaves a whole interval after it, so the last "off" question leaves the whole delay after that answer.
    // Without the new start of the timer it would leave about 0.3 s after it here.
    void aSlowFirstAnswerStillGivesTheWholeDelay() {
        ScreensaverLog log;
        QSignalSpy     closeSpy(m_check, &UndockCheck::closeRequested);
        m_check->setCloseOnWake(1);
        m_check->onPowerSupplyChanged(true);
        m_check->setScreensaverOpen(true);  // asks at once; the timer runs
        emit m_api->respBatteryRest(MockCoreRecorder::lastBatteryRestId(), 200, onCharger(), QString());
        QTRY_COMPARE_WITH_TIMEOUT(calls(), 2, 3000);  // the timer asked again
        QTest::qWait(150);                            // ... and the answer takes its time
        emit m_api->respBatteryRest(MockCoreRecorder::lastBatteryRestId(), 200, offCharger(), QString());
        QTRY_COMPARE_WITH_TIMEOUT(calls(), 3, 3000);
        emit m_api->respBatteryRest(MockCoreRecorder::lastBatteryRestId(), 200, offCharger(), QString());
        QCOMPARE(closeSpy.count(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(calls(), 4, 3000);
        emit m_api->respBatteryRest(MockCoreRecorder::lastBatteryRestId(), 200, offCharger(), QString());
        QCOMPARE(closeSpy.count(), 1);
        const int atLeast = closeLineAtLeast(log, QStringLiteral("3"));
        QVERIFY2(atLeast >= 500,
                 qPrintable(log.matching(QStringLiteral("closes the screensaver")).join(QLatin1Char('|'))));
        QCOMPARE(qint64(atLeast), m_check->m_closedCut.atLeast());
    }

    // the Undock delay lowered while a run is under way: the run completes at its next "off" answer
    void aDelayLoweredDuringARunCompletesItAtTheNextAnswer() {
        m_check->setUndockCloseMs(2000);
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        answer(200, onCharger());
        for (int i = 0; i < 4; ++i) {
            answer(200, offCharger());
        }
        QCOMPARE(closeSpy.count(), 0);
        m_check->setUndockCloseMs(500);
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 1);
    }

    // the close line: the choice, how long the reading was "off" at least, how many "off" answers, and the delay set
    void closeLineSaysHowLongAtLeastAndHowManyAnswers_data() {
        QTest::addColumn<int>("choice");
        QTest::addColumn<int>("delayMs");
        QTest::addColumn<QString>("line");
        QTest::newRow("Undocked, 500 ms")
            << 1 << 500
            << QStringLiteral(
                   "Close on wake (Undocked): off the charger for at least %1 ms, 3 \"off\" answers in a "
                   "row (quick undock check, Undock delay 500 ms), closes the screensaver");
        QTest::newRow("Always, 2000 ms")
            << 2 << 2000
            << QStringLiteral(
                   "Close on wake (Always): off the charger for at least %1 ms, 9 \"off\" answers in a row "
                   "(quick undock check, Undock delay 2000 ms), closes the screensaver");
    }
    void closeLineSaysHowLongAtLeastAndHowManyAnswers() {
        QFETCH(int, choice);
        QFETCH(int, delayMs);
        QFETCH(QString, line);
        ScreensaverLog log;
        m_check->setUndockCloseMs(delayMs);
        startWatching(choice);
        answer(200, onCharger());
        // The first "off" answer comes 20 ms or more after its question left, and the last "off" question leaves
        // 20 ms or more before its answer comes. "At least" is from that first answer to that last question; the two
        // waits tell it from the other three pairs of those four moments. (The test asks at once after each answer,
        // so the figure is far below the delay here. On a remote the timer keeps it at the delay or above: see
        // pollsEveryQuarterSecond.)
        QTest::qWait(20);
        give(200, offCharger());
        const qint64 firstLo = m_arrivedLo;
        const qint64 firstHi = m_arrivedHi;
        ask();
        for (int i = 2; i < ui::undockReadingsNeeded(delayMs); ++i) {
            answer(200, offCharger());
        }
        QTest::qWait(20);
        give(200, offCharger());
        QVERIFY(m_check->undockPending());
        const qint64 lastSent = m_answeredSentAt;
        const int    figure = closeLineAtLeast(log, QString::number(ui::undockReadingsNeeded(delayMs)));
        QVERIFY2(
            figure >= lastSent - firstHi && figure <= lastSent - firstLo,
            qPrintable(QStringLiteral("%1, not %2 to %3").arg(figure).arg(lastSent - firstHi).arg(lastSent - firstLo)));
        QCOMPARE(log.matching(QStringLiteral("closes the screensaver")).first(), line.arg(figure));
    }

    // A cut shorter than the delay (a bump) closes nothing; its "off" answers leave a line: how long the reading
    // was "off" at least and at most, how many "off" answers came, and what ended the run. "At most" needs an "on the
    // charger" answer on both sides of the run; a single "off" answer gives no "at least". (Which moments the figures
    // are made of: theTimesOfARunAreWhenItsQuestionsLeftAndItsAnswersCame. Every form to the digit: theRunLine.)
    void aRunThatClosesNothingLeavesALine() {
        ScreensaverLog log;
        QSignalSpy     closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        answer(200, onCharger());
        answer(200, offCharger());
        answer(200, offCharger());
        answer(200, onCharger());  // two "off" answers between two "on" ones
        answer(200, offCharger());
        answer(200, onCharger());  // one
        answer(200, offCharger());
        answer(200, offCharger());
        answer(200, noField());  // two, ended by an answer without power_supply
        answer(200, offCharger());
        answer(200, noField());  // one
        answer(200, offCharger());
        answer(200, offCharger());
        answer(0, QVariantMap(), QStringLiteral("timeout"));  // two, ended by a failed answer
        answer(200, offCharger());
        answer(500, QVariantMap(), QStringLiteral("Internal"));  // one
        QCOMPARE(closeSpy.count(), 0);
        const QStringList lines = log.matching(QStringLiteral("quick undock check: off the charger"));
        QCOMPARE(lines.count(), 6);
        const QString two = QStringLiteral(" ms \\(2 \"off\" answers\\), then ");
        QVERIFY2(matches(lines.at(0), QStringLiteral("^quick undock check: off the charger for \\d+ to \\d+") + two +
                                          QStringLiteral("on the charger again: the screensaver stays$")),
                 qPrintable(lines.at(0)));
        QVERIFY2(matches(lines.at(1), QStringLiteral("^quick undock check: off the charger for at most \\d+ ms "
                                                     "\\(1 \"off\" answer\\), then on the charger again: the "
                                                     "screensaver stays$")),
                 qPrintable(lines.at(1)));
        QVERIFY2(matches(lines.at(2), QStringLiteral("^quick undock check: off the charger for at least \\d+") + two +
                                          QStringLiteral("an answer without power_supply: the screensaver stays$")),
                 qPrintable(lines.at(2)));
        QCOMPARE(lines.at(3), QStringLiteral("quick undock check: off the charger at 1 answer, then an answer without "
                                             "power_supply: the screensaver stays"));
        QVERIFY2(matches(lines.at(4), QStringLiteral("^quick undock check: off the charger for at least \\d+") + two +
                                          QStringLiteral("a failed answer: the screensaver stays$")),
                 qPrintable(lines.at(4)));
        QCOMPARE(lines.at(5), QStringLiteral("quick undock check: off the charger at 1 answer, then a failed answer: "
                                             "the screensaver stays"));
    }

    // The moments a run line is made of. Real time passes between every question and its answer and between every
    // answer and the next question, so each moment is told from its neighbours:
    //     q1 . "on" . q2 . "off" . q3 . "off" . q4 . "on"
    // "At least" is from the first "off" answer to q3, the question of the last "off" answer. "At most" is from q1,
    // the question of the last "on" answer before the run, to the "on" answer after it. A second run, later, counts
    // from its own moments: the review found that nothing noticed when it counted from the first run's.
    void theTimesOfARunAreWhenItsQuestionsLeftAndItsAnswersCame() {
        ScreensaverLog           log;
        const int                gap = 20;
        const QString            part = QStringLiteral("quick undock check: off the charger");
        const QRegularExpression twoBetweenTwoOn(
            QStringLiteral("^quick undock check: off the charger for (\\d+) to (\\d+) ms \\(2 \"off\" answers\\), then "
                           "on the charger again: the screensaver stays$"));
        startWatching();
        const qint64 q1 = m_check->m_sentAt;
        answerSlowly(gap, 200, onCharger());
        QCOMPARE(m_check->m_cut.lastOnSentAt, q1);
        answerSlowly(gap, 200, offCharger());
        const qint64 firstLo = m_arrivedLo;
        const qint64 firstHi = m_arrivedHi;
        QVERIFY(arrivedThen(m_check->m_cut.firstOffAt));
        QCOMPARE(m_check->m_cut.lastOffSentAt, m_answeredSentAt);
        answerSlowly(gap, 200, offCharger());
        const qint64 q3 = m_answeredSentAt;
        QCOMPARE(m_check->m_cut.lastOffSentAt, q3);
        QVERIFY(m_check->m_cut.firstOffAt >= firstLo && m_check->m_cut.firstOffAt <= firstHi);  // still the first
        QCOMPARE(m_check->m_cut.lastOnSentAt, q1);
        answerSlowly(gap, 200, onCharger());
        const qint64 q4 = m_answeredSentAt;
        QCOMPARE(m_check->m_cut.lastOnSentAt, q4);  // from now on; the line below was written with q1
        QCOMPARE(log.matching(part).count(), 1);
        const QString                 firstLine = log.matching(part).at(0);
        const QRegularExpressionMatch first = twoBetweenTwoOn.match(firstLine);
        QVERIFY2(first.hasMatch(), qPrintable(firstLine));
        qint64 atLeast = first.captured(1).toLongLong();
        qint64 atMost = first.captured(2).toLongLong();
        QVERIFY2(atLeast >= q3 - firstHi && atLeast <= q3 - firstLo, qPrintable(firstLine));
        QVERIFY2(atMost >= m_arrivedLo - q1 && atMost <= m_arrivedHi - q1, qPrintable(firstLine));
        QVERIFY2(atLeast >= gap && atMost >= atLeast + 5 * gap, qPrintable(firstLine));  // one wait, and six more

        // a second run of two
        answerSlowly(gap, 200, offCharger());
        const qint64 secondLo = m_arrivedLo;
        const qint64 secondHi = m_arrivedHi;
        QVERIFY(arrivedThen(m_check->m_cut.firstOffAt));
        answerSlowly(gap, 200, offCharger());
        const qint64 q6 = m_answeredSentAt;
        answerSlowly(gap, 200, onCharger());
        QCOMPARE(log.matching(part).count(), 2);
        const QString                 secondLine = log.matching(part).at(1);
        const QRegularExpressionMatch second = twoBetweenTwoOn.match(secondLine);
        QVERIFY2(second.hasMatch(), qPrintable(secondLine));
        atLeast = second.captured(1).toLongLong();
        atMost = second.captured(2).toLongLong();
        QVERIFY2(atLeast >= q6 - secondHi && atLeast <= q6 - secondLo, qPrintable(secondLine));
        QVERIFY2(atMost >= m_arrivedLo - q4 && atMost <= m_arrivedHi - q4, qPrintable(secondLine));
    }

    // "At most" needs an "on the charger" answer before the run in the same watch: what the reading did before a
    // watch began, nobody asked. A lift that wakes a dark remote starts a watch whose first answer is already "off".
    void aRunBeforeAnyOnAnswerOfItsWatchHasNoAtMost() {
        ScreensaverLog log;
        startWatching();
        answer(200, onCharger());  // the endpoint is trusted for the UI run
        m_check->onPowerModeChanged(Power::Normal, Power::Low_power);
        m_check->onPowerModeChanged(Power::Low_power, Power::Normal);  // a new watch
        m_check->m_pollTimer.stop();
        answer(200, offCharger());
        answer(200, offCharger());
        answer(200, onCharger());
        answer(200, offCharger());
        answer(200, onCharger());  // this run had an "on" answer of its watch before it
        m_check->onPowerModeChanged(Power::Normal, Power::Low_power);
        m_check->onPowerModeChanged(Power::Low_power, Power::Normal);  // and a new watch again
        m_check->m_pollTimer.stop();
        answer(200, offCharger());
        answer(200, onCharger());
        const QStringList lines = log.matching(QStringLiteral("quick undock check: off the charger"));
        QCOMPARE(lines.count(), 3);
        QVERIFY2(matches(lines.at(0), QStringLiteral("^quick undock check: off the charger for at least \\d+ ms \\(2 "
                                                     "\"off\" answers\\), then on the charger again: the screensaver "
                                                     "stays$")),
                 qPrintable(lines.at(0)));
        QVERIFY2(matches(lines.at(1), QStringLiteral("^quick undock check: off the charger for at most \\d+ ms \\(1 "
                                                     "\"off\" answer\\), then on the charger again: the screensaver "
                                                     "stays$")),
                 qPrintable(lines.at(1)));
        QCOMPARE(lines.at(2), QStringLiteral("quick undock check: off the charger at 1 answer, then on the charger "
                                             "again: the screensaver stays"));
    }

    // Every form of the run line, to the digit, from times written into the check. The run: its first "off" answer
    // came at 1294 and the question of its last "off" answer left at 1545 (the one question of a single answer left
    // at 1250, before that answer came). Around it: the question of the last "on" answer before the run left at 1000,
    // and the "on" answer after it came at 1795 (after a single "off" answer: at 1545).
    void theRunLine_data() {
        QTest::addColumn<int>("answers");
        QTest::addColumn<bool>("onBefore");  // this watch had an "on the charger" answer before the run
        QTest::addColumn<bool>("onAfter");   // an "on the charger" answer ended the run
        QTest::addColumn<QString>("then");
        QTest::addColumn<bool>("stays");
        QTest::addColumn<QString>("text");
        const QString on = QStringLiteral("on the charger again");
        QTest::newRow("two, between two on answers")
            << 2 << true << true << on << true
            << QStringLiteral(
                   "for 251 to 795 ms (2 \"off\" answers), then on the charger again: the screensaver stays");
        QTest::newRow("one, between two on answers")
            << 1 << true << true << on << true
            << QStringLiteral(
                   "for at most 545 ms (1 \"off\" answer), then on the charger again: the screensaver stays");
        QTest::newRow("two, no on answer before")
            << 2 << false << true << on << true
            << QStringLiteral(
                   "for at least 251 ms (2 \"off\" answers), then on the charger again: the screensaver "
                   "stays");
        QTest::newRow("one, no on answer before")
            << 1 << false << true << on << true
            << QStringLiteral("at 1 answer, then on the charger again: the screensaver stays");
        QTest::newRow("two, cut short")
            << 2 << true << false << QStringLiteral("the display went off") << false
            << QStringLiteral("for at least 251 ms (2 \"off\" answers), then the display went off: the check stopped");
        QTest::newRow("one, a failed answer")
            << 1 << true << false << QStringLiteral("a failed answer") << true
            << QStringLiteral("at 1 answer, then a failed answer: the screensaver stays");
    }
    void theRunLine() {
        QFETCH(int, answers);
        QFETCH(bool, onBefore);
        QFETCH(bool, onAfter);
        QFETCH(QString, then);
        QFETCH(bool, stays);
        QFETCH(QString, text);
        ScreensaverLog log;
        m_check->m_cut.firstOffAt = 1294;
        m_check->m_cut.lastOffSentAt = answers == 1 ? 1250 : 1545;
        m_check->m_cut.lastOnSentAt = onBefore ? 1000 : -1;
        const qint64 onAgainAt = !onAfter ? -1 : answers == 1 ? 1545 : 1795;
        m_check->logRunEnded(answers, qPrintable(then), stays, onAgainAt);
        QCOMPARE(log.matching(QStringLiteral("quick undock check")),
                 QStringList{QStringLiteral("quick undock check: off the charger ") + text});
    }

    // a run cut short because the watch itself stopped is logged too, with what stopped it (the phantom-wake check
    // can switch the display off in the middle of a pick-up). In the app only "the screensaver closed", "the display
    // went off" and "the core disconnected" can be the reason: see UndockCheck::whyNotWatching(). Whatever came
    // before the run, such a line says "at least" only: no "on the charger" answer ended the run.
    void aRunCutShortByAStopLeavesALine_data() {
        QTest::addColumn<int>("how");
        QTest::addColumn<QString>("then");
        QTest::newRow("dark") << 0 << QStringLiteral("the display went off");
        QTest::newRow("closed") << 1 << QStringLiteral("the screensaver closed");
        QTest::newRow("the core's report") << 2 << QStringLiteral("the core reported the charger gone");
        QTest::newRow("Never") << 3 << QStringLiteral("Close on wake was set to Never");
        QTest::newRow("disconnected") << 4 << QStringLiteral("the core disconnected");
    }
    void aRunCutShortByAStopLeavesALine() {
        QFETCH(int, how);
        QFETCH(QString, then);
        ScreensaverLog log;
        startWatching();
        answer(200, onCharger());
        answer(200, offCharger());
        answer(200, offCharger());
        switch (how) {
            case 0:
                m_check->onPowerModeChanged(Power::Normal, Power::Low_power);
                break;
            case 1:
                m_check->setScreensaverOpen(false);
                break;
            case 2:
                m_check->onPowerSupplyChanged(false);
                break;
            case 3:
                m_check->setCloseOnWake(0);
                break;
            default:
                m_check->onCoreDisconnected();
                break;
        }
        QVERIFY(!m_check->m_watching);
        const QStringList lines = log.matching(QStringLiteral("quick undock check: off the charger"));
        QCOMPARE(lines.count(), 1);
        QVERIFY2(matches(lines.first(), QStringLiteral("^quick undock check: off the charger for at least \\d+ ms \\(2 "
                                                       "\"off\" answers\\), then %1: the check stopped$")
                                            .arg(then)),
                 qPrintable(lines.first()));
        // No quick close is waiting for the core here, so none of these, the core's report among them, writes the
        // line that answers one ("the core reported the charger gone ... after the quick undock check").
        QCOMPARE(log.matching(QStringLiteral("Close on wake:")).count(), 0);
    }

    // a stop with no run under way, and the run that closed the screensaver, leave no such line
    void noRunNoLine() {
        ScreensaverLog log;
        startWatching();
        answer(200, onCharger());
        m_check->onPowerModeChanged(Power::Normal, Power::Low_power);
        m_check->onPowerModeChanged(Power::Low_power, Power::Normal);
        m_check->m_pollTimer.stop();
        liftUntilClosed();
        QCOMPARE(log.matching(QStringLiteral("quick undock check: off the charger")).count(), 0);
        QCOMPARE(log.matching(QStringLiteral("closes the screensaver")).count(), 1);
    }

    // main.qml closes the screensaver inside closeRequested, which comes back as setScreensaverOpen(false) before the
    // emit returns. So the request is the last thing the check does at a close: by then the run is over, the watch
    // has stopped and the asking after the close has begun. The close leaves no "run that closed nothing" line, and
    // the asking goes on.
    void theCloseIsAskedForLast() {
        ScreensaverLog log;
        connect(m_check, &UndockCheck::closeRequested, this, [this]() { m_check->setScreensaverOpen(false); });
        startWatching();
        liftUntilClosed();
        QVERIFY(!m_check->screensaverOpen());
        QCOMPARE(log.matching(QStringLiteral("quick undock check: off the charger")).count(), 0);
        QCOMPARE(log.matching(QStringLiteral("closes the screensaver")).count(), 1);
        QVERIFY(!m_check->m_watching);
        QVERIFY(m_check->m_following);
        QVERIFY(m_check->m_pollTimer.isActive());
        QVERIFY(m_check->undockPending());
    }

    // A charger contact that flickers must not fill the log: RUN_LINES_PER_OPENING lines per opening, the last of them
    // saying so. Every such line counts, whether an answer ended the run or the watch stopped. 20, so that five bumps
    // in one opening get a line each even if a bump gives more than one run.
    void runLinesAreCappedPerOpening() {
        QCOMPARE(UndockCheck::RUN_LINES_PER_OPENING, 20);
        ScreensaverLog log;
        startWatching();
        answer(200, onCharger());
        // 24 runs of one "off" answer: every third is cut short by the display going off, the others end with "on"
        for (int i = 0; i < 24; ++i) {
            answer(200, offCharger());
            if (i % 3 == 2) {
                m_check->onPowerModeChanged(Power::Normal, Power::Low_power);
                m_check->onPowerModeChanged(Power::Low_power, Power::Normal);
                m_check->m_pollTimer.stop();
            } else {
                answer(200, onCharger());
            }
        }
        const QString part = QStringLiteral("quick undock check: off the charger");
        QStringList   lines = log.matching(part);
        QCOMPARE(lines.count(), 20);
        QCOMPARE(lines.filter(QStringLiteral(": the check stopped")).count(), 6);  // runs 3, 6, ... 18 of the 20
        QCOMPARE(lines.filter(QStringLiteral(": the screensaver stays")).count(), 14);
        const QString last = QStringLiteral(" (no more such lines until the screensaver opens again)");
        QVERIFY2(lines.at(19).endsWith(last), qPrintable(lines.at(19)));
        QVERIFY2(!lines.at(18).contains(last), qPrintable(lines.at(18)));
        // a new opening logs again
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        m_check->m_pollTimer.stop();
        answer(200, offCharger());
        answer(200, onCharger());
        lines = log.matching(part);
        QCOMPARE(lines.count(), 21);
        QVERIFY2(!lines.at(20).contains(last), qPrintable(lines.at(20)));
    }

    // After a close it goes on asking while undockPending, for the log only: however many "off" answers come, it
    // closes nothing again
    void afterACloseItKeepsAskingAndClosesNothing() {
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);  // as main.qml does at the close
        QCOMPARE(closeSpy.count(), 1);
        int asked = calls();
        for (int i = 0; i < 10; ++i) {
            answer(200, offCharger());
            QCOMPARE(calls(), ++asked);
        }
        QCOMPARE(closeSpy.count(), 1);
        QVERIFY(m_check->undockPending());
        QVERIFY(m_check->m_following);
        QVERIFY(!m_check->m_watching);
        QCOMPARE(m_check->m_offSinceClose, 10);
        QCOMPARE(m_check->m_backOnAt, qint64(-1));
    }

    // ... the first "on the charger" answer is noted and ends the asking; the wait for the core goes on. The line
    // written when that wait runs out says between which two moments the reading was "on the charger" again (the
    // question of the last "off" answer since the close, and that "on" answer), and how long it was off in all: at
    // least from the first "off" answer of the run that closed to that same question, at most from the question of
    // the last "on" answer before that run to that "on" answer. A bump longer than the delay, or a lift and put back.
    // Real time passes between every question and its answer and between every answer and the next question, so that
    // each moment the check keeps is told from its neighbours.
    void afterACloseTheFirstOnAnswerEndsTheAsking() {
        ScreensaverLog log;
        const int      gap = 20;
        startWatching();
        const qint64 onSent = m_check->m_sentAt;  // the question of the last "on" answer before the run
        answerSlowly(gap, 200, onCharger());
        answerSlowly(gap, 200, offCharger());
        const qint64 firstLo = m_arrivedLo;
        const qint64 firstHi = m_arrivedHi;
        answerSlowly(gap, 200, offCharger());
        answerSlowly(gap, 200, offCharger());  // closes; the question after it has left
        QVERIFY(m_check->undockPending());
        // the record of the close
        QVERIFY(arrivedThen(m_check->m_closedAt));
        const qint64 closedAt = m_check->m_closedAt;
        const qint64 firstOffAt = m_check->m_closedCut.firstOffAt;
        QVERIFY(firstOffAt >= firstLo && firstOffAt <= firstHi);
        QCOMPARE(m_check->m_closedCut.lastOffSentAt, m_answeredSentAt);
        QCOMPARE(m_check->m_closedCut.lastOnSentAt, onSent);
        m_check->setScreensaverOpen(false);

        answerSlowly(gap, 200, offCharger());  // an "off" answer since the close
        const qint64 offSent = m_answeredSentAt;
        QCOMPARE(m_check->m_offSinceClose, 1);
        QVERIFY(arrivedThen(m_check->m_lastOffAt));
        QCOMPARE(m_check->m_closedCut.lastOffSentAt, offSent);  // "at least" goes on up to this question
        QCOMPARE(m_check->m_closedCut.firstOffAt, firstOffAt);
        QCOMPARE(m_check->m_closedCut.lastOnSentAt, onSent);
        QCOMPARE(m_check->m_closedAt, closedAt);

        const int asked = calls();
        QTest::qWait(gap);
        give(200, onCharger());  // "on the charger" again
        QVERIFY(arrivedThen(m_check->m_backOnAt));
        const qint64 backOnAt = m_check->m_backOnAt;
        QVERIFY(!m_check->m_following);
        QVERIFY(!m_check->m_pollTimer.isActive());
        ask();
        QCOMPARE(calls(), asked);  // nothing more is asked
        QVERIFY(m_check->undockPending());
        letThePendingWaitRunOut();
        QCOMPARE(log.matching(QStringLiteral("did not report the charger gone")),
                 QStringList{QStringLiteral("Close on wake: the core did not report the charger gone within 6 s of "
                                            "the quick undock check: on the charger again between %1 and %2 s after "
                                            "the close, so off for %3 to %4 s in all")
                                 .arg(secondsDown(offSent - closedAt), secondsUp(backOnAt - closedAt),
                                      secondsDown(offSent - firstOffAt), secondsUp(backOnAt - onSent))});
    }

    // ... with no "off" answer between the close and that "on" answer, the line can only say "within"
    void afterACloseAnOnAnswerAtOnceSaysWithin() {
        ScreensaverLog log;
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        const UndockCheck::Cut cut = m_check->m_closedCut;
        QTest::qWait(20);
        give(200, onCharger());
        QVERIFY(arrivedThen(m_check->m_backOnAt));
        QVERIFY(!m_check->m_following);
        QCOMPARE(m_check->m_closedCut.lastOffSentAt, cut.lastOffSentAt);  // an "on" answer leaves the run's times alone
        letThePendingWaitRunOut();
        QCOMPARE(log.matching(QStringLiteral("did not report the charger gone")),
                 QStringList{QStringLiteral("Close on wake: the core did not report the charger gone within 6 s of "
                                            "the quick undock check: on the charger again within %1 s after the "
                                            "close, so off for %2 to %3 s in all")
                                 .arg(secondsUp(m_check->m_backOnAt - m_check->m_closedAt),
                                      secondsDown(cut.lastOffSentAt - cut.firstOffAt),
                                      secondsUp(m_check->m_backOnAt - cut.lastOnSentAt))});
    }

    // ... with "off" answers since and no "on" one, the line says when the last of them came and how many there were
    // (the core is late, or its report was missed; or the asking ended early, which that time then shows). An answer
    // without power_supply is neither "off" nor "on": it is not counted, and when it or a failed answer came after
    // the last "off" one, the line says "the last "off" answer" and not "the last answer".
    void afterACloseStillOffIsSaid_data() {
        QTest::addColumn<int>("offAnswers");
        QTest::addColumn<int>("after");  // after the last "off" answer: 0 nothing, 1 an answer without power_supply,
                                         // 2 a failed answer
        QTest::addColumn<QString>("what");
        QTest::newRow("one") << 1 << 0
                             << QStringLiteral(
                                    "still off the charger at the last answer, %1 s after the close (1 "
                                    "\"off\" answer since the close, none \"on\")");
        QTest::newRow("three") << 3 << 0
                               << QStringLiteral(
                                      "still off the charger at the last answer, %1 s after the close (3 "
                                      "\"off\" answers since the close, none \"on\")");
        QTest::newRow("three, then one without power_supply")
            << 3 << 1
            << QStringLiteral(
                   "still off the charger at the last \"off\" answer, %1 s after the close (3 \"off\" "
                   "answers since the close, none \"on\")");
        QTest::newRow("two, then a failed one")
            << 2 << 2
            << QStringLiteral(
                   "still off the charger at the last \"off\" answer, %1 s after the close (2 \"off\" "
                   "answers since the close, none \"on\")");
    }
    void afterACloseStillOffIsSaid() {
        QFETCH(int, offAnswers);
        QFETCH(int, after);
        QFETCH(QString, what);
        ScreensaverLog log;
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        for (int i = 0; i < offAnswers; ++i) {
            if (i > 0) {
                answer(200, noField());  // between two "off" answers
            }
            QTest::qWait(20);
            answer(200, offCharger());
        }
        QCOMPARE(m_check->m_offSinceClose, offAnswers);
        QVERIFY(arrivedThen(m_check->m_lastOffAt));
        const qint64 lastOffAt = m_check->m_lastOffAt;
        if (after != 0) {
            QTest::qWait(20);  // 0.02 s later: a line that took this answer's time would show
            if (after == 1) {
                answer(200, noField());
            } else {
                answer(0, QVariantMap(), QStringLiteral("timeout"));
            }
        }
        QCOMPARE(m_check->m_lastOffAt, lastOffAt);
        letThePendingWaitRunOut();
        QCOMPARE(log.matching(QStringLiteral("did not report the charger gone")),
                 QStringList{QStringLiteral("Close on wake: the core did not report the charger gone within 6 s of "
                                            "the quick undock check: ") +
                             what.arg(secondsNearest(lastOffAt - m_check->m_closedAt))});
    }

    // ... and with neither an "off" nor an "on" answer since the close the line says just that, whatever the reason:
    // nothing came, only answers without power_supply, or only failed ones. It names no cause (build 1.6.4-keys1's
    // line guessed "a bump it didn't see, or a lift and put back").
    void afterACloseWithoutAnOffOrOnAnswerTheLineSaysSo_data() {
        QTest::addColumn<int>("what");
        QTest::newRow("no answer at all") << 0;
        QTest::newRow("answers without power_supply") << 1;
        QTest::newRow("failed answers") << 2;
    }
    void afterACloseWithoutAnOffOrOnAnswerTheLineSaysSo() {
        QFETCH(int, what);
        ScreensaverLog log;
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        if (what == 1) {
            answer(200, noField());
            answer(200, noField());
        } else if (what == 2) {
            answer(0, QVariantMap(), QStringLiteral("timeout"));
            answer(500, QVariantMap(), QStringLiteral("Internal"));
        }
        letThePendingWaitRunOut();
        const QStringList lines = log.matching(QStringLiteral("did not report the charger gone"));
        QCOMPARE(lines.count(), 1);
        QCOMPARE(lines.first(),
                 QStringLiteral("Close on wake: the core did not report the charger gone within 6 s of the quick "
                                "undock check: no \"off\" or \"on\" answer since the close"));
    }

    // the asking after a close ends with undockPending: at the core's report, at the time-out and when the core
    // disconnects. An answer still on its way is dropped, and nothing more is asked.
    void afterACloseTheAskingEndsWithThePendingWait_data() {
        QTest::addColumn<int>("how");
        QTest::newRow("the core's report") << 0;
        QTest::newRow("the time-out") << 1;
        QTest::newRow("the core disconnects") << 2;
    }
    void afterACloseTheAskingEndsWithThePendingWait() {
        QFETCH(int, how);
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        const int late = MockCoreRecorder::lastBatteryRestId();
        switch (how) {
            case 0:
                m_check->onPowerSupplyChanged(false);
                break;
            case 1:
                letThePendingWaitRunOut();
                break;
            default:
                m_check->onCoreDisconnected();
                break;
        }
        QVERIFY(!m_check->undockPending());
        QVERIFY(!m_check->m_following);
        QVERIFY(!m_check->m_pollTimer.isActive());
        const int asked = calls();
        emit m_api->respBatteryRest(late, 200, offCharger(), QString());
        m_check->poll();
        QCOMPARE(calls(), asked);
        QCOMPARE(m_check->m_offSinceClose, 0);
        QCOMPARE(closeSpy.count(), 1);
    }

    // Three failed answers in a row end the asking after a close, quietly: nothing depends on those answers and no
    // screensaver is open, so they neither write the one warning of the UI run nor mark the check as stopped. The wait
    // for the core's report goes on. A watch that fails three times later still gets that warning.
    void afterACloseThreeFailuresEndTheAskingQuietly() {
        WarningCount warnings;
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        answer(500, QVariantMap(), QStringLiteral("Internal"));
        QVERIFY(m_check->m_following);
        const int asked = calls();
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        QVERIFY(!m_check->m_following);
        QVERIFY(!m_check->m_pollTimer.isActive());
        QCOMPARE(calls(), asked);
        QVERIFY(m_check->undockPending());
        QVERIFY(!m_check->m_warned);
        QVERIFY(!m_check->m_stoppedThisOpen);
        QCOMPARE(warnings.count(), 0);

        m_check->setScreensaverOpen(true);  // a later watch
        m_check->m_pollTimer.stop();
        QVERIFY(m_check->m_watching);
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        QVERIFY(m_check->m_watching);
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        QVERIFY(!m_check->m_watching);
        QVERIFY(m_check->m_stoppedThisOpen);
        QVERIFY(m_check->m_warned);
        QCOMPARE(warnings.count(), 1);
    }

    // a 404 and a 401 / 403 after a close say something true about the endpoint and are handled as while watching:
    // the warning, and off for the UI run or until the next opening
    void afterACloseARefusalIsHandledAsWhileWatching_data() {
        QTest::addColumn<int>("code");
        QTest::addColumn<bool>("forTheRun");
        QTest::newRow("404") << 404 << true;
        QTest::newRow("401") << 401 << false;
        QTest::newRow("403") << 403 << false;
    }
    void afterACloseARefusalIsHandledAsWhileWatching() {
        QFETCH(int, code);
        QFETCH(bool, forTheRun);
        WarningCount warnings;
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        answer(code, QVariantMap(), QStringLiteral("refused"));
        QVERIFY(!m_check->m_following);
        QVERIFY(m_check->undockPending());
        QCOMPARE(warnings.count(), 1);
        QCOMPARE(m_check->m_disabled, forTheRun);
        m_check->setScreensaverOpen(true);
        QCOMPARE(m_check->m_watching, !forTheRun);
    }

    // After a close, as while watching, it asks only while the display is on: its timer stops when the display goes
    // off and starts again when it comes back, with a first question at once. Once the asking has ended, the display
    // starts nothing.
    void afterACloseItAsksOnlyWhileTheDisplayIsOn() {
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        QVERIFY(m_check->m_pollTimer.isActive());
        m_check->onPowerModeChanged(Power::Normal, Power::Idle);  // dimmed: the display is still on
        QVERIFY(m_check->m_pollTimer.isActive());
        m_check->onPowerModeChanged(Power::Idle, Power::Low_power);
        QVERIFY(m_check->m_following);  // the wait goes on
        QVERIFY(!m_check->m_pollTimer.isActive());
        const int asked = calls();
        answer(200, offCharger());  // the answer that was on its way is taken; no new question in the dark
        QCOMPARE(m_check->m_offSinceClose, 1);
        QCOMPARE(calls(), asked);
        m_check->poll();
        QCOMPARE(calls(), asked);
        m_check->onPowerModeChanged(Power::Low_power, Power::Normal);
        QVERIFY(m_check->m_pollTimer.isActive());
        QCOMPARE(calls(), asked + 1);  // asked at once

        answer(200, onCharger());  // ends the asking
        QVERIFY(!m_check->m_following);
        QVERIFY(m_check->undockPending());
        m_check->onPowerModeChanged(Power::Normal, Power::Low_power);
        m_check->onPowerModeChanged(Power::Low_power, Power::Normal);
        QVERIFY(!m_check->m_pollTimer.isActive());
        QCOMPARE(calls(), asked + 1);
    }

    // The screensaver opens again while undockPending (nothing in main.qml does that with the settings the UI offers,
    // on a remote or in the desktop simulator; in the simulator an idle timeout under 6 s written into config.ini
    // would; kept as a safety net): the watch takes over. An answer asked for before that is dropped, the first
    // question leaves at once, and another full run of "off" answers closes it again. undockPending stays true.
    void aReopeningWhilePendingIsWatchedAgain() {
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        QSignalSpy pendingSpy(m_check, &UndockCheck::undockPendingChanged);
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        answer(200, offCharger());
        const int late = MockCoreRecorder::lastBatteryRestId();
        const int asked = calls();
        m_check->setScreensaverOpen(true);
        m_check->m_pollTimer.stop();
        QVERIFY(m_check->m_watching);
        QVERIFY(m_check->m_following);
        QCOMPARE(calls(), asked + 1);
        emit m_api->respBatteryRest(late, 200, offCharger(), QString());
        QCOMPARE(m_check->m_run.offInARow, 0);
        answer(200, offCharger());
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 1);
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 2);
        QVERIFY(m_check->undockPending());
        QCOMPARE(pendingSpy.count(), 1);  // it stayed true all along
        QVERIFY(m_check->m_pendingTimer.isActive());
        QVERIFY(m_check->m_following);
        QVERIFY(!m_check->m_watching);
    }

    // That second close starts the wait for the core's report and the record of the close anew. The wait is 400 ms
    // here instead of 6 s, and 300 ms of the first one have passed when the second close comes: the second wait must
    // still last its whole 400 ms (its timer is precise: never early, and a slow machine only makes it longer). The
    // record: the first close had noted an "off" answer and an "on" one; the second starts without them, and its line
    // says what came after it.
    void aSecondCloseStartsTheWaitAndItsRecordAnew() {
        ScreensaverLog log;
        QSignalSpy     closeSpy(m_check, &UndockCheck::closeRequested);
        m_check->m_pendingTimer.setInterval(400);
        // 2 ms first: at 0 ms of the check's clock, a moment that was kept could not be told from none kept
        QTest::qWait(2);
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        answer(200, offCharger());
        QCOMPARE(m_check->m_offSinceClose, 1);
        m_check->setScreensaverOpen(true);  // the watch takes over
        m_check->m_pollTimer.stop();
        answer(200, onCharger());
        QVERIFY(m_check->m_backOnAt >= 0);
        QTest::qWait(300);
        answer(200, offCharger());
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 1);
        give(200, offCharger());  // the second close
        const qint64 secondLo = m_arrivedLo;
        QCOMPARE(closeSpy.count(), 2);
        QVERIFY(m_check->undockPending());
        QVERIFY(arrivedThen(m_check->m_closedAt));  // this close, not the first
        // ... and its run, not the first close's (whose first "off" answer came 300 ms and more earlier)
        QCOMPARE(m_check->m_closedCut.firstOffAt, m_check->m_cut.firstOffAt);
        QCOMPARE(m_check->m_backOnAt, qint64(-1));
        QCOMPARE(m_check->m_offSinceClose, 0);
        QVERIFY(m_check->m_following);
        m_check->setScreensaverOpen(false);
        ask();
        answer(200, offCharger());
        QTRY_VERIFY_WITH_TIMEOUT(!m_check->undockPending(), 3000);
        const qint64 waited = now() - secondLo;
        QVERIFY2(waited >= 400, qPrintable(QString::number(waited)));
        const QStringList lines = log.matching(QStringLiteral("did not report the charger gone"));
        QVERIFY(!lines.isEmpty());
        QVERIFY2(lines.last().contains(QStringLiteral(": still off the charger at the last answer, ")) &&
                     lines.last().endsWith(QStringLiteral("(1 \"off\" answer since the close, none \"on\")")),
                 qPrintable(lines.last()));
    }

    // ... and an "on the charger" answer of that watch is noted for the log and ends only the asking for the log: the
    // watch goes on, and a later lift closes the screensaver again
    void aReopeningWhilePending_anOnAnswerIsNotedAndTheWatchGoesOn() {
        QSignalSpy closeSpy(m_check, &UndockCheck::closeRequested);
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);  // the watch takes over; its timer runs (not stopped in this test)
        QVERIFY(m_check->m_pollTimer.isActive());
        answer(200, onCharger());
        QVERIFY(!m_check->m_following);
        QVERIFY(arrivedThen(m_check->m_backOnAt));
        QVERIFY(m_check->m_watching);
        QVERIFY(m_check->m_pollTimer.isActive());  // the watch's timer goes on: only the asking for the log ended
        QVERIFY(m_check->undockPending());
        answer(200, offCharger());
        answer(200, offCharger());
        answer(200, offCharger());
        QCOMPARE(closeSpy.count(), 2);
        QVERIFY(m_check->m_following);
    }

    // when the watch that took over stops, the asking for the log stops with it (they share the poll timer)
    void aReopeningWhilePending_theWatchStoppingEndsTheAsking() {
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        QVERIFY(m_check->m_watching);
        QVERIFY(m_check->m_following);
        m_check->onPowerModeChanged(Power::Normal, Power::Low_power);
        QVERIFY(!m_check->m_watching);
        QVERIFY(!m_check->m_following);
        QVERIFY(!m_check->m_pollTimer.isActive());
        QVERIFY(m_check->undockPending());
    }

    // ... and it fails like any watch: the third failed answer in a row stops it, with the one warning of the UI run.
    // Only an asking that is nothing but the asking for the log ends quietly
    // (afterACloseThreeFailuresEndTheAskingQuietly).
    void aReopeningWhilePending_theWatchFailsLikeAnyWatch() {
        WarningCount warnings;
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        m_check->setScreensaverOpen(true);
        m_check->m_pollTimer.stop();
        QVERIFY(m_check->m_watching);
        QVERIFY(m_check->m_following);
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        QVERIFY(m_check->m_watching);
        QCOMPARE(warnings.count(), 0);
        answer(0, QVariantMap(), QStringLiteral("timeout"));
        QVERIFY(!m_check->m_watching);
        QVERIFY(!m_check->m_following);
        QVERIFY(m_check->m_stoppedThisOpen);
        QCOMPARE(warnings.count(), 1);
        QVERIFY(m_check->undockPending());  // the wait for the core's report goes on
    }

    // Everything is in place before the check says that a close is pending: the 6 s wait, the asking for the log and
    // its timer. A slot on undockPendingChanged that ends the wait there and then (here the core's report) therefore
    // leaves nothing running behind it. The close is still asked for, last.
    void aWaitEndedInsideItsOwnSignalLeavesNothingRunning() {
        ScreensaverLog log;
        QStringList    order;
        connect(m_check, &UndockCheck::undockPendingChanged, this, [this, &order]() {
            order << (m_check->undockPending() ? QStringLiteral("pending") : QStringLiteral("not pending"));
            if (m_check->undockPending()) {
                m_check->onPowerSupplyChanged(false);
            }
        });
        connect(m_check, &UndockCheck::closeRequested, this, [&order]() { order << QStringLiteral("close"); });
        startWatching();
        answer(200, onCharger());
        answer(200, offCharger());
        answer(200, offCharger());
        const int asked = calls();
        answer(200, offCharger());
        QCOMPARE(order,
                 QStringList({QStringLiteral("pending"), QStringLiteral("not pending"), QStringLiteral("close")}));
        QVERIFY(!m_check->undockPending());
        QVERIFY(!m_check->m_pendingTimer.isActive());
        QVERIFY(!m_check->m_following);
        QVERIFY(!m_check->m_pollTimer.isActive());
        QVERIFY(!m_check->m_watching);
        QCOMPARE(calls(), asked);  // and nothing more is asked
        QCOMPARE(log.matching(QStringLiteral("the core reported the charger gone")).count(), 1);
    }

    // Every form of the line written when the 6 s run out, to the digit, from times written into the check. The close
    // came at 10000. The run that closed: its first "off" answer came at 9451, the question of its last "off" answer
    // left at 9955 (504 ms later), and the question of the last "on" answer before it left at 9161, or there was no
    // such answer in that watch. Since the close, in ms counted from it: when the question of the last "off" answer
    // left (offSent), when that answer came (offArrived), and when the first "on" answer came (backOn; -1: none).
    // A lower bound is rounded down, an upper bound up and a moment to the nearest, and each rounding has figures
    // here that another one would print differently: 267 and 816 ms as lower bounds (0.26 and 0.81); 503, 253, 1342
    // and 1092 ms as upper bounds (0.51, 0.26, 1.35 and 1.10); 5796 and 293 ms as moments (5.80 and 0.29).
    void theTimeOutLine_data() {
        QTest::addColumn<bool>("onBefore");
        QTest::addColumn<int>("offSinceClose");
        QTest::addColumn<qint64>("offSent");
        QTest::addColumn<qint64>("offArrived");
        QTest::addColumn<bool>("offAtLastAnswer");
        QTest::addColumn<qint64>("backOn");
        QTest::addColumn<QString>("what");
        QTest::newRow("on again, after an off answer")
            << true << 1 << qint64(267) << qint64(312) << true << qint64(503)
            << QStringLiteral(
                   "on the charger again between 0.26 and 0.51 s after the close, so off for 0.81 to 1.35 "
                   "s in all");
        QTest::newRow("on again at once")
            << true << 0 << qint64(0) << qint64(0) << false << qint64(253)
            << QStringLiteral("on the charger again within 0.26 s after the close, so off for 0.50 to 1.10 s in all");
        QTest::newRow("on again, after an off answer; no on answer before the run")
            << false << 1 << qint64(267) << qint64(312) << true << qint64(503)
            << QStringLiteral(
                   "on the charger again between 0.26 and 0.51 s after the close, so off for at least 0.81 "
                   "s in all");
        QTest::newRow("on again at once; no on answer before the run")
            << false << 0 << qint64(0) << qint64(0) << false << qint64(253)
            << QStringLiteral("on the charger again within 0.26 s after the close, so off for at least 0.50 s in all");
        QTest::newRow("still off, 23 answers")
            << true << 23 << qint64(5751) << qint64(5796) << true << qint64(-1)
            << QStringLiteral(
                   "still off the charger at the last answer, 5.80 s after the close (23 \"off\" answers "
                   "since the close, none \"on\")");
        QTest::newRow("still off, one answer")
            << true << 1 << qint64(251) << qint64(293) << true << qint64(-1)
            << QStringLiteral(
                   "still off the charger at the last answer, 0.29 s after the close (1 \"off\" answer "
                   "since the close, none \"on\")");
        QTest::newRow("still off, then an answer that was not off")
            << true << 4 << qint64(1001) << qint64(1045) << false << qint64(-1)
            << QStringLiteral(
                   "still off the charger at the last \"off\" answer, 1.05 s after the close (4 \"off\" "
                   "answers since the close, none \"on\")");
        QTest::newRow("neither") << true << 0 << qint64(0) << qint64(0) << false << qint64(-1)
                                 << QStringLiteral("no \"off\" or \"on\" answer since the close");
    }
    void theTimeOutLine() {
        QFETCH(bool, onBefore);
        QFETCH(int, offSinceClose);
        QFETCH(qint64, offSent);
        QFETCH(qint64, offArrived);
        QFETCH(bool, offAtLastAnswer);
        QFETCH(qint64, backOn);
        QFETCH(QString, what);
        ScreensaverLog log;
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);
        const qint64 closedAt = 10000;
        m_check->m_closedAt = closedAt;
        m_check->m_closedCut.firstOffAt = 9451;
        m_check->m_closedCut.lastOffSentAt = offSinceClose > 0 ? closedAt + offSent : 9955;
        m_check->m_closedCut.lastOnSentAt = onBefore ? 9161 : -1;
        m_check->m_offSinceClose = offSinceClose;
        m_check->m_lastOffAt = closedAt + offArrived;
        m_check->m_offAtLastAnswer = offAtLastAnswer;
        m_check->m_backOnAt = backOn < 0 ? -1 : closedAt + backOn;
        m_check->pendingTimedOut();
        QVERIFY(!m_check->undockPending());
        QCOMPARE(log.matching(QStringLiteral("did not report the charger gone")),
                 QStringList{QStringLiteral("Close on wake: the core did not report the charger gone within 6 s of "
                                            "the quick undock check: ") +
                             what});
    }

    // The line for the core's report: how long after the close it came, as a plain number (gone 3.08 s after, not
    // gone "3.08" s after). At a second lift, later, it counts from that lift's close: the review found that nothing
    // noticed when it counted from the first.
    void theCoreReportLineCountsFromItsOwnClose() {
        ScreensaverLog           log;
        const QString            part = QStringLiteral("the core reported the charger gone");
        const QRegularExpression coreLine(
            QStringLiteral("^Close on wake: the core reported the charger gone "
                           "(\\d+)\\.(\\d\\d) s after the quick undock check$"));
        // 2 ms first: at 0 ms of the check's clock, a moment that was kept could not be told from none kept
        QTest::qWait(2);
        startWatching();
        liftUntilClosed();
        m_check->setScreensaverOpen(false);  // main.qml closes
        QTest::qWait(30);
        qint64 lo = now() - m_check->m_closedAt;
        m_check->onPowerSupplyChanged(false);  // the core's report of the first lift
        qint64 hi = now() - m_check->m_closedAt;
        QVERIFY(!m_check->undockPending());
        QCOMPARE(log.matching(part).count(), 1);
        QRegularExpressionMatch m = coreLine.match(log.matching(part).at(0));
        QVERIFY2(m.hasMatch(), qPrintable(log.matching(part).at(0)));
        qint64 hundredths = m.captured(1).toLongLong() * 100 + m.captured(2).toLongLong();
        QVERIFY2(hundredths >= (lo + 5) / 10 && hundredths <= (hi + 5) / 10, qPrintable(log.matching(part).at(0)));
        QVERIFY(hundredths >= 3);

        m_check->onPowerSupplyChanged(true);  // back on the charger
        m_check->setScreensaverOpen(true);    // the screensaver opens again
        m_check->m_pollTimer.stop();
        answer(200, offCharger());
        answer(200, offCharger());
        give(200, offCharger());  // the second lift closes (the endpoint is trusted since the first)
        QVERIFY(m_check->undockPending());
        QVERIFY(arrivedThen(m_check->m_closedAt));
        m_check->setScreensaverOpen(false);
        lo = now() - m_check->m_closedAt;
        m_check->onPowerSupplyChanged(false);  // the core's report of the second lift
        hi = now() - m_check->m_closedAt;
        QCOMPARE(log.matching(part).count(), 2);
        m = coreLine.match(log.matching(part).at(1));
        QVERIFY2(m.hasMatch(), qPrintable(log.matching(part).at(1)));
        hundredths = m.captured(1).toLongLong() * 100 + m.captured(2).toLongLong();
        QVERIFY2(hundredths >= (lo + 5) / 10 && hundredths <= (hi + 5) / 10, qPrintable(log.matching(part).at(1)));
    }

    // ... and when the reading had been "on the charger" again before the core's report came (the remote was put
    // back, and the core saw the charger gone all the same or later), the line adds when, counted from the close too.
    // The lift is slow here: the run that closes takes 80 ms or more, so that a line counting from its first "off"
    // answer instead of from the close would show. The core's report comes 160 ms or more after the close. The moment
    // of the "on" answer is written into the check (0.114 and 0.116 s after the close: a moment is rounded to the
    // nearest).
    void theCoreReportLineAddsWhenTheReadingWasOnAgain_data() {
        QTest::addColumn<qint64>("backOn");
        QTest::addColumn<QString>("said");
        QTest::newRow("0.114 s after the close") << qint64(114) << QStringLiteral("0.11");
        QTest::newRow("0.116 s after the close") << qint64(116) << QStringLiteral("0.12");
    }
    void theCoreReportLineAddsWhenTheReadingWasOnAgain() {
        QFETCH(qint64, backOn);
        QFETCH(QString, said);
        ScreensaverLog log;
        startWatching();
        answer(200, onCharger());
        answerSlowly(20, 200, offCharger());
        answerSlowly(20, 200, offCharger());
        answerSlowly(20, 200, offCharger());  // closes
        QVERIFY(m_check->undockPending());
        const qint64 closedAt = m_check->m_closedAt;
        QVERIFY(arrivedThen(closedAt));
        QVERIFY(closedAt - m_check->m_closedCut.firstOffAt >= 80);
        m_check->setScreensaverOpen(false);
        give(200, onCharger());  // ends the asking
        QVERIFY(m_check->m_backOnAt >= 0);
        QTest::qWait(140);
        m_check->m_backOnAt = closedAt + backOn;
        const qint64 lo = now() - closedAt;
        m_check->onPowerSupplyChanged(false);
        const qint64                  hi = now() - closedAt;
        const QStringList             lines = log.matching(QStringLiteral("the core reported the charger gone"));
        const QRegularExpressionMatch m =
            QRegularExpression(QStringLiteral("^Close on wake: the core reported the charger gone (\\d+)\\.(\\d\\d) s "
                                              "after the quick undock check; the reading had said \"on the charger\" "
                                              "again (\\d+\\.\\d\\d) s after the close$"))
                .match(lines.value(0));
        QCOMPARE(lines.count(), 1);
        QVERIFY2(m.hasMatch(), qPrintable(lines.first()));
        const qint64 hundredths = m.captured(1).toLongLong() * 100 + m.captured(2).toLongLong();
        QVERIFY2(hundredths >= (lo + 5) / 10 && hundredths <= (hi + 5) / 10, qPrintable(lines.first()));
        QVERIFY(lo >= 160);
        QCOMPARE(m.captured(3), said);
    }
};

}  // namespace hw
}  // namespace uc

QTEST_GUILESS_MAIN(uc::hw::UndockCheckTest)

#include "test_undockCheck.moc"
