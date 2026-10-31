// Copyright (c) 2026 madalone. v1.6.0 — QML tests for the WiFi diagnostics sheet (W13, with the W2 link details).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Wifi 1.0

import "qrc:/settings/settings" as SettingsPages

Item {
    id: testRoot
    width: 480
    height: 850

    SettingsPages.WifiDiagnostics {
        id: diag
    }

    // a KeyValueRow by its key
    function findRow(item, key) {
        if (item.key !== undefined && item.fullValue !== undefined && item.key === key) return item;
        var kids = item.children || [];
        for (var i = 0; i < kids.length; i++) {
            var found = findRow(kids[i], key);
            if (found) return found;
        }
        return null;
    }

    // a Button by its label
    function findButton(item, label) {
        if (item.trigger !== undefined && item.text === label) return item;
        var kids = item.children || [];
        for (var i = 0; i < kids.length; i++) {
            var found = findButton(kids[i], label);
            if (found) return found;
        }
        return null;
    }

    TestCase {
        name: "WifiDiagnostics"
        when: windowShown

        function init() {
            Wifi.resetDefaults();
        }

        function cleanup() {
            diag.close();
            tryCompare(diag, "visible", false);
        }

        function openSheet() {
            diag.open();
            tryCompare(diag, "opened", true);
        }

        function test_opening_asksForAFreshSample() {
            openSheet();
            verify(Wifi.statusRequests >= 1, "getWifiStatus called on open");
        }

        function test_channel_data() {
            return [
                { tag: "2.4 GHz, channel 1", frequency: 2412, channel: "1" },
                { tag: "2.4 GHz, channel 6", frequency: 2437, channel: "6" },
                { tag: "2.4 GHz, channel 13", frequency: 2472, channel: "13" },
                { tag: "2.4 GHz, channel 14", frequency: 2484, channel: "14" },
                { tag: "5 GHz, channel 36", frequency: 5180, channel: "36" },
                { tag: "unknown", frequency: 0, channel: "—" }
            ];
        }

        function test_channel(data) {
            Wifi.currentNetwork.frequency = data.frequency;
            openSheet();
            var row = findRow(diag.contentItem, "Channel");
            verify(row, "Channel row");
            compare(row.value, data.channel);
        }

        function test_signal_showsSnrOnlyWhenTheFirmwareReportsIt() {
            openSheet();
            var row = findRow(diag.contentItem, "Signal");
            verify(row, "Signal row");
            compare(row.value, "-55 dBm");
            Wifi.currentSnr = 30;
            compare(row.value, "-55 dBm  (SNR 30 dB)");
        }

        function test_linkDetails() {
            openSheet();
            compare(findRow(diag.contentItem, "Link speed").value, "72 Mbps");
            compare(findRow(diag.contentItem, "BSSID").value, "aa:bb:cc:dd:ee:ff");
        }

        function test_throughput_hiddenUntilKnown() {
            openSheet();
            var row = findRow(diag.contentItem, "Throughput");
            verify(row, "Throughput row");
            verify(!row.visible, "hidden while the firmware reports 0");
            Wifi.currentEstimatedThroughput = 150;
            verify(row.visible, "shown once known");
            compare(row.value, "150 Mbps");
        }

        function test_sinceLastDrop_data() {
            return [
                { tag: "never", secs: -1, text: "None since start-up" },
                { tag: "seconds", secs: 42, text: "42 s" },
                { tag: "minutes", secs: 125, text: "2 min" },
                { tag: "hours", secs: 7500, text: "2 h 5 min" }
            ];
        }

        function test_sinceLastDrop(data) {
            Wifi.secondsSinceLastDisconnect = data.secs;
            openSheet();
            compare(findRow(diag.contentItem, "Since last drop").value, data.text);
        }

        function test_currentSession() {
            openSheet();
            var row = findRow(diag.contentItem, "Current session");
            Wifi.currentSessionDurationSec = 3725;
            compare(row.value, "01:02:05");
            Wifi.isConnected = false;
            compare(row.value, "Disconnected");
        }

        function test_drops() {
            Wifi.disconnectCount = 3;
            openSheet();
            compare(findRow(diag.contentItem, "Drops since start-up").value, "3");
        }

        function test_keypad_startsOnResetAndReachesClose() {
            openSheet();
            var reset = findButton(diag.contentItem, "Reset counters");
            var close = findButton(diag.contentItem, "Close");
            verify(reset && close, "both buttons");
            // the sheet claims the focus for its first control once it owns the input (deferred)
            tryVerify(function() { return reset.activeFocus; }, 2000, "Reset counters selected on open");
            keyClick(Qt.Key_Down);
            tryVerify(function() { return close.activeFocus; }, 2000, "DOWN reaches Close");
            keyClick(Qt.Key_Up);
            tryVerify(function() { return reset.activeFocus; }, 2000, "UP back to Reset counters");
        }

        function test_reset_asksForConfirmationFirst() {
            Wifi.disconnectCount = 5;
            openSheet();
            var before = ui.warningNotifications;
            findButton(diag.contentItem, "Reset counters").trigger();
            compare(ui.warningNotifications, before + 1, "a confirmation is shown");
            compare(Wifi.disconnectCount, 5, "nothing is reset before it is confirmed");
        }
    }
}
