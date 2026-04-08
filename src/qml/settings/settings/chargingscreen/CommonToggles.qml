// Copyright (c) 2024 madalone. Common toggles component (clock, battery).
// SPDX-License-Identifier: GPL-3.0-or-later

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

    property Item firstFocusItem: (ScreensaverConfig.theme === "minimal" || ScreensaverConfig.theme === "analog") ? showBatterySwitch : showClockSwitch
    property Item lastFocusItem: ScreensaverConfig.showBatteryEnabled ? batteryDockedSwitch : showBatterySwitch
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // Leading separator
    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 2. SHOW CLOCK (hidden for Minimal — clock is always on)
    ColumnLayout {
        visible: ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; wrapMode: Text.WordWrap; color: colors.offwhite
                text: qsTr("Show clock"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: showClockSwitch
                objectName: "showClockSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.showClock
                trigger: function() { ScreensaverConfig.showClock = !ScreensaverConfig.showClock; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Show clock"
                Component.onCompleted: if (visible) showClockSwitch.forceActiveFocus()
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: ScreensaverConfig.showClock ? clockDockedSwitch : showBatterySwitch
            }
        }
    }

    // 2b. CLOCK CHARGING ONLY (visible when Show clock is on)
    ColumnLayout {
        visible: ScreensaverConfig.showClock && ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Charging only"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: clockDockedSwitch
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.clockDockedOnly
                trigger: function() { ScreensaverConfig.clockDockedOnly = !ScreensaverConfig.clockDockedOnly; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Clock charging only"
                KeyNavigation.up: showClockSwitch; KeyNavigation.down: clockFontRow
            }
        }
    }

    // 2c. CLOCK FONT (visible when Show clock is on)
    ColumnLayout {
        visible: ScreensaverConfig.showClock && ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Font"); font: fonts.primaryFont(26) }
        RowLayout {
            id: clockFontRow
            spacing: 10
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: clockDockedSwitch; KeyNavigation.down: clockColorRow
            Keys.onLeftPressed: root.settingsPage.cycleOption(["primary","secondary"], ScreensaverConfig.clockFont, function(v){ ScreensaverConfig.clockFont = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["primary","secondary"], ScreensaverConfig.clockFont, function(v){ ScreensaverConfig.clockFont = v }, 1)
            Repeater {
                model: [
                    { name: "primary", label: "Poppins" },
                    { name: "secondary", label: "Space Mono" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 40; radius: 8
                    color: ScreensaverConfig.clockFont === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: ScreensaverConfig.clockFont === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: ScreensaverConfig.clockFont = modelData.name
                    }
                }
            }
        }
    }

    // 2d. CLOCK COLOR (visible when Show clock is on)
    ColumnLayout {
        visible: ScreensaverConfig.showClock && ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Color"); font: fonts.primaryFont(26) }
        RowLayout {
            id: clockColorRow
            spacing: 6; focus: true
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: clockFontRow; KeyNavigation.down: clockGradientRow
            Keys.onLeftPressed: {
                var cols = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"];
                root.settingsPage.cycleOption(cols, ScreensaverConfig.clockColor, function(v){ ScreensaverConfig.clockColor = v }, -1);
            }
            Keys.onRightPressed: {
                var cols = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"];
                root.settingsPage.cycleOption(cols, ScreensaverConfig.clockColor, function(v){ ScreensaverConfig.clockColor = v }, 1);
            }
            Repeater {
                model: [
                    { color: "#ffffff" },
                    { color: "#00ff41" },
                    { color: "#00b4d8" },
                    { color: "#ff0040" },
                    { color: "#ffbf00" },
                    { color: "#bf00ff" },
                    { color: "#d0d0d0" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 36; radius: 6
                    color: modelData.color
                    border {
                        color: ScreensaverConfig.clockColor === modelData.color ? colors.offwhite : colors.medium
                        width: ScreensaverConfig.clockColor === modelData.color ? 3 : 1
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: ScreensaverConfig.clockColor = modelData.color
                    }
                }
            }
        }
    }

    // 2d2. CLOCK GRADIENT PRESETS (visible when Show clock is on)
    ColumnLayout {
        visible: ScreensaverConfig.showClock && ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            id: clockGradientRow
            spacing: 6; focus: true
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: clockColorRow; KeyNavigation.down: clockSizeSlider
            Keys.onLeftPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.clockColor, function(v){ ScreensaverConfig.clockColor = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.clockColor, function(v){ ScreensaverConfig.clockColor = v }, 1)
            // Rainbow
            Rectangle {
                Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
                border { color: ScreensaverConfig.clockColor === "rainbow" ? colors.offwhite : colors.medium; width: ScreensaverConfig.clockColor === "rainbow" ? 3 : 1 }
                Rectangle {
                    anchors.fill: parent; anchors.margins: 1; radius: 5
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#ff0000" }
                        GradientStop { position: 0.25; color: "#ffbf00" }
                        GradientStop { position: 0.5; color: "#00ff41" }
                        GradientStop { position: 0.75; color: "#0000ff" }
                        GradientStop { position: 1.0; color: "#ff0000" }
                    }
                }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.clockColor = "rainbow" }
            }
            // Rainbow+
            Rectangle {
                Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
                border { color: ScreensaverConfig.clockColor === "rainbow_gradient" ? colors.offwhite : colors.medium; width: ScreensaverConfig.clockColor === "rainbow_gradient" ? 3 : 1 }
                Rectangle {
                    anchors.fill: parent; anchors.margins: 1; radius: 5
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#ff0000" }
                        GradientStop { position: 0.2; color: "#ffff00" }
                        GradientStop { position: 0.4; color: "#00ff80" }
                        GradientStop { position: 0.6; color: "#0080ff" }
                        GradientStop { position: 0.8; color: "#8000ff" }
                        GradientStop { position: 1.0; color: "#ff0000" }
                    }
                }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.clockColor = "rainbow_gradient" }
            }
            // Neon
            Rectangle {
                Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
                border { color: ScreensaverConfig.clockColor === "neon" ? colors.offwhite : colors.medium; width: ScreensaverConfig.clockColor === "neon" ? 3 : 1 }
                Rectangle {
                    anchors.fill: parent; anchors.margins: 1; radius: 5
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#ff8080" }
                        GradientStop { position: 0.2; color: "#ffff80" }
                        GradientStop { position: 0.4; color: "#80ffd0" }
                        GradientStop { position: 0.6; color: "#80d0ff" }
                        GradientStop { position: 0.8; color: "#d080ff" }
                        GradientStop { position: 1.0; color: "#ff8080" }
                    }
                }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.clockColor = "neon" }
            }
        }
    }

    // 2e. CLOCK SIZE (visible when Show clock is on)
    ColumnLayout {
        visible: ScreensaverConfig.showClock && ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Size"); font: fonts.primaryFont(26) }
        Components.Slider {
            id: clockSizeSlider
            height: 60; Layout.fillWidth: true
            from: 24; to: 96; stepSize: 4
            value: ScreensaverConfig.clockSize; live: true
            onMoved: ScreensaverConfig.clockSize = value
            onUserInteractionEnded: ScreensaverConfig.clockSize = value
            highlight: activeFocus && ui.keyNavigationEnabled
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: clockGradientRow; KeyNavigation.down: clock24hSwitch
        }
    }

    // 2f. CLOCK 24H TOGGLE
    ColumnLayout {
        visible: ScreensaverConfig.showClock && ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("24-hour clock"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: clock24hSwitch
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.clockClock24h
                trigger: function() { ScreensaverConfig.clockClock24h = !ScreensaverConfig.clockClock24h; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: clockSizeSlider; KeyNavigation.down: clockShowDateSwitch
            }
        }
    }

    // 2g. SHOW DATE TOGGLE
    ColumnLayout {
        visible: ScreensaverConfig.showClock && ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Show date"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: clockShowDateSwitch
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.clockShowDate
                trigger: function() { ScreensaverConfig.clockShowDate = !ScreensaverConfig.clockShowDate; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: clock24hSwitch
                KeyNavigation.down: ScreensaverConfig.clockShowDate ? clockDateSizeSlider : showBatterySwitch
            }
        }
    }

    // 2h. DATE SIZE (visible when show date is on)
    ColumnLayout {
        visible: ScreensaverConfig.showClock && ScreensaverConfig.clockShowDate && ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Date size"); font: fonts.primaryFont(26) }
        Components.Slider {
            id: clockDateSizeSlider
            height: 60; Layout.fillWidth: true
            from: 12; to: 40; stepSize: 2
            value: ScreensaverConfig.clockDateSize; live: true
            onMoved: ScreensaverConfig.clockDateSize = value
            onUserInteractionEnded: ScreensaverConfig.clockDateSize = value
            highlight: activeFocus && ui.keyNavigationEnabled
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: clockShowDateSwitch; KeyNavigation.down: showBatterySwitch
        }
    }

    Rectangle { visible: ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog"; Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 3. SHOW BATTERY
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; wrapMode: Text.WordWrap; color: colors.offwhite
                text: qsTr("Show battery"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: showBatterySwitch
                objectName: "showBatterySwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.showBatteryEnabled
                trigger: function() { ScreensaverConfig.showBatteryEnabled = !ScreensaverConfig.showBatteryEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Show battery"
                KeyNavigation.up: (ScreensaverConfig.theme !== "minimal" && ScreensaverConfig.theme !== "analog")
                    ? (ScreensaverConfig.showClock
                        ? (ScreensaverConfig.clockShowDate ? clockDateSizeSlider : clockShowDateSwitch)
                        : showClockSwitch)
                    : root.navUpTarget
                KeyNavigation.down: batteryDockedSwitch
            }
        }
    }

    // 3b. BATTERY DOCKED ONLY (visible when Show battery is on)
    ColumnLayout {
        visible: ScreensaverConfig.showBatteryEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.light
                text: qsTr("Charging only"); font: fonts.primaryFont(26)
            }
            Components.Switch {
                id: batteryDockedSwitch
                objectName: "batteryDockedSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.batteryDockedOnly
                trigger: function() { ScreensaverConfig.batteryDockedOnly = !ScreensaverConfig.batteryDockedOnly; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Charging only"
                KeyNavigation.up: showBatterySwitch
                KeyNavigation.down: root.navDownTarget
            }
        }
    }
}
