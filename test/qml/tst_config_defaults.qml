// Copyright (c) 2024 madalone. Tests for Config default values and reset.
// Validates that MockConfig defaults match the QSettings defaults in config.cpp.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Config 1.0

TestCase {
    id: testCase
    name: "ConfigDefaults"
    when: windowShown

    function init() {
        Config.resetDefaults();
    }

    // ── All 40 charging* defaults ─────────────────────────────
    // Values sourced from config.cpp QSettings::value() calls

    function test_default_theme() { compare(Config.chargingTheme, "matrix"); }
    function test_default_showClock() { compare(Config.chargingShowClock, false); }
    function test_default_showBattery() { compare(Config.chargingShowBattery, true); }
    function test_default_matrixColor() { compare(Config.chargingMatrixColor, "#00ff41"); }
    function test_default_matrixSpeed() { compare(Config.chargingMatrixSpeed, 50); }
    function test_default_matrixDensity() { compare(Config.chargingMatrixDensity, 70); }
    function test_default_matrixColorMode() { compare(Config.chargingMatrixColorMode, "green"); }
    function test_default_matrixTrail() { compare(Config.chargingMatrixTrail, 50); }
    function test_default_matrixFontSize() { compare(Config.chargingMatrixFontSize, 16); }
    function test_default_matrixCharset() { compare(Config.chargingMatrixCharset, "ascii"); }
    function test_default_matrixGlow() { compare(Config.chargingMatrixGlow, true); }
    function test_default_matrixGlitch() { compare(Config.chargingMatrixGlitch, true); }
    function test_default_matrixGlitchRate() { compare(Config.chargingMatrixGlitchRate, 30); }
    function test_default_matrixGlitchFlash() { compare(Config.chargingMatrixGlitchFlash, true); }
    function test_default_matrixGlitchStutter() { compare(Config.chargingMatrixGlitchStutter, true); }
    function test_default_matrixGlitchReverse() { compare(Config.chargingMatrixGlitchReverse, true); }
    function test_default_matrixGlitchDirection() { compare(Config.chargingMatrixGlitchDirection, true); }
    function test_default_matrixGlitchDirRate() { compare(Config.chargingMatrixGlitchDirRate, 30); }
    function test_default_matrixGlitchDirCardinal() { compare(Config.chargingMatrixGlitchDirCardinal, false); }
    function test_default_matrixGlitchChaos() { compare(Config.chargingMatrixGlitchChaos, false); }
    function test_default_matrixGlitchChaosFrequency() { compare(Config.chargingMatrixGlitchChaosFrequency, 50); }
    function test_default_matrixGlitchChaosSurge() { compare(Config.chargingMatrixGlitchChaosSurge, true); }
    function test_default_matrixGlitchChaosScramble() { compare(Config.chargingMatrixGlitchChaosScramble, true); }
    function test_default_matrixGlitchChaosFreeze() { compare(Config.chargingMatrixGlitchChaosFreeze, true); }
    function test_default_matrixGlitchChaosScatter() { compare(Config.chargingMatrixGlitchChaosScatter, true); }
    function test_default_matrixFade() { compare(Config.chargingMatrixFade, 60); }
    function test_default_matrixDirection() { compare(Config.chargingMatrixDirection, "down"); }
    function test_default_matrixInvertTrail() { compare(Config.chargingMatrixInvertTrail, false); }
    function test_default_matrixMessages() { compare(Config.chargingMatrixMessages, ""); }
    function test_default_matrixMessageInterval() { compare(Config.chargingMatrixMessageInterval, 10); }
    function test_default_matrixMessageRandom() { compare(Config.chargingMatrixMessageRandom, true); }
    function test_default_matrixMessageDirection() { compare(Config.chargingMatrixMessageDirection, "horizontal-lr"); }
    function test_default_matrixMessageFlash() { compare(Config.chargingMatrixMessageFlash, true); }
    function test_default_matrixMessagePulse() { compare(Config.chargingMatrixMessagePulse, true); }
    function test_default_tapToClose() { compare(Config.chargingTapToClose, true); }
    function test_default_idleEnabled() { compare(Config.chargingIdleEnabled, false); }
    function test_default_idleTimeout() { compare(Config.chargingIdleTimeout, 45); }
    function test_default_motionToClose() { compare(Config.chargingMotionToClose, false); }
    function test_default_batteryDockedOnly() { compare(Config.chargingBatteryDockedOnly, true); }

    // ── Reset restores all values ─────────────────────────────

    function test_reset_restores_after_changes() {
        Config.chargingTheme = "minimal";
        Config.chargingShowClock = true;
        Config.chargingMatrixSpeed = 99;
        Config.chargingMatrixGlitch = false;
        Config.chargingMatrixMessages = "MODIFIED";
        Config.chargingIdleEnabled = true;

        Config.resetDefaults();

        compare(Config.chargingTheme, "matrix", "theme reset");
        compare(Config.chargingShowClock, false, "clock reset");
        compare(Config.chargingMatrixSpeed, 50, "speed reset");
        compare(Config.chargingMatrixGlitch, true, "glitch reset");
        compare(Config.chargingMatrixMessages, "", "messages reset");
        compare(Config.chargingIdleEnabled, false, "idle reset");
    }
}
