// Copyright (c) 2026 madalone. Theme selector component.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import "qrc:/components" as Components
import ScreensaverConfig 1.0

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

    GridLayout {
        id: themeRow
        objectName: "themeRow"
        columns: 3
        rowSpacing: 10
        columnSpacing: 10
        Layout.fillWidth: true
        focus: true
        Accessible.name: "Theme selector"
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: root.navUpTarget
        KeyNavigation.down: root.navDownTarget
        Keys.onLeftPressed: root.settingsPage.cycleOption(["matrix","starfield","minimal","analog","tvstatic"], ScreensaverConfig.theme, function(v){ ScreensaverConfig.theme = v }, -1)
        Keys.onRightPressed: root.settingsPage.cycleOption(["matrix","starfield","minimal","analog","tvstatic"], ScreensaverConfig.theme, function(v){ ScreensaverConfig.theme = v }, 1)
        Repeater {
            model: [
                { name: "matrix", label: "Matrix" },
                { name: "starfield", label: "Starfield" },
                { name: "minimal", label: "Minimal" },
                { name: "analog", label: "Analog" },
                { name: "tvstatic", label: "TV Static" }
            ]
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                radius: 8
                color: ScreensaverConfig.theme === modelData.name ? colors.offwhite : colors.dark
                border { color: colors.medium; width: 1 }
                Text {
                    anchors.centerIn: parent; text: modelData.label
                    color: ScreensaverConfig.theme === modelData.name ? colors.black : colors.offwhite
                    font: fonts.primaryFont(22)
                    elide: Text.ElideRight
                    width: parent.width - 12
                    horizontalAlignment: Text.AlignHCenter
                }
                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: ScreensaverConfig.theme = modelData.name
                }
            }
        }
    }
}
