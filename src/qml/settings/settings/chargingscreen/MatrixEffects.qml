// Copyright (c) 2024 madalone. Matrix effects settings — visual toggles + glitch core.
// Sub-sections: DirectionGlitchSection, ChaosSection, TapSection, MessageSection.
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

    property alias firstFocusItem: invertTrailSwitch
    property alias lastFocusItem: messageSection.lastFocusItem
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // Leading separator
    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 10c. INVERT TRAIL
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Invert trail"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: invertTrailSwitch
                objectName: "invertTrailSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixInvertTrail
                trigger: function() { Config.chargingMatrixInvertTrail = !Config.chargingMatrixInvertTrail; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: glowSwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 11. HEAD GLOW
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Head glow"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: glowSwitch
                objectName: "glowSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlow
                trigger: function() { Config.chargingMatrixGlow = !Config.chargingMatrixGlow; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: invertTrailSwitch; KeyNavigation.down: glitchSwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 11. GLITCH EFFECT
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Glitch effect"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: glitchSwitch
                objectName: "glitchSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitch
                trigger: function() { Config.chargingMatrixGlitch = !Config.chargingMatrixGlitch; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glowSwitch; KeyNavigation.down: glitchRateSlider
            }
        }
    }

    // 12. GLITCH INTENSITY (visible when glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Glitch intensity"); font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: glitchRateSlider
            objectName: "glitchRateSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 80; stepSize: 5
            value: Config.chargingMatrixGlitchRate; live: true
            onValueChanged: Config.chargingMatrixGlitchRate = value
            onUserInteractionEnded: Config.chargingMatrixGlitchRate = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: glitchSwitch; KeyNavigation.down: glitchFlashSwitch
        }
    }

    // 12b. COLUMN FLASH (sub-toggle, visible when glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Column flash"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchFlashSwitch
                objectName: "glitchFlashSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchFlash
                trigger: function() { Config.chargingMatrixGlitchFlash = !Config.chargingMatrixGlitchFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glitchRateSlider; KeyNavigation.down: glitchStutterSwitch
            }
        }
    }

    // 12c. COLUMN STUTTER
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Column stutter"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchStutterSwitch
                objectName: "glitchStutterSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchStutter
                trigger: function() { Config.chargingMatrixGlitchStutter = !Config.chargingMatrixGlitchStutter; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glitchFlashSwitch; KeyNavigation.down: glitchReverseSwitch
            }
        }
    }

    // 12d. REVERSE GLOW
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Reverse glow"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchReverseSwitch
                objectName: "glitchReverseSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchReverse
                trigger: function() { Config.chargingMatrixGlitchReverse = !Config.chargingMatrixGlitchReverse; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glitchStutterSwitch; KeyNavigation.down: directionGlitchSection.firstFocusItem
            }
        }
    }

    // --- Extracted sub-components ---

    DirectionGlitchSection {
        id: directionGlitchSection
        settingsPage: root.settingsPage
        Layout.fillWidth: true
        navUpTarget: glitchReverseSwitch
        navDownTarget: chaosSection.firstFocusItem
    }

    ChaosSection {
        id: chaosSection
        settingsPage: root.settingsPage
        Layout.fillWidth: true
        navUpTarget: directionGlitchSection.lastFocusItem
        navDownTarget: tapSection.firstFocusItem
    }

    TapSection {
        id: tapSection
        settingsPage: root.settingsPage
        Layout.fillWidth: true
        navUpTarget: chaosSection.lastFocusItem
        navDownTarget: messageSection.firstFocusItem
    }

    MessageSection {
        id: messageSection
        settingsPage: root.settingsPage
        Layout.fillWidth: true
        navUpTarget: tapSection.lastFocusItem
        navDownTarget: root.navDownTarget
    }
}
