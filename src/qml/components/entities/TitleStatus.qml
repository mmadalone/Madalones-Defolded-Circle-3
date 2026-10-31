// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Mod 3: connecting / core-disconnected / update indicators, always-on WiFi chip,
// WiFi warning only when disconnected or without signal.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Wifi 1.0
import Wifi.SignalStrength 1.0
import Battery 1.0
import Config 1.0
import SoftwareUpdate 1.0

import "qrc:/components" as Components

// The status icons at the right of an entity screen's title, in one row so that they line up and never
// cover each other: the integration of the entity is not connected, the WiFi is down or weak, the
// battery (with "show battery everywhere"), and the spinner while a command for the entity runs. The
// host anchors the row to the right, clear of its close icon.
Row {
    id: titleStatus
    spacing: 10

    // the entity's integration reports a state other than connected
    property bool integrationDisconnected: false
    // a command for the entity is in progress or retried after a wake-up
    property bool commandInProgress: false

    // madalone (Mod 3): integrations connecting (ui.isConnecting), same animation as the status bar's
    Item {
        id: integrationLoadingIndicator
        width: 40
        height: 40
        anchors.verticalCenter: parent.verticalCenter
        visible: ui.isConnecting

        property int circleSize: 14

        Rectangle {
            id: fillCircle
            width: integrationLoadingIndicator.circleSize; height: integrationLoadingIndicator.circleSize
            radius: 7
            color: colors.offwhite
            x: 0
            z: 1
            anchors.verticalCenter: parent.verticalCenter
        }

        Rectangle {
            id: outlineCircle
            width: integrationLoadingIndicator.circleSize; height: integrationLoadingIndicator.circleSize
            radius: integrationLoadingIndicator.circleSize/2
            color: colors.offwhite
            opacity: 0.3
            x: 20
            z: 2
            anchors.verticalCenter: parent.verticalCenter
        }

        SequentialAnimation {
            running: integrationLoadingIndicator.visible
            loops: Animation.Infinite

            ParallelAnimation {
                NumberAnimation { target: fillCircle; properties: "z"; to: 1; duration: 1 }
                NumberAnimation { target: outlineCircle; properties: "z"; to: 2; duration: 1 }
            }

            ParallelAnimation {
                NumberAnimation { target: fillCircle; properties: "x"; to: integrationLoadingIndicator.circleSize; easing.type: Easing.OutExpo; duration: 400 }
                NumberAnimation { target: outlineCircle; properties: "x"; to: 0; easing.type: Easing.OutExpo; duration: 400 }
            }

            PauseAnimation { duration: 500 }

            ParallelAnimation {
                NumberAnimation { target: fillCircle; properties: "z"; to: 2; duration: 1 }
                NumberAnimation { target: outlineCircle; properties: "z"; to: 1; duration: 1 }
            }

            ParallelAnimation {
                NumberAnimation { target: fillCircle; properties: "x"; to: 0; easing.type: Easing.OutExpo; duration: 400 }
                NumberAnimation { target: outlineCircle; properties: "x"; to: integrationLoadingIndicator.circleSize; easing.type: Easing.OutExpo; duration: 400 }
            }

            PauseAnimation { duration: 500 }
        }
    }

    // madalone (Mod 3): the remote lost its connection to the core
    Rectangle {
        width: 12; height: 12
        radius: 6
        color: colors.red
        anchors.verticalCenter: parent.verticalCenter
        visible: !ui.coreConnected
    }

    // madalone (Mod 3): a software update is available
    Components.Icon {
        icon: "uc:cloud-arrow-down"
        size: 60
        color: colors.yellow
        anchors.verticalCenter: parent.verticalCenter
        visible: SoftwareUpdate.updateAvailable
    }

    Components.Icon {
        icon: "uc:link-slash"
        color: colors.red
        size: 40
        anchors.verticalCenter: parent.verticalCenter
        visible: titleStatus.integrationDisconnected
    }

    Components.Icon {
        icon: "uc:wifi"
        color: colors.offwhite
        opacity: 0.5
        size: 60
        anchors.verticalCenter: parent.verticalCenter
        // madalone (v1.4.0/v1.4.21): no warning for WEAK, which the Remote 3 reports for good signals; hidden while
        // the always-on WiFi chip carries the state
        visible: !Config.showWifiEveryWhere && (!Wifi.isConnected || Wifi.currentNetwork.signalStrength === SignalStrength.NONE)

        Components.Icon {
            size: 60
            icon: {
                switch (Wifi.currentNetwork.signalStrength) {
                case SignalStrength.NONE:
                    return "";
                case SignalStrength.WEAK:
                    return "uc:wifi-weak";
                default:
                    return "";
                }
            }
            opacity: icon === "" ? 0 : 1
            anchors.centerIn: parent
        }

        Rectangle {
            width: 30
            height: 2
            color: colors.red
            rotation: -45
            transformOrigin: Item.Center
            anchors.centerIn: parent
            visible: !Wifi.isConnected
        }
    }

    // madalone (Mod 3, v1.4.21): WiFi indicator on every entity screen ("Show WiFi indicator everywhere")
    Loader {
        active: Config.showWifiEveryWhere
        visible: active
        anchors.verticalCenter: parent.verticalCenter
        source: "qrc:/components/overlays/WifiStatusChip.qml"
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 5
        visible: Config.showBatteryEveryWhere

        Text {
            anchors.verticalCenter: parent.verticalCenter
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
            width: 16
            height: 30
            anchors.verticalCenter: parent.verticalCenter
            visible: !Battery.isCharging

            Rectangle {
                width: parent.width
                height: (parent.height * Battery.level / 100) + (Battery.level < 10 ? 2 : 0)
                radius: 4
                color: Battery.low ? colors.red : colors.offwhite
                opacity: 0.8
                anchors { horizontalCenter: batteryBg.horizontalCenter; bottom: batteryBg.bottom; bottomMargin: 1 }
            }

            Rectangle {
                id: batteryBg
                width: parent.width
                height: parent.height
                radius: 4
                color: colors.offwhite
                opacity: 0.3
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
            }
        }
    }

    Image {
        id: commandLoadingIndicator
        width: 32; height: 32
        anchors.verticalCenter: parent.verticalCenter
        source: "qrc:/images/loader_small.png"
        fillMode: Image.PreserveAspectFit
        visible: titleStatus.commandInProgress

        RotationAnimation on rotation {
            running: commandLoadingIndicator.visible
            loops: Animation.Infinite
            from: 0; to: 360
            duration: 1200
        }
    }
}
