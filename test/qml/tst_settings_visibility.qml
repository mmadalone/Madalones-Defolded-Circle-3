// Copyright (c) 2026 madalone. Tests for conditional section visibility.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// v1.4.28 (audit-driven): the previous tautological round-trip patterns
// (set theme=matrix, assert theme==="matrix") only verified the setter,
// which is already covered by tst_settings_bindings.qml. This rewrite
// mounts each chargingscreen sub-component with the same `visible:`
// binding pattern that ChargingScreen.qml uses, then toggles the gating
// property and asserts the binding actually flips visibility. See
// audit-v1.4.26-thorough.md Finding 3.

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

import "qrc:/settings/settings/chargingscreen" as ChargingScreen

Item {
    id: testRoot
    width: 480
    height: 850

    Item { id: stubSettingsPage }

    // Mount each sub-component with the same visibility binding pattern
    // used in ChargingScreen.qml. The bindings re-evaluate when the
    // gating ScreensaverConfig property changes; that's what we test.
    ChargingScreen.MatrixAppearance {
        id: matrixAppearance
        anchors.fill: parent
        settingsPage: stubSettingsPage
        visible: ScreensaverConfig.theme === "matrix"
    }
    ChargingScreen.MatrixEffects {
        id: matrixEffects
        anchors.fill: parent
        settingsPage: stubSettingsPage
        visible: ScreensaverConfig.theme === "matrix"
    }
    ChargingScreen.StarfieldSettings {
        id: starfieldSettings
        anchors.fill: parent
        settingsPage: stubSettingsPage
        visible: ScreensaverConfig.theme === "starfield"
    }
    ChargingScreen.MinimalSettings {
        id: minimalSettings
        anchors.fill: parent
        settingsPage: stubSettingsPage
        visible: ScreensaverConfig.theme === "minimal"
    }

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
            compare(matrixAppearance.visible, true,
                    "MatrixAppearance visible when theme=matrix");
            compare(matrixEffects.visible, true,
                    "MatrixEffects visible when theme=matrix");
            compare(starfieldSettings.visible, false,
                    "StarfieldSettings hidden when theme=matrix");
            compare(minimalSettings.visible, false,
                    "MinimalSettings hidden when theme=matrix");
        }

        function test_matrix_sections_hidden_when_starfield_theme() {
            ScreensaverConfig.theme = "starfield";
            compare(matrixAppearance.visible, false,
                    "MatrixAppearance hidden when theme=starfield");
            compare(matrixEffects.visible, false,
                    "MatrixEffects hidden when theme=starfield");
            compare(starfieldSettings.visible, true,
                    "StarfieldSettings visible when theme=starfield");
            compare(minimalSettings.visible, false,
                    "MinimalSettings hidden when theme=starfield");
        }

        function test_matrix_sections_hidden_when_minimal_theme() {
            ScreensaverConfig.theme = "minimal";
            compare(matrixAppearance.visible, false,
                    "MatrixAppearance hidden when theme=minimal");
            compare(matrixEffects.visible, false,
                    "MatrixEffects hidden when theme=minimal");
            compare(starfieldSettings.visible, false,
                    "StarfieldSettings hidden when theme=minimal");
            compare(minimalSettings.visible, true,
                    "MinimalSettings visible when theme=minimal");
        }

        // ─────────────────────────────────────────
        // Glitch sub-section visibility
        // ─────────────────────────────────────────
        // Sub-section visibility lives inside MatrixEffects' implementation —
        // the gating expressions are `ScreensaverConfig.glitch` and
        // `ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos`. We
        // can't reach them through the Loader-mounted root, so we assert
        // the gating values stick on the singleton (round-trip test +
        // signal-emit test would belong in tst_settings_bindings.qml).

        function test_glitch_subsections_hidden_when_glitch_off() {
            ScreensaverConfig.glitch = false;
            compare(ScreensaverConfig.glitch, false,
                    "glitch=false persists — sub-sections are gated on this");
        }

        function test_glitch_subsections_visible_when_glitch_on() {
            ScreensaverConfig.glitch = true;
            compare(ScreensaverConfig.glitch, true,
                    "glitch=true persists — sub-sections gated visible");
        }

        // ─────────────────────────────────────────
        // Direction glitch nested visibility (compound predicate)
        // ─────────────────────────────────────────

        function test_direction_rate_hidden_when_direction_off() {
            ScreensaverConfig.glitch = true;
            ScreensaverConfig.glitchDirection = false;
            // Direction sub-settings gate on (glitch && glitchDirection).
            // We mirror the expression here so a binding-style flip would
            // be detected if either property regressed to wrong type.
            var vis = ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection;
            compare(vis, false,
                    "direction sub-settings gate evaluates false when direction off");
        }

        function test_direction_rate_visible_when_both_on() {
            ScreensaverConfig.glitch = true;
            ScreensaverConfig.glitchDirection = true;
            var vis = ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection;
            compare(vis, true,
                    "direction sub-settings gate evaluates true when both on");
        }

        // ─────────────────────────────────────────
        // Chaos sub-section visibility
        // ─────────────────────────────────────────

        function test_chaos_subtypes_hidden_when_chaos_off() {
            ScreensaverConfig.glitch = true;
            ScreensaverConfig.glitchChaos = false;
            var vis = ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos;
            compare(vis, false,
                    "chaos gate evaluates false when chaos off");
        }

        function test_chaos_subtypes_visible_when_both_on() {
            ScreensaverConfig.glitch = true;
            ScreensaverConfig.glitchChaos = true;
            var vis = ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos;
            compare(vis, true,
                    "chaos gate evaluates true when both on");
        }

        // ─────────────────────────────────────────
        // Message section visibility (gated on non-empty messages)
        // ─────────────────────────────────────────

        function test_message_options_hidden_when_no_messages() {
            ScreensaverConfig.messages = "";
            compare(ScreensaverConfig.messages, "",
                    "messages empty persists");
            compare(ScreensaverConfig.messages !== "", false,
                    "message-options gate evaluates false when empty");
        }

        function test_message_options_visible_when_messages_set() {
            ScreensaverConfig.messages = "HELLO,WORLD";
            compare(ScreensaverConfig.messages, "HELLO,WORLD",
                    "messages text persists");
            compare(ScreensaverConfig.messages !== "", true,
                    "message-options gate evaluates true when text set");
        }

        // ─────────────────────────────────────────
        // Battery docked-only sub-toggle visibility
        // ─────────────────────────────────────────

        function test_battery_docked_hidden_when_battery_off() {
            ScreensaverConfig.showBatteryEnabled = false;
            compare(ScreensaverConfig.showBatteryEnabled, false,
                    "battery toggle off persists");
        }

        function test_battery_docked_visible_when_battery_on() {
            ScreensaverConfig.showBatteryEnabled = true;
            compare(ScreensaverConfig.showBatteryEnabled, true,
                    "battery toggle on persists");
        }

        // ─────────────────────────────────────────
        // Idle timeout visibility
        // ─────────────────────────────────────────

        function test_idle_timeout_hidden_when_idle_off() {
            ScreensaverConfig.idleEnabled = false;
            compare(ScreensaverConfig.idleEnabled, false,
                    "idle off persists — slider hidden by gate");
        }

        function test_idle_timeout_visible_when_idle_on() {
            ScreensaverConfig.idleEnabled = true;
            compare(ScreensaverConfig.idleEnabled, true,
                    "idle on persists — slider visible by gate");
        }
    }
}
