// Copyright (c) 2024 madalone. Minimal clock charging screen theme.
// Implements BaseTheme interface — see BaseTheme.qml for contract
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Config 1.0
import ScreensaverConfig 1.0

import "qrc:/components/overlays" as Overlays

Item {
    id: root
    anchors.fill: parent

    // Runtime state (set by ChargingScreen, not config)
    property bool displayOff: false
    property bool isClosing: false

    property bool showBattery: ScreensaverConfig.showBattery
    // Clock is always visible in Minimal theme — it's the entire theme.
    // The global showClock toggle only affects overlays on Matrix/Starfield.
    property bool showClock: true

    function interactiveInput(action) {}

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    // Large centered digital clock
    Text {
        id: timeText
        visible: root.showClock
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -40
        color: colors.offwhite
        font: fonts.primaryFont(96)

        text: {
            var h = ui.time.getHours();
            var m = ui.time.getMinutes();

            if (!Config.clock24h) {
                h = h % 12;
                if (h === 0) h = 12;
            }

            var hStr = h < 10 ? "0" + h : "" + h;
            var mStr = m < 10 ? "0" + m : "" + m;
            return hStr + ":" + mStr;
        }
    }

    // AM/PM indicator for 12h mode
    Text {
        visible: root.showClock && !Config.clock24h
        anchors {
            top: timeText.bottom
            topMargin: 8
            horizontalCenter: parent.horizontalCenter
        }
        color: Qt.rgba(1, 1, 1, 0.5)
        font: fonts.primaryFont(24)
        text: ui.time.getHours() >= 12 ? "PM" : "AM"
    }

    // Date
    Text {
        visible: root.showClock
        anchors {
            bottom: timeText.top
            bottomMargin: 12
            horizontalCenter: parent.horizontalCenter
        }
        color: Qt.rgba(1, 1, 1, 0.4)
        font: fonts.primaryFont(20)
        text: {
            var d = ui.time;
            var days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
            var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
            return days[d.getDay()] + ", " + months[d.getMonth()] + " " + d.getDate();
        }
    }

    // Battery overlay
    Overlays.BatteryOverlay {
        visible: root.showBattery
        anchors {
            bottom: parent.bottom
            bottomMargin: 40
            horizontalCenter: parent.horizontalCenter
        }
    }
}
