// Copyright (c) 2024 madalone. Charging screen settings page.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/settings" as Settings
import "qrc:/settings/settings/chargingscreen" as ChargingScreenComponents

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
                navDownTarget: matrixAppearance.visible ? matrixAppearance.firstFocusItem : generalBehavior.firstFocusItem
            }

            ChargingScreenComponents.MatrixAppearance {
                id: matrixAppearance
                settingsPage: chargingScreenPage
                visible: Config.chargingTheme === "matrix"
                Layout.fillWidth: true
                navUpTarget: themeSelector.lastFocusItem
                navDownTarget: matrixEffects.firstFocusItem
            }

            ChargingScreenComponents.MatrixEffects {
                id: matrixEffects
                settingsPage: chargingScreenPage
                visible: Config.chargingTheme === "matrix"
                Layout.fillWidth: true
                navUpTarget: matrixAppearance.lastFocusItem
                navDownTarget: generalBehavior.firstFocusItem
            }

            ChargingScreenComponents.GeneralBehavior {
                id: generalBehavior
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navUpTarget: matrixEffects.visible ? matrixEffects.lastFocusItem : themeSelector.lastFocusItem
            }

            Item { Layout.preferredHeight: 40 }
        }
    }
}
