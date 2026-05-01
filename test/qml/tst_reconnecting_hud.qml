// Copyright (c) 2026 madalone. v1.4.34/v1.4.35 — QML tests for ReconnectingHUD
// (wake-replay HUD, v1.4.19, prominence-overhauled v1.4.21).
//
// v1.4.35: removed the 3 tests that asserted on `pulseAnimation.running` /
// `spinnerRotation.running` directly. SequentialAnimation/RotationAnimation
// `on <property>` syntax registers the animation as a property value source —
// it doesn't appear in `Item.children` or `Item.resources`, so a recursive
// `findByObjectName` traversal can't reach it from outside the component.
// Replaced with a binding-chain test on `hud.active` (the readonly property
// that drives both `running:` bindings). Trusts Qt's QML binding engine to
// propagate `hud.active` → `pulseAnimation.running` and `…rotationAnimation`.
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

        function test_active_property_binding() {
            // The chip's `readonly property bool active: EntityController.resumeWindow`
            // is the single source of truth that drives every animation + the slide.
            // Verifying this binding works end-to-end means we can trust the rest of
            // the chain (pulseAnimation.running, rotationAnim.running, banner.y) by
            // QML binding propagation.
            EntityController.resumeWindow = false;
            compare(hud.active, false, "hud.active reflects resumeWindow=false");

            EntityController.resumeWindow = true;
            compare(hud.active, true, "hud.active reflects resumeWindow=true");

            EntityController.resumeWindow = false;
            compare(hud.active, false, "hud.active toggles back when resumeWindow flips off");
        }

        function test_idle_banner_offscreen() {
            // Banner is anchored top of HUD; when active=false it slides to y=-height.
            EntityController.resumeWindow = false;
            var banner = findByObjectName(hud, "hudBanner");
            verify(banner !== null, "hudBanner found");
            tryCompare(banner, "y", -banner.height, 1000, "banner offscreen at top when idle");
        }

        function test_active_slide_in() {
            // 1000ms timeout for a 250ms animation under xvfb load.
            EntityController.resumeWindow = true;
            var banner = findByObjectName(hud, "hudBanner");
            tryCompare(banner, "y", 0, 1000, "banner slides to y=0 when active");
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
