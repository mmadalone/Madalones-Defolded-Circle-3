// Copyright (c) 2026 madalone. WiFi diagnostics sheet (W13, v1.4.17; link details W2 since v1.6.0): signal history,
// link details and drop counters, in the style of the WiFi info sheet it opens from.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Wifi 1.0

import "qrc:/components" as Components

Popup {
    id: wifiDiagnostics
    width: parent.width; height: parent.height
    y: 500
    opacity: 0
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    enter: Transition {
        SequentialAnimation {
            ParallelAnimation {
                PropertyAnimation { properties: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { properties: "y"; from: 500; to: 0; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    }

    exit: Transition {
        SequentialAnimation {
            PropertyAnimation { properties: "y"; from: 0; to: 500; easing.type: Easing.InExpo; duration: 300 }
            PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0 }
        }
    }

    // let the popup take the keyboard focus so the buttons can be reached with the d-pad
    focus: true

    onOpened: {
        buttonNavigation.takeControl();
        Wifi.getWifiStatus();   // a first sample right away, so the history shows something on the first open
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        manageFocus: true
        initialFocusItem: resetButton
        scrollTarget: infoFlickable
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    wifiDiagnostics.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    wifiDiagnostics.close();
                }
            }
        }
    }

    // Every 5 s while the sheet is open; the 30 s background poll (W6) keeps running. Each status adds a sample
    // to the signal history kept in C++.
    Timer {
        id: fastPollTimer
        interval: 5000
        repeat: true
        running: wifiDiagnostics.opened
        onTriggered: Wifi.getWifiStatus()
    }

    background: Components.Dim {}

    MouseArea {
        anchors { top: parent.top; bottom: infoContainer.top; left: parent.left; right: parent.right }
        onClicked: wifiDiagnostics.close();
    }

    function fmtUptime(secs) {
        var h = Math.floor(secs / 3600);
        var m = Math.floor((secs % 3600) / 60);
        var s = secs % 60;
        var pad = function(n) { return n < 10 ? "0" + n : "" + n; };
        return pad(h) + ":" + pad(m) + ":" + pad(s);
    }

    function fmtSinceDrop(secs) {
        //: No WiFi connection drop since the remote started
        if (secs < 0) return qsTr("None since start-up");
        //: Duration in seconds, e.g. "42 s"
        if (secs < 60) return qsTr("%1 s").arg(secs);
        //: Duration in minutes, e.g. "17 min"
        if (secs < 3600) return qsTr("%1 min").arg(Math.floor(secs / 60));
        //: Duration in hours and minutes, e.g. "2 h 05 min"
        return qsTr("%1 h %2 min").arg(Math.floor(secs / 3600)).arg(Math.floor((secs % 3600) / 60));
    }

    // 2.4 GHz: channel 1 is 2412 MHz, the channels are 5 MHz apart, 14 is 2484 MHz. 5 GHz: (f - 5000) / 5.
    function channelOf(f) {
        if (f === 2484) return "14";
        if (f >= 2412 && f <= 2472) return ((f - 2412) / 5 + 1).toString();
        if (f >= 5000) return ((f - 5000) / 5).toString();
        return "—";
    }

    Item {
        id: infoContainer
        width: ui.width
        height: Math.min(infoColumn.height + 40, wifiDiagnostics.height - 60)
        anchors.bottom: parent.bottom

        Components.Sheet {
            anchors.fill: parent
        }

        Flickable {
            id: infoFlickable
            anchors { fill: parent; topMargin: 20; bottomMargin: 20 }
            contentWidth: width
            contentHeight: infoColumn.height
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            maximumFlickVelocity: 6000
            flickDeceleration: 1000
            clip: true

            ColumnLayout {
                id: infoColumn
                spacing: 0
                width: parent.width

                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.bottomMargin: 6
                    wrapMode: Text.WordWrap
                    color: colors.textPrimary
                    text: qsTr("WiFi diagnostics")
                    font: fonts.label()
                }

                Components.Divider {}

                // signal history: the last 60 samples, with the thresholds of the signal strength tiers
                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 14
                    wrapMode: Text.WordWrap
                    color: colors.textSecondary
                    //: Graph of the WiFi signal strength over the last minutes
                    text: qsTr("Signal history")
                    font: fonts.help()
                }

                Canvas {
                    id: rssiSparkline
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 10
                    Layout.bottomMargin: 14
                    Layout.preferredHeight: 80
                    antialiasing: true

                    readonly property int rssiMin: -100
                    readonly property int rssiMax: -30

                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.clearRect(0, 0, width, height);

                        // reference lines at the SignalStrength tier thresholds (-60 / -76 / -84 dBm)
                        ctx.strokeStyle = colors.divider;
                        ctx.lineWidth = 2;
                        ctx.setLineDash([4, 4]);
                        var thresholds = [-60, -76, -84];
                        for (var t = 0; t < thresholds.length; t++) {
                            var ty = height - ((thresholds[t] - rssiMin) / (rssiMax - rssiMin)) * height;
                            ctx.beginPath();
                            ctx.moveTo(0, ty);
                            ctx.lineTo(width, ty);
                            ctx.stroke();
                        }
                        ctx.setLineDash([]);

                        var samples = Wifi.rssiHistory;
                        if (samples.length < 2) return;

                        ctx.strokeStyle = colors.green;
                        ctx.lineWidth = 3;
                        ctx.beginPath();
                        for (var i = 0; i < samples.length; i++) {
                            var x = (i / (samples.length - 1)) * width;
                            var rssi = Math.max(rssiMin, Math.min(rssiMax, samples[i]));
                            var y = height - ((rssi - rssiMin) / (rssiMax - rssiMin)) * height;
                            if (i === 0) ctx.moveTo(x, y);
                            else ctx.lineTo(x, y);
                        }
                        ctx.stroke();
                    }

                    Connections {
                        target: Wifi
                        ignoreUnknownSignals: true
                        function onRssiHistoryChanged() { rssiSparkline.requestPaint(); }
                    }

                    Component.onCompleted: requestPaint()
                }

                Components.Divider {}

                // link details (W2: moved here from the info sheet in v1.6.0; the firmware leaves noise and SNR at 0)
                Components.KeyValueRow {
                    //: WiFi signal strength in dBm
                    key: qsTr("Signal")
                    value: Wifi.currentRssi + " dBm" + (Wifi.currentSnr > 0 ? "  (SNR " + Wifi.currentSnr + " dB)" : "")
                }

                Components.KeyValueRow {
                    key: qsTr("Link speed")
                    value: Wifi.currentLinkSpeed + " Mbps"
                }

                Components.KeyValueRow {
                    visible: Wifi.currentEstimatedThroughput > 0
                    //: Estimated data rate of the WiFi connection
                    key: qsTr("Throughput")
                    value: Wifi.currentEstimatedThroughput + " Mbps"
                }

                Components.KeyValueRow {
                    //: Hardware address of the WiFi access point the remote is connected to
                    key: qsTr("BSSID")
                    value: Wifi.currentBssid
                    // an address is read in full: under the key when it does not fit next to it
                    fullValue: true
                }

                Components.KeyValueRow {
                    //: WiFi channel number
                    key: qsTr("Channel")
                    value: wifiDiagnostics.channelOf(Wifi.currentNetwork.frequency)
                }

                // connection drops since the remote started
                Components.KeyValueRow {
                    //: Number of times the WiFi connection was lost since the remote started
                    key: qsTr("Drops since start-up")
                    value: Wifi.disconnectCount
                }

                Components.KeyValueRow {
                    //: How long the current WiFi connection has lasted
                    key: qsTr("Current session")
                    value: Wifi.isConnected ? wifiDiagnostics.fmtUptime(Wifi.currentSessionDurationSec)
                                            : qsTr("Disconnected")
                }

                Components.KeyValueRow {
                    key: qsTr("Since last drop")
                    value: wifiDiagnostics.fmtSinceDrop(Wifi.secondsSinceLastDisconnect)
                    showDivider: false
                }

                Components.Button {
                    id: resetButton
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 10
                    text: qsTr("Reset counters")
                    variant: "secondary"

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: closeButton

                    trigger: function() {
                        ui.createActionableWarningNotification(
                            qsTr("Reset diagnostic counters?"),
                            qsTr("This sets the drop counter to zero and clears the signal history."),
                            "uc:triangle-exclamation",
                            function() { Wifi.resetDiagnosticCounters(); },
                            qsTr("Reset"));
                    }
                }

                Components.Button {
                    id: closeButton
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 20
                    text: qsTr("Close")
                    variant: "secondary"

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: resetButton

                    trigger: function() { wifiDiagnostics.close(); }
                }
            }
        }

        // the one scroll indicator of the sheet when its content is taller than the screen
        Components.ScrollIndicator {
            parentObj: infoFlickable
        }
    }
}
