// Copyright (c) 2026 madalone. Direction glitch settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
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

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true
    readonly property bool trailsShown: ScreensaverConfig.glitch && ScreensaverConfig.glitchDirection
    // the glitch directions that are on, as bits of glitchDirMask
    readonly property var dirBits: [0, 1, 2, 3, 4, 5, 6, 7].filter(function(bit) {
        return (ScreensaverConfig.glitchDirMask & (1 << bit)) !== 0;
    })

    // Helper to toggle a bit in the mask with minimum-1 guard
    function toggleDirBit(bit) {
        var newMask = ScreensaverConfig.glitchDirMask ^ (1 << bit);
        if (newMask > 0) {
            ScreensaverConfig.glitchDirMask = newMask;
            Haptic.play(Haptic.Click);
        }
    }

    spacing: 0

    // DIRECTION CHANGE, and its trails while it is on
    Components.SettingRow {
        visible: ScreensaverConfig.glitch
        title: qsTr("Direction change")
        showDivider: !ScreensaverConfig.glitchDirection

        Components.Switch {
            id: glitchDirectionSwitch
            objectName: "glitchDirectionSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchDirection
            trigger: function() { ScreensaverConfig.glitchDirection = !ScreensaverConfig.glitchDirection; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Direction change"
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: glitchDirRateSlider
        }
    }

    Components.SettingRow {
        visible: root.trailsShown
        title: qsTr("Frequency")
        help: qsTr("How often streams send off glitch trails.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: glitchDirRateSlider
            objectName: "glitchDirRateSlider"
            height: 60
            from: 5; to: 80; stepSize: 5
            value: ScreensaverConfig.glitchDirRate; live: true
            onMoved: ScreensaverConfig.glitchDirRate = value
            onUserInteractionEnded: ScreensaverConfig.glitchDirRate = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Frequency " + value
            KeyNavigation.up: glitchDirectionSwitch
            KeyNavigation.down: glitchDirLengthSlider
        }
    }

    Components.SettingRow {
        visible: root.trailsShown
        title: qsTr("Trail length")
        help: qsTr("The length of the glitch trails.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: glitchDirLengthSlider
            objectName: "glitchDirLengthSlider"
            height: 60
            from: 3; to: 30; stepSize: 1; live: true
            value: ScreensaverConfig.glitchDirLength
            onMoved: ScreensaverConfig.glitchDirLength = value
            onUserInteractionEnded: ScreensaverConfig.glitchDirLength = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Trail length " + value
            KeyNavigation.up: glitchDirRateSlider
            KeyNavigation.down: glitchDirCardinalRow
        }
    }

    // 8 direction toggles: every chip is a toggle of its own, at least one stays on
    Components.SettingRow {
        visible: root.trailsShown
        title: qsTr("Glitch directions")
        controlBelow: true
        showDivider: false

        Column {
            width: parent.width
            spacing: 10

            // Cardinal directions (bits 0-3: down, up, left, right)
            Components.ChipPicker {
                id: glitchDirCardinalRow
                objectName: "glitchDirCardinalRow"
                Accessible.name: "Glitch cardinal direction toggles"
                multiple: true
                columns: 4
                model: [
                    { value: 0, label: "↓ " + qsTr("Down") },
                    { value: 1, label: "↑ " + qsTr("Up") },
                    { value: 3, label: "← " + qsTr("Left") },
                    { value: 2, label: "→ " + qsTr("Right") }
                ]
                currentValues: root.dirBits
                onPicked: root.toggleDirBit(value)
                KeyNavigation.up: glitchDirLengthSlider
                KeyNavigation.down: glitchDirDiagRow
            }

            // Diagonal directions (bits 4-7: down-right, down-left, up-right, up-left)
            Components.ChipPicker {
                id: glitchDirDiagRow
                objectName: "glitchDirDiagRow"
                Accessible.name: "Glitch diagonal direction toggles"
                multiple: true
                columns: 4
                model: [
                    //: Direction down and to the right, abbreviated to fit a small button
                    { value: 4, label: "↘ " + qsTr("D-R") },
                    //: Direction down and to the left, abbreviated to fit a small button
                    { value: 5, label: "↙ " + qsTr("D-L") },
                    //: Direction up and to the right, abbreviated to fit a small button
                    { value: 6, label: "↗ " + qsTr("U-R") },
                    //: Direction up and to the left, abbreviated to fit a small button
                    { value: 7, label: "↖ " + qsTr("U-L") }
                ]
                currentValues: root.dirBits
                onPicked: root.toggleDirBit(value)
                KeyNavigation.up: glitchDirCardinalRow
                KeyNavigation.down: glitchDirFadeSlider
            }
        }
    }

    Components.SettingRow {
        visible: root.trailsShown
        title: qsTr("Trail fade")
        help: qsTr("How long the glitch trails linger.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: glitchDirFadeSlider
            objectName: "glitchDirFadeSlider"
            height: 60
            from: 0; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchDirFade
            onMoved: ScreensaverConfig.glitchDirFade = value
            onUserInteractionEnded: ScreensaverConfig.glitchDirFade = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Trail fade " + value
            KeyNavigation.up: glitchDirDiagRow
            KeyNavigation.down: glitchDirSpeedSlider
        }
    }

    Components.SettingRow {
        visible: root.trailsShown
        title: qsTr("Trail speed")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: glitchDirSpeedSlider
            objectName: "glitchDirSpeedSlider"
            height: 60
            from: 10; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchDirSpeed
            onMoved: ScreensaverConfig.glitchDirSpeed = value
            onUserInteractionEnded: ScreensaverConfig.glitchDirSpeed = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Trail speed " + value
            KeyNavigation.up: glitchDirFadeSlider
            KeyNavigation.down: glitchRandomColorSwitch
        }
    }

    Components.SettingRow {
        visible: root.trailsShown
        title: qsTr("Random color")

        Components.Switch {
            id: glitchRandomColorSwitch
            objectName: "glitchRandomColorSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchRandomColor
            trigger: function() { ScreensaverConfig.glitchRandomColor = !ScreensaverConfig.glitchRandomColor; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Random color"
            KeyNavigation.up: glitchDirSpeedSlider
            KeyNavigation.down: root.navDownTarget
        }
    }
}
