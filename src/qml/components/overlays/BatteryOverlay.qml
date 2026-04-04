// Copyright (c) 2024 madalone. Custom charging screen overlay.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Battery 1.0

import "qrc:/components" as Components

Item {
    id: root
    width: childrenRect.width
    height: 70

    // Color based on charge level
    readonly property color batteryColor: {
        var level = Battery.level;
        if (level >= 86) return "#00ff41";      // Full — green
        if (level >= 61) return "#7fff00";      // Good — light green
        if (level >= 31) return "#ffd700";      // Medium — yellow
        if (level >= 16) return "#ff8c00";      // Low — orange
        return "#ff3333";                        // Critical — red
    }

    Components.Icon {
        id: icon
        icon: "uc:bolt"
        color: root.batteryColor
        anchors { left: parent.left; leftMargin: -10 }
        size: 60
    }

    Text {
        color: root.batteryColor
        text: {
            if (Battery.level >= 100 && !Battery.isCharging)
                return "100% - " + qsTr("Fully charged");
            return Battery.level + "%" + (Battery.isCharging ? qsTr(" - Charging") : "");
        }
        anchors { left: icon.right; leftMargin: 10; verticalCenter: icon.verticalCenter }
        font: fonts.primaryFont(24)
    }
}
