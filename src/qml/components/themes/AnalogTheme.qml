// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Extracted from stock ChargingScreen as a theme option.
// Implements BaseTheme interface — see BaseTheme.qml for contract.
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

    function interactiveInput(action) {}

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    // Analog clock — extracted from UC stock ChargingScreen
    Item {
        id: clock
        width: parent.width - 80; height: width
        anchors.centerIn: parent

        property int hours: ui.time.getHours()
        property int minutes: ui.time.getMinutes()
        property int seconds: ui.time.getSeconds()

        // Hour dots (12 positions, cardinal dots brighter)
        Repeater {
            model: 12

            Item {
                height: parent.height / 2
                transformOrigin: Item.Bottom
                rotation: index * 30
                x: parent.width / 2
                y: 0

                Rectangle {
                    width: 12; height: 12; radius: 6
                    color: colors.offwhite
                    opacity: index === 0 || index === 3 || index === 6 || index === 9 ? 1 : 0.6
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 4 }
                }
            }
        }

        // Second hand
        Item {
            id: secondHand
            anchors { top: parent.top; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
            rotation: 6 * clock.seconds
            antialiasing: true

            Rectangle {
                width: 1; height: clock.width / 2 - 20
                color: colors.red
                anchors.horizontalCenter: parent.horizontalCenter
                y: parent.height * 0.05
                antialiasing: true
            }
        }

        // Minute hand
        Item {
            id: minuteHand
            anchors { top: parent.top; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
            rotation: 6 * clock.minutes
            antialiasing: true

            Rectangle {
                width: 4; height: clock.width / 2 - 40
                color: colors.offwhite
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.verticalCenter }
                antialiasing: true
            }
        }

        // Hour hand
        Item {
            id: hourHand
            anchors { top: parent.top; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
            rotation: 30 * (clock.hours % 12) + 0.5 * clock.minutes
            antialiasing: true

            Rectangle {
                width: 4; height: clock.width / 2 - 80
                color: colors.offwhite
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.verticalCenter }
                antialiasing: true
            }
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
