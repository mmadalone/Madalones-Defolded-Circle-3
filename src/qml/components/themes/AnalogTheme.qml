// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Extracted from stock ChargingScreen as a theme option.
// Copyright (c) 2026 madalone. Native screen-off animation — sweep-to-12 then fall-to-6.
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

    // Screen-off animation state (driven by shutoffAnim).
    // shutoffDim: 0..1 opacity of the dim-to-black overlay.
    // _screenOffAnimating: true while shutoffAnim is active — used by the
    // defensive onDisplayOffChanged handler to detect dispatch races.
    property real shutoffDim: 0.0
    property bool _screenOffAnimating: false

    // ============================================================================
    // Screen-off animation protocol (Tier 2 native override)
    // See BaseTheme.qml for protocol documentation.
    // Sweep-to-12 (600 ms) -> fall-to-6 + dim (400 ms) -> black hold (300 ms).
    // Total 1300 ms = screenOffLeadMs — matches TV Static's lead time.
    //
    // Configurable hand selection via ScreensaverConfig.analogShutoffHands:
    //   "all"  — sec + min + hour all spin and fall together (default)
    //   "main" — only min + hour animate; second hand opacity fades 1 -> 0
    //            during phase 1
    // ============================================================================
    readonly property bool providesNativeScreenOff: true
    readonly property int screenOffLeadMs: 1300

    function startScreenOff() {
        shutoffAnim.complete();
        shutoffAnim.stop();
        // Capture current live rotation values from the clock bindings so
        // phase 1 sweeps from wherever the hands are right now.
        var secNow  = 6 * clock.seconds;
        var minNow  = 6 * clock.minutes;
        var hourNow = 30 * (clock.hours % 12) + 0.5 * clock.minutes;

        var animateSec = (ScreensaverConfig.analogShutoffHands !== "main");

        // Phase 1 rotation targets (to: 720 = 12 o'clock after at least
        // one full forward rotation). In "main" mode, second-hand's rotation
        // stays put (from === to) so only its opacity fades.
        secPhase1Rot.from  = secNow;
        secPhase1Rot.to    = animateSec ? 720 : secNow;
        minPhase1.from     = minNow;
        minPhase1.to       = 720;
        hourPhase1.from    = hourNow;
        hourPhase1.to      = 720;

        // Phase 1 second-hand opacity fade — only in "main" mode.
        // In "all" mode, from === to === 1.0 means no visible change.
        secPhase1Fade.from = 1.0;
        secPhase1Fade.to   = animateSec ? 1.0 : 0.0;

        // Phase 2 rotation targets (720 -> 900 = 180° past 12 = 6 o'clock).
        // "main" mode second hand stays at captured angle throughout phase 2.
        secPhase2.from  = animateSec ? 720 : secNow;
        secPhase2.to    = animateSec ? 900 : secNow;
        minPhase2.from  = 720;
        minPhase2.to    = 900;
        hourPhase2.from = 720;
        hourPhase2.to   = 900;

        // Reset overlay and second-hand opacity for a clean start.
        root.shutoffDim = 0.0;
        secondHand.opacity = 1.0;

        root._screenOffAnimating = true;
        shutoffAnim.start();
    }

    function cancelScreenOff() {
        shutoffAnim.complete();
        shutoffAnim.stop();
        root._screenOffAnimating = false;
        root.shutoffDim = 0.0;
        // Restore live clock bindings on the hand rotations. Direct
        // NumberAnimation writes broke them imperatively; Qt.binding(fn)
        // reassigns equivalent expressions. Codebase convention — see 18
        // other Qt.binding() usages across src/qml/**.
        secondHand.rotation = Qt.binding(function() { return 6 * clock.seconds; });
        minuteHand.rotation = Qt.binding(function() { return 6 * clock.minutes; });
        hourHand.rotation   = Qt.binding(function() {
            return 30 * (clock.hours % 12) + 0.5 * clock.minutes;
        });
        // Reset second-hand opacity (only touched in "main" mode, but reset
        // unconditionally for belt-and-suspenders).
        secondHand.opacity = 1.0;
    }

    function finalizeScreenOff() {
        // Display is about to physically blank. Stop the animation wherever
        // it is and ensure the dim overlay is fully opaque. Hand positions
        // don't matter — nothing is visible. The next wake calls
        // cancelScreenOff() and restores the live bindings.
        shutoffAnim.complete();
        shutoffAnim.stop();
        root.shutoffDim = 1.0;
    }

    // Belt-and-suspenders: hard-reset state on fresh instance so stale
    // values can never leave the theme stuck in a dimmed state. Matches
    // TvStaticTheme.qml:77-80 defensive pattern.
    //
    // CRITICAL — Qt 5.15 qmlcachegen × scene-graph binding race. DO NOT
    // REMOVE OR REFACTOR the Qt.binding() pre-conversion below without
    // re-verifying "first wake after fresh boot" across MULTIPLE cold
    // reboots. This regression is silent (no error, just a black clock)
    // and will ship to production unless tested specifically.
    //
    // ALSO: pre-emptively replace the compiled QML rotation bindings with
    // Qt.binding() JS-closure bindings. This is the fix for the "first
    // wake after fresh boot stays black" bug. Without this, the first
    // animation cycle operates on compiled bindings (generated by
    // qmlcachegen), and the break-then-restore via Qt.binding() leaves the
    // scene graph in an unexpected state that never re-renders on wake.
    // By converting upfront, cycle 1 starts with the same binding type
    // (JS closure) as cycles 2+, and the animation lifecycle behaves
    // uniformly.
    Component.onCompleted: {
        shutoffAnim.stop();
        root._screenOffAnimating = false;
        root.shutoffDim = 0.0;
        secondHand.opacity = 1.0;
        secondHand.rotation = Qt.binding(function() { return 6 * clock.seconds; });
        minuteHand.rotation = Qt.binding(function() { return 6 * clock.minutes; });
        hourHand.rotation   = Qt.binding(function() {
            return 30 * (clock.hours % 12) + 0.5 * clock.minutes;
        });
    }

    // Defensive: on ANY wake (displayOff -> false), check if we have any
    // residual shutdown state — stuck dim overlay, mid-animation, or hands
    // still at their animated end positions — and force a clean reset.
    // Mirrors TvStaticTheme.qml:87-91 which checks `u_tvOff > 0.0`.
    // Without this, the main-path cancel via ChargingScreen.cancelScreenOffEffect()
    // can race with the scene-graph resume on first wake after fresh boot,
    // leaving shutoffDim = 1.0 visible.
    onDisplayOffChanged: {
        if (!root.displayOff && (root._screenOffAnimating || root.shutoffDim > 0.001)) {
            root.cancelScreenOff();
        }
    }

    // Any user input while mid-animation should cancel immediately. This is
    // redundant with ChargingScreen.sendDirection/fireTapEffects dispatch but
    // provides a safety net if the dispatch races with the input.
    function interactiveInput(action) {
        if (root._screenOffAnimating || root.shutoffDim > 0.001) {
            root.cancelScreenOff();
        }
    }

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

    // ============================================================================
    // Screen-off animation: sweep-to-12 -> fall-to-6 + dim -> black hold
    // ============================================================================
    // All from/to values are configured imperatively in startScreenOff()
    // so we can capture current live angles and branch on config mode.
    //
    // Phase 1 (600 ms, OutQuart): selected hands spin forward to 720°
    //   (= 12 o'clock after at least 360° and up to 720° of visible
    //   clockwise rotation — Item.rotation is a plain qreal with no
    //   normalisation, so animating e.g. 30° -> 720° produces 690° of
    //   visible motion — LoadingScreen.qml:202-203 uses the same pattern).
    //   In "main" mode, second-hand opacity fades 1 -> 0 in parallel while
    //   its rotation stays put (from === to).
    // Phase 2 (400 ms, InCubic): selected hands continue 720° -> 900° (=
    //   180° past 12 = 6 o'clock visually) and shutoffDim opacity animates
    //   0 -> 1 in parallel.
    // Phase 3 (300 ms): pure-black hold, absorbs drift between animation
    //   end and the core's Low_power transition.
    SequentialAnimation {
        id: shutoffAnim

        // Auto-clear the animating flag whenever the animation stops
        // (natural completion OR explicit stop()/complete()). Without this,
        // _screenOffAnimating would stay true after natural completion,
        // which is fine for the defensive onDisplayOffChanged gate but
        // clearer state if we keep the flag in sync with actual animation
        // state.
        onStopped: root._screenOffAnimating = false

        // ---- Phase 1 — sweep to 12 ----
        ParallelAnimation {
            NumberAnimation {
                id: secPhase1Rot
                target: secondHand
                property: "rotation"
                duration: 600
                easing.type: Easing.OutQuart
            }
            NumberAnimation {
                id: secPhase1Fade
                target: secondHand
                property: "opacity"
                duration: 600
                easing.type: Easing.InQuad
            }
            NumberAnimation {
                id: minPhase1
                target: minuteHand
                property: "rotation"
                duration: 600
                easing.type: Easing.OutQuart
            }
            NumberAnimation {
                id: hourPhase1
                target: hourHand
                property: "rotation"
                duration: 600
                easing.type: Easing.OutQuart
            }
        }

        // ---- Phase 2 — fall to 6 + dim to black ----
        ParallelAnimation {
            NumberAnimation {
                id: secPhase2
                target: secondHand
                property: "rotation"
                duration: 400
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                id: minPhase2
                target: minuteHand
                property: "rotation"
                duration: 400
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                id: hourPhase2
                target: hourHand
                property: "rotation"
                duration: 400
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: root
                property: "shutoffDim"
                from: 0.0
                to: 1.0
                duration: 400
                easing.type: Easing.InCubic
            }
        }

        // ---- Phase 3 — pure-black hold ----
        PauseAnimation { duration: 300 }
    }

    // Dim-to-black overlay — driven by shutoffDim during phase 2 of the
    // shutdown animation. Sits on top of everything (including the battery
    // overlay) via z: 100.
    //
    // `visible:` gated on shutoffDim > 0.001 so the Rectangle is completely
    // excluded from the scene graph when clean. This avoids any risk of a
    // transparent-but-present Rectangle blocking re-render on the first
    // wake after a fresh boot (suspected cause of the "stuck black" bug
    // observed on cold-start first cycle).
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: root.shutoffDim
        visible: root.shutoffDim > 0.001
        z: 100
    }
}
