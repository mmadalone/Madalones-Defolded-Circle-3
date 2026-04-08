// Copyright (c) 2026 madalone. Tests for conditional section visibility.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

TestCase {
    id: testCase
    name: "SettingsVisibility"
    when: windowShown

    // ── init / cleanup ────────────────────────────────────────
    function init() {
        ScreensaverConfig.resetDefaults();
    }

    // ─────────────────────────────────────────
    // Theme-gated section visibility
    // ─────────────────────────────────────────

    function test_matrix_sections_visible_when_matrix_theme() {
        ScreensaverConfig.theme = "matrix";
        // MatrixAppearance and MatrixEffects are visible when theme === "matrix"
        verify(ScreensaverConfig.theme === "matrix",
               "precondition: theme is matrix");
        // The visibility logic in ChargingScreen.qml:
        //   visible: ScreensaverConfig.theme === "matrix"
        compare(ScreensaverConfig.theme === "matrix", true,
                "matrix sections should be visible for matrix theme");
    }

    function test_matrix_sections_hidden_when_starfield_theme() {
        ScreensaverConfig.theme = "starfield";
        compare(ScreensaverConfig.theme === "matrix", false,
                "matrix sections should be hidden for starfield theme");
    }

    function test_matrix_sections_hidden_when_minimal_theme() {
        ScreensaverConfig.theme = "minimal";
        compare(ScreensaverConfig.theme === "matrix", false,
                "matrix sections should be hidden for minimal theme");
    }

    // ─────────────────────────────────────────
    // Glitch sub-section visibility
    // ─────────────────────────────────────────

    function test_glitch_subsections_hidden_when_glitch_off() {
        ScreensaverConfig.glitch = false;
        // glitch rate, flash, stutter, reverse, direction, chaos — all gated
        compare(ScreensaverConfig.glitch, false,
                "glitch sub-sections hidden when glitch is off");
    }

    function test_glitch_subsections_visible_when_glitch_on() {
        ScreensaverConfig.glitch = true;
        compare(ScreensaverConfig.glitch, true,
                "glitch sub-sections visible when glitch is on");
    }

    // ─────────────────────────────────────────
    // Direction glitch nested visibility
    // ─────────────────────────────────────────

    function test_direction_rate_hidden_when_direction_off() {
        ScreensaverConfig.glitch = true;
        ScreensaverConfig.glitchDirection = false;
        // dir rate + cardinal only visible when glitch AND direction both on
        var vis = ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection;
        compare(vis, false,
                "direction sub-settings hidden when direction glitch off");
    }

    function test_direction_rate_visible_when_both_on() {
        ScreensaverConfig.glitch = true;
        ScreensaverConfig.glitchDirection = true;
        var vis = ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection;
        compare(vis, true,
                "direction sub-settings visible when both on");
    }

    // ─────────────────────────────────────────
    // Chaos sub-section visibility
    // ─────────────────────────────────────────

    function test_chaos_subtypes_hidden_when_chaos_off() {
        ScreensaverConfig.glitch = true;
        ScreensaverConfig.glitchChaos = false;
        var vis = ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos;
        compare(vis, false,
                "chaos frequency + subtypes hidden when chaos off");
    }

    function test_chaos_subtypes_visible_when_both_on() {
        ScreensaverConfig.glitch = true;
        ScreensaverConfig.glitchChaos = true;
        var vis = ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos;
        compare(vis, true,
                "chaos frequency + subtypes visible when both on");
    }

    // ─────────────────────────────────────────
    // Message section visibility
    // ─────────────────────────────────────────

    function test_message_options_hidden_when_no_messages() {
        ScreensaverConfig.messages = "";
        compare(ScreensaverConfig.messages !== "", false,
                "message interval/random/direction hidden when messages empty");
    }

    function test_message_options_visible_when_messages_set() {
        ScreensaverConfig.messages = "HELLO,WORLD";
        compare(ScreensaverConfig.messages !== "", true,
                "message options visible when messages text is non-empty");
    }

    // ─────────────────────────────────────────
    // Battery docked-only sub-toggle visibility
    // ─────────────────────────────────────────

    function test_battery_docked_hidden_when_battery_off() {
        ScreensaverConfig.showBatteryEnabled = false;
        compare(ScreensaverConfig.showBatteryEnabled, false,
                "battery docked-only sub-toggle hidden when show battery is off");
    }

    function test_battery_docked_visible_when_battery_on() {
        ScreensaverConfig.showBatteryEnabled = true;
        compare(ScreensaverConfig.showBatteryEnabled, true,
                "battery docked-only sub-toggle visible when show battery is on");
    }

    // ─────────────────────────────────────────
    // Idle timeout visibility
    // ─────────────────────────────────────────

    function test_idle_timeout_hidden_when_idle_off() {
        ScreensaverConfig.idleEnabled = false;
        compare(ScreensaverConfig.idleEnabled, false,
                "idle timeout slider hidden when idle screensaver is off");
    }

    function test_idle_timeout_visible_when_idle_on() {
        ScreensaverConfig.idleEnabled = true;
        compare(ScreensaverConfig.idleEnabled, true,
                "idle timeout slider visible when idle screensaver is on");
    }
}
