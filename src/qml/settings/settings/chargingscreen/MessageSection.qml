// Copyright (c) 2024 madalone. Subliminal and hidden message settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property Item firstFocusItem: ScreensaverConfig.messages !== "" ? subliminalSwitch : messagesEnabledSwitch
    property alias lastFocusItem: messagePulseSwitch
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // ─────────────────────────────────────────
    // SUBLIMINAL MESSAGES
    // ─────────────────────────────────────────

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium; visible: ScreensaverConfig.messages !== "" }

    // 12p. SUBLIMINAL MASTER TOGGLE
    ColumnLayout {
        visible: ScreensaverConfig.messages !== ""
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.offwhite; text: qsTr("Subliminal messages"); font: fonts.primaryFont(30) }
            Components.Switch {
                id: subliminalSwitch
                objectName: "subliminalSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.subliminal
                trigger: function() { ScreensaverConfig.subliminal = !ScreensaverConfig.subliminal; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Subliminal messages"
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: ScreensaverConfig.subliminal ? subliminalStreamSwitch : messagesInput
            }
        }
    }

    // 12q. IN-STREAM MODE
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.subliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("In-stream injection"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: subliminalStreamSwitch
                objectName: "subliminalStreamSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.subliminalStream
                trigger: function() { ScreensaverConfig.subliminalStream = !ScreensaverConfig.subliminalStream; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "In-stream injection"
                KeyNavigation.up: subliminalSwitch; KeyNavigation.down: subliminalOverlaySwitch
            }
        }
    }

    // 12r. OVERLAY SPANNING MODE
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.subliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Overlay spanning"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: subliminalOverlaySwitch
                objectName: "subliminalOverlaySwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.subliminalOverlay
                trigger: function() { ScreensaverConfig.subliminalOverlay = !ScreensaverConfig.subliminalOverlay; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Overlay spanning"
                KeyNavigation.up: subliminalStreamSwitch; KeyNavigation.down: subliminalFlashSwitch
            }
        }
    }

    // 12s. FLASH MODE
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.subliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Flash mode"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: subliminalFlashSwitch
                objectName: "subliminalFlashSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.subliminalFlash
                trigger: function() { ScreensaverConfig.subliminalFlash = !ScreensaverConfig.subliminalFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Flash mode"
                KeyNavigation.up: subliminalOverlaySwitch; KeyNavigation.down: subliminalIntervalSlider
            }
        }
    }

    // 12t. SUBLIMINAL INTERVAL
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.subliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Interval") + " (" + ScreensaverConfig.subliminalInterval + "s)"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: subliminalIntervalSlider
            objectName: "subliminalIntervalSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 1; to: 30; stepSize: 1; live: true
            value: ScreensaverConfig.subliminalInterval
            onMoved: ScreensaverConfig.subliminalInterval = value
            onUserInteractionEnded: ScreensaverConfig.subliminalInterval = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Interval " + value + "s"
            KeyNavigation.up: subliminalFlashSwitch; KeyNavigation.down: subliminalDurationSlider
        }
    }

    // 12u. SUBLIMINAL DURATION
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.subliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Duration") + " (" + ScreensaverConfig.subliminalDuration + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: subliminalDurationSlider
            objectName: "subliminalDurationSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 40; stepSize: 1; live: true
            value: ScreensaverConfig.subliminalDuration
            onMoved: ScreensaverConfig.subliminalDuration = value
            onUserInteractionEnded: ScreensaverConfig.subliminalDuration = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Duration " + value
            KeyNavigation.up: subliminalIntervalSlider; KeyNavigation.down: messagesInput
        }
    }

    // ─────────────────────────────────────────
    // HIDDEN MESSAGES
    // ─────────────────────────────────────────

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // HIDDEN MESSAGES MASTER TOGGLE
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Hidden messages"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: messagesEnabledSwitch
                objectName: "messagesEnabledSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.messagesEnabled
                trigger: function() { ScreensaverConfig.messagesEnabled = !ScreensaverConfig.messagesEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Hidden messages"
                KeyNavigation.up: (ScreensaverConfig.messages !== "" && ScreensaverConfig.subliminal) ? subliminalDurationSlider : subliminalSwitch
                KeyNavigation.down: ScreensaverConfig.messagesEnabled ? messagesInput : root.navDownTarget
            }
        }
    }

    // 13. MESSAGES TEXT (visible when toggle is on)
    ColumnLayout {
        visible: ScreensaverConfig.messagesEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Components.InputField {
            id: messagesInput
            objectName: "messagesInput"
            Layout.fillWidth: true
            inputField.text: ScreensaverConfig.messages
            inputField.placeholderText: "HELLO, WORLD, WAKE UP"
            inputField.onTextChanged: ScreensaverConfig.messages = inputField.text
            Accessible.name: "Hidden messages"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: messagesEnabledSwitch
            KeyNavigation.down: messageIntervalSlider
        }

    }

    // 13b. MESSAGE INTERVAL
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.messagesEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Message interval") + " (" + ScreensaverConfig.messageInterval + "s)"
            font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: messageIntervalSlider
            objectName: "messageIntervalSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 60; stepSize: 5
            value: ScreensaverConfig.messageInterval; live: true
            onMoved: ScreensaverConfig.messageInterval = value
            onUserInteractionEnded: ScreensaverConfig.messageInterval = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Message interval " + value + "s"
            KeyNavigation.up: messagesInput; KeyNavigation.down: messageRandomSwitch
        }
    }

    // 13c. RANDOM ORDER
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.messagesEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Random order"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: messageRandomSwitch
                objectName: "messageRandomSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.messageRandom
                trigger: function() { ScreensaverConfig.messageRandom = !ScreensaverConfig.messageRandom; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Random order"
                KeyNavigation.up: messageIntervalSlider; KeyNavigation.down: messageDirRow
            }
        }
    }

    // 13d. MESSAGE DIRECTION
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.messagesEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Message direction"); font: fonts.primaryFont(30)
        }

        RowLayout {
            id: messageDirRow
            objectName: "messageDirRow"
            spacing: 8; focus: true
            Accessible.name: "Message direction"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: messageRandomSwitch; KeyNavigation.down: messageFlashSwitch
            Keys.onLeftPressed: root.settingsPage.cycleOption(["horizontal-lr","horizontal-rl","vertical-tb","vertical-bt","stream"], ScreensaverConfig.messageDirection, function(v){ ScreensaverConfig.messageDirection = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["horizontal-lr","horizontal-rl","vertical-tb","vertical-bt","stream"], ScreensaverConfig.messageDirection, function(v){ ScreensaverConfig.messageDirection = v }, 1)
            Repeater {
                model: [
                    { name: "horizontal-lr", label: "H \u2192" },
                    { name: "horizontal-rl", label: "\u2190 H" },
                    { name: "vertical-tb",   label: "V \u2193" },
                    { name: "vertical-bt",   label: "\u2191 V" },
                    { name: "stream",        label: "Rain" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    color: ScreensaverConfig.messageDirection === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: ScreensaverConfig.messageDirection === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: ScreensaverConfig.messageDirection = modelData.name
                    }
                }
            }
        }
    }

    // 13e. SURROUNDING FLASH (visible when messages set)
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.messagesEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Surrounding flash"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: messageFlashSwitch
                objectName: "messageFlashSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.messageFlash
                trigger: function() { ScreensaverConfig.messageFlash = !ScreensaverConfig.messageFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Surrounding flash"
                KeyNavigation.up: messageDirRow; KeyNavigation.down: messagePulseSwitch
            }
        }
    }

    // 13f. BRIGHTNESS PULSE (visible when messages set)
    ColumnLayout {
        visible: ScreensaverConfig.messages !== "" && ScreensaverConfig.messagesEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Brightness pulse"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: messagePulseSwitch
                objectName: "messagePulseSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.messagePulse
                trigger: function() { ScreensaverConfig.messagePulse = !ScreensaverConfig.messagePulse; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Brightness pulse"
                KeyNavigation.up: messageFlashSwitch
                KeyNavigation.down: root.navDownTarget
            }
        }
    }
}
