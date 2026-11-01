// Copyright (c) 2026 madalone. General behavior settings component.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: tapToCloseSwitch
    property alias lastFocusItem: debugOverlaySwitch
    property Item navUpTarget
    property Item navDownTarget

    // ChargingScreen.qml reveals the whole row of the focused control: the rows are the children of this root
    readonly property bool settingsSection: true
    readonly property bool matrix: ScreensaverConfig.theme === "matrix"

    spacing: 0

    Components.SettingRow {
        title: qsTr("Double-tap to close")
        // madalone (v1.4.43): clarify single-tap vs double-tap semantics.
        help: qsTr("On: tap twice to dismiss the screensaver (anti-accidental). Off: a single tap dismisses.")

        Components.Switch {
            id: tapToCloseSwitch
            objectName: "tapToCloseSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapToClose
            trigger: function() { ScreensaverConfig.tapToClose = !ScreensaverConfig.tapToClose; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Double-tap to close"
            KeyNavigation.up: root.navUpTarget
            KeyNavigation.down: motionToCloseSwitch
        }
    }

    Components.SettingRow {
        title: qsTr("Close on wake")

        Components.Switch {
            id: motionToCloseSwitch
            objectName: "motionToCloseSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.motionToClose
            trigger: function() { ScreensaverConfig.motionToClose = !ScreensaverConfig.motionToClose; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Close on wake"
            KeyNavigation.up: tapToCloseSwitch
            KeyNavigation.down: root.matrix ? dpadEnabledSwitch : idleEnabledSwitch
        }
    }

    // DPAD INTERACTIVE (Matrix only — Starfield/Minimal have no interactive input), and its settings while it is on
    Components.SettingRow {
        visible: root.matrix
        title: qsTr("DPAD interactive")
        showDivider: !ScreensaverConfig.dpadEnabled

        Components.Switch {
            id: dpadEnabledSwitch
            objectName: "dpadEnabledSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.dpadEnabled
            trigger: function() { ScreensaverConfig.dpadEnabled = !ScreensaverConfig.dpadEnabled; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "DPAD interactive"
            KeyNavigation.up: motionToCloseSwitch
            KeyNavigation.down: ScreensaverConfig.dpadEnabled ? dpadPersistSwitch : tapDirectionSwitch
        }
    }

    Components.SettingRow {
        visible: root.matrix && ScreensaverConfig.dpadEnabled
        title: qsTr("Remember direction")
        showDivider: false

        Components.Switch {
            id: dpadPersistSwitch
            objectName: "dpadPersistSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.dpadPersist
            trigger: function() {
                ScreensaverConfig.dpadPersist = !ScreensaverConfig.dpadPersist;
                if (!ScreensaverConfig.dpadPersist) ScreensaverConfig.lastDirection = "";
            }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Remember direction"
            KeyNavigation.up: dpadEnabledSwitch
            KeyNavigation.down: touchbarSpeedSwitch
        }
    }

    Components.SettingRow {
        visible: root.matrix && ScreensaverConfig.dpadEnabled
        title: qsTr("Touchbar speed")

        Components.Switch {
            id: touchbarSpeedSwitch
            objectName: "touchbarSpeedSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.dpadTouchbarSpeed
            trigger: function() { ScreensaverConfig.dpadTouchbarSpeed = !ScreensaverConfig.dpadTouchbarSpeed; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Touchbar speed control"
            KeyNavigation.up: dpadPersistSwitch
            KeyNavigation.down: tapDirectionSwitch
        }
    }

    // TOUCH DIRECTIONS (Matrix only, mutually exclusive with DPAD interactive), and its settings while it is on
    Components.SettingRow {
        visible: root.matrix
        title: qsTr("Touch directions")
        help: ScreensaverConfig.tapDirection ? qsTr("Tap screen zones to change direction. Triple-tap center to close.") : ""
        showDivider: !ScreensaverConfig.tapDirection

        Components.Switch {
            id: tapDirectionSwitch
            objectName: "tapDirectionSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapDirection
            trigger: function() { ScreensaverConfig.tapDirection = !ScreensaverConfig.tapDirection; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Touch directions"
            KeyNavigation.up: ScreensaverConfig.dpadEnabled ? touchbarSpeedSwitch : dpadEnabledSwitch
            KeyNavigation.down: ScreensaverConfig.tapDirection ? tapDirPersistSwitch : idleEnabledSwitch
        }
    }

    Components.SettingRow {
        visible: root.matrix && ScreensaverConfig.tapDirection
        title: qsTr("Remember direction")
        showDivider: false

        Components.Switch {
            id: tapDirPersistSwitch
            objectName: "tapDirPersistSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.dpadPersist
            trigger: function() {
                ScreensaverConfig.dpadPersist = !ScreensaverConfig.dpadPersist;
                if (!ScreensaverConfig.dpadPersist) ScreensaverConfig.lastDirection = "";
            }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Remember direction"
            KeyNavigation.up: tapDirectionSwitch
            KeyNavigation.down: swipeSpeedSwitch
        }
    }

    Components.SettingRow {
        visible: root.matrix && ScreensaverConfig.tapDirection
        title: qsTr("Swipe speed")

        Components.Switch {
            id: swipeSpeedSwitch
            objectName: "swipeSpeedSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.tapSwipeSpeed
            trigger: function() { ScreensaverConfig.tapSwipeSpeed = !ScreensaverConfig.tapSwipeSpeed; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Swipe to adjust speed"
            KeyNavigation.up: tapDirPersistSwitch
            KeyNavigation.down: idleEnabledSwitch
        }
    }

    // IDLE SCREENSAVER, and its timeout while it is on
    Components.SettingRow {
        title: qsTr("Idle screensaver")
        showDivider: !ScreensaverConfig.idleEnabled

        Components.Switch {
            id: idleEnabledSwitch
            objectName: "idleEnabledSwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.idleEnabled
            trigger: function() { ScreensaverConfig.idleEnabled = !ScreensaverConfig.idleEnabled; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Idle screensaver"
            KeyNavigation.up: root.matrix ? (ScreensaverConfig.tapDirection ? swipeSpeedSwitch : tapDirectionSwitch)
                                          : motionToCloseSwitch
            KeyNavigation.down: ScreensaverConfig.idleEnabled ? idleTimeoutSlider : dockedRearmSlider
        }
    }

    Components.SettingRow {
        visible: ScreensaverConfig.idleEnabled
        title: qsTr("Idle timeout")
        value: ScreensaverConfig.idleTimeout + "s"
        controlBelow: true

        Components.Slider {
            id: idleTimeoutSlider
            objectName: "idleTimeoutSlider"
            height: 60
            from: 15; to: 55; stepSize: 5
            value: ScreensaverConfig.idleTimeout; live: true; showLiveValue: false
            onMoved: ScreensaverConfig.idleTimeout = value
            onUserInteractionEnded: ScreensaverConfig.idleTimeout = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Idle timeout " + value
            KeyNavigation.up: idleEnabledSwitch
            KeyNavigation.down: dockedRearmSlider
        }
    }

    // DOCKED REARM TIMEOUT (madalone v1.4.15) — always visible, independent of idleEnabled
    Components.SettingRow {
        title: qsTr("Run after dismissal while docked")
        value: ScreensaverConfig.reopenWhileDockedSec + "s"
        help: qsTr("Restart the screensaver after this many seconds of inactivity when on the dock.")
        controlBelow: true
        // the last row of the page unless the Matrix profiling switch follows
        showDivider: root.matrix

        Components.Slider {
            id: dockedRearmSlider
            objectName: "dockedRearmSlider"
            height: 60
            from: 5; to: 120; stepSize: 5
            value: ScreensaverConfig.reopenWhileDockedSec; live: true; showLiveValue: false
            onMoved: ScreensaverConfig.reopenWhileDockedSec = value
            onUserInteractionEnded: ScreensaverConfig.reopenWhileDockedSec = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Docked rearm timeout " + value
            KeyNavigation.up: idleTimeoutSlider
            // the end of the chain on the other themes, where the page scrolls on to its end
            KeyNavigation.down: root.matrix ? debugOverlaySwitch : root.navDownTarget
        }
    }

    // DEBUG — ATLAS PROFILING OVERLAY (Matrix only; profiling tool)
    // Shows an on-screen strip with the last buildCombinedAtlas + first-paint
    // phase timings. Useful for regression investigation. Off by default.
    Components.SettingRow {
        visible: root.matrix
        title: qsTr("Atlas profiling overlay")
        help: qsTr("Shows atlas build phase timings at the top of the Matrix screensaver. Profiling tool — leave off during normal use.")
        showDivider: false

        Components.Switch {
            id: debugOverlaySwitch
            objectName: "debugOverlaySwitch"
            icon: "uc:check"
            checked: ScreensaverConfig.debugAtlasOverlay
            trigger: function() { ScreensaverConfig.debugAtlasOverlay = !ScreensaverConfig.debugAtlasOverlay; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Atlas profiling overlay"
            KeyNavigation.up: dockedRearmSlider
            KeyNavigation.down: root.navDownTarget
        }
    }
}
