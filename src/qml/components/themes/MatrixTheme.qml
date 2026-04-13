// Copyright (c) 2026 madalone. Matrix rain charging screen theme.
// Uses C++ QQuickItem (MatrixRainItem) for GPU-accelerated rendering.
// No theme-native screen-off animation — Matrix falls through to the
// shared ScreenOffOverlay (fade/pixelate/dissolve/etc.) selected in
// Power settings, same as Starfield and Minimal.
// Config binding handled in C++ via ScreensaverConfig → MatrixRainItem::bindToScreensaverConfig().
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import Battery 1.0
import MatrixRain 1.0
import ScreensaverConfig 1.0

import "qrc:/components/overlays" as Overlays

Item {
    id: root
    anchors.fill: parent

    // Runtime state (set by ChargingScreen, not config)
    property bool isClosing: false
    property bool displayOff: false
    property alias matrixRainItem: matrixRain  // expose for ChargingScreen enter state machine

    // DPAD gravity override. Initial value from Config via binding (evaluates once
    // at creation). DPAD assignment breaks the binding (intended — imperative takes over).
    // Connections handler re-syncs when Config changes after that.
    property bool localGravity: ScreensaverConfig.gravityMode

    Connections {
        target: ScreensaverConfig
        function onGravityModeChanged() { root.localGravity = ScreensaverConfig.gravityMode; }
    }

    // Forward interactive input from ChargingScreen DPAD to C++ MatrixRain.
    // Sync localGravity so the QML binding doesn't reassert the config value
    // onto the C++ property, which would undo the DPAD direction change.
    function interactiveInput(action) {
        if (action === "up" || action === "down" || action === "left" || action === "right" ||
            action === "up-left" || action === "down-left" || action === "up-right" || action === "down-right") {
            root.localGravity = true;
        } else if (action === "restore") {
            root.localGravity = false;
        }
        matrixRain.interactiveInput(action);
    }

    // Bypass QML running-binding race on wake via C++ defensive reset.
    function cancelScreenOff() {
        matrixRain.resetAfterScreenOff();
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    MatrixRain {
        id: matrixRain
        anchors.fill: parent
        // All config properties are wired in C++ via bindToScreensaverConfig().
        // Only non-config properties bound here:
        gravityMode: root.localGravity
        displayOff: root.displayOff
        // Intentionally NOT gating running on `!root.displayOff`. Pausing
        // the sim on display-off transitions causes a scene-graph race on
        // wake — the running binding fires setRunning(false)→setRunning(true)
        // in the same QML event loop tick that mutates theme state, and
        // the first updatePaintNode() after wake can submit an empty node,
        // leaving the rain area black. Qt/KDAB docs explicitly warn against
        // binding a custom QQuickItem's runtime state to high-frequency
        // property transitions. Keeping the sim ticking through display-off
        // matches the pre-screen-off-animation-system behaviour that worked
        // reliably; Qt's platform integration stops compositing frames when
        // the display is physically off so GPU cost is near-zero, and
        // advanceSimulation() costs ~4% of one core on UC3 — acceptable.
        running: root.visible && !root.isClosing
    }

    // Clock overlay
    Overlays.ClockOverlay {
        visible: ScreensaverConfig.showClock && (!ScreensaverConfig.clockDockedOnly || Battery.powerSupply)
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: ScreensaverConfig.clockPosition === "top" ? parent.top : undefined
            topMargin: ScreensaverConfig.clockPosition === "top" ? parent.height * 0.15 : 0
            verticalCenter: ScreensaverConfig.clockPosition === "center" ? parent.verticalCenter : undefined
            bottom: ScreensaverConfig.clockPosition === "bottom" ? parent.bottom : undefined
            bottomMargin: ScreensaverConfig.clockPosition === "bottom" ? parent.height * 0.15 : 0
        }
    }

    // Battery overlay
    Overlays.BatteryOverlay {
        visible: ScreensaverConfig.showBattery
        anchors {
            bottom: parent.bottom
            bottomMargin: 40
            horizontalCenter: parent.horizontalCenter
        }
    }
}
