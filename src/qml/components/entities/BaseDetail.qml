// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Entity.Controller 1.0
import Haptic 1.0

import Config 1.0

import Wifi 1.0
import Wifi.SignalStrength 1.0
import SoftwareUpdate 1.0

import Integration.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: entityBaseDetailContainer
    color: colors.black
    width: parent.width
    height: parent.height

    signal closed()

    state: "closed"

    states: [
        State {
            name: "open"
            PropertyChanges {target: iconClose; opacity: 1 }
            PropertyChanges {target: entityBaseDetailContainer; y: 0 }
        },
        State {
            name: "closed"
            PropertyChanges {target: iconClose; opacity: 0 }
            PropertyChanges {target: entityBaseDetailContainer; y: entityBaseDetailContainer.height }
        }
    ]

    transitions: [
        Transition {
            to: "open"
            SequentialAnimation {
                ParallelAnimation {
                    PropertyAction { target: containerMain.item; property: "state"; value: "hidden" }
                    PropertyAnimation { target: entityBaseDetailContainer; properties: "y"; easing.type: Easing.OutExpo; duration: entityBaseDetailContainer.skipAnimation ? 0 : 300 }
                }
                PropertyAnimation { target: iconClose; properties: "opacity"; easing.type: Easing.OutExpo; duration: entityBaseDetailContainer.skipAnimation ? 0 : 300 }
                PauseAnimation { duration: 500 }
            }
        },
        Transition {
            to: "closed"
            SequentialAnimation {
                PauseAnimation { duration: entityBaseDetailContainer.skipAnimation ? 300 : 0 }
                ParallelAnimation {
                    PropertyAnimation { target: entityBaseDetailContainer; properties: "y"; easing.type: Easing.InExpo; duration: 300 }
                    PropertyAnimation { target: iconClose; properties: "opacity"; easing.type: Easing.InExpo; duration: 200 }
                    PropertyAction { target: containerMain.item; property: "state"; value: entityBaseDetailContainer.skipAnimation ? "hidden" : "visible" }
                }
            }

            onRunningChanged: {
                if ((state == "closed") && (!running))
                    entityBaseDetailContainer.closed();
            }
        }
    ]

    property string entityId
    property QtObject entityObj
    property QtObject integrationObj: QtObject {
        property string state
    }

    property bool skipAnimation: false
    property var overrideConfig: ([])

    property alias iconClose: iconClose
    property alias buttonNavigation: buttonNavigation

    // WiFi warning predicate aligned with StatusBar.qml:252 (the home-screen
    // reference). Drops the WEAK condition that earlier inherited from the
    // pre-Mod-3 detail-page implementations — UC3's embedded WiFi reports
    // WEAK for signals that are subjectively fine, producing a permanently-
    // visible warning even when the home StatusBar correctly hides it.
    // Now: detail pages and home screen behave identically (warn only on
    // disconnect or NONE signal).
    readonly property bool _wifiWarningActive:
        !Wifi.isConnected
        || Wifi.currentNetwork.signalStrength === SignalStrength.NONE

    function open(skipAnimation = false) {
        // get the latest entity data from the core
        EntityController.refreshEntity(entityId);

        buttonNavigation.takeControl();
        entityBaseDetailContainer.skipAnimation = skipAnimation;
        entityBaseDetailContainer.state = "open";
    }

    function close() {
        entityBaseDetailContainer.state = "closed";
//        buttonNavigation.releaseControl();

        if (entityBaseDetailContainer.skipAnimation) {
            entityBaseDetailContainer.closed();
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        ignoreInput: entityObj.state == 0
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    if (entityBaseDetailContainer.state == "open") {
                        entityBaseDetailContainer.close();
                    }
                },
                "long_press": function() {
                    if (entityBaseDetailContainer.state == "open") {
                        entityBaseDetailContainer.close();
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    if (entityBaseDetailContainer.state == "open") {
                        entityBaseDetailContainer.close();
                    }
                },
                "long_press": function() {
                    if (entityBaseDetailContainer.state == "open") {
                        entityBaseDetailContainer.close();
                    }
                }
            }
        }
        overrideConfig: entityBaseDetailContainer.overrideConfig
    }

    Components.Icon {
        id: iconClose
        color: colors.offwhite
        opacity: 0
        icon: "uc:xmark"
        anchors { right: parent.right; top: parent.top; topMargin: 5 }
        size: 70
        z: 1000

        Components.HapticMouseArea {
            width: parent.width + 20; height: parent.height + 20
            anchors.centerIn: parent
            enabled: entityBaseDetailContainer.state == "open"
            onClicked: {
                entityBaseDetailContainer.close();
            }
        }
    }

    RowLayout {
        id: titleStatusStrip
        anchors {
            right: iconClose.left
            rightMargin: 10
            verticalCenter: iconClose.verticalCenter
        }
        spacing: 5
        z: 1001

        // 1. Integration loading spinner (ui.isConnecting)
        Item {
            id: integrationLoadingIndicator

            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignVCenter

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

        // 2. Core-disconnected dot (!ui.coreConnected)
        Rectangle {
            Layout.alignment: Qt.AlignVCenter

            width: 12; height: 12
            radius: 6
            color: colors.red
            visible: !ui.coreConnected
        }

        // 3. Software update indicator
        Components.Icon {
            Layout.leftMargin: -10
            Layout.rightMargin: -10

            icon: "uc:cloud-arrow-down"
            size: 60
            color: colors.yellow
            visible: SoftwareUpdate.updateAvailable
        }

        // 4. WiFi warning (detail-page wider predicate: NONE || WEAK || disconnected)
        Components.Icon {
            id: iconWifiWarning

            Layout.leftMargin: -10
            Layout.rightMargin: -10

            icon: "uc:wifi"
            color: colors.offwhite
            opacity: 0.5
            size: 60
            visible: entityBaseDetailContainer._wifiWarningActive

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

        // 5. Integration-disconnected (per-entity)
        Components.Icon {
            id: iconIntegrationDisconnected

            Layout.preferredWidth: visible ? 40 : 0
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignVCenter

            icon: "uc:link-slash"
            color: colors.red
            size: 40
            visible: integrationObj.state != "connected" && integrationObj.state != ""
        }

        // 6. Battery chip (rightmost, fixed anchor)
        Loader {
            id: batteryChipLoader

            Layout.preferredWidth: (active && item) ? item.implicitWidth : 0
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignVCenter

            active: Config.showBatteryOnDetailPages
            source: "qrc:/components/overlays/BatteryStatusChip.qml"
        }
    }

    Rectangle {
        id: unavailableOverlay
        color: colors.black
        opacity: entityObj.state == 0 ? 0.85 : 0
        anchors { top: iconClose.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
        z: 2000

        onOpacityChanged: {
            if (unavailableOverlay.opacity == 0) {
                showUnavailableIconTimer.stop();
                unavailableOverlayIcon.visible = false;
            } else {
                showUnavailableIconTimer.start();
            }
        }

        Timer {
            id: showUnavailableIconTimer
            repeat: false
            running: false
            interval: 1000

            onTriggered: {
                unavailableOverlayIcon.visible = true;
            }
        }

        MouseArea {
            enabled: unavailableOverlay.opacity != 0
            anchors.fill: parent
        }

        Components.Icon {
            id: unavailableOverlayIcon
            color: colors.red
            anchors.centerIn: parent
            icon: "uc:ban"
            size: 120
            visible: false
        }

        Text {
            visible: unavailableOverlayIcon.visible
            text: qsTr("Entity unavailable")
            width: parent.width - 40
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: 2
            elide: Text.ElideRight
            color: colors.red
            font: fonts.primaryFont(30)
            lineHeight: 0.8
            anchors { horizontalCenter: parent.horizontalCenter; top: unavailableOverlayIcon.bottom; topMargin: 20 }
        }
    }
}
