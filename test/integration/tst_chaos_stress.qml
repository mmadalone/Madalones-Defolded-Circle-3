// Copyright (c) 2024 madalone. Chaos stress test for MatrixRainItem.
// Exercises all chaos/glitch features at maximum intensity with the real renderer.
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
        glitch: true
        glitchRate: 100
        glitchChaos: true
        glitchChaosFrequency: 100
        glitchChaosIntensity: 100
        glitchChaosSurge: true
        glitchChaosScramble: true
        glitchChaosFreeze: true
        glitchChaosScatter: true
        glitchChaosScatterRate: 100
        glitchChaosScatterLength: 40
        glitchDirection: true
        glitchDirRate: 100
        glitchDirLength: 30
        glitchRandomColor: true
    }

    TestCase {
        name: "ChaosStress"
        when: windowShown

        function test_maxChaosNocrash() {
            // Run at max chaos for 3 seconds
            wait(3000)
            verify(rain.running, "Survived 3 seconds at max chaos")
        }

        function test_chaosToggleCycle() {
            for (var i = 0; i < 50; i++) {
                rain.glitchChaos = !rain.glitchChaos
            }
            wait(500)
            verify(rain.running, "Survived 50 chaos toggle cycles")
        }

        function test_scatterBurstStress() {
            rain.glitchChaosScatter = true
            rain.glitchChaosScatterRate = 100
            rain.glitchChaosScatterLength = 40
            wait(5000)
            verify(rain.running, "Survived 5 seconds of max scatter")
        }
    }
}
