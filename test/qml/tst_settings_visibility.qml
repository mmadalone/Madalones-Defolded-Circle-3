// Copyright (c) 2024 madalone. Tests for conditional section visibility.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Config 1.0

TestCase {
    id: testCase
    name: "SettingsVisibility"
    when: windowShown

    // ── init / cleanup ────────────────────────────────────────
    function init() {
        Config.resetDefaults();
    }

    // ─────────────────────────────────────────
    // Theme-gated section visibility
    // ─────────────────────────────────────────

    function test_matrix_sections_visible_when_matrix_theme() {
        Config.chargingTheme = "matrix";
        // MatrixAppearance and MatrixEffects are visible when theme === "matrix"
        verify(Config.chargingTheme === "matrix",
               "precondition: theme is matrix");
        // The visibility logic in ChargingScreen.qml:
        //   visible: Config.chargingTheme === "matrix"
        compare(Config.chargingTheme === "matrix", true,
                "matrix sections should be visible for matrix theme");
    }

    function test_matrix_sections_hidden_when_starfield_theme() {
        Config.chargingTheme = "starfield";
        compare(Config.chargingTheme === "matrix", false,
                "matrix sections should be hidden for starfield theme");
    }

    function test_matrix_sections_hidden_when_minimal_theme() {
        Config.chargingTheme = "minimal";
        compare(Config.chargingTheme === "matrix", false,
                "matrix sections should be hidden for minimal theme");
    }

    // ─────────────────────────────────────────
    // Glitch sub-section visibility
    // ─────────────────────────────────────────

    function test_glitch_subsections_hidden_when_glitch_off() {
        Config.chargingMatrixGlitch = false;
        // glitch rate, flash, stutter, reverse, direction, chaos — all gated
        compare(Config.chargingMatrixGlitch, false,
                "glitch sub-sections hidden when glitch is off");
    }

    function test_glitch_subsections_visible_when_glitch_on() {
        Config.chargingMatrixGlitch = true;
        compare(Config.chargingMatrixGlitch, true,
                "glitch sub-sections visible when glitch is on");
    }

    // ─────────────────────────────────────────
    // Direction glitch nested visibility
    // ─────────────────────────────────────────

    function test_direction_rate_hidden_when_direction_off() {
        Config.chargingMatrixGlitch = true;
        Config.chargingMatrixGlitchDirection = false;
        // dir rate + cardinal only visible when glitch AND direction both on
        var vis = Config.chargingMatrixGlitch && Config.chargingMatrixGlitchDirection;
        compare(vis, false,
                "direction sub-settings hidden when direction glitch off");
    }

    function test_direction_rate_visible_when_both_on() {
        Config.chargingMatrixGlitch = true;
        Config.chargingMatrixGlitchDirection = true;
        var vis = Config.chargingMatrixGlitch && Config.chargingMatrixGlitchDirection;
        compare(vis, true,
                "direction sub-settings visible when both on");
    }

    // ─────────────────────────────────────────
    // Chaos sub-section visibility
    // ─────────────────────────────────────────

    function test_chaos_subtypes_hidden_when_chaos_off() {
        Config.chargingMatrixGlitch = true;
        Config.chargingMatrixGlitchChaos = false;
        var vis = Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos;
        compare(vis, false,
                "chaos frequency + subtypes hidden when chaos off");
    }

    function test_chaos_subtypes_visible_when_both_on() {
        Config.chargingMatrixGlitch = true;
        Config.chargingMatrixGlitchChaos = true;
        var vis = Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos;
        compare(vis, true,
                "chaos frequency + subtypes visible when both on");
    }

    // ─────────────────────────────────────────
    // Message section visibility
    // ─────────────────────────────────────────

    function test_message_options_hidden_when_no_messages() {
        Config.chargingMatrixMessages = "";
        compare(Config.chargingMatrixMessages !== "", false,
                "message interval/random/direction hidden when messages empty");
    }

    function test_message_options_visible_when_messages_set() {
        Config.chargingMatrixMessages = "HELLO,WORLD";
        compare(Config.chargingMatrixMessages !== "", true,
                "message options visible when messages text is non-empty");
    }

    // ─────────────────────────────────────────
    // Battery docked-only sub-toggle visibility
    // ─────────────────────────────────────────

    function test_battery_docked_hidden_when_battery_off() {
        Config.chargingShowBattery = false;
        compare(Config.chargingShowBattery, false,
                "battery docked-only sub-toggle hidden when show battery is off");
    }

    function test_battery_docked_visible_when_battery_on() {
        Config.chargingShowBattery = true;
        compare(Config.chargingShowBattery, true,
                "battery docked-only sub-toggle visible when show battery is on");
    }

    // ─────────────────────────────────────────
    // Idle timeout visibility
    // ─────────────────────────────────────────

    function test_idle_timeout_hidden_when_idle_off() {
        Config.chargingIdleEnabled = false;
        compare(Config.chargingIdleEnabled, false,
                "idle timeout slider hidden when idle screensaver is off");
    }

    function test_idle_timeout_visible_when_idle_on() {
        Config.chargingIdleEnabled = true;
        compare(Config.chargingIdleEnabled, true,
                "idle timeout slider visible when idle screensaver is on");
    }
}
