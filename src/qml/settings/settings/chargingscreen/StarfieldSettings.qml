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
            objectName: "starfieldDensitySlider"
            height: 60
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.starfieldDensity; live: true
            onMoved: ScreensaverConfig.starfieldDensity = value
            onUserInteractionEnded: ScreensaverConfig.starfieldDensity = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: starfieldSpeedSlider
            KeyNavigation.down: starfieldTouchbarPicker
        }
    }

    // TOUCHBAR (madalone v1.6.4): what a slide along the remote's touch slider changes while the Starfield screensaver
    // shows (ScreensaverConfig.starfieldTouchbar, read by components/ChargingScreen.qml): the Animation speed, the Star
    // density (the default, and what a slide always changed up to v1.6.3) or nothing. The property only ever reads as
    // "speed", "density" or "off" (src/ui/screensaverTouchbar.h), so the row binds it as it is. Matrix keeps its own
    // "Touchbar speed" switch under General behavior.
    Components.SettingRow {
        id: starfieldTouchbarRow
        objectName: "starfieldTouchbarRow"
        //: Screensaver setting (Starfield): what sliding along the remote's touch slider changes while the screensaver
        //: shows. Its three choices are Speed, Density and Off.
        title: qsTr("Touchbar")
        help: {
            switch (ScreensaverConfig.starfieldTouchbar) {
            case "speed":
                //: Help of "Touchbar" set to Speed. "Animation speed" is the name of the Starfield setting above it.
                return qsTr("Sliding along the touch slider changes the Animation speed.");
            case "off":
                //: Help of "Touchbar" set to Off
                return qsTr("The touch slider does nothing while the screensaver shows.");
            default:
                //: Help of "Touchbar" set to Density. "Star density" is the name of the Starfield setting above it.
                return qsTr("Sliding along the touch slider changes the Star density.");
            }
        }
        controlBelow: true

        Components.ChipPicker {
            id: starfieldTouchbarPicker
            objectName: "starfieldTouchbarPicker"
            model: [
                //: Choice of "Touchbar": the touch slider changes the Starfield's Animation speed. A chip holds about
                //: 9 characters.
                { value: "speed", label: qsTr("Speed") },
                //: Choice of "Touchbar": the touch slider changes the Star density. A chip holds about 9 characters.
                { value: "density", label: qsTr("Density") },
                //: Choice of "Touchbar": the touch slider does nothing on the screensaver. A chip holds about
                //: 9 characters.
                { value: "off", label: qsTr("Off") }
            ]
            currentValue: ScreensaverConfig.starfieldTouchbar
            onPicked: {
                ScreensaverConfig.starfieldTouchbar = value;
                // the help line above the chips follows the choice (in English each of the three wraps to two lines, a
                // translation may not); with the keypad, bring the row back into view once the layout has followed (a
                // touch is left alone, as in the page's reveal)
                if (ui.keyNavigationActive && root.settingsPage && root.settingsPage.ensureVisible) {
                    Qt.callLater(root.settingsPage.ensureVisible, starfieldTouchbarRow);
                }
            }
            Accessible.name: "Touchbar"
            KeyNavigation.up: starfieldDensitySlider
            KeyNavigation.down: starfieldStarSizeSlider
        }
    }

    Components.SettingRow {
        title: qsTr("Star size")
        controlBelow: true

        Components.Slider {
            id: starfieldStarSizeSlider
            objectName: "starfieldStarSizeSlider"
            height: 60
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.starfieldStarSize; live: true
            onMoved: ScreensaverConfig.starfieldStarSize = value
            onUserInteractionEnded: ScreensaverConfig.starfieldStarSize = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: starfieldTouchbarPicker
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
