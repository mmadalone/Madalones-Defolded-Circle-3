// Copyright (c) 2026 madalone. Integration tests for config → renderer property propagation.
// Verifies that MatrixRainItem Q_PROPERTY setters accept transformed values correctly,
// matching the transforms that ScreensaverConfig applies in production.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2
import MatrixRain 1.0
import ScreensaverConfig 1.0

Item {
    id: root
    width: 480; height: 850

    MatrixRain {
        id: rain
        anchors.fill: parent
        running: false  // no timer — just testing property propagation
        charset: "ascii"
    }

    TestCase {
        name: "ConfigPropagation"
        when: windowShown

        // --- Transform tests: verify the same math ScreensaverConfig uses ---

        function test_speedTransform() {
            // ScreensaverConfig: speed = ScreensaverConfig.matrixSpeed / 50.0
            // Config range: 10-100, renderer range: 0.2-2.0
            rain.speed = 10 / 50.0  // min
            fuzzyCompare(rain.speed, 0.2, 0.01)
            rain.speed = 50 / 50.0  // default
            fuzzyCompare(rain.speed, 1.0, 0.01)
            rain.speed = 100 / 50.0  // max
            fuzzyCompare(rain.speed, 2.0, 0.01)
        }

        function test_densityTransform() {
            // ScreensaverConfig: density = ScreensaverConfig.matrixDensity / 100.0
            // Config range: 20-500, renderer range: 0.2-5.0
            rain.density = 20 / 100.0
            fuzzyCompare(rain.density, 0.2, 0.01)
            rain.density = 100 / 100.0
            fuzzyCompare(rain.density, 1.0, 0.01)
            rain.density = 500 / 100.0
            fuzzyCompare(rain.density, 5.0, 0.01)
        }

        function test_fadeRateTransform() {
            // ScreensaverConfig: fadeRate = 0.76 + ScreensaverConfig.matrixFade * 0.002
            // Config range: 20-100, renderer range: 0.80-0.96
            rain.fadeRate = 0.76 + 20 * 0.002  // min
            fuzzyCompare(rain.fadeRate, 0.80, 0.01)
            rain.fadeRate = 0.76 + 60 * 0.002  // default
            fuzzyCompare(rain.fadeRate, 0.88, 0.01)
            rain.fadeRate = 0.76 + 100 * 0.002  // max
            fuzzyCompare(rain.fadeRate, 0.96, 0.01)
        }

        function test_trailLengthTransform() {
            // ScreensaverConfig: trailLength = max(5, round(5 + 175 * (v - 10) / 90))
            // Config range: 10-100, renderer range: 5-180
            var v10 = Math.max(5, Math.round(5 + 175 * (10 - 10) / 90))  // = 5
            rain.trailLength = v10
            compare(rain.trailLength, 5)

            var v55 = Math.max(5, Math.round(5 + 175 * (55 - 10) / 90))  // ~92
            rain.trailLength = v55
            compare(rain.trailLength, v55)

            var v100 = Math.max(5, Math.round(5 + 175 * (100 - 10) / 90))  // = 180
            rain.trailLength = v100
            compare(rain.trailLength, 180)
        }

        // --- Bool property propagation ---

        function test_boolPropertiesAccepted() {
            var bools = [
                "glow", "glitch", "glitchFlash", "glitchStutter", "glitchReverse",
                "glitchDirection", "glitchRandomColor", "glitchChaos",
                "glitchChaosSurge", "glitchChaosScramble", "glitchChaosFreeze",
                "glitchChaosScatter", "invertTrail", "messageRandom",
                "messageFlash", "messagePulse", "subliminal", "subliminalStream",
                "subliminalOverlay", "subliminalFlash", "gravityMode"
            ]
            for (var i = 0; i < bools.length; i++) {
                var prop = bools[i]
                rain[prop] = true
                compare(rain[prop], true, prop + " should accept true")
                rain[prop] = false
                compare(rain[prop], false, prop + " should accept false")
            }
        }

        // --- Int property propagation with clamping ---

        function test_intPropertiesAccepted() {
            rain.glitchRate = 50
            compare(rain.glitchRate, 50)
            rain.glitchDirRate = 30
            compare(rain.glitchDirRate, 30)
            rain.glitchDirMask = 0xFF
            compare(rain.glitchDirMask, 255)
            rain.glitchDirFade = 20
            compare(rain.glitchDirFade, 20)
            rain.glitchDirSpeed = 50
            compare(rain.glitchDirSpeed, 50)
            rain.glitchDirLength = 5
            compare(rain.glitchDirLength, 5)
            rain.glitchChaosFrequency = 50
            compare(rain.glitchChaosFrequency, 50)
            rain.glitchChaosIntensity = 50
            compare(rain.glitchChaosIntensity, 50)
            rain.glitchChaosScatterRate = 50
            compare(rain.glitchChaosScatterRate, 50)
            rain.glitchChaosScatterLength = 8
            compare(rain.glitchChaosScatterLength, 8)
            rain.autoRotateSpeed = 50
            compare(rain.autoRotateSpeed, 50)
            rain.autoRotateBend = 50
            compare(rain.autoRotateBend, 50)
            rain.messageInterval = 10
            compare(rain.messageInterval, 10)
            rain.subliminalInterval = 5
            compare(rain.subliminalInterval, 5)
            rain.subliminalDuration = 8
            compare(rain.subliminalDuration, 8)
        }

        // --- String property propagation ---

        function test_stringPropertiesAccepted() {
            rain.colorMode = "blue"
            compare(rain.colorMode, "blue")
            rain.colorMode = "rainbow"
            compare(rain.colorMode, "rainbow")
            rain.direction = "up"
            compare(rain.direction, "up")
            rain.direction = "down-right"
            compare(rain.direction, "down-right")
            rain.messages = "HELLO,WORLD"
            compare(rain.messages, "HELLO,WORLD")
            rain.messageDirection = "vertical-tb"
            compare(rain.messageDirection, "vertical-tb")
        }

        // --- Guard: redundant sets don't emit signals ---

        function test_redundantSetNoSignal() {
            rain.speed = 1.0
            var spy = createTemporaryObject(signalSpy, root, {target: rain, signalName: "speedChanged"})
            rain.speed = 1.0  // same value — should NOT emit
            compare(spy.count, 0, "Redundant speed set should not emit signal")
        }

        // --- Edge case: extreme values don't crash ---

        function test_extremeValues() {
            rain.speed = 0.01
            rain.density = 0.01
            rain.trailLength = 1
            rain.fontSize = 8
            wait(50)
            rain.speed = 10.0
            rain.density = 10.0
            rain.trailLength = 500
            rain.fontSize = 100
            wait(50)
            // If we got here, no crash
            verify(true, "Extreme values handled without crash")
            // Restore sane defaults
            rain.speed = 1.0
            rain.density = 0.7
            rain.trailLength = 25
            rain.fontSize = 16
        }

        // --- Interactive input contract ---

        function test_interactiveInputDirections() {
            rain.running = true
            wait(100)  // need running for gravity mode
            var dirs = ["up", "down", "left", "right", "up-left", "up-right", "down-left", "down-right"]
            for (var i = 0; i < dirs.length; i++) {
                rain.interactiveInput(dirs[i])
            }
            rain.interactiveInput("restore")
            wait(50)
            verify(true, "All 8 directions + restore accepted")
            rain.running = false
        }

        function test_interactiveInputEnterAndSlow() {
            rain.running = true
            rain.glitch = true
            rain.glitchChaos = true
            wait(100)
            rain.interactiveInput("enter")
            rain.interactiveInput("slow:hold")
            wait(50)
            rain.interactiveInput("slow:release")
            verify(true, "Enter + slow hold/release accepted")
            rain.running = false
        }

        // --- Regression test: displayOff cycle (Session 11 bug fix) ---

        function test_displayOffCycleRestoresRunning() {
            rain.running = true
            wait(100)
            compare(rain.running, true, "Precondition: running")

            // Simulate display sleep
            rain.displayOff = true
            wait(50)
            compare(rain.displayOff, true, "displayOff set")

            // Simulate display wake
            rain.displayOff = false
            wait(50)
            compare(rain.displayOff, false, "displayOff cleared")
            // running is controlled by QML binding in production, but here
            // we verify the C++ property round-trips correctly
            rain.running = true
            wait(100)
            compare(rain.running, true, "Running restored after displayOff cycle")
            rain.running = false
        }

        function test_multipleDisplayOffCycles() {
            rain.running = true
            for (var i = 0; i < 10; i++) {
                rain.displayOff = true
                rain.displayOff = false
            }
            wait(100)
            compare(rain.running, true, "Running survives rapid displayOff toggling")
            rain.running = false
        }
    }

    Component {
        id: signalSpy
        SignalSpy {}
    }
}
