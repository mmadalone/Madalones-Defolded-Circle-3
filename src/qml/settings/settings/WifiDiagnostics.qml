// Copyright (c) 2026 madalone. WiFi Diagnostics popup (W13, v1.4.17). RSSI sparkline + drop counter.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

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

    onOpened: {
        buttonNavigation.takeControl();
        Wifi.getWifiStatus();   // immediate first sample so the sparkline shows something on first open
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": { "pressed": function() { wifiDiagnostics.close(); } },
            "HOME": { "pressed": function() { wifiDiagnostics.close(); } }
        }
    }

    // 5 s cadence while popup is open. The W6 30 s background poll continues regardless;
    // each getWifiStatus call pushes a fresh RSSI sample to the C++ ring buffer.
    Timer {
        id: fastPollTimer
        interval: 5000
        repeat: true
        running: wifiDiagnostics.opened
        onTriggered: Wifi.getWifiStatus()
    }

    background: Item {
        Rectangle {
            id: bg
            width: parent.width
            height: infoContainer.height + 40
            color: colors.black
            anchors.bottom: parent.bottom
        }

        Item {
            id: gradient
            width: parent.width; height: parent.height - bg.height
            anchors { bottom: bg.top; horizontalCenter: parent.horizontalCenter }

            LinearGradient {
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(0, parent.height)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: colors.transparent }
                    GradientStop { position: 1.0; color: colors.black }
                }
            }
        }
    }

    MouseArea {
        anchors { top: parent.top; bottom: infoContainer.top; left: parent.left; right: parent.right }
        onClicked: wifiDiagnostics.close();
    }

    // ---- Helpers ----------------------------------------------------------

    function fmtUptime(secs) {
        if (secs < 0) return qsTr("None since boot");
        var h = Math.floor(secs / 3600);
        var m = Math.floor((secs % 3600) / 60);
        var s = secs % 60;
        var pad = function(n) { return n < 10 ? "0" + n : "" + n; };
        return pad(h) + ":" + pad(m) + ":" + pad(s);
    }

    function fmtSinceDrop(secs) {
        if (secs < 0) return qsTr("None since boot");
        if (secs < 60) return secs + qsTr(" s");
        if (secs < 3600) return Math.floor(secs / 60) + qsTr(" min");
        return Math.floor(secs / 3600) + qsTr(" h ") + Math.floor((secs % 3600) / 60) + qsTr(" min");
    }

    Rectangle {
        id: infoContainer
        width: ui.width
        height: Math.min(infoFlickable.contentHeight + 20, parent.height - 60)
        color: colors.dark
        radius: ui.cornerRadiusSmall
        anchors.bottom: parent.bottom
        clip: true

        // Top-left back arrow — explicit close affordance, mirrors WifiInfo v1.4.15 pattern.
        Components.HapticMouseArea {
            id: backArrowArea
            width: 60; height: 60
            anchors { top: parent.top; left: parent.left; topMargin: 10; leftMargin: 10 }
            z: 10
            onClicked: wifiDiagnostics.close()

            Components.Icon {
                anchors.centerIn: parent
                icon: "uc:arrow-left"
                color: colors.offwhite
                size: 60
            }
        }

        Flickable {
            id: infoFlickable
            anchors { top: parent.top; topMargin: 60; left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: 10 }
            contentHeight: infoColumn.implicitHeight + 20
            contentWidth: width
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            maximumFlickVelocity: 6000
            flickDeceleration: 1000
            clip: true

            ColumnLayout {
                id: infoColumn
                spacing: 20
                width: parent.width - 40
                anchors.horizontalCenter: parent.horizontalCenter

                Item { height: 1 }

                Text {
                    Layout.fillWidth: true
                    color: colors.offwhite
                    text: qsTr("WiFi Diagnostics")
                    font: fonts.primaryFont(30)
                }

                Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width; height: 2; color: colors.medium }

                // ---- Signal strength sparkline ----
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: signalLabel.height + signalValue.height + rssiSparkline.height + 30

                    Text {
                        id: signalLabel
                        anchors { left: parent.left; top: parent.top }
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: qsTr("Signal strength")
                        font: fonts.secondaryFont(24)
                    }
                    Text {
                        id: signalValue
                        anchors { right: parent.right; top: parent.top }
                        color: colors.offwhite
                        text: Wifi.currentRssi + " dBm"
                        font: fonts.secondaryFont(24)
                    }

                    Canvas {
                        id: rssiSparkline
                        anchors { left: parent.left; right: parent.right; top: signalLabel.bottom; topMargin: 20 }
                        height: 80
                        antialiasing: true

                        readonly property int rssiMin: -100
                        readonly property int rssiMax: -30

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);

                            // Reference lines at SignalStrength tier thresholds (-60 / -76 / -84).
                            ctx.strokeStyle = colors.medium;
                            ctx.lineWidth = 1;
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
                            ctx.lineWidth = 2;
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
                }

                Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width; height: 2; color: colors.medium }

                // ---- Link speed ----
                Item {
                    width: parent.width; height: childrenRect.height

                    Text {
                        id: linkSpeedLabel
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: qsTr("Link speed")
                        font: fonts.secondaryFont(24)
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: Wifi.currentLinkSpeed + " Mbps"
                        font: fonts.secondaryFont(24)
                        anchors { top: linkSpeedLabel.bottom }
                    }
                }

                Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width; height: 2; color: colors.medium }

                // ---- Connection stats ----
                Item {
                    width: parent.width; height: childrenRect.height

                    Text {
                        id: dropsLabel
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: qsTr("Drops since boot")
                        font: fonts.secondaryFont(24)
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: Wifi.disconnectCount
                        font: fonts.secondaryFont(24)
                        anchors { top: dropsLabel.bottom }
                    }
                }

                Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width; height: 2; color: colors.medium }

                Item {
                    width: parent.width; height: childrenRect.height

                    Text {
                        id: uptimeLabel
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: qsTr("Current session")
                        font: fonts.secondaryFont(24)
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: Wifi.isConnected ? wifiDiagnostics.fmtUptime(Wifi.currentSessionDurationSec) : qsTr("Disconnected")
                        font: fonts.secondaryFont(24)
                        anchors { top: uptimeLabel.bottom }
                    }
                }

                Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width; height: 2; color: colors.medium }

                Item {
                    width: parent.width; height: childrenRect.height

                    Text {
                        id: lastDropLabel
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: qsTr("Time since last drop")
                        font: fonts.secondaryFont(24)
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: wifiDiagnostics.fmtSinceDrop(Wifi.secondsSinceLastDisconnect)
                        font: fonts.secondaryFont(24)
                        anchors { top: lastDropLabel.bottom }
                    }
                }

                // ---- Reset counters ----
                Components.Button {
                    Layout.fillWidth: true
                    text: qsTr("Reset counters")
                    color: colors.secondaryButton
                    trigger: function() {
                        ui.createActionableWarningNotification(
                            qsTr("Reset diagnostic counters?"),
                            qsTr("Are you sure you want to zero the drop counter and clear the RSSI history?"),
                            "uc:triangle-exclamation",
                            function() { Wifi.resetDiagnosticCounters(); },
                            qsTr("Reset"));
                    }
                }

                Item { height: 1 }
            }
        }
    }
}
