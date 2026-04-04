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
                KeyNavigation.up: root.navUpTarget; KeyNavigation.down: tapFlashSwitch
            }
        }
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
                KeyNavigation.up: tapBurstSwitch; KeyNavigation.down: tapScrambleSwitch
            }
        }
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
                KeyNavigation.up: tapScrambleSwitch; KeyNavigation.down: tapMessageSwitch
            }
        }
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
                KeyNavigation.up: tapSpawnSwitch; KeyNavigation.down: tapRandomizeSwitch
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
                KeyNavigation.up: tapMessageSwitch
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
