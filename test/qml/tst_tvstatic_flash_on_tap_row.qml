// Copyright (c) 2026 madalone. v1.6.5 — TV Static's "Flash on tap" row: while "Double-tap to close" is off a tap closes
// the screensaver, so a tap can't flash it, and the switch is greyed out the page's way (enabled off, opacity 0.4, as
// Tracking bar speed and Flash interval) with a help line saying why; the keypad skips it, as Qt's KeyNavigation skips a
// disabled item, and a tap on it changes nothing.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

import "qrc:/settings/settings/chargingscreen" as ChargingScreen

Item {
    id: testRoot
    width: 480
    height: 2400

    function findByObjectName(item, name) {
        if (item.objectName === name) return item;
        if (!item.children) return null;
        for (var i = 0; i < item.children.length; i++) {
            var found = findByObjectName(item.children[i], name);
            if (found) return found;
        }
        return null;
    }

    Item { id: stubSettingsPage }

    ChargingScreen.TvStaticSettings {
        id: tvStaticSettings
        width: 480
        settingsPage: stubSettingsPage
    }

    TestCase {
        name: "TvStaticFlashOnTapRow"
        when: windowShown

        property var row: null
        property var sw: null

        function item(name) {
            var it = findByObjectName(tvStaticSettings, name);
            verify(it !== null, name + " exists");
            return it;
        }

        function init() {
            ScreensaverConfig.resetDefaults();   // Double-tap to close on, Flash on tap on
            row = item("flashOnTapRow");
            sw = item("flashOnTapSwitch");
        }

        function test_enabledWithDoubleTapToClose() {
            compare(row.title, "Flash on tap");
            compare(sw.enabled, true);
            compare(sw.opacity, 1.0);
            compare(row.help, "", "no help line");
        }

        function test_greyedWithoutDoubleTapToClose() {
            ScreensaverConfig.tapToClose = false;
            compare(sw.enabled, false);
            fuzzyCompare(sw.opacity, 0.4, 0.001);
            compare(row.help, "Needs Double-tap to close: while that is off, one tap closes the screensaver.");
            ScreensaverConfig.tapToClose = true;
            compare(sw.enabled, true, "back on with Double-tap to close");
            compare(row.help, "");
        }

        // a tap on the greyed switch changes nothing; on the enabled one it toggles (the control case)
        function test_tapOnTheGreyedSwitch() {
            ScreensaverConfig.tapToClose = false;
            mouseClick(sw);
            compare(ScreensaverConfig.tvStaticFlashOnTap, true, "unchanged while greyed out");
            ScreensaverConfig.tapToClose = true;
            mouseClick(sw);
            compare(ScreensaverConfig.tvStaticFlashOnTap, false, "a tap toggles it when it isn't greyed out");
        }

        // DOWN from Tint and UP from Auto flash bursts: the switch while it is enabled, past it while it is greyed out
        function test_keypad_data() {
            return [ { tag: "Double-tap to close on", doubleTap: true, between: "flashOnTapSwitch" },
                     { tag: "Double-tap to close off", doubleTap: false, between: "" } ];
        }
        function test_keypad(data) {
            ScreensaverConfig.tapToClose = data.doubleTap;
            var tint = item("tvStaticTintPicker");
            var auto = item("channelFlashAutoSwitch");
            tint.forceActiveFocus();
            tryVerify(function() { return tint.activeFocus; }, 2000, "Tint focused");
            keyClick(Qt.Key_Down);
            if (data.between !== "") {
                tryVerify(function() { return sw.activeFocus; }, 2000, "DOWN from Tint reaches Flash on tap");
                keyClick(Qt.Key_Down);
            }
            tryVerify(function() { return auto.activeFocus; }, 2000, "DOWN reaches Auto flash bursts");
            keyClick(Qt.Key_Up);
            if (data.between !== "") {
                tryVerify(function() { return sw.activeFocus; }, 2000, "UP from Auto flash bursts reaches Flash on tap");
                keyClick(Qt.Key_Up);
            }
            tryVerify(function() { return tint.activeFocus; }, 2000, "UP reaches Tint");
            compare(ScreensaverConfig.tvStaticFlashOnTap, true, "the walk changed nothing");
        }
    }
}
