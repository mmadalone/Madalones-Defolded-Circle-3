// Copyright (c) 2026 madalone. Charging screen settings page.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components
import "qrc:/settings/settings/chargingscreen" as ChargingScreenComponents
import ScreensaverConfig 1.0

Settings.Page {
    id: chargingScreenPage

    // Cycle a selector's value left/right via DPAD
    function cycleOption(options, current, setter, delta) {
        for (var i = 0; i < options.length; i++) {
            if (current === options[i]) {
                var next = i + delta;
                if (next >= 0 && next < options.length) {
                    setter(options[next]);
                    Haptic.play(Haptic.Click);
                }
                return;
            }
        }
    }

    // Scroll to focused item when navigating with DPAD
    function ensureVisible(item) {
        if (!item) return;
        var yy = item.mapToItem(content, 0, 0).y;
        if (yy < flickable.contentY) {
            flickable.contentY = yy - 20;
        } else if (yy + item.height > flickable.contentY + flickable.height) {
            flickable.contentY = yy + item.height - flickable.height + 20;
        }
    }

    // --- Theme sub-page Components (instantiated on-demand by the Loaders
    // below). Using `sourceComponent:` + inline Component instead of `source:`
    // because every sub-page declares `required property Item settingsPage`
    // which Qt 5.15 enforces at instantiation time -- `source:` + `onLoaded:`
    // sets the property too late and leaves the Loader.item uninstantiated.
    // Declaring the Component here lets us pass settingsPage + navUpTarget
    // + navDownTarget declaratively at construction time. ---

    Component {
        id: matrixAppearanceComponent
        ChargingScreenComponents.MatrixAppearance {
            settingsPage: chargingScreenPage
            Layout.fillWidth: true
            navUpTarget: commonToggles.lastFocusItem
            navDownTarget: matrixEffectsLoader.item
                         ? matrixEffectsLoader.item.firstFocusItem
                         : generalBehavior.firstFocusItem
        }
    }

    Component {
        id: matrixEffectsComponent
        ChargingScreenComponents.MatrixEffects {
            settingsPage: chargingScreenPage
            Layout.fillWidth: true
            navUpTarget: matrixAppearanceLoader.item
                       ? matrixAppearanceLoader.item.lastFocusItem
                       : commonToggles.lastFocusItem
            navDownTarget: generalBehavior.firstFocusItem
        }
    }

    Component {
        id: starfieldSettingsComponent
        ChargingScreenComponents.StarfieldSettings {
            settingsPage: chargingScreenPage
            Layout.fillWidth: true
            navUpTarget: commonToggles.lastFocusItem
            navDownTarget: generalBehavior.firstFocusItem
        }
    }

    Component {
        id: minimalSettingsComponent
        ChargingScreenComponents.MinimalSettings {
            settingsPage: chargingScreenPage
            Layout.fillWidth: true
            navUpTarget: commonToggles.lastFocusItem
            navDownTarget: generalBehavior.firstFocusItem
        }
    }

    Component {
        id: tvStaticSettingsComponent
        ChargingScreenComponents.TvStaticSettings {
            settingsPage: chargingScreenPage
            Layout.fillWidth: true
            navUpTarget: commonToggles.lastFocusItem
            navDownTarget: generalBehavior.firstFocusItem
        }
    }

    Component {
        id: analogSettingsComponent
        ChargingScreenComponents.AnalogSettings {
            settingsPage: chargingScreenPage
            Layout.fillWidth: true
            navUpTarget: commonToggles.lastFocusItem
            navDownTarget: generalBehavior.firstFocusItem
        }
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        ColumnLayout {
            id: content
            spacing: 20
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            ChargingScreenComponents.ThemeSelector {
                id: themeSelector
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navDownTarget: commonToggles.firstFocusItem
            }

            ChargingScreenComponents.CommonToggles {
                id: commonToggles
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navUpTarget: themeSelector.lastFocusItem
                navDownTarget: matrixAppearanceLoader.item ? matrixAppearanceLoader.item.firstFocusItem
                             : starfieldLoader.item        ? starfieldLoader.item.firstFocusItem
                             : minimalLoader.item          ? minimalLoader.item.firstFocusItem
                             : tvStaticLoader.item         ? tvStaticLoader.item.firstFocusItem
                             : analogLoader.item           ? analogLoader.item.firstFocusItem
                             : generalBehavior.firstFocusItem
            }

            // --- Theme-specific settings sub-pages (deferred via Loader) ---
            // Each Loader instantiates its sub-page only when the corresponding
            // theme is active. `asynchronous: true` spreads the build across
            // multiple frames on theme switch so there's no hard stall.
            // `visible: status === Loader.Ready` both hides the item during
            // async load (Qt 5.15 best practice) AND excludes inactive Loaders
            // from the parent ColumnLayout's `spacing: 20` so there are no
            // dead gaps between siblings.

            Loader {
                id: matrixAppearanceLoader
                Layout.fillWidth: true
                active: ScreensaverConfig.theme === "matrix"
                asynchronous: true
                visible: status === Loader.Ready
                sourceComponent: matrixAppearanceComponent
            }

            Loader {
                id: matrixEffectsLoader
                Layout.fillWidth: true
                active: ScreensaverConfig.theme === "matrix"
                asynchronous: true
                visible: status === Loader.Ready
                sourceComponent: matrixEffectsComponent
            }

            Loader {
                id: starfieldLoader
                Layout.fillWidth: true
                active: ScreensaverConfig.theme === "starfield"
                asynchronous: true
                visible: status === Loader.Ready
                sourceComponent: starfieldSettingsComponent
            }

            Loader {
                id: minimalLoader
                Layout.fillWidth: true
                active: ScreensaverConfig.theme === "minimal"
                asynchronous: true
                visible: status === Loader.Ready
                sourceComponent: minimalSettingsComponent
            }

            Loader {
                id: tvStaticLoader
                Layout.fillWidth: true
                active: ScreensaverConfig.theme === "tvstatic"
                asynchronous: true
                visible: status === Loader.Ready
                sourceComponent: tvStaticSettingsComponent
            }

            Loader {
                id: analogLoader
                Layout.fillWidth: true
                active: ScreensaverConfig.theme === "analog"
                asynchronous: true
                visible: status === Loader.Ready
                sourceComponent: analogSettingsComponent
            }

            ChargingScreenComponents.GeneralBehavior {
                id: generalBehavior
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navUpTarget: matrixEffectsLoader.item ? matrixEffectsLoader.item.lastFocusItem
                           : starfieldLoader.item     ? starfieldLoader.item.lastFocusItem
                           : minimalLoader.item       ? minimalLoader.item.lastFocusItem
                           : tvStaticLoader.item      ? tvStaticLoader.item.lastFocusItem
                           : analogLoader.item        ? analogLoader.item.lastFocusItem
                           : commonToggles.lastFocusItem
            }

            Item { Layout.preferredHeight: 40 }
        }
    }
}
