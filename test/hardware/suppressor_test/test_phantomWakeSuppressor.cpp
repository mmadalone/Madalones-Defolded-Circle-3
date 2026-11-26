// Copyright (c) 2026 madalone. Unit tests for PhantomWakeSuppressor (Mod 6).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// COVERAGE GOALS
// ──────────────
// 1. PRIMARY: regression-prevention for the same v1.4.14-style setPowerMode
//    body-shape bug that affected Mod 6 from v1.4.20 → v1.4.21 (force-back was
//    a no-op due to firmware rejecting "power_mode" field name with HTTP 400).
//    See test_forceLowPower_body_shape.
// 2. State machine — the wake-detection path at phantomWakeSuppressor.cpp::onPowerModeChanged
// 3. wasAsleep gate — distinguishes phantom wakes from Mod 5's IDLE→NORMAL pings
// 4. Grace cancellation paths (user input, entity command, dock event, core disconnect)
// 5. v1.4.23 input-lookback skip-grace logic (the wake-press timing fix)
// 6. setGraceMs / setInputLookbackMs clamping
// 7. setEnabled mid-grace cancels the active timer
// 8. v1.6.5: the quick undock check's close counts as a real wake (onQuickUndock), and the log names what cancelled a
//    grace or made a wake skip it

#include <QtTest>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QSignalSpy>

#include "../../../src/core/core.h"
#include "../../../src/hardware/phantomWakeSuppressor.h"
#include "../../../src/system/power.h"
#include "../mock_core_api.h"

namespace {

// the info lines of uc.hw while one of these exists, oldest first (they still reach QtTest's handler)
class HwLog {
 public:
    HwLog() {
        s_lines.clear();
        s_previous = qInstallMessageHandler(&HwLog::handle);
    }
    ~HwLog() { qInstallMessageHandler(s_previous); }
    HwLog(const HwLog&) = delete;
    HwLog& operator=(const HwLog&) = delete;

    // the lines that contain `part`
    QStringList matching(const QString& part) const { return s_lines.filter(part); }

 private:
    static void handle(QtMsgType type, const QMessageLogContext& context, const QString& message) {
        if (type == QtInfoMsg && context.category && qstrcmp(context.category, "uc.hw") == 0) {
            s_lines.append(message);
        }
        if (s_previous) {
            s_previous(type, context, message);
        }
    }

    static QStringList      s_lines;
    static QtMessageHandler s_previous;
};

QStringList      HwLog::s_lines;
QtMessageHandler HwLog::s_previous = nullptr;

}  // namespace

class PhantomWakeSuppressorTest : public QObject {
    Q_OBJECT

 private:
    uc::core::Api*                  m_api        = nullptr;
    uc::hw::PhantomWakeSuppressor*  m_suppressor = nullptr;

    using PowerMode = uc::hw::Power::PowerMode;

    // Default to short grace window for fast tests.
    static constexpr int kFastGraceMs = 100;

 private slots:
    void init() {
        uc::test::MockCoreRecorder::reset();
        m_api        = new uc::core::Api(QString("ws://test"), this);
        m_suppressor = new uc::hw::PhantomWakeSuppressor(m_api, this);
        // Default to enabled with fast grace window for tests.
        m_suppressor->setEnabled(true);
        m_suppressor->setGraceMs(kFastGraceMs);
    }

    void cleanup() {
        delete m_suppressor;
        m_suppressor = nullptr;
        delete m_api;
        m_api = nullptr;
    }

    // ──────────────────────────────────────────────────────────────────────
    // 1. REGRESSION-PREVENTION: forceLowPower() sends correct WS RPC body.
    //    Mirror of the keeper's body-shape test. The bug applied to BOTH Mod 5
    //    and Mod 6 — same Api::setPowerMode call site.
    // ──────────────────────────────────────────────────────────────────────
    void test_forceLowPower_body_shape() {
        // Trigger a phantom wake: LOW_POWER → NORMAL with no input.
        // Wait for the grace window to expire so forceLowPower fires.
        m_suppressor->setInputLookbackMs(0);  // disable lookback so we don't accidentally skip grace
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        // Wait for grace timer to expire + forceLowPower to fire.
        QTest::qWait(kFastGraceMs + 50);
        QVERIFY(!m_suppressor->getArmed());

        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);

