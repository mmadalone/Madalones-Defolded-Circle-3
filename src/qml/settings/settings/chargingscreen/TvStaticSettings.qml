// Copyright (c) 2026 madalone. TV Static theme settings panel.
// Conditionally shown in ChargingScreen settings page when theme === "tvstatic".
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

    property alias firstFocusItem: intensitySlider
    property alias lastFocusItem: flashBrightnessSlider

    Layout.fillWidth: true
    Layout.leftMargin: 10; Layout.rightMargin: 10
    spacing: 20

    // ---- Snow intensity ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Snow intensity"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: intensitySlider
        height: 60; Layout.fillWidth: true
        from: 0; to: 100; stepSize: 5
        value: ScreensaverConfig.tvStaticIntensity; live: true
        onMoved: ScreensaverConfig.tvStaticIntensity = value
        onUserInteractionEnded: ScreensaverConfig.tvStaticIntensity = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: root.navUpTarget
        KeyNavigation.down: snowSizeSlider
    }

    // ---- Snow size (pixel cell) ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Snow size"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: snowSizeSlider
        height: 60; Layout.fillWidth: true
        from: 1; to: 8; stepSize: 1
        value: ScreensaverConfig.tvStaticSnowSize; live: true
        onMoved: ScreensaverConfig.tvStaticSnowSize = value
        onUserInteractionEnded: ScreensaverConfig.tvStaticSnowSize = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: intensitySlider
        KeyNavigation.down: scanlineStrengthSlider
    }

    // ---- Scanline strength ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Scanline strength"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: scanlineStrengthSlider
        height: 60; Layout.fillWidth: true
        from: 0; to: 100; stepSize: 5
        value: ScreensaverConfig.tvStaticScanlineStrength; live: true
        onMoved: ScreensaverConfig.tvStaticScanlineStrength = value
        onUserInteractionEnded: ScreensaverConfig.tvStaticScanlineStrength = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: snowSizeSlider
        KeyNavigation.down: scanlineSpeedSlider
    }

    // ---- Scanline roll speed (-100..100, 0 = static) ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Scanline roll speed"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: scanlineSpeedSlider
        height: 60; Layout.fillWidth: true
        from: -100; to: 100; stepSize: 5
        value: ScreensaverConfig.tvStaticScanlineSpeed; live: true
        onMoved: ScreensaverConfig.tvStaticScanlineSpeed = value
        onUserInteractionEnded: ScreensaverConfig.tvStaticScanlineSpeed = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: scanlineStrengthSlider
        KeyNavigation.down: chromaSlider
    }

    // ---- Chroma bleed ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Chroma bleed (VHS)"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: chromaSlider
        height: 60; Layout.fillWidth: true
        from: 0; to: 100; stepSize: 5
        value: ScreensaverConfig.tvStaticChromaAmount; live: true
        onMoved: ScreensaverConfig.tvStaticChromaAmount = value
        onUserInteractionEnded: ScreensaverConfig.tvStaticChromaAmount = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: scanlineSpeedSlider
        KeyNavigation.down: trackingEnableSwitch
    }

    // ---- Tracking bar enable ----
    RowLayout {
        spacing: 10
        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Rolling tracking bar"); font: fonts.primaryFont(30)
        }
        Components.Switch {
            id: trackingEnableSwitch
            icon: "uc:check"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            checked: ScreensaverConfig.tvStaticTrackingEnable
            trigger: function() { ScreensaverConfig.tvStaticTrackingEnable = !ScreensaverConfig.tvStaticTrackingEnable; }
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: chromaSlider
            KeyNavigation.down: trackingSpeedSlider
        }
    }

    // ---- Tracking bar speed ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Tracking bar speed"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: trackingSpeedSlider
        height: 60; Layout.fillWidth: true
        from: 0; to: 200; stepSize: 10
        value: ScreensaverConfig.tvStaticTrackingSpeed; live: true
        enabled: ScreensaverConfig.tvStaticTrackingEnable
        opacity: enabled ? 1.0 : 0.4
        onMoved: ScreensaverConfig.tvStaticTrackingSpeed = value
        onUserInteractionEnded: ScreensaverConfig.tvStaticTrackingSpeed = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: trackingEnableSwitch
        KeyNavigation.down: tintColorRow
    }

    // ---- Tint color (7 solid swatches — matches Starfield) ----
    Text { Layout.fillWidth: true; color: colors.offwhite; text: qsTr("Tint"); font: fonts.primaryFont(30) }
    RowLayout {
        id: tintColorRow
        spacing: 6; focus: true
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: trackingSpeedSlider
        KeyNavigation.down: flashOnTapSwitch
        Keys.onLeftPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; root.settingsPage.cycleOption(c, ScreensaverConfig.tvStaticTint, function(v){ ScreensaverConfig.tvStaticTint = v }, -1) }
        Keys.onRightPressed: { var c = ["#ffffff","#00ff41","#00b4d8","#ff0040","#ffbf00","#bf00ff","#d0d0d0"]; root.settingsPage.cycleOption(c, ScreensaverConfig.tvStaticTint, function(v){ ScreensaverConfig.tvStaticTint = v }, 1) }
        Repeater {
            model: [{ color: "#ffffff" },{ color: "#00ff41" },{ color: "#00b4d8" },{ color: "#ff0040" },{ color: "#ffbf00" },{ color: "#bf00ff" },{ color: "#d0d0d0" }]
            Rectangle {
                Layout.fillWidth: true; height: 36; radius: 6; color: modelData.color
                border { color: ScreensaverConfig.tvStaticTint === modelData.color ? colors.offwhite : colors.medium; width: ScreensaverConfig.tvStaticTint === modelData.color ? 3 : 1 }
                Components.HapticMouseArea { anchors.fill: parent; onClicked: ScreensaverConfig.tvStaticTint = modelData.color }
            }
        }
    }

    // NOTE: The "TV-off effect" section that previously lived here has moved
    // to Settings → Power saving → "Screen off animations" (shared system).
    // The TV Static CRT collapse is still the default behaviour when the user
    // picks "theme-native" as the effect style in Power saving.

    // ============ Channel Flash section ============
    Text {
        Layout.fillWidth: true; Layout.topMargin: 8; color: colors.medium
        text: qsTr("— Channel flash —"); font: fonts.primaryFont(22)
    }

    // ---- Flash on tap ----
    RowLayout {
        spacing: 10
        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Flash on tap"); font: fonts.primaryFont(30)
        }
        Components.Switch {
            id: flashOnTapSwitch
            icon: "uc:check"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            checked: ScreensaverConfig.tvStaticFlashOnTap
            trigger: function() { ScreensaverConfig.tvStaticFlashOnTap = !ScreensaverConfig.tvStaticFlashOnTap; }
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: tintColorRow
            KeyNavigation.down: channelFlashAutoSwitch
        }
    }

    // ---- Auto channel flashes ----
    RowLayout {
        spacing: 10
        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Auto flash bursts"); font: fonts.primaryFont(30)
        }
        Components.Switch {
            id: channelFlashAutoSwitch
            icon: "uc:check"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            checked: ScreensaverConfig.tvStaticChannelFlashAuto
            trigger: function() { ScreensaverConfig.tvStaticChannelFlashAuto = !ScreensaverConfig.tvStaticChannelFlashAuto; }
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: flashOnTapSwitch
            KeyNavigation.down: flashIntervalSlider
        }
    }

    // ---- Auto flash interval (seconds, ±50% jitter) ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Flash interval (seconds)"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: flashIntervalSlider
        height: 60; Layout.fillWidth: true
        from: 3; to: 120; stepSize: 1
        value: ScreensaverConfig.tvStaticFlashInterval; live: true
        enabled: ScreensaverConfig.tvStaticChannelFlashAuto
        opacity: enabled ? 1.0 : 0.4
        onMoved: ScreensaverConfig.tvStaticFlashInterval = value
        onUserInteractionEnded: ScreensaverConfig.tvStaticFlashInterval = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: channelFlashAutoSwitch
        KeyNavigation.down: flashDurationSlider
    }

    // ---- Flash duration (ms) ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Flash duration (ms)"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: flashDurationSlider
        height: 60; Layout.fillWidth: true
        from: 80; to: 1000; stepSize: 20
        value: ScreensaverConfig.tvStaticFlashDuration; live: true
        onMoved: ScreensaverConfig.tvStaticFlashDuration = value
        onUserInteractionEnded: ScreensaverConfig.tvStaticFlashDuration = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: flashIntervalSlider
        KeyNavigation.down: flashBrightnessSlider
    }

    // ---- Flash brightness ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Flash brightness"); font: fonts.primaryFont(30)
    }
    Components.Slider {
        id: flashBrightnessSlider
        height: 60; Layout.fillWidth: true
        from: 0; to: 100; stepSize: 5
        value: ScreensaverConfig.tvStaticFlashBrightness; live: true
        onMoved: ScreensaverConfig.tvStaticFlashBrightness = value
        onUserInteractionEnded: ScreensaverConfig.tvStaticFlashBrightness = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: flashDurationSlider
        KeyNavigation.down: root.navDownTarget
    }
}
