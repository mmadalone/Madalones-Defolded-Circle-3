// Copyright (c) 2026 madalone. TV Static theme settings panel.
// Conditionally shown in ChargingScreen settings page when theme === "tvstatic".
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0
import Palettes 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property Item navUpTarget
    property Item navDownTarget

    property alias firstFocusItem: intensitySlider
    property alias lastFocusItem: flashBrightnessSlider

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true

    Layout.fillWidth: true
    spacing: 0

    // ---- Snow intensity ----
    Components.SettingRow {
        title: qsTr("Snow intensity")
        controlBelow: true

        Components.Slider {
            id: intensitySlider
            height: 60
            from: 0; to: 100; stepSize: 5
            value: ScreensaverConfig.tvStaticIntensity; live: true
            onMoved: ScreensaverConfig.tvStaticIntensity = value
            onUserInteractionEnded: ScreensaverConfig.tvStaticIntensity = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: snowSizeSlider
        }
    }

    // ---- Snow size (pixel cell) ----
    Components.SettingRow {
        title: qsTr("Snow size")
        controlBelow: true

        Components.Slider {
            id: snowSizeSlider
            height: 60
            from: 1; to: 8; stepSize: 1
            value: ScreensaverConfig.tvStaticSnowSize; live: true
            onMoved: ScreensaverConfig.tvStaticSnowSize = value
            onUserInteractionEnded: ScreensaverConfig.tvStaticSnowSize = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: intensitySlider
            KeyNavigation.down: scanlineStrengthSlider
        }
    }

    // ---- Scanline strength ----
    Components.SettingRow {
        title: qsTr("Scanline strength")
        controlBelow: true

        Components.Slider {
            id: scanlineStrengthSlider
            height: 60
            from: 0; to: 100; stepSize: 5
            value: ScreensaverConfig.tvStaticScanlineStrength; live: true
            onMoved: ScreensaverConfig.tvStaticScanlineStrength = value
            onUserInteractionEnded: ScreensaverConfig.tvStaticScanlineStrength = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: snowSizeSlider
            KeyNavigation.down: scanlineSpeedSlider
        }
    }

    // ---- Scanline roll speed (-100..100, 0 = static) ----
    Components.SettingRow {
        title: qsTr("Scanline roll speed")
        controlBelow: true

        Components.Slider {
            id: scanlineSpeedSlider
            height: 60
            from: -100; to: 100; stepSize: 5
            value: ScreensaverConfig.tvStaticScanlineSpeed; live: true
            onMoved: ScreensaverConfig.tvStaticScanlineSpeed = value
            onUserInteractionEnded: ScreensaverConfig.tvStaticScanlineSpeed = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: scanlineStrengthSlider
            KeyNavigation.down: chromaSlider
        }
    }

    // ---- Chroma bleed ----
    Components.SettingRow {
        title: qsTr("Chroma bleed (VHS)")
        controlBelow: true

        Components.Slider {
            id: chromaSlider
            height: 60
            from: 0; to: 100; stepSize: 5
            value: ScreensaverConfig.tvStaticChromaAmount; live: true
            onMoved: ScreensaverConfig.tvStaticChromaAmount = value
            onUserInteractionEnded: ScreensaverConfig.tvStaticChromaAmount = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: scanlineSpeedSlider
            KeyNavigation.down: trackingEnableSwitch
        }
    }

    // ---- Tracking bar enable, and its speed (off while the bar is) ----
    Components.SettingRow {
        title: qsTr("Rolling tracking bar")
        showDivider: false

        Components.Switch {
            id: trackingEnableSwitch
            icon: "uc:check"
            checked: ScreensaverConfig.tvStaticTrackingEnable
            trigger: function() { ScreensaverConfig.tvStaticTrackingEnable = !ScreensaverConfig.tvStaticTrackingEnable; }
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: chromaSlider
            KeyNavigation.down: trackingSpeedSlider
        }
    }

    Components.SettingRow {
        title: qsTr("Tracking bar speed")
        controlBelow: true

        Components.Slider {
            id: trackingSpeedSlider
            height: 60
            from: 0; to: 200; stepSize: 10
            value: ScreensaverConfig.tvStaticTrackingSpeed; live: true
            enabled: ScreensaverConfig.tvStaticTrackingEnable
            opacity: enabled ? 1.0 : 0.4
            onMoved: ScreensaverConfig.tvStaticTrackingSpeed = value
            onUserInteractionEnded: ScreensaverConfig.tvStaticTrackingSpeed = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: trackingEnableSwitch
            KeyNavigation.down: tintColorRow
        }
    }

    // ---- Tint color (7 solid swatches — matches Starfield) ----
    Components.SettingRow {
        title: qsTr("Tint")
        controlBelow: true

        Components.ChipPicker {
            id: tintColorRow
            columns: 4
            model: Palettes.textSwatches
            currentValue: ScreensaverConfig.tvStaticTint
            onPicked: ScreensaverConfig.tvStaticTint = value
            KeyNavigation.up: trackingSpeedSlider
            KeyNavigation.down: flashOnTapSwitch
        }
    }

    // NOTE: The "TV-off effect" section that previously lived here has moved
    // to Settings → Power saving → "Screen off animations" (shared system).
    // The TV Static CRT collapse is still the default behaviour when the user
    // picks "theme-native" as the effect style in Power saving.

    // ============ Channel Flash section ============
    // the heading and its first setting, one row for the reveal
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 20
            Layout.topMargin: 20
            wrapMode: Text.WordWrap
            color: colors.textPrimary
            text: qsTr("Channel flash")
            font: fonts.heading()
        }

        // ---- Flash on tap ----
        Components.SettingRow {
            title: qsTr("Flash on tap")

            Components.Switch {
                id: flashOnTapSwitch
                icon: "uc:check"
                checked: ScreensaverConfig.tvStaticFlashOnTap
                trigger: function() { ScreensaverConfig.tvStaticFlashOnTap = !ScreensaverConfig.tvStaticFlashOnTap; }
                highlight: activeFocus && ui.keyNavigationActive
                KeyNavigation.up: tintColorRow
                KeyNavigation.down: channelFlashAutoSwitch
            }
        }
    }

    // ---- Auto channel flashes, and their interval (off while they are) ----
    Components.SettingRow {
        title: qsTr("Auto flash bursts")
        showDivider: false

        Components.Switch {
            id: channelFlashAutoSwitch
            icon: "uc:check"
            checked: ScreensaverConfig.tvStaticChannelFlashAuto
            trigger: function() { ScreensaverConfig.tvStaticChannelFlashAuto = !ScreensaverConfig.tvStaticChannelFlashAuto; }
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: flashOnTapSwitch
            KeyNavigation.down: flashIntervalSlider
        }
    }

    // ---- Auto flash interval (seconds, ±50% jitter) ----
    Components.SettingRow {
        title: qsTr("Flash interval (seconds)")
        value: String(ScreensaverConfig.tvStaticFlashInterval)
        controlBelow: true

        Components.Slider {
            id: flashIntervalSlider
            height: 60
            from: 3; to: 120; stepSize: 1
            value: ScreensaverConfig.tvStaticFlashInterval; live: true; showLiveValue: false
            enabled: ScreensaverConfig.tvStaticChannelFlashAuto
            opacity: enabled ? 1.0 : 0.4
            onMoved: ScreensaverConfig.tvStaticFlashInterval = value
            onUserInteractionEnded: ScreensaverConfig.tvStaticFlashInterval = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: channelFlashAutoSwitch
            KeyNavigation.down: flashDurationSlider
        }
    }

    // ---- Flash duration (ms) ----
    Components.SettingRow {
        title: qsTr("Flash duration (ms)")
        value: String(ScreensaverConfig.tvStaticFlashDuration)
        controlBelow: true

        Components.Slider {
            id: flashDurationSlider
            height: 60
            from: 80; to: 1000; stepSize: 20
            value: ScreensaverConfig.tvStaticFlashDuration; live: true; showLiveValue: false
            onMoved: ScreensaverConfig.tvStaticFlashDuration = value
            onUserInteractionEnded: ScreensaverConfig.tvStaticFlashDuration = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: flashIntervalSlider
            KeyNavigation.down: flashBrightnessSlider
        }
    }

    // ---- Flash brightness ----
    Components.SettingRow {
        title: qsTr("Flash brightness")
        controlBelow: true

        Components.Slider {
            id: flashBrightnessSlider
            height: 60
            from: 0; to: 100; stepSize: 5
            value: ScreensaverConfig.tvStaticFlashBrightness; live: true
            onMoved: ScreensaverConfig.tvStaticFlashBrightness = value
            onUserInteractionEnded: ScreensaverConfig.tvStaticFlashBrightness = value
            highlight: activeFocus && ui.keyNavigationActive
            KeyNavigation.up: flashDurationSlider
            KeyNavigation.down: root.navDownTarget
        }
    }
}
