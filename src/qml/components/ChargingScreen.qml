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

    readonly property int doubleTapMs: 300   // max interval between taps for double-tap detection

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

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            // Escape buttons — dismiss screensaver (gated by tapToClose)
            "BACK": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "HOME": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            // Interactive DPAD — controls rain direction / triggers chaos (gated by dpadEnabled)
            "DPAD_UP":     { "pressed": function() { if (ScreensaverConfig.dpadEnabled && themeLoader.item && themeLoader.item.interactiveInput) { themeLoader.item.interactiveInput("up"); chargingScreenRoot.saveDirection("up"); } } },
            "DPAD_DOWN":   { "pressed": function() { if (ScreensaverConfig.dpadEnabled && themeLoader.item && themeLoader.item.interactiveInput) { themeLoader.item.interactiveInput("down"); chargingScreenRoot.saveDirection("down"); } } },
            "DPAD_LEFT":   { "pressed": function() { if (ScreensaverConfig.dpadEnabled && themeLoader.item && themeLoader.item.interactiveInput) { themeLoader.item.interactiveInput("left"); chargingScreenRoot.saveDirection("left"); } } },
            "DPAD_RIGHT":  { "pressed": function() { if (ScreensaverConfig.dpadEnabled && themeLoader.item && themeLoader.item.interactiveInput) { themeLoader.item.interactiveInput("right"); chargingScreenRoot.saveDirection("right"); } } },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (!ScreensaverConfig.dpadEnabled) return;
                    if (!matrixRainRef) return;
                    matrixRainRef.enterPressed();
                },
                "released": function() {
                    if (!matrixRainRef) return;
                    matrixRainRef.enterReleased();
                }
            },
            // Other buttons — dismiss screensaver (gated by tapToClose)
            "VOICE": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "VOLUME_UP": { "pressed": function() { if (ScreensaverConfig.dpadEnabled && themeLoader.item && themeLoader.item.interactiveInput) { themeLoader.item.interactiveInput("up-left"); chargingScreenRoot.saveDirection("up-left"); } } },
            "VOLUME_DOWN": { "pressed": function() { if (ScreensaverConfig.dpadEnabled && themeLoader.item && themeLoader.item.interactiveInput) { themeLoader.item.interactiveInput("down-left"); chargingScreenRoot.saveDirection("down-left"); } } },
            "GREEN": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "YELLOW": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "RED": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "BLUE": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "CHANNEL_UP": { "pressed": function() { if (ScreensaverConfig.dpadEnabled && themeLoader.item && themeLoader.item.interactiveInput) { themeLoader.item.interactiveInput("up-right"); chargingScreenRoot.saveDirection("up-right"); } } },
            "CHANNEL_DOWN": { "pressed": function() { if (ScreensaverConfig.dpadEnabled && themeLoader.item && themeLoader.item.interactiveInput) { themeLoader.item.interactiveInput("down-right"); chargingScreenRoot.saveDirection("down-right"); } } },
            "MUTE": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "PREV": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "PLAY": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "NEXT": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "POWER": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "STOP": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "RECORD": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } },
            "MENU": { "pressed": function() { if (ScreensaverConfig.tapToClose) chargingScreenRoot.close(); } }
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 50 }
    }

    background: Rectangle { color: "black" }

    // Double-tap to close — single tap triggers corruption burst at touch point
    property real lastTapX: 0
    property real lastTapY: 0
    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (doubleTapTimer.running) {
                doubleTapTimer.stop();
                if (ScreensaverConfig.tapToClose) chargingScreenRoot.close();
            } else {
                chargingScreenRoot.lastTapX = mouse.x;
                chargingScreenRoot.lastTapY = mouse.y;
                doubleTapTimer.restart();
            }
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

    Timer {
        id: doubleTapTimer; interval: doubleTapMs
        onTriggered: {
            // Single tap confirmed — fire corruption burst at tap point
            // Format: "tap:x,y,burst,flash,scramble,spawn,message[,R{chance}]"
            if (themeLoader.item && themeLoader.item.interactiveInput) {
                var flags = (ScreensaverConfig.tapBurst ? "1" : "0") + "," +
                            (ScreensaverConfig.tapFlash ? "1" : "0") + "," +
                            (ScreensaverConfig.tapScramble ? "1" : "0") + "," +
                            (ScreensaverConfig.tapSpawn ? "1" : "0") + "," +
                            (ScreensaverConfig.tapMessage ? "1" : "0") +
                            (ScreensaverConfig.tapRandomize ? ",R" + ScreensaverConfig.tapRandomizeChance : "");
                themeLoader.item.interactiveInput("tap:" + chargingScreenRoot.lastTapX + "," + chargingScreenRoot.lastTapY + "," + flags);
            }
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
