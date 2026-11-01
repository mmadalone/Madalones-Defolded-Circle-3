// Copyright (c) 2026 madalone. Analog theme settings panel.
// Conditionally shown in ChargingScreen settings page when theme === "analog".
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
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

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true

    Layout.fillWidth: true
    spacing: 0

    // ---- Shutdown hands picker ----
    // Controls which hands animate during the native screen-off sequence
    // (only active when Settings → Power saving → Screen off animations
    // is set to "Theme (native)" and the sequence fires). Hardcoded timing
    // and easing; this is the only user-exposed knob.
    Components.SettingRow {
        title: qsTr("Shutdown hands")
        help: qsTr("Which hands animate when the display is about to turn off")
        controlBelow: true

        // one chip per line: two side by side cut "Minute + hour only" short
        Components.ChipPicker {
            id: shutoffHandsRow
            columns: 1
            model: [
                { value: "all",  label: qsTr("All three hands") },
                { value: "main", label: qsTr("Minute + hour only") }
            ]
            currentValue: ScreensaverConfig.analogShutoffHands
            onPicked: ScreensaverConfig.analogShutoffHands = value
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: root.navDownTarget
        }
    }
}
