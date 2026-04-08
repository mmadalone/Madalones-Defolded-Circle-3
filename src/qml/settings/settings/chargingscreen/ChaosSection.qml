// Copyright (c) 2026 madalone. Chaos event settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: glitchChaosSwitch
    property Item lastFocusItem: {
        if (ScreensaverConfig.glitchChaos && ScreensaverConfig.glitchChaosScatter) return chaosScatterLengthSlider;
        return chaosWipeSwitch;  // fallback — last always-visible toggle when scatter is off
    }
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // 12h. CHAOS EVENTS
    ColumnLayout {
        visible: ScreensaverConfig.glitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Chaos events"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchChaosSwitch
                objectName: "glitchChaosSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchChaos
                trigger: function() { ScreensaverConfig.glitchChaos = !ScreensaverConfig.glitchChaos; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Chaos events"
                KeyNavigation.up: root.navUpTarget; KeyNavigation.down: chaosFrequencySlider
            }
        }
    }

    // 12i. CHAOS FREQUENCY
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Frequency"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: chaosFrequencySlider
            objectName: "chaosFrequencySlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchChaosFrequency
            onMoved: ScreensaverConfig.glitchChaosFrequency = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosFrequency = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Frequency " + value
            KeyNavigation.up: glitchChaosSwitch; KeyNavigation.down: chaosIntensitySlider
        }
    }

    // 12i2. CHAOS INTENSITY
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Intensity"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: chaosIntensitySlider
            objectName: "chaosIntensitySlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchChaosIntensity
            onMoved: ScreensaverConfig.glitchChaosIntensity = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosIntensity = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Intensity " + value
            KeyNavigation.up: chaosFrequencySlider; KeyNavigation.down: chaosSurgeSwitch
        }
    }

    // 12j. CHAOS SUB-TYPES
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Surge (flash)"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosSurgeSwitch
                objectName: "chaosSurgeSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchChaosSurge
                trigger: function() { ScreensaverConfig.glitchChaosSurge = !ScreensaverConfig.glitchChaosSurge; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Surge (flash)"
                KeyNavigation.up: chaosIntensitySlider; KeyNavigation.down: chaosScrambleSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scramble (mutate)"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosScrambleSwitch
                objectName: "chaosScrambleSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchChaosScramble
                trigger: function() { ScreensaverConfig.glitchChaosScramble = !ScreensaverConfig.glitchChaosScramble; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Scramble (mutate)"
                KeyNavigation.up: chaosSurgeSwitch; KeyNavigation.down: chaosFreezeSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Freeze (stutter)"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosFreezeSwitch
                objectName: "chaosFreezeSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchChaosFreeze
                trigger: function() { ScreensaverConfig.glitchChaosFreeze = !ScreensaverConfig.glitchChaosFreeze; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Freeze (stutter)"
                KeyNavigation.up: chaosScrambleSwitch; KeyNavigation.down: chaosSquareBurstSwitch
            }
        }
        // Square burst — ABOVE scatter
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Square burst"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosSquareBurstSwitch
                objectName: "chaosSquareBurstSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchChaosSquareBurst
                trigger: function() { ScreensaverConfig.glitchChaosSquareBurst = !ScreensaverConfig.glitchChaosSquareBurst; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Square burst"
                KeyNavigation.up: chaosFreezeSwitch
                KeyNavigation.down: ScreensaverConfig.glitchChaosSquareBurst ? chaosSquareBurstSizeSlider : chaosRippleSwitch
            }
        }
    }

    // 12j2. SQUARE BURST SIZE (visible when square burst is on)
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos && ScreensaverConfig.glitchChaosSquareBurst
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 70; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Square size") + " (" + ScreensaverConfig.glitchChaosSquareBurstSize + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: chaosSquareBurstSizeSlider
            objectName: "chaosSquareBurstSizeSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 10; stepSize: 1; live: true
            value: ScreensaverConfig.glitchChaosSquareBurstSize
            onMoved: ScreensaverConfig.glitchChaosSquareBurstSize = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosSquareBurstSize = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Square size " + value
            KeyNavigation.up: chaosSquareBurstSwitch; KeyNavigation.down: chaosRippleSwitch
        }
    }

    // 12j3. Ripple
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Ripple"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosRippleSwitch
                objectName: "chaosRippleSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchChaosRipple
                trigger: function() { ScreensaverConfig.glitchChaosRipple = !ScreensaverConfig.glitchChaosRipple; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Ripple"
                KeyNavigation.up: ScreensaverConfig.glitchChaosSquareBurst ? chaosSquareBurstSizeSlider : chaosSquareBurstSwitch
                KeyNavigation.down: chaosWipeSwitch
            }
        }
    }

    // 12j4. Screen wipe
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Screen wipe"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosWipeSwitch
                objectName: "chaosWipeSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchChaosWipe
                trigger: function() { ScreensaverConfig.glitchChaosWipe = !ScreensaverConfig.glitchChaosWipe; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Screen wipe"
                KeyNavigation.up: chaosRippleSwitch; KeyNavigation.down: chaosScatterSwitch
            }
        }
    }

    // 12j5. Scatter (burst)
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scatter (burst)"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosScatterSwitch
                objectName: "chaosScatterSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.glitchChaosScatter
                trigger: function() { ScreensaverConfig.glitchChaosScatter = !ScreensaverConfig.glitchChaosScatter; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Scatter (burst)"
                KeyNavigation.up: chaosWipeSwitch
                KeyNavigation.down: ScreensaverConfig.glitchChaosScatter ? chaosScatterRateSlider : root.navDownTarget
            }
        }
    }

    // 12k. SCATTER FREQUENCY
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos && ScreensaverConfig.glitchChaosScatter
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 70; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scatter frequency"); font: fonts.primaryFont(22) }
        Components.Slider {
            id: chaosScatterRateSlider
            objectName: "chaosScatterRateSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 100; stepSize: 5; live: true
            value: ScreensaverConfig.glitchChaosScatterRate
            onMoved: ScreensaverConfig.glitchChaosScatterRate = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosScatterRate = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Scatter frequency " + value
            KeyNavigation.up: chaosScatterSwitch; KeyNavigation.down: chaosScatterLengthSlider
        }
    }

    // 12l. SCATTER TRAIL LENGTH
    ColumnLayout {
        visible: ScreensaverConfig.glitch && ScreensaverConfig.glitchChaos && ScreensaverConfig.glitchChaosScatter
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 70; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scatter trail length"); font: fonts.primaryFont(22) }
        Components.Slider {
            id: chaosScatterLengthSlider
            objectName: "chaosScatterLengthSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 3; to: 40; stepSize: 1; live: true
            value: ScreensaverConfig.glitchChaosScatterLength
            onMoved: ScreensaverConfig.glitchChaosScatterLength = value
            onUserInteractionEnded: ScreensaverConfig.glitchChaosScatterLength = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Scatter trail length " + value
            KeyNavigation.up: chaosScatterRateSlider; KeyNavigation.down: root.navDownTarget
        }
    }
}
