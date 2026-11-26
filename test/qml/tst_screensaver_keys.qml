// Copyright (c) 2026 madalone. v1.6.5 — the remote's keys on the screensaver. Only Matrix uses keys (while "DPAD
// interactive" is on); on Starfield, Minimal, Analog and TV Static every key closes it. A key that closes it closes it
// on its RELEASE, and only a key pressed while the screensaver was in front, so neither the release nor the repeats of
// a held key reach the page behind (up to v1.6.4 the close came on the press, and they did). A remembered direction is
// used on Matrix only, and both ways to steer may be on.
// When something else closes the screensaver under a key that is still down (a wake, a lift, a tap, another key), the
// page behind acts on neither that key's repeats nor its release: ChargingScreen's onClosed hands the keys it still has
// down to InputController.dropPendingKeys(). A handler that listens to every owner (main.qml's VOICE handler) still
// gets the release of a press it saw. A key pressed once the screensaver is gone acts on the page.
// Runs the REAL ChargingScreen.qml, loaded asynchronously like main.qml's chargingScreenLoader (switched off when the
// popup has closed), over a page that logs every key reaching it. The keys go through the REAL InputController
// (src/ui/inputController.cpp is compiled into the QML test binary): ui.inputController.pressKey() / releaseKey() send
// the key events the keypad makes (test/qml/tst_qml_main.cpp, TestInputController). The decisions are
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

        // An activity screen has a VOICE handler of its own (entities/activity/deviceclass/Activity.qml): a long press
        // starts the activity's voice assistant, the release stops it. Like every handler of a page it is scoped to
        // the page.
        property var activityLog: []
        function recordActivity(entry) {
            var l = page.activityLog.slice();
            l.push(entry);
            page.activityLog = l;
        }
        Components.ButtonNavigation {
            id: activityVoiceNav
            defaultConfig: {
                "VOICE": { "long_press": function() { page.recordActivity("start"); },
                           "released": function() { page.recordActivity("stop"); } }
            }
        }
    }

    // A handler that listens to every owner (overrideActive). VOICE as in main.qml: a long press starts the voice
    // assistant, the release stops it. VOLUME_UP as on Page.qml's activity tile or a light's brightness page: a
    // `pressed` action, which such a handler runs for every press it is handed, a repeat included. It sees the press
    // of a key made on the screensaver.
    Components.ButtonNavigation {
        id: voiceNav
        overrideActive: true
        property var log: []
        function record(entry) {
            var l = voiceNav.log.slice();
            l.push(entry);
            voiceNav.log = l;
        }
        defaultConfig: {
            "VOICE": { "long_press": function() { voiceNav.record("start"); },
                       "released": function() { voiceNav.record("stop"); } },
            "VOLUME_UP": { "pressed": function() { voiceNav.record("vol"); } }
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
            page.activityLog = [];
            ssLoader.active = true;
            tryVerify(function() { return ssLoader.item !== null && ssLoader.item.opened; }, 5000, "screensaver open");
        }

        // a quick press: the release comes 120 ms after the press
        function tapKey(key) {
            ui.inputController.pressKey(key, false);
            wait(120);
            ui.inputController.releaseKey(key);
        }

        // The same on the screensaver, with a look in between: it must still be there while the key is down. A key
        // closes it when it is let go; since the close hands over the keys still down, a key that closed it on its
        // press would no longer show as a leak to the page behind.
        function tapKeyOnScreensaver(key) {
            ui.inputController.pressKey(key, false);
            wait(120);
            verify(stillOpen(), key + ": still open while the key is down");
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
            tapKeyOnScreensaver(d.key);
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
            tapKeyOnScreensaver(d.key);
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
            tapKeyOnScreensaver(d.key);
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

        // --- a key that is still down when something else closes the screensaver ---------------------------------
        // "Something else" is Close on wake at the wake that key caused, the quick undock check, the core's charger
        // report, a tap or another key: each ends in the popup's close(). main.qml then destroys the popup, and
        // InputController would send the rest of the hold to the page behind (the remembered owner is gone). The
        // popup's onClosed hands the keys it still has down to InputController.dropPendingKeys() first.

        function stillOpen() { return ssLoader.active && ssLoader.item !== null && ssLoader.item.opened; }

        function waitGone() {
            tryVerify(function() { return !ssLoader.active && ssLoader.item === null; }, 2000, "screensaver gone");
            wait(30);   // the Loader deletes the popup on the next turn of the event loop
        }

        // the rest of a hold: three auto-repeat presses 150 ms apart, then the release
        function restOfTheHold(key) {
            for (var i = 0; i < 3; ++i) {
                wait(150);
                ui.inputController.pressKey(key, true);
            }
            wait(150);
            ui.inputController.releaseKey(key);
            wait(50);
        }

        // a closing key, a Matrix steering key and Matrix's OK, each held while the screensaver is closed from outside
        function test_heldKeyWhenSomethingElseCloses_data() {
            return [ { tag: "starfield DPAD_UP", theme: "starfield", key: "DPAD_UP" },
                     { tag: "minimal BACK", theme: "minimal", key: "BACK" },
                     { tag: "tvstatic VOLUME_UP", theme: "tvstatic", key: "VOLUME_UP" },
                     { tag: "matrix DPAD_UP (steers)", theme: "matrix", key: "DPAD_UP" },
                     { tag: "matrix DPAD_MIDDLE (OK)", theme: "matrix", key: "DPAD_MIDDLE" },
                     { tag: "matrix BACK", theme: "matrix", key: "BACK" } ];
        }
        function test_heldKeyWhenSomethingElseCloses(d) {
            openScreensaver(d.theme);
            ui.inputController.pressKey(d.key, false);
            wait(100);
            verify(stillOpen(), "still open while the key is held");
            ssLoader.item.close();
            waitGone();
            restOfTheHold(d.key);
            compare(JSON.stringify(page.log), "[]", "neither the repeats nor the release reached the page behind");
            compare(ui.inputController.activeItem, page, "the page has the input again");
        }

        // Matrix: a steering key is held, and another key closes the screensaver
        function test_heldSteeringKeyWhenAnotherKeyCloses() {
            openScreensaver("matrix");
            var rain = ssLoader.item.matrixRainRef;
            ui.inputController.pressKey("DPAD_UP", false);
            wait(100);
            verify(rain.calls.indexOf("input:up") >= 0, "DPAD_UP steers: " + JSON.stringify(rain.calls));
            tapKey("BACK");
            waitGone();
            restOfTheHold("DPAD_UP");
            compare(JSON.stringify(page.log), "[]", "nothing of the steering key reached the page behind");
        }

        // "Double-tap to close" off: one tap closes it, here under a held key
        function test_heldKeyWhenATapCloses() {
            ScreensaverConfig.tapToClose = false;
            openScreensaver("starfield");
            ui.inputController.pressKey("DPAD_UP", false);
            wait(100);
            verify(stillOpen(), "still open while the key is held");
            mouseClick(ssLoader.item.contentItem, 240, 425);
            waitGone();
            restOfTheHold("DPAD_UP");
            compare(JSON.stringify(page.log), "[]", "nothing of the held key reached the page behind");
        }

        // a key pressed while the screensaver is closing (the 50 ms exit): the popup still has the input and gets the
        // press, so the rest of that hold is dropped too
        function test_keyPressedWhileClosing() {
            openScreensaver("starfield");
            ssLoader.item.close();
            verify(ssLoader.item !== null, "still closing");
            ui.inputController.pressKey("DPAD_UP", false);
            waitGone();
            restOfTheHold("DPAD_UP");
            compare(JSON.stringify(page.log), "[]", "nothing of that key reached the page behind");
        }

        // the screensaver comes back while the key is still down (the docked re-arm, a wake on the charger): the new
        // one gets nothing of that hold either, so its release doesn't close it
        function test_keyHeldAcrossAReopen() {
            openScreensaver("starfield");
            ui.inputController.pressKey("DPAD_UP", false);
            wait(100);
            verify(stillOpen(), "still open while the key is held");
            ssLoader.item.close();
            waitGone();
            ssLoader.active = true;
            tryVerify(function() { return ssLoader.item !== null && ssLoader.item.opened; }, 5000, "open again");
            restOfTheHold("DPAD_UP");
            wait(300);
            verify(ssLoader.active && ssLoader.item !== null && ssLoader.item.opened,
                   "the second screensaver stays open");
            compare(JSON.stringify(page.log), "[]", "and nothing reached the page behind");
        }

        // A handler that listens to every owner saw the press, so it must get the release too: without it the voice
        // assistant that the hold started would never stop, and the handler would ignore the next VOICE press. The
        // page behind still gets nothing.
        function test_heldVoiceKeyStillGetsItsRelease() {
            openScreensaver("starfield");
            voiceNav.log = [];
            ui.inputController.pressKey("VOICE", false);
            wait(100);
            verify(stillOpen(), "still open while the key is held");
            ssLoader.item.close();
            waitGone();
            tryVerify(function() { return voiceNav.log.length === 1; }, 2000, "the hold started the voice assistant");
            restOfTheHold("VOICE");
            compare(JSON.stringify(voiceNav.log), JSON.stringify(["start", "stop"]), "and the release stopped it");
            compare(JSON.stringify(page.log), "[]", "nothing of the key reached the page behind");

            ui.inputController.pressKey("VOICE", false);
            tryVerify(function() { return voiceNav.log.length === 3; }, 2000, "the next hold starts it again");
            ui.inputController.releaseKey("VOICE");
            compare(JSON.stringify(voiceNav.log), JSON.stringify(["start", "stop", "start", "stop"]));
        }

        // A limit, as it is today: a page with a VOICE handler of its own (an activity screen) never sees a VOICE
        // press made on the screensaver, and after the close it gets nothing of that hold either. So the hold starts
        // no assistant there; let go and press VOICE again. The handler that listens to every owner still gets its
        // press and its release. (Up to v1.6.4 the page got the repeats, and its assistant started 0.8 s after the
        // first of them.)
        function test_heldVoiceKeyStartsNothingOnAPageWithItsOwnVoiceHandler() {
            openScreensaver("starfield");
            voiceNav.log = [];
            page.activityLog = [];
            ui.inputController.pressKey("VOICE", false);
            wait(100);
            verify(stillOpen(), "still open while the key is held");
            ssLoader.item.close();
            waitGone();
            // held for a good second more: time enough for a long press of the page to fire, had it got a repeat
            for (var i = 0; i < 8; ++i) {
                wait(150);
                ui.inputController.pressKey("VOICE", true);
            }
            wait(150);
            ui.inputController.releaseKey("VOICE");
            wait(50);
            compare(JSON.stringify(page.activityLog), "[]", "the page's own VOICE handler got nothing of the hold");
            compare(JSON.stringify(page.log), "[]", "nor did its other handlers");
            compare(JSON.stringify(voiceNav.log), JSON.stringify(["start", "stop"]),
                    "the handler that listens to every owner started and stopped");
        }

        // A handler that listens to every owner acts on the press made on the screensaver (upstream's rule) and on
        // nothing else of that hold: it runs its `pressed` action for every press it is handed, so the repeats of a
        // dropped key must be handed to no handler at all
        function test_alwaysOnHandlerGetsNoDroppedRepeat() {
            openScreensaver("starfield");
            voiceNav.log = [];
            ui.inputController.pressKey("VOLUME_UP", false);
            wait(100);
            verify(stillOpen(), "still open while the key is held");
            ssLoader.item.close();
            waitGone();
            restOfTheHold("VOLUME_UP");
            compare(JSON.stringify(voiceNav.log), JSON.stringify(["vol"]),
                    "one press: the one made on the screensaver");
            compare(JSON.stringify(page.log), "[]", "nothing of the key reached the page behind");
        }

        // the limit: a key pressed once the screensaver is gone is a key on the page, like any other
        function test_pressAfterItIsGoneActsOnThePage() {
            openScreensaver("starfield");
            ssLoader.item.close();
            waitGone();
            tapKey("DPAD_UP");
            wait(50);
            compare(JSON.stringify(page.log), JSON.stringify(["p:DPAD_UP", "r:DPAD_UP"]), "the page got that key");
        }

        // only the screensaver's keys are dropped: a key pressed on the page before the screensaver opened, and held
        // through its opening and closing, stays the page's
        function test_keyHeldOnThePageKeepsGoingToThePage() {
            ScreensaverConfig.theme = "starfield";
            pageNav.takeControl();
            page.log = [];
            ui.inputController.pressKey("DPAD_UP", false);
            ssLoader.active = true;
            tryVerify(function() { return ssLoader.item !== null && ssLoader.item.opened; }, 5000, "screensaver open");
            ssLoader.item.close();
            waitGone();
            restOfTheHold("DPAD_UP");
            compare(JSON.stringify(page.log),
                    JSON.stringify(["p:DPAD_UP", "p:DPAD_UP", "p:DPAD_UP", "p:DPAD_UP", "r:DPAD_UP"]),
                    "the press, the repeats and the release all reached the page");
        }
    }
}
