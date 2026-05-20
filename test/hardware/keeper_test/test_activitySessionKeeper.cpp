// Copyright (c) 2026 madalone. Unit tests for ActivitySessionKeeper (Mod 5).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// COVERAGE GOALS
// ──────────────
// 1. State machine: enabled / on-AC / media / idle-timer matrix from evaluateSession()
// 2. Curated-allowlist check on entityCommandIssued
// 3. Defensive cleanup paths (core disconnect, AC unplug, setEnabled flip)
// 4. REST inhibitor lifecycle:
//    - Activate edge sends POST exactly once (no delay → BLOCK mode)
//    - Deactivate edge sends DELETE with stored ID
//    - Orphan cleanup on reconnect deletes pre-crash inhibitors but keeps current one
//    - Hard-fail on POST failure (no silent fallback path exists in v1.5.1+)
//
// MOCK STRATEGY
// ─────────────
// Stub-impl approach: real `core.h` declarations + minimal stub `core::Api` impl
// (mock_core_api.{h,cpp}) recording REST inhibitor calls (createInhibitor /
// deleteInhibitor / listInhibitors) into MockCoreRecorder for inspection.

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>

#include "../../../src/core/core.h"
#include "../../../src/hardware/activitySessionKeeper.h"
#include "../../../src/ui/entity/mediaPlayer.h"
#include "../mock_core_api.h"

class ActivitySessionKeeperTest : public QObject {
    Q_OBJECT

 private:
    uc::core::Api*                       m_api      = nullptr;
    uc::hw::ActivitySessionKeeper*       m_keeper   = nullptr;

    static constexpr int kPlaying = uc::ui::entity::MediaPlayerStates::Playing;
    static constexpr int kPaused  = uc::ui::entity::MediaPlayerStates::Paused;
    static constexpr int kOff     = uc::ui::entity::MediaPlayerStates::Off;

 private slots:
    void init() {
        uc::test::MockCoreRecorder::reset();
        m_api    = new uc::core::Api(QString("ws://test"), this);
        m_keeper = new uc::hw::ActivitySessionKeeper(m_api, this);
    }

    void cleanup() {
        delete m_keeper;
        m_keeper = nullptr;
        delete m_api;
        m_api = nullptr;
    }

    // ──────────────────────────────────────────────────────────────────────
    // 1. evaluateSession truth table — state machine matrix.
    // ──────────────────────────────────────────────────────────────────────
    void test_evaluateSession_disabled_never_active() {
        m_keeper->setEnabled(false);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(!m_keeper->getActive());
    }

    void test_evaluateSession_battery_only_when_AC_required() {
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(false);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(!m_keeper->getActive());
    }

