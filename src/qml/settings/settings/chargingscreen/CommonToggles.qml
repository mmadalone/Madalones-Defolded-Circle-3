// Copyright (c) 2026 madalone. Common toggles component (clock, battery).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0
import Palettes 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property Item firstFocusItem: clockSettings ? showClockSwitch : showBatterySwitch
    property Item lastFocusItem: ScreensaverConfig.showBatteryEnabled ? batteryTextSizeSlider : showBatterySwitch
    property Item navUpTarget
    property Item navDownTarget

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true
    // Minimal and Analog always show their own clock
    readonly property bool clockSettings: ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
    readonly property bool clockShown: clockSettings && ScreensaverConfig.showClock
    readonly property bool dateShown: clockShown && ScreensaverConfig.clockShowDate

    spacing: 0

    // SHOW CLOCK, and the clock's settings while it is on
    Components.SettingRow {
        visible: root.clockSettings
        title: qsTr("Show clock")
        showDivider: !ScreensaverConfig.showClock

        Components.Switch {
            id: showClockSwitch
            objectName: "showClockSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.showClock
            trigger: function() { ScreensaverConfig.showClock = !ScreensaverConfig.showClock; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Show clock"
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: ScreensaverConfig.showClock ? clockDockedSwitch : showBatterySwitch
        }
    }

    Components.SettingRow {
        visible: root.clockShown
        title: qsTr("Charging only")
        help: qsTr("Shows the clock only while the remote is charging.")
        showDivider: false

        Components.Switch {
            id: clockDockedSwitch
            icon: "uc:check"
            checked: ScreensaverConfig.clockDockedOnly
            trigger: function() { ScreensaverConfig.clockDockedOnly = !ScreensaverConfig.clockDockedOnly; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Clock charging only"
            KeyNavigation.up: showClockSwitch
            KeyNavigation.down: clockFontRow
        }
    }

    Components.SettingRow {
        visible: root.clockShown
        title: qsTr("Font")
        controlBelow: true
        showDivider: false

        Components.ChipPicker {
            id: clockFontRow
            columns: 2
            model: [
                { value: "primary",   label: "Poppins" },
                { value: "secondary", label: "Space Mono" }
            ]
            currentValue: ScreensaverConfig.clockFont
            onPicked: ScreensaverConfig.clockFont = value
            KeyNavigation.up: clockDockedSwitch
            KeyNavigation.down: clockColorRow
        }
    }

    Components.SettingRow {
        visible: root.clockShown
        title: qsTr("Color")
        controlBelow: true
        showDivider: false

        Column {
            width: parent.width
            spacing: 10

            Components.ChipPicker {
                id: clockColorRow
                columns: 4
                model: Palettes.textSwatches
                currentValue: ScreensaverConfig.clockColor
                onPicked: ScreensaverConfig.clockColor = value
                KeyNavigation.up: clockFontRow
                KeyNavigation.down: clockGradientRow
            }

            Components.ChipPicker {
                id: clockGradientRow
                columns: 3
                model: Palettes.gradientSwatches
                currentValue: ScreensaverConfig.clockColor
                onPicked: ScreensaverConfig.clockColor = value
                KeyNavigation.up: clockColorRow
                KeyNavigation.down: clockSizeSlider
            }
        }
    }

    Components.SettingRow {
        visible: root.clockShown
        title: qsTr("Size")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: clockSizeSlider
            height: 60
            from: 24; to: 96; stepSize: 4
            value: ScreensaverConfig.clockSize; live: true
            onMoved: ScreensaverConfig.clockSize = value
            onUserInteractionEnded: ScreensaverConfig.clockSize = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: clockGradientRow
            KeyNavigation.down: clock24hSwitch
        }
    }

    Components.SettingRow {
        visible: root.clockShown
        title: qsTr("24-hour clock")
        showDivider: false

        Components.Switch {
            id: clock24hSwitch
            icon: "uc:check"
            checked: ScreensaverConfig.clockClock24h
            trigger: function() { ScreensaverConfig.clockClock24h = !ScreensaverConfig.clockClock24h; }
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: clockSizeSlider
            KeyNavigation.down: clockShowDateSwitch
        }
    }

    Components.SettingRow {
        visible: root.clockShown
        title: qsTr("Show date")
        showDivider: false

        Components.Switch {
            id: clockShowDateSwitch
            icon: "uc:check"
            checked: ScreensaverConfig.clockShowDate
            trigger: function() { ScreensaverConfig.clockShowDate = !ScreensaverConfig.clockShowDate; }
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: clock24hSwitch
            KeyNavigation.down: ScreensaverConfig.clockShowDate ? clockDateSizeSlider : clockPositionRow
        }
    }

    Components.SettingRow {
        visible: root.dateShown
        title: qsTr("Date size")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: clockDateSizeSlider
            height: 60
            from: 12; to: 40; stepSize: 2
            value: ScreensaverConfig.clockDateSize; live: true
            onMoved: ScreensaverConfig.clockDateSize = value
            onUserInteractionEnded: ScreensaverConfig.clockDateSize = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: clockShowDateSwitch
            KeyNavigation.down: clockDateColorRow
        }
    }

    Components.SettingRow {
        visible: root.dateShown
        title: qsTr("Date color")
        controlBelow: true
        showDivider: false

        Column {
            width: parent.width
            spacing: 10

            Components.ChipPicker {
                id: clockDateColorRow
                columns: 4
                model: Palettes.textSwatches
                currentValue: ScreensaverConfig.clockDateColor
                onPicked: ScreensaverConfig.clockDateColor = value
                KeyNavigation.up: clockDateSizeSlider
                KeyNavigation.down: clockDateGradientRow
            }

            Components.ChipPicker {
                id: clockDateGradientRow
                columns: 3
                model: Palettes.gradientSwatches
                currentValue: ScreensaverConfig.clockDateColor
                onPicked: ScreensaverConfig.clockDateColor = value
                KeyNavigation.up: clockDateColorRow
                KeyNavigation.down: clockPositionRow
            }
        }
    }

    Components.SettingRow {
        visible: root.clockShown
        title: qsTr("Position")
        controlBelow: true

        Components.ChipPicker {
            id: clockPositionRow
            columns: 3
            model: [
                { value: "top",    label: qsTr("Top") },
                { value: "center", label: qsTr("Center") },
                { value: "bottom", label: qsTr("Bottom") }
            ]
            currentValue: ScreensaverConfig.clockPosition
            onPicked: ScreensaverConfig.clockPosition = value
            KeyNavigation.up: ScreensaverConfig.clockShowDate ? clockDateGradientRow : clockShowDateSwitch
            KeyNavigation.down: showBatterySwitch
        }
    }

    // SHOW BATTERY, and the battery's settings while it is on
    Components.SettingRow {
        title: qsTr("Show battery")
        showDivider: !ScreensaverConfig.showBatteryEnabled

        Components.Switch {
            id: showBatterySwitch
            objectName: "showBatterySwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.showBatteryEnabled
            trigger: function() { ScreensaverConfig.showBatteryEnabled = !ScreensaverConfig.showBatteryEnabled; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Show battery"
            KeyNavigation.up: root.clockSettings ? (ScreensaverConfig.showClock ? clockPositionRow : showClockSwitch)
                                                 : root.navUpTarget
            KeyNavigation.down: batteryDockedSwitch
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.showBatteryEnabled
        title: qsTr("Charging only")
        help: qsTr("Shows the battery level only while the remote is charging.")
        showDivider: false

        Components.Switch {
            id: batteryDockedSwitch
            objectName: "batteryDockedSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.batteryDockedOnly
            trigger: function() { ScreensaverConfig.batteryDockedOnly = !ScreensaverConfig.batteryDockedOnly; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Charging only"
            KeyNavigation.up: showBatterySwitch
            KeyNavigation.down: batteryTextSizeSlider
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.showBatteryEnabled
        title: qsTr("Text size")
        controlBelow: true

        Components.Slider {
            id: batteryTextSizeSlider
            height: 60
            from: 16; to: 48; stepSize: 2
            value: ScreensaverConfig.batteryTextSize; live: true
            onMoved: ScreensaverConfig.batteryTextSize = value
            onUserInteractionEnded: ScreensaverConfig.batteryTextSize = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: batteryDockedSwitch
            KeyNavigation.down: root.navDownTarget
        }
    }
}
