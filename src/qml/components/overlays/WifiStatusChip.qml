// Copyright (c) 2026 madalone. Compact WiFi signal-strength chip for entity detail pages
// (v1.4.21, sibling to BatteryStatusChip). Renders the same icon family as the home-screen
// StatusBar's always-visible WiFi indicator (Mod 4 W3) so detail pages get parity. When
// enabled, replaces the existing BaseDetail.qml WiFi-warning icon (which only surfaces on
// sub-optimal signal) — chip carries the full info including disconnected red-X state.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Wifi 1.0
import Wifi.SignalStrength 1.0

import "qrc:/components" as Components

Item {
    id: root
    implicitWidth: 40
    implicitHeight: 40

    // Base wifi icon (always visible — neutral chrome glyph, signal level overlaid by inner Icon).
    Components.Icon {
        objectName: "wifiBase"   // v1.4.34: testability annotation
        anchors.centerIn: parent
        size: 40
        icon: "uc:wifi"
        color: colors.offwhite
        opacity: 1.0

        // Signal-strength overlay icon (uc:wifi-01/02/03 per tier). Empty when disconnected
        // or NONE — fades to opacity 0 to reveal the base "uc:wifi" silhouette underneath.
        Components.Icon {
            objectName: "wifiOverlay"   // v1.4.34: testability annotation
            anchors.centerIn: parent
            size: 40
            icon: {
                if (!Wifi.isConnected) return "";
                switch (Wifi.currentNetwork.signalStrength) {
                case SignalStrength.NONE:      return "";
                case SignalStrength.WEAK:      return "uc:wifi-01";
                case SignalStrength.OK:
                case SignalStrength.GOOD:      return "uc:wifi-02";
                case SignalStrength.EXCELLENT: return "uc:wifi-03";
                default:                       return "";
                }
            }
            color: colors.offwhite
            opacity: icon === "" ? 0 : 1
        }

        // Red strikethrough when disconnected (matches StatusBar pattern at lines 278-286).
        Rectangle {
            objectName: "wifiStrikethrough"   // v1.4.34: testability annotation
            anchors.centerIn: parent
            width: 30
            height: 2
            color: colors.red
            rotation: -45
            transformOrigin: Item.Center
            visible: !Wifi.isConnected
        }
    }
}
