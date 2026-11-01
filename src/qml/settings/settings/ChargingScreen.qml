// Copyright (c) 2026 madalone. Charging screen settings page.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

import "qrc:/settings" as Settings
import "qrc:/settings/settings/chargingscreen" as ChargingScreenComponents
import ScreensaverConfig 1.0

Settings.Page {
    id: chargingScreenPage

    // UC's settings-page contract (settings/Page.qml): the page claims the focus for this control once it
    // owns the input, instead of a control calling forceActiveFocus() from Component.onCompleted. The
    // previous page is destroyed asynchronously and took the focus away again (d-pad dead after re-opening
    // this page, 4 of 7 tries in the desktop simulator). The theme picker is the first control of the page
    // (docs/design-system.md section 6). scrollTarget lets UC's button navigation keep the focused control on
    // screen.
    initialFocusItem: themeSelector.firstFocusItem
    scrollTarget: flickable

    // While the keypad is in use, show the whole setting row of the focused control - its title, help text and
    // control - not only the control. UC's ensureVisible() reveals the direct child of the page layout around the
    // control; here that is a whole section of the page, taller than the screen, so it reveals the control alone and
    // the title above a slider or a chip grid stays cut off when walking up. Each section marks its root with
    // `settingsSection`, and the row is the child of that root on the way up from the focused control. Touch is left
    // alone: scrolling to a title would move a slider under the finger.
    readonly property Item keypadFocusItem: Window.activeFocusItem

    onKeypadFocusItemChanged: {
        if (!ui.keyNavigationActive) {
            return;
        }

        for (let item = keypadFocusItem; item && item.parent; item = item.parent) {
            if (item.parent.settingsSection === true) {
                ensureVisible(item);
                return;
            }
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

        // the sections are the children of this layout, each a column of setting rows (Components.SettingRow) that
        // bring their own padding and dividers
        ColumnLayout {
            id: content
            spacing: 0
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
            // `visible: status === Loader.Ready` hides the item during async
            // load (Qt 5.15 best practice).

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
