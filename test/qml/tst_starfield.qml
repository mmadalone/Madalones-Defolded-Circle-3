// Copyright (c) 2026 madalone. Tests for StarfieldTheme config surface.
// Validates that the StarfieldTheme.qml consumer interface
// (ScreensaverConfig.starfield* properties and theme selection) exposes
// correct defaults, accepts writes, and emits change signals.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

TestCase {
    id: testCase
    name: "StarfieldTheme"
    when: windowShown

    // Signal spies — one per property the theme reads
    SignalSpy { id: themeSpy;         target: ScreensaverConfig; signalName: "themeChanged" }
    SignalSpy { id: speedSpy;         target: ScreensaverConfig; signalName: "starfieldSpeedChanged" }
    SignalSpy { id: densitySpy;       target: ScreensaverConfig; signalName: "starfieldDensityChanged" }
    SignalSpy { id: colorSpy;         target: ScreensaverConfig; signalName: "starfieldColorChanged" }
    SignalSpy { id: starSizeSpy;      target: ScreensaverConfig; signalName: "starfieldStarSizeChanged" }
    SignalSpy { id: trailLengthSpy;   target: ScreensaverConfig; signalName: "starfieldTrailLengthChanged" }

    function init() {
        ScreensaverConfig.resetDefaults();
        themeSpy.clear();
        speedSpy.clear();
        densitySpy.clear();
        colorSpy.clear();
        starSizeSpy.clear();
        trailLengthSpy.clear();
    }

    // ── Theme selection ───────────────────────────────────────

    function test_theme_selection() {
        ScreensaverConfig.theme = "starfield";
        compare(ScreensaverConfig.theme, "starfield", "theme selected as starfield");
        compare(themeSpy.count, 1, "theme change signal emitted once");
    }

    // ── starfieldSpeed ────────────────────────────────────────

    function test_starfieldSpeed_default() {
        compare(ScreensaverConfig.starfieldSpeed, 50, "default speed is 50");
    }

    function test_starfieldSpeed_write() {
        ScreensaverConfig.starfieldSpeed = 80;
        compare(ScreensaverConfig.starfieldSpeed, 80, "speed updated to 80");
        compare(speedSpy.count, 1, "speed signal emitted once");
    }

    function test_starfieldSpeed_no_signal_on_same_value() {
        ScreensaverConfig.starfieldSpeed = 50;  // already default
        compare(speedSpy.count, 0, "no signal when value unchanged");
    }

    // ── starfieldDensity ──────────────────────────────────────

    function test_starfieldDensity_default() {
        compare(ScreensaverConfig.starfieldDensity, 50, "default density is 50");
    }

    function test_starfieldDensity_write() {
        ScreensaverConfig.starfieldDensity = 100;
        compare(ScreensaverConfig.starfieldDensity, 100, "density updated to 100");
        compare(densitySpy.count, 1, "density signal emitted once");
    }

    // ── starfieldColor ────────────────────────────────────────

    function test_starfieldColor_default() {
        compare(ScreensaverConfig.starfieldColor, "#ffffff", "default color is white");
    }

    function test_starfieldColor_solid_hex() {
        ScreensaverConfig.starfieldColor = "#ff0000";
        compare(ScreensaverConfig.starfieldColor, "#ff0000", "color updated to red");
        compare(colorSpy.count, 1, "color signal emitted once");
    }

    function test_starfieldColor_gradient_modes() {
        // StarfieldTheme.qml recognizes these gradient keywords via isGradient()
        var gradients = ["rainbow", "rainbow_gradient", "neon"];
        for (var i = 0; i < gradients.length; ++i) {
            ScreensaverConfig.starfieldColor = gradients[i];
            compare(ScreensaverConfig.starfieldColor, gradients[i],
                    "color accepts gradient keyword " + gradients[i]);
        }
        compare(colorSpy.count, gradients.length,
                "one signal per gradient change");
    }

    // ── starfieldStarSize ─────────────────────────────────────

    function test_starfieldStarSize_default() {
        compare(ScreensaverConfig.starfieldStarSize, 50, "default star size is 50");
    }

    function test_starfieldStarSize_write() {
        ScreensaverConfig.starfieldStarSize = 25;
        compare(ScreensaverConfig.starfieldStarSize, 25, "star size updated to 25");
        compare(starSizeSpy.count, 1, "star size signal emitted once");
    }

    // ── starfieldTrailLength ──────────────────────────────────

    function test_starfieldTrailLength_default() {
        compare(ScreensaverConfig.starfieldTrailLength, 50, "default trail length is 50");
    }

    function test_starfieldTrailLength_write() {
        ScreensaverConfig.starfieldTrailLength = 75;
        compare(ScreensaverConfig.starfieldTrailLength, 75, "trail length updated to 75");
        compare(trailLengthSpy.count, 1, "trail length signal emitted once");
    }

    // ── Cross-property sanity: multiple writes in one session ─

    function test_combined_writes_emit_distinct_signals() {
        ScreensaverConfig.starfieldSpeed = 60;
        ScreensaverConfig.starfieldDensity = 70;
        ScreensaverConfig.starfieldColor = "rainbow";
        ScreensaverConfig.starfieldStarSize = 30;
        ScreensaverConfig.starfieldTrailLength = 40;

        compare(speedSpy.count, 1, "speed signal emitted exactly once");
        compare(densitySpy.count, 1, "density signal emitted exactly once");
        compare(colorSpy.count, 1, "color signal emitted exactly once");
        compare(starSizeSpy.count, 1, "star size signal emitted exactly once");
        compare(trailLengthSpy.count, 1, "trail length signal emitted exactly once");

        // Reset restores defaults and does NOT emit per-property signals
        // (resetDefaults is a bulk operation in the real impl)
        ScreensaverConfig.resetDefaults();
        compare(ScreensaverConfig.starfieldSpeed, 50, "speed restored");
        compare(ScreensaverConfig.starfieldDensity, 50, "density restored");
        compare(ScreensaverConfig.starfieldColor, "#ffffff", "color restored");
        compare(ScreensaverConfig.starfieldStarSize, 50, "star size restored");
        compare(ScreensaverConfig.starfieldTrailLength, 50, "trail length restored");
    }
}
