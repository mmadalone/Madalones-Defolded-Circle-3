// Copyright (c) 2026 madalone. Unit tests for EntityController::onPowerModeChanged (resume-window state machine).
// SPDX-License-Identifier: GPL-3.0-or-later
//
// COVERAGE GOALS
// ──────────────
// Closes the gap flagged by the v1.5.2 (upstream 0.73.5) merge: onPowerModeChanged — the
// wake/standby state machine that drives the Wake-replay HUD's `resumeWindow` — had NO direct
// test. The HUD QML test writes `resumeWindow` on a hand-written mock, bypassing the real logic.
// Upstream reworks this exact function on most UI merges (it conflicted on BOTH the 0.73.4 and
// 0.73.5 merges), so this guards our v1.4.19 LOW_POWER broadening + upstream's debounce against
// a botched future merge.
//
// 1. SUSPEND   → NORMAL arms the resume window (upstream's base case)
// 2. LOW_POWER → NORMAL arms it — our v1.4.19 broadening. UCR3 sleeps via LOW_POWER and never
//    enters SUSPEND; upstream's SUSPEND-only form would NOT arm here, silently disabling
//    wake-replay on real hardware. THIS is the regression guard that matters most.
// 3. IDLE      → NORMAL does NOT arm (IDLE is dimmed-but-awake, not standby)
// 4. NORMAL with no prior sleep does NOT arm
// 5. Debounce: a 2nd wake while a window is already open does NOT re-arm (upstream's
//    `m_wasSuspended && !m_resumeWindow` guard)
// 6. resumeTimeoutWindowSec == 0 → onPowerModeChanged is a no-op (the early-out gate)
//
// MOCK STRATEGY — same stub-impl core::Api as keeper/suppressor (mock_core_api.{h,cpp}).
// onPowerModeChanged touches only local state (m_wasSuspended / m_resumeWindow) + a QTimer;
// it calls NO core::Api methods, so the stub suffices for this path.

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>

#include "../../../src/core/core.h"
#include "../../../src/core/enums.h"
#include "../../../src/config/config.h"
#include "../../../src/ui/entity/entityController.h"
#include "../mock_core_api.h"

class EntityControllerTest : public QObject {
    Q_OBJECT

 private:
    uc::core::Api*            m_api        = nullptr;
    uc::ui::EntityController* m_controller = nullptr;

    using PM = uc::core::PowerEnums::PowerMode;

    // Resume window in SECONDS; the ctor multiplies by 1000 → m_resumeTimerTimeout ms.
    // 2 s keeps the singleShot timer from firing during the synchronous assertions
    // (no event loop is spun in the test bodies, so it never fires).
    static constexpr int kResumeWindowSec = 2;

    void makeController(int resumeWindowSec) {
        m_controller = new uc::ui::EntityController(
            m_api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, resumeWindowSec, this);
    }

 private slots:
    void init() {
        uc::test::MockCoreRecorder::reset();
        m_api = new uc::core::Api(QStringLiteral("ws://test"), this);
        makeController(kResumeWindowSec);
    }

    void cleanup() {
        // The ctor asserts s_instance == nullptr; the dtor resets it. Delete before the
        // next test constructs, or the next ctor's Q_ASSERT fires.
        delete m_controller;
        m_controller = nullptr;
        delete m_api;
        m_api = nullptr;
    }

    // 1. SUSPEND → NORMAL arms.
    void test_suspendThenNormal_arms() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::SUSPEND);
        QVERIFY(!m_controller->getResumeWindow());   // asleep: flag set, window not yet armed
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 1);
    }

    // 2. LOW_POWER → NORMAL arms (v1.4.19 broadening — upstream's SUSPEND-only form would FAIL this).
    void test_lowPowerThenNormal_arms() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::LOW_POWER);
        QVERIFY(!m_controller->getResumeWindow());
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 1);
    }

    // 3. IDLE → NORMAL does NOT arm (IDLE is not standby).
    void test_idleThenNormal_doesNotArm() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::IDLE);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(!m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 0);
    }

    // 4. NORMAL with no prior sleep does NOT arm.
    void test_normalWithoutSleep_doesNotArm() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(!m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 0);
    }

    // 5. Debounce: a 2nd asleep→NORMAL while still armed does NOT re-arm or re-emit.
    void test_secondWakeWhileArmed_doesNotReArm() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::SUSPEND);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 1);

        // Another sleep→wake while the window is still open.
        m_controller->onPowerModeChanged(PM::LOW_POWER);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(m_controller->getResumeWindow());   // still armed from the first wake
        QCOMPARE(spy.count(), 1);                    // NOT re-emitted (debounce)
    }

    // 6. resumeTimeoutWindowSec == 0 → onPowerModeChanged is a no-op (early-out gate).
    void test_zeroTimeout_isNoOp() {
        // init() built a 2 s controller; rebuild with a zero window.
        delete m_controller;
        m_controller = nullptr;
        makeController(0);

        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::SUSPEND);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(!m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_MAIN(EntityControllerTest)
#include "test_entityController.moc"
