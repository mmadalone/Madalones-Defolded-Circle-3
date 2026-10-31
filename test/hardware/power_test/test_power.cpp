// Copyright (c) 2026 madalone. Unit tests for Power::windowShown (v1.5.3 port of upstream 42f9888).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The UI window is hidden while the display of a remote is off, so that nothing is rendered. Before v1.5.3 it was
// hidden only on the change from Idle to Low_power, the core's own timeout path. Two other ways into Low_power left
// it visible and rendering with the display off:
// - the power mode API going from Normal straight to Low_power, which is how the PhantomWakeSuppressor (Mod 6)
//   sends an unattended wake-up back to sleep;
// - the UI starting while the display is off: Power starts as Normal and the first answer of the core is Low_power.
// A desktop run never hides its window.
//
// Adapted from upstream test/hardware/test_power.cpp (42f9888) to the stub core::Api of this test tree, with one
// deliberate difference: an unchanged mode is still announced through powerModeChanged here, as it was before
// v1.5.3, while upstream returns early (see Power::setPowerMode).

#include <QSignalSpy>
#include <QtTest>

#include "../../../src/core/core.h"
#include "../../../src/system/power.h"
#include "../mock_core_api.h"

using CoreMode = uc::core::PowerEnums::PowerMode;
using Mode = uc::hw::Power::PowerMode;

Q_DECLARE_METATYPE(CoreMode)

class PowerTest : public QObject {
    Q_OBJECT

 private:
    uc::core::Api* m_api = nullptr;

 private slots:
    void init() {
        uc::test::MockCoreRecorder::reset();
        m_api = new uc::core::Api(QStringLiteral("ws://test"), this);
    }

    void cleanup() {
        // every test destroys its Power on the stack first, which resets Power::s_instance for the next one
        delete m_api;
        m_api = nullptr;
    }

    void displayOff_hidesTheWindowWhateverTheModeBefore_data() {
        QTest::addColumn<CoreMode>("before");
        QTest::addColumn<CoreMode>("displayOff");

        QTest::newRow("Idle to Low_power (timeout)") << CoreMode::IDLE << CoreMode::LOW_POWER;
        QTest::newRow("Normal to Low_power (power mode API, Mod 6)") << CoreMode::NORMAL << CoreMode::LOW_POWER;
        QTest::newRow("Normal to Suspend") << CoreMode::NORMAL << CoreMode::SUSPEND;
        QTest::newRow("Idle to Suspend") << CoreMode::IDLE << CoreMode::SUSPEND;
    }

    void displayOff_hidesTheWindowWhateverTheModeBefore() {
        QFETCH(CoreMode, before);
        QFETCH(CoreMode, displayOff);

        uc::hw::Power power(m_api, true);
        emit m_api->powerModeChanged(before);
        QVERIFY(power.isWindowShown());

        QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);
        emit m_api->powerModeChanged(displayOff);

        QVERIFY(!power.isWindowShown());
        QCOMPARE(windowShownChanged.count(), 1);
    }

    void displayOn_showsTheWindowAgain_data() {
        QTest::addColumn<CoreMode>("displayOff");
        QTest::addColumn<CoreMode>("displayOn");

        QTest::newRow("Low_power to Normal (wake-up)") << CoreMode::LOW_POWER << CoreMode::NORMAL;
        QTest::newRow("Low_power to Idle (power mode API)") << CoreMode::LOW_POWER << CoreMode::IDLE;
        QTest::newRow("Suspend to Normal (wake-up)") << CoreMode::SUSPEND << CoreMode::NORMAL;
    }

    void displayOn_showsTheWindowAgain() {
        QFETCH(CoreMode, displayOff);
        QFETCH(CoreMode, displayOn);

        uc::hw::Power power(m_api, true);
        emit m_api->powerModeChanged(displayOff);
        QVERIFY(!power.isWindowShown());

        QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);
        emit m_api->powerModeChanged(displayOn);

        QVERIFY(power.isWindowShown());
        QCOMPARE(windowShownChanged.count(), 1);
    }

    void startWhileTheDisplayIsOff_hidesTheWindow() {
        uc::hw::Power power(m_api, true);

        // the UI starts as Normal: the window is shown until the core says otherwise
        QCOMPARE(power.getPowerMode(), Mode::Normal);
        QVERIFY(power.isWindowShown());

        QSignalSpy powerModeChanged(&power, &uc::hw::Power::powerModeChanged);
        QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);

        // the first power mode answer after the start
        emit m_api->powerModeChanged(CoreMode::LOW_POWER);

        QCOMPARE(powerModeChanged.count(), 1);
        QCOMPARE(powerModeChanged.at(0).at(0).value<Mode>(), Mode::Normal);
        QCOMPARE(powerModeChanged.at(0).at(1).value<Mode>(), Mode::Low_power);
        QVERIFY(!power.isWindowShown());
        QCOMPARE(windowShownChanged.count(), 1);
    }

    // Our difference from upstream: an unchanged mode is still announced, but the window does not change.
    void sameMode_isStillAnnounced_windowUnchanged() {
        uc::hw::Power power(m_api, true);
        emit m_api->powerModeChanged(CoreMode::LOW_POWER);

        QSignalSpy powerModeChanged(&power, &uc::hw::Power::powerModeChanged);
        QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);
        emit m_api->powerModeChanged(CoreMode::LOW_POWER);

        QCOMPARE(powerModeChanged.count(), 1);
        QCOMPARE(windowShownChanged.count(), 0);

        // both display-off modes hide the window: Low_power to Suspend changes the mode, not the window
        emit m_api->powerModeChanged(CoreMode::SUSPEND);

        QCOMPARE(powerModeChanged.count(), 2);
        QCOMPARE(windowShownChanged.count(), 0);
        QVERIFY(!power.isWindowShown());
    }

    // Mod 6 after a real wake, as seen on fw 2.11.0: forced Low_power, the core back to Normal ~15 ms later, a few
    // times over. Every round hides and shows the window, and nothing stays hidden once the remote stays awake.
    void wakePingPong_endsShown() {
        uc::hw::Power power(m_api, true);
        QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);

        for (int round = 0; round < 3; ++round) {
            emit m_api->powerModeChanged(CoreMode::LOW_POWER);
            QVERIFY(!power.isWindowShown());
            emit m_api->powerModeChanged(CoreMode::NORMAL);
            QVERIFY(power.isWindowShown());
        }
        QCOMPARE(windowShownChanged.count(), 6);
    }

    void desktop_neverHidesTheWindow() {
        uc::hw::Power power(m_api, false);
        QSignalSpy windowShownChanged(&power, &uc::hw::Power::windowShownChanged);

        for (CoreMode mode : {CoreMode::IDLE, CoreMode::LOW_POWER, CoreMode::SUSPEND, CoreMode::NORMAL}) {
            emit m_api->powerModeChanged(mode);
            QVERIFY(power.isWindowShown());
        }
        QCOMPARE(windowShownChanged.count(), 0);
    }
};

QTEST_GUILESS_MAIN(PowerTest)

#include "test_power.moc"
