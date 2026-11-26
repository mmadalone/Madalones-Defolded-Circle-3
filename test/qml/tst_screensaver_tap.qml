// Copyright (c) 2026 madalone. v1.6.5 — what a screen tap does on the screensaver: TV Static's "Flash on tap" (a lone
// tap flashes it once the double-tap window has passed; up to v1.6.4 a tap never reached TV Static), Matrix's lone tap
// (its tap effects, now through ChargingScreen.fireLoneTap()), a theme name the Loader doesn't know (it loads Matrix,
// and its taps now behave as on Matrix), and a lone tap whose window ends during the close (no TypeError).
// Runs the REAL ChargingScreen.qml, loaded asynchronously like main.qml's chargingScreenLoader, driven by mouse events;
// the decisions are ScreensaverConfig.loneTapAction() / matrixLoaded (src/ui/screensaverInputPolicy.h).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0
import TestLog 1.0

Item {
    id: root
    width: 480
    height: 850

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
        name: "ScreensaverTap"
        when: windowShown

        SignalSpy { id: flashSpy; signalName: "u_flashChanged" }

        // the loaded theme: the item of ChargingScreen's theme Loader
        function themeItem() {
            if (!ssLoader.item || !ssLoader.item.contentItem) return null;
            var kids = ssLoader.item.contentItem.children;
            for (var i = 0; i < kids.length; ++i) {
                var k = kids[i];
                if (k.item !== undefined && k.item !== null && k.item.hasOwnProperty("u_flash")) return k.item;
            }
            return null;
        }

        function openScreensaver(theme) {
            ScreensaverConfig.theme = theme;
            ssLoader.active = true;
            tryVerify(function() { return ssLoader.item !== null && ssLoader.item.opened; }, 5000, "screensaver open");
            if (theme === "tvstatic") {
                var t = themeItem();
                verify(t !== null, "the TV Static theme is loaded");
                flashSpy.target = t;
                flashSpy.clear();
            }
        }

        function tapAt(x, y) { mouseClick(ssLoader.item.contentItem, x, y); }
        function tap() { tapAt(240, 425); }

        function tapCalls(rain) {
            var n = 0;
            for (var i = 0; i < rain.calls.length; ++i)
                if (rain.calls[i].indexOf("input:tap:") === 0) ++n;
            return n;
        }

        function init() {
            ScreensaverConfig.resetDefaults();
            ScreensaverConfig.tvStaticChannelFlashAuto = false;   // only the taps flash it
            ScreensaverConfig.lastDirection = "";
            flashSpy.target = null;
            flashSpy.clear();
        }

        function cleanup() {
            flashSpy.target = null;
            if (ssLoader.item) ssLoader.item.close();
            else ssLoader.active = false;
            tryVerify(function() { return !ssLoader.active; }, 3000, "screensaver closed");
            wait(20);
        }

        // with "Double-tap to close" on (the default), a lone tap flashes TV Static once the double-tap window passed
        function test_tvStaticLoneTapFlashes() {
            openScreensaver("tvstatic");
            tap();
            compare(flashSpy.count, 0, "nothing during the double-tap window");
            tryVerify(function() { return flashSpy.count > 0; }, 1000, "a lone tap flashes TV Static");
            verify(ssLoader.item.opened, "a lone tap doesn't close it");
        }

        function test_tvStaticDoubleTapClosesWithoutFlash() {
            openScreensaver("tvstatic");
            tap();
            wait(100);
            tap();
            tryVerify(function() { return !ssLoader.active; }, 2000, "a double tap closes it");
            compare(flashSpy.count, 0, "no flash from the taps of a double tap");
        }

        function test_tvStaticFlashOnTapOff() {
            ScreensaverConfig.tvStaticFlashOnTap = false;
            openScreensaver("tvstatic");
            tap();
            wait(500);
            compare(flashSpy.count, 0, "no flash with Flash on tap off");
            verify(ssLoader.item.opened);
        }

        function test_singleTapClosesWithDoubleTapToCloseOff() {
            ScreensaverConfig.tapToClose = false;
            openScreensaver("tvstatic");
            tap();
            tryVerify(function() { return !ssLoader.active; }, 1000, "one tap closes it");
            compare(flashSpy.count, 0, "no flash on a tap that closes it");
        }

        // Matrix's lone tap: one tap-effects input after the double-tap window, and the screensaver stays open
        function test_matrixLoneTapFiresTapEffectsOnce() {
            openScreensaver("matrix");
            var rain = ssLoader.item.matrixRainRef;
            verify(rain !== null, "the MatrixRain stub is loaded");
            tap();
            wait(150);
            compare(tapCalls(rain), 0, "nothing during the double-tap window");
            tryVerify(function() { return tapCalls(rain) === 1; }, 1000, "one tap-effects input: " + JSON.stringify(rain.calls));
            wait(200);
            compare(tapCalls(rain), 1, "exactly one");
            verify(ssLoader.item.opened, "a lone tap doesn't close it");
        }

        // the first tap of a double tap fires no tap effects
        function test_matrixDoubleTapClosesWithoutTapEffects() {
            openScreensaver("matrix");
            var rain = ssLoader.item.matrixRainRef;
            tap();
            wait(100);
            tap();
            var calls = rain.calls;
            tryVerify(function() { return !ssLoader.active; }, 2000, "a double tap closes it");
            var n = 0;
            for (var i = 0; i < calls.length; ++i) if (calls[i].indexOf("input:tap:") === 0) ++n;
            compare(n, 0, "no tap effects from a double tap");
        }

        // Touch directions on: an edge zone steers and fires the tap effects at once (no double-tap wait)
        function test_matrixZoneTapFiresAtOnce() {
            ScreensaverConfig.tapDirection = true;
            openScreensaver("matrix");
            var rain = ssLoader.item.matrixRainRef;
            tapAt(40, 425);   // the left zone
            verify(rain.calls.indexOf("input:left") >= 0, "steers left at once: " + JSON.stringify(rain.calls));
            compare(tapCalls(rain), 1, "and fires the tap effects at once");
            verify(ssLoader.item.opened);
        }

        // a theme name the Loader doesn't know loads Matrix; its taps then behave as on Matrix (lone tap: tap effects;
        // Touch directions: zones). Up to v1.6.4 both compared the name with "matrix" and did nothing.
        function test_unknownThemeNameTapsLikeMatrix() {
            openScreensaver("aurora");
            var rain = ssLoader.item.matrixRainRef;
            verify(rain !== null, "Matrix loaded for an unknown theme name");
            tap();
            tryVerify(function() { return tapCalls(rain) === 1; }, 1000, "lone tap: tap effects");
            ScreensaverConfig.tapDirection = true;
            tapAt(440, 425);  // the right zone
            verify(rain.calls.indexOf("input:right") >= 0, "zone tap steers: " + JSON.stringify(rain.calls));
        }

        // a lone tap whose window ends while the screensaver is closing (its theme already unloaded) raises no
        // TypeError: the close stops the window's timer, and fireLoneTap() checks the theme
        function test_tapThenCloseIsQuiet() {
            openScreensaver("tvstatic");
            var before = TestLog.typeErrors();
            tap();
            wait(275);
            ssLoader.item.close();   // the double-tap window runs out during the 50 ms exit
            wait(400);
            compare(TestLog.typeErrors(), before, "no TypeError");
        }
    }
}
