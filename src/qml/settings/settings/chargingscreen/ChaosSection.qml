// Copyright (c) 2024 madalone. Chaos event settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: glitchChaosSwitch
    property alias lastFocusItem: chaosScatterLengthSlider
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // 12h. CHAOS EVENTS
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
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
                checked: Config.chargingMatrixGlitchChaos
                trigger: function() { Config.chargingMatrixGlitchChaos = !Config.chargingMatrixGlitchChaos; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: root.navUpTarget; KeyNavigation.down: chaosFrequencySlider
            }
        }
    }

    // 12i. CHAOS FREQUENCY
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos
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
            value: Config.chargingMatrixGlitchChaosFrequency
            onValueChanged: Config.chargingMatrixGlitchChaosFrequency = value
            onUserInteractionEnded: Config.chargingMatrixGlitchChaosFrequency = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: glitchChaosSwitch; KeyNavigation.down: chaosIntensitySlider
        }
    }

    // 12i2. CHAOS INTENSITY
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos
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
            value: Config.chargingMatrixGlitchChaosIntensity
            onValueChanged: Config.chargingMatrixGlitchChaosIntensity = value
            onUserInteractionEnded: Config.chargingMatrixGlitchChaosIntensity = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: chaosFrequencySlider; KeyNavigation.down: chaosSurgeSwitch
        }
    }

    // 12j. CHAOS SUB-TYPES
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos
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
                checked: Config.chargingMatrixGlitchChaosSurge
                trigger: function() { Config.chargingMatrixGlitchChaosSurge = !Config.chargingMatrixGlitchChaosSurge; }
                highlight: activeFocus && ui.keyNavigationEnabled
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
                checked: Config.chargingMatrixGlitchChaosScramble
                trigger: function() { Config.chargingMatrixGlitchChaosScramble = !Config.chargingMatrixGlitchChaosScramble; }
                highlight: activeFocus && ui.keyNavigationEnabled
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
                checked: Config.chargingMatrixGlitchChaosFreeze
                trigger: function() { Config.chargingMatrixGlitchChaosFreeze = !Config.chargingMatrixGlitchChaosFreeze; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: chaosScrambleSwitch; KeyNavigation.down: chaosScatterSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scatter (burst)"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosScatterSwitch
                objectName: "chaosScatterSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchChaosScatter
                trigger: function() { Config.chargingMatrixGlitchChaosScatter = !Config.chargingMatrixGlitchChaosScatter; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: chaosFreezeSwitch; KeyNavigation.down: chaosScatterRateSlider
            }
        }
    }

    // 12k. SCATTER FREQUENCY
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos && Config.chargingMatrixGlitchChaosScatter
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
            value: Config.chargingMatrixGlitchChaosScatterRate
            onValueChanged: Config.chargingMatrixGlitchChaosScatterRate = value
            onUserInteractionEnded: Config.chargingMatrixGlitchChaosScatterRate = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: chaosScatterSwitch; KeyNavigation.down: chaosScatterLengthSlider
        }
    }

    // 12l. SCATTER TRAIL LENGTH
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos && Config.chargingMatrixGlitchChaosScatter
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
            value: Config.chargingMatrixGlitchChaosScatterLength
            onValueChanged: Config.chargingMatrixGlitchChaosScatterLength = value
            onUserInteractionEnded: Config.chargingMatrixGlitchChaosScatterLength = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: chaosScatterRateSlider; KeyNavigation.down: root.navDownTarget
        }
    }
}
