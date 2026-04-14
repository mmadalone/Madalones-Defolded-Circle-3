// Copyright (c) 2026 madalone. General behavior settings component.
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

    property alias firstFocusItem: tapToCloseSwitch
    property alias lastFocusItem: debugOverlaySwitch
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
                checked: ScreensaverConfig.tapToClose
                trigger: function() { ScreensaverConfig.tapToClose = !ScreensaverConfig.tapToClose; }
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
                checked: ScreensaverConfig.motionToClose
                trigger: function() { ScreensaverConfig.motionToClose = !ScreensaverConfig.motionToClose; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Close on wake"
                KeyNavigation.up: tapToCloseSwitch
                KeyNavigation.down: ScreensaverConfig.theme === "matrix" ? dpadEnabledSwitch : idleEnabledSwitch
            }
        }
    }

    Rectangle { visible: ScreensaverConfig.theme === "matrix"; Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 15b. DPAD INTERACTIVE (Matrix only — Starfield/Minimal have no interactive input)
    ColumnLayout {
        visible: ScreensaverConfig.theme === "matrix"
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
                checked: ScreensaverConfig.dpadEnabled
                trigger: function() { ScreensaverConfig.dpadEnabled = !ScreensaverConfig.dpadEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "DPAD interactive"
                KeyNavigation.up: motionToCloseSwitch
                KeyNavigation.down: ScreensaverConfig.dpadEnabled ? dpadPersistSwitch : tapDirectionSwitch
            }
        }
    }

    // 15c. PERSIST DIRECTION (visible when DPAD is on + Matrix theme)
    ColumnLayout {
        visible: ScreensaverConfig.theme === "matrix" && ScreensaverConfig.dpadEnabled
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
                checked: ScreensaverConfig.dpadPersist
                trigger: function() {
                    ScreensaverConfig.dpadPersist = !ScreensaverConfig.dpadPersist;
                    if (!ScreensaverConfig.dpadPersist) ScreensaverConfig.lastDirection = "";
                }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Remember direction"
                KeyNavigation.up: dpadEnabledSwitch; KeyNavigation.down: touchbarSpeedSwitch
            }
        }
    }

    // 15d. TOUCHBAR SPEED (visible when DPAD is on + Matrix theme)
    ColumnLayout {
        visible: ScreensaverConfig.theme === "matrix" && ScreensaverConfig.dpadEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Touchbar speed"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: touchbarSpeedSwitch
                objectName: "touchbarSpeedSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.dpadTouchbarSpeed
                trigger: function() { ScreensaverConfig.dpadTouchbarSpeed = !ScreensaverConfig.dpadTouchbarSpeed; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Touchbar speed control"
                KeyNavigation.up: dpadPersistSwitch; KeyNavigation.down: tapDirectionSwitch
            }
        }
    }

    Rectangle { visible: ScreensaverConfig.theme === "matrix"; Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 15d. TOUCH DIRECTIONS (Matrix only, mutually exclusive with DPAD interactive)
    ColumnLayout {
        visible: ScreensaverConfig.theme === "matrix"
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
                checked: ScreensaverConfig.tapDirection
                trigger: function() { ScreensaverConfig.tapDirection = !ScreensaverConfig.tapDirection; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Touch directions"
                KeyNavigation.up: ScreensaverConfig.dpadEnabled ? touchbarSpeedSwitch : dpadEnabledSwitch
                KeyNavigation.down: ScreensaverConfig.tapDirection ? tapDirPersistSwitch : idleEnabledSwitch
            }
        }
        Text {
            visible: ScreensaverConfig.tapDirection
            Layout.fillWidth: true; Layout.leftMargin: 10; Layout.rightMargin: 10
            color: colors.medium; wrapMode: Text.WordWrap
            text: qsTr("Tap screen zones to change direction. Triple-tap center to close.")
            font: fonts.primaryFont(26)
        }
    }

    // 15e. REMEMBER DIRECTION (visible when touch directions is on + Matrix theme)
    ColumnLayout {
        visible: ScreensaverConfig.theme === "matrix" && ScreensaverConfig.tapDirection
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
                checked: ScreensaverConfig.dpadPersist
                trigger: function() {
                    ScreensaverConfig.dpadPersist = !ScreensaverConfig.dpadPersist;
                    if (!ScreensaverConfig.dpadPersist) ScreensaverConfig.lastDirection = "";
                }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Remember direction"
                KeyNavigation.up: tapDirectionSwitch; KeyNavigation.down: swipeSpeedSwitch
            }
        }
    }

    // 15f. SWIPE SPEED (visible when touch directions is on + Matrix theme)
    ColumnLayout {
        visible: ScreensaverConfig.theme === "matrix" && ScreensaverConfig.tapDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Swipe speed"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: swipeSpeedSwitch
                objectName: "swipeSpeedSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.tapSwipeSpeed
                trigger: function() { ScreensaverConfig.tapSwipeSpeed = !ScreensaverConfig.tapSwipeSpeed; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Swipe to adjust speed"
                KeyNavigation.up: tapDirPersistSwitch; KeyNavigation.down: idleEnabledSwitch
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
                checked: ScreensaverConfig.idleEnabled
                trigger: function() { ScreensaverConfig.idleEnabled = !ScreensaverConfig.idleEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Idle screensaver"
                KeyNavigation.up: ScreensaverConfig.theme === "matrix"
                    ? (ScreensaverConfig.tapDirection ? swipeSpeedSwitch : tapDirectionSwitch)
                    : motionToCloseSwitch
                KeyNavigation.down: ScreensaverConfig.idleEnabled ? idleTimeoutSlider : debugOverlaySwitch
            }
        }
    }

    // 16b. IDLE TIMEOUT (visible when idle is on)
    ColumnLayout {
        visible: ScreensaverConfig.idleEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Idle timeout") + " (" + ScreensaverConfig.idleTimeout + "s)"
            font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: idleTimeoutSlider
            objectName: "idleTimeoutSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 15; to: 55; stepSize: 5
            value: ScreensaverConfig.idleTimeout; live: true
            onMoved: ScreensaverConfig.idleTimeout = value
            onUserInteractionEnded: ScreensaverConfig.idleTimeout = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Idle timeout " + value
            KeyNavigation.up: idleEnabledSwitch
            KeyNavigation.down: debugOverlaySwitch
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 17. DEBUG — ATLAS PROFILING OVERLAY (Matrix only; profiling tool)
    // Shows an on-screen strip with the last buildCombinedAtlas + first-paint
    // phase timings. Useful for regression investigation. Off by default.
    ColumnLayout {
        visible: ScreensaverConfig.theme === "matrix"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Atlas profiling overlay"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: debugOverlaySwitch
                objectName: "debugOverlaySwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.debugAtlasOverlay
                trigger: function() { ScreensaverConfig.debugAtlasOverlay = !ScreensaverConfig.debugAtlasOverlay; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Atlas profiling overlay"
                KeyNavigation.up: ScreensaverConfig.idleEnabled ? idleTimeoutSlider : idleEnabledSwitch
                KeyNavigation.down: root.navDownTarget
            }
        }
        Text {
            Layout.fillWidth: true; Layout.leftMargin: 10; Layout.rightMargin: 10
            color: colors.medium; wrapMode: Text.WordWrap
            text: qsTr("Shows atlas build phase timings at the top of the Matrix screensaver. Profiling tool — leave off during normal use.")
            font: fonts.primaryFont(26)
        }
    }
}
