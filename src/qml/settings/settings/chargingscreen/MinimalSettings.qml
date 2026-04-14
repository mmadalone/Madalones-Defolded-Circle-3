// Copyright (c) 2026 madalone. Minimal clock theme settings (extracted from ChargingScreen.qml).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage
    property Item navUpTarget
    property Item navDownTarget

    property alias firstFocusItem: minimalClock24hSwitch
    property alias lastFocusItem: minimalDateSlider

    Layout.fillWidth: true
    Layout.leftMargin: 10; Layout.rightMargin: 10
    spacing: 20

    RowLayout {
        spacing: 10
        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("24-hour clock"); font: fonts.primaryFont(30)
        }
        Components.Switch {
            id: minimalClock24hSwitch
            icon: "uc:check"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            checked: ScreensaverConfig.minimalClock24h
            trigger: function() { ScreensaverConfig.minimalClock24h = !ScreensaverConfig.minimalClock24h; }
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: minimalFontRow
        }
    }

    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Font"); font: fonts.primaryFont(30)
    }
    RowLayout {
        id: minimalFontRow
        spacing: 10
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: minimalClock24hSwitch
        KeyNavigation.down: minimalTimeColorSolidRow
        Keys.onLeftPressed: root.settingsPage.cycleOption(["primary","secondary"], ScreensaverConfig.minimalFont, function(v){ ScreensaverConfig.minimalFont = v }, -1)
        Keys.onRightPressed: root.settingsPage.cycleOption(["primary","secondary"], ScreensaverConfig.minimalFont, function(v){ ScreensaverConfig.minimalFont = v }, 1)
        Repeater {
            model: [
                { name: "primary", label: "Poppins" },
                { name: "secondary", label: "Space Mono" }
            ]
            Rectangle {
                Layout.fillWidth: true; height: 50; radius: 8
                color: ScreensaverConfig.minimalFont === modelData.name ? colors.offwhite : colors.dark
                border { color: colors.medium; width: 1 }
                Text {
                    anchors.centerIn: parent; text: modelData.label
                    color: ScreensaverConfig.minimalFont === modelData.name ? colors.black : colors.offwhite
                    font: fonts.primaryFont(24)
                }
                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: ScreensaverConfig.minimalFont = modelData.name
                }
            }
        }
    }

    // Time color
    Text { Layout.fillWidth: true; color: colors.offwhite; text: qsTr("Time color"); font: fonts.primaryFont(30) }
    RowLayout {
        id: minimalTimeColorSolidRow
        spacing: 6; focus: true
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: minimalFontRow; KeyNavigation.down: minimalTimeGradientRow
        Keys.onLeftPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; root.settingsPage.cycleOption(c, ScreensaverConfig.minimalTimeColor, function(v){ ScreensaverConfig.minimalTimeColor = v }, -1) }
        Keys.onRightPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; root.settingsPage.cycleOption(c, ScreensaverConfig.minimalTimeColor, function(v){ ScreensaverConfig.minimalTimeColor = v }, 1) }
        Repeater {
            model: [{ color: "#ffffff" },{ color: "#00ff41" },{ color: "#00b4d8" },{ color: "#ff0040" },{ color: "#ffbf00" },{ color: "#bf00ff" },{ color: "#d0d0d0" }]
            Rectangle {
                Layout.fillWidth: true; height: 36; radius: 6; color: modelData.color
                border { color: ScreensaverConfig.minimalTimeColor === modelData.color ? colors.offwhite : colors.medium; width: ScreensaverConfig.minimalTimeColor === modelData.color ? 3 : 1 }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.minimalTimeColor = modelData.color }
            }
        }
    }
    RowLayout {
        id: minimalTimeGradientRow
        spacing: 6; focus: true
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: minimalTimeColorSolidRow; KeyNavigation.down: minimalDateColorSolidRow
        Keys.onLeftPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.minimalTimeColor, function(v){ ScreensaverConfig.minimalTimeColor = v }, -1)
        Keys.onRightPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.minimalTimeColor, function(v){ ScreensaverConfig.minimalTimeColor = v }, 1)
        Rectangle {
            Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
            border { color: ScreensaverConfig.minimalTimeColor === "rainbow" ? colors.offwhite : colors.medium; width: ScreensaverConfig.minimalTimeColor === "rainbow" ? 3 : 1 }
            Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 5; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#ff0000" } GradientStop { position: 0.25; color: "#ffbf00" } GradientStop { position: 0.5; color: "#00ff41" } GradientStop { position: 0.75; color: "#0000ff" } GradientStop { position: 1.0; color: "#ff0000" } } }
            Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.minimalTimeColor = "rainbow" }
        }
        Rectangle {
            Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
            border { color: ScreensaverConfig.minimalTimeColor === "rainbow_gradient" ? colors.offwhite : colors.medium; width: ScreensaverConfig.minimalTimeColor === "rainbow_gradient" ? 3 : 1 }
            Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 5; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#ff0000" } GradientStop { position: 0.2; color: "#ffff00" } GradientStop { position: 0.4; color: "#00ff80" } GradientStop { position: 0.6; color: "#0080ff" } GradientStop { position: 0.8; color: "#8000ff" } GradientStop { position: 1.0; color: "#ff0000" } } }
            Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.minimalTimeColor = "rainbow_gradient" }
        }
        Rectangle {
            Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
            border { color: ScreensaverConfig.minimalTimeColor === "neon" ? colors.offwhite : colors.medium; width: ScreensaverConfig.minimalTimeColor === "neon" ? 3 : 1 }
            Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 5; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#ff8080" } GradientStop { position: 0.2; color: "#ffff80" } GradientStop { position: 0.4; color: "#80ffd0" } GradientStop { position: 0.6; color: "#80d0ff" } GradientStop { position: 0.8; color: "#d080ff" } GradientStop { position: 1.0; color: "#ff8080" } } }
            Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.minimalTimeColor = "neon" }
        }
    }

    // Date color
    Text { Layout.fillWidth: true; color: colors.offwhite; text: qsTr("Date color"); font: fonts.primaryFont(30) }
    RowLayout {
        id: minimalDateColorSolidRow
        spacing: 6; focus: true
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: minimalTimeGradientRow; KeyNavigation.down: minimalDateGradientRow
        Keys.onLeftPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#666666"]; root.settingsPage.cycleOption(c, ScreensaverConfig.minimalDateColor, function(v){ ScreensaverConfig.minimalDateColor = v }, -1) }
        Keys.onRightPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#666666"]; root.settingsPage.cycleOption(c, ScreensaverConfig.minimalDateColor, function(v){ ScreensaverConfig.minimalDateColor = v }, 1) }
        Repeater {
            model: [{ color: "#ffffff" },{ color: "#00ff41" },{ color: "#00b4d8" },{ color: "#ff0040" },{ color: "#ffbf00" },{ color: "#bf00ff" },{ color: "#666666" }]
            Rectangle {
                Layout.fillWidth: true; height: 36; radius: 6; color: modelData.color
                border { color: ScreensaverConfig.minimalDateColor === modelData.color ? colors.offwhite : colors.medium; width: ScreensaverConfig.minimalDateColor === modelData.color ? 3 : 1 }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.minimalDateColor = modelData.color }
            }
        }
    }
    RowLayout {
        id: minimalDateGradientRow
        spacing: 6; focus: true
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: minimalDateColorSolidRow; KeyNavigation.down: minimalClockSlider
        Keys.onLeftPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.minimalDateColor, function(v){ ScreensaverConfig.minimalDateColor = v }, -1)
        Keys.onRightPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.minimalDateColor, function(v){ ScreensaverConfig.minimalDateColor = v }, 1)
        Rectangle {
            Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
            border { color: ScreensaverConfig.minimalDateColor === "rainbow" ? colors.offwhite : colors.medium; width: ScreensaverConfig.minimalDateColor === "rainbow" ? 3 : 1 }
            Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 5; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#ff0000" } GradientStop { position: 0.25; color: "#ffbf00" } GradientStop { position: 0.5; color: "#00ff41" } GradientStop { position: 0.75; color: "#0000ff" } GradientStop { position: 1.0; color: "#ff0000" } } }
            Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.minimalDateColor = "rainbow" }
        }
        Rectangle {
            Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
            border { color: ScreensaverConfig.minimalDateColor === "rainbow_gradient" ? colors.offwhite : colors.medium; width: ScreensaverConfig.minimalDateColor === "rainbow_gradient" ? 3 : 1 }
            Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 5; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#ff0000" } GradientStop { position: 0.2; color: "#ffff00" } GradientStop { position: 0.4; color: "#00ff80" } GradientStop { position: 0.6; color: "#0080ff" } GradientStop { position: 0.8; color: "#8000ff" } GradientStop { position: 1.0; color: "#ff0000" } } }
            Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.minimalDateColor = "rainbow_gradient" }
        }
        Rectangle {
            Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
            border { color: ScreensaverConfig.minimalDateColor === "neon" ? colors.offwhite : colors.medium; width: ScreensaverConfig.minimalDateColor === "neon" ? 3 : 1 }
            Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 5; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#ff8080" } GradientStop { position: 0.2; color: "#ffff80" } GradientStop { position: 0.4; color: "#80ffd0" } GradientStop { position: 0.6; color: "#80d0ff" } GradientStop { position: 0.8; color: "#d080ff" } GradientStop { position: 1.0; color: "#ff8080" } } }
            Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.minimalDateColor = "neon" }
        }
    }

    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Clock size"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: minimalClockSlider
        height: 60; Layout.fillWidth: true
        from: 48; to: 144; stepSize: 4
        value: ScreensaverConfig.minimalClockSize; live: true
        onMoved: ScreensaverConfig.minimalClockSize = value
        onUserInteractionEnded: ScreensaverConfig.minimalClockSize = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: minimalDateGradientRow
        KeyNavigation.down: minimalDateSlider
    }

    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Date size"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: minimalDateSlider
        height: 60; Layout.fillWidth: true
        from: 16; to: 48; stepSize: 2
        value: ScreensaverConfig.minimalDateSize; live: true
        onMoved: ScreensaverConfig.minimalDateSize = value
        onUserInteractionEnded: ScreensaverConfig.minimalDateSize = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: minimalClockSlider
        KeyNavigation.down: root.navDownTarget
    }
}
