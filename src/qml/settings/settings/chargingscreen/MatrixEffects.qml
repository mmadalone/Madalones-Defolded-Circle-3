// Copyright (c) 2024 madalone. Matrix effects settings — visual toggles + glitch core.
// Sub-sections: DirectionGlitchSection, ChaosSection, TapSection, MessageSection.
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
                checked: ScreensaverConfig.invertTrail
                trigger: function() { ScreensaverConfig.invertTrail = !ScreensaverConfig.invertTrail; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Invert trail"
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
                checked: ScreensaverConfig.glow
                trigger: function() { ScreensaverConfig.glow = !ScreensaverConfig.glow; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Head glow"
                KeyNavigation.up: invertTrailSwitch; KeyNavigation.down: glowFadeSlider
            }
        }
    }

    // GLOW FADE (residual glow duration)
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text {
            Layout.fillWidth: true; color: colors.light
            text: qsTr("Glow fade"); font: fonts.primaryFont(26)
        }
        Components.Slider {
            id: glowFadeSlider
            objectName: "glowFadeSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 0; to: 100; stepSize: 5
            value: ScreensaverConfig.glowFade; live: true
            onMoved: ScreensaverConfig.glowFade = value
            onUserInteractionEnded: ScreensaverConfig.glowFade = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Glow fade " + value
            KeyNavigation.up: glowSwitch; KeyNavigation.down: depthGlowSwitch
        }
    }

    // DEPTH GLOW (glow cells shrink with age for depth illusion)
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Depth glow"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: depthGlowSwitch
                objectName: "depthGlowSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.depthGlow
                trigger: function() { ScreensaverConfig.depthGlow = !ScreensaverConfig.depthGlow; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Depth glow"
                KeyNavigation.up: glowFadeSlider; KeyNavigation.down: depthGlowMinSlider
            }
        }
    }

    // DEPTH GLOW MIN SIZE (visible when depth glow is on)
    ColumnLayout {
        visible: ScreensaverConfig.depthGlow
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text {
            Layout.fillWidth: true; color: colors.light
            text: qsTr("Min size"); font: fonts.primaryFont(26)
        }
        Components.Slider {
            id: depthGlowMinSlider
            objectName: "depthGlowMinSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 90; stepSize: 5
            value: ScreensaverConfig.depthGlowMin; live: true
            onMoved: ScreensaverConfig.depthGlowMin = value
            onUserInteractionEnded: ScreensaverConfig.depthGlowMin = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Minimum glow size " + value + "%"
            KeyNavigation.up: depthGlowSwitch; KeyNavigation.down: layersEnabledSwitch
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // RAIN LAYERS (multi-grid depth)
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Rain layers"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: layersEnabledSwitch
                objectName: "layersEnabledSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.layersEnabled
                trigger: function() { ScreensaverConfig.layersEnabled = !ScreensaverConfig.layersEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Rain layers"
                KeyNavigation.up: ScreensaverConfig.depthGlow ? depthGlowMinSlider : depthGlowSwitch
                KeyNavigation.down: depthEnabledSwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // COLOR LAYERS
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Color layers"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: depthEnabledSwitch
                objectName: "depthEnabledSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.depthEnabled
                trigger: function() { ScreensaverConfig.depthEnabled = !ScreensaverConfig.depthEnabled; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Color layers"
                KeyNavigation.up: layersEnabledSwitch; KeyNavigation.down: depthIntensitySlider
            }
        }
    }

    // COLOR LAYERS INTENSITY (visible when depth is on)
    ColumnLayout {
        visible: ScreensaverConfig.depthEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text {
            Layout.fillWidth: true; color: colors.light
            text: qsTr("Intensity"); font: fonts.primaryFont(26)
        }
        Components.Slider {
            id: depthIntensitySlider
            objectName: "depthIntensitySlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.depthIntensity; live: true
            onMoved: ScreensaverConfig.depthIntensity = value
            onUserInteractionEnded: ScreensaverConfig.depthIntensity = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Depth intensity " + value
            KeyNavigation.up: depthEnabledSwitch; KeyNavigation.down: depthOverlaySwitch
        }
    }

    // COLOR LAYERS OVERLAY MODE (visible when depth is on)
    ColumnLayout {
        visible: ScreensaverConfig.depthEnabled
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Overlay mode"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: depthOverlaySwitch
                objectName: "depthOverlaySwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: ScreensaverConfig.depthOverlay
                trigger: function() { ScreensaverConfig.depthOverlay = !ScreensaverConfig.depthOverlay; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Depth overlay mode"
                KeyNavigation.up: depthIntensitySlider; KeyNavigation.down: glitchSwitch
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
                checked: ScreensaverConfig.glitch
                trigger: function() { ScreensaverConfig.glitch = !ScreensaverConfig.glitch; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Glitch effect"
                KeyNavigation.up: depthOverlaySwitch; KeyNavigation.down: glitchRateSlider
            }
        }
    }

    // 12. GLITCH INTENSITY (visible when glitch is on)
    ColumnLayout {
        visible: ScreensaverConfig.glitch
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
            value: ScreensaverConfig.glitchRate; live: true
            onMoved: ScreensaverConfig.glitchRate = value
            onUserInteractionEnded: ScreensaverConfig.glitchRate = value
            highlight: activeFocus && ui.keyNavigationEnabled
            Accessible.name: "Glitch intensity " + value
            KeyNavigation.up: glitchSwitch; KeyNavigation.down: glitchFlashSwitch
        }
    }

    // 12b. COLUMN FLASH (sub-toggle, visible when glitch is on)
    ColumnLayout {
        visible: ScreensaverConfig.glitch
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
                checked: ScreensaverConfig.glitchFlash
                trigger: function() { ScreensaverConfig.glitchFlash = !ScreensaverConfig.glitchFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Column flash"
                KeyNavigation.up: glitchRateSlider; KeyNavigation.down: glitchStutterSwitch
            }
        }
    }

    // 12c. COLUMN STUTTER
    ColumnLayout {
        visible: ScreensaverConfig.glitch
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
                checked: ScreensaverConfig.glitchStutter
                trigger: function() { ScreensaverConfig.glitchStutter = !ScreensaverConfig.glitchStutter; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Column stutter"
                KeyNavigation.up: glitchFlashSwitch; KeyNavigation.down: glitchReverseSwitch
            }
        }
    }

    // 12d. REVERSE GLOW
    ColumnLayout {
        visible: ScreensaverConfig.glitch
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
                checked: ScreensaverConfig.glitchReverse
                trigger: function() { ScreensaverConfig.glitchReverse = !ScreensaverConfig.glitchReverse; }
                highlight: activeFocus && ui.keyNavigationEnabled
                Accessible.name: "Reverse glow"
                KeyNavigation.up: glitchStutterSwitch; KeyNavigation.down: directionGlitchSection.firstFocusItem
            }
        }
    }

    // --- Extracted sub-components ---

    DirectionGlitchSection {
        id: directionGlitchSection
        visible: ScreensaverConfig.glitch
        settingsPage: root.settingsPage
        Layout.fillWidth: true
        navUpTarget: glitchReverseSwitch
        navDownTarget: chaosSection.firstFocusItem
    }

    ChaosSection {
        id: chaosSection
        visible: ScreensaverConfig.glitch
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
