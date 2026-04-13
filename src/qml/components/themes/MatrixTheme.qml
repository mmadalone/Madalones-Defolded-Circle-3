// Copyright (c) 2026 madalone. Matrix rain charging screen theme.
// Copyright (c) 2026 madalone. Native screen-off animation — rain drains
//   off the grid. Uses a single C++ flag (MatrixRain.spawnSuppress) that
//   prevents new streams from respawning. Existing streams continue their
//   motion and exit the grid naturally. Cascade adds a temporary speed
//   boost so the drain happens faster; Drain uses normal speed. Both
//   end with an empty grid (black background), then the display blanks.
//   Implements the Tier 2 protocol documented in BaseTheme.qml.
// Implements BaseTheme interface — see BaseTheme.qml for contract
// Uses C++ QQuickItem (MatrixRainItem) for GPU-accelerated rendering.
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

    // ============================================================================
    // Screen-off animation protocol (Tier 2 native override)
    // See BaseTheme.qml for protocol documentation.
    //
    // Mechanism:
    //   1. startScreenOff() flips matrixRain.spawnSuppress = true. The
    //      simulation's inactive-stream respawn branch is now guarded:
    //      streams that drop off the grid are NOT replaced. For cascade,
    //      matrixRain.speed is also bumped to max so the existing streams
    //      plunge off quickly.
    //   2. The simulation keeps ticking normally. Active streams continue
    //      their natural motion and exit the grid one by one.
    //   3. After ~0.5-2 s (depending on preset + user's speed setting),
    //      all streams are inactive and the grid is empty. The theme's
    //      black background Rectangle (line ~135) is what shows.
    //   4. The core's Low_power transition then fires and the display
    //      physically blanks. Any "all-black hold" period before Low_power
    //      is handled by the ChargingScreen safety timer (already
    //      deployed), which closes the popup at leadMs+1500ms if the
    //      display hasn't blanked — bounded heat exposure.
    //
    // Cancel restores spawnSuppress=false and the previous speed, so
    // the simulation resumes spawning new streams and the rain regrows.
    // ============================================================================
    readonly property bool providesNativeScreenOff: true
    readonly property int  screenOffLeadMs: ScreensaverConfig.matrixShutoffDuration

    property bool _screenOffAnimating: false
    // Saved state for restore on cancel.
    property real _savedSpeed: 1.0

    function startScreenOff() {
        // Capture current speed so cancel can restore it.
        root._savedSpeed = matrixRain.speed;
        root._screenOffAnimating = true;

        // Suppress new spawns — existing streams will drain off the grid
        // naturally and not be replaced.
        matrixRain.spawnSuppress = true;

        // Single drain mode: cascade sweep. A horizontal kill-line rolls
        // top→bottom clearing cells via charGrid=-1 + cellAge=CELL_AGE_MAX.
        matrixRain.drainSpeedMultiplier = 1.0;
        matrixRain.drainMode = 1;
    }

    function cancelScreenOff() {
        root._screenOffAnimating = false;
        // Restore spawning, speed multiplier, drain mode, and original speed.
        matrixRain.spawnSuppress = false;
        matrixRain.drainSpeedMultiplier = 1.0;
        matrixRain.drainMode = 0;
        matrixRain.speed = root._savedSpeed;
    }

    function finalizeScreenOff() {
        // Display is about to physically blank. Leave spawnSuppress=true
        // so the grid stays empty and nothing new renders while the
        // display transitions to off. Reset the drain multiplier and mode
        // so they don't persist into the next wake. cancelScreenOff() on
        // the next wake clears the flag and restores speed.
        matrixRain.drainSpeedMultiplier = 1.0;
        matrixRain.drainMode = 0;
    }

    // Defensive: if displayOff goes back to false while the fall-off
    // flag is still set (race with ChargingScreen's cancel dispatch),
    // force a reset. Mirrors AnalogTheme's pattern.
    onDisplayOffChanged: {
        if (!root.displayOff && root._screenOffAnimating) {
            root.cancelScreenOff();
        }
    }

    // Belt-and-suspenders: hard-reset state on fresh instance so stale
    // flag values can never leave the theme stuck in a drained state.
    Component.onCompleted: {
        root._screenOffAnimating = false;
        matrixRain.spawnSuppress = false;
        matrixRain.drainSpeedMultiplier = 1.0;
        matrixRain.drainMode = 0;
    }

    // Forward interactive input from ChargingScreen DPAD to C++ MatrixRain.
    // Sync localGravity so the QML binding doesn't reassert the config value
    // onto the C++ property, which would undo the DPAD direction change.
    //
    // Guard: when fall-off is active, drop the input entirely and cancel.
    // ChargingScreen separately calls cancelScreenOffEffect() on every
    // sendDirection/fireTapEffects dispatch — this guard prevents the
    // tap from ALSO firing a direction change or tap effect on top of
    // the cancel. Matches Analog's behaviour.
    function interactiveInput(action) {
        if (root._screenOffAnimating) {
            root.cancelScreenOff();
            return;
        }
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