    void test_evaluateSession_AC_no_media_no_idle() {
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);
        QVERIFY(!m_keeper->getActive());
    }

    void test_evaluateSession_media_playing_activates() {
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);

        QSignalSpy activeSpy(m_keeper, &uc::hw::ActivitySessionKeeper::activeChanged);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);

        QVERIFY(m_keeper->getActive());
        QCOMPARE(activeSpy.count(), 1);
    }

    void test_evaluateSession_idle_timer_only_activates() {
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onEntityCommandIssued("media_player.tv", "PLAY_PAUSE");
        QVERIFY(m_keeper->getActive());
    }

    void test_evaluateSession_media_stops_then_idle_expires_deactivates() {
        m_keeper->setIdleTimeoutSec(1);
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        m_keeper->onEntityCommandIssued("media_player.tv", "PLAY_PAUSE");

        QVERIFY(m_keeper->getActive());

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPaused);
        QVERIFY(m_keeper->getActive());

        QTest::qWait(1200);
        QVERIFY(!m_keeper->getActive());
    }

    // ──────────────────────────────────────────────────────────────────────
    // 2. onMediaPlayerStateChanged active-set bookkeeping
    // ──────────────────────────────────────────────────────────────────────
    void test_mediaPlayer_set_addAndRemove() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(m_keeper->getActive());

        m_keeper->onMediaPlayerStateChanged("media_player.kodi", kPlaying);
        QVERIFY(m_keeper->getActive());

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPaused);
        QVERIFY(m_keeper->getActive());

        m_keeper->onMediaPlayerStateChanged("media_player.kodi", kOff);
        QVERIFY(!m_keeper->getActive());
    }

    void test_mediaPlayer_set_dedupePlaying() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(m_keeper->getActive());

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPaused);
        QVERIFY(!m_keeper->getActive());
    }

    // ──────────────────────────────────────────────────────────────────────
    // 3. onEntityCommandIssued — curated allowlist coverage
    // ──────────────────────────────────────────────────────────────────────
    void test_entityCommand_allowlist_data() {
        QTest::addColumn<QString>("command");
        QTest::addColumn<bool>("shouldArm");

        QTest::newRow("PLAY_PAUSE")     << QString("PLAY_PAUSE")    << true;
        QTest::newRow("play_pause")     << QString("play_pause")    << true;  // case-insensitive
        QTest::newRow("VOLUME_UP")      << QString("VOLUME_UP")     << true;
        QTest::newRow("VOLUME_DOWN")    << QString("VOLUME_DOWN")   << true;
        QTest::newRow("MUTE_TOGGLE")    << QString("MUTE_TOGGLE")   << true;
        QTest::newRow("CURSOR_UP")      << QString("CURSOR_UP")     << true;
        QTest::newRow("CURSOR_LEFT")    << QString("CURSOR_LEFT")   << true;
        QTest::newRow("CURSOR_ENTER")   << QString("CURSOR_ENTER")  << true;
        QTest::newRow("CHANNEL_UP")     << QString("CHANNEL_UP")    << true;
        QTest::newRow("NEXT")           << QString("NEXT")          << true;
        QTest::newRow("PREVIOUS")       << QString("PREVIOUS")      << true;
        QTest::newRow("FAST_FORWARD")   << QString("FAST_FORWARD")  << true;
        QTest::newRow("REWIND")         << QString("REWIND")        << true;
        QTest::newRow("STOP")           << QString("STOP")          << true;
        QTest::newRow("PAUSE")          << QString("PAUSE")         << true;
        QTest::newRow("PLAY")           << QString("PLAY")          << true;
        QTest::newRow("SEEK")           << QString("SEEK")          << true;

        QTest::newRow("BROWSE")           << QString("BROWSE")           << false;
        QTest::newRow("STATE_QUERY")      << QString("STATE_QUERY")      << false;
        QTest::newRow("GET_CAPABILITIES") << QString("GET_CAPABILITIES") << false;
        QTest::newRow("REFRESH")          << QString("REFRESH")          << false;
        QTest::newRow("empty")            << QString("")                 << false;
        QTest::newRow("ON")               << QString("ON")               << false;  // power-on is not a session keeper
        QTest::newRow("OFF")              << QString("OFF")              << false;
    }

    void test_entityCommand_allowlist() {
        QFETCH(QString, command);
        QFETCH(bool, shouldArm);

        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);

        QVERIFY(!m_keeper->getActive());

        m_keeper->onEntityCommandIssued("media_player.x", command);

        QCOMPARE(m_keeper->getActive(), shouldArm);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 4. onPowerSupplyChanged toggles correctly
    // ──────────────────────────────────────────────────────────────────────
    void test_powerSupply_unplug_with_AC_required_deactivates() {
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(m_keeper->getActive());

        m_keeper->onPowerSupplyChanged(false);
        QVERIFY(!m_keeper->getActive());
    }

    void test_powerSupply_unplug_without_AC_required_stays_active() {
        m_keeper->setRequireAcPower(false);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(false);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(m_keeper->getActive());
    }

    // ──────────────────────────────────────────────────────────────────────
    // 5. onCoreDisconnected defensive cleanup
    // ──────────────────────────────────────────────────────────────────────
    void test_coreDisconnected_clears_all_state() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        m_keeper->onEntityCommandIssued("media_player.tv", "PLAY_PAUSE");
        QVERIFY(m_keeper->getActive());

        m_keeper->onCoreDisconnected();
        QVERIFY(!m_keeper->getActive());

        // A NEW media event re-populates the set and re-activates.
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(m_keeper->getActive());
    }

    // ──────────────────────────────────────────────────────────────────────
    // 6. setEnabled toggles correctly mid-session
    // ──────────────────────────────────────────────────────────────────────
    void test_setEnabled_offMidSession_deactivates() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(m_keeper->getActive());

        m_keeper->setEnabled(false);
        QVERIFY(!m_keeper->getActive());
    }

    // ──────────────────────────────────────────────────────────────────────
    // 7. setIdleTimeoutSec mid-session doesn't double-fire the inhibitor POST
    // ──────────────────────────────────────────────────────────────────────
    void test_idleTimeout_changeDuringActive_doesntDoubleCreate() {
        uc::test::MockCoreRecorder::pushNextCreateResponse(201, "tmout-uuid");
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QCoreApplication::processEvents();
        QCOMPARE(uc::test::MockCoreRecorder::createInhibitorCallCount(), 1);

        m_keeper->setIdleTimeoutSec(120);
        QCoreApplication::processEvents();
        QCOMPARE(uc::test::MockCoreRecorder::createInhibitorCallCount(), 1);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 8. REST inhibitor lifecycle — inactive→active edge POSTs exactly once.
    //    who="madalone.session-keeper", no delay (BLOCK mode), why populated.
    // ──────────────────────────────────────────────────────────────────────
    void test_inhibitorApi_creates_on_active() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);

        uc::test::MockCoreRecorder::pushNextCreateResponse(201, "test-uuid-abc");

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QCoreApplication::processEvents();

        QCOMPARE(uc::test::MockCoreRecorder::createInhibitorCallCount(), 1);
        QCOMPARE(uc::test::MockCoreRecorder::lastInhibitorWho(), QString("madalone.session-keeper"));
        QCOMPARE(uc::test::MockCoreRecorder::lastInhibitorDelay(), -1);  // BLOCK mode (delay omitted)
        QVERIFY2(!uc::test::MockCoreRecorder::lastInhibitorWhy().isEmpty(),
                 "why field should be populated for human-readable firmware logs");
    }

    // ──────────────────────────────────────────────────────────────────────
    // 9. REST inhibitor lifecycle — active→inactive edge DELETEs the stored ID.
    // ──────────────────────────────────────────────────────────────────────
    void test_inhibitorApi_deletes_on_inactive() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);
        uc::test::MockCoreRecorder::pushNextCreateResponse(201, "test-uuid-1");

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QCoreApplication::processEvents();
        QCOMPARE(uc::test::MockCoreRecorder::createInhibitorCallCount(), 1);

        uc::test::MockCoreRecorder::pushNextDeleteResponse(200);

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kOff);
        QCoreApplication::processEvents();

        QCOMPARE(uc::test::MockCoreRecorder::deleteInhibitorCallCount(), 1);
        QCOMPARE(uc::test::MockCoreRecorder::lastDeletedInhibitorId(), QString("test-uuid-1"));
    }

    // ──────────────────────────────────────────────────────────────────────
    // 10. Orphan cleanup on reconnect — UI crashes mid-session, restarts,
    //     entity events refire BEFORE onCoreConnected (signal-slot ordering),
    //     keeper creates a fresh inhibitor — firmware now has BOTH the new one
    //     AND the pre-crash orphan. Cleanup must skip the currently-tracked one.
    // ──────────────────────────────────────────────────────────────────────
    void test_inhibitorApi_orphanCleanup_on_reconnect() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);

        // Pre-disconnect: activate once.
        uc::test::MockCoreRecorder::pushNextCreateResponse(201, "first-uuid");
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QCoreApplication::processEvents();
        QCOMPARE(uc::test::MockCoreRecorder::createInhibitorCallCount(), 1);

        // Disconnect clears local inhibitorId; firmware-side "first-uuid" persists as orphan.
        m_keeper->onCoreDisconnected();
        // Entity events refire post-reconnect; keeper re-activates and POSTs fresh.
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);
        uc::test::MockCoreRecorder::pushNextCreateResponse(201, "current-uuid");
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QCoreApplication::processEvents();

        const int createCountBeforeCleanup = uc::test::MockCoreRecorder::createInhibitorCallCount();
        QCOMPARE(createCountBeforeCleanup, 2);

        // Inject the list response — both inhibitors present.
        uc::core::Inhibitor orphan;
        orphan.id   = "first-uuid";
        orphan.who  = "madalone.session-keeper";
        orphan.mode = "BLOCK";
        uc::core::Inhibitor current;
        current.id   = "current-uuid";
        current.who  = "madalone.session-keeper";
        current.mode = "BLOCK";
        uc::test::MockCoreRecorder::pushNextListResponse(200, { orphan, current });

        m_keeper->onCoreConnected();
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();  // list resp → delete loop

        QCOMPARE(uc::test::MockCoreRecorder::listInhibitorsCallCount(), 1);
        // Exactly 1 delete: the orphan. The currently-tracked "current-uuid" must NOT be deleted.
        QCOMPARE(uc::test::MockCoreRecorder::deleteInhibitorCallCount(), 1);
        QCOMPARE(uc::test::MockCoreRecorder::lastDeletedInhibitorId(), QString("first-uuid"));
        // No new create fired — we already have a tracked inhibitor; activateInhibitor() early-returns.
        QCOMPARE(uc::test::MockCoreRecorder::createInhibitorCallCount(), createCountBeforeCleanup);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 11. Hard-fail on REST POST 401 — no fallback path exists in v1.5.1+.
    //     Keeper logs loudly; m_active stays true (state machine doesn't depend
    //     on effector success). Auto-revert handles the worst-case scenario.
    // ──────────────────────────────────────────────────────────────────────
    void test_inhibitorApi_create_fails_hardFail() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);

        uc::test::MockCoreRecorder::pushNextCreateResponse(401, QString(), "Unauthorized");

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QCoreApplication::processEvents();

        QCOMPARE(uc::test::MockCoreRecorder::createInhibitorCallCount(), 1);
        // No DELETE follow-up (nothing to delete; m_inhibitorId stays empty).
        QCOMPARE(uc::test::MockCoreRecorder::deleteInhibitorCallCount(), 0);
        // State machine stays active — effector failure doesn't roll back evaluateSession().
        QVERIFY(m_keeper->getActive());
    }
};

QTEST_MAIN(ActivitySessionKeeperTest)
#include "test_activitySessionKeeper.moc"
