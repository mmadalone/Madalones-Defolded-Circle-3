// Copyright (c) 2026 madalone. v1.6.5 — Matrix's two ways to steer on the screensaver's General behavior: "DPAD
// interactive" and "Touch directions" may both be on, the page then shows "Remember direction" once (the row under DPAD
// interactive; both rows set the same switch), and the keypad walks every shown row down and back up.
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
        name: "SteeringRows"
        when: windowShown

        function item(name) {
            var it = findByObjectName(generalBehavior, name);
            verify(it !== null, name + " exists");
            return it;
        }

        function init() {
            ScreensaverConfig.resetDefaults();   // Matrix, DPAD interactive on, Touch directions off
            ScreensaverConfig.theme = "matrix";
        }

        function shown(name) { return item(name).visible; }

        // one "Remember direction" row in every combination
        function test_rememberDirectionShownOnce_data() {
            return [
                { tag: "DPAD interactive only", dpad: true, touch: false, dpadRow: true, touchRow: false },
                { tag: "Touch directions only", dpad: false, touch: true, dpadRow: false, touchRow: true },
                { tag: "both", dpad: true, touch: true, dpadRow: true, touchRow: false },
                { tag: "neither", dpad: false, touch: false, dpadRow: false, touchRow: false }
            ];
        }
        function test_rememberDirectionShownOnce(data) {
            ScreensaverConfig.dpadEnabled = data.dpad;
            ScreensaverConfig.tapDirection = data.touch;
            tryVerify(function() { return shown("dpadPersistSwitch") === data.dpadRow; }, 2000,
                      "the row under DPAD interactive");
            compare(shown("tapDirPersistSwitch"), data.touchRow, "the row under Touch directions");
            compare(shown("touchbarSpeedSwitch"), data.dpad, "Touchbar speed follows DPAD interactive");
            compare(shown("swipeSpeedSwitch"), data.touch, "Swipe speed follows Touch directions");
        }

        // both switches may stay on: turning one on leaves the other alone
        function test_bothSwitchesStayOn() {
            item("tapDirectionSwitch").trigger();
            compare(ScreensaverConfig.tapDirection, true);
            compare(ScreensaverConfig.dpadEnabled, true, "DPAD interactive stays on");
            item("dpadEnabledSwitch").trigger();
            item("dpadEnabledSwitch").trigger();
            compare(ScreensaverConfig.dpadEnabled, true);
            compare(ScreensaverConfig.tapDirection, true, "Touch directions stays on");
        }

        // DOWN through the given switches from the first, then UP back
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

        function test_keypad_both() {
            ScreensaverConfig.tapDirection = true;
            walk(["dpadEnabledSwitch", "dpadPersistSwitch", "touchbarSpeedSwitch", "tapDirectionSwitch",
                  "swipeSpeedSwitch", "idleEnabledSwitch"]);
        }

        function test_keypad_touchOnly() {
            ScreensaverConfig.dpadEnabled = false;
            ScreensaverConfig.tapDirection = true;
            walk(["dpadEnabledSwitch", "tapDirectionSwitch", "tapDirPersistSwitch", "swipeSpeedSwitch",
                  "idleEnabledSwitch"]);
        }

        function test_keypad_dpadOnly() {
            walk(["dpadEnabledSwitch", "dpadPersistSwitch", "touchbarSpeedSwitch", "tapDirectionSwitch",
                  "idleEnabledSwitch"]);
        }
    }
}
