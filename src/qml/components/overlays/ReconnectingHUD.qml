// Copyright (c) 2026 madalone. ReconnectingHUD overlay (v1.4.19 W2 Wake-replay HUD,
// v1.4.21 prominence overhaul). Top-banner shown while EntityController.resumeWindow is
// true — the existing 500 ms retry-on-failure-during-wake at entityController.cpp:609-650
// was already shipping commands transparently; this banner makes the otherwise-silent
// retry visible.
//
// v1.4.21: bumped from 60 px → 120 px height, font 24 → 32, spinner 36 → 56,
// `colors.dark` → `colors.medium` (one tier brighter for active-state contrast), added
// subtle 3-second pulse animation. User reported the v1.4.19 incarnation was easy to miss
// during the 1-3 s real-world resume window. Z stays 9998 — screensaver MouseArea at 9999
// still suppresses the HUD when active (intentional).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Entity.Controller 1.0

import "qrc:/components" as Components

Item {
    id: hudRoot
    height: 120

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
        color: colors.medium
        y: hudRoot.active ? 0 : -height

        Behavior on y {
            NumberAnimation {
                duration: 250
                easing.type: hudRoot.active ? Easing.OutExpo : Easing.InExpo
            }
        }

        // Subtle pulse on opacity while active. 3-second cycle (1500 ms in / 1500 ms out)
        // is calm but visibly alive. Stops when banner slides out — saves cycles when idle.
        SequentialAnimation on opacity {
            id: pulseAnimation
            running: hudRoot.active
            loops: Animation.Infinite
            NumberAnimation { from: 0.95; to: 1.0; duration: 1500; easing.type: Easing.InOutSine }
            NumberAnimation { from: 1.0; to: 0.95; duration: 1500; easing.type: Easing.InOutSine }
        }

        RowLayout {
            anchors {
                fill: parent
                leftMargin: 30
                rightMargin: 30
            }
            spacing: 24

            // Spinning indicator — same pattern as WifiNetworkList.qml:97-110, Discovery.qml:209.
            Image {
                id: spinner
                Layout.preferredWidth: 56
                Layout.preferredHeight: 56
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
                font: fonts.primaryFont(32)
                elide: Text.ElideRight
            }
        }
    }
}
