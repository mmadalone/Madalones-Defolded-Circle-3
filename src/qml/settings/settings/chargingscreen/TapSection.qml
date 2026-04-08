// Copyright (c) 2024 madalone. Tap effect settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: tapBurstSwitch
    property Item lastFocusItem: ScreensaverConfig.tapRandomize ? tapRandomizeChanceSlider : tapRandomizeSwitch
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
                checked: ScreensaverConfig.tapBurst
                trigger: function() { ScreensaverConfig.tapBurst = !ScreensaverConfig.tapBurst; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Scatter burst"
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: ScreensaverConfig.tapBurst ? tapBurstCountSlider : tapFlashSwitch
            }
        }
    }

    // Burst count
    ColumnLayout {
        visible: ScreensaverConfig.tapBurst
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail count") + " (" + ScreensaverConfig.tapBurstCount + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapBurstCountSlider
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 50; stepSize: 5; live: true
            value: ScreensaverConfig.tapBurstCount
            onMoved: ScreensaverConfig.tapBurstCount = value
            onUserInteractionEnded: ScreensaverConfig.tapBurstCount = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Trail count " + value
            KeyNavigation.up: tapBurstSwitch; KeyNavigation.down: tapBurstLengthSlider
        }
    }

    // Burst trail length
    ColumnLayout {
        visible: ScreensaverConfig.tapBurst
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail length") + " (" + ScreensaverConfig.tapBurstLength + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapBurstLengthSlider
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 15; stepSize: 1; live: true
            value: ScreensaverConfig.tapBurstLength
            onMoved: ScreensaverConfig.tapBurstLength = value
            onUserInteractionEnded: ScreensaverConfig.tapBurstLength = value
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
                checked: ScreensaverConfig.tapFlash
                trigger: function() { ScreensaverConfig.tapFlash = !ScreensaverConfig.tapFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Flash shockwave"
                KeyNavigation.up: ScreensaverConfig.tapBurst ? tapBurstLengthSlider : tapBurstSwitch
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
                checked: ScreensaverConfig.tapScramble
                trigger: function() { ScreensaverConfig.tapScramble = !ScreensaverConfig.tapScramble; }
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
                checked: ScreensaverConfig.tapSpawn
                trigger: function() { ScreensaverConfig.tapSpawn = !ScreensaverConfig.tapSpawn; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Stream spawn"
                KeyNavigation.up: tapScrambleSwitch
                KeyNavigation.down: ScreensaverConfig.tapSpawn ? tapSpawnCountSlider : tapMessageSwitch
            }
        }
    }

    // Spawn count
    ColumnLayout {
        visible: ScreensaverConfig.tapSpawn
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Spawn count") + " (" + ScreensaverConfig.tapSpawnCount + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapSpawnCountSlider
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 12; stepSize: 1; live: true
            value: ScreensaverConfig.tapSpawnCount
            onMoved: ScreensaverConfig.tapSpawnCount = value
            onUserInteractionEnded: ScreensaverConfig.tapSpawnCount = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Spawn count " + value
            KeyNavigation.up: tapSpawnSwitch; KeyNavigation.down: tapSpawnLengthSlider
        }
    }

    // Spawn trail length
    ColumnLayout {
        visible: ScreensaverConfig.tapSpawn
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Spawn length") + " (" + ScreensaverConfig.tapSpawnLength + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapSpawnLengthSlider
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 3; to: 20; stepSize: 1; live: true
            value: ScreensaverConfig.tapSpawnLength
            onMoved: ScreensaverConfig.tapSpawnLength = value
            onUserInteractionEnded: ScreensaverConfig.tapSpawnLength = value
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
                checked: ScreensaverConfig.tapMessage
                trigger: function() { ScreensaverConfig.tapMessage = !ScreensaverConfig.tapMessage; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Show message"
                KeyNavigation.up: ScreensaverConfig.tapSpawn ? tapSpawnLengthSlider : tapSpawnSwitch
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
                checked: ScreensaverConfig.tapSquareBurst
                trigger: function() { ScreensaverConfig.tapSquareBurst = !ScreensaverConfig.tapSquareBurst; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Square burst"
                KeyNavigation.up: tapMessageSwitch
                KeyNavigation.down: ScreensaverConfig.tapSquareBurst ? tapSquareBurstSizeSlider : tapRippleSwitch
            }
        }
    }

    // Square burst size
    ColumnLayout {
        visible: ScreensaverConfig.tapSquareBurst
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Square size") + " (" + ScreensaverConfig.tapSquareBurstSize + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapSquareBurstSizeSlider
            objectName: "tapSquareBurstSizeSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 10; stepSize: 1; live: true
            value: ScreensaverConfig.tapSquareBurstSize
            onMoved: ScreensaverConfig.tapSquareBurstSize = value
            onUserInteractionEnded: ScreensaverConfig.tapSquareBurstSize = value
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
                checked: ScreensaverConfig.tapRipple
                trigger: function() { ScreensaverConfig.tapRipple = !ScreensaverConfig.tapRipple; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Ripple"
                KeyNavigation.up: ScreensaverConfig.tapSquareBurst ? tapSquareBurstSizeSlider : tapSquareBurstSwitch
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
                checked: ScreensaverConfig.tapWipe
                trigger: function() { ScreensaverConfig.tapWipe = !ScreensaverConfig.tapWipe; }
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
                checked: ScreensaverConfig.tapRandomize
                trigger: function() { ScreensaverConfig.tapRandomize = !ScreensaverConfig.tapRandomize; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Randomize"
                KeyNavigation.up: tapWipeSwitch
                KeyNavigation.down: ScreensaverConfig.tapRandomize ? tapRandomizeChanceSlider : root.navDownTarget
            }
        }
    }

    // 12o. TAP RANDOMIZE CHANCE
    ColumnLayout {
        visible: ScreensaverConfig.tapRandomize
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 70; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Chance") + " (" + ScreensaverConfig.tapRandomizeChance + "%)"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapRandomizeChanceSlider
            objectName: "tapRandomizeChanceSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 90; stepSize: 5; live: true
            value: ScreensaverConfig.tapRandomizeChance
            onMoved: ScreensaverConfig.tapRandomizeChance = value
            onUserInteractionEnded: ScreensaverConfig.tapRandomizeChance = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Chance " + value + "%"
            KeyNavigation.up: tapRandomizeSwitch; KeyNavigation.down: root.navDownTarget
        }
    }
}
