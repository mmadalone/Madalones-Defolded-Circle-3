// Copyright (c) 2026 madalone. Starfield theme settings (extracted from ChargingScreen.qml).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0
import Palettes 1.0

ColumnLayout {
    id: root

    required property Item settingsPage
    property Item navUpTarget
    property Item navDownTarget

    property alias firstFocusItem: starfieldSpeedSlider
    property alias lastFocusItem: starfieldColorGradientRow

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true

    Layout.fillWidth: true
    spacing: 0

    Components.SettingRow {
        title: qsTr("Animation speed")
        controlBelow: true

        Components.Slider {
            id: starfieldSpeedSlider
            height: 60
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.starfieldSpeed; live: true
            onMoved: ScreensaverConfig.starfieldSpeed = value
            onUserInteractionEnded: ScreensaverConfig.starfieldSpeed = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: starfieldDensitySlider
        }
    }

    Components.SettingRow {
        title: qsTr("Star density")
        controlBelow: true

        Components.Slider {
            id: starfieldDensitySlider
            height: 60
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.starfieldDensity; live: true
            onMoved: ScreensaverConfig.starfieldDensity = value
            onUserInteractionEnded: ScreensaverConfig.starfieldDensity = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: starfieldSpeedSlider
            KeyNavigation.down: starfieldStarSizeSlider
        }
    }

    Components.SettingRow {
        title: qsTr("Star size")
        controlBelow: true

        Components.Slider {
            id: starfieldStarSizeSlider
            height: 60
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.starfieldStarSize; live: true
            onMoved: ScreensaverConfig.starfieldStarSize = value
            onUserInteractionEnded: ScreensaverConfig.starfieldStarSize = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: starfieldDensitySlider
            KeyNavigation.down: starfieldTrailSlider
        }
    }

    Components.SettingRow {
        title: qsTr("Trail length")
        controlBelow: true

        Components.Slider {
            id: starfieldTrailSlider
            height: 60
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.starfieldTrailLength; live: true
            onMoved: ScreensaverConfig.starfieldTrailLength = value
            onUserInteractionEnded: ScreensaverConfig.starfieldTrailLength = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: starfieldStarSizeSlider
            KeyNavigation.down: starfieldColorSolidRow
        }
    }

    Components.SettingRow {
        title: qsTr("Star color")
        controlBelow: true

        Column {
            width: parent.width
            spacing: 10

            Components.ChipPicker {
                id: starfieldColorSolidRow
                columns: 4
                model: Palettes.textSwatches
                currentValue: ScreensaverConfig.starfieldColor
                onPicked: ScreensaverConfig.starfieldColor = value
                KeyNavigation.up: starfieldTrailSlider
                KeyNavigation.down: starfieldColorGradientRow
            }

            Components.ChipPicker {
                id: starfieldColorGradientRow
                columns: 3
                model: Palettes.gradientSwatches
                currentValue: ScreensaverConfig.starfieldColor
                onPicked: ScreensaverConfig.starfieldColor = value
                KeyNavigation.up: starfieldColorSolidRow
                KeyNavigation.down: root.navDownTarget
            }
        }
    }
}
