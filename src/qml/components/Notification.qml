// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Banner durations and the error banner (Settings → User interface → Messages).
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 NOTIFICATION COMPONENT
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
 
import Haptic 1.0
import Config 1.0

Popup {
    id: notification
    x: 0; y: 0
    width: parent.width
    height: notificationBg.height
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    // madalone (v1.6.3): how long the banner on screen stays. Set per banner from Settings → User interface →
    // Messages: the error banner (in place of an error pop-up) and the other banners have a duration each.
    property int displayMs: 4000

    // madalone (v1.6.3): upstream's handler, shared by both kinds of banner
    function show(message, warning, durationMs) {
        notificationMessage.text = message;
        notificationBg.color = warning ? colors.red : colors.buttonPrimary
        notification.displayMs = durationMs;

        // a message replacing one that is already showing gets the full display time instead of
        // whatever is left of the previous one, which made it flash by
        if (notification.opened) {
            displayTimer.restart();
        }

        notification.open();
    }

    Connections {
        target: ui.notification
        ignoreUnknownSignals: true

        // madalone (v1.6.3): every banner but the error banner, with "Other banner duration" (whether a banner for an
        // unavailable device is created at all is Notification's decision, in C++)
        function onNotificationCreated(message, warning) {
            notification.show(message, warning, Config.otherBannerSec * 1000);
        }

        // madalone (v1.6.3): the banner in place of an error pop-up ("Show error pop-ups" off)
        function onErrorBannerCreated(message, warning) {
            notification.show(message, warning, Config.errorBannerSec * 1000);
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 200 }
        NumberAnimation { property: "scale"; from: 0.9; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        NumberAnimation { target: rot; property: "angle"; from: 90; to: 0; easing.type: Easing.OutExpo; duration: 300 }
        NumberAnimation { target: notificationBg; property: "y"; from: -100; to: 10; easing.type: Easing.OutExpo; duration: 200 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 200 }
    }

    background: Item {}

    contentItem: Rectangle {
        id: notificationBg
        width: parent.width - 20
        height: notificationMessage.implicitHeight + 30
        radius: ui.cornerRadiusSmall
        anchors.horizontalCenter: parent.horizontalCenter

        transform: Rotation {
            id: rot
            origin.x: notificationBg.width / 2; origin.y: 0
            axis { x: 1; y: 0; z: 0 }
            angle: 0
        }

        Text {
            id: notificationMessage
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter
            anchors { left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
            font: fonts.prose()
            lineHeight: fonts.proseLineHeight
        }
    }

    onOpened: displayTimer.restart()
    onClosed: displayTimer.stop()

    Timer {
        id: displayTimer
        repeat: false
        interval: notification.displayMs
        onTriggered: notification.close()
    }
}
