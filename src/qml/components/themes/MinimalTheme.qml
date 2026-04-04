// Copyright (c) 2024 madalone. Minimal clock charging screen theme.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Config 1.0

import "qrc:/components/overlays" as Overlays

Item {
    id: root
    anchors.fill: parent

    property bool showBattery: true

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    // Large centered digital clock
    Text {
        id: timeText
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
        visible: !Config.clock24h
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
