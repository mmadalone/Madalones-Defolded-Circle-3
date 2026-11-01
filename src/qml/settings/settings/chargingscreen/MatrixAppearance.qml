// Copyright (c) 2026 madalone. Matrix appearance settings component.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0
import Palettes 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: solidColorRow
    property alias lastFocusItem: dirDiagRow
    property Item navUpTarget
    property Item navDownTarget

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true

    spacing: 0

    // COLOR: a solid colour or a gradient
    Components.SettingRow {
        title: qsTr("Color")
        controlBelow: true

        Column {
            width: parent.width
            spacing: 10

            Components.ChipPicker {
                id: solidColorRow
                objectName: "solidColorRow"
                Accessible.name: "Solid color selector"
                columns: 6
                model: Palettes.matrixSwatches
                currentValue: ScreensaverConfig.colorMode
                onPicked: ScreensaverConfig.colorMode = value
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: gradientColorRow
            }

            Components.ChipPicker {
                id: gradientColorRow
                objectName: "gradientColorRow"
                Accessible.name: "Gradient color selector"
                columns: 3
                model: Palettes.gradientSwatches
                currentValue: ScreensaverConfig.colorMode
                onPicked: ScreensaverConfig.colorMode = value
                KeyNavigation.up: solidColorRow
                KeyNavigation.down: charsetRow
            }
        }
    }

    Components.SettingRow {
        title: qsTr("Characters")
        controlBelow: true

        Components.ChipPicker {
            id: charsetRow
            objectName: "charsetRow"
            Accessible.name: "Character set selector"
            columns: 4
            model: [
                { value: "katakana", label: "Kana" },
                { value: "ascii",    label: "ABC" },
                { value: "binary",   label: "01" },
                { value: "digits",   label: "123" }
            ]
            currentValue: ScreensaverConfig.charset
            onPicked: ScreensaverConfig.charset = value
            KeyNavigation.up: gradientColorRow
            KeyNavigation.down: fontSizeSlider
        }
    }

    Components.SettingRow {
        title: qsTr("Font size")
        controlBelow: true

        Components.Slider {
            id: fontSizeSlider
            objectName: "fontSizeSlider"
            height: 60
            from: 10; to: 60; stepSize: 1
            value: ScreensaverConfig.fontSize; live: true; showLiveValue: true
            onMoved: ScreensaverConfig.fontSize = value
            onUserInteractionEnded: ScreensaverConfig.fontSize = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Font size " + value
            KeyNavigation.up: charsetRow
            KeyNavigation.down: speedSlider
        }
    }

    Components.SettingRow {
        title: qsTr("Animation speed")
        controlBelow: true

        Components.Slider {
            id: speedSlider
            objectName: "speedSlider"
            height: 60
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.matrixSpeed; live: true
            onMoved: ScreensaverConfig.matrixSpeed = value
            onUserInteractionEnded: ScreensaverConfig.matrixSpeed = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Animation speed " + value
            KeyNavigation.up: fontSizeSlider
            KeyNavigation.down: densitySlider
        }
    }

    Components.SettingRow {
        title: qsTr("Column density")
        controlBelow: true

        Components.Slider {
            id: densitySlider
            objectName: "densitySlider"
            height: 60
            from: 20; to: 500; stepSize: 5
            value: ScreensaverConfig.matrixDensity; live: true
            onMoved: ScreensaverConfig.matrixDensity = value
            onUserInteractionEnded: ScreensaverConfig.matrixDensity = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Column density " + value
            KeyNavigation.up: speedSlider
            KeyNavigation.down: trailSlider
        }
    }

    Components.SettingRow {
        title: qsTr("Trail length")
        controlBelow: true

        Components.Slider {
            id: trailSlider
            objectName: "trailSlider"
            height: 60
            from: 5; to: 100; stepSize: 5
            value: ScreensaverConfig.matrixTrail; live: true
            onMoved: ScreensaverConfig.matrixTrail = value
            onUserInteractionEnded: ScreensaverConfig.matrixTrail = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Trail length " + value
            KeyNavigation.up: densitySlider
            KeyNavigation.down: fadeSlider
        }
    }

    Components.SettingRow {
        title: qsTr("Trail fade")
        controlBelow: true

        Components.Slider {
            id: fadeSlider
            objectName: "fadeSlider"
            height: 60
            from: 20; to: 100; stepSize: 5
            value: ScreensaverConfig.matrixFade; live: true
            onMoved: ScreensaverConfig.matrixFade = value
            onUserInteractionEnded: ScreensaverConfig.matrixFade = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Trail fade " + value
            KeyNavigation.up: trailSlider
            KeyNavigation.down: gravitySwitch
        }
    }

    // AUTO-ROTATE, and its settings while it is on
    Components.SettingRow {
        title: qsTr("Auto-rotate")
        showDivider: !ScreensaverConfig.gravityMode

        Components.Switch {
            id: gravitySwitch
            objectName: "gravitySwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.gravityMode
            trigger: function() { ScreensaverConfig.gravityMode = !ScreensaverConfig.gravityMode; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Auto-rotate"
            KeyNavigation.up: fadeSlider
            KeyNavigation.down: ScreensaverConfig.gravityMode ? autoRotateSpeedSlider : dirCardinalRow
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.gravityMode
        title: qsTr("Rotation speed")
        value: ScreensaverConfig.autoRotateSpeed + "%"
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: autoRotateSpeedSlider
            objectName: "autoRotateSpeedSlider"
            height: 60
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.autoRotateSpeed; live: true; showLiveValue: false
            onMoved: ScreensaverConfig.autoRotateSpeed = value
            onUserInteractionEnded: ScreensaverConfig.autoRotateSpeed = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Rotation speed " + value
            KeyNavigation.up: gravitySwitch
            KeyNavigation.down: autoRotateSmoothnessSlider
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.gravityMode
        title: qsTr("Trail bend")
        value: ScreensaverConfig.autoRotateBend + "%"
        controlBelow: true

        Components.Slider {
            id: autoRotateSmoothnessSlider
            objectName: "autoRotateSmoothnessSlider"
            height: 60
            from: 5; to: 100; stepSize: 5
            value: ScreensaverConfig.autoRotateBend; live: true; showLiveValue: false
            onMoved: ScreensaverConfig.autoRotateBend = value
            onUserInteractionEnded: ScreensaverConfig.autoRotateBend = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Trail bend " + value
            KeyNavigation.up: autoRotateSpeedSlider
            KeyNavigation.down: dirCardinalRow
        }
    }

    // DIRECTION: off while auto-rotate turns the rain
    Components.SettingRow {
        title: qsTr("Direction")
        controlBelow: true

        Column {
            width: parent.width
            spacing: 10

            Components.ChipPicker {
                id: dirCardinalRow
                objectName: "dirCardinalRow"
                Accessible.name: "Cardinal direction selector"
                enabled: !ScreensaverConfig.gravityMode
                opacity: enabled ? 1.0 : 0.4
                columns: 4
                model: [
                    { value: "down",  label: "↓ " + qsTr("Down") },
                    { value: "up",    label: "↑ " + qsTr("Up") },
                    { value: "left",  label: "← " + qsTr("Left") },
                    { value: "right", label: "→ " + qsTr("Right") }
                ]
                currentValue: ScreensaverConfig.direction
                onPicked: ScreensaverConfig.direction = value
                KeyNavigation.up: ScreensaverConfig.gravityMode ? autoRotateSmoothnessSlider : gravitySwitch
                KeyNavigation.down: dirDiagRow
            }

            Components.ChipPicker {
                id: dirDiagRow
                objectName: "dirDiagRow"
                Accessible.name: "Diagonal direction selector"
                enabled: !ScreensaverConfig.gravityMode
                opacity: enabled ? 1.0 : 0.4
                columns: 4
                model: [
                    //: Direction down and to the right, abbreviated to fit a small button
                    { value: "down-right", label: "↘ " + qsTr("D-R") },
                    //: Direction down and to the left, abbreviated to fit a small button
                    { value: "down-left",  label: "↙ " + qsTr("D-L") },
                    //: Direction up and to the right, abbreviated to fit a small button
                    { value: "up-right",   label: "↗ " + qsTr("U-R") },
                    //: Direction up and to the left, abbreviated to fit a small button
                    { value: "up-left",    label: "↖ " + qsTr("U-L") }
                ]
                currentValue: ScreensaverConfig.direction
                onPicked: ScreensaverConfig.direction = value
                KeyNavigation.up: dirCardinalRow
                KeyNavigation.down: root.navDownTarget
            }
        }
    }
}
