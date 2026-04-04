// Copyright (c) 2024 madalone. Custom charging screen overlay.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Config 1.0

Item {
    id: root
    width: parent.width
    height: timeText.height

    property bool use24h: Config.clock24h

    Text {
        id: timeText
        anchors.centerIn: parent
        color: Qt.rgba(1, 1, 1, 0.85)
        font: fonts.primaryFont(48)

        text: {
            var h = ui.time.getHours();
            var m = ui.time.getMinutes();
            var suffix = "";

            if (!root.use24h) {
                suffix = h >= 12 ? " PM" : " AM";
                h = h % 12;
                if (h === 0) h = 12;
            }

            var hStr = h < 10 ? "0" + h : "" + h;
            var mStr = m < 10 ? "0" + m : "" + m;
            return hStr + ":" + mStr + suffix;
        }
    }
}
