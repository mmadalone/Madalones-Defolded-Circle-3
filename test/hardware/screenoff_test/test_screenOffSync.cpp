// Copyright (c) 2026 madalone. Unit tests for ScreenOffSync (screen-off animation in step with the display switch-off).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 1. The dim-phase rule against the schedule measured in the core simulator (2026-11-02, T = 5..60 s).
// 2. startDelayMs: from the Idle entry and from the middle of the dim phase, the floor at 0, -1 outside Idle.
// 3. requestDisplayOff: LOW_POWER with the "mode" body field, only in Idle, only after markEffectStart() and with no
//    input since, once per dim phase, and not when the request cannot be sent.
// 4. fitLeadMs: the "Animation length" shortened so the animation still ends END_MARGIN_MS before the earliest
//    switch-off (also for a screensaver opened during the dim phase), unchanged outside Idle (2026-11-03).

#include <QtTest>
#include <QCoreApplication>

#include "../../../src/core/core.h"
#include "../../../src/hardware/screenOffSync.h"
#include "../../../src/system/power.h"
#include "../mock_core_api.h"

class ScreenOffSyncTest : public QObject {
    Q_OBJECT

 private:
    uc::core::Api*         m_api  = nullptr;
    uc::hw::ScreenOffSync* m_sync = nullptr;

    using PowerMode = uc::hw::Power::PowerMode;

    void enterIdle() { m_sync->onPowerModeChanged(PowerMode::Normal, PowerMode::Idle); }

 private slots:
    void init() {
        uc::test::MockCoreRecorder::reset();
        m_api  = new uc::core::Api(QString("ws://test"), this);
        m_sync = new uc::hw::ScreenOffSync(m_api, this);
        m_sync->setDisplayOffSec(30);
    }

    void cleanup() {
        delete m_sync;
        m_sync = nullptr;
        delete m_api;
        m_api = nullptr;
        uc::test::MockCoreRecorder::setConnected(true);
    }

    // 1. Shortest dim phase = T - max(T/2, 3) s: the measured minimums 2/5/8/15/23/30 s for T = 5/10/15/30/45/60
    void test_dimPhaseRule_data() {
        QTest::addColumn<int>("seconds");
        QTest::addColumn<int>("expectedMs");
        QTest::newRow("never off") << 0 << 0;
        QTest::newRow("3 s") << 3 << 0;
        QTest::newRow("4 s") << 4 << 1000;
        QTest::newRow("5 s (web configurator)") << 5 << 2000;
        QTest::newRow("10 s (slider minimum)") << 10 << 5000;
        QTest::newRow("15 s") << 15 << 8000;
        QTest::newRow("30 s") << 30 << 15000;
        QTest::newRow("45 s") << 45 << 23000;
        QTest::newRow("60 s (slider maximum)") << 60 << 30000;
    }
    void test_dimPhaseRule() {
        QFETCH(int, seconds);
        QFETCH(int, expectedMs);
        QCOMPARE(uc::hw::ScreenOffSync::dimPhaseMinMsFor(seconds), expectedMs);
        m_sync->setDisplayOffSec(seconds);
        QCOMPARE(m_sync->dimPhaseMinMs(), expectedMs);
    }

    // 2. Delays
    void test_noDelayOutsideIdle() {
        QCOMPARE(m_sync->msSinceIdle(), -1);
        QCOMPARE(m_sync->startDelayMs(800), -1);
        m_sync->onPowerModeChanged(PowerMode::Normal, PowerMode::Low_power);
        QCOMPARE(m_sync->startDelayMs(800), -1);
    }

    void test_delayFromIdleEntry() {
        enterIdle();
        // 15000 - 250 - 800 (shared overlay), 15000 - 250 - 1300 (TV Static, Analog), a few ms elapsed at most
        const int shared = m_sync->startDelayMs(800), native = m_sync->startDelayMs(1300);
        QVERIFY2(shared <= 13950 && shared >= 13900 && native <= 13450 && native >= 13400,
                 qPrintable(QString("delays %1 / %2").arg(shared).arg(native)));
    }

    void test_delayMidIdle() {
        // A screensaver that opens during the dim phase gets only the remaining time
        enterIdle();
        QTest::qWait(300);
        const int since = m_sync->msSinceIdle(), delay = m_sync->startDelayMs(800);
        QVERIFY2(since >= 290 && delay <= 13950 - 290 && delay >= 13950 - since - 30,
                 qPrintable(QString("%1 ms into Idle: delay %2").arg(since).arg(delay)));
    }

    void test_delayFloorsAtZero() {
        m_sync->setDisplayOffSec(4);  // 1 s dim phase: shorter than margin + lead
        enterIdle();
        QCOMPARE(m_sync->startDelayMs(800), 0);
        QCOMPARE(m_sync->startDelayMs(1300), 0);
    }

    void test_idleClearedOnWake() {
        enterIdle();
        QVERIFY(m_sync->msSinceIdle() >= 0);
        m_sync->onPowerModeChanged(PowerMode::Idle, PowerMode::Normal);
        QCOMPARE(m_sync->msSinceIdle(), -1);
        QCOMPARE(m_sync->startDelayMs(800), -1);
    }

    // 3. The request
    void test_requestSendsLowPower() {
        enterIdle();
        m_sync->markEffectStart();
        QVERIFY(m_sync->requestDisplayOff());
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
        const QVariantMap body = uc::test::MockCoreRecorder::lastSentMsgData();
        QVERIFY2(body.contains("mode") && !body.contains("power_mode"), "the set_power_mode body field is 'mode'");
        QCOMPARE(body.value("mode").toString(), QString("LOW_POWER"));
    }

