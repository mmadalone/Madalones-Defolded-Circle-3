// Copyright (c) 2026 madalone. v1.6.5 — the "Undock delay" row of the screensaver's General behavior (GitHub issue #3):
// shown for Close on wake Undocked and Always, a slider of 500-2000 ms in steps of 250 over
// ScreensaverConfig.undockCloseMs (default 500), and the keypad path Close on wake → Undock delay → the next row and back.
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

    ChargingScreen.GeneralBehavior {
        id: generalBehavior
        width: 480
        settingsPage: stubSettingsPage
    }

    TestCase {
        name: "UndockDelayRow"
        when: windowShown

        property var row: null
        property var slider: null

        function item(name) {
            var it = findByObjectName(generalBehavior, name);
            verify(it !== null, name + " exists");
            return it;
        }

        function init() {
            ScreensaverConfig.resetDefaults();   // Close on wake Never, Undock delay 500 ms
            row = item("undockDelayRow");
            slider = item("undockDelaySlider");
        }

        function test_shownForTheChoicesThatCloseOnChargerLoss_data() {
            return [ { tag: "Never", choice: 0, shown: false },
                     { tag: "Undocked", choice: 1, shown: true },
                     { tag: "Always", choice: 2, shown: true } ];
        }
        function test_shownForTheChoicesThatCloseOnChargerLoss(data) {
            ScreensaverConfig.closeOnWake = data.choice;
            compare(row.visible, data.shown);
        }

        function test_theRow() {
            ScreensaverConfig.closeOnWake = 1;
            compare(row.title, "Undock delay");
            compare(row.value, "500 ms");
            verify(row.help.length > 0, "a help line");
            compare(slider.from, 500);
            compare(slider.to, 2000);
            compare(slider.stepSize, 250);
            compare(slider.value, 500);
            ScreensaverConfig.undockCloseMs = 1750;
            compare(slider.value, 1750, "follows the stored value");
            compare(row.value, "1750 ms");
        }

        // RIGHT / LEFT on the focused slider store the value, one step of 250 ms at a time, and stop at the ends
        function test_keysStoreTheValue() {
            ScreensaverConfig.closeOnWake = 1;
            slider.forceActiveFocus();
            tryVerify(function() { return slider.activeFocus; }, 2000, "the slider focused");
            keyClick(Qt.Key_Right);
            compare(ScreensaverConfig.undockCloseMs, 750);
            keyClick(Qt.Key_Left);
            keyClick(Qt.Key_Left);
            compare(ScreensaverConfig.undockCloseMs, 500, "stops at 500");
            for (var i = 0; i < 8; ++i) keyClick(Qt.Key_Right);
            compare(ScreensaverConfig.undockCloseMs, 2000, "stops at 2000");
        }

        // DOWN from Close on wake to the slider and on to the next row, then UP back
        function walk(names) {
            var first = item(names[0]);
            first.forceActiveFocus();
            tryVerify(function() { return first.activeFocus; }, 2000, names[0] + " focused");
            for (var i = 1; i < names.length; ++i) {
                keyClick(Qt.Key_Down);
                var it = item(names[i]);
                tryVerify(function() { return it.activeFocus; }, 2000, "DOWN reaches " + names[i]);
            }
            for (var j = names.length - 2; j >= 0; --j) {
                keyClick(Qt.Key_Up);
                var back = item(names[j]);
                tryVerify(function() { return back.activeFocus; }, 2000, "UP reaches " + names[j]);
            }
        }

        function test_keypad_data() {
            return [ { tag: "Matrix, Undocked", theme: "matrix", choice: 1, next: "dpadEnabledSwitch" },
                     { tag: "Matrix, Always", theme: "matrix", choice: 2, next: "dpadEnabledSwitch" },
                     { tag: "Minimal, Undocked", theme: "minimal", choice: 1, next: "idleEnabledSwitch" } ];
        }
        function test_keypad(data) {
            ScreensaverConfig.theme = data.theme;
            ScreensaverConfig.closeOnWake = data.choice;
            walk(["closeOnWakePicker", "undockDelaySlider", data.next]);
            compare(ScreensaverConfig.closeOnWake, data.choice, "the walk changed no choice");
            compare(ScreensaverConfig.undockCloseMs, 500, "nor the delay");
        }

        // with Never the row is skipped both ways
        function test_keypad_never() {
            ScreensaverConfig.theme = "matrix";
            walk(["closeOnWakePicker", "dpadEnabledSwitch"]);
        }
    }
}
