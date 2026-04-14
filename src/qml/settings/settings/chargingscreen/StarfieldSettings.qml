// Copyright (c) 2026 madalone. Starfield theme settings (extracted from ChargingScreen.qml).
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

    property alias firstFocusItem: starfieldSpeedSlider
    property alias lastFocusItem: starfieldColorGradientRow

    Layout.fillWidth: true
    Layout.leftMargin: 10; Layout.rightMargin: 10
    spacing: 20

    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Animation speed"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: starfieldSpeedSlider
        height: 60; Layout.fillWidth: true
        from: 10; to: 100; stepSize: 5
        value: ScreensaverConfig.starfieldSpeed; live: true
        onMoved: ScreensaverConfig.starfieldSpeed = value
        onUserInteractionEnded: ScreensaverConfig.starfieldSpeed = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: root.navUpTarget
        KeyNavigation.down: starfieldDensitySlider
    }

    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Star density"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: starfieldDensitySlider
        height: 60; Layout.fillWidth: true
        from: 10; to: 100; stepSize: 5
        value: ScreensaverConfig.starfieldDensity; live: true
        onMoved: ScreensaverConfig.starfieldDensity = value
        onUserInteractionEnded: ScreensaverConfig.starfieldDensity = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: starfieldSpeedSlider
        KeyNavigation.down: starfieldStarSizeSlider
    }

    Text { Layout.fillWidth: true; color: colors.offwhite; text: qsTr("Star size"); font: fonts.primaryFont(30) }
    Components.Slider {
        id: starfieldStarSizeSlider
        height: 60; Layout.fillWidth: true
        from: 10; to: 100; stepSize: 5
        value: ScreensaverConfig.starfieldStarSize; live: true
        onMoved: ScreensaverConfig.starfieldStarSize = value
        onUserInteractionEnded: ScreensaverConfig.starfieldStarSize = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: starfieldDensitySlider; KeyNavigation.down: starfieldTrailSlider
    }

    Text { Layout.fillWidth: true; color: colors.offwhite; text: qsTr("Trail length"); font: fonts.primaryFont(30) }
    Components.Slider {
        id: starfieldTrailSlider
        height: 60; Layout.fillWidth: true
        from: 10; to: 100; stepSize: 5
        value: ScreensaverConfig.starfieldTrailLength; live: true
        onMoved: ScreensaverConfig.starfieldTrailLength = value
        onUserInteractionEnded: ScreensaverConfig.starfieldTrailLength = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: starfieldStarSizeSlider; KeyNavigation.down: starfieldColorSolidRow
    }

    Text { Layout.fillWidth: true; color: colors.offwhite; text: qsTr("Star color"); font: fonts.primaryFont(30) }
    RowLayout {
        id: starfieldColorSolidRow
        spacing: 6; focus: true
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: starfieldTrailSlider; KeyNavigation.down: starfieldColorGradientRow
        Keys.onLeftPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; root.settingsPage.cycleOption(c, ScreensaverConfig.starfieldColor, function(v){ ScreensaverConfig.starfieldColor = v }, -1) }
        Keys.onRightPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; root.settingsPage.cycleOption(c, ScreensaverConfig.starfieldColor, function(v){ ScreensaverConfig.starfieldColor = v }, 1) }
        Repeater {
            model: [{ color: "#ffffff" },{ color: "#00ff41" },{ color: "#00b4d8" },{ color: "#ff0040" },{ color: "#ffbf00" },{ color: "#bf00ff" },{ color: "#d0d0d0" }]
            Rectangle {
                Layout.fillWidth: true; height: 36; radius: 6; color: modelData.color
                border { color: ScreensaverConfig.starfieldColor === modelData.color ? colors.offwhite : colors.medium; width: ScreensaverConfig.starfieldColor === modelData.color ? 3 : 1 }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.starfieldColor = modelData.color }
            }
        }
    }
    RowLayout {
        id: starfieldColorGradientRow
        spacing: 6; focus: true
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: starfieldColorSolidRow; KeyNavigation.down: root.navDownTarget
        Keys.onLeftPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.starfieldColor, function(v){ ScreensaverConfig.starfieldColor = v }, -1)
        Keys.onRightPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.starfieldColor, function(v){ ScreensaverConfig.starfieldColor = v }, 1)
        Rectangle {
            Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
            border { color: ScreensaverConfig.starfieldColor === "rainbow" ? colors.offwhite : colors.medium; width: ScreensaverConfig.starfieldColor === "rainbow" ? 3 : 1 }
            Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 5; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#ff0000" } GradientStop { position: 0.25; color: "#ffbf00" } GradientStop { position: 0.5; color: "#00ff41" } GradientStop { position: 0.75; color: "#0000ff" } GradientStop { position: 1.0; color: "#ff0000" } } }
            Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.starfieldColor = "rainbow" }
        }
        Rectangle {
            Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
            border { color: ScreensaverConfig.starfieldColor === "rainbow_gradient" ? colors.offwhite : colors.medium; width: ScreensaverConfig.starfieldColor === "rainbow_gradient" ? 3 : 1 }
            Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 5; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#ff0000" } GradientStop { position: 0.2; color: "#ffff00" } GradientStop { position: 0.4; color: "#00ff80" } GradientStop { position: 0.6; color: "#0080ff" } GradientStop { position: 0.8; color: "#8000ff" } GradientStop { position: 1.0; color: "#ff0000" } } }
            Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.starfieldColor = "rainbow_gradient" }
        }
        Rectangle {
            Layout.fillWidth: true; height: 36; radius: 6; color: "transparent"
            border { color: ScreensaverConfig.starfieldColor === "neon" ? colors.offwhite : colors.medium; width: ScreensaverConfig.starfieldColor === "neon" ? 3 : 1 }
            Rectangle { anchors.fill: parent; anchors.margins: 1; radius: 5; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: "#ff8080" } GradientStop { position: 0.2; color: "#ffff80" } GradientStop { position: 0.4; color: "#80ffd0" } GradientStop { position: 0.6; color: "#80d0ff" } GradientStop { position: 0.8; color: "#d080ff" } GradientStop { position: 1.0; color: "#ff8080" } } }
            Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.starfieldColor = "neon" }
        }
    }
}
