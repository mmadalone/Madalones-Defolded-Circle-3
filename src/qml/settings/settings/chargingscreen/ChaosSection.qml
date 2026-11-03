// Copyright (c) 2026 madalone. Chaos event settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: glitchChaosSwitch
    // the last visible control: the scatter switch shows with the events, its two sliders only while it is on
    property Item lastFocusItem: {
        if (!ScreensaverConfig.glitchChaos) return glitchChaosSwitch;
        return ScreensaverConfig.glitchChaosScatter ? chaosScatterLengthSlider : chaosScatterSwitch;
    }
    property Item navUpTarget
    property Item navDownTarget

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true
    readonly property bool eventsShown: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos

    spacing: 0

    // CHAOS EVENTS, and their settings while they are on
    Components.SettingRow {
        visible: ScreensaverConfig.glitch
        title: qsTr("Chaos events")
        showDivider: !ScreensaverConfig.glitchChaos

        Components.Switch {
            id: glitchChaosSwitch
            objectName: "glitchChaosSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchChaos
            trigger: function() { ScreensaverConfig.glitchChaos = !ScreensaverConfig.glitchChaos; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Chaos events"
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: chaosFrequencySlider
        }
    }

    Components.SettingRow {
        visible: root.eventsShown
        title: qsTr("Frequency")
        help: qsTr("How often a chaos event happens.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: chaosFrequencySlider
            objectName: "chaosFrequencySlider"
            height: 60
            from: 5; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchChaosFrequency
            onMoved: ScreensaverConfig.glitchChaosFrequency = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosFrequency = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Frequency " + value
            KeyNavigation.up: glitchChaosSwitch
            KeyNavigation.down: chaosIntensitySlider
        }
    }

    Components.SettingRow {
        visible: root.eventsShown
        title: qsTr("Intensity")
        help: qsTr("How strong each chaos event is.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: chaosIntensitySlider
            objectName: "chaosIntensitySlider"
            height: 60
            from: 5; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchChaosIntensity
            onMoved: ScreensaverConfig.glitchChaosIntensity = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosIntensity = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Intensity " + value
            KeyNavigation.up: chaosFrequencySlider
            KeyNavigation.down: chaosSurgeSwitch
        }
    }

    // CHAOS EVENT TYPES
    Components.SettingRow {
        visible: root.eventsShown
        title: qsTr("Surge (flash)")
        showDivider: false

        Components.Switch {
            id: chaosSurgeSwitch
            objectName: "chaosSurgeSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchChaosSurge
            trigger: function() { ScreensaverConfig.glitchChaosSurge = !ScreensaverConfig.glitchChaosSurge; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Surge (flash)"
            KeyNavigation.up: chaosIntensitySlider
            KeyNavigation.down: chaosScrambleSwitch
        }
    }

    Components.SettingRow {
        visible: root.eventsShown
        title: qsTr("Scramble (mutate)")
        showDivider: false

        Components.Switch {
            id: chaosScrambleSwitch
            objectName: "chaosScrambleSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchChaosScramble
            trigger: function() { ScreensaverConfig.glitchChaosScramble = !ScreensaverConfig.glitchChaosScramble; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Scramble (mutate)"
            KeyNavigation.up: chaosSurgeSwitch
            KeyNavigation.down: chaosFreezeSwitch
        }
    }

    Components.SettingRow {
        visible: root.eventsShown
        title: qsTr("Freeze (stutter)")
        showDivider: false

        Components.Switch {
            id: chaosFreezeSwitch
            objectName: "chaosFreezeSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchChaosFreeze
            trigger: function() { ScreensaverConfig.glitchChaosFreeze = !ScreensaverConfig.glitchChaosFreeze; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Freeze (stutter)"
            KeyNavigation.up: chaosScrambleSwitch
            KeyNavigation.down: chaosSquareBurstSwitch
        }
    }

    // Square burst — ABOVE scatter
    Components.SettingRow {
        visible: root.eventsShown
        title: qsTr("Square burst")
        showDivider: false

        Components.Switch {
            id: chaosSquareBurstSwitch
            objectName: "chaosSquareBurstSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchChaosSquareBurst
            trigger: function() { ScreensaverConfig.glitchChaosSquareBurst = !ScreensaverConfig.glitchChaosSquareBurst; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Square burst"
            KeyNavigation.up: chaosFreezeSwitch
            KeyNavigation.down: ScreensaverConfig.glitchChaosSquareBurst ? chaosSquareBurstSizeSlider : chaosRippleSwitch
        }
    }

    Components.SettingRow {
        visible: root.eventsShown && ScreensaverConfig.glitchChaosSquareBurst
        title: qsTr("Square size")
        value: String(ScreensaverConfig.glitchChaosSquareBurstSize)
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: chaosSquareBurstSizeSlider
            objectName: "chaosSquareBurstSizeSlider"
            height: 60
            from: 2; to: 10; stepSize: 1; live: true; showLiveValue: false
            value: ScreensaverConfig.glitchChaosSquareBurstSize
            onMoved: ScreensaverConfig.glitchChaosSquareBurstSize = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosSquareBurstSize = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Square size " + value
            KeyNavigation.up: chaosSquareBurstSwitch
            KeyNavigation.down: chaosRippleSwitch
        }
    }

    Components.SettingRow {
        visible: root.eventsShown
        title: qsTr("Ripple")
        showDivider: false

        Components.Switch {
            id: chaosRippleSwitch
            objectName: "chaosRippleSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchChaosRipple
            trigger: function() { ScreensaverConfig.glitchChaosRipple = !ScreensaverConfig.glitchChaosRipple; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Ripple"
            KeyNavigation.up: ScreensaverConfig.glitchChaosSquareBurst ? chaosSquareBurstSizeSlider : chaosSquareBurstSwitch
            KeyNavigation.down: chaosWipeSwitch
        }
    }

    Components.SettingRow {
        visible: root.eventsShown
        title: qsTr("Screen wipe")
        showDivider: false

        Components.Switch {
            id: chaosWipeSwitch
            objectName: "chaosWipeSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchChaosWipe
            trigger: function() { ScreensaverConfig.glitchChaosWipe = !ScreensaverConfig.glitchChaosWipe; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Screen wipe"
            KeyNavigation.up: chaosRippleSwitch
            KeyNavigation.down: ScreensaverConfig.glitchChaosWipe ? chaosWipeSpeedSlider : chaosScatterSwitch
        }
    }

    Components.SettingRow {
        visible: root.eventsShown && ScreensaverConfig.glitchChaosWipe
        title: qsTr("Wipe speed")
        value: ScreensaverConfig.glitchChaosWipeSpeed + "%"
        help: qsTr("How fast the wipe crosses the screen. 100% is the normal speed.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: chaosWipeSpeedSlider
            objectName: "chaosWipeSpeedSlider"
            height: 60
            from: 50; to: 600; stepSize: 25; live: true; showLiveValue: false
            value: ScreensaverConfig.glitchChaosWipeSpeed
            onMoved: ScreensaverConfig.glitchChaosWipeSpeed = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosWipeSpeed = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Wipe speed " + value + "%"
            KeyNavigation.up: chaosWipeSwitch
            KeyNavigation.down: chaosScatterSwitch
        }
    }

    Components.SettingRow {
        visible: root.eventsShown
        title: qsTr("Scatter (burst)")
        showDivider: !ScreensaverConfig.glitchChaosScatter

        Components.Switch {
            id: chaosScatterSwitch
            objectName: "chaosScatterSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchChaosScatter
            trigger: function() { ScreensaverConfig.glitchChaosScatter = !ScreensaverConfig.glitchChaosScatter; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Scatter (burst)"
            KeyNavigation.up: ScreensaverConfig.glitchChaosWipe ? chaosWipeSpeedSlider : chaosWipeSwitch
            KeyNavigation.down: ScreensaverConfig.glitchChaosScatter ? chaosScatterRateSlider : root.navDownTarget
        }
    }

    Components.SettingRow {
        visible: root.eventsShown && ScreensaverConfig.glitchChaosScatter
        title: qsTr("Scatter frequency")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: chaosScatterRateSlider
            objectName: "chaosScatterRateSlider"
            height: 60
            from: 5; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchChaosScatterRate
            onMoved: ScreensaverConfig.glitchChaosScatterRate = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosScatterRate = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Scatter frequency " + value
            KeyNavigation.up: chaosScatterSwitch
            KeyNavigation.down: chaosScatterLengthSlider
        }
    }

    Components.SettingRow {
        visible: root.eventsShown && ScreensaverConfig.glitchChaosScatter
        title: qsTr("Scatter trail length")
        controlBelow: true

        Components.Slider {
            id: chaosScatterLengthSlider
            objectName: "chaosScatterLengthSlider"
            height: 60
            from: 3; to: 40; stepSize: 1; live: true
            value: ScreensaverConfig.glitchChaosScatterLength
            onMoved: ScreensaverConfig.glitchChaosScatterLength = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosScatterLength = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Scatter trail length " + value
            KeyNavigation.up: chaosScatterRateSlider
            KeyNavigation.down: root.navDownTarget
        }
    }
}
