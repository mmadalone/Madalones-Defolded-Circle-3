// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. WiFi UX bundle: live link diagnostics + reassociate button + top-left back arrow.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import Wifi 1.0

import "qrc:/components" as Components

Popup {
    id: wifiInfo
    width: parent.width; height: parent.height
    y: 500
    opacity: 0
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    enter: Transition {
        SequentialAnimation {
            ParallelAnimation {
                PropertyAnimation { properties: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { properties: "y"; from: 500; to: 0; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    }

    exit: Transition {
        SequentialAnimation {
            PropertyAnimation { properties: "y"; from: 0; to: 500; easing.type: Easing.InExpo; duration: 300 }
            PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0 }
        }
    }

    function showWifiInfo(id, ssid, macAddress, ipAddress) {
        wifiInfo.wifiNetworkId = id;
        wifiInfo.ssid = ssid;
        wifiInfo.macAddress = macAddress;
        wifiInfo.ipAddress = ipAddress;
        wifiInfo.open();
    }

    property string parentController
    property string wifiNetworkId
    property string ssid
    property string macAddress
    property string ipAddress

    onOpened: {
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();
        wifiInfo.ssid = "";
        wifiInfo.macAddress = "";
        wifiInfo.ipAddress = "";
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    wifiInfo.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    wifiInfo.close();
                }
            }
        }
    }

    background: Item {
        Rectangle {
            id: bg
            width: parent.width
            height: infoContainer.height + 40
            color: colors.black
            anchors.bottom: parent.bottom
        }

        Item {
            id: gradient
            width: parent.width; height: parent.height - bg.height
            anchors { bottom: bg.top; horizontalCenter: parent.horizontalCenter }

            LinearGradient {
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(0, parent.height)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: colors.transparent }
                    GradientStop { position: 1.0; color: colors.black }
                }
            }
        }
    }

    MouseArea {
        anchors { top: parent.top; bottom: infoContainer.top; left: parent.left; right: parent.right }
        onClicked: wifiInfo.close();
    }

    Rectangle {
        id: infoContainer
        width: ui.width
        // madalone (v1.4.15b): cap at viewport-60. Action buttons now live INSIDE the Flickable
        // at the bottom of scrollable content (per user feedback) — no separate pinned buttons block.
        height: Math.min(infoFlickable.contentHeight + 20, parent.height - 60)
        color: colors.dark
        radius: ui.cornerRadiusSmall
        anchors.bottom: parent.bottom
        clip: true

        // madalone (v1.4.15): top-left back arrow — explicit close affordance.
        // This popup is conceptually a sub-section of WiFi settings, not a modal,
        // so a back arrow matches the navigation pattern better than a modal X.
        Components.HapticMouseArea {
            id: backArrowArea
            width: 60; height: 60
            anchors { top: parent.top; left: parent.left; topMargin: 10; leftMargin: 10 }
            z: 10
            onClicked: wifiInfo.close()

            Components.Icon {
                anchors.centerIn: parent
                icon: "uc:arrow-left"
                color: colors.offwhite
                size: 60
            }
        }

        // madalone (v1.4.15b): Flickable now holds diagnostic rows AND action buttons —
        // buttons are at the bottom of scrollable content (user scrolls to find them).
        Flickable {
            id: infoFlickable
            anchors { top: parent.top; topMargin: 60; left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: 10 }
            contentHeight: infoColumn.implicitHeight + 20
            contentWidth: width
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            maximumFlickVelocity: 6000
            flickDeceleration: 1000
            clip: true

            ColumnLayout {
                id: infoColumn
                spacing: 20
                width: parent.width - 40
                anchors.horizontalCenter: parent.horizontalCenter

            Item {
                height: 1
            }

            Text {
                id: currentNetworkSSID
                Layout.alignment: Qt.AlignLeft
                width: parent.width
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.offwhite
                text: wifiInfo.ssid == "" ? Wifi.currentNetwork.id : wifiInfo.ssid
                font: fonts.primaryFont(30)
            }

            Item {
                Layout.topMargin: -20
                Layout.leftMargin: -10
                Layout.preferredHeight: 30

                Components.Icon {
                    id: currentNetworkConnectedIcon
                    icon: Wifi.isConnected ? "uc:check" : "uc:xmark"
                    size: 60
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    width: parent.width
                    color: colors.offwhite
                    text: Wifi.currentNetwork.frequency < 5000 ? "2.4 GHz" : "5 GHz"
                    font: fonts.secondaryFont(24)
                    anchors { left: currentNetworkConnectedIcon.right; verticalCenter: currentNetworkConnectedIcon.verticalCenter }
                }
            }

            // madalone: live link diagnostics from WifiStatus
            Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width; height: 2; color: colors.medium }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: signalLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Signal")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: Wifi.currentRssi + " dBm" + (Wifi.currentSnr > 0 ? "  (SNR " + Wifi.currentSnr + " dB)" : "")
                    font: fonts.secondaryFont(24)
                    anchors { top: signalLabel.bottom }
                }
            }

            Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width; height: 2; color: colors.medium }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: linkSpeedLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Link speed")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: Wifi.currentLinkSpeed + " Mbps"
                    font: fonts.secondaryFont(24)
                    anchors { top: linkSpeedLabel.bottom }
                }
            }

            // madalone: hide Throughput row when firmware doesn't populate est_throughput (always 0 on observed UCR3 WS responses)
            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width; height: visible ? 2 : 0
                color: colors.medium
                visible: Wifi.currentEstimatedThroughput > 0
            }

            Item {
                width: parent.width
                height: visible ? childrenRect.height : 0
                visible: Wifi.currentEstimatedThroughput > 0

                Text {
                    id: throughputLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Throughput")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: Wifi.currentEstimatedThroughput + " Mbps"
                    font: fonts.secondaryFont(24)
                    anchors { top: throughputLabel.bottom }
                }
            }

            Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width; height: 2; color: colors.medium }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: bssidLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("BSSID")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: Wifi.currentBssid
                    font: fonts.secondaryFont(24)
                    anchors { top: bssidLabel.bottom }
                }
            }

            Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width; height: 2; color: colors.medium }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: channelLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Channel")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: {
                        var f = Wifi.currentNetwork.frequency;
                        if (f === 2484) return "14";
                        if (f >= 2412 && f <= 2472) return ((f - 2412) / 5 + 1).toString();
                        if (f >= 5000) return ((f - 5000) / 5).toString();
                        return "—";
                    }
                    font: fonts.secondaryFont(24)
                    anchors { top: channelLabel.bottom }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width; height: 2
                color: colors.medium
            }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: macAddressLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("MAC address")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: wifiInfo.macAddress
                    font: fonts.secondaryFont(24)
                    anchors { top: macAddressLabel.bottom }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width; height: 2
                color: colors.medium
            }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: ipAddressLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("IP address")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: wifiInfo.ipAddress
                    font: fonts.secondaryFont(24)
                    anchors { top: ipAddressLabel.bottom }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width; height: 2
                color: colors.medium
            }

            Item {
                width: parent.width
                height: childrenRect.height

                Text {
                    id: keyManagementLabel
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Key management")
                    font: fonts.secondaryFont(24)
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: Wifi.currentNetwork.keyManagement
                    font: fonts.secondaryFont(24)
                    anchors { top: keyManagementLabel.bottom }
                }
            }

            // madalone (v1.4.15b): action buttons folded back into Flickable per user feedback.
            // User scrolls through diagnostics to reach them at the bottom.
            Components.Button {
                Layout.fillWidth: true
                text: Wifi.isConnected ? qsTr("Disconnect") : qsTr("Connect")
                trigger: function() {
                    if (Wifi.isConnected) {
                        Wifi.disconnect();
                    } else {
                        Wifi.connectSavedNetwork(wifiInfo.wifiNetworkId);
                    }
                    wifiInfo.close();
                    ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                }
            }

            // REASSOCIATE — re-do 4-way handshake without full deauth
            Components.Button {
                Layout.fillWidth: true
                text: qsTr("Reconnect")
                visible: Wifi.isConnected
                trigger: function() {
                    loading.start();
                    Wifi.reassociate();
                    ui.setTimeOut(2500, ()=>{ loading.success(); });
                }
            }

            Components.Button {
                Layout.fillWidth: true
                text: qsTr("Delete")
                color: colors.red
                trigger: function() {
                    Wifi.deleteSavedNetwork(wifiInfo.ssid);
                    wifiInfo.close();
                    ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                }
            }

            Item {
                height: 1
            }
            }   // ColumnLayout (infoColumn)
        }       // Flickable (infoFlickable)
    }           // Rectangle (infoContainer)
}               // Popup (wifiInfo)
