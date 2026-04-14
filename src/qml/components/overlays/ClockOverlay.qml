// Copyright (c) 2026 madalone. Custom charging screen clock overlay.
// Supports solid color and rainbow gradient via GradientText component.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import ScreensaverConfig 1.0

Item {
    id: root
    width: parent.width
    height: dateText.visible ? (dateText.height + 12 + timeText.height + (ampmText.visible ? ampmText.height + 8 : 0))
                             : timeText.height + (ampmText.visible ? ampmText.height + 8 : 0)

    // Cached date string. The date only changes once per day, but the old
    // implementation rebuilt the string on every 1Hz `ui.time` tick via
    // `void(ui.time)` -- ~864k recomputes/day. The Timer below refreshes once
    // per minute (1440 fires/day, 98% fewer) with `triggeredOnStart: true` so
    // the string is populated as soon as the overlay becomes visible. Accuracy
    // is within ~1 minute, which is fine for a date display.
    property string _cachedDateString: ""

    Timer {
        running: dateText.visible
        interval: 60000
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            var d = new Date();
            var days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
            var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
            root._cachedDateString = days[d.getDay()] + ", " + months[d.getMonth()] + " " + d.getDate();
        }
    }

    // Font selector: "primary" = Poppins (sans), "secondary" = Space Mono (mono)
    function clockFont(size) {
        return ScreensaverConfig.clockFont === "secondary"
            ? fonts.secondaryFont(size) : fonts.primaryFont(size);
    }

    function isGradient(v) {
        return v === "rainbow" || v === "rainbow_gradient" || v === "neon";
    }

    // Date line (above time)
    GradientText {
        id: dateText
        visible: ScreensaverConfig.clockShowDate
        anchors {
            bottom: timeText.top
            bottomMargin: 12
            horizontalCenter: parent.horizontalCenter
        }
        colorValue: ScreensaverConfig.clockDateColor
        font: root.clockFont(ScreensaverConfig.clockDateSize)
        text: root._cachedDateString
    }

    // Time
    GradientText {
        id: timeText
        anchors.horizontalCenter: parent.horizontalCenter
        colorValue: ScreensaverConfig.clockColor
        font: root.clockFont(ScreensaverConfig.clockSize)

        text: {
            var h = ui.time.getHours();
            var m = ui.time.getMinutes();

            if (!ScreensaverConfig.clockClock24h) {
                h = h % 12;
                if (h === 0) h = 12;
            }

            var hStr = h < 10 ? "0" + h : "" + h;
            var mStr = m < 10 ? "0" + m : "" + m;
            return hStr + ":" + mStr;
        }
    }

    // AM/PM indicator (below time, only in 12h mode)
    Text {
        id: ampmText
        visible: !ScreensaverConfig.clockClock24h
        anchors {
            top: timeText.bottom
            topMargin: 8
            horizontalCenter: parent.horizontalCenter
        }
        color: root.isGradient(ScreensaverConfig.clockColor)
            ? Qt.rgba(1, 1, 1, 0.5)
            : ScreensaverConfig.clockColor
        opacity: root.isGradient(ScreensaverConfig.clockColor) ? 1.0 : 0.5
        font: root.clockFont(Math.round(ScreensaverConfig.clockSize * 0.25))
        text: ui.time.getHours() >= 12 ? "PM" : "AM"
    }
}
