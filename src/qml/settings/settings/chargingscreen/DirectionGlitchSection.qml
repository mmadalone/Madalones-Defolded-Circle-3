// Copyright (c) 2024 madalone. Direction glitch settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: glitchDirectionSwitch
    property alias lastFocusItem: glitchRandomColorSwitch
    property Item navUpTarget
    property Item navDownTarget

    // Helper to toggle a bit in the mask with minimum-1 guard
    function toggleDirBit(bit) {
        var newMask = ScreensaverConfig.glitchDirMask ^ (1 << bit);
        if (newMask > 0) {
            ScreensaverConfig.glitchDirMask = newMask;
            Haptic.play(Haptic.Click);
        }
    }

    spacing: 20

    // 12e. DIRECTION CHANGE
    ColumnLayout {
        visible: ScreensaverConfig.glitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Direction change"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchDirectionSwitch
                objectName: "glitchDirectionSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchDirection
                trigger: function() { ScreensaverConfig.glitchDirection = !ScreensaverConfig.glitchDirection; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Direction change"
                KeyNavigation.up: root.navUpTarget; KeyNavigation.down: glitchDirRateSlider
            }
        }
    }

    // 12f. DIRECTION GLITCH FREQUENCY (visible when direction glitch is on)
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.light
            text: qsTr("Frequency"); font: fonts.primaryFont(24)
        }
        Components.Slider {
            id: glitchDirRateSlider
            objectName: "glitchDirRateSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 80; stepSize: 5
            value: ScreensaverConfig.glitchDirRate; live: true
            onMoved: ScreensaverConfig.glitchDirRate = value
            onUserInteractionEnded: ScreensaverConfig.glitchDirRate = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Frequency " + value
            KeyNavigation.up: glitchDirectionSwitch; KeyNavigation.down: glitchDirLengthSlider
        }
    }

    // 12f2. DIRECTION GLITCH LENGTH (visible when direction glitch is on)
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail length"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: glitchDirLengthSlider
            objectName: "glitchDirLengthSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 3; to: 30; stepSize: 1; live: true
            value: ScreensaverConfig.glitchDirLength
            onMoved: ScreensaverConfig.glitchDirLength = value
            onUserInteractionEnded: ScreensaverConfig.glitchDirLength = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Trail length " + value
            KeyNavigation.up: glitchDirRateSlider; KeyNavigation.down: glitchDirCardinalRow
        }
    }

    // 12g. DIRECTION TOGGLES — 8 individual direction toggles (visible when direction glitch is on)
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10

        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Glitch directions"); font: fonts.primaryFont(24) }

        // Cardinal directions (bits 0-3: down, up, left, right)
        RowLayout {
            id: glitchDirCardinalRow
            objectName: "glitchDirCardinalRow"
            spacing: 8; focus: true
            Accessible.name: "Glitch cardinal direction toggles"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: glitchDirLengthSlider; KeyNavigation.down: glitchDirDiagRow
            Repeater {
                model: [
                    { bit: 0, label: "\u2193 Down" },
                    { bit: 1, label: "\u2191 Up" },
                    { bit: 3, label: "\u2190 Left" },
                    { bit: 2, label: "\u2192 Right" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    property bool checked: (ScreensaverConfig.glitchDirMask & (1 << modelData.bit)) !== 0
                    color: checked ? colors.offwhite : colors.dark
                    border { color: checked ? colors.offwhite : colors.medium; width: checked ? 3 : 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: parent.checked ? colors.black : colors.offwhite
                        font: fonts.primaryFont(20)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: root.toggleDirBit(modelData.bit)
                    }
                }
            }
        }

        // Diagonal directions (bits 4-7: down-right, down-left, up-right, up-left)
        RowLayout {
            id: glitchDirDiagRow
            objectName: "glitchDirDiagRow"
            spacing: 8; focus: true
            Accessible.name: "Glitch diagonal direction toggles"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: glitchDirCardinalRow; KeyNavigation.down: glitchDirFadeSlider
            Repeater {
                model: [
                    { bit: 4, label: "\u2198 D-R" },
                    { bit: 5, label: "\u2199 D-L" },
                    { bit: 6, label: "\u2197 U-R" },
                    { bit: 7, label: "\u2196 U-L" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    property bool checked: (ScreensaverConfig.glitchDirMask & (1 << modelData.bit)) !== 0
                    color: checked ? colors.offwhite : colors.dark
                    border { color: checked ? colors.offwhite : colors.medium; width: checked ? 3 : 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: parent.checked ? colors.black : colors.offwhite
                        font: fonts.primaryFont(20)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: root.toggleDirBit(modelData.bit)
                    }
                }
            }
        }
    }

    // 12g2. TRAIL FADE (visible when direction glitch is on)
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail fade"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: glitchDirFadeSlider
            objectName: "glitchDirFadeSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 0; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchDirFade
            onMoved: ScreensaverConfig.glitchDirFade = value
            onUserInteractionEnded: ScreensaverConfig.glitchDirFade = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Trail fade " + value
            KeyNavigation.up: glitchDirDiagRow; KeyNavigation.down: glitchDirSpeedSlider
        }
    }

    // 12g3. TRAIL SPEED (visible when direction glitch is on)
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail speed"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: glitchDirSpeedSlider
            objectName: "glitchDirSpeedSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchDirSpeed
            onMoved: ScreensaverConfig.glitchDirSpeed = value
            onUserInteractionEnded: ScreensaverConfig.glitchDirSpeed = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Trail speed " + value
            KeyNavigation.up: glitchDirFadeSlider; KeyNavigation.down: glitchRandomColorSwitch
        }
    }

    // 12g4. RANDOM COLOR (visible when direction glitch is on)
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Random color"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: glitchRandomColorSwitch
                objectName: "glitchRandomColorSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchRandomColor
                trigger: function() { ScreensaverConfig.glitchRandomColor = !ScreensaverConfig.glitchRandomColor; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Random color"
                KeyNavigation.up: glitchDirSpeedSlider; KeyNavigation.down: root.navDownTarget
            }
        }
    }
}
