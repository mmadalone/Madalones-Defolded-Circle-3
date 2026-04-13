// Copyright (c) 2026 madalone. Tests for AnalogTheme config surface.
// Validates ScreensaverConfig.theme = "analog" selection, the single
// analog-specific property (analogShutoffHands), and the shared
// overlay toggles the theme consumes.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

TestCase {
    id: testCase
    name: "AnalogTheme"
    when: windowShown

    SignalSpy { id: themeSpy;               target: ScreensaverConfig; signalName: "themeChanged" }
    SignalSpy { id: shutoffHandsSpy;        target: ScreensaverConfig; signalName: "analogShutoffHandsChanged" }
    SignalSpy { id: showClockSpy;           target: ScreensaverConfig; signalName: "showClockChanged" }
    SignalSpy { id: showBatterySpy;         target: ScreensaverConfig; signalName: "showBatteryEnabledChanged" }

    function init() {
        ScreensaverConfig.resetDefaults();
        themeSpy.clear();
        shutoffHandsSpy.clear();
        showClockSpy.clear();
        showBatterySpy.clear();
    }

    // ── Theme selection ───────────────────────────────────────

    function test_theme_selection() {
        ScreensaverConfig.theme = "analog";
        compare(ScreensaverConfig.theme, "analog", "theme selected as analog");
        compare(themeSpy.count, 1, "theme change signal emitted once");
    }

    // ── analogShutoffHands ────────────────────────────────────
    // The native screen-off animation in AnalogTheme.qml (sweep/fall
    // sequence on the hour/minute/second hands) is driven by this
    // property. Values: "all", "second", "none".

    function test_analogShutoffHands_default() {
        compare(ScreensaverConfig.analogShutoffHands, "all",
                "default shutoff mode animates all hands");
    }

    function test_analogShutoffHands_valid_values() {
        var modes = ["all", "second", "none"];
        for (var i = 0; i < modes.length; ++i) {
            ScreensaverConfig.analogShutoffHands = modes[i];
            compare(ScreensaverConfig.analogShutoffHands, modes[i],
                    "shutoff mode accepts " + modes[i]);
        }
        compare(shutoffHandsSpy.count, modes.length - 1,
                "signal fires for every unique value change (skipping initial 'all' write)");
    }

    function test_analogShutoffHands_no_signal_on_same_value() {
        ScreensaverConfig.analogShutoffHands = "all";  // already default
        compare(shutoffHandsSpy.count, 0, "no signal when value unchanged");
    }

    // ── Shared overlay toggles ────────────────────────────────
    // AnalogTheme.qml doesn't render a digital clock overlay (it IS the
    // clock), but it DOES respect the BatteryOverlay toggle.

    function test_showBatteryEnabled_toggle() {
        compare(ScreensaverConfig.showBatteryEnabled, true,
                "battery overlay on by default");
        ScreensaverConfig.showBatteryEnabled = false;
        compare(ScreensaverConfig.showBatteryEnabled, false, "battery overlay off");
        compare(showBatterySpy.count, 1, "battery overlay signal emitted");
    }

    // ── Combined: analog as the default-after-switch-from-matrix ─

    function test_analog_preserves_overlay_state_on_switch() {
        // Switching themes should NOT mutate shared overlay properties
        ScreensaverConfig.showBatteryEnabled = false;
        ScreensaverConfig.theme = "analog";
        compare(ScreensaverConfig.showBatteryEnabled, false,
                "battery toggle state preserved when switching to analog");
    }
}
