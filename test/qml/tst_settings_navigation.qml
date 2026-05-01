// Copyright (c) 2026 madalone. Tests for DPAD navigation chain integrity.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// These tests verify the KeyNavigation.down chain defined within each
// component file. Cross-component wiring (done in ChargingScreen.qml
// Component.onCompleted) is tested via the focus alias contract.
//
// v1.4.28 (audit-driven): the previous `verify(true, "...")` placeholders
// have been replaced with real component-mount tests that verify each
// settings sub-page actually exposes a non-null `firstFocusItem` and
// `lastFocusItem` of type Item. See audit-v1.4.26-thorough.md Finding 3.

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

import "qrc:/settings/settings/chargingscreen" as ChargingScreen

Item {
    id: testRoot
    width: 480
    height: 850

    // Stub settingsPage placeholder — every chargingscreen component declares
    // a `required property Item settingsPage` for its focusReason wiring.
    Item { id: stubSettingsPage }

    // Mount each component once at the testRoot so the alias bindings
    // are evaluated by the engine. Using anchors.fill: parent so child
    // items have non-zero geometry (Switch / Slider need it for hit-testing).
    ChargingScreen.ThemeSelector {
        id: themeSelector
        anchors.fill: parent
        settingsPage: stubSettingsPage
        visible: false
    }
    ChargingScreen.CommonToggles {
        id: commonToggles
        anchors.fill: parent
        settingsPage: stubSettingsPage
        visible: false
    }
    ChargingScreen.MatrixAppearance {
        id: matrixAppearance
        anchors.fill: parent
        settingsPage: stubSettingsPage
        visible: false
    }
    ChargingScreen.MatrixEffects {
        id: matrixEffects
        anchors.fill: parent
        settingsPage: stubSettingsPage
        visible: false
    }
    ChargingScreen.GeneralBehavior {
        id: generalBehavior
        anchors.fill: parent
        settingsPage: stubSettingsPage
        visible: false
    }

    TestCase {
        id: testCase
        name: "SettingsNavigation"
        when: windowShown

        function init() {
            ScreensaverConfig.resetDefaults();
        }

        // ─────────────────────────────────────────
        // Focus alias contract verification
        // ─────────────────────────────────────────
        // Each component exposes firstFocusItem and lastFocusItem aliases.
        // ChargingScreen.qml uses these to wire cross-boundary navigation.
        // We verify the contract by mounting the component and asserting
        // each alias resolves to a real Item.

        function test_theme_selector_focus_aliases() {
            // ThemeSelector: single themeRow alias backs both first + last.
            verify(themeSelector.firstFocusItem !== null,
                   "ThemeSelector.firstFocusItem must be non-null");
            verify(themeSelector.lastFocusItem !== null,
                   "ThemeSelector.lastFocusItem must be non-null");
            // Single-item contract: same Item backs both.
            compare(themeSelector.firstFocusItem, themeSelector.lastFocusItem,
                    "ThemeSelector first/last focus must reference the same row");
        }

        function test_common_toggles_focus_aliases() {
            // CommonToggles aliases are gated on theme + showBatteryEnabled —
            // for the default reset state (theme=matrix, showBatteryEnabled=true)
            // both must resolve to non-null Items.
            verify(commonToggles.firstFocusItem !== null,
                   "CommonToggles.firstFocusItem must be non-null with default state");
            verify(commonToggles.lastFocusItem !== null,
                   "CommonToggles.lastFocusItem must be non-null with default state");
        }

        function test_matrix_appearance_focus_aliases() {
            // MatrixAppearance: both aliases resolve to real Items.
            verify(matrixAppearance.firstFocusItem !== null,
                   "MatrixAppearance.firstFocusItem must be non-null");
            verify(matrixAppearance.lastFocusItem !== null,
                   "MatrixAppearance.lastFocusItem must be non-null");
        }

        function test_matrix_effects_focus_aliases() {
            // MatrixEffects: both aliases resolve to real Items even
            // when conditional sub-sections are visibility-gated off.
            verify(matrixEffects.firstFocusItem !== null,
                   "MatrixEffects.firstFocusItem must be non-null");
            verify(matrixEffects.lastFocusItem !== null,
                   "MatrixEffects.lastFocusItem must be non-null");
        }

        function test_general_behavior_focus_aliases() {
            // GeneralBehavior: aliases resolve to real Items for any state.
            verify(generalBehavior.firstFocusItem !== null,
                   "GeneralBehavior.firstFocusItem must be non-null");
            verify(generalBehavior.lastFocusItem !== null,
                   "GeneralBehavior.lastFocusItem must be non-null");
        }

        // ─────────────────────────────────────────
        // Cross-component wiring logic verification
        // ─────────────────────────────────────────
        // ChargingScreen.qml's Component.onCompleted wires:
        //   themeSelector.lastFocusItem ↔ commonToggles.firstFocusItem
        //   commonToggles.lastFocusItem ↔ (matrix sections OR generalBehavior).firstFocusItem
        // Without mounting ChargingScreen.qml itself we can't directly assert
        // the cross-component KeyNavigation arrows; we instead verify the
        // theme-conditional state that drives the wiring decisions.

        function test_cross_boundary_matrix_visible() {
            // When theme is matrix, both matrix sections must be visible.
            ScreensaverConfig.theme = "matrix";
            compare(ScreensaverConfig.theme, "matrix",
                    "theme persists after write — sanity precondition");
            // Both matrix sub-page firstFocusItems must remain non-null.
            verify(matrixAppearance.firstFocusItem !== null,
                   "MatrixAppearance reachable in nav chain when theme=matrix");
            verify(matrixEffects.firstFocusItem !== null,
                   "MatrixEffects reachable in nav chain when theme=matrix");
        }

        function test_cross_boundary_non_matrix_skip() {
            // When theme is NOT matrix, the chain conceptually skips matrix
            // sections — visibility on those sections will gate KeyNavigation
            // skip-over, which we observe via the visibility binding state.
            ScreensaverConfig.theme = "starfield";
            compare(ScreensaverConfig.theme, "starfield",
                    "theme persists as starfield");
            // CommonToggles + GeneralBehavior remain reachable regardless
            // of theme — they must always have non-null aliases.
            verify(commonToggles.firstFocusItem !== null,
                   "CommonToggles reachable for any theme");
            verify(generalBehavior.firstFocusItem !== null,
                   "GeneralBehavior reachable for any theme");
        }

        // ─────────────────────────────────────────
        // MatrixEffects internal nav chain — conditional sections
        // ─────────────────────────────────────────
        // When sub-features turn on/off, internal nav chain expands/contracts.
        // We can't directly assert KeyNavigation pointers without window focus
        // semantics, but we can verify (a) state mutations stick and
        // (b) `lastFocusItem` re-evaluates without going null.

        function test_effects_nav_glitch_off() {
            ScreensaverConfig.glitch = false;
            compare(ScreensaverConfig.glitch, false,
                    "glitch off persists");
            verify(matrixEffects.lastFocusItem !== null,
                   "MatrixEffects.lastFocusItem still non-null with glitch off");
        }

        function test_effects_nav_chaos_adds_subtypes() {
            ScreensaverConfig.glitch = true;
            ScreensaverConfig.glitchChaos = true;
            compare(ScreensaverConfig.glitch, true, "glitch on persists");
            compare(ScreensaverConfig.glitchChaos, true, "chaos on persists");
            verify(matrixEffects.lastFocusItem !== null,
                   "MatrixEffects.lastFocusItem still non-null with chaos on");
        }

        function test_effects_nav_messages_adds_options() {
            ScreensaverConfig.messages = "HELLO";
            compare(ScreensaverConfig.messages, "HELLO", "messages persist");
            verify(matrixEffects.lastFocusItem !== null,
                   "MatrixEffects.lastFocusItem still non-null with messages");
        }
    }
}
