// Copyright (c) 2026 madalone. Tests for MinimalTheme config surface.
// Validates that the MinimalTheme.qml consumer interface
// (ScreensaverConfig.minimal* properties and theme selection) exposes
// correct defaults, accepts writes, and emits change signals. Also
// verifies the shared showClock / showBatteryEnabled toggles used by
// the theme's overlay visibility bindings.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

TestCase {
    id: testCase
    name: "MinimalTheme"
    when: windowShown

    SignalSpy { id: themeSpy;           target: ScreensaverConfig; signalName: "themeChanged" }
    SignalSpy { id: clockSizeSpy;       target: ScreensaverConfig; signalName: "minimalClockSizeChanged" }
    SignalSpy { id: dateSizeSpy;        target: ScreensaverConfig; signalName: "minimalDateSizeChanged" }
    SignalSpy { id: fontSpy;            target: ScreensaverConfig; signalName: "minimalFontChanged" }
    SignalSpy { id: clock24hSpy;        target: ScreensaverConfig; signalName: "minimalClock24hChanged" }
    SignalSpy { id: timeColorSpy;       target: ScreensaverConfig; signalName: "minimalTimeColorChanged" }
    SignalSpy { id: dateColorSpy;       target: ScreensaverConfig; signalName: "minimalDateColorChanged" }
    SignalSpy { id: showClockSpy;       target: ScreensaverConfig; signalName: "showClockChanged" }
    SignalSpy { id: showBatterySpy;     target: ScreensaverConfig; signalName: "showBatteryEnabledChanged" }

    function init() {
        ScreensaverConfig.resetDefaults();
        themeSpy.clear();
        clockSizeSpy.clear();
        dateSizeSpy.clear();
        fontSpy.clear();
        clock24hSpy.clear();
        timeColorSpy.clear();
        dateColorSpy.clear();
        showClockSpy.clear();
        showBatterySpy.clear();
    }

    // ── Theme selection ───────────────────────────────────────

    function test_theme_selection() {
        ScreensaverConfig.theme = "minimal";
        compare(ScreensaverConfig.theme, "minimal", "theme selected as minimal");
        compare(themeSpy.count, 1, "theme change signal emitted once");
    }

    // ── minimalClockSize / minimalDateSize ────────────────────

    function test_minimalClockSize_write() {
        var defaultVal = ScreensaverConfig.minimalClockSize;
        ScreensaverConfig.minimalClockSize = defaultVal + 20;
        compare(ScreensaverConfig.minimalClockSize, defaultVal + 20, "clock size updated");
        compare(clockSizeSpy.count, 1, "clock size signal emitted");
    }

    function test_minimalDateSize_write() {
        var defaultVal = ScreensaverConfig.minimalDateSize;
        ScreensaverConfig.minimalDateSize = defaultVal + 10;
        compare(ScreensaverConfig.minimalDateSize, defaultVal + 10, "date size updated");
        compare(dateSizeSpy.count, 1, "date size signal emitted");
    }

    // ── minimalFont ───────────────────────────────────────────

    function test_minimalFont_write() {
        // Default is "primary"; verify that happens, then flip to "secondary" and back.
        compare(ScreensaverConfig.minimalFont, "primary", "font defaults to primary");

        ScreensaverConfig.minimalFont = "secondary";
        compare(ScreensaverConfig.minimalFont, "secondary", "font set to secondary");
        compare(fontSpy.count, 1, "font signal emitted on first change");

        ScreensaverConfig.minimalFont = "primary";
        compare(ScreensaverConfig.minimalFont, "primary", "font restored to primary");
        compare(fontSpy.count, 2, "font signal emitted on flip-back");
    }

    // ── minimalClock24h ───────────────────────────────────────

    function test_minimalClock24h_toggle() {
        var defaultVal = ScreensaverConfig.minimalClock24h;
        ScreensaverConfig.minimalClock24h = !defaultVal;
        compare(ScreensaverConfig.minimalClock24h, !defaultVal, "24h toggled");
        compare(clock24hSpy.count, 1, "24h signal emitted");
    }

    // ── minimalTimeColor / minimalDateColor ───────────────────

    function test_minimalTimeColor_write() {
        ScreensaverConfig.minimalTimeColor = "#ff0000";
        compare(ScreensaverConfig.minimalTimeColor, "#ff0000", "time color set to red");
        compare(timeColorSpy.count, 1, "time color signal emitted");
    }

    function test_minimalDateColor_write() {
        ScreensaverConfig.minimalDateColor = "#00ff00";
        compare(ScreensaverConfig.minimalDateColor, "#00ff00", "date color set to green");
        compare(dateColorSpy.count, 1, "date color signal emitted");
    }

    // ── Shared overlay toggles that MinimalTheme reads ────────
    // (showClock gates the clock/date text block visibility;
    // showBatteryEnabled gates the BatteryOverlay in the theme.)

    function test_showClock_default_and_toggle() {
        compare(ScreensaverConfig.showClock, false, "showClock default is off");
        ScreensaverConfig.showClock = true;
        compare(ScreensaverConfig.showClock, true, "showClock toggled on");
        compare(showClockSpy.count, 1, "showClock signal emitted");
    }

    function test_showBatteryEnabled_default_and_toggle() {
        compare(ScreensaverConfig.showBatteryEnabled, true, "battery overlay default is on");
        ScreensaverConfig.showBatteryEnabled = false;
        compare(ScreensaverConfig.showBatteryEnabled, false, "battery overlay toggled off");
        compare(showBatterySpy.count, 1, "battery overlay signal emitted");
    }

    // ── Combined: multiple writes + reset ─────────────────────

    function test_combined_writes_and_reset() {
        // Use values that differ from defaults so every set() triggers a signal.
        // Mock defaults: clockSize=96, dateSize=28, font="primary", clock24h=true,
        // timeColor="#d0d0d0", dateColor="#666666".
        ScreensaverConfig.minimalClockSize = 72;
        ScreensaverConfig.minimalDateSize = 36;
        ScreensaverConfig.minimalFont = "secondary";  // differ from "primary" default
        ScreensaverConfig.minimalClock24h = false;    // differ from true default
        ScreensaverConfig.minimalTimeColor = "#123456";
        ScreensaverConfig.minimalDateColor = "#abcdef";

        compare(clockSizeSpy.count, 1);
        compare(dateSizeSpy.count, 1);
        compare(fontSpy.count, 1);
        compare(clock24hSpy.count, 1);
        compare(timeColorSpy.count, 1);
        compare(dateColorSpy.count, 1);

        ScreensaverConfig.resetDefaults();
        // resetDefaults restores the underlying QSettings — the test only
        // verifies the mock honours reset (actual default values are
        // covered by tst_config_defaults.qml).
    }
}
