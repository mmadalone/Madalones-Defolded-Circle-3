// Copyright (c) 2024 madalone. Tap effect settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: tapBurstSwitch
    property Item lastFocusItem: Config.chargingMatrixTapRandomize ? tapRandomizeChanceSlider : tapRandomizeSwitch
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // Leading separator
    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 12m. TAP EFFECTS
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Tap effects"); font: fonts.primaryFont(30)
        }

        // --- Scatter burst + sub-settings ---
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scatter burst"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapBurstSwitch
                objectName: "tapBurstSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapBurst
                trigger: function() { Config.chargingMatrixTapBurst = !Config.chargingMatrixTapBurst; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Scatter burst"
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: Config.chargingMatrixTapBurst ? tapBurstCountSlider : tapFlashSwitch
            }
        }
    }

    // Burst count
    ColumnLayout {
        visible: Config.chargingMatrixTapBurst
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail count") + " (" + Config.chargingMatrixTapBurstCount + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapBurstCountSlider
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 50; stepSize: 5; live: true
            value: Config.chargingMatrixTapBurstCount
            onValueChanged: Config.chargingMatrixTapBurstCount = value
            onUserInteractionEnded: Config.chargingMatrixTapBurstCount = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Trail count " + value
            KeyNavigation.up: tapBurstSwitch; KeyNavigation.down: tapBurstLengthSlider
        }
    }

    // Burst trail length
    ColumnLayout {
        visible: Config.chargingMatrixTapBurst
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail length") + " (" + Config.chargingMatrixTapBurstLength + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapBurstLengthSlider
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 15; stepSize: 1; live: true
            value: Config.chargingMatrixTapBurstLength
            onValueChanged: Config.chargingMatrixTapBurstLength = value
            onUserInteractionEnded: Config.chargingMatrixTapBurstLength = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Trail length " + value
            KeyNavigation.up: tapBurstCountSlider; KeyNavigation.down: tapFlashSwitch
        }
    }

    // --- Flash shockwave ---
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Flash shockwave"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapFlashSwitch
                objectName: "tapFlashSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapFlash
                trigger: function() { Config.chargingMatrixTapFlash = !Config.chargingMatrixTapFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Flash shockwave"
                KeyNavigation.up: Config.chargingMatrixTapBurst ? tapBurstLengthSlider : tapBurstSwitch
                KeyNavigation.down: tapScrambleSwitch
            }
        }

        // --- Character scramble ---
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Character scramble"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapScrambleSwitch
                objectName: "tapScrambleSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapScramble
                trigger: function() { Config.chargingMatrixTapScramble = !Config.chargingMatrixTapScramble; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Character scramble"
                KeyNavigation.up: tapFlashSwitch; KeyNavigation.down: tapSpawnSwitch
            }
        }

        // --- Stream spawn + sub-settings ---
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Stream spawn"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapSpawnSwitch
                objectName: "tapSpawnSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapSpawn
                trigger: function() { Config.chargingMatrixTapSpawn = !Config.chargingMatrixTapSpawn; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Stream spawn"
                KeyNavigation.up: tapScrambleSwitch
                KeyNavigation.down: Config.chargingMatrixTapSpawn ? tapSpawnCountSlider : tapMessageSwitch
            }
        }
    }

    // Spawn count
    ColumnLayout {
        visible: Config.chargingMatrixTapSpawn
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Spawn count") + " (" + Config.chargingMatrixTapSpawnCount + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapSpawnCountSlider
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 12; stepSize: 1; live: true
            value: Config.chargingMatrixTapSpawnCount
            onValueChanged: Config.chargingMatrixTapSpawnCount = value
            onUserInteractionEnded: Config.chargingMatrixTapSpawnCount = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Spawn count " + value
            KeyNavigation.up: tapSpawnSwitch; KeyNavigation.down: tapSpawnLengthSlider
        }
    }

    // Spawn trail length
    ColumnLayout {
        visible: Config.chargingMatrixTapSpawn
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Spawn length") + " (" + Config.chargingMatrixTapSpawnLength + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapSpawnLengthSlider
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 3; to: 20; stepSize: 1; live: true
            value: Config.chargingMatrixTapSpawnLength
            onValueChanged: Config.chargingMatrixTapSpawnLength = value
            onUserInteractionEnded: Config.chargingMatrixTapSpawnLength = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Spawn length " + value
            KeyNavigation.up: tapSpawnCountSlider; KeyNavigation.down: tapMessageSwitch
        }
    }

    // --- Show message ---
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Show message"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapMessageSwitch
                objectName: "tapMessageSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapMessage
                trigger: function() { Config.chargingMatrixTapMessage = !Config.chargingMatrixTapMessage; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Show message"
                KeyNavigation.up: Config.chargingMatrixTapSpawn ? tapSpawnLengthSlider : tapSpawnSwitch
                KeyNavigation.down: tapSquareBurstSwitch
            }
        }

        // --- Square burst + sub-settings ---
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Square burst"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapSquareBurstSwitch
                objectName: "tapSquareBurstSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapSquareBurst
                trigger: function() { Config.chargingMatrixTapSquareBurst = !Config.chargingMatrixTapSquareBurst; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Square burst"
                KeyNavigation.up: tapMessageSwitch
                KeyNavigation.down: Config.chargingMatrixTapSquareBurst ? tapSquareBurstSizeSlider : tapRippleSwitch
            }
        }
    }

    // Square burst size
    ColumnLayout {
        visible: Config.chargingMatrixTapSquareBurst
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Square size") + " (" + Config.chargingMatrixTapSquareBurstSize + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapSquareBurstSizeSlider
            objectName: "tapSquareBurstSizeSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 10; stepSize: 1; live: true
            value: Config.chargingMatrixTapSquareBurstSize
            onValueChanged: Config.chargingMatrixTapSquareBurstSize = value
            onUserInteractionEnded: Config.chargingMatrixTapSquareBurstSize = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Square size " + value
            KeyNavigation.up: tapSquareBurstSwitch; KeyNavigation.down: tapRippleSwitch
        }
    }

    // --- Ripple ---
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Ripple"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapRippleSwitch
                objectName: "tapRippleSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapRipple
                trigger: function() { Config.chargingMatrixTapRipple = !Config.chargingMatrixTapRipple; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Ripple"
                KeyNavigation.up: Config.chargingMatrixTapSquareBurst ? tapSquareBurstSizeSlider : tapSquareBurstSwitch
                KeyNavigation.down: tapWipeSwitch
            }
        }

        // --- Wipe ---
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Screen wipe"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapWipeSwitch
                objectName: "tapWipeSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapWipe
                trigger: function() { Config.chargingMatrixTapWipe = !Config.chargingMatrixTapWipe; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Screen wipe"
                KeyNavigation.up: tapRippleSwitch; KeyNavigation.down: tapRandomizeSwitch
            }
        }
    }

    // 12n. TAP RANDOMIZE
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Randomize"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapRandomizeSwitch
                objectName: "tapRandomizeSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapRandomize
                trigger: function() { Config.chargingMatrixTapRandomize = !Config.chargingMatrixTapRandomize; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Randomize"
                KeyNavigation.up: tapWipeSwitch
                KeyNavigation.down: Config.chargingMatrixTapRandomize ? tapRandomizeChanceSlider : root.navDownTarget
            }
        }
    }

    // 12o. TAP RANDOMIZE CHANCE
    ColumnLayout {
        visible: Config.chargingMatrixTapRandomize
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 70; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Chance") + " (" + Config.chargingMatrixTapRandomizeChance + "%)"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapRandomizeChanceSlider
            objectName: "tapRandomizeChanceSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 90; stepSize: 5; live: true
            value: Config.chargingMatrixTapRandomizeChance
            onValueChanged: Config.chargingMatrixTapRandomizeChance = value
            onUserInteractionEnded: Config.chargingMatrixTapRandomizeChance = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Chance " + value + "%"
            KeyNavigation.up: tapRandomizeSwitch; KeyNavigation.down: root.navDownTarget
        }
    }
}
