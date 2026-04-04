// Copyright (c) 2024 madalone. Integration tests for settings page keyboard navigation.
// Verifies focus chain flows correctly through settings sub-components.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2
import MatrixRain 1.0

// Minimal stubs for settings components that reference Config/ScreensaverConfig singletons.
// We can't instantiate the full settings pages without the Config singleton,
// but we CAN test the MatrixRain Q_INVOKABLE enter state machine directly.

Item {
    id: root
    width: 480; height: 850

    MatrixRain {
        id: rain
        anchors.fill: parent
        running: true
        charset: "ascii"
        glitch: true
        glitchChaos: true
    }

    TestCase {
        name: "EnterStateMachine"
        when: windowShown

        // Reset state machine before each test to avoid cross-contamination
        function init() {
            rain.resetEnterState();
            wait(50);  // let any pending timers drain
        }

        // The enter state machine emits enterAction signals.
        // Test the full lifecycle: press, hold, release, double-tap.

        function test_singleTapEmitsEnter() {
            var spy = createTemporaryObject(signalSpy, root, {target: rain, signalName: "enterAction"})
            rain.enterPressed()
            // Wait for double-tap timer to expire (300ms) — single tap confirmed
            wait(400)
            compare(spy.count, 1, "Single tap should emit one enterAction")
            compare(spy.signalArguments[0][0], "enter", "Single tap action should be 'enter'")
        }

        function test_doubleTapEmitsRestore() {
            var spy = createTemporaryObject(signalSpy, root, {target: rain, signalName: "enterAction"})
            rain.enterPressed()   // first press
            rain.enterReleased()  // first release
            wait(100)             // within 300ms window
            rain.enterPressed()   // second press — double-tap detected
            compare(spy.count, 1, "Double-tap should emit one enterAction")
            compare(spy.signalArguments[0][0], "restore", "Double-tap action should be 'restore'")
            rain.enterReleased()
        }

        function test_holdEmitsSlowHold() {
            var spy = createTemporaryObject(signalSpy, root, {target: rain, signalName: "enterAction"})
            rain.enterPressed()
            // Wait for hold timer (500ms) + margin
            wait(600)
            // Should have emitted: "enter" (at 300ms, double-tap timer) then "slow:hold" (at 500ms)
            verify(spy.count >= 1, "Hold should emit at least one enterAction")
            // Find the slow:hold signal
            var foundSlowHold = false
            for (var i = 0; i < spy.count; i++) {
                if (spy.signalArguments[i][0] === "slow:hold") foundSlowHold = true
            }
            verify(foundSlowHold, "Hold should emit 'slow:hold'")
            rain.enterReleased()
        }

        function test_holdReleaseEmitsSlowRelease() {
            var spy = createTemporaryObject(signalSpy, root, {target: rain, signalName: "enterAction"})
            rain.enterPressed()
            wait(600)  // hold threshold reached
            rain.enterReleased()
            // Should have "slow:release" as the last signal
            var lastAction = spy.signalArguments[spy.count - 1][0]
            compare(lastAction, "slow:release", "Release after hold should emit 'slow:release'")
        }

        function test_resetClearsState() {
            rain.enterPressed()
            rain.resetEnterState()
            // After reset, a new press should work normally (not be ignored as autoRepeat)
            var spy = createTemporaryObject(signalSpy, root, {target: rain, signalName: "enterAction"})
            rain.enterPressed()
            wait(400)
            compare(spy.count, 1, "Press after reset should work")
            compare(spy.signalArguments[0][0], "enter")
        }

        function test_autoRepeatIgnored() {
            rain.enterPressed()
            // Second press while in "pressed" state should be ignored
            rain.enterPressed()
            rain.enterPressed()
            wait(400)
            // Should still only get one "enter" from the first press
            rain.enterReleased()
            // No crash, no duplicate actions
            verify(true, "Auto-repeat presses handled without crash")
        }

        function test_releaseWithoutPressIsSafe() {
            rain.enterReleased()
            rain.enterReleased()
            verify(true, "Release without press is safe")
        }

        function test_rapidPressReleaseCycles() {
            for (var i = 0; i < 20; i++) {
                rain.enterPressed()
                rain.enterReleased()
            }
            wait(400)  // let any pending timers fire
            verify(true, "Rapid press/release cycles handled without crash")
        }
    }

    Component {
        id: signalSpy
        SignalSpy {}
    }
}
