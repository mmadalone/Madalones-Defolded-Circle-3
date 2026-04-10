// Copyright (c) 2026 madalone. Analog theme settings panel.
// Conditionally shown in ChargingScreen settings page when theme === "analog".
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property Item navUpTarget
    property Item navDownTarget

    property alias firstFocusItem: shutoffHandsRow
    property alias lastFocusItem: shutoffHandsRow

    Layout.fillWidth: true
    Layout.leftMargin: 10; Layout.rightMargin: 10
    spacing: 20

    // ---- Shutdown hands picker ----
    // Controls which hands animate during the native screen-off sequence
    // (only active when Settings → Power saving → Screen off animations
    // is set to "Theme (native)" and the sequence fires). Hardcoded timing
    // and easing; this is the only user-exposed knob.
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Shutdown hands")
        font: fonts.primaryFont(30)
    }
    Text {
        Layout.fillWidth: true; color: colors.medium
        text: qsTr("Which hands animate when the display is about to turn off")
        font: fonts.primaryFont(20)
        wrapMode: Text.WordWrap
    }
    RowLayout {
        id: shutoffHandsRow
        spacing: 10
        Layout.fillWidth: true
        focus: true
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: root.navUpTarget
        KeyNavigation.down: root.navDownTarget
        Keys.onLeftPressed: root.settingsPage.cycleOption(
            ["all", "main"],
            ScreensaverConfig.analogShutoffHands,
            function(v) { ScreensaverConfig.analogShutoffHands = v },
            -1)
        Keys.onRightPressed: root.settingsPage.cycleOption(
            ["all", "main"],
            ScreensaverConfig.analogShutoffHands,
            function(v) { ScreensaverConfig.analogShutoffHands = v },
            1)
        Repeater {
            model: [
                { value: "all",  label: qsTr("All three hands") },
                { value: "main", label: qsTr("Minute + hour only") }
            ]
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                radius: 8
                color: ScreensaverConfig.analogShutoffHands === modelData.value ? colors.offwhite : colors.dark
                border {
                    color: colors.medium
                    width: 1
                }
                Text {
                    anchors.centerIn: parent
                    text: modelData.label
                    color: ScreensaverConfig.analogShutoffHands === modelData.value ? colors.black : colors.offwhite
                    font: fonts.primaryFont(22)
                }
                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: ScreensaverConfig.analogShutoffHands = modelData.value
                }
            }
        }
    }
}
