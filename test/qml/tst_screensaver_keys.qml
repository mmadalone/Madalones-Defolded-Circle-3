// Copyright (c) 2026 madalone. v1.6.5 — the remote's keys on the screensaver. Only Matrix uses keys (while "DPAD
// interactive" is on); on Starfield, Minimal, Analog and TV Static every key closes it. A key that closes it closes it
// on its RELEASE, and only a key pressed while the screensaver was in front, so neither the release nor the repeats of
// a held key reach the page behind (up to v1.6.4 the close came on the press, and they did). A remembered direction is
// used on Matrix only, and both ways to steer may be on.
// Runs the REAL ChargingScreen.qml, loaded asynchronously like main.qml's chargingScreenLoader (switched off when the
// popup has closed), over a page that logs every key reaching it. Keys go through MockInputController.pressKey() /
// releaseKey(), which route them as the real InputController does (src/ui/inputController.cpp). The decisions are
// ScreensaverConfig.keyAction() / keyDirection() / directionToRestore() (src/ui/screensaverInputPolicy.h).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

import "qrc:/components" as Components

Item {
    id: root
    width: 480
    height: 850

    readonly property var allKeys: ["BACK", "HOME", "VOICE", "VOLUME_UP", "VOLUME_DOWN", "GREEN", "YELLOW", "RED",
        "BLUE", "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT", "DPAD_MIDDLE", "CHANNEL_UP", "CHANNEL_DOWN", "MUTE",
        "PREV", "PLAY", "NEXT", "POWER", "STOP", "RECORD", "MENU"]
    readonly property var nineKeys: ["DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT", "DPAD_MIDDLE", "VOLUME_UP",
        "VOLUME_DOWN", "CHANNEL_UP", "CHANNEL_DOWN"]
    readonly property var otherKeys: ["BACK", "HOME", "VOICE", "GREEN", "YELLOW", "RED", "BLUE", "MUTE", "PREV",
        "PLAY", "NEXT", "POWER", "STOP", "RECORD", "MENU"]

    // The page behind the screensaver (for instance a cover's detail page, which opens the cover on the release of
    // DPAD_UP: entities/cover/deviceclass/Blind.qml). It logs every press and release that reaches it.
    Item {
        id: page
        anchors.fill: parent
        property var log: []
        function record(entry) {
            var l = page.log.slice();
            l.push(entry);
            page.log = l;
        }
        function makeConfig() {
            var config = {};
            for (var i = 0; i < root.allKeys.length; ++i) {
                (function(k) {
                    config[k] = { "pressed": function() { page.record("p:" + k); },
                                  "released": function() { page.record("r:" + k); } };
                })(root.allKeys[i]);
            }
            return config;
        }
        Components.ButtonNavigation {
            id: pageNav
            defaultConfig: page.makeConfig()
        }
    }

    // like main.qml's chargingScreenLoader: asynchronous, open when loaded, switched off when the popup has closed
    Loader {
        id: ssLoader
        anchors.fill: parent
        active: false
        asynchronous: true
        source: "qrc:/components/ChargingScreen.qml"
        onStatusChanged: if (status === Loader.Ready) item.open()

        Connections {
            target: ssLoader.item
            ignoreUnknownSignals: true
            function onClosed() { ssLoader.active = false; }
        }
    }

    TestCase {
        id: tc
        name: "ScreensaverKeys"
        when: windowShown

        function openScreensaver(theme) {
            ScreensaverConfig.theme = theme;
            pageNav.takeControl();
            page.log = [];
            ssLoader.active = true;
            tryVerify(function() { return ssLoader.item !== null && ssLoader.item.opened; }, 5000, "screensaver open");
        }

        // a quick press: the release comes 120 ms after the press
        function tapKey(key) {
            ui.inputController.pressKey(key, false);
            wait(120);
            ui.inputController.releaseKey(key);
        }

        // the loaded TV Static theme (the item of ChargingScreen's theme Loader with u_flash)
        function themeItem() {
            if (!ssLoader.item || !ssLoader.item.contentItem) return null;
            var kids = ssLoader.item.contentItem.children;
            for (var i = 0; i < kids.length; ++i) {
                var k = kids[i];
                if (k.item !== undefined && k.item !== null && k.item.hasOwnProperty("u_flash")) return k.item;
            }
            return null;
        }

        function init() {
            ScreensaverConfig.resetDefaults();
            ScreensaverConfig.tvStaticChannelFlashAuto = false;
            ScreensaverConfig.lastDirection = "";
        }

        function cleanup() {
            if (ssLoader.item) ssLoader.item.close();
            else ssLoader.active = false;
            tryVerify(function() { return !ssLoader.active; }, 3000, "screensaver closed");
            ui.inputController.releaseControl(page);
            wait(20);
        }

        // on the four clock themes each of the 9 keys closes the screensaver, and neither its release nor anything else
        // reaches the page behind; no direction is stored
        function test_nineKeysCloseClockThemes_data() {
            var rows = [];
            var themes = ["starfield", "minimal", "analog", "tvstatic"];
            for (var t = 0; t < themes.length; ++t)
                for (var k = 0; k < root.nineKeys.length; ++k)
                    rows.push({ tag: themes[t] + " " + root.nineKeys[k], theme: themes[t], key: root.nineKeys[k] });
            return rows;
        }
        function test_nineKeysCloseClockThemes(d) {
            openScreensaver(d.theme);
            tapKey(d.key);
            tryVerify(function() { return !ssLoader.active; }, 1000, d.key + " closes the screensaver");
            wait(50);
            compare(JSON.stringify(page.log), "[]", "nothing reached the page behind");
            compare(ScreensaverConfig.lastDirection, "", "no direction stored");
        }

        // the 15 keys that always closed it: their release must not reach the page behind either
        function test_otherKeysDontReachThePage_data() {
            var rows = [];
            for (var k = 0; k < root.otherKeys.length; ++k)
                rows.push({ tag: root.otherKeys[k], key: root.otherKeys[k] });
            return rows;
        }
        function test_otherKeysDontReachThePage(d) {
            openScreensaver("matrix");
            tapKey(d.key);
            tryVerify(function() { return !ssLoader.active; }, 1000, d.key + " closes the screensaver");
            wait(50);
            compare(JSON.stringify(page.log), "[]", "nothing reached the page behind");
        }

        // a held key auto-repeats: it keeps the screensaver until it is let go, and no repeat reaches the page behind
        function test_heldKeyDoesntReachThePage_data() {
            return [ { tag: "starfield DPAD_UP", theme: "starfield", key: "DPAD_UP", dpad: true },
                     { tag: "matrix DPAD_DOWN, DPAD interactive off", theme: "matrix", key: "DPAD_DOWN", dpad: false },
                     { tag: "matrix VOLUME_UP, DPAD interactive off", theme: "matrix", key: "VOLUME_UP", dpad: false } ];
        }
        function test_heldKeyDoesntReachThePage(d) {
            ScreensaverConfig.dpadEnabled = d.dpad;
            openScreensaver(d.theme);
            ui.inputController.pressKey(d.key, false);
            for (var i = 0; i < 3; ++i) {
                wait(150);
                ui.inputController.pressKey(d.key, true);
            }
            verify(ssLoader.active && ssLoader.item.opened, "still open while the key is held");
            wait(150);
            ui.inputController.releaseKey(d.key);
            tryVerify(function() { return !ssLoader.active; }, 1000, "closed after the key is let go");
            wait(50);
            compare(JSON.stringify(page.log), "[]", "no press, repeat or release reached the page behind");
        }

        // a release whose press the screensaver never saw (e.g. the press that woke the remote) doesn't close it
        function test_loneReleaseKeepsItOpen_data() {
            return [ { tag: "DPAD_UP", key: "DPAD_UP" }, { tag: "BACK", key: "BACK" } ];
        }
        function test_loneReleaseKeepsItOpen(d) {
            openScreensaver("starfield");
            ui.inputController.releaseKey(d.key);
            wait(300);
            verify(ssLoader.active && ssLoader.item.opened, "still open");
        }

        // Matrix with DPAD interactive on (the default): the 8 direction keys steer and store the direction
        function test_matrixKeysSteer_data() {
            return [ { tag: "DPAD_UP", key: "DPAD_UP", dir: "up" }, { tag: "DPAD_LEFT", key: "DPAD_LEFT", dir: "left" },
                     { tag: "VOLUME_UP", key: "VOLUME_UP", dir: "up-left" },
                     { tag: "CHANNEL_DOWN", key: "CHANNEL_DOWN", dir: "down-right" } ];
        }
        function test_matrixKeysSteer(d) {
            openScreensaver("matrix");
            var rain = ssLoader.item.matrixRainRef;
            verify(rain !== null, "the MatrixRain stub is loaded");
            tapKey(d.key);
            wait(100);
            verify(rain.calls.indexOf("input:" + d.dir) >= 0, "steers: " + JSON.stringify(rain.calls));
            compare(ScreensaverConfig.lastDirection, d.dir, "and stores the direction");
            verify(ssLoader.active && ssLoader.item.opened, "and stays open");
            compare(JSON.stringify(page.log), "[]", "nothing reached the page behind");
        }

        // Matrix: OK is the rain's burst / restore / slow-down key (enterPressed on the press, enterReleased on the
        // release)
        function test_matrixOk() {
            openScreensaver("matrix");
            var rain = ssLoader.item.matrixRainRef;
            tapKey("DPAD_MIDDLE");
            wait(100);
            verify(rain.calls.indexOf("enterPressed") >= 0, JSON.stringify(rain.calls));
            verify(rain.calls.indexOf("enterReleased") >= 0, JSON.stringify(rain.calls));
            verify(ssLoader.active && ssLoader.item.opened, "stays open");
        }

        // Matrix with DPAD interactive off: the 9 keys close it, and nothing reaches the page behind
        function test_matrixKeysCloseWithDpadOff_data() {
            var rows = [];
            for (var k = 0; k < root.nineKeys.length; ++k)
                rows.push({ tag: root.nineKeys[k], key: root.nineKeys[k] });
            return rows;
        }
        function test_matrixKeysCloseWithDpadOff(d) {
            ScreensaverConfig.dpadEnabled = false;
            openScreensaver("matrix");
            tapKey(d.key);
            tryVerify(function() { return !ssLoader.active; }, 1000, d.key + " closes the screensaver");
            wait(50);
            compare(JSON.stringify(page.log), "[]", "nothing reached the page behind");
            compare(ScreensaverConfig.lastDirection, "", "no direction stored");
        }

        // a theme name the Loader doesn't know loads Matrix, and the keys steer it like Matrix
        function test_unknownThemeNameSteersLikeMatrix() {
            openScreensaver("aurora");
            var rain = ssLoader.item.matrixRainRef;
            verify(rain !== null, "Matrix loaded for an unknown theme name");
            tapKey("DPAD_UP");
            wait(100);
            verify(rain.calls.indexOf("input:up") >= 0, JSON.stringify(rain.calls));
            verify(ssLoader.active && ssLoader.item.opened, "stays open");
        }

        // a direction remembered on Matrix is not handed to TV Static, which flashed once when it opened up to v1.6.4.
        // A 5 s flash keeps u_flash above 0 long enough to see one that played while the theme loaded.
        function test_tvStaticOpensWithoutFlash() {
            ScreensaverConfig.tvStaticFlashDuration = 5000;
            ScreensaverConfig.lastDirection = "up";
            openScreensaver("tvstatic");
            var t = themeItem();
            verify(t !== null, "the TV Static theme is loaded");
            compare(t.u_flash, 0, "no flash when TV Static opens");
        }

        // both ways to steer may be on: nothing switches one off (up to v1.6.4 the open screensaver did), and the keys
        // still steer
        function test_bothSteeringSwitchesStayOn() {
            openScreensaver("matrix");
            ScreensaverConfig.tapDirection = true;   // DPAD interactive is on by default
            wait(50);
            compare(ScreensaverConfig.dpadEnabled, true, "DPAD interactive stays on");
            compare(ScreensaverConfig.tapDirection, true, "Touch directions stays on");
            ScreensaverConfig.dpadEnabled = false;
            ScreensaverConfig.dpadEnabled = true;
            wait(50);
            compare(ScreensaverConfig.tapDirection, true, "turning DPAD interactive on leaves Touch directions on");
            var rain = ssLoader.item.matrixRainRef;
            tapKey("DPAD_LEFT");
            wait(100);
            verify(rain.calls.indexOf("input:left") >= 0, "the keys steer: " + JSON.stringify(rain.calls));
        }
    }
}
