// Copyright (c) 2026 madalone. Theme selector component.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: themeRow
    property alias lastFocusItem: themeRow
    property Item navUpTarget
    property Item navDownTarget

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true

    spacing: 0

    Components.SettingRow {
        title: qsTr("Theme")
        controlBelow: true

        Components.ChipPicker {
            id: themeRow
            objectName: "themeRow"
            Accessible.name: "Theme selector"
            columns: 3
            model: [
                { value: "matrix",    label: qsTr("Matrix") },
                { value: "starfield", label: qsTr("Starfield") },
                { value: "minimal",   label: qsTr("Minimal") },
                { value: "analog",    label: qsTr("Analog") },
                { value: "tvstatic",  label: qsTr("TV Static") }
            ]
            currentValue: ScreensaverConfig.theme
            onPicked: ScreensaverConfig.theme = value
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: root.navDownTarget
        }
    }
}
