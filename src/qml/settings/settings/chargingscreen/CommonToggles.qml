// Copyright (c) 2024 madalone. Common toggles component (clock, battery).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property Item firstFocusItem: ScreensaverConfig.theme === "minimal" ? showBatterySwitch : showClockSwitch
    property Item lastFocusItem: ScreensaverConfig.showBatteryEnabled ? batteryDockedSwitch : showBatterySwitch
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // Leading separator
    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 2. SHOW CLOCK (hidden for Minimal — clock is always on)
    ColumnLayout {
        visible: ScreensaverConfig.theme !== "minimal"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; wrapMode: Text.WordWrap; color: colors.offwhite
                text: qsTr("Show clock"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: showClockSwitch
                objectName: "showClockSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.showClock
                trigger: function() { ScreensaverConfig.showClock = !ScreensaverConfig.showClock; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Show clock"
                Component.onCompleted: showClockSwitch.forceActiveFocus()
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: showBatterySwitch
            }
        }
    }

    Rectangle { visible: ScreensaverConfig.theme !== "minimal"; Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 3. SHOW BATTERY
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; wrapMode: Text.WordWrap; color: colors.offwhite
                text: qsTr("Show battery"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: showBatterySwitch
                objectName: "showBatterySwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.showBatteryEnabled
                trigger: function() { ScreensaverConfig.showBatteryEnabled = !ScreensaverConfig.showBatteryEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Show battery"
                KeyNavigation.up: ScreensaverConfig.theme !== "minimal" ? showClockSwitch : root.navUpTarget
                KeyNavigation.down: batteryDockedSwitch
            }
        }
    }

    // 3b. BATTERY DOCKED ONLY (visible when Show battery is on)
    ColumnLayout {
        visible: ScreensaverConfig.showBatteryEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.light
                text: qsTr("Charging only"); font: fonts.primaryFont(26)
            }
            Components.Switch {
                id: batteryDockedSwitch
                objectName: "batteryDockedSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.batteryDockedOnly
                trigger: function() { ScreensaverConfig.batteryDockedOnly = !ScreensaverConfig.batteryDockedOnly; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Charging only"
                KeyNavigation.up: showBatterySwitch
                KeyNavigation.down: root.navDownTarget
            }
        }
    }
}
