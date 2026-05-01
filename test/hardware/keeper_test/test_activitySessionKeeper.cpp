// Copyright (c) 2026 madalone. Unit tests for ActivitySessionKeeper (Mod 5).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// COVERAGE GOALS
// ──────────────
// 1. PRIMARY: regression-prevention for the v1.4.14 → v1.4.21 setPowerMode body-shape bug
//    (firmware expects WS RPC field name "mode", not "power_mode"). See test_ping_body_shape.
// 2. State machine: enabled / on-AC / media / idle-timer matrix from evaluateSession()
// 3. Curated-allowlist check on entityCommandIssued
// 4. Defensive cleanup paths (core disconnect, AC unplug)
// 5. Timer cadence (270 s) and immediate first-ping behavior
//
// MOCK STRATEGY
// ─────────────
// Stub-impl approach: real `core.h` declarations + minimal stub `core::Api` impl
// (mock_core_api.{h,cpp}). Captures setPowerMode call into the MockCoreRecorder
// for inspection. See mock_core_api.h header comment for the regression contract.

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
    // Test fixtures — fresh per test via init/cleanup.
    uc::core::Api*                       m_api      = nullptr;
    uc::hw::ActivitySessionKeeper*       m_keeper   = nullptr;

    // Convenience aliases — match production enum values.
    static constexpr int kPlaying = uc::ui::entity::MediaPlayerStates::Playing;
    static constexpr int kPaused  = uc::ui::entity::MediaPlayerStates::Paused;
    static constexpr int kOff     = uc::ui::entity::MediaPlayerStates::Off;

 private slots:
    void init() {
        // Reset mock recorder + create fresh objects.
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
    // 1. REGRESSION-PREVENTION: ping() sends the correct WS RPC body shape.
    //    This is the test that would have caught the v1.4.14 → v1.4.21 bug
    //    on day one if it had existed: the body field name MUST be "mode",
    //    NOT "power_mode" (the latter is the REST query convention).
    // ──────────────────────────────────────────────────────────────────────
    void test_ping_body_shape() {
        // Force keeper into active state — enabled + on AC + media playing.
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);

        // evaluateSession() should have fired an immediate ping when active flipped.
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);

        // Body shape: msg_data must have "mode" (NOT "power_mode") with value "NORMAL".
        const QVariantMap msgData = uc::test::MockCoreRecorder::lastSentMsgData();
        QVERIFY2(msgData.contains("mode"),
                 "WS RPC body must use field name 'mode' (not 'power_mode'). "
                 "This is the v1.4.22 fix at core.cpp:821 — firmware's serde rejects 'power_mode' with HTTP 400.");
        QVERIFY2(!msgData.contains("power_mode"),
                 "WS RPC body must NOT contain 'power_mode' field. That's the REST query convention "
                 "(PUT /api/system/power?power_mode=NORMAL); WS RPC bodies use 'mode'.");
        QCOMPARE(msgData.value("mode").toString(), QString("NORMAL"));
        QCOMPARE(uc::test::MockCoreRecorder::lastPowerModeValue(), QString("NORMAL"));

        // Full envelope shape: kind=req, msg=set_power_mode, msg_data={"mode":"NORMAL"}
        const QString json = uc::test::MockCoreRecorder::lastSentRequestJson();
        QVERIFY(json.contains("\"kind\":\"req\""));
        QVERIFY(json.contains("\"msg\":\"set_power_mode\""));
        QVERIFY(json.contains("\"mode\":\"NORMAL\""));
        QVERIFY2(!json.contains("\"power_mode\":"),
                 qPrintable(QString("Outgoing JSON must not contain \"power_mode\" field: %1").arg(json)));
    }

    // ──────────────────────────────────────────────────────────────────────
    // 2. evaluateSession truth table — matrix per the keeper's logic at
    //    activitySessionKeeper.cpp:117-133.
    // ──────────────────────────────────────────────────────────────────────
    void test_evaluateSession_disabled_never_active() {
        // enabled=false => never active no matter what
        m_keeper->setEnabled(false);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);

        QVERIFY(!m_keeper->getActive());
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_evaluateSession_battery_only_when_AC_required() {
        // enabled=true, requireAcPower=true, onAc=false => never active
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(false);  // off AC
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);

        QVERIFY(!m_keeper->getActive());
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_evaluateSession_AC_no_media_no_idle() {
        // enabled=true, on AC, no media playing, no idle timer => not active
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);
        // No media events, no entity commands

        QVERIFY(!m_keeper->getActive());
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
    }

    void test_evaluateSession_media_playing_activates() {
        // enabled=true, on AC, media playing => active + ping fires
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);

        QSignalSpy activeSpy(m_keeper, &uc::hw::ActivitySessionKeeper::activeChanged);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);

        QVERIFY(m_keeper->getActive());
        QCOMPARE(activeSpy.count(), 1);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
        QCOMPARE(uc::test::MockCoreRecorder::lastPowerModeValue(), QString("NORMAL"));
    }

    void test_evaluateSession_idle_timer_only_activates() {
        // enabled=true, on AC, no media, but idle timer running => active
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);

        // Curated command starts the idle timer.
        m_keeper->onEntityCommandIssued("media_player.tv", "PLAY_PAUSE");

        QVERIFY(m_keeper->getActive());
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
    }

    void test_evaluateSession_media_stops_then_idle_expires_deactivates() {
        // Set short idle timer so the test doesn't take forever.
        m_keeper->setIdleTimeoutSec(1);  // 1-second idle window
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        m_keeper->onEntityCommandIssued("media_player.tv", "PLAY_PAUSE");  // arms idle timer

        QVERIFY(m_keeper->getActive());
        const int initialPings = uc::test::MockCoreRecorder::setPowerModeCallCount();
        QVERIFY(initialPings >= 1);

        // Media stops — but idle timer is still running, so still active.
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPaused);
        QVERIFY(m_keeper->getActive());

        // Wait for idle timer to expire (1 sec + slack).
        QTest::qWait(1200);

        // No media + idle expired => no longer active.
        QVERIFY(!m_keeper->getActive());
    }

    // ──────────────────────────────────────────────────────────────────────
    // 3. onMediaPlayerStateChanged active-set bookkeeping
    // ──────────────────────────────────────────────────────────────────────
    void test_mediaPlayer_set_addAndRemove() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);

        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(m_keeper->getActive());

        // Add second playing entity — still active.
        m_keeper->onMediaPlayerStateChanged("media_player.kodi", kPlaying);
        QVERIFY(m_keeper->getActive());

        // First stops — second still playing, still active.
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPaused);
        QVERIFY(m_keeper->getActive());

        // Both stop — no longer active (no idle timer either).
        m_keeper->onMediaPlayerStateChanged("media_player.kodi", kOff);
        QVERIFY(!m_keeper->getActive());
    }

    void test_mediaPlayer_set_dedupePlaying() {
        // Two Playing events for the same entity should not double-count.
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
    // 4. onEntityCommandIssued — curated allowlist coverage
    // ──────────────────────────────────────────────────────────────────────
    void test_entityCommand_allowlist_data() {
        QTest::addColumn<QString>("command");
        QTest::addColumn<bool>("shouldArm");

        // Allowlisted (case-insensitive — the keeper toUpper()s before lookup)
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

        // NOT allowlisted (poll noise / capability fetches)
        QTest::newRow("BROWSE")         << QString("BROWSE")        << false;
        QTest::newRow("STATE_QUERY")    << QString("STATE_QUERY")   << false;
        QTest::newRow("GET_CAPABILITIES") << QString("GET_CAPABILITIES") << false;
        QTest::newRow("REFRESH")        << QString("REFRESH")       << false;
        QTest::newRow("empty")          << QString("")              << false;
        QTest::newRow("ON")             << QString("ON")            << false;  // power-on is not a session keeper
        QTest::newRow("OFF")            << QString("OFF")           << false;
    }

    void test_entityCommand_allowlist() {
        QFETCH(QString, command);
        QFETCH(bool, shouldArm);

        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);

        // Without the command, not active.
        QVERIFY(!m_keeper->getActive());

        m_keeper->onEntityCommandIssued("media_player.x", command);

        QCOMPARE(m_keeper->getActive(), shouldArm);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 5. onPowerSupplyChanged toggles correctly
    // ──────────────────────────────────────────────────────────────────────
    void test_powerSupply_unplug_with_AC_required_deactivates() {
        m_keeper->setRequireAcPower(true);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(m_keeper->getActive());

        // Unplug AC -> should deactivate immediately.
        m_keeper->onPowerSupplyChanged(false);
        QVERIFY(!m_keeper->getActive());
    }

    void test_powerSupply_unplug_without_AC_required_stays_active() {
        // requireAcPower=false means battery-only is fine.
        m_keeper->setRequireAcPower(false);
        m_keeper->setEnabled(true);
        m_keeper->onPowerSupplyChanged(false);  // off AC from start
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);

        QVERIFY(m_keeper->getActive());
    }

    // ──────────────────────────────────────────────────────────────────────
    // 6. onCoreDisconnected defensive cleanup
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

        // After disconnect, even "Playing" events shouldn't reactivate
        // immediately because the active media set was cleared. (But a NEW
        // media event will repopulate the set.)
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QVERIFY(m_keeper->getActive());  // re-enters active state from new event
    }

    // ──────────────────────────────────────────────────────────────────────
    // 7. Ping cadence — 270 s, NOT 60, NOT 300.
    //    Verify by direct property access (the keeper exposes m_pingTimer
    //    indirectly via getActive(), but the interval is private — we test
    //    by behavioral inference: triggering active should produce ONE ping
    //    immediately, and we don't have time in unit tests to wait 270 s).
    //    The first-ping-immediate behavior is what we assert here.
    // ──────────────────────────────────────────────────────────────────────
    void test_pingCadence_firstPingImmediate() {
        // First-ping-immediate: when active flips true, ping() is called RIGHT AWAY,
        // not 270 s later. This is the implementation at activitySessionKeeper.cpp:128-129.
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);

        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 0);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);

        // Should be EXACTLY 1 ping immediately, not 0 (would mean delayed) and
        // not >1 (would mean repeated).
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 8. Ping does not double-fire when media re-Plays during active session
    // ──────────────────────────────────────────────────────────────────────
    void test_pingCadence_noDoubleFireDuringActive() {
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);

        // While already active, second media event should NOT re-fire ping
        // (ping timer is already running; we don't reset on each media change).
        m_keeper->onMediaPlayerStateChanged("media_player.kodi", kPlaying);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);

        // Curated commands during active session should also not re-fire ping
        // (the idle timer arms but doesn't trigger a fresh ping).
        m_keeper->onEntityCommandIssued("media_player.tv", "VOLUME_UP");
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
    }

    // ──────────────────────────────────────────────────────────────────────
    // 9. setEnabled toggles correctly mid-session
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
    // 10. setIdleTimeoutSec restarts the timer if active mid-session
    // ──────────────────────────────────────────────────────────────────────
    void test_idleTimeout_changeDuringActive_doesntDoubleFire() {
        // Setting idle timeout shouldn't itself trigger a ping.
        m_keeper->setEnabled(true);
        m_keeper->setRequireAcPower(true);
        m_keeper->onPowerSupplyChanged(true);
        m_keeper->onMediaPlayerStateChanged("media_player.tv", kPlaying);
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);

        m_keeper->setIdleTimeoutSec(120);
        // Should not have triggered a fresh ping.
        QCOMPARE(uc::test::MockCoreRecorder::setPowerModeCallCount(), 1);
    }
};

QTEST_MAIN(ActivitySessionKeeperTest)
#include "test_activitySessionKeeper.moc"
