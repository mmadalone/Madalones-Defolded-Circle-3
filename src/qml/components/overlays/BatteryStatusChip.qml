// Copyright (c) 2026 madalone. Compact battery chip for entity detail pages.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Config 1.0
import Battery 1.0

import "qrc:/components" as Components

Item {
    id: root
    implicitWidth: batteryRow.implicitWidth
    implicitHeight: 40

    RowLayout {
        id: batteryRow
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0

        Text {
            Layout.alignment: Qt.AlignVCenter

            color: colors.offwhite
            text: Battery.level
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font: fonts.primaryFontCapitalized(22)
            visible: Battery.isCharging || Config.showBatteryPercentage
        }

        Components.Icon {
            icon: "uc:bolt"
            color: colors.offwhite
            size: 40
            visible: Battery.isCharging
        }

        Item {
            Layout.leftMargin: 5
            Layout.preferredWidth: 16
            Layout.preferredHeight: 30
            Layout.alignment: Qt.AlignVCenter

            visible: !Battery.isCharging

            Rectangle {
                width: parent.width
                height: (parent.height * Battery.level / 100) + (Battery.level < 10 ? 2 : 0)
                radius: 4
                color: Battery.low ? colors.red : colors.offwhite
                opacity: 0.8
                anchors { horizontalCenter: batteryBottom.horizontalCenter; bottom: batteryBottom.bottom; bottomMargin: 1 }
            }

            Rectangle {
                id: batteryBottom
                width: parent.width; height: parent.height
                radius: 4
                color: colors.offwhite
                opacity: 0.3
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
            }
        }
    }
}
