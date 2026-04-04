// Copyright (c) 2024 madalone. Theme selector component.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: themeRow
    property alias lastFocusItem: themeRow
    property Item navUpTarget
    property Item navDownTarget

    Layout.alignment: Qt.AlignCenter
    Layout.leftMargin: 10; Layout.rightMargin: 10
    spacing: 10

    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Theme"); font: fonts.primaryFont(30)
    }

    RowLayout {
        id: themeRow
        objectName: "themeRow"
        spacing: 10; focus: true
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: root.navUpTarget
        KeyNavigation.down: root.navDownTarget
        Keys.onLeftPressed: root.settingsPage.cycleOption(["matrix","starfield","minimal"], Config.chargingTheme, function(v){ Config.chargingTheme = v }, -1)
        Keys.onRightPressed: root.settingsPage.cycleOption(["matrix","starfield","minimal"], Config.chargingTheme, function(v){ Config.chargingTheme = v }, 1)
        Repeater {
            model: [
                { name: "matrix", label: "Matrix" },
                { name: "starfield", label: "Starfield" },
                { name: "minimal", label: "Minimal" }
            ]
            Rectangle {
                Layout.fillWidth: true; height: 50; radius: 8
                color: Config.chargingTheme === modelData.name ? colors.offwhite : colors.dark
                border { color: colors.medium; width: 1 }
                Text {
                    anchors.centerIn: parent; text: modelData.label
                    color: Config.chargingTheme === modelData.name ? colors.black : colors.offwhite
                    font: fonts.primaryFont(24)
                }
                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: Config.chargingTheme = modelData.name
                }
            }
        }
    }
}
