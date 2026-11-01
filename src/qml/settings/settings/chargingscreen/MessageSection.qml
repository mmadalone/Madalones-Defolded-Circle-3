// Copyright (c) 2026 madalone. Subliminal and hidden message settings. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property Item firstFocusItem: ScreensaverConfig.messages !== "" ? subliminalSwitch : messagesEnabledSwitch
    property alias lastFocusItem: messagePulseSwitch
    property Item navUpTarget
    property Item navDownTarget

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true
    readonly property bool subliminalShown: ScreensaverConfig.messages !== "" && ScreensaverConfig.subliminal
    readonly property bool messageOptionsShown: ScreensaverConfig.messages !== "" && ScreensaverConfig.messagesEnabled

    spacing: 0

    // ─────────────────────────────────────────
    // SUBLIMINAL MESSAGES
    // ─────────────────────────────────────────

    Components.SettingRow {
        visible: ScreensaverConfig.messages !== ""
        title: qsTr("Subliminal messages")
        showDivider: !ScreensaverConfig.subliminal

        Components.Switch {
            id: subliminalSwitch
            objectName: "subliminalSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.subliminal
            trigger: function() { ScreensaverConfig.subliminal = !ScreensaverConfig.subliminal; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Subliminal messages"
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: ScreensaverConfig.subliminal ? subliminalStreamSwitch : messagesEnabledSwitch
        }
    }

    Components.SettingRow {
        visible: root.subliminalShown
        title: qsTr("In-stream injection")
        showDivider: false

        Components.Switch {
            id: subliminalStreamSwitch
            objectName: "subliminalStreamSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.subliminalStream
            trigger: function() { ScreensaverConfig.subliminalStream = !ScreensaverConfig.subliminalStream; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "In-stream injection"
            KeyNavigation.up: subliminalSwitch
            KeyNavigation.down: subliminalOverlaySwitch
        }
    }

    Components.SettingRow {
        visible: root.subliminalShown
        title: qsTr("Overlay spanning")
        showDivider: false

        Components.Switch {
            id: subliminalOverlaySwitch
            objectName: "subliminalOverlaySwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.subliminalOverlay
            trigger: function() { ScreensaverConfig.subliminalOverlay = !ScreensaverConfig.subliminalOverlay; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Overlay spanning"
            KeyNavigation.up: subliminalStreamSwitch
            KeyNavigation.down: subliminalFlashSwitch
        }
    }

    Components.SettingRow {
        visible: root.subliminalShown
        title: qsTr("Flash mode")
        showDivider: false

        Components.Switch {
            id: subliminalFlashSwitch
            objectName: "subliminalFlashSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.subliminalFlash
            trigger: function() { ScreensaverConfig.subliminalFlash = !ScreensaverConfig.subliminalFlash; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Flash mode"
            KeyNavigation.up: subliminalOverlaySwitch
            KeyNavigation.down: subliminalIntervalSlider
        }
    }

    Components.SettingRow {
        visible: root.subliminalShown
        title: qsTr("Interval")
        value: ScreensaverConfig.subliminalInterval + "s"
        help: qsTr("The time between two subliminal messages.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: subliminalIntervalSlider
            objectName: "subliminalIntervalSlider"
            height: 60
            from: 1; to: 30; stepSize: 1; live: true; showLiveValue: false
            value: ScreensaverConfig.subliminalInterval
            onMoved: ScreensaverConfig.subliminalInterval = value
            onUserInteractionEnded: ScreensaverConfig.subliminalInterval = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Interval " + value + "s"
            KeyNavigation.up: subliminalFlashSwitch
            KeyNavigation.down: subliminalDurationSlider
        }
    }

    Components.SettingRow {
        visible: root.subliminalShown
        title: qsTr("Duration")
        value: String(ScreensaverConfig.subliminalDuration)
        help: qsTr("How long a subliminal message stays visible.")
        controlBelow: true

        Components.Slider {
            id: subliminalDurationSlider
            objectName: "subliminalDurationSlider"
            height: 60
            from: 2; to: 40; stepSize: 1; live: true; showLiveValue: false
            value: ScreensaverConfig.subliminalDuration
            onMoved: ScreensaverConfig.subliminalDuration = value
            onUserInteractionEnded: ScreensaverConfig.subliminalDuration = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Duration " + value
            KeyNavigation.up: subliminalIntervalSlider
            KeyNavigation.down: messagesEnabledSwitch
        }
    }

    // ─────────────────────────────────────────
    // HIDDEN MESSAGES
    // ─────────────────────────────────────────

    Components.SettingRow {
        title: qsTr("Hidden messages")
        showDivider: !ScreensaverConfig.messagesEnabled

        Components.Switch {
            id: messagesEnabledSwitch
            objectName: "messagesEnabledSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.messagesEnabled
            trigger: function() { ScreensaverConfig.messagesEnabled = !ScreensaverConfig.messagesEnabled; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Hidden messages"
            KeyNavigation.up: (ScreensaverConfig.messages !== "" && ScreensaverConfig.subliminal) ? subliminalDurationSlider : subliminalSwitch
            KeyNavigation.down: ScreensaverConfig.messagesEnabled ? messagesInput : root.navDownTarget
        }
    }

    // the messages (typing is touch only: the keypad does not operate the on-screen keyboard)
    Components.SettingRow {
        visible: ScreensaverConfig.messagesEnabled
        controlBelow: true
        showDivider: ScreensaverConfig.messages === ""

        Components.InputField {
            id: messagesInput
            objectName: "messagesInput"
            inputField.text: ScreensaverConfig.messages
            inputField.placeholderText: "HELLO, WORLD, WAKE UP"
            inputField.onTextChanged: ScreensaverConfig.messages = inputField.text
            Accessible.name: "Hidden messages"
            KeyNavigation.up: messagesEnabledSwitch
            KeyNavigation.down: messageIntervalSlider

            // the field's own ring follows its text input, which only takes the focus on a tap; the keypad selects
            // the field as a whole
            Components.Selectable {
                selected: messagesInput.activeFocus
                radius: messagesInput.radius
            }
        }
    }

    Components.SettingRow {
        visible: root.messageOptionsShown
        title: qsTr("Message interval")
        value: ScreensaverConfig.messageInterval + "s"
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: messageIntervalSlider
            objectName: "messageIntervalSlider"
            height: 60
            from: 5; to: 60; stepSize: 5
            value: ScreensaverConfig.messageInterval; live: true; showLiveValue: false
            onMoved: ScreensaverConfig.messageInterval = value
            onUserInteractionEnded: ScreensaverConfig.messageInterval = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Message interval " + value + "s"
            KeyNavigation.up: messagesInput
            KeyNavigation.down: messageRandomSwitch
        }
    }

    Components.SettingRow {
        visible: root.messageOptionsShown
        title: qsTr("Random order")
        showDivider: false

        Components.Switch {
            id: messageRandomSwitch
            objectName: "messageRandomSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.messageRandom
            trigger: function() { ScreensaverConfig.messageRandom = !ScreensaverConfig.messageRandom; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Random order"
            KeyNavigation.up: messageIntervalSlider
            KeyNavigation.down: messageDirRow
        }
    }

    Components.SettingRow {
        visible: root.messageOptionsShown
        title: qsTr("Message direction")
        controlBelow: true
        showDivider: false

        Components.ChipPicker {
            id: messageDirRow
            objectName: "messageDirRow"
            Accessible.name: "Message direction"
            columns: 5
            model: [
                { value: "horizontal-lr", label: "H →" },
                { value: "horizontal-rl", label: "← H" },
                { value: "vertical-tb",   label: "V ↓" },
                { value: "vertical-bt",   label: "↑ V" },
                //: The message runs down the rain streams
                { value: "stream",        label: qsTr("Rain") }
            ]
            currentValue: ScreensaverConfig.messageDirection
            onPicked: ScreensaverConfig.messageDirection = value
            KeyNavigation.up: messageRandomSwitch
            KeyNavigation.down: messageFlashSwitch
        }
    }

    Components.SettingRow {
        visible: root.messageOptionsShown
        title: qsTr("Surrounding flash")
        showDivider: false

        Components.Switch {
            id: messageFlashSwitch
            objectName: "messageFlashSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.messageFlash
            trigger: function() { ScreensaverConfig.messageFlash = !ScreensaverConfig.messageFlash; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Surrounding flash"
            KeyNavigation.up: messageDirRow
            KeyNavigation.down: messagePulseSwitch
        }
    }

    Components.SettingRow {
        visible: root.messageOptionsShown
        title: qsTr("Brightness pulse")

        Components.Switch {
            id: messagePulseSwitch
            objectName: "messagePulseSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.messagePulse
            trigger: function() { ScreensaverConfig.messagePulse = !ScreensaverConfig.messagePulse; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Brightness pulse"
            KeyNavigation.up: messageFlashSwitch
            KeyNavigation.down: root.navDownTarget
        }
    }
}
