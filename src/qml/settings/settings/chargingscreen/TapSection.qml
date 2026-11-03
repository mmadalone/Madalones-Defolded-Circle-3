// Copyright (c) 2026 madalone. Tap effect settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: tapEnabledSwitch
    property Item lastFocusItem: ScreensaverConfig.tapRandomize ? tapRandomizeChanceSlider : tapRandomizeSwitch
    property Item navUpTarget
    property Item navDownTarget

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true

    spacing: 0

    // TAP EFFECTS: the heading and the master switch, one row for the reveal
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 20
            Layout.topMargin: 20
            wrapMode: Text.WordWrap
            color: colors.textPrimary
            text: qsTr("Tap effects")
            font: fonts.heading()
        }

        Components.SettingRow {
            title: qsTr("Enable tap effects")
            showDivider: !ScreensaverConfig.tapEnabled

            Components.Switch {
                id: tapEnabledSwitch
                objectName: "tapEnabledSwitch"
                icon: "uc:check"
                checked: ScreensaverConfig.tapEnabled
                trigger: function() { ScreensaverConfig.tapEnabled = !ScreensaverConfig.tapEnabled; }
                highlight: activeFocus && ui.keyNavigationActive
                Accessible.name: "Enable tap effects"
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: ScreensaverConfig.tapEnabled ? tapBurstSwitch : root.navDownTarget
            }
        }
    }

    // --- Scatter burst + sub-settings ---
    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled
        title: qsTr("Scatter burst")
        showDivider: false

        Components.Switch {
            id: tapBurstSwitch
            objectName: "tapBurstSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapBurst
            trigger: function() { ScreensaverConfig.tapBurst = !ScreensaverConfig.tapBurst; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Scatter burst"
            KeyNavigation.up: tapEnabledSwitch
            KeyNavigation.down: ScreensaverConfig.tapBurst ? tapBurstCountSlider : tapFlashSwitch
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled && ScreensaverConfig.tapBurst
        title: qsTr("Trail count")
        value: String(ScreensaverConfig.tapBurstCount)
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: tapBurstCountSlider
            height: 60
            from: 10; to: 50; stepSize: 5; live: true; showLiveValue: false
            value: ScreensaverConfig.tapBurstCount
            onMoved: ScreensaverConfig.tapBurstCount = value
            onUserInteractionEnded: ScreensaverConfig.tapBurstCount = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Trail count " + value
            KeyNavigation.up: tapBurstSwitch
            KeyNavigation.down: tapBurstLengthSlider
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled && ScreensaverConfig.tapBurst
        title: qsTr("Trail length")
        value: String(ScreensaverConfig.tapBurstLength)
        help: qsTr("The length of the burst trails.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: tapBurstLengthSlider
            height: 60
            from: 2; to: 15; stepSize: 1; live: true; showLiveValue: false
            value: ScreensaverConfig.tapBurstLength
            onMoved: ScreensaverConfig.tapBurstLength = value
            onUserInteractionEnded: ScreensaverConfig.tapBurstLength = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Trail length " + value
            KeyNavigation.up: tapBurstCountSlider
            KeyNavigation.down: tapFlashSwitch
        }
    }

    // --- Flash shockwave ---
    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled
        title: qsTr("Flash shockwave")
        showDivider: false

        Components.Switch {
            id: tapFlashSwitch
            objectName: "tapFlashSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapFlash
            trigger: function() { ScreensaverConfig.tapFlash = !ScreensaverConfig.tapFlash; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Flash shockwave"
            KeyNavigation.up: ScreensaverConfig.tapBurst ? tapBurstLengthSlider : tapBurstSwitch
            KeyNavigation.down: tapScrambleSwitch
        }
    }

    // --- Character scramble ---
    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled
        title: qsTr("Character scramble")
        showDivider: false

        Components.Switch {
            id: tapScrambleSwitch
            objectName: "tapScrambleSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapScramble
            trigger: function() { ScreensaverConfig.tapScramble = !ScreensaverConfig.tapScramble; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Character scramble"
            KeyNavigation.up: tapFlashSwitch
            KeyNavigation.down: tapSpawnSwitch
        }
    }

    // --- Stream spawn + sub-settings ---
    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled
        title: qsTr("Stream spawn")
        showDivider: false

        Components.Switch {
            id: tapSpawnSwitch
            objectName: "tapSpawnSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapSpawn
            trigger: function() { ScreensaverConfig.tapSpawn = !ScreensaverConfig.tapSpawn; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Stream spawn"
            KeyNavigation.up: tapScrambleSwitch
            KeyNavigation.down: ScreensaverConfig.tapSpawn ? tapSpawnCountSlider : tapMessageSwitch
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled && ScreensaverConfig.tapSpawn
        title: qsTr("Spawn count")
        value: String(ScreensaverConfig.tapSpawnCount)
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: tapSpawnCountSlider
            height: 60
            from: 2; to: 12; stepSize: 1; live: true; showLiveValue: false
            value: ScreensaverConfig.tapSpawnCount
            onMoved: ScreensaverConfig.tapSpawnCount = value
            onUserInteractionEnded: ScreensaverConfig.tapSpawnCount = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Spawn count " + value
            KeyNavigation.up: tapSpawnSwitch
            KeyNavigation.down: tapSpawnLengthSlider
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled && ScreensaverConfig.tapSpawn
        title: qsTr("Spawn length")
        value: String(ScreensaverConfig.tapSpawnLength)
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: tapSpawnLengthSlider
            height: 60
            from: 3; to: 20; stepSize: 1; live: true; showLiveValue: false
            value: ScreensaverConfig.tapSpawnLength
            onMoved: ScreensaverConfig.tapSpawnLength = value
            onUserInteractionEnded: ScreensaverConfig.tapSpawnLength = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Spawn length " + value
            KeyNavigation.up: tapSpawnCountSlider
            KeyNavigation.down: tapMessageSwitch
        }
    }

    // --- Show message ---
    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled
        title: qsTr("Show message")
        showDivider: false

        Components.Switch {
            id: tapMessageSwitch
            objectName: "tapMessageSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapMessage
            trigger: function() { ScreensaverConfig.tapMessage = !ScreensaverConfig.tapMessage; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Show message"
            KeyNavigation.up: ScreensaverConfig.tapSpawn ? tapSpawnLengthSlider : tapSpawnSwitch
            KeyNavigation.down: tapSquareBurstSwitch
        }
    }

    // --- Square burst + sub-settings ---
    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled
        title: qsTr("Square burst")
        showDivider: false

        Components.Switch {
            id: tapSquareBurstSwitch
            objectName: "tapSquareBurstSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapSquareBurst
            trigger: function() { ScreensaverConfig.tapSquareBurst = !ScreensaverConfig.tapSquareBurst; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Square burst"
            KeyNavigation.up: tapMessageSwitch
            KeyNavigation.down: ScreensaverConfig.tapSquareBurst ? tapSquareBurstSizeSlider : tapRippleSwitch
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled && ScreensaverConfig.tapSquareBurst
        title: qsTr("Square size")
        value: String(ScreensaverConfig.tapSquareBurstSize)
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: tapSquareBurstSizeSlider
            objectName: "tapSquareBurstSizeSlider"
            height: 60
            from: 2; to: 10; stepSize: 1; live: true; showLiveValue: false
            value: ScreensaverConfig.tapSquareBurstSize
            onMoved: ScreensaverConfig.tapSquareBurstSize = value
            onUserInteractionEnded: ScreensaverConfig.tapSquareBurstSize = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Square size " + value
            KeyNavigation.up: tapSquareBurstSwitch
            KeyNavigation.down: tapRippleSwitch
        }
    }

    // --- Ripple ---
    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled
        title: qsTr("Ripple")
        showDivider: false

        Components.Switch {
            id: tapRippleSwitch
            objectName: "tapRippleSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapRipple
            trigger: function() { ScreensaverConfig.tapRipple = !ScreensaverConfig.tapRipple; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Ripple"
            KeyNavigation.up: ScreensaverConfig.tapSquareBurst ? tapSquareBurstSizeSlider : tapSquareBurstSwitch
            KeyNavigation.down: tapWipeSwitch
        }
    }

    // --- Wipe ---
    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled
        title: qsTr("Screen wipe")
        showDivider: false

        Components.Switch {
            id: tapWipeSwitch
            objectName: "tapWipeSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapWipe
            trigger: function() { ScreensaverConfig.tapWipe = !ScreensaverConfig.tapWipe; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Screen wipe"
            KeyNavigation.up: tapRippleSwitch
            KeyNavigation.down: ScreensaverConfig.tapWipe ? tapWipeSpeedSlider : tapRandomizeSwitch
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled && ScreensaverConfig.tapWipe
        title: qsTr("Wipe speed")
        value: ScreensaverConfig.tapWipeSpeed + "%"
        help: qsTr("How fast the wipe crosses the screen. 100% is the normal speed.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: tapWipeSpeedSlider
            objectName: "tapWipeSpeedSlider"
            height: 60
            from: 50; to: 600; stepSize: 25; live: true; showLiveValue: false
            value: ScreensaverConfig.tapWipeSpeed
            onMoved: ScreensaverConfig.tapWipeSpeed = value
            onUserInteractionEnded: ScreensaverConfig.tapWipeSpeed = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Wipe speed " + value + "%"
            KeyNavigation.up: tapWipeSwitch
            KeyNavigation.down: tapRandomizeSwitch
        }
    }

    // TAP RANDOMIZE, and its chance while it is on
    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled
        title: qsTr("Randomize")
        showDivider: !ScreensaverConfig.tapRandomize

        Components.Switch {
            id: tapRandomizeSwitch
            objectName: "tapRandomizeSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapRandomize
            trigger: function() { ScreensaverConfig.tapRandomize = !ScreensaverConfig.tapRandomize; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Randomize"
            KeyNavigation.up: ScreensaverConfig.tapWipe ? tapWipeSpeedSlider : tapWipeSwitch
            KeyNavigation.down: ScreensaverConfig.tapRandomize ? tapRandomizeChanceSlider : root.navDownTarget
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.tapEnabled && ScreensaverConfig.tapRandomize
        title: qsTr("Chance")
        value: ScreensaverConfig.tapRandomizeChance + "%"
        help: qsTr("How likely each effect is to play on a tap.")
        controlBelow: true

        Components.Slider {
            id: tapRandomizeChanceSlider
            objectName: "tapRandomizeChanceSlider"
            height: 60
            from: 10; to: 90; stepSize: 5; live: true; showLiveValue: false
            value: ScreensaverConfig.tapRandomizeChance
            onMoved: ScreensaverConfig.tapRandomizeChance = value
            onUserInteractionEnded: ScreensaverConfig.tapRandomizeChance = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Chance " + value + "%"
            KeyNavigation.up: tapRandomizeSwitch
            KeyNavigation.down: root.navDownTarget
        }
    }
}
