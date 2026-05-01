// Copyright (c) 2026 madalone. v1.4.34 — QML tests for ReconnectingHUD (wake-replay HUD,
// v1.4.19, prominence-overhauled v1.4.21).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Entity.Controller 1.0

import "qrc:/components/overlays" as Overlays

Item {
    id: testRoot
    width: 480
    height: 200

    function findByObjectName(item, name) {
        if (item.objectName === name) return item;
        if (!item.children) return null;
        for (var i = 0; i < item.children.length; i++) {
            var found = findByObjectName(item.children[i], name);
            if (found) return found;
        }
        return null;
    }

    Overlays.ReconnectingHUD {
        id: hud
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
    }

    TestCase {
        id: testCase
        name: "ReconnectingHUD"
        when: windowShown

        function init() {
            EntityController.resetDefaults();   // resumeWindow=false
        }

        function test_idle_state() {
            EntityController.resumeWindow = false;
            var banner  = findByObjectName(hud, "hudBanner");
            verify(banner !== null, "hudBanner found");

            // Banner slides to y=-height when idle. Use tryCompare for the 250ms slide animation.
            tryCompare(banner, "y", -banner.height, 1000, "banner offscreen at top when idle");

            // Animations stop when idle (saves CPU per memory).
            // Note: pulseAnimation and rotation animations are nested children of banner+spinner.
            // We assert via banner's children traversal — pulseAnimation lives under banner.
            // Direct `running` access is via the SequentialAnimation's objectName.
            var pulse = findByObjectName(banner, "hudPulse");
            verify(pulse !== null, "hudPulse animation found");
            tryCompare(pulse, "running", false, 500, "pulse stopped when idle");
        }

        function test_active_slide_in() {
            EntityController.resumeWindow = true;
            var banner = findByObjectName(hud, "hudBanner");
            // 1000ms timeout for a 250ms animation under xvfb load — generous safety margin.
            tryCompare(banner, "y", 0, 1000, "banner slides to y=0 when active");
        }

        function test_active_animations_running() {
            EntityController.resumeWindow = true;
            var banner = findByObjectName(hud, "hudBanner");
            var pulse  = findByObjectName(banner, "hudPulse");
            var spinner = findByObjectName(banner, "hudSpinner");
            verify(spinner !== null, "hudSpinner image found");

            tryCompare(pulse, "running", true, 500, "pulse animation running when active");

            // Spinner's RotationAnimation is a child of the spinner Image. Use objectName lookup.
            var rotation = findByObjectName(spinner, "hudSpinnerRotation");
            verify(rotation !== null, "hudSpinnerRotation found");
            tryCompare(rotation, "running", true, 500, "spinner rotation running when active");
        }

        function test_idle_stops_animations() {
            EntityController.resumeWindow = true;
            var banner = findByObjectName(hud, "hudBanner");
            var pulse  = findByObjectName(banner, "hudPulse");
            tryCompare(pulse, "running", true, 500, "pulse running while active (precondition)");

            EntityController.resumeWindow = false;
            tryCompare(pulse, "running", false, 500, "pulse stops after deactivation");
        }

        function test_round_trip() {
            // Activate → wait for slide-in → deactivate → wait for slide-out.
            // Catches the directional-easing path: easing.type: hudRoot.active ? OutExpo : InExpo.
            EntityController.resumeWindow = true;
            var banner = findByObjectName(hud, "hudBanner");
            tryCompare(banner, "y", 0, 1000, "slid in");

            EntityController.resumeWindow = false;
            tryCompare(banner, "y", -banner.height, 1000, "slid back out");
        }
    }
}
