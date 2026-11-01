// Copyright (c) 2026 madalone. Matrix effects settings — visual toggles + glitch core.
// Sub-sections: DirectionGlitchSection, ChaosSection, TapSection, MessageSection.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: invertTrailSwitch
    property alias lastFocusItem: messageSection.lastFocusItem
    property Item navUpTarget
    property Item navDownTarget

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true

    spacing: 0

    Components.SettingRow {
        title: qsTr("Invert trail")

        Components.Switch {
            id: invertTrailSwitch
            objectName: "invertTrailSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.invertTrail
            trigger: function() { ScreensaverConfig.invertTrail = !ScreensaverConfig.invertTrail; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Invert trail"
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: glowSwitch
        }
    }

    Components.SettingRow {
        title: qsTr("Head glow")

        Components.Switch {
            id: glowSwitch
            objectName: "glowSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glow
            trigger: function() { ScreensaverConfig.glow = !ScreensaverConfig.glow; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Head glow"
            KeyNavigation.up: invertTrailSwitch
            KeyNavigation.down: glowFadeSlider
        }
    }

    // GLOW FADE: how long a character keeps glowing after the head of its stream has passed
    Components.SettingRow {
        title: qsTr("Glow fade")
        controlBelow: true

        Components.Slider {
            id: glowFadeSlider
            objectName: "glowFadeSlider"
            height: 60
            from: 0; to: 100; stepSize: 5
            value: ScreensaverConfig.glowFade; live: true
            onMoved: ScreensaverConfig.glowFade = value
            onUserInteractionEnded: ScreensaverConfig.glowFade = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Glow fade " + value
            KeyNavigation.up: glowSwitch
            KeyNavigation.down: depthGlowSwitch
        }
    }

    // DEPTH GLOW: glowing characters shrink with age, for depth
    Components.SettingRow {
        title: qsTr("Depth glow")
        showDivider: !ScreensaverConfig.depthGlow

        Components.Switch {
            id: depthGlowSwitch
            objectName: "depthGlowSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.depthGlow
            trigger: function() { ScreensaverConfig.depthGlow = !ScreensaverConfig.depthGlow; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Depth glow"
            KeyNavigation.up: glowFadeSlider
            KeyNavigation.down: depthGlowMinSlider
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.depthGlow
        title: qsTr("Min size")
        help: qsTr("The smallest size a fading character shrinks to.")
        controlBelow: true

        Components.Slider {
            id: depthGlowMinSlider
            objectName: "depthGlowMinSlider"
            height: 60
            from: 10; to: 90; stepSize: 5
            value: ScreensaverConfig.depthGlowMin; live: true
            onMoved: ScreensaverConfig.depthGlowMin = value
            onUserInteractionEnded: ScreensaverConfig.depthGlowMin = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Minimum glow size " + value + "%"
            KeyNavigation.up: depthGlowSwitch
            KeyNavigation.down: layersEnabledSwitch
        }
    }

    // RAIN LAYERS (multi-grid depth)
    Components.SettingRow {
        title: qsTr("Rain layers")

        Components.Switch {
            id: layersEnabledSwitch
            objectName: "layersEnabledSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.layersEnabled
            trigger: function() { ScreensaverConfig.layersEnabled = !ScreensaverConfig.layersEnabled; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Rain layers"
            KeyNavigation.up: ScreensaverConfig.depthGlow ? depthGlowMinSlider : depthGlowSwitch
            KeyNavigation.down: depthEnabledSwitch
        }
    }

    // COLOR LAYERS, and their settings while they are on
    Components.SettingRow {
        title: qsTr("Color layers")
        showDivider: !ScreensaverConfig.depthEnabled

        Components.Switch {
            id: depthEnabledSwitch
            objectName: "depthEnabledSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.depthEnabled
            trigger: function() { ScreensaverConfig.depthEnabled = !ScreensaverConfig.depthEnabled; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Color layers"
            KeyNavigation.up: layersEnabledSwitch
            KeyNavigation.down: depthIntensitySlider
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.depthEnabled
        title: qsTr("Intensity")
        help: qsTr("How much the color layers differ from each other.")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: depthIntensitySlider
            objectName: "depthIntensitySlider"
            height: 60
            from: 10; to: 100; stepSize: 5
            value: ScreensaverConfig.depthIntensity; live: true
            onMoved: ScreensaverConfig.depthIntensity = value
            onUserInteractionEnded: ScreensaverConfig.depthIntensity = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Depth intensity " + value
            KeyNavigation.up: depthEnabledSwitch
            KeyNavigation.down: depthOverlaySwitch
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.depthEnabled
        title: qsTr("Overlay mode")
        help: qsTr("Applies the color layers to about a third of the streams only.")

        Components.Switch {
            id: depthOverlaySwitch
            objectName: "depthOverlaySwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.depthOverlay
            trigger: function() { ScreensaverConfig.depthOverlay = !ScreensaverConfig.depthOverlay; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Depth overlay mode"
            KeyNavigation.up: depthIntensitySlider
            KeyNavigation.down: glitchSwitch
        }
    }

    // GLITCH EFFECT, and its settings while it is on; direction change and chaos events follow as sections of their own
    Components.SettingRow {
        title: qsTr("Glitch effect")
        showDivider: !ScreensaverConfig.glitch

        Components.Switch {
            id: glitchSwitch
            objectName: "glitchSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitch
            trigger: function() { ScreensaverConfig.glitch = !ScreensaverConfig.glitch; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Glitch effect"
            KeyNavigation.up: depthOverlaySwitch
            KeyNavigation.down: glitchRateSlider
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.glitch
        title: qsTr("Glitch intensity")
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: glitchRateSlider
            objectName: "glitchRateSlider"
            height: 60
            from: 5; to: 80; stepSize: 5
            value: ScreensaverConfig.glitchRate; live: true
            onMoved: ScreensaverConfig.glitchRate = value
            onUserInteractionEnded: ScreensaverConfig.glitchRate = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Glitch intensity " + value
            KeyNavigation.up: glitchSwitch
            KeyNavigation.down: glitchFlashSwitch
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.glitch
        title: qsTr("Column flash")
        showDivider: false

        Components.Switch {
            id: glitchFlashSwitch
            objectName: "glitchFlashSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchFlash
            trigger: function() { ScreensaverConfig.glitchFlash = !ScreensaverConfig.glitchFlash; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Column flash"
            KeyNavigation.up: glitchRateSlider
            KeyNavigation.down: glitchStutterSwitch
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.glitch
        title: qsTr("Column stutter")
        showDivider: false

        Components.Switch {
            id: glitchStutterSwitch
            objectName: "glitchStutterSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchStutter
            trigger: function() { ScreensaverConfig.glitchStutter = !ScreensaverConfig.glitchStutter; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Column stutter"
            KeyNavigation.up: glitchFlashSwitch
            KeyNavigation.down: glitchReverseSwitch
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.glitch
        title: qsTr("Reverse glow")

        Components.Switch {
            id: glitchReverseSwitch
            objectName: "glitchReverseSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.glitchReverse
            trigger: function() { ScreensaverConfig.glitchReverse = !ScreensaverConfig.glitchReverse; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Reverse glow"
            KeyNavigation.up: glitchStutterSwitch
            KeyNavigation.down: directionGlitchSection.firstFocusItem
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
