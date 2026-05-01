// Copyright (c) 2026 madalone. v1.4.34 — QML tests for BatteryStatusChip (Mod 3).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Battery 1.0
import Config 1.0

import "qrc:/components/overlays" as Overlays

Item {
    id: testRoot
    width: 200
    height: 100

    // ── Helper: recursive findChild by objectName (more reliable than children[] traversal). ──
    function findByObjectName(item, name) {
        if (item.objectName === name) return item;
        if (!item.children) return null;
        for (var i = 0; i < item.children.length; i++) {
            var found = findByObjectName(item.children[i], name);
            if (found) return found;
        }
        return null;
    }

    Overlays.BatteryStatusChip {
        id: chip
        anchors.centerIn: parent
    }

    TestCase {
        id: testCase
        name: "BatteryStatusChip"
        when: windowShown

        function init() {
            Battery.resetDefaults();
            Config.resetDefaults();
        }

        function test_charging_state() {
            Battery.isCharging = true;
            Battery.level = 80;

            var bolt = findByObjectName(chip, "batteryBolt");
            var text = findByObjectName(chip, "batteryText");
            var bar  = findByObjectName(chip, "batteryBarWrapper");

            verify(bolt !== null, "batteryBolt found");
            verify(text !== null, "batteryText found");
            verify(bar  !== null, "batteryBarWrapper found");

            compare(bolt.visible, true,  "bolt visible while charging");
            compare(bar.visible,  false, "bar hidden while charging (Item gates !isCharging)");
            compare(text.visible, true,  "percent text visible while charging (OR clause)");
        }

        function test_discharging_normal() {
            Battery.isCharging = false;
            Battery.level = 75;
            Battery.low = false;

            var bolt = findByObjectName(chip, "batteryBolt");
            var bar  = findByObjectName(chip, "batteryBarWrapper");
            var fill = findByObjectName(chip, "batteryBarFill");

            compare(bolt.visible, false, "bolt hidden while discharging");
            compare(bar.visible,  true,  "bar wrapper visible while discharging");
            compare(fill.color,   colors.offwhite, "bar fill color is offwhite at normal battery");
        }

        function test_discharging_low_color() {
            Battery.isCharging = false;
            Battery.level = 50;        // intentionally NOT < 10 — color is driven by .low, not .level
            Battery.low = true;

            var fill = findByObjectName(chip, "batteryBarFill");
            compare(fill.color, colors.red, "bar fill color is red when Battery.low=true (independent of level)");
        }

        function test_discharging_low_height_growth() {
            // Catches the binding-asymmetry bug if anyone refactors the chip:
            // height growth fires off `level < 10`, color fires off `low`. They're independent.
            Battery.isCharging = false;
            Battery.level = 8;
            Battery.low = false;       // intentionally false to isolate the height path

            var fill = findByObjectName(chip, "batteryBarFill");
            var bar  = findByObjectName(chip, "batteryBarWrapper");
            // height = (parent.height * level / 100) + (level < 10 ? 2 : 0)
            //       = (30 * 8 / 100) + 2 = 4.4
            // The +2 boost confirms the level<10 path is hit.
            verify(fill.height > (bar.height * 8 / 100),
                   "bar fill height has the +2px boost when level<10 (bypass: low=false ensures color path doesn't interfere)");
        }

        function test_percentage_text_gating_when_not_charging() {
            Battery.isCharging = false;
            Config.showBatteryPercentage = false;
            var text = findByObjectName(chip, "batteryText");
            compare(text.visible, false, "text hidden when !charging && !showPercent");

            Config.showBatteryPercentage = true;
            compare(text.visible, true, "text visible when showPercent flips on");

            Config.showBatteryPercentage = false;
            compare(text.visible, false, "text hidden again when showPercent flips off");
        }

        function test_percentage_text_always_when_charging() {
            Battery.isCharging = true;
            Config.showBatteryPercentage = false;
            var text = findByObjectName(chip, "batteryText");
            compare(text.visible, true, "text visible while charging regardless of showPercent (OR clause precedence)");
        }
    }
}
