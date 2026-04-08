// Copyright (c) 2024 madalone. Charging screen settings page.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

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

            ChargingScreenComponents.CommonToggles {
                id: commonToggles
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navDownTarget: themeSelector.firstFocusItem
            }

            ChargingScreenComponents.ThemeSelector {
                id: themeSelector
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navUpTarget: commonToggles.lastFocusItem
                navDownTarget: matrixAppearance.visible ? matrixAppearance.firstFocusItem
                             : starfieldSettings.visible ? starfieldSpeedSlider
                             : generalBehavior.firstFocusItem
            }

            ChargingScreenComponents.MatrixAppearance {
                id: matrixAppearance
                settingsPage: chargingScreenPage
                visible: ScreensaverConfig.theme === "matrix"
                Layout.fillWidth: true
                navUpTarget: themeSelector.lastFocusItem
                navDownTarget: matrixEffects.firstFocusItem
            }

            ChargingScreenComponents.MatrixEffects {
                id: matrixEffects
                settingsPage: chargingScreenPage
                visible: ScreensaverConfig.theme === "matrix"
                Layout.fillWidth: true
                navUpTarget: matrixAppearance.lastFocusItem
                navDownTarget: generalBehavior.firstFocusItem
            }

            // --- Starfield settings (speed + density) ---
            ColumnLayout {
                id: starfieldSettings
                visible: ScreensaverConfig.theme === "starfield"
                Layout.fillWidth: true
                Layout.leftMargin: 10; Layout.rightMargin: 10
                spacing: 20

                Text {
                    Layout.fillWidth: true; color: colors.offwhite
                    text: qsTr("Animation speed"); font: fonts.primaryFont(30)
                }
                Components.Slider {
                    id: starfieldSpeedSlider
                    height: 60; Layout.fillWidth: true
                    from: 10; to: 100; stepSize: 5
                    value: ScreensaverConfig.starfieldSpeed; live: true
                    onMoved: ScreensaverConfig.starfieldSpeed = value
                    onUserInteractionEnded: ScreensaverConfig.starfieldSpeed = value
                    highlight: activeFocus && ui.keyNavigationEnabled
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: themeSelector.lastFocusItem
                    KeyNavigation.down: starfieldDensitySlider
                }

                Text {
                    Layout.fillWidth: true; color: colors.offwhite
                    text: qsTr("Star density"); font: fonts.primaryFont(30)
                }
                Components.Slider {
                    id: starfieldDensitySlider
                    height: 60; Layout.fillWidth: true
                    from: 10; to: 100; stepSize: 5
                    value: ScreensaverConfig.starfieldDensity; live: true
                    onMoved: ScreensaverConfig.starfieldDensity = value
                    onUserInteractionEnded: ScreensaverConfig.starfieldDensity = value
                    highlight: activeFocus && ui.keyNavigationEnabled
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: starfieldSpeedSlider
                    KeyNavigation.down: generalBehavior.firstFocusItem
                }
            }

            ChargingScreenComponents.GeneralBehavior {
                id: generalBehavior
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navUpTarget: matrixEffects.visible ? matrixEffects.lastFocusItem
                           : starfieldSettings.visible ? starfieldDensitySlider
                           : themeSelector.lastFocusItem
            }

            Item { Layout.preferredHeight: 40 }
        }
    }
}
