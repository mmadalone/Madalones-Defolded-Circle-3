// Copyright (c) 2026 madalone. v1.4.34 — QML tests for WifiStatusChip (Mod 4 v1.4.21).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Wifi 1.0
import Wifi.SignalStrength 1.0

import "qrc:/components/overlays" as Overlays

Item {
    id: testRoot
    width: 200
    height: 100

    function findByObjectName(item, name) {
        if (item.objectName === name) return item;
        if (!item.children) return null;
        for (var i = 0; i < item.children.length; i++) {
            var found = findByObjectName(item.children[i], name);
            if (found) return found;
        }
        return null;
    }

    Overlays.WifiStatusChip {
        id: chip
        anchors.centerIn: parent
    }

    TestCase {
        id: testCase
        name: "WifiStatusChip"
        when: windowShown

        function init() {
            Wifi.resetDefaults();
        }

        function test_disconnected() {
            Wifi.isConnected = false;
            var overlay  = findByObjectName(chip, "wifiOverlay");
            var strike   = findByObjectName(chip, "wifiStrikethrough");
            verify(overlay !== null, "wifiOverlay found");
            verify(strike !== null,  "wifiStrikethrough found");

            compare(strike.visible, true, "strikethrough visible when disconnected");
            compare(overlay.icon, "",     "overlay icon empty when disconnected");
            compare(overlay.opacity, 0,   "overlay invisible when icon is empty");
        }

        function test_connected_NONE() {
            Wifi.isConnected = true;
            Wifi.setSignalStrength(SignalStrength.NONE);
            var overlay = findByObjectName(chip, "wifiOverlay");
            var strike  = findByObjectName(chip, "wifiStrikethrough");
            compare(overlay.icon, "",       "NONE → overlay empty");
            compare(overlay.opacity, 0,     "NONE → overlay invisible");
            compare(strike.visible, false,  "strikethrough hidden when connected");
        }

        function test_connected_WEAK() {
            Wifi.isConnected = true;
            Wifi.setSignalStrength(SignalStrength.WEAK);
            var overlay = findByObjectName(chip, "wifiOverlay");
            compare(overlay.icon, "uc:wifi-01", "WEAK → uc:wifi-01");
            compare(overlay.opacity, 1,         "WEAK → overlay visible");
        }

        function test_connected_OK_and_GOOD() {
            // The chip's switch has fall-through: case OK: case GOOD: return "uc:wifi-02".
            // Both must produce the same icon.
            Wifi.isConnected = true;

            Wifi.setSignalStrength(SignalStrength.OK);
            var overlay = findByObjectName(chip, "wifiOverlay");
            compare(overlay.icon, "uc:wifi-02", "OK → uc:wifi-02 (fall-through case)");

            Wifi.setSignalStrength(SignalStrength.GOOD);
            compare(overlay.icon, "uc:wifi-02", "GOOD → uc:wifi-02 (fall-through case)");
        }

        function test_connected_EXCELLENT() {
            Wifi.isConnected = true;
            Wifi.setSignalStrength(SignalStrength.EXCELLENT);
            var overlay = findByObjectName(chip, "wifiOverlay");
            compare(overlay.icon, "uc:wifi-03", "EXCELLENT → uc:wifi-03");
        }

        function test_strength_change_rebinds() {
            // Catches the `Wifi.currentNetwork.signalStrength` re-bind path. The mock's
            // MockWifiNetwork emits signalStrengthChanged on mutation (deliberate divergence
            // from production CONSTANT — see MockWifi.h header comment). If the binding
            // doesn't re-evaluate, this test catches it.
            Wifi.isConnected = true;
            Wifi.setSignalStrength(SignalStrength.WEAK);
            var overlay = findByObjectName(chip, "wifiOverlay");
            compare(overlay.icon, "uc:wifi-01", "starting at WEAK");

            Wifi.setSignalStrength(SignalStrength.EXCELLENT);
            tryCompare(overlay, "icon", "uc:wifi-03", 100, "icon updates after strength change");
        }
    }
}