    void test_requestOncePerDimPhase() {
        enterIdle();
        m_sync->markEffectStart();
        QVERIFY(m_sync->requestDisplayOff());
        QVERIFY(m_sync->requestDisplayOff());  // already sent: true, no second message
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
    }

    void test_noRequestOutsideIdle_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("Normal") << static_cast<int>(PowerMode::Normal);
        QTest::newRow("Low_power") << static_cast<int>(PowerMode::Low_power);
        QTest::newRow("Suspend") << static_cast<int>(PowerMode::Suspend);
    }
    void test_noRequestOutsideIdle() {
        QFETCH(int, mode);
        enterIdle();
        m_sync->markEffectStart();
        m_sync->onPowerModeChanged(PowerMode::Idle, static_cast<PowerMode>(mode));
        m_sync->markEffectStart();
        QVERIFY(!m_sync->requestDisplayOff());
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_noRequestAfterInput() {
        // A button, touch or touchbar press during the animation: the user is there, the display stays on
        enterIdle();
        m_sync->markEffectStart();
        m_sync->onUserInput();
        QVERIFY(!m_sync->requestDisplayOff());
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_inputBeforeTheAnimationDoesNotVeto() {
        enterIdle();
        m_sync->onUserInput();
        m_sync->markEffectStart();
        QVERIFY(m_sync->requestDisplayOff());
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
    }

    void test_noRequestWithoutEffectStart() {
        enterIdle();
        QVERIFY(!m_sync->requestDisplayOff());
        // A new dim phase needs its own markEffectStart()
        m_sync->markEffectStart();
        m_sync->onPowerModeChanged(PowerMode::Idle, PowerMode::Normal);
        enterIdle();
        QVERIFY(!m_sync->requestDisplayOff());
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_noRequestWithoutConnection() {
        delete m_sync;  // one instance at a time (QML singleton)
        m_sync = nullptr;
        uc::test::MockCoreRecorder::setConnected(false);
        uc::core::Api offline(QString("ws://test"));
        uc::hw::ScreenOffSync sync(&offline);
        sync.setDisplayOffSec(30);
        sync.onPowerModeChanged(PowerMode::Normal, PowerMode::Idle);
        sync.markEffectStart();
        QVERIFY(!sync.requestDisplayOff());
    }

    // 4. fitLeadMs
    void test_fitUnchangedOutsideIdle() {
        QCOMPARE(m_sync->fitLeadMs(800), 800);
        QCOMPARE(m_sync->fitLeadMs(4500), 4500);
        m_sync->onPowerModeChanged(PowerMode::Normal, PowerMode::Low_power);
        QCOMPARE(m_sync->fitLeadMs(4500), 4500);
    }

    void test_fitFromIdleEntry_data() {
        QTest::addColumn<int>("seconds");
        QTest::addColumn<int>("lead");
        QTest::addColumn<int>("expected");  // the room from the Idle entry is dimPhaseMin - 250
        QTest::newRow("30 s, 800 fits") << 30 << 800 << 800;
        QTest::newRow("30 s, 4500 fits") << 30 << 4500 << 4500;
        QTest::newRow("30 s, 20000 cut to 14750") << 30 << 20000 << 14750;
        QTest::newRow("10 s (slider minimum), 4500 fits") << 10 << 4500 << 4500;
        QTest::newRow("10 s, 5000 cut to 4750") << 10 << 5000 << 4750;
        QTest::newRow("5 s (web configurator), 4000 cut to 1750") << 5 << 4000 << 1750;
        QTest::newRow("4 s, 800 cut to 750") << 4 << 800 << 750;
        QTest::newRow("3 s: no room") << 3 << 800 << 0;
        QTest::newRow("0 (behaves like 3 s): no room") << 0 << 800 << 0;
    }
    void test_fitFromIdleEntry() {
        QFETCH(int, seconds);
        QFETCH(int, lead);
        QFETCH(int, expected);
        m_sync->setDisplayOffSec(seconds);
        enterIdle();
        const int fitted = m_sync->fitLeadMs(lead);
        // A cut length loses the few ms elapsed since enterIdle(); one that fits is returned as it is
        const bool ok = (expected == lead) ? fitted == lead : (fitted <= expected && fitted >= qMax(0, expected - 30));
        QVERIFY2(ok, qPrintable(QString("T = %1 s, %2 ms -> %3 ms (expected %4)").arg(seconds).arg(lead).arg(fitted)
                                    .arg(expected)));
        // A shortened animation starts at once and ends END_MARGIN_MS before the earliest switch-off
        if (fitted < lead) QCOMPARE(m_sync->startDelayMs(fitted), 0);
    }

    void test_fitMidIdle() {
        // A screensaver opened during the dim phase gets only the time left
        m_sync->setDisplayOffSec(10);  // dim phase at least 5000 ms: room 4750 ms from the Idle entry
        enterIdle();
        QTest::qWait(300);
        const int since = m_sync->msSinceIdle(), fitted = m_sync->fitLeadMs(4700);
        QVERIFY2(since >= 290 && fitted <= 4750 - since && fitted >= 4750 - since - 30,
                 qPrintable(QString("%1 ms into Idle: 4700 -> %2").arg(since).arg(fitted)));
    }

    void test_fitNegativeLead() {
        enterIdle();
        QCOMPARE(m_sync->fitLeadMs(-5), 0);
    }
};

QTEST_MAIN(ScreenOffSyncTest)
#include "test_screenOffSync.moc"
