// Copyright (c) 2024 madalone. Matrix appearance settings component.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: solidColorRow
    property alias lastFocusItem: dirDiagRow
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // Leading separator
    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 4. COLOR
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Color"); font: fonts.primaryFont(30)
        }

        // Solid colors
        RowLayout {
            id: solidColorRow
            objectName: "solidColorRow"
            spacing: 6; focus: true
            Accessible.name: "Solid color selector"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: gradientColorRow
            Keys.onLeftPressed: root.settingsPage.cycleOption(["green","blue","red","amber","white","purple"], Config.chargingMatrixColorMode, function(v){ Config.chargingMatrixColorMode = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["green","blue","red","amber","white","purple"], Config.chargingMatrixColorMode, function(v){ Config.chargingMatrixColorMode = v }, 1)
            Repeater {
                model: [
                    { name: "green",  color: "#00ff41" },
                    { name: "blue",   color: "#00b4d8" },
                    { name: "red",    color: "#ff0040" },
                    { name: "amber",  color: "#ffbf00" },
                    { name: "white",  color: "#ffffff" },
                    { name: "purple", color: "#bf00ff" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    color: modelData.color
                    border {
                        color: Config.chargingMatrixColorMode === modelData.name ? colors.offwhite : colors.medium
                        width: Config.chargingMatrixColorMode === modelData.name ? 3 : 1
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: Config.chargingMatrixColorMode = modelData.name
                    }
                }
            }
        }

        // Gradient presets
        RowLayout {
            id: gradientColorRow
            objectName: "gradientColorRow"
            spacing: 6; focus: true
            Accessible.name: "Gradient color selector"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: solidColorRow; KeyNavigation.down: charsetRow
            Keys.onLeftPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], Config.chargingMatrixColorMode, function(v){ Config.chargingMatrixColorMode = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], Config.chargingMatrixColorMode, function(v){ Config.chargingMatrixColorMode = v }, 1)
            // Rainbow (12 hues)
            Rectangle {
                Layout.fillWidth: true; height: 46; radius: 8
                color: "transparent"
                border { color: Config.chargingMatrixColorMode === "rainbow" ? colors.offwhite : colors.medium; width: Config.chargingMatrixColorMode === "rainbow" ? 3 : 1 }
                Rectangle {
                    anchors.fill: parent; anchors.margins: 1; radius: 7
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#ff0000" }
                        GradientStop { position: 0.25; color: "#ffbf00" }
                        GradientStop { position: 0.5; color: "#00ff41" }
                        GradientStop { position: 0.75; color: "#0000ff" }
                        GradientStop { position: 1.0; color: "#ff0000" }
                    }
                }
                Text { anchors.centerIn: parent; text: "Rainbow"; color: colors.offwhite; font: fonts.secondaryFont(18) }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: Config.chargingMatrixColorMode = "rainbow" }
            }
            // Rainbow+ (24 hues, smoother)
            Rectangle {
                Layout.fillWidth: true; height: 46; radius: 8
                color: "transparent"
                border { color: Config.chargingMatrixColorMode === "rainbow_gradient" ? colors.offwhite : colors.medium; width: Config.chargingMatrixColorMode === "rainbow_gradient" ? 3 : 1 }
                Rectangle {
                    anchors.fill: parent; anchors.margins: 1; radius: 7
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#ff0000" }
                        GradientStop { position: 0.1; color: "#ff8000" }
                        GradientStop { position: 0.2; color: "#ffff00" }
                        GradientStop { position: 0.3; color: "#80ff00" }
                        GradientStop { position: 0.4; color: "#00ff80" }
                        GradientStop { position: 0.5; color: "#00ffff" }
                        GradientStop { position: 0.6; color: "#0080ff" }
                        GradientStop { position: 0.7; color: "#0000ff" }
                        GradientStop { position: 0.8; color: "#8000ff" }
                        GradientStop { position: 0.9; color: "#ff00ff" }
                        GradientStop { position: 1.0; color: "#ff0000" }
                    }
                }
                Text { anchors.centerIn: parent; text: "Rainbow+"; color: colors.offwhite; font: fonts.secondaryFont(18) }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: Config.chargingMatrixColorMode = "rainbow_gradient" }
            }
            // Neon (24 hues, high lightness)
            Rectangle {
                Layout.fillWidth: true; height: 46; radius: 8
                color: "transparent"
                border { color: Config.chargingMatrixColorMode === "neon" ? colors.offwhite : colors.medium; width: Config.chargingMatrixColorMode === "neon" ? 3 : 1 }
                Rectangle {
                    anchors.fill: parent; anchors.margins: 1; radius: 7
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#ff8080" }
                        GradientStop { position: 0.1; color: "#ffd080" }
                        GradientStop { position: 0.2; color: "#ffff80" }
                        GradientStop { position: 0.3; color: "#d0ff80" }
                        GradientStop { position: 0.4; color: "#80ffd0" }
                        GradientStop { position: 0.5; color: "#80ffff" }
                        GradientStop { position: 0.6; color: "#80d0ff" }
                        GradientStop { position: 0.7; color: "#8080ff" }
                        GradientStop { position: 0.8; color: "#d080ff" }
                        GradientStop { position: 0.9; color: "#ff80ff" }
                        GradientStop { position: 1.0; color: "#ff8080" }
                    }
                }
                Text { anchors.centerIn: parent; text: "Neon"; color: colors.offwhite; font: fonts.secondaryFont(18) }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: Config.chargingMatrixColorMode = "neon" }
            }
        }

    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 5. CHARACTERS
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Characters"); font: fonts.primaryFont(30)
        }

        RowLayout {
            id: charsetRow
            objectName: "charsetRow"
            spacing: 8; focus: true
            Accessible.name: "Character set selector"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: gradientColorRow; KeyNavigation.down: fontSizeSlider
            Keys.onLeftPressed: root.settingsPage.cycleOption(["katakana","ascii","binary","digits"], Config.chargingMatrixCharset, function(v){ Config.chargingMatrixCharset = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["katakana","ascii","binary","digits"], Config.chargingMatrixCharset, function(v){ Config.chargingMatrixCharset = v }, 1)
            Repeater {
                model: [
                    { name: "katakana", label: "Kana" },
                    { name: "ascii", label: "ABC" },
                    { name: "binary", label: "01" },
                    { name: "digits", label: "123" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    color: Config.chargingMatrixCharset === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: Config.chargingMatrixCharset === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: Config.chargingMatrixCharset = modelData.name
                    }
                }
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 6. FONT SIZE
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Font size"); font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: fontSizeSlider
            objectName: "fontSizeSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 60; stepSize: 1
            value: Config.chargingMatrixFontSize; live: true; showLiveValue: true
            onMoved: Config.chargingMatrixFontSize = value
            onUserInteractionEnded: Config.chargingMatrixFontSize = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Font size " + value
            KeyNavigation.up: charsetRow; KeyNavigation.down: speedSlider
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 7. ANIMATION SPEED
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Animation speed"); font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: speedSlider
            objectName: "speedSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 100; stepSize: 5
            value: Config.chargingMatrixSpeed; live: true
            onMoved: Config.chargingMatrixSpeed = value
            onUserInteractionEnded: Config.chargingMatrixSpeed = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Animation speed " + value
            KeyNavigation.up: fontSizeSlider; KeyNavigation.down: densitySlider
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 8. COLUMN DENSITY
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Column density"); font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: densitySlider
            objectName: "densitySlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 20; to: 500; stepSize: 5
            value: Config.chargingMatrixDensity; live: true
            onMoved: Config.chargingMatrixDensity = value
            onUserInteractionEnded: Config.chargingMatrixDensity = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Column density " + value
            KeyNavigation.up: speedSlider; KeyNavigation.down: trailSlider
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 9. TRAIL LENGTH
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Trail length"); font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: trailSlider
            objectName: "trailSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 100; stepSize: 5
            value: Config.chargingMatrixTrail; live: true
            onMoved: Config.chargingMatrixTrail = value
            onUserInteractionEnded: Config.chargingMatrixTrail = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Trail length " + value
            KeyNavigation.up: densitySlider; KeyNavigation.down: fadeSlider
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 10. TRAIL FADE
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Trail fade"); font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: fadeSlider
            objectName: "fadeSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 20; to: 100; stepSize: 5
            value: Config.chargingMatrixFade; live: true
            onMoved: Config.chargingMatrixFade = value
            onUserInteractionEnded: Config.chargingMatrixFade = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Trail fade " + value
            KeyNavigation.up: trailSlider; KeyNavigation.down: dirCardinalRow
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 10a. AUTO-ROTATE TOGGLE
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Auto-rotate"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: gravitySwitch
                objectName: "gravitySwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGravity
                trigger: function() { Config.chargingMatrixGravity = !Config.chargingMatrixGravity; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Auto-rotate"
                KeyNavigation.up: fadeSlider
                KeyNavigation.down: autoRotateSpeedSlider
            }
        }
    }

    // 10a2. AUTO-ROTATE SPEED (visible when auto-rotate is on)
    ColumnLayout {
        visible: Config.chargingMatrixGravity
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Rotation speed") + " (" + Config.chargingMatrixAutoRotateSpeed + "%)"
            font: fonts.primaryFont(26)
        }
        Components.Slider {
            id: autoRotateSpeedSlider
            objectName: "autoRotateSpeedSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 100; stepSize: 5
            value: Config.chargingMatrixAutoRotateSpeed; live: true
            onMoved: Config.chargingMatrixAutoRotateSpeed = value
            onUserInteractionEnded: Config.chargingMatrixAutoRotateSpeed = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Rotation speed " + value
            KeyNavigation.up: gravitySwitch; KeyNavigation.down: autoRotateSmoothnessSlider
        }
    }

    // 10a3. AUTO-ROTATE SMOOTHNESS (visible when auto-rotate is on)
    ColumnLayout {
        visible: Config.chargingMatrixGravity
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Trail bend") + " (" + Config.chargingMatrixAutoRotateBend + "%)"
            font: fonts.primaryFont(26)
        }
        Components.Slider {
            id: autoRotateSmoothnessSlider
            objectName: "autoRotateSmoothnessSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 100; stepSize: 5
            value: Config.chargingMatrixAutoRotateBend; live: true
            onMoved: Config.chargingMatrixAutoRotateBend = value
            onUserInteractionEnded: Config.chargingMatrixAutoRotateBend = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Trail bend " + value
            KeyNavigation.up: autoRotateSpeedSlider; KeyNavigation.down: dirCardinalRow
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 10b. DIRECTION
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        opacity: Config.chargingMatrixGravity ? 0.4 : 1.0

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Direction"); font: fonts.primaryFont(30)
        }

        // Cardinal directions
        RowLayout {
            id: dirCardinalRow
            objectName: "dirCardinalRow"
            spacing: 8; focus: true
            enabled: !Config.chargingMatrixGravity
            Accessible.name: "Cardinal direction selector"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: autoRotateSmoothnessSlider
            KeyNavigation.down: dirDiagRow
            Keys.onLeftPressed: root.settingsPage.cycleOption(["down","up","left","right"], Config.chargingMatrixDirection, function(v){ Config.chargingMatrixDirection = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["down","up","left","right"], Config.chargingMatrixDirection, function(v){ Config.chargingMatrixDirection = v }, 1)
            Repeater {
                model: [
                    { name: "down",  label: "\u2193 Down" },
                    { name: "up",    label: "\u2191 Up" },
                    { name: "left",  label: "\u2190 Left" },
                    { name: "right", label: "\u2192 Right" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    color: Config.chargingMatrixDirection === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: Config.chargingMatrixDirection === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: Config.chargingMatrixDirection = modelData.name
                    }
                }
            }
        }

        // Diagonal directions
        RowLayout {
            id: dirDiagRow
            objectName: "dirDiagRow"
            spacing: 8; focus: true
            enabled: !Config.chargingMatrixGravity
            Accessible.name: "Diagonal direction selector"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: dirCardinalRow
            KeyNavigation.down: root.navDownTarget
            Keys.onLeftPressed: root.settingsPage.cycleOption(["down-right","down-left","up-right","up-left"], Config.chargingMatrixDirection, function(v){ Config.chargingMatrixDirection = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["down-right","down-left","up-right","up-left"], Config.chargingMatrixDirection, function(v){ Config.chargingMatrixDirection = v }, 1)
            Repeater {
                model: [
                    { name: "down-right", label: "\u2198 D-R" },
                    { name: "down-left",  label: "\u2199 D-L" },
                    { name: "up-right",   label: "\u2197 U-R" },
                    { name: "up-left",    label: "\u2196 U-L" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    color: Config.chargingMatrixDirection === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: Config.chargingMatrixDirection === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: Config.chargingMatrixDirection = modelData.name
                    }
                }
            }
        }
    }
}
