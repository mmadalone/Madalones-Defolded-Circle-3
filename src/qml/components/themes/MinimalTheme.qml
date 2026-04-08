// Copyright (c) 2024 madalone. Minimal clock charging screen theme.
// Implements BaseTheme interface — see BaseTheme.qml for contract
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

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
    property bool showClock: true

    function interactiveInput(action) {}

    // Font selector: "primary" = Poppins (sans), "secondary" = Space Mono (mono)
    function themeFont(size) {
        return ScreensaverConfig.minimalFont === "secondary"
            ? fonts.secondaryFont(size) : fonts.primaryFont(size);
    }

    function isGradient(v) {
        return v === "rainbow" || v === "rainbow_gradient" || v === "neon";
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    // Large centered digital clock
    Overlays.GradientText {
        id: timeText
        visible: root.showClock
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -40
        colorValue: ScreensaverConfig.minimalTimeColor
        font: root.themeFont(ScreensaverConfig.minimalClockSize)

        text: {
            var h = ui.time.getHours();
            var m = ui.time.getMinutes();

            if (!ScreensaverConfig.minimalClock24h) {
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
        visible: root.showClock && !ScreensaverConfig.minimalClock24h
        anchors {
            top: timeText.bottom
            topMargin: 8
            horizontalCenter: parent.horizontalCenter
        }
        // For gradient modes, use white at 50% opacity; for solid, use the time color at 50%
        color: root.isGradient(ScreensaverConfig.minimalTimeColor)
            ? Qt.rgba(1, 1, 1, 0.5)
            : Qt.rgba(Qt.lighter(ScreensaverConfig.minimalTimeColor, 1.0).r,
                       Qt.lighter(ScreensaverConfig.minimalTimeColor, 1.0).g,
                       Qt.lighter(ScreensaverConfig.minimalTimeColor, 1.0).b, 0.5)
        font: root.themeFont(Math.round(ScreensaverConfig.minimalClockSize * 0.25))
        text: ui.time.getHours() >= 12 ? "PM" : "AM"
    }

    // Date
    Overlays.GradientText {
        visible: root.showClock
        anchors {
            bottom: timeText.top
            bottomMargin: 12
            horizontalCenter: parent.horizontalCenter
        }
        colorValue: ScreensaverConfig.minimalDateColor
        font: root.themeFont(ScreensaverConfig.minimalDateSize)
        text: {
            // ui.time is QTime (no date). Use JS Date() for the date portion.
            void(ui.time);  // trigger rebinding on time change
            var d = new Date();
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
