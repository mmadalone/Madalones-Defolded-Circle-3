// Copyright (c) 2026 madalone. Minimal clock theme settings (extracted from ChargingScreen.qml).
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

    property alias firstFocusItem: minimalClock24hSwitch
    property alias lastFocusItem: minimalDateSlider

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true

    Layout.fillWidth: true
    spacing: 0

    Components.SettingRow {
        title: qsTr("24-hour clock")

        Components.Switch {
            id: minimalClock24hSwitch
            icon: "uc:check"
            checked: ScreensaverConfig.minimalClock24h
            trigger: function() { ScreensaverConfig.minimalClock24h = !ScreensaverConfig.minimalClock24h; }
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: minimalFontRow
        }
    }

    Components.SettingRow {
        title: qsTr("Font")
        controlBelow: true

        Components.ChipPicker {
            id: minimalFontRow
            columns: 2
            model: [
                { value: "primary",   label: "Poppins" },
                { value: "secondary", label: "Space Mono" }
            ]
            currentValue: ScreensaverConfig.minimalFont
            onPicked: ScreensaverConfig.minimalFont = value
            KeyNavigation.up: minimalClock24hSwitch
            KeyNavigation.down: minimalTimeColorSolidRow
        }
    }

    Components.SettingRow {
        title: qsTr("Time color")
        controlBelow: true

        Column {
            width: parent.width
            spacing: 10

            Components.ChipPicker {
                id: minimalTimeColorSolidRow
                columns: 4
                model: Palettes.textSwatches
                currentValue: ScreensaverConfig.minimalTimeColor
                onPicked: ScreensaverConfig.minimalTimeColor = value
                KeyNavigation.up: minimalFontRow
                KeyNavigation.down: minimalTimeGradientRow
            }

            Components.ChipPicker {
                id: minimalTimeGradientRow
                columns: 3
                model: Palettes.gradientSwatches
                currentValue: ScreensaverConfig.minimalTimeColor
                onPicked: ScreensaverConfig.minimalTimeColor = value
                KeyNavigation.up: minimalTimeColorSolidRow
                KeyNavigation.down: minimalDateColorSolidRow
            }
        }
    }

    Components.SettingRow {
        title: qsTr("Date color")
        controlBelow: true

        Column {
            width: parent.width
            spacing: 10

            Components.ChipPicker {
                id: minimalDateColorSolidRow
                columns: 4
                model: Palettes.minimalDateSwatches
                currentValue: ScreensaverConfig.minimalDateColor
                onPicked: ScreensaverConfig.minimalDateColor = value
                KeyNavigation.up: minimalTimeGradientRow
                KeyNavigation.down: minimalDateGradientRow
            }

            Components.ChipPicker {
                id: minimalDateGradientRow
                columns: 3
                model: Palettes.gradientSwatches
                currentValue: ScreensaverConfig.minimalDateColor
                onPicked: ScreensaverConfig.minimalDateColor = value
                KeyNavigation.up: minimalDateColorSolidRow
                KeyNavigation.down: minimalClockSlider
            }
        }
    }

    Components.SettingRow {
        title: qsTr("Clock size")
        controlBelow: true

        Components.Slider {
            id: minimalClockSlider
            height: 60
            from: 48; to: 144; stepSize: 4
            value: ScreensaverConfig.minimalClockSize; live: true
            onMoved: ScreensaverConfig.minimalClockSize = value
            onUserInteractionEnded: ScreensaverConfig.minimalClockSize = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: minimalDateGradientRow
            KeyNavigation.down: minimalDateSlider
        }
    }

    Components.SettingRow {
        title: qsTr("Date size")
        controlBelow: true

        Components.Slider {
            id: minimalDateSlider
            height: 60
            from: 16; to: 48; stepSize: 2
            value: ScreensaverConfig.minimalDateSize; live: true
            onMoved: ScreensaverConfig.minimalDateSize = value
            onUserInteractionEnded: ScreensaverConfig.minimalDateSize = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: minimalClockSlider
            KeyNavigation.down: root.navDownTarget
        }
    }
}
