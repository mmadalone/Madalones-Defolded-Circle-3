// Copyright (c) 2024 madalone. Subliminal and hidden message settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: subliminalSwitch
    property alias lastFocusItem: messagePulseSwitch
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // ─────────────────────────────────────────
    // SUBLIMINAL MESSAGES
    // ─────────────────────────────────────────

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium; visible: Config.chargingMatrixMessages !== "" }

    // 12p. SUBLIMINAL MASTER TOGGLE
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== ""
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
                checked: Config.chargingMatrixSubliminal
                trigger: function() { Config.chargingMatrixSubliminal = !Config.chargingMatrixSubliminal; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Subliminal messages"
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: Config.chargingMatrixSubliminal ? subliminalStreamSwitch : messagesInput
            }
        }
    }

    // 12q. IN-STREAM MODE
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
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
                checked: Config.chargingMatrixSubliminalStream
                trigger: function() { Config.chargingMatrixSubliminalStream = !Config.chargingMatrixSubliminalStream; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "In-stream injection"
                KeyNavigation.up: subliminalSwitch; KeyNavigation.down: subliminalOverlaySwitch
            }
        }
    }

    // 12r. OVERLAY SPANNING MODE
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
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
                checked: Config.chargingMatrixSubliminalOverlay
                trigger: function() { Config.chargingMatrixSubliminalOverlay = !Config.chargingMatrixSubliminalOverlay; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Overlay spanning"
                KeyNavigation.up: subliminalStreamSwitch; KeyNavigation.down: subliminalFlashSwitch
            }
        }
    }

    // 12s. FLASH MODE
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
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
                checked: Config.chargingMatrixSubliminalFlash
                trigger: function() { Config.chargingMatrixSubliminalFlash = !Config.chargingMatrixSubliminalFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Flash mode"
                KeyNavigation.up: subliminalOverlaySwitch; KeyNavigation.down: subliminalIntervalSlider
            }
        }
    }

    // 12t. SUBLIMINAL INTERVAL
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Interval") + " (" + Config.chargingMatrixSubliminalInterval + "s)"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: subliminalIntervalSlider
            objectName: "subliminalIntervalSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 1; to: 30; stepSize: 1; live: true
            value: Config.chargingMatrixSubliminalInterval
            onMoved: Config.chargingMatrixSubliminalInterval = value
            onUserInteractionEnded: Config.chargingMatrixSubliminalInterval = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Interval " + value + "s"
            KeyNavigation.up: subliminalFlashSwitch; KeyNavigation.down: subliminalDurationSlider
        }
    }

    // 12u. SUBLIMINAL DURATION
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Duration") + " (" + Config.chargingMatrixSubliminalDuration + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: subliminalDurationSlider
            objectName: "subliminalDurationSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 40; stepSize: 1; live: true
            value: Config.chargingMatrixSubliminalDuration
            onMoved: Config.chargingMatrixSubliminalDuration = value
            onUserInteractionEnded: Config.chargingMatrixSubliminalDuration = value
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
                checked: Config.chargingMatrixMessagesEnabled
                trigger: function() { Config.chargingMatrixMessagesEnabled = !Config.chargingMatrixMessagesEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Hidden messages"
                KeyNavigation.up: (Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal) ? subliminalDurationSlider : subliminalSwitch
                KeyNavigation.down: Config.chargingMatrixMessagesEnabled ? messagesInput : root.navDownTarget
            }
        }
    }

    // 13. MESSAGES TEXT (visible when toggle is on)
    ColumnLayout {
        visible: Config.chargingMatrixMessagesEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Components.InputField {
            id: messagesInput
            objectName: "messagesInput"
            Layout.fillWidth: true
            inputField.text: Config.chargingMatrixMessages
            inputField.placeholderText: "HELLO, WORLD, WAKE UP"
            inputField.onTextChanged: Config.chargingMatrixMessages = inputField.text
            Accessible.name: "Hidden messages"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: messagesEnabledSwitch
            KeyNavigation.down: messageIntervalSlider
        }

    }

    // 13b. MESSAGE INTERVAL
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixMessagesEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Message interval") + " (" + Config.chargingMatrixMessageInterval + "s)"
            font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: messageIntervalSlider
            objectName: "messageIntervalSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 60; stepSize: 5
            value: Config.chargingMatrixMessageInterval; live: true
            onMoved: Config.chargingMatrixMessageInterval = value
            onUserInteractionEnded: Config.chargingMatrixMessageInterval = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Message interval " + value + "s"
            KeyNavigation.up: messagesInput; KeyNavigation.down: messageRandomSwitch
        }
    }

    // 13c. RANDOM ORDER
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixMessagesEnabled
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
                checked: Config.chargingMatrixMessageRandom
                trigger: function() { Config.chargingMatrixMessageRandom = !Config.chargingMatrixMessageRandom; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Random order"
                KeyNavigation.up: messageIntervalSlider; KeyNavigation.down: messageDirRow
            }
        }
    }

    // 13d. MESSAGE DIRECTION
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixMessagesEnabled
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
            Keys.onLeftPressed: root.settingsPage.cycleOption(["horizontal-lr","horizontal-rl","vertical-tb","vertical-bt","stream"], Config.chargingMatrixMessageDirection, function(v){ Config.chargingMatrixMessageDirection = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["horizontal-lr","horizontal-rl","vertical-tb","vertical-bt","stream"], Config.chargingMatrixMessageDirection, function(v){ Config.chargingMatrixMessageDirection = v }, 1)
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
                    color: Config.chargingMatrixMessageDirection === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: Config.chargingMatrixMessageDirection === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: Config.chargingMatrixMessageDirection = modelData.name
                    }
                }
            }
        }
    }

    // 13e. SURROUNDING FLASH (visible when messages set)
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixMessagesEnabled
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
                checked: Config.chargingMatrixMessageFlash
                trigger: function() { Config.chargingMatrixMessageFlash = !Config.chargingMatrixMessageFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Surrounding flash"
                KeyNavigation.up: messageDirRow; KeyNavigation.down: messagePulseSwitch
            }
        }
    }

    // 13f. BRIGHTNESS PULSE (visible when messages set)
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixMessagesEnabled
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
                checked: Config.chargingMatrixMessagePulse
                trigger: function() { Config.chargingMatrixMessagePulse = !Config.chargingMatrixMessagePulse; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Brightness pulse"
                KeyNavigation.up: messageFlashSwitch
                KeyNavigation.down: root.navDownTarget
            }
        }
    }
}
