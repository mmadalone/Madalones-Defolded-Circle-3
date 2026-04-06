// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2024 madalone. Configurable screensaver with theme support.
// Config propagation handled in C++ via ScreensaverConfig singleton.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Config 1.0
import ScreensaverConfig 1.0

import "qrc:/components" as Components

Popup {
    id: chargingScreenRoot
    width: ui.width; height: ui.height
    opacity: 0
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0

    readonly property int doubleTapMs: 300   // max interval between taps for double-tap detection (normal mode)
    readonly property int zoneTapMs: 400     // generous interval for multi-tap detection in zone mode

    property bool isClosing: false
    property bool displayOff: false

    // Forward runtime state changes to the loaded theme
    onIsClosingChanged: if (themeLoader.item && themeLoader.item.hasOwnProperty("isClosing")) themeLoader.item.isClosing = isClosing;
    onDisplayOffChanged: if (themeLoader.item && themeLoader.item.hasOwnProperty("displayOff")) themeLoader.item.displayOff = displayOff;

    // Persist DPAD direction between sessions (gated by dpadPersist setting)
    function saveDirection(dir) { if (Config.chargingMatrixDpadPersist) Config.chargingMatrixLastDirection = dir; }
    function restoreDirection() {
        if (!Config.chargingMatrixDpadPersist) return;
        if (!ScreensaverConfig.dpadEnabled) return;
        var dir = ScreensaverConfig.lastDirection;
        if (dir !== "" && themeLoader.item && themeLoader.item.interactiveInput)
            themeLoader.item.interactiveInput(dir);
    }

    // --- Touch-zone direction mapping ---
    // Map (x, y) pixel coordinates to one of 9 zones (3x3 grid)
    // Returns direction string or "enter" for center zone
    function zoneFromTap(x, y) {
        var col = Math.floor(x / (width / 3));
        var row = Math.floor(y / (height / 3));
        col = Math.min(Math.max(col, 0), 2);
        row = Math.min(Math.max(row, 0), 2);
        var map = [
            ["up-left",    "up",    "up-right"],
            ["left",       "enter", "right"],
            ["down-left",  "down",  "down-right"]
        ];
        return map[row][col];
    }

    // Kill theme rendering immediately when close starts
    onOpenedChanged: {
        if (!opened) {
            isClosing = true;
            if (matrixRainRef) matrixRainRef.resetEnterState();
            themeLoader.active = false;
        }
    }

    onOpened: {
        isClosing = false;
        themeLoader.active = true;
        if (themeLoader.item) {
            // Only take focus if theme actually loaded — prevents invisible Popup
            // from consuming keys when the theme fails to render.
            buttonNavigation.takeControl();
            if (themeLoader.item.hasOwnProperty("isClosing")) themeLoader.item.isClosing = false;
            if (themeLoader.item.hasOwnProperty("displayOff")) themeLoader.item.displayOff = chargingScreenRoot.displayOff;
        }
    }

    onClosed: {
        isClosing = false;
        buttonNavigation.releaseControl();
    }

    // --- Helper: send direction input to theme ---
    function sendDirection(dir) {
        if (themeLoader.item && themeLoader.item.interactiveInput)
            themeLoader.item.interactiveInput(dir);
        chargingScreenRoot.saveDirection(dir);
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            // All physical buttons dismiss the screensaver unconditionally.
            // DPAD buttons: interactive when dpadEnabled, otherwise dismiss.
            "BACK": { "pressed": function() { chargingScreenRoot.close(); } },
            "HOME": { "pressed": function() { chargingScreenRoot.close(); } },
            "DPAD_UP":     { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("up");
                else chargingScreenRoot.close();
            }},
            "DPAD_DOWN":   { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("down");
                else chargingScreenRoot.close();
            }},
            "DPAD_LEFT":   { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("left");
                else chargingScreenRoot.close();
            }},
            "DPAD_RIGHT":  { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("right");
                else chargingScreenRoot.close();
            }},
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (ScreensaverConfig.dpadEnabled) {
                        if (matrixRainRef) matrixRainRef.enterPressed();
                    } else {
                        chargingScreenRoot.close();
                    }
                },
                "released": function() {
                    if (!matrixRainRef) return;
                    matrixRainRef.enterReleased();
                }
            },
            "VOICE":  { "pressed": function() { chargingScreenRoot.close(); } },
            "VOLUME_UP": { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("up-left");
                else chargingScreenRoot.close();
            }},
            "VOLUME_DOWN": { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("down-left");
                else chargingScreenRoot.close();
            }},
            "GREEN":  { "pressed": function() { chargingScreenRoot.close(); } },
            "YELLOW": { "pressed": function() { chargingScreenRoot.close(); } },
            "RED":    { "pressed": function() { chargingScreenRoot.close(); } },
            "BLUE":   { "pressed": function() { chargingScreenRoot.close(); } },
            "CHANNEL_UP": { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("up-right");
                else chargingScreenRoot.close();
            }},
            "CHANNEL_DOWN": { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("down-right");
                else chargingScreenRoot.close();
            }},
            "MUTE":   { "pressed": function() { chargingScreenRoot.close(); } },
            "PREV":   { "pressed": function() { chargingScreenRoot.close(); } },
            "PLAY":   { "pressed": function() { chargingScreenRoot.close(); } },
            "NEXT":   { "pressed": function() { chargingScreenRoot.close(); } },
            "POWER":  { "pressed": function() { chargingScreenRoot.close(); } },
            "STOP":   { "pressed": function() { chargingScreenRoot.close(); } },
            "RECORD": { "pressed": function() { chargingScreenRoot.close(); } },
            "MENU":   { "pressed": function() { chargingScreenRoot.close(); } }
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 50 }
    }

    background: Rectangle { color: "black" }

    // --- Gesture handling ---
    // Three gesture types distinguished by movement + duration:
    //   Tap: press+release with < swipeThreshold movement
    //   Swipe: press+drag with >= swipeThreshold vertical movement → speed adjust
    //   Hold: press without movement for holdSlowMs → slow, holdPauseMs → pause
    //
    // Tap behavior:
    //   Normal mode: single tap = effects, double tap = close
    //   Zone mode: edge tap = direction + effects, center multi-tap = glitch/restore/close

    readonly property int swipeThreshold: 30  // px movement to classify as swipe vs tap
    readonly property int holdSlowMs: 500     // hold duration to start slowing
    readonly property int holdPauseMs: 1500   // hold duration to pause completely

    property real lastTapX: 0
    property real lastTapY: 0
    property int centerTapCount: 0
    property real pressStartX: 0
    property real pressStartY: 0
    property bool isDragging: false
    property bool isHolding: false
    property int holdStage: 0  // 0=none, 1=slow, 2=paused

    MouseArea {
        anchors.fill: parent

        onPressed: {
            chargingScreenRoot.pressStartX = mouse.x;
            chargingScreenRoot.pressStartY = mouse.y;
            chargingScreenRoot.isDragging = false;
            chargingScreenRoot.isHolding = false;
            chargingScreenRoot.holdStage = 0;
            holdSlowTimer.restart();
        }

        onPositionChanged: {
            var dy = Math.abs(mouse.y - chargingScreenRoot.pressStartY);
            var dx = Math.abs(mouse.x - chargingScreenRoot.pressStartX);
            if (dy > chargingScreenRoot.swipeThreshold || dx > chargingScreenRoot.swipeThreshold) {
                chargingScreenRoot.isDragging = true;
                holdSlowTimer.stop();
                holdPauseTimer.stop();
                // If we were holding, release the hold effect
                if (chargingScreenRoot.holdStage > 0) {
                    chargingScreenRoot.holdStage = 0;
                    if (matrixRainRef) matrixRainRef.interactiveInput("slow:release");
                }
            }
        }

        onReleased: {
            holdSlowTimer.stop();
            holdPauseTimer.stop();

            // Release hold effect if active
            if (chargingScreenRoot.holdStage > 0) {
                chargingScreenRoot.holdStage = 0;
                chargingScreenRoot.isHolding = false;
                if (matrixRainRef) matrixRainRef.interactiveInput("slow:release");
                return;
            }

            if (chargingScreenRoot.isDragging) {
                // --- Swipe gesture: adjust speed ---
                var deltaY = mouse.y - chargingScreenRoot.pressStartY;
                // Swipe up (negative deltaY) = faster, swipe down = slower
                // Map swipe distance to speed change: 100px ≈ 10 speed units
                var speedDelta = Math.round(-deltaY / 10);
                if (speedDelta !== 0) {
                    var newSpeed = Math.min(100, Math.max(10, Config.chargingMatrixSpeed + speedDelta));
                    Config.chargingMatrixSpeed = newSpeed;
                }
                chargingScreenRoot.isDragging = false;
                return;
            }

            // --- Tap gesture ---
            chargingScreenRoot.isDragging = false;
            chargingScreenRoot.isHolding = false;
            chargingScreenRoot.lastTapX = mouse.x;
            chargingScreenRoot.lastTapY = mouse.y;

            if (ScreensaverConfig.tapDirection) {
                // --- Zone direction mode ---
                var zone = chargingScreenRoot.zoneFromTap(mouse.x, mouse.y);

                if (zone !== "enter") {
                    // Edge zone — send direction + fire effects, reset center counter
                    chargingScreenRoot.centerTapCount = 0;
                    centerTapTimer.stop();
                    chargingScreenRoot.sendDirection(zone);
                    chargingScreenRoot.fireTapEffects();
                } else {
                    // Center zone — multi-tap state machine
                    chargingScreenRoot.centerTapCount++;
                    centerTapTimer.restart();

                    if (chargingScreenRoot.centerTapCount <= 2) {
                        // Tap 1-2: enter/glitch + effects
                        if (matrixRainRef) {
                            matrixRainRef.enterPressed();
                            matrixRainRef.enterReleased();
                        }
                        chargingScreenRoot.fireTapEffects();
                    } else if (chargingScreenRoot.centerTapCount === 3) {
                        // Tap 3: restore direction
                        if (themeLoader.item && themeLoader.item.interactiveInput)
                            themeLoader.item.interactiveInput("restore");
                        Config.chargingMatrixLastDirection = "";
                    } else if (chargingScreenRoot.centerTapCount >= 4) {
                        // Tap 4: close screensaver
                        centerTapTimer.stop();
                        chargingScreenRoot.centerTapCount = 0;
                        chargingScreenRoot.close();
                    }
                }
            } else {
                // --- Normal mode: double-tap to close ---
                if (doubleTapTimer.running) {
                    doubleTapTimer.stop();
                    if (ScreensaverConfig.tapToClose) chargingScreenRoot.close();
                } else {
                    doubleTapTimer.restart();
                }
            }
        }
    }

    // Hold stage 1 — slow to 25% after 500ms
    Timer {
        id: holdSlowTimer; interval: holdSlowMs
        onTriggered: {
            chargingScreenRoot.isHolding = true;
            chargingScreenRoot.holdStage = 1;
            if (matrixRainRef) matrixRainRef.interactiveInput("slow:hold");
            holdPauseTimer.restart();
        }
    }
    // Hold stage 2 — pause after 1500ms total
    Timer {
        id: holdPauseTimer; interval: holdPauseMs - holdSlowMs
        onTriggered: {
            chargingScreenRoot.holdStage = 2;
            // Pause by setting speed to near-zero temporarily (timer stops at TICK_MAX_MS)
            if (matrixRainRef) matrixRainRef.setRunning(false);
        }
    }

    // Connect C++ enter state machine signals to interactiveInput + direction persistence
    property var matrixRainRef: themeLoader.item && themeLoader.item.hasOwnProperty("matrixRainItem")
                                ? themeLoader.item.matrixRainItem : null
    Connections {
        target: matrixRainRef
        ignoreUnknownSignals: true
        function onEnterAction(action) {
            if (action === "restore") Config.chargingMatrixLastDirection = "";
            if (themeLoader.item && themeLoader.item.interactiveInput)
                themeLoader.item.interactiveInput(action);
        }
    }

    // Build tap effect flags string from current config
    function tapEffectFlags() {
        return (ScreensaverConfig.tapBurst ? "1" : "0") + "," +
               (ScreensaverConfig.tapFlash ? "1" : "0") + "," +
               (ScreensaverConfig.tapScramble ? "1" : "0") + "," +
               (ScreensaverConfig.tapSpawn ? "1" : "0") + "," +
               (ScreensaverConfig.tapMessage ? "1" : "0") + "," +
               (ScreensaverConfig.tapSquareBurst ? "1" : "0") + "," +
               (ScreensaverConfig.tapRipple ? "1" : "0") + "," +
               (ScreensaverConfig.tapWipe ? "1" : "0") +
               (ScreensaverConfig.tapRandomize ? ",R" + ScreensaverConfig.tapRandomizeChance : "");
    }

    // Fire tap effects at last recorded tap position
    function fireTapEffects() {
        if (themeLoader.item && themeLoader.item.interactiveInput)
            themeLoader.item.interactiveInput("tap:" + lastTapX + "," + lastTapY + "," + tapEffectFlags());
    }

    // Normal mode timer — single tap confirmed after 300ms
    Timer {
        id: doubleTapTimer; interval: doubleTapMs
        onTriggered: chargingScreenRoot.fireTapEffects();
    }

    // Zone mode timer — reset center tap counter after 400ms of inactivity
    Timer {
        id: centerTapTimer; interval: zoneTapMs
        onTriggered: chargingScreenRoot.centerTapCount = 0;
    }

    // --- Mutual exclusion: tapDirection ON → dpadEnabled OFF ---
    Connections {
        target: Config
        function onChargingMatrixTapDirectionChanged() {
            if (Config.chargingMatrixTapDirection && Config.chargingMatrixDpadEnabled)
                Config.chargingMatrixDpadEnabled = false;
        }
        function onChargingMatrixDpadEnabledChanged() {
            if (Config.chargingMatrixDpadEnabled && Config.chargingMatrixTapDirection)
                Config.chargingMatrixTapDirection = false;
        }
    }

    Loader {
        id: themeLoader
        anchors.fill: parent
        source: {
            switch (ScreensaverConfig.theme) {
                case "matrix": return "qrc:/components/themes/MatrixTheme.qml";
                case "starfield": return "qrc:/components/themes/StarfieldTheme.qml";
                case "minimal": return "qrc:/components/themes/MinimalTheme.qml";
                default: return "qrc:/components/themes/MatrixTheme.qml";
            }
        }

        onLoaded: {
            if (!item) return;
            // Runtime state — not config, must be set explicitly
            if (item.hasOwnProperty("isClosing")) item.isClosing = chargingScreenRoot.isClosing;
            if (item.hasOwnProperty("displayOff")) item.displayOff = chargingScreenRoot.displayOff;
            // Restore persisted DPAD direction from previous session
            chargingScreenRoot.restoreDirection();
        }
    }

    // Theme switch when config changes
    Connections {
        target: ScreensaverConfig
        function onThemeChanged() {
            themeLoader.source = Qt.binding(function() {
                switch (ScreensaverConfig.theme) {
                    case "matrix": return "qrc:/components/themes/MatrixTheme.qml";
                    case "starfield": return "qrc:/components/themes/StarfieldTheme.qml";
                    case "minimal": return "qrc:/components/themes/MinimalTheme.qml";
                    default: return "qrc:/components/themes/MatrixTheme.qml";
                }
            });
        }
    }
}
