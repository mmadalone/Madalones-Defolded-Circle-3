// Copyright (c) 2024 madalone. Integration tests for MatrixRainItem lifecycle.
// Tests the real renderer in a QQuickWindow with OpenGL — no mocks.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2
import MatrixRain 1.0

Item {
    id: root
    width: 480; height: 850

    MatrixRain {
        id: rain
        anchors.fill: parent
        running: true
        color: "#00ff41"
        colorMode: "green"
        speed: 1.0
        density: 0.7
        trailLength: 25
        fontSize: 16
        charset: "ascii"
    }

    TestCase {
        name: "MatrixRainLifecycle"
        when: windowShown

        function test_itemCreated() {
            verify(rain.width > 0, "Item has non-zero width")
            verify(rain.height > 0, "Item has non-zero height")
        }

        function test_rendersWithoutCrash() {
            // Wait for a few render frames
            wait(200)
            verify(rain.running, "Item is still running after rendering")
        }

        function test_allDirections() {
            var dirs = ["down", "up", "left", "right", "down-right", "down-left", "up-right", "up-left"]
            for (var i = 0; i < dirs.length; i++) {
                rain.direction = dirs[i]
                wait(50)  // let a frame render
            }
            // If we got here without crashing, all directions work
            verify(true)
        }

        function test_allCharsets() {
            var sets = ["ascii", "katakana", "binary", "digits"]
            for (var i = 0; i < sets.length; i++) {
                rain.charset = sets[i]
                wait(100)  // atlas rebuild
            }
            verify(true)
        }

        function test_allColorModes() {
            var modes = ["green", "blue", "red", "amber", "white", "purple", "rainbow", "rainbow_gradient", "neon"]
            for (var i = 0; i < modes.length; i++) {
                rain.colorMode = modes[i]
                wait(100)  // atlas rebuild
            }
            verify(true)
        }

        function test_rapidPropertyChanges() {
            // Toggle all bool properties rapidly — stress test for change guard + signal emit
            for (var i = 0; i < 20; i++) {
                rain.glitch = !rain.glitch
                rain.glow = !rain.glow
                rain.invertTrail = !rain.invertTrail
                rain.glitchFlash = !rain.glitchFlash
                rain.glitchStutter = !rain.glitchStutter
                rain.glitchReverse = !rain.glitchReverse
                rain.glitchDirection = !rain.glitchDirection
                rain.glitchChaos = !rain.glitchChaos
            }
            wait(100)
            verify(rain.running, "Item survived rapid property toggles")
        }

        function test_extremeDensity() {
            rain.density = 5.0  // max density
            rain.fontSize = 10  // min font
            wait(200)
            verify(rain.running, "Item handles extreme density without crash")
            rain.density = 0.7
            rain.fontSize = 16
        }

        function test_runningToggle() {
            rain.running = false
            wait(100)
            compare(rain.running, false, "Item stopped")
            rain.running = true
            wait(200)
            compare(rain.running, true, "Item resumes after running toggle")
        }

        function test_displayOffToggle() {
            rain.displayOff = true
            wait(100)
            compare(rain.displayOff, true, "displayOff is true")
            rain.displayOff = false
            wait(200)
            compare(rain.displayOff, false, "Item resumes after displayOff toggle")
        }
    }
}
