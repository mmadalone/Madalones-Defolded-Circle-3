// Copyright (c) 2026 madalone. ReconnectingHUD overlay (v1.4.19, W2 Wake-replay HUD).
// Top-banner shown while EntityController.resumeWindow is true — the existing 500 ms
// retry-on-failure-during-wake at entityController.cpp:609-650 was already shipping
// commands transparently; this banner just makes the otherwise-silent retry visible.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Entity.Controller 1.0

import "qrc:/components" as Components

Item {
    id: hudRoot
    height: 60

    // Root Item is parent-anchored by main.qml. Internal Rectangle slides in/out.
    // Visible-binding driven entirely by EntityController.resumeWindow:
    //   - Set true on Power.powerModeChanged(LOW_POWER|SUSPEND → NORMAL)
    //   - Set false when the resumeTimeoutWindowSec timer expires
    readonly property bool active: EntityController.resumeWindow

    // Block touches from passing through? No — non-modal banner, taps fall through.
    // The HUD is purely informational; the user's button presses keep landing on
    // the underlying UI as normal (and get queued by m_pendingCommands during
    // the wake gap, retried automatically).

    Rectangle {
        id: banner
        width: parent.width
        height: parent.height
        color: colors.dark
        opacity: 0.95
        y: hudRoot.active ? 0 : -height

        Behavior on y {
            NumberAnimation {
                duration: 250
                easing.type: hudRoot.active ? Easing.OutExpo : Easing.InExpo
            }
        }

        RowLayout {
            anchors {
                fill: parent
                leftMargin: 20
                rightMargin: 20
            }
            spacing: 16

            // Spinning indicator — same pattern as WifiNetworkList.qml:97-110, Discovery.qml:209.
            Image {
                id: spinner
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                source: "qrc:/images/loader_small.png"
                fillMode: Image.PreserveAspectFit
                asynchronous: true

                RotationAnimation on rotation {
                    running: hudRoot.active
                    loops: Animation.Infinite
                    from: 0; to: 360
                    duration: 2000
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                color: colors.offwhite
                text: qsTr("Reconnecting…")
                font: fonts.primaryFont(24)
                elide: Text.ElideRight
            }
        }
    }
}
