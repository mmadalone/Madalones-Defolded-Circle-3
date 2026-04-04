// Copyright (c) 2024 madalone. Common toggles component (clock, battery).
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

    property alias firstFocusItem: showClockSwitch
    property alias lastFocusItem: batteryDockedSwitch
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // Leading separator
    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 2. SHOW CLOCK
    ColumnLayout {
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
                checked: Config.chargingShowClock
                trigger: function() { Config.chargingShowClock = !Config.chargingShowClock; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Show clock"
                Component.onCompleted: showClockSwitch.forceActiveFocus()
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: showBatterySwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

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
                checked: Config.chargingShowBattery
                trigger: function() { Config.chargingShowBattery = !Config.chargingShowBattery; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Show battery"
                KeyNavigation.up: showClockSwitch
                KeyNavigation.down: batteryDockedSwitch
            }
        }
    }

    // 3b. BATTERY DOCKED ONLY (visible when Show battery is on)
    ColumnLayout {
        visible: Config.chargingShowBattery
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
                checked: Config.chargingBatteryDockedOnly
                trigger: function() { Config.chargingBatteryDockedOnly = !Config.chargingBatteryDockedOnly; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Charging only"
                KeyNavigation.up: showBatterySwitch
                KeyNavigation.down: root.navDownTarget
            }
        }
    }
}
