// Copyright (c) 2026 madalone. Tests for TvStaticTheme config surface.
// Validates ScreensaverConfig.theme = "tvstatic" selection and all 13
// tvStatic* properties (defaults + write + signal). TvStatic has the
// largest per-theme config surface of any current screensaver theme.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

TestCase {
    id: testCase
    name: "TvStaticTheme"
    when: windowShown

    SignalSpy { id: themeSpy;              target: ScreensaverConfig; signalName: "themeChanged" }
    SignalSpy { id: intensitySpy;          target: ScreensaverConfig; signalName: "tvStaticIntensityChanged" }
    SignalSpy { id: snowSizeSpy;           target: ScreensaverConfig; signalName: "tvStaticSnowSizeChanged" }
    SignalSpy { id: scanlineStrengthSpy;   target: ScreensaverConfig; signalName: "tvStaticScanlineStrengthChanged" }
    SignalSpy { id: scanlineSpeedSpy;      target: ScreensaverConfig; signalName: "tvStaticScanlineSpeedChanged" }
    SignalSpy { id: chromaAmountSpy;       target: ScreensaverConfig; signalName: "tvStaticChromaAmountChanged" }
    SignalSpy { id: trackingEnableSpy;     target: ScreensaverConfig; signalName: "tvStaticTrackingEnableChanged" }
    SignalSpy { id: trackingSpeedSpy;      target: ScreensaverConfig; signalName: "tvStaticTrackingSpeedChanged" }
    SignalSpy { id: flashOnTapSpy;         target: ScreensaverConfig; signalName: "tvStaticFlashOnTapChanged" }
    SignalSpy { id: channelFlashAutoSpy;   target: ScreensaverConfig; signalName: "tvStaticChannelFlashAutoChanged" }
    SignalSpy { id: flashIntervalSpy;      target: ScreensaverConfig; signalName: "tvStaticFlashIntervalChanged" }
    SignalSpy { id: flashDurationSpy;      target: ScreensaverConfig; signalName: "tvStaticFlashDurationChanged" }
    SignalSpy { id: flashBrightnessSpy;    target: ScreensaverConfig; signalName: "tvStaticFlashBrightnessChanged" }
    SignalSpy { id: tintSpy;               target: ScreensaverConfig; signalName: "tvStaticTintChanged" }

    function init() {
        ScreensaverConfig.resetDefaults();
        themeSpy.clear();
        intensitySpy.clear();
        snowSizeSpy.clear();
        scanlineStrengthSpy.clear();
        scanlineSpeedSpy.clear();
        chromaAmountSpy.clear();
        trackingEnableSpy.clear();
        trackingSpeedSpy.clear();
        flashOnTapSpy.clear();
        channelFlashAutoSpy.clear();
        flashIntervalSpy.clear();
        flashDurationSpy.clear();
        flashBrightnessSpy.clear();
        tintSpy.clear();
    }

    // ── Theme selection ───────────────────────────────────────

    function test_theme_selection() {
        ScreensaverConfig.theme = "tvstatic";
        compare(ScreensaverConfig.theme, "tvstatic", "theme selected as tvstatic");
        compare(themeSpy.count, 1, "theme change signal emitted once");
    }

    // ── Noise core: intensity + snow size ─────────────────────

    function test_tvStaticIntensity_defaults_and_write() {
        compare(ScreensaverConfig.tvStaticIntensity, 70, "default intensity is 70");
        ScreensaverConfig.tvStaticIntensity = 90;
        compare(ScreensaverConfig.tvStaticIntensity, 90, "intensity updated");
        compare(intensitySpy.count, 1, "intensity signal emitted once");
    }

    function test_tvStaticSnowSize_defaults_and_write() {
        compare(ScreensaverConfig.tvStaticSnowSize, 2, "default snow size is 2 px per cell");
        ScreensaverConfig.tvStaticSnowSize = 4;
        compare(ScreensaverConfig.tvStaticSnowSize, 4, "snow size updated");
        compare(snowSizeSpy.count, 1, "snow size signal emitted once");
    }

    // ── Scanlines: strength + speed ───────────────────────────

    function test_tvStaticScanlineStrength_defaults_and_write() {
        compare(ScreensaverConfig.tvStaticScanlineStrength, 35, "default scanline strength is 35");
        ScreensaverConfig.tvStaticScanlineStrength = 80;
        compare(ScreensaverConfig.tvStaticScanlineStrength, 80, "scanline strength updated");
        compare(scanlineStrengthSpy.count, 1, "scanline strength signal emitted once");
    }

    function test_tvStaticScanlineSpeed_defaults_and_write() {
        compare(ScreensaverConfig.tvStaticScanlineSpeed, 0, "default scanline speed is 0 (static)");
        ScreensaverConfig.tvStaticScanlineSpeed = 50;
        compare(ScreensaverConfig.tvStaticScanlineSpeed, 50, "scanline speed updated");
        compare(scanlineSpeedSpy.count, 1, "scanline speed signal emitted once");
    }

    // ── Chroma + tracking ─────────────────────────────────────

    function test_tvStaticChromaAmount_defaults_and_write() {
        compare(ScreensaverConfig.tvStaticChromaAmount, 25, "default chroma amount is 25");
        ScreensaverConfig.tvStaticChromaAmount = 60;
        compare(ScreensaverConfig.tvStaticChromaAmount, 60, "chroma updated");
        compare(chromaAmountSpy.count, 1, "chroma signal emitted once");
    }

    function test_tvStaticTrackingEnable_defaults_and_toggle() {
        compare(ScreensaverConfig.tvStaticTrackingEnable, true, "tracking enabled by default");
        ScreensaverConfig.tvStaticTrackingEnable = false;
        compare(ScreensaverConfig.tvStaticTrackingEnable, false, "tracking toggled off");
        compare(trackingEnableSpy.count, 1, "tracking enable signal emitted once");
    }

    function test_tvStaticTrackingSpeed_defaults_and_write() {
        compare(ScreensaverConfig.tvStaticTrackingSpeed, 40, "default tracking speed is 40");
        ScreensaverConfig.tvStaticTrackingSpeed = 100;
        compare(ScreensaverConfig.tvStaticTrackingSpeed, 100, "tracking speed updated");
        compare(trackingSpeedSpy.count, 1, "tracking speed signal emitted once");
    }

    // ── Flash system (channel-change effect) ──────────────────

    function test_tvStaticFlashOnTap_defaults_and_toggle() {
        compare(ScreensaverConfig.tvStaticFlashOnTap, true, "flash on tap enabled by default");
        ScreensaverConfig.tvStaticFlashOnTap = false;
        compare(ScreensaverConfig.tvStaticFlashOnTap, false, "flash on tap toggled off");
        compare(flashOnTapSpy.count, 1, "flash on tap signal emitted once");
    }

    function test_tvStaticChannelFlashAuto_defaults_and_toggle() {
        compare(ScreensaverConfig.tvStaticChannelFlashAuto, true,
                "auto channel flash enabled by default");
        ScreensaverConfig.tvStaticChannelFlashAuto = false;
        compare(ScreensaverConfig.tvStaticChannelFlashAuto, false,
                "auto channel flash toggled off");
        compare(channelFlashAutoSpy.count, 1, "auto channel flash signal emitted once");
    }

    function test_tvStaticFlashInterval_defaults_and_write() {
        compare(ScreensaverConfig.tvStaticFlashInterval, 20,
                "default flash interval is 20 seconds");
        ScreensaverConfig.tvStaticFlashInterval = 45;
        compare(ScreensaverConfig.tvStaticFlashInterval, 45, "flash interval updated");
        compare(flashIntervalSpy.count, 1, "flash interval signal emitted once");
    }

    function test_tvStaticFlashDuration_defaults_and_write() {
        compare(ScreensaverConfig.tvStaticFlashDuration, 400,
                "default flash duration is 400 ms");
        ScreensaverConfig.tvStaticFlashDuration = 800;
        compare(ScreensaverConfig.tvStaticFlashDuration, 800, "flash duration updated");
        compare(flashDurationSpy.count, 1, "flash duration signal emitted once");
    }

    function test_tvStaticFlashBrightness_defaults_and_write() {
        compare(ScreensaverConfig.tvStaticFlashBrightness, 100,
                "default flash brightness is 100");
        ScreensaverConfig.tvStaticFlashBrightness = 50;
        compare(ScreensaverConfig.tvStaticFlashBrightness, 50, "flash brightness updated");
        compare(flashBrightnessSpy.count, 1, "flash brightness signal emitted once");
    }

    // ── Tint ──────────────────────────────────────────────────

    function test_tvStaticTint_default_and_write() {
        compare(ScreensaverConfig.tvStaticTint, "#ffffff", "default tint is white");
        ScreensaverConfig.tvStaticTint = "#00ff00";
        compare(ScreensaverConfig.tvStaticTint, "#00ff00", "tint updated to green");
        compare(tintSpy.count, 1, "tint signal emitted once");
    }

    // ── Combined: full config sweep with signal discipline ────

    function test_combined_writes_emit_exactly_once() {
        ScreensaverConfig.tvStaticIntensity = 99;
        ScreensaverConfig.tvStaticSnowSize = 5;
        ScreensaverConfig.tvStaticScanlineStrength = 60;
        ScreensaverConfig.tvStaticScanlineSpeed = 20;
        ScreensaverConfig.tvStaticChromaAmount = 45;
        ScreensaverConfig.tvStaticTrackingEnable = false;
        ScreensaverConfig.tvStaticTrackingSpeed = 75;
        ScreensaverConfig.tvStaticFlashOnTap = false;
        ScreensaverConfig.tvStaticChannelFlashAuto = false;
        ScreensaverConfig.tvStaticFlashInterval = 30;
        ScreensaverConfig.tvStaticFlashDuration = 600;
        ScreensaverConfig.tvStaticFlashBrightness = 75;
        ScreensaverConfig.tvStaticTint = "#ff00ff";

        compare(intensitySpy.count, 1);
        compare(snowSizeSpy.count, 1);
        compare(scanlineStrengthSpy.count, 1);
        compare(scanlineSpeedSpy.count, 1);
        compare(chromaAmountSpy.count, 1);
        compare(trackingEnableSpy.count, 1);
        compare(trackingSpeedSpy.count, 1);
        compare(flashOnTapSpy.count, 1);
        compare(channelFlashAutoSpy.count, 1);
        compare(flashIntervalSpy.count, 1);
        compare(flashDurationSpy.count, 1);
        compare(flashBrightnessSpy.count, 1);
        compare(tintSpy.count, 1);
    }
}
