// Copyright (c) 2024 madalone. Tests for the cycleOption() function.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Config 1.0

TestCase {
    id: testCase
    name: "CycleOption"
    when: windowShown

    // ── Local reimplementation of ChargingScreen.cycleOption ──
    // This isolates the pure logic from the full component tree.
    property var lastSetValue: undefined

    function cycleOption(options, current, setter, delta) {
        for (var i = 0; i < options.length; i++) {
            if (current === options[i]) {
                var next = i + delta;
                if (next >= 0 && next < options.length) {
                    setter(options[next]);
                }
                return;
            }
        }
    }

    function setter(v) { lastSetValue = v; }

    // ── init / cleanup ────────────────────────────────────────
    function init() {
        lastSetValue = undefined;
        Config.resetDefaults();
    }

    // ── Tests ─────────────────────────────────────────────────

    function test_cycle_forward_mid() {
        var options = ["matrix", "starfield", "minimal"];
        cycleOption(options, "matrix", setter, 1);
        compare(lastSetValue, "starfield",
                "cycling forward from first element selects second");
    }

    function test_cycle_backward_mid() {
        var options = ["matrix", "starfield", "minimal"];
        cycleOption(options, "starfield", setter, -1);
        compare(lastSetValue, "matrix",
                "cycling backward from middle selects first");
    }

    function test_cycle_forward_at_end_does_nothing() {
        var options = ["matrix", "starfield", "minimal"];
        cycleOption(options, "minimal", setter, 1);
        compare(lastSetValue, undefined,
                "cycling forward at last element does not call setter");
    }

    function test_cycle_backward_at_start_does_nothing() {
        var options = ["matrix", "starfield", "minimal"];
        cycleOption(options, "matrix", setter, -1);
        compare(lastSetValue, undefined,
                "cycling backward at first element does not call setter");
    }

    function test_cycle_unknown_value_does_nothing() {
        var options = ["matrix", "starfield", "minimal"];
        cycleOption(options, "nonexistent", setter, 1);
        compare(lastSetValue, undefined,
                "cycling with unknown current value does not call setter");
    }

    function test_cycle_single_item_list() {
        var options = ["only"];
        cycleOption(options, "only", setter, 1);
        compare(lastSetValue, undefined,
                "single-item list cannot cycle forward");
        cycleOption(options, "only", setter, -1);
        compare(lastSetValue, undefined,
                "single-item list cannot cycle backward");
    }

    function test_cycle_integrated_theme_forward() {
        // Test via actual Config property writes
        Config.chargingTheme = "matrix";
        var options = ["matrix", "starfield", "minimal"];
        cycleOption(options, Config.chargingTheme,
                    function(v) { Config.chargingTheme = v; }, 1);
        compare(Config.chargingTheme, "starfield",
                "theme cycles from matrix to starfield via Config");
    }

    function test_cycle_integrated_colormode() {
        Config.chargingMatrixColorMode = "blue";
        var options = ["green", "blue", "red", "amber", "white", "purple"];
        cycleOption(options, Config.chargingMatrixColorMode,
                    function(v) { Config.chargingMatrixColorMode = v; }, 1);
        compare(Config.chargingMatrixColorMode, "red",
                "color mode cycles from blue to red via Config");
    }
}
