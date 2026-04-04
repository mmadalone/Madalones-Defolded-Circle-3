// Copyright (c) 2024 madalone. General behavior settings component.
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

    property alias firstFocusItem: tapToCloseSwitch
    property alias lastFocusItem: idleTimeoutSlider
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // Leading separator
    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 14. TAP TO CLOSE
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Double-tap to close"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: tapToCloseSwitch
                objectName: "tapToCloseSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingTapToClose
                trigger: function() { Config.chargingTapToClose = !Config.chargingTapToClose; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: motionToCloseSwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 15. CLOSE ON WAKE
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Close on wake"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: motionToCloseSwitch
                objectName: "motionToCloseSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMotionToClose
                trigger: function() { Config.chargingMotionToClose = !Config.chargingMotionToClose; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: tapToCloseSwitch; KeyNavigation.down: idleEnabledSwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 16. IDLE SCREENSAVER
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Idle screensaver"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: idleEnabledSwitch
                objectName: "idleEnabledSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingIdleEnabled
                trigger: function() { Config.chargingIdleEnabled = !Config.chargingIdleEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: motionToCloseSwitch; KeyNavigation.down: idleTimeoutSlider
            }
        }
    }

    // 16b. IDLE TIMEOUT (visible when idle is on)
    ColumnLayout {
        visible: Config.chargingIdleEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Idle timeout") + " (" + Config.chargingIdleTimeout + "s)"
            font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: idleTimeoutSlider
            objectName: "idleTimeoutSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 15; to: 55; stepSize: 5
            value: Config.chargingIdleTimeout; live: true
            onValueChanged: Config.chargingIdleTimeout = value
            onUserInteractionEnded: Config.chargingIdleTimeout = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: idleEnabledSwitch
        }
    }
}
