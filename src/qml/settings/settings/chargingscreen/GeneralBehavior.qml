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
                Accessible.name: "Double-tap to close"
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
                Accessible.name: "Close on wake"
                KeyNavigation.up: tapToCloseSwitch; KeyNavigation.down: dpadEnabledSwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 15b. DPAD INTERACTIVE
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("DPAD interactive"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: dpadEnabledSwitch
                objectName: "dpadEnabledSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixDpadEnabled
                trigger: function() { Config.chargingMatrixDpadEnabled = !Config.chargingMatrixDpadEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "DPAD interactive"
                KeyNavigation.up: motionToCloseSwitch
                KeyNavigation.down: Config.chargingMatrixDpadEnabled ? dpadPersistSwitch : tapDirectionSwitch
            }
        }
    }

    // 15c. PERSIST DIRECTION (visible when DPAD is on)
    ColumnLayout {
        visible: Config.chargingMatrixDpadEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Remember direction"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: dpadPersistSwitch
                objectName: "dpadPersistSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixDpadPersist
                trigger: function() {
                    Config.chargingMatrixDpadPersist = !Config.chargingMatrixDpadPersist;
                    if (!Config.chargingMatrixDpadPersist) Config.chargingMatrixLastDirection = "";
                }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Remember direction"
                KeyNavigation.up: dpadEnabledSwitch; KeyNavigation.down: tapDirectionSwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 15d. TOUCH DIRECTIONS (mutually exclusive with DPAD interactive)
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Touch directions"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: tapDirectionSwitch
                objectName: "tapDirectionSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapDirection
                trigger: function() { Config.chargingMatrixTapDirection = !Config.chargingMatrixTapDirection; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Touch directions"
                KeyNavigation.up: Config.chargingMatrixDpadEnabled ? dpadPersistSwitch : dpadEnabledSwitch
                KeyNavigation.down: Config.chargingMatrixTapDirection ? tapDirPersistSwitch : idleEnabledSwitch
            }
        }
        Text {
            visible: Config.chargingMatrixTapDirection
            Layout.fillWidth: true; Layout.leftMargin: 10; Layout.rightMargin: 10
            color: colors.medium; wrapMode: Text.WordWrap
            text: qsTr("Tap screen zones to change direction. Triple-tap center to close.")
            font: fonts.primaryFont(26)
        }
    }

    // 15e. REMEMBER DIRECTION (visible when touch directions is on)
    ColumnLayout {
        visible: Config.chargingMatrixTapDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Remember direction"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapDirPersistSwitch
                objectName: "tapDirPersistSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixDpadPersist
                trigger: function() {
                    Config.chargingMatrixDpadPersist = !Config.chargingMatrixDpadPersist;
                    if (!Config.chargingMatrixDpadPersist) Config.chargingMatrixLastDirection = "";
                }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Remember direction"
                KeyNavigation.up: tapDirectionSwitch; KeyNavigation.down: idleEnabledSwitch
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
                Accessible.name: "Idle screensaver"
                KeyNavigation.up: Config.chargingMatrixTapDirection ? tapDirPersistSwitch : tapDirectionSwitch
                KeyNavigation.down: idleTimeoutSlider
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
            onMoved: Config.chargingIdleTimeout = value
            onUserInteractionEnded: Config.chargingIdleTimeout = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Idle timeout " + value
            KeyNavigation.up: idleEnabledSwitch
        }
    }
}
