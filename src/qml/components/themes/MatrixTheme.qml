// Copyright (c) 2024 madalone. Matrix rain charging screen theme.
// Uses C++ QQuickItem (MatrixRainItem) for GPU-accelerated rendering.
// Config binding handled in C++ via ScreensaverConfig → MatrixRainItem::bindToScreensaverConfig().
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
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

    // DPAD gravity override — imperative assignment only (no declarative binding).
    // Prevents the QML binding fight from Session 10 Bug 5: DPAD sets localGravity
    // directly, and Config changes re-sync via the Connections handler below.
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
        running: root.visible && !root.isClosing && !root.displayOff
    }

    // Clock overlay
    Overlays.ClockOverlay {
        visible: ScreensaverConfig.showClock
        anchors {
            top: parent.top
            topMargin: parent.height * 0.15
            horizontalCenter: parent.horizontalCenter
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
