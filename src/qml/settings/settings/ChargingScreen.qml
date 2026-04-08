// Copyright (c) 2024 madalone. Charging screen settings page.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components
import "qrc:/settings/settings/chargingscreen" as ChargingScreenComponents
import ScreensaverConfig 1.0

Settings.Page {
    id: chargingScreenPage

    // Cycle a selector's value left/right via DPAD
    function cycleOption(options, current, setter, delta) {
        for (var i = 0; i < options.length; i++) {
            if (current === options[i]) {
                var next = i + delta;
                if (next >= 0 && next < options.length) {
                    setter(options[next]);
                    Haptic.play(Haptic.Click);
                }
                return;
            }
        }
    }

    // Scroll to focused item when navigating with DPAD
    function ensureVisible(item) {
        if (!item) return;
        var yy = item.mapToItem(content, 0, 0).y;
        if (yy < flickable.contentY) {
            flickable.contentY = yy - 20;
        } else if (yy + item.height > flickable.contentY + flickable.height) {
            flickable.contentY = yy + item.height - flickable.height + 20;
        }
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        ColumnLayout {
            id: content
            spacing: 20
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            ChargingScreenComponents.ThemeSelector {
                id: themeSelector
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navDownTarget: commonToggles.firstFocusItem
            }

            ChargingScreenComponents.CommonToggles {
                id: commonToggles
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navUpTarget: themeSelector.lastFocusItem
                navDownTarget: matrixAppearance.visible ? matrixAppearance.firstFocusItem
                             : starfieldSettings.visible ? starfieldSpeedSlider
                             : minimalSettings.visible ? minimalClock24hSwitch
                             : generalBehavior.firstFocusItem
            }

            ChargingScreenComponents.MatrixAppearance {
                id: matrixAppearance
                settingsPage: chargingScreenPage
                visible: ScreensaverConfig.theme === "matrix"
                Layout.fillWidth: true
                navUpTarget: commonToggles.lastFocusItem
                navDownTarget: matrixEffects.firstFocusItem
            }

            ChargingScreenComponents.MatrixEffects {
                id: matrixEffects
                settingsPage: chargingScreenPage
                visible: ScreensaverConfig.theme === "matrix"
                Layout.fillWidth: true
                navUpTarget: matrixAppearance.lastFocusItem
                navDownTarget: generalBehavior.firstFocusItem
            }

            // --- Starfield settings (speed + density) ---
            ColumnLayout {
                id: starfieldSettings
                visible: ScreensaverConfig.theme === "starfield"
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: commonToggles.lastFocusItem
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: starfieldStarSizeSlider; KeyNavigation.down: starfieldColorSolidRow
                }

                Text { Layout.fillWidth: true; color: colors.offwhite; text: qsTr("Star color"); font: fonts.primaryFont(30) }
                RowLayout {
                    id: starfieldColorSolidRow
                    spacing: 6; focus: true
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: starfieldTrailSlider; KeyNavigation.down: starfieldColorGradientRow
                    Keys.onLeftPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; chargingScreenPage.cycleOption(c, ScreensaverConfig.starfieldColor, function(v){ ScreensaverConfig.starfieldColor = v }, -1) }
                    Keys.onRightPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; chargingScreenPage.cycleOption(c, ScreensaverConfig.starfieldColor, function(v){ ScreensaverConfig.starfieldColor = v }, 1) }
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: starfieldColorSolidRow; KeyNavigation.down: generalBehavior.firstFocusItem
                    Keys.onLeftPressed: chargingScreenPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.starfieldColor, function(v){ ScreensaverConfig.starfieldColor = v }, -1)
                    Keys.onRightPressed: chargingScreenPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.starfieldColor, function(v){ ScreensaverConfig.starfieldColor = v }, 1)
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

            // --- Minimal settings (clock + date size) ---
            ColumnLayout {
                id: minimalSettings
                visible: ScreensaverConfig.theme === "minimal"
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
                        onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                        checked: ScreensaverConfig.minimalClock24h
                        trigger: function() { ScreensaverConfig.minimalClock24h = !ScreensaverConfig.minimalClock24h; }
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.up: commonToggles.lastFocusItem
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: minimalClock24hSwitch
                    KeyNavigation.down: minimalTimeColorSolidRow
                    Keys.onLeftPressed: chargingScreenPage.cycleOption(["primary","secondary"], ScreensaverConfig.minimalFont, function(v){ ScreensaverConfig.minimalFont = v }, -1)
                    Keys.onRightPressed: chargingScreenPage.cycleOption(["primary","secondary"], ScreensaverConfig.minimalFont, function(v){ ScreensaverConfig.minimalFont = v }, 1)
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: minimalFontRow; KeyNavigation.down: minimalTimeGradientRow
                    Keys.onLeftPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; chargingScreenPage.cycleOption(c, ScreensaverConfig.minimalTimeColor, function(v){ ScreensaverConfig.minimalTimeColor = v }, -1) }
                    Keys.onRightPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; chargingScreenPage.cycleOption(c, ScreensaverConfig.minimalTimeColor, function(v){ ScreensaverConfig.minimalTimeColor = v }, 1) }
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: minimalTimeColorSolidRow; KeyNavigation.down: minimalDateColorSolidRow
                    Keys.onLeftPressed: chargingScreenPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.minimalTimeColor, function(v){ ScreensaverConfig.minimalTimeColor = v }, -1)
                    Keys.onRightPressed: chargingScreenPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.minimalTimeColor, function(v){ ScreensaverConfig.minimalTimeColor = v }, 1)
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: minimalTimeGradientRow; KeyNavigation.down: minimalDateGradientRow
                    Keys.onLeftPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#666666"]; chargingScreenPage.cycleOption(c, ScreensaverConfig.minimalDateColor, function(v){ ScreensaverConfig.minimalDateColor = v }, -1) }
                    Keys.onRightPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#666666"]; chargingScreenPage.cycleOption(c, ScreensaverConfig.minimalDateColor, function(v){ ScreensaverConfig.minimalDateColor = v }, 1) }
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: minimalDateColorSolidRow; KeyNavigation.down: minimalClockSlider
                    Keys.onLeftPressed: chargingScreenPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.minimalDateColor, function(v){ ScreensaverConfig.minimalDateColor = v }, -1)
                    Keys.onRightPressed: chargingScreenPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.minimalDateColor, function(v){ ScreensaverConfig.minimalDateColor = v }, 1)
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
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
                    onActiveFocusChanged: if (activeFocus) chargingScreenPage.ensureVisible(this)
                    KeyNavigation.up: minimalClockSlider
                    KeyNavigation.down: generalBehavior.firstFocusItem
                }
            }

            ChargingScreenComponents.GeneralBehavior {
                id: generalBehavior
                settingsPage: chargingScreenPage
                Layout.fillWidth: true
                navUpTarget: matrixEffects.visible ? matrixEffects.lastFocusItem
                           : starfieldSettings.visible ? starfieldColorGradientRow
                           : minimalSettings.visible ? minimalDateSlider
                           : commonToggles.lastFocusItem
            }

            Item { Layout.preferredHeight: 40 }
        }
    }
}
