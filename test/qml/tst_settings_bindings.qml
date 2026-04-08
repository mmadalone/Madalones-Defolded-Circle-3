// Copyright (c) 2024 madalone. Tests for Config property bindings (read + write).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

TestCase {
    id: testCase
    name: "SettingsBindings"
    when: windowShown

    // Signal spy wiring — one spy per major property group
    SignalSpy { id: themeSpy;       target: Config; signalName: "chargingThemeChanged" }
    SignalSpy { id: clockSpy;       target: Config; signalName: "chargingShowClockChanged" }
    SignalSpy { id: batterySpy;     target: Config; signalName: "chargingShowBatteryChanged" }
    SignalSpy { id: colorModeSpy;   target: Config; signalName: "chargingMatrixColorModeChanged" }
    SignalSpy { id: charsetSpy;     target: Config; signalName: "chargingMatrixCharsetChanged" }
    SignalSpy { id: speedSpy;       target: Config; signalName: "chargingMatrixSpeedChanged" }
    SignalSpy { id: densitySpy;     target: Config; signalName: "chargingMatrixDensityChanged" }
    SignalSpy { id: trailSpy;       target: Config; signalName: "chargingMatrixTrailChanged" }
    SignalSpy { id: fadeSpy;        target: Config; signalName: "chargingMatrixFadeChanged" }
    SignalSpy { id: fontSizeSpy;    target: Config; signalName: "chargingMatrixFontSizeChanged" }
    SignalSpy { id: directionSpy;   target: Config; signalName: "chargingMatrixDirectionChanged" }
    SignalSpy { id: glowSpy;        target: Config; signalName: "chargingMatrixGlowChanged" }
    SignalSpy { id: glitchSpy;      target: Config; signalName: "chargingMatrixGlitchChanged" }
    SignalSpy { id: glitchRateSpy;  target: Config; signalName: "chargingMatrixGlitchRateChanged" }
    SignalSpy { id: invertTrailSpy; target: Config; signalName: "chargingMatrixInvertTrailChanged" }
    SignalSpy { id: messagesSpy;    target: Config; signalName: "chargingMatrixMessagesChanged" }
    SignalSpy { id: idleEnabledSpy; target: Config; signalName: "chargingIdleEnabledChanged" }
    SignalSpy { id: idleTimeoutSpy; target: Config; signalName: "chargingIdleTimeoutChanged" }
    SignalSpy { id: tapCloseSpy;    target: Config; signalName: "chargingTapToCloseChanged" }
    SignalSpy { id: motionCloseSpy; target: Config; signalName: "chargingMotionToCloseChanged" }
    SignalSpy { id: chaosSpy;       target: Config; signalName: "chargingMatrixGlitchChaosChanged" }

    // ── init / cleanup ────────────────────────────────────────
    function init() {
        ScreensaverConfig.resetDefaults();
        themeSpy.clear(); clockSpy.clear(); batterySpy.clear();
        colorModeSpy.clear(); charsetSpy.clear(); speedSpy.clear();
        densitySpy.clear(); trailSpy.clear(); fadeSpy.clear();
        fontSizeSpy.clear(); directionSpy.clear(); glowSpy.clear();
        glitchSpy.clear(); glitchRateSpy.clear(); invertTrailSpy.clear();
        messagesSpy.clear(); idleEnabledSpy.clear(); idleTimeoutSpy.clear();
        tapCloseSpy.clear(); motionCloseSpy.clear(); chaosSpy.clear();
    }

    // ─────────────────────────────────────────
    // Theme selector bindings
    // ─────────────────────────────────────────

    function test_theme_default() {
        compare(ScreensaverConfig.theme, "matrix", "default theme is matrix");
    }

    function test_theme_write_read() {
        ScreensaverConfig.theme = "starfield";
        compare(ScreensaverConfig.theme, "starfield", "theme updated to starfield");
        compare(themeSpy.count, 1, "theme signal emitted once");
    }

    function test_theme_no_signal_on_same_value() {
        ScreensaverConfig.theme = "matrix";  // already default
        compare(themeSpy.count, 0, "no signal when value unchanged");
    }

    // ─────────────────────────────────────────
    // Boolean toggle bindings
    // ─────────────────────────────────────────

    function test_show_clock_toggle() {
        compare(ScreensaverConfig.showClock, false, "clock off by default");
        ScreensaverConfig.showClock = true;
        compare(ScreensaverConfig.showClock, true, "clock toggled on");
        compare(clockSpy.count, 1, "clock signal emitted");
    }

    function test_show_battery_toggle() {
        compare(ScreensaverConfig.showBatteryEnabled, true, "battery on by default");
        ScreensaverConfig.showBatteryEnabled = false;
        compare(ScreensaverConfig.showBatteryEnabled, false, "battery toggled off");
        compare(batterySpy.count, 1, "battery signal emitted");
    }

    function test_glow_toggle() {
        compare(ScreensaverConfig.glow, true, "glow on by default");
        ScreensaverConfig.glow = false;
        compare(ScreensaverConfig.glow, false, "glow toggled off");
        compare(glowSpy.count, 1, "glow signal emitted");
    }

    function test_glitch_toggle() {
        compare(ScreensaverConfig.glitch, true, "glitch on by default");
        ScreensaverConfig.glitch = false;
        compare(ScreensaverConfig.glitch, false, "glitch toggled off");
        compare(glitchSpy.count, 1, "glitch signal emitted");
    }

    function test_invert_trail_toggle() {
        compare(ScreensaverConfig.invertTrail, false, "invert trail off by default");
        ScreensaverConfig.invertTrail = true;
        compare(ScreensaverConfig.invertTrail, true, "invert trail toggled on");
        compare(invertTrailSpy.count, 1, "invert trail signal emitted");
    }

    function test_tap_to_close_toggle() {
        compare(ScreensaverConfig.tapToClose, true, "tap to close on by default");
        ScreensaverConfig.tapToClose = false;
        compare(ScreensaverConfig.tapToClose, false, "tap to close toggled off");
        compare(tapCloseSpy.count, 1, "tap to close signal emitted");
    }

    function test_motion_to_close_toggle() {
        compare(ScreensaverConfig.motionToClose, false, "motion to close off by default");
        ScreensaverConfig.motionToClose = true;
        compare(ScreensaverConfig.motionToClose, true, "motion to close toggled on");
        compare(motionCloseSpy.count, 1, "motion to close signal emitted");
    }

    function test_idle_enabled_toggle() {
        compare(ScreensaverConfig.idleEnabled, false, "idle disabled by default");
        ScreensaverConfig.idleEnabled = true;
        compare(ScreensaverConfig.idleEnabled, true, "idle toggled on");
        compare(idleEnabledSpy.count, 1, "idle enabled signal emitted");
    }

    function test_chaos_toggle() {
        compare(ScreensaverConfig.glitchChaos, false, "chaos off by default");
        ScreensaverConfig.glitchChaos = true;
        compare(ScreensaverConfig.glitchChaos, true, "chaos toggled on");
        compare(chaosSpy.count, 1, "chaos signal emitted");
    }

    // ─────────────────────────────────────────
    // String selector bindings
    // ─────────────────────────────────────────

    function test_color_mode_cycle() {
        compare(ScreensaverConfig.colorMode, "green", "default color mode is green");
        ScreensaverConfig.colorMode = "rainbow";
        compare(ScreensaverConfig.colorMode, "rainbow", "color mode set to rainbow");
        compare(colorModeSpy.count, 1, "color mode signal emitted");
    }

    function test_charset_cycle() {
        compare(ScreensaverConfig.charset, "ascii", "default charset is ascii");
        ScreensaverConfig.charset = "katakana";
        compare(ScreensaverConfig.charset, "katakana", "charset set to katakana");
        compare(charsetSpy.count, 1, "charset signal emitted");
    }

    function test_direction_write() {
        compare(ScreensaverConfig.direction, "down", "default direction is down");
        ScreensaverConfig.direction = "up-left";
        compare(ScreensaverConfig.direction, "up-left", "direction set to up-left");
        compare(directionSpy.count, 1, "direction signal emitted");
    }

    // ─────────────────────────────────────────
    // Integer slider bindings
    // ─────────────────────────────────────────

    function test_speed_write() {
        compare(ScreensaverConfig.matrixSpeed, 50, "default speed is 50");
        ScreensaverConfig.matrixSpeed = 80;
        compare(ScreensaverConfig.matrixSpeed, 80, "speed set to 80");
        compare(speedSpy.count, 1, "speed signal emitted");
    }

    function test_density_write() {
        compare(ScreensaverConfig.matrixDensity, 70, "default density is 70");
        ScreensaverConfig.matrixDensity = 150;
        compare(ScreensaverConfig.matrixDensity, 150, "density set to 150");
        compare(densitySpy.count, 1, "density signal emitted");
    }

    function test_trail_write() {
        compare(ScreensaverConfig.matrixTrail, 50, "default trail is 50");
        ScreensaverConfig.matrixTrail = 75;
        compare(ScreensaverConfig.matrixTrail, 75, "trail set to 75");
        compare(trailSpy.count, 1, "trail signal emitted");
    }

    function test_fade_write() {
        compare(ScreensaverConfig.matrixFade, 60, "default fade is 60");
        ScreensaverConfig.matrixFade = 90;
        compare(ScreensaverConfig.matrixFade, 90, "fade set to 90");
        compare(fadeSpy.count, 1, "fade signal emitted");
    }

    function test_font_size_write() {
        compare(ScreensaverConfig.fontSize, 16, "default font size is 16");
        ScreensaverConfig.fontSize = 32;
        compare(ScreensaverConfig.fontSize, 32, "font size set to 32");
        compare(fontSizeSpy.count, 1, "font size signal emitted");
    }

    function test_glitch_rate_write() {
        compare(ScreensaverConfig.glitchRate, 30, "default glitch rate is 30");
        ScreensaverConfig.glitchRate = 60;
        compare(ScreensaverConfig.glitchRate, 60, "glitch rate set to 60");
        compare(glitchRateSpy.count, 1, "glitch rate signal emitted");
    }

    function test_idle_timeout_write() {
        compare(ScreensaverConfig.idleTimeout, 45, "default idle timeout is 45");
        ScreensaverConfig.idleTimeout = 30;
        compare(ScreensaverConfig.idleTimeout, 30, "idle timeout set to 30");
        compare(idleTimeoutSpy.count, 1, "idle timeout signal emitted");
    }

    // ─────────────────────────────────────────
    // Messages text binding
    // ─────────────────────────────────────────

    function test_messages_default_empty() {
        compare(ScreensaverConfig.messages, "", "messages empty by default");
    }

    function test_messages_write() {
        ScreensaverConfig.messages = "WAKE UP,NEO";
        compare(ScreensaverConfig.messages, "WAKE UP,NEO", "messages updated");
        compare(messagesSpy.count, 1, "messages signal emitted");
    }
}
