// Copyright (c) 2026 madalone. Matrix appearance settings component.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import "qrc:/components" as Components
import ScreensaverConfig 1.0
import Palettes 1.0

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
            Keys.onLeftPressed: root.settingsPage.cycleOption(["green","blue","red","amber","white","purple"], ScreensaverConfig.colorMode, function(v){ ScreensaverConfig.colorMode = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["green","blue","red","amber","white","purple"], ScreensaverConfig.colorMode, function(v){ ScreensaverConfig.colorMode = v }, 1)
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
                        color: ScreensaverConfig.colorMode === modelData.name ? colors.offwhite : colors.medium
                        width: ScreensaverConfig.colorMode === modelData.name ? 3 : 1
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: ScreensaverConfig.colorMode = modelData.name
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
            Keys.onLeftPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.colorMode, function(v){ ScreensaverConfig.colorMode = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["rainbow","rainbow_gradient","neon"], ScreensaverConfig.colorMode, function(v){ ScreensaverConfig.colorMode = v }, 1)
            // Rainbow (12 hues)
            Rectangle {
                Layout.fillWidth: true; height: 46; radius: 8
                color: "transparent"
                border { color: ScreensaverConfig.colorMode === "rainbow" ? colors.offwhite : colors.medium; width: ScreensaverConfig.colorMode === "rainbow" ? 3 : 1 }
                Rectangle {
                    anchors.fill: parent; anchors.margins: 1; radius: 7
                    gradient: Palettes.rainbow
                }
                Text { anchors.centerIn: parent; text: "Rainbow"; color: colors.offwhite; font: fonts.secondaryFont(18) }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.colorMode = "rainbow" }
            }
            // Rainbow+ (24 hues, smoother)
            Rectangle {
                Layout.fillWidth: true; height: 46; radius: 8
                color: "transparent"
                border { color: ScreensaverConfig.colorMode === "rainbow_gradient" ? colors.offwhite : colors.medium; width: ScreensaverConfig.colorMode === "rainbow_gradient" ? 3 : 1 }
                Rectangle {
                    anchors.fill: parent; anchors.margins: 1; radius: 7
                    gradient: Palettes.rainbowPlus
                }
                Text { anchors.centerIn: parent; text: "Rainbow+"; color: colors.offwhite; font: fonts.secondaryFont(18) }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.colorMode = "rainbow_gradient" }
            }
            // Neon (24 hues, high lightness)
            Rectangle {
                Layout.fillWidth: true; height: 46; radius: 8
                color: "transparent"
                border { color: ScreensaverConfig.colorMode === "neon" ? colors.offwhite : colors.medium; width: ScreensaverConfig.colorMode === "neon" ? 3 : 1 }
                Rectangle {
                    anchors.fill: parent; anchors.margins: 1; radius: 7
                    gradient: Palettes.neon
                }
                Text { anchors.centerIn: parent; text: "Neon"; color: colors.offwhite; font: fonts.secondaryFont(18) }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.colorMode = "neon" }
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
            Keys.onLeftPressed: root.settingsPage.cycleOption(["katakana","ascii","binary","digits"], ScreensaverConfig.charset, function(v){ ScreensaverConfig.charset = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["katakana","ascii","binary","digits"], ScreensaverConfig.charset, function(v){ ScreensaverConfig.charset = v }, 1)
            Repeater {
                model: [
                    { name: "katakana", label: "Kana" },
                    { name: "ascii", label: "ABC" },
                    { name: "binary", label: "01" },
                    { name: "digits", label: "123" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    color: ScreensaverConfig.charset === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: ScreensaverConfig.charset === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: ScreensaverConfig.charset = modelData.name
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
            value: ScreensaverConfig.fontSize; live: true; showLiveValue: true
            onMoved: ScreensaverConfig.fontSize = value
            onUserInteractionEnded: ScreensaverConfig.fontSize = value
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
            value: ScreensaverConfig.matrixSpeed; live: true
            onMoved: ScreensaverConfig.matrixSpeed = value
            onUserInteractionEnded: ScreensaverConfig.matrixSpeed = value
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
            value: ScreensaverConfig.matrixDensity; live: true
            onMoved: ScreensaverConfig.matrixDensity = value
            onUserInteractionEnded: ScreensaverConfig.matrixDensity = value
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
            value: ScreensaverConfig.matrixTrail; live: true
            onMoved: ScreensaverConfig.matrixTrail = value
            onUserInteractionEnded: ScreensaverConfig.matrixTrail = value
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
            value: ScreensaverConfig.matrixFade; live: true
            onMoved: ScreensaverConfig.matrixFade = value
            onUserInteractionEnded: ScreensaverConfig.matrixFade = value
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
                checked: ScreensaverConfig.gravityMode
                trigger: function() { ScreensaverConfig.gravityMode = !ScreensaverConfig.gravityMode; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Auto-rotate"
                KeyNavigation.up: fadeSlider
                KeyNavigation.down: ScreensaverConfig.gravityMode ? autoRotateSpeedSlider : dirCardinalRow
            }
        }
    }

    // 10a2. AUTO-ROTATE SPEED (visible when auto-rotate is on)
    ColumnLayout {
        visible: ScreensaverConfig.gravityMode
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Rotation speed") + " (" + ScreensaverConfig.autoRotateSpeed + "%)"
            font: fonts.primaryFont(26)
        }
        Components.Slider {
            id: autoRotateSpeedSlider
            objectName: "autoRotateSpeedSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.autoRotateSpeed; live: true
            onMoved: ScreensaverConfig.autoRotateSpeed = value
            onUserInteractionEnded: ScreensaverConfig.autoRotateSpeed = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Rotation speed " + value
            KeyNavigation.up: gravitySwitch; KeyNavigation.down: autoRotateSmoothnessSlider
        }
    }

    // 10a3. AUTO-ROTATE SMOOTHNESS (visible when auto-rotate is on)
    ColumnLayout {
        visible: ScreensaverConfig.gravityMode
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Trail bend") + " (" + ScreensaverConfig.autoRotateBend + "%)"
            font: fonts.primaryFont(26)
        }
        Components.Slider {
            id: autoRotateSmoothnessSlider
            objectName: "autoRotateSmoothnessSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 100; stepSize: 5
            value: ScreensaverConfig.autoRotateBend; live: true
            onMoved: ScreensaverConfig.autoRotateBend = value
            onUserInteractionEnded: ScreensaverConfig.autoRotateBend = value
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
        opacity: ScreensaverConfig.gravityMode ? 0.4 : 1.0

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Direction"); font: fonts.primaryFont(30)
        }

        // Cardinal directions
        RowLayout {
            id: dirCardinalRow
            objectName: "dirCardinalRow"
            spacing: 8; focus: true
            enabled: !ScreensaverConfig.gravityMode
            Accessible.name: "Cardinal direction selector"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: ScreensaverConfig.gravityMode ? autoRotateSmoothnessSlider : gravitySwitch
            KeyNavigation.down: dirDiagRow
            Keys.onLeftPressed: root.settingsPage.cycleOption(["down","up","left","right"], ScreensaverConfig.direction, function(v){ ScreensaverConfig.direction = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["down","up","left","right"], ScreensaverConfig.direction, function(v){ ScreensaverConfig.direction = v }, 1)
            Repeater {
                model: [
                    { name: "down",  label: "\u2193 Down" },
                    { name: "up",    label: "\u2191 Up" },
                    { name: "left",  label: "\u2190 Left" },
                    { name: "right", label: "\u2192 Right" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    color: ScreensaverConfig.direction === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: ScreensaverConfig.direction === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: ScreensaverConfig.direction = modelData.name
                    }
                }
            }
        }

        // Diagonal directions
        RowLayout {
            id: dirDiagRow
            objectName: "dirDiagRow"
            spacing: 8; focus: true
            enabled: !ScreensaverConfig.gravityMode
            Accessible.name: "Diagonal direction selector"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: dirCardinalRow
            KeyNavigation.down: root.navDownTarget
            Keys.onLeftPressed: root.settingsPage.cycleOption(["down-right","down-left","up-right","up-left"], ScreensaverConfig.direction, function(v){ ScreensaverConfig.direction = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["down-right","down-left","up-right","up-left"], ScreensaverConfig.direction, function(v){ ScreensaverConfig.direction = v }, 1)
            Repeater {
                model: [
                    { name: "down-right", label: "\u2198 D-R" },
                    { name: "down-left",  label: "\u2199 D-L" },
                    { name: "up-right",   label: "\u2197 U-R" },
                    { name: "up-left",    label: "\u2196 U-L" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    color: ScreensaverConfig.direction === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: ScreensaverConfig.direction === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: ScreensaverConfig.direction = modelData.name
                    }
                }
            }
        }
    }
}
