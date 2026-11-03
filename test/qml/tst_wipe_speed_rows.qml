// Copyright (c) 2026 madalone. The "Wipe speed" rows under the Tap effects and the Chaos events "Screen wipe" switch
// (2026-11-03): shown only while the switch is on, and reached with the keypad from it and back.
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

    Column {
        width: parent.width

        ChargingScreen.TapSection {
            id: tapSection
            width: parent.width
            settingsPage: stubSettingsPage
        }
        ChargingScreen.ChaosSection {
            id: chaosSection
            width: parent.width
            settingsPage: stubSettingsPage
        }
    }

    TestCase {
        name: "WipeSpeedRows"
        when: windowShown

        function init() {
            ScreensaverConfig.resetDefaults();
            ScreensaverConfig.tapEnabled = true;
            ScreensaverConfig.glitch = true;
            ScreensaverConfig.glitchChaos = true;
        }

        function controls(section, wipeName, speedName, nextName) {
            var c = {
                wipe: findByObjectName(section, wipeName),
                speed: findByObjectName(section, speedName),
                next: findByObjectName(section, nextName)
            };
            verify(c.wipe && c.speed && c.next, "the Screen wipe switch, the Wipe speed slider and the next switch");
            return c;
        }

        // Down from the switch to the slider and on to the next switch, then Up back
        function walk(c) {
            c.wipe.forceActiveFocus();
            tryVerify(function() { return c.wipe.activeFocus; }, 2000, "Screen wipe focused");
            keyClick(Qt.Key_Down);
            tryVerify(function() { return c.speed.activeFocus; }, 2000, "DOWN from Screen wipe reaches Wipe speed");
            keyClick(Qt.Key_Down);
            tryVerify(function() { return c.next.activeFocus; }, 2000, "DOWN from Wipe speed reaches the next switch");
            keyClick(Qt.Key_Up);
            tryVerify(function() { return c.speed.activeFocus; }, 2000, "UP back to Wipe speed");
            keyClick(Qt.Key_Up);
            tryVerify(function() { return c.wipe.activeFocus; }, 2000, "UP back to Screen wipe");
        }

        // With the switch off the row is hidden and the keypad goes straight to the next switch and back
        function skip(c) {
            verify(!c.speed.visible, "Wipe speed hidden while Screen wipe is off");
            c.wipe.forceActiveFocus();
            tryVerify(function() { return c.wipe.activeFocus; }, 2000, "Screen wipe focused");
            keyClick(Qt.Key_Down);
            tryVerify(function() { return c.next.activeFocus; }, 2000, "DOWN skips the hidden Wipe speed");
            keyClick(Qt.Key_Up);
            tryVerify(function() { return c.wipe.activeFocus; }, 2000, "UP skips it back");
        }

        function test_tap_keypadThroughWipeSpeed() {
            ScreensaverConfig.tapWipe = true;
            var c = controls(tapSection, "tapWipeSwitch", "tapWipeSpeedSlider", "tapRandomizeSwitch");
            tryVerify(function() { return c.speed.visible; }, 2000, "Wipe speed shown while Screen wipe is on");
            walk(c);
        }

        function test_tap_wipeSpeedSkippedWhenWipeOff() {
            ScreensaverConfig.tapWipe = false;
            skip(controls(tapSection, "tapWipeSwitch", "tapWipeSpeedSlider", "tapRandomizeSwitch"));
        }

        function test_chaos_keypadThroughWipeSpeed() {
            ScreensaverConfig.glitchChaosWipe = true;
            var c = controls(chaosSection, "chaosWipeSwitch", "chaosWipeSpeedSlider", "chaosScatterSwitch");
            tryVerify(function() { return c.speed.visible; }, 2000, "Wipe speed shown while Screen wipe is on");
            walk(c);
        }

        function test_chaos_wipeSpeedSkippedWhenWipeOff() {
            ScreensaverConfig.glitchChaosWipe = false;
            skip(controls(chaosSection, "chaosWipeSwitch", "chaosWipeSpeedSlider", "chaosScatterSwitch"));
        }

        function test_slidersFollowTheSettings() {
            ScreensaverConfig.tapWipe = true;
            ScreensaverConfig.glitchChaosWipe = true;
            var tapSpeed = findByObjectName(tapSection, "tapWipeSpeedSlider");
            var chaosSpeed = findByObjectName(chaosSection, "chaosWipeSpeedSlider");
            compare(tapSpeed.value, 100);
            compare(chaosSpeed.value, 100);
            compare(tapSpeed.from, 50);
            compare(tapSpeed.to, 600);
            ScreensaverConfig.tapWipeSpeed = 250;
            ScreensaverConfig.glitchChaosWipeSpeed = 450;
            compare(tapSpeed.value, 250);
            compare(chaosSpeed.value, 450);
        }
    }
}
