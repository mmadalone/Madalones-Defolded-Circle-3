// Copyright (c) 2024 madalone. Tests for Config default values and reset.
// Validates that MockConfig defaults match the QSettings defaults in config.cpp.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

TestCase {
    id: testCase
    name: "ConfigDefaults"
    when: windowShown

    function init() {
        ScreensaverConfig.resetDefaults();
    }

    // ── All 40 charging* defaults ─────────────────────────────
    // Values sourced from config.cpp QSettings::value() calls

    function test_default_theme() { compare(ScreensaverConfig.theme, "matrix"); }
    function test_default_showClock() { compare(ScreensaverConfig.showClock, false); }
    function test_default_showBattery() { compare(ScreensaverConfig.showBatteryEnabled, true); }
    function test_default_matrixColor() { compare(ScreensaverConfig.matrixColor, "#00ff41"); }
    function test_default_matrixSpeed() { compare(ScreensaverConfig.matrixSpeed, 50); }
    function test_default_matrixDensity() { compare(ScreensaverConfig.matrixDensity, 70); }
    function test_default_matrixColorMode() { compare(ScreensaverConfig.colorMode, "green"); }
    function test_default_matrixTrail() { compare(ScreensaverConfig.matrixTrail, 50); }
    function test_default_matrixFontSize() { compare(ScreensaverConfig.fontSize, 16); }
    function test_default_matrixCharset() { compare(ScreensaverConfig.charset, "ascii"); }
    function test_default_matrixGlow() { compare(ScreensaverConfig.glow, true); }
    function test_default_matrixGlitch() { compare(ScreensaverConfig.glitch, true); }
    function test_default_matrixGlitchRate() { compare(ScreensaverConfig.glitchRate, 30); }
    function test_default_matrixGlitchFlash() { compare(ScreensaverConfig.glitchFlash, true); }
    function test_default_matrixGlitchStutter() { compare(ScreensaverConfig.glitchStutter, true); }
    function test_default_matrixGlitchReverse() { compare(ScreensaverConfig.glitchReverse, true); }
    function test_default_matrixGlitchDirection() { compare(ScreensaverConfig.glitchDirection, true); }
    function test_default_matrixGlitchDirRate() { compare(ScreensaverConfig.glitchDirRate, 30); }
    function test_default_matrixGlitchDirCardinal() { compare(ScreensaverConfig.glitchDirCardinal, false); }
    function test_default_matrixGlitchChaos() { compare(ScreensaverConfig.glitchChaos, false); }
    function test_default_matrixGlitchChaosFrequency() { compare(ScreensaverConfig.glitchChaosFrequency, 50); }
    function test_default_matrixGlitchChaosSurge() { compare(ScreensaverConfig.glitchChaosSurge, true); }
    function test_default_matrixGlitchChaosScramble() { compare(ScreensaverConfig.glitchChaosScramble, true); }
    function test_default_matrixGlitchChaosFreeze() { compare(ScreensaverConfig.glitchChaosFreeze, true); }
    function test_default_matrixGlitchChaosScatter() { compare(ScreensaverConfig.glitchChaosScatter, true); }
    function test_default_matrixFade() { compare(ScreensaverConfig.matrixFade, 60); }
    function test_default_matrixDirection() { compare(ScreensaverConfig.direction, "down"); }
    function test_default_matrixInvertTrail() { compare(ScreensaverConfig.invertTrail, false); }
    function test_default_matrixMessages() { compare(ScreensaverConfig.messages, ""); }
    function test_default_matrixMessageInterval() { compare(ScreensaverConfig.messageInterval, 10); }
    function test_default_matrixMessageRandom() { compare(ScreensaverConfig.messageRandom, true); }
    function test_default_matrixMessageDirection() { compare(ScreensaverConfig.messageDirection, "horizontal-lr"); }
    function test_default_matrixMessageFlash() { compare(ScreensaverConfig.messageFlash, true); }
    function test_default_matrixMessagePulse() { compare(ScreensaverConfig.messagePulse, true); }
    function test_default_tapToClose() { compare(ScreensaverConfig.tapToClose, true); }
    function test_default_idleEnabled() { compare(ScreensaverConfig.idleEnabled, false); }
    function test_default_idleTimeout() { compare(ScreensaverConfig.idleTimeout, 45); }
    function test_default_motionToClose() { compare(ScreensaverConfig.motionToClose, false); }
    function test_default_batteryDockedOnly() { compare(ScreensaverConfig.batteryDockedOnly, true); }

    // ── Reset restores all values ─────────────────────────────

    function test_reset_restores_after_changes() {
        ScreensaverConfig.theme = "minimal";
        ScreensaverConfig.showClock = true;
        ScreensaverConfig.matrixSpeed = 99;
        ScreensaverConfig.glitch = false;
        ScreensaverConfig.messages = "MODIFIED";
        ScreensaverConfig.idleEnabled = true;

        ScreensaverConfig.resetDefaults();

        compare(ScreensaverConfig.theme, "matrix", "theme reset");
        compare(ScreensaverConfig.showClock, false, "clock reset");
        compare(ScreensaverConfig.matrixSpeed, 50, "speed reset");
        compare(ScreensaverConfig.glitch, true, "glitch reset");
        compare(ScreensaverConfig.messages, "", "messages reset");
        compare(ScreensaverConfig.idleEnabled, false, "idle reset");
    }
}
