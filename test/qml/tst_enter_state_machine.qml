// Copyright (c) 2024 madalone. QML tests for ChargingScreen enter button state machine.
// Tests state transitions: idle→pressed→held, idle→pressed→idle, double-tap detection.
// State machine logic extracted from src/qml/components/ChargingScreen.qml lines 61-157.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

TestCase {
    id: testCase
    name: "EnterStateMachine"
    when: windowShown

    // --- Configuration (matches ChargingScreen.qml) ---
    readonly property int doubleTapMs: 300
    readonly property int holdThresholdMs: 500

    // --- State machine (exact copy from ChargingScreen.qml lines 137-157) ---
    property string enterState: "idle"
    property var inputLog: []

    function mockInteractiveInput(action) {
        inputLog.push(action);
    }

    Timer {
        id: enterDoubleTapTimer; interval: testCase.doubleTapMs
        onTriggered: testCase.mockInteractiveInput("enter")
    }
    Timer {
        id: enterHoldTimer; interval: testCase.holdThresholdMs
        onTriggered: {
            testCase.enterState = "held";
            enterDoubleTapTimer.stop();
            testCase.mockInteractiveInput("slow:hold");
        }
    }

    // --- Button handlers (exact copy from ChargingScreen.qml lines 61-88) ---
    function simulatePress() {
        // Matches DPAD_MIDDLE "pressed" handler
        if (enterState !== "idle") return;  // ignore autoRepeat
        enterState = "pressed";
        if (enterDoubleTapTimer.running) {
            // Second press within window — double-tap → restore
            enterDoubleTapTimer.stop();
            enterHoldTimer.stop();
            enterState = "idle";
            mockInteractiveInput("restore");
        } else {
            // First press — start hold + double-tap timers
            enterHoldTimer.restart();
            enterDoubleTapTimer.restart();
        }
    }

    function simulateRelease() {
        // Matches DPAD_MIDDLE "released" handler
        if (enterState === "held") {
            mockInteractiveInput("slow:release");
        }
        // enterDoubleTapTimer NOT stopped on release — needed for double-tap detection
        enterHoldTimer.stop();
        enterState = "idle";
    }

    // --- Test lifecycle ---
    function init() {
        enterState = "idle";
        inputLog = [];
        enterHoldTimer.stop();
        enterDoubleTapTimer.stop();
    }

    // ─────────────────────────────────────────
    // Initial state
    // ─────────────────────────────────────────
    function test_initial_state() {
        compare(enterState, "idle", "initial state should be idle");
        compare(inputLog.length, 0, "no actions on init");
    }

    // ─────────────────────────────────────────
    // idle → pressed transition
    // ─────────────────────────────────────────
    function test_press_transitions_to_pressed() {
        simulatePress();
        compare(enterState, "pressed", "press should transition to pressed");
    }

    function test_press_starts_both_timers() {
        simulatePress();
        verify(enterHoldTimer.running, "hold timer should start on press");
        verify(enterDoubleTapTimer.running, "double-tap timer should start on press");
    }

    function test_press_fires_no_action() {
        simulatePress();
        compare(inputLog.length, 0, "no action fires on initial press");
    }

    // ─────────────────────────────────────────
    // pressed → held transition (hold timer fires)
    // ─────────────────────────────────────────
    function test_hold_transition() {
        simulatePress();
        compare(enterState, "pressed");
        wait(holdThresholdMs + 100);
        compare(enterState, "held", "should transition to held after threshold");
    }

    function test_hold_fires_enter_then_slow_hold() {
        // Double-tap timer fires at 300ms ("enter"), hold timer at 500ms ("slow:hold")
        simulatePress();
        wait(holdThresholdMs + 100);
        compare(inputLog.length, 2, "two actions: enter at 300ms + slow:hold at 500ms");
        compare(inputLog[0], "enter", "chaos burst fires first (double-tap timer)");
        compare(inputLog[1], "slow:hold", "slowdown fires second (hold timer)");
    }

    function test_hold_stops_double_tap_timer() {
        simulatePress();
        wait(holdThresholdMs + 100);
        verify(!enterDoubleTapTimer.running, "double-tap timer stopped when entering held");
    }

    // ─────────────────────────────────────────
    // held → idle transition (release after hold)
    // ─────────────────────────────────────────
    function test_held_release_returns_to_idle() {
        simulatePress();
        wait(holdThresholdMs + 100);
        compare(enterState, "held");
        simulateRelease();
        compare(enterState, "idle", "release from held should return to idle");
    }

    function test_held_release_fires_slow_release() {
        simulatePress();
        wait(holdThresholdMs + 100);
        simulateRelease();
        compare(inputLog.length, 3, "enter + hold + release = 3 actions");
        compare(inputLog[0], "enter");
        compare(inputLog[1], "slow:hold");
        compare(inputLog[2], "slow:release", "release from held fires slow:release");
    }

    // ─────────────────────────────────────────
    // pressed → idle transition (quick release, single tap confirmed)
    // ─────────────────────────────────────────
    function test_quick_release_returns_to_idle() {
        simulatePress();
        wait(50);
        simulateRelease();
        compare(enterState, "idle", "quick release returns to idle");
    }

    function test_quick_release_no_immediate_action() {
        simulatePress();
        wait(50);
        simulateRelease();
        compare(inputLog.length, 0, "no action immediately after quick release");
    }

    function test_single_tap_confirmed_after_window() {
        simulatePress();
        wait(50);
        simulateRelease();
        // Double-tap window still open — wait for it to expire
        wait(doubleTapMs + 100);
        compare(inputLog.length, 1, "single tap confirmed after double-tap window");
        compare(inputLog[0], "enter", "single tap fires enter (chaos burst)");
    }

    // ─────────────────────────────────────────
    // Double-tap detection (second press within window)
    // ─────────────────────────────────────────
    function test_double_tap_fires_restore() {
        simulatePress();
        simulateRelease();
        wait(100);  // within 300ms double-tap window
        simulatePress();
        compare(inputLog[inputLog.length - 1], "restore", "double-tap fires restore");
    }

    function test_double_tap_returns_to_idle() {
        simulatePress();
        simulateRelease();
        wait(100);
        simulatePress();
        compare(enterState, "idle", "double-tap returns to idle immediately");
    }

    function test_double_tap_stops_both_timers() {
        simulatePress();
        simulateRelease();
        wait(100);
        simulatePress();
        verify(!enterHoldTimer.running, "hold timer stopped on double-tap");
        verify(!enterDoubleTapTimer.running, "double-tap timer stopped on double-tap");
    }

    function test_double_tap_no_enter_action() {
        // Double-tap should fire "restore", NOT "enter"
        simulatePress();
        simulateRelease();
        wait(100);
        simulatePress();
        // Wait past the original double-tap timer interval
        wait(doubleTapMs + 100);
        // Only "restore" should be in the log — no "enter"
        for (var i = 0; i < inputLog.length; i++) {
            verify(inputLog[i] !== "enter",
                "double-tap should suppress enter action, got: " + inputLog.join(", "));
        }
    }

    function test_double_tap_requires_second_press_within_window() {
        // Second press AFTER window expires → two single taps, not a double-tap
        simulatePress();
        simulateRelease();
        wait(doubleTapMs + 100);  // past the window
        compare(inputLog.length, 1, "first single tap confirmed");
        compare(inputLog[0], "enter");
        // Second press is a new single tap
        simulatePress();
        compare(enterState, "pressed", "second press starts fresh");
        verify(!inputLog.some(function(a) { return a === "restore"; }),
            "no restore action when presses are outside window");
    }

    // ─────────────────────────────────────────
    // Auto-repeat rejection
    // ─────────────────────────────────────────
    function test_autorepeat_ignored_in_pressed() {
        simulatePress();
        compare(enterState, "pressed");
        simulatePress();  // autoRepeat — enterState !== "idle" → ignored
        compare(enterState, "pressed", "autoRepeat should not change state");
        compare(inputLog.length, 0, "autoRepeat should not fire any action");
    }

    function test_autorepeat_ignored_in_held() {
        simulatePress();
        wait(holdThresholdMs + 100);
        compare(enterState, "held");
        simulatePress();  // autoRepeat — enterState !== "idle" → ignored
        compare(enterState, "held", "autoRepeat in held should not change state");
    }

    // ─────────────────────────────────────────
    // Full sequence: press → hold → release cycle
    // ─────────────────────────────────────────
    function test_full_hold_cycle() {
        // Press
        simulatePress();
        compare(enterState, "pressed");
        compare(inputLog.length, 0);
        // Hold fires (double-tap timer at 300ms, hold timer at 500ms)
        wait(holdThresholdMs + 100);
        compare(enterState, "held");
        compare(inputLog, ["enter", "slow:hold"]);
        // Release
        simulateRelease();
        compare(enterState, "idle");
        compare(inputLog, ["enter", "slow:hold", "slow:release"]);
    }
}