        // Body shape: msg_data must contain "mode" with value "LOW_POWER".
        const QVariantMap msgData = uc::test::MockCoreRecorder::lastSentMsgData();
        QVERIFY2(msgData.contains("mode"),
                 "WS RPC body must use field name 'mode' (not 'power_mode'). "
                 "v1.4.22 fix at core.cpp:821 — firmware's serde rejects 'power_mode' with HTTP 400.");
        QVERIFY2(!msgData.contains("power_mode"),
                 "WS RPC body must NOT contain 'power_mode' field.");
        QCOMPARE(msgData.value("mode").toString(), QString("LOW_POWER"));
        QCOMPARE(uc::test::MockCoreRecorder::lastPowerModeValue(), QString("LOW_POWER"));

        const QString json = uc::test::MockCoreRecorder::lastSentRequestJson();
        QVERIFY(json.contains("\"kind\":\"req\""));
        QVERIFY(json.contains("\"msg\":\"set_power_mode\""));
        QVERIFY(json.contains("\"mode\":\"LOW_POWER\""));
        QVERIFY2(!json.contains("\"power_mode\":"),
                 qPrintable(QString("Outgoing JSON must not contain 'power_mode' field: %1").arg(json)));
    }

    // ──────────────────────────────────────────────────────────────────────
    // 2. onPowerModeChanged matrix
    // ──────────────────────────────────────────────────────────────────────
    void test_onPowerModeChanged_lowPowerToNormal_armsGrace() {
        m_suppressor->setInputLookbackMs(0);  // skip-grace logic disabled
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());
    }

    void test_onPowerModeChanged_suspendToNormal_armsGrace() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Suspend, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());
    }

    void test_onPowerModeChanged_idleToNormal_doesNotArm() {
        // wasAsleep gate: IDLE → NORMAL is Mod 5's ping artifact, NOT a wake.
        // This is the precedent at entityController.cpp:761-762 that the suppressor
        // explicitly mirrors (see phantomWakeSuppressor.cpp:78-84).
        m_suppressor->onPowerModeChanged(PowerMode::Idle, PowerMode::Normal);
        QVERIFY(!m_suppressor->getArmed());
    }

    void test_onPowerModeChanged_normalToNormal_doesNothing() {
        m_suppressor->onPowerModeChanged(PowerMode::Normal, PowerMode::Normal);
        QVERIFY(!m_suppressor->getArmed());
    }

    void test_onPowerModeChanged_normalToIdle_cancelsActiveGrace() {
        // Set up active grace.
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        // Firmware itself transitions away from NORMAL during grace —
        // the suppressor should defer to the firmware's own transition.
        m_suppressor->onPowerModeChanged(PowerMode::Normal, PowerMode::Idle);
        QVERIFY(!m_suppressor->getArmed());
    }

    void test_onPowerModeChanged_disabledNeverArms() {
        m_suppressor->setEnabled(false);
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(!m_suppressor->getArmed());
    }

    // ──────────────────────────────────────────────────────────────────────
    // 3. Grace cancellation paths
    // ──────────────────────────────────────────────────────────────────────
    void test_onUserInput_duringGrace_cancels() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        m_suppressor->onUserInput();
        QVERIFY(!m_suppressor->getArmed());

        // Wait past grace window — forceLowPower should NOT fire.
        QTest::qWait(kFastGraceMs + 50);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_onEntityCommandIssued_mediaCommand_duringGrace_cancels() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        // Real command form ("<entity type>.<command>"); the v1.5.4 allowlist never matched it.
        m_suppressor->onEntityCommandIssued("media_player.tv", "media_player.play_pause");
        QVERIFY(!m_suppressor->getArmed());

        QTest::qWait(kFastGraceMs + 50);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_onEntityCommandIssued_anyCommand_duringGrace_cancels() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        // Waking the remote by switching a light on is a real wake, not a phantom one.
        m_suppressor->onEntityCommandIssued("light.desk", "light.toggle");
        QVERIFY(!m_suppressor->getArmed());

        QTest::qWait(kFastGraceMs + 50);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_onPowerSupplyChanged_duringGrace_cancels_dockEvent() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        // Dock event (placing on charger) = user activity.
        m_suppressor->onPowerSupplyChanged(true);
        QVERIFY(!m_suppressor->getArmed());
    }

    void test_onPowerSupplyChanged_duringGrace_cancels_undockEvent() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        // Undock = user activity too (both directions count).
        m_suppressor->onPowerSupplyChanged(false);
        QVERIFY(!m_suppressor->getArmed());
    }

    void test_onCoreDisconnected_duringGrace_cancels() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        m_suppressor->onCoreDisconnected();
        QVERIFY(!m_suppressor->getArmed());

        QTest::qWait(kFastGraceMs + 50);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_onCoreDisconnected_noActiveGrace_isHarmless() {
        // Defensive: calling disconnect when no grace is active should be a no-op.
        QVERIFY(!m_suppressor->getArmed());
        m_suppressor->onCoreDisconnected();
        QVERIFY(!m_suppressor->getArmed());
    }

    // v1.6.5: the quick undock check found the remote off its charger (UndockCheck::closeRequested). That counts as
    // activity: a grace window that is still running is cancelled, and the display stays on. On a remote the close
    // has to come before the grace runs out for that: about 0.6-0.85 s after the wake at the default Undock delay.
    void test_onQuickUndock_duringGrace_cancels() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        m_suppressor->onQuickUndock();
        QVERIFY(!m_suppressor->getArmed());

        QTest::qWait(kFastGraceMs + 50);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_onQuickUndock_noActiveGrace_isHarmless() {
        QSignalSpy armedSpy(m_suppressor, &uc::hw::PhantomWakeSuppressor::armedChanged);
        QVERIFY(!m_suppressor->getArmed());
        m_suppressor->onQuickUndock();
        QVERIFY(!m_suppressor->getArmed());
        QCOMPARE(armedSpy.count(), 0);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    // v1.6.5: the cancel line names what cancelled the grace. A charger change was logged as "user input" before,
    // which made a device log read as if a button had been pressed.
    void test_cancelReasons_nameTheirSource_data() {
        QTest::addColumn<int>("source");
        QTest::addColumn<QString>("reason");
        QTest::newRow("a key or a touch") << 0 << QStringLiteral("user input");
        QTest::newRow("an entity command") << 1 << QStringLiteral("entity command");
        QTest::newRow("the core's charger report") << 2 << QStringLiteral("charger change");
        QTest::newRow("the quick undock check") << 3 << QStringLiteral("off the charger (quick undock check)");
    }

    void test_cancelReasons_nameTheirSource() {
        QFETCH(int, source);
        QFETCH(QString, reason);
        HwLog log;
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());
        switch (source) {
            case 0:
                m_suppressor->onUserInput();
                break;
            case 1:
                m_suppressor->onEntityCommandIssued("light.desk", "light.toggle");
                break;
            case 2:
                m_suppressor->onPowerSupplyChanged(false);
                break;
            default:
                m_suppressor->onQuickUndock();
                break;
        }
        QVERIFY(!m_suppressor->getArmed());
        const QStringList lines = log.matching(QStringLiteral("PhantomWakeSuppressor cancelled:"));
        QCOMPARE(lines.count(), 1);
        QCOMPARE(lines.first(), QStringLiteral("PhantomWakeSuppressor cancelled: %1").arg(reason));
    }

    // ──────────────────────────────────────────────────────────────────────
    // 4. v1.4.23 input-lookback skip-grace
    // ──────────────────────────────────────────────────────────────────────
    void test_inputLookback_recentInput_skipsGrace() {
        // Recent input, then wake — should SKIP arming (not just cancel).
        m_suppressor->setInputLookbackMs(500);
        m_suppressor->onUserInput();  // sets m_lastInputTimer to "now"

        // Immediately a wake event arrives — "input was recent" → skip arming.
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY2(!m_suppressor->getArmed(),
                 "Wake within m_inputLookbackMs of last input should skip arming entirely (v1.4.23 fix)");
    }

    void test_inputLookback_zeroDisablesSkip() {
        // Lookback 0 disables the skip — even with recent input, grace arms.
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onUserInput();
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());
    }

    void test_inputLookback_oldInputArmsNormally() {
        // Input older than lookback should NOT skip the grace timer.
        m_suppressor->setInputLookbackMs(50);  // very short window
        m_suppressor->onUserInput();
        QTest::qWait(100);  // wait past lookback window
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());
    }

    // v1.6.5: a close by the quick undock check counts as recent activity too, and the skip line names the kind of
    // activity instead of calling everything "input". It also says how long ago that activity was: 60 ms pass here
    // (QTest::qWait waits at least that long), so that this number is told from the lookback printed after it. The
    // lookback is the longest there is, 2000 ms: a machine that stalls for less than 1.9 s between the two calls
    // changes nothing.
    void test_inputLookback_quickUndockCounts() {
        HwLog log;
        m_suppressor->setInputLookbackMs(2000);
        m_suppressor->onQuickUndock();
        QTest::qWait(60);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(!m_suppressor->getArmed());
        const QStringList lines = log.matching(QStringLiteral("skipping grace"));
        QCOMPARE(lines.count(), 1);
        const QRegularExpressionMatch line =
            QRegularExpression(QStringLiteral("^PhantomWakeSuppressor: skipping grace \\(off the charger \\(quick "
                                              "undock check\\) (\\d+) ms ago, lookback 2000 ms\\)$"))
                .match(lines.first());
        QVERIFY2(line.hasMatch(), qPrintable(lines.first()));
        const int ago = line.captured(1).toInt();
        QVERIFY2(ago >= 60 && ago < 2000, qPrintable(lines.first()));
    }

    void test_inputLookback_skipLineNamesTheKind_data() {
        QTest::addColumn<int>("source");
        QTest::addColumn<QString>("kind");
        QTest::newRow("a key or a touch") << 0 << QStringLiteral("user input");
        QTest::newRow("an entity command") << 1 << QStringLiteral("entity command");
        QTest::newRow("the core's charger report") << 2 << QStringLiteral("charger change");
    }

    void test_inputLookback_skipLineNamesTheKind() {
        QFETCH(int, source);
        QFETCH(QString, kind);
        HwLog log;
        m_suppressor->setInputLookbackMs(2000);  // the longest: see test_inputLookback_quickUndockCounts
        switch (source) {
            case 0:
                m_suppressor->onUserInput();
                break;
            case 1:
                m_suppressor->onEntityCommandIssued("light.desk", "light.toggle");
                break;
            default:
                m_suppressor->onPowerSupplyChanged(true);
                break;
        }
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(!m_suppressor->getArmed());
        const QStringList lines = log.matching(QStringLiteral("skipping grace"));
        QCOMPARE(lines.count(), 1);
        QVERIFY2(lines.first().startsWith(QStringLiteral("PhantomWakeSuppressor: skipping grace (%1 ").arg(kind)),
                 qPrintable(lines.first()));
        QVERIFY2(lines.first().endsWith(QStringLiteral(" ms ago, lookback 2000 ms)")), qPrintable(lines.first()));
    }

    void test_inputLookback_entityCommandUpdatesLastInput() {
        // An entity command also restarts m_lastInputTimer (firmware delivers the wake
        // press before powerModeChanged, so it has to count as recent input).
        m_suppressor->setInputLookbackMs(500);
        m_suppressor->onEntityCommandIssued("media_player.tv", "media_player.play_pause");
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(!m_suppressor->getArmed());
    }

    void test_inputLookback_irButtonUpdatesLastInput() {
        // IR buttons arrive as "remote.send" with the IR code in the params.
        m_suppressor->setInputLookbackMs(500);
        m_suppressor->onEntityCommandIssued("remote.tv_ir", "remote.send");
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(!m_suppressor->getArmed());
    }

    void test_onUserInput_outsideGrace_updatesLastInput() {
        // v1.4.23: m_lastInputTimer is updated even when grace is not active.
        // This is critical: future wake events must see the input as "recent".
        m_suppressor->setInputLookbackMs(500);
        m_suppressor->onUserInput();
        // No grace was armed — but the timer was updated.
        QVERIFY(!m_suppressor->getArmed());

        // Subsequent wake should see input as recent and skip.
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(!m_suppressor->getArmed());
    }

    // ──────────────────────────────────────────────────────────────────────
    // 5. Grace timer fires forceLowPower at expected time
    // ──────────────────────────────────────────────────────────────────────
    void test_graceTimer_firesForceAtExpectedTime() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->setGraceMs(150);  // 150 ms

        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        // Before grace expires — no force yet.
        QTest::qWait(50);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);

        // After grace expires — exactly 1 force call.
        QTest::qWait(150);  // total wait now ~200 ms, past 150 ms grace
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
        QCOMPARE(uc::test::MockCoreRecorder::lastPowerModeValue(), QString("LOW_POWER"));
    }

    // ──────────────────────────────────────────────────────────────────────
    // 6. setGraceMs clamping
    // ──────────────────────────────────────────────────────────────────────
    void test_setGraceMs_clampLow() {
        m_suppressor->setGraceMs(50);  // below 100 minimum
        QCOMPARE(m_suppressor->getGraceMs(), 100);

        m_suppressor->setGraceMs(-100);
        QCOMPARE(m_suppressor->getGraceMs(), 100);

        m_suppressor->setGraceMs(0);
        QCOMPARE(m_suppressor->getGraceMs(), 100);
    }

    void test_setGraceMs_clampHigh() {
        m_suppressor->setGraceMs(10000);  // above 5000 maximum
        QCOMPARE(m_suppressor->getGraceMs(), 5000);

        m_suppressor->setGraceMs(99999);
        QCOMPARE(m_suppressor->getGraceMs(), 5000);
    }

    void test_setGraceMs_inRange() {
        m_suppressor->setGraceMs(100);
        QCOMPARE(m_suppressor->getGraceMs(), 100);

        m_suppressor->setGraceMs(500);
        QCOMPARE(m_suppressor->getGraceMs(), 500);

        m_suppressor->setGraceMs(2500);
        QCOMPARE(m_suppressor->getGraceMs(), 2500);

        m_suppressor->setGraceMs(5000);
        QCOMPARE(m_suppressor->getGraceMs(), 5000);
    }

    void test_setGraceMs_changeDuringActiveGrace_restartsWithNewInterval() {
        // setGraceMs while grace is active should restart the timer with the new value.
        m_suppressor->setGraceMs(2000);
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        // Shrink grace to a small value — should restart timer and fire quickly.
        m_suppressor->setGraceMs(100);
        QVERIFY(m_suppressor->getArmed());

        QTest::qWait(150);
        // Should have fired forceLowPower with the NEW (shorter) grace.
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 7. setInputLookbackMs clamping
    // ──────────────────────────────────────────────────────────────────────
    void test_setInputLookbackMs_clampLow() {
        m_suppressor->setInputLookbackMs(-100);
        QCOMPARE(m_suppressor->getInputLookbackMs(), 0);
    }

    void test_setInputLookbackMs_clampHigh() {
        m_suppressor->setInputLookbackMs(99999);
        QCOMPARE(m_suppressor->getInputLookbackMs(), 2000);
    }

    void test_setInputLookbackMs_inRange() {
        m_suppressor->setInputLookbackMs(0);
        QCOMPARE(m_suppressor->getInputLookbackMs(), 0);

        m_suppressor->setInputLookbackMs(500);
        QCOMPARE(m_suppressor->getInputLookbackMs(), 500);

        m_suppressor->setInputLookbackMs(2000);
        QCOMPARE(m_suppressor->getInputLookbackMs(), 2000);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 8. setEnabled mid-grace cancels active timer
    // ──────────────────────────────────────────────────────────────────────
    void test_setEnabled_offMidGrace_cancels() {
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        m_suppressor->setEnabled(false);
        QVERIFY(!m_suppressor->getArmed());

        QTest::qWait(kFastGraceMs + 50);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 9. armedChanged signal fires correctly
    // ──────────────────────────────────────────────────────────────────────
    void test_armedChanged_signal_lifecycle() {
        m_suppressor->setInputLookbackMs(0);
        QSignalSpy armedSpy(m_suppressor, &uc::hw::PhantomWakeSuppressor::armedChanged);

        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        // Arming -> at least one signal.
        QVERIFY(armedSpy.count() >= 1);

        m_suppressor->onUserInput();
        // Cancellation -> another signal.
        QVERIFY(armedSpy.count() >= 2);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 10. Subsequent wake events during grace restart the timer
    // ──────────────────────────────────────────────────────────────────────
    void test_subsequentWake_duringGrace_restartsTimer() {
        // Comment in phantomWakeSuppressor.cpp:21-23 says "Restarting (m_graceTimer.start())
        // resets the countdown — used when a second wake event lands during an already-armed
        // grace window."
        //
        // Test design: use a long enough grace window so timing slack on slow CI doesn't
        // cause flake. Grace=400ms, restart at 100ms. Without restart, would fire at 400ms.
        // With restart, fires at 500ms. Check at 450ms (must NOT have fired) and 600ms
        // (must have fired exactly once).
        m_suppressor->setInputLookbackMs(0);
        m_suppressor->setGraceMs(400);

        m_suppressor->onPowerModeChanged(PowerMode::Low_power, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        QTest::qWait(100);  // 100ms elapsed, 300ms remaining on original

        // Second wake — should restart the timer to a fresh 400ms window.
        m_suppressor->onPowerModeChanged(PowerMode::Suspend, PowerMode::Normal);
        QVERIFY(m_suppressor->getArmed());

        // Wait until we're past the ORIGINAL timer's would-be expiry (450ms total)
        // but well before the RESTARTED timer's expiry (500ms from restart).
        QTest::qWait(350);  // total elapsed: 450ms; since restart: 350ms
        // If restart worked, no force yet (350 < 400 since restart).
        // If restart didn't work, original timer would have fired at 400ms.
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);

        // Wait past the restarted timer's expiry.
        QTest::qWait(150);  // total elapsed: 600ms; since restart: 500ms
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
    }
};

QTEST_MAIN(PhantomWakeSuppressorTest)
#include "test_phantomWakeSuppressor.moc"
