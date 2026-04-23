// Copyright (c) 2026 madalone. Custom charging screen overlay.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Battery 1.0
import ScreensaverConfig 1.0
import Palettes 1.0

import "qrc:/components" as Components

Item {
    id: root
    width: childrenRect.width
    height: 70

    // Battery-level color tier (5 thresholds: full / good / medium / low / critical).
    // Tier mapping centralised in the Palettes singleton.
    readonly property color batteryColor: Palettes.batteryColor(Battery.level)

    Components.Icon {
        id: icon
        icon: "uc:bolt"
        color: root.batteryColor
        anchors { left: parent.left; leftMargin: -10 }
        size: ScreensaverConfig.batteryTextSize * 2.5
    }

    Text {
        color: root.batteryColor
        text: {
            // Fully translatable strings (lupdate picks these up via qsTr).
            // Percentage is injected via %1 placeholder so translators can
            // adjust word ordering / punctuation freely.
            if (Battery.level >= 100 && !Battery.isCharging)
                return qsTr("100% - Fully charged");
            if (Battery.isCharging)
                return qsTr("%1% - Charging").arg(Battery.level);
            return qsTr("%1%").arg(Battery.level);
        }
        anchors { left: icon.right; leftMargin: 10; verticalCenter: icon.verticalCenter }
        font: fonts.primaryFont(ScreensaverConfig.batteryTextSize)
    }
}
