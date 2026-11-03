// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Configurable screensaver with theme support.
// Config propagation handled in C++ via ScreensaverConfig singleton.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Battery 1.0
import Config 1.0
import Power 1.0
import Power.Modes 1.0
import ScreenOffSync 1.0
import ScreensaverConfig 1.0
import SoundEffects 1.0
import TouchSlider 1.0

import "qrc:/components" as Components
import "qrc:/components/overlays" as Overlays

Popup {
    id: chargingScreenRoot
    width: ui.width; height: ui.height
    opacity: 0
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0

    readonly property int doubleTapMs: 300   // max interval between taps for double-tap detection (normal mode)
    readonly property int zoneTapMs: 400     // generous interval for multi-tap detection in zone mode

    property bool isClosing: false
    property bool displayOff: false

    // Shortest shared-overlay animation (the "Animation length" slider's minimum). With less of the dim phase
    // left than this, the screensaver plays none and the core switches the display off by itself. The length
    // itself is ScreensaverConfig.screenOffEffectLengthMs; a theme's own effect has its screenOffLeadMs.
    readonly property int screenOffMinLeadMs: 300
    // True while an effect (either shared overlay OR theme-native) is playing.
    // Covers the theme-native path where screenOffOverlay.progress stays at 0.
    // Reset by cancel; NOT cleared by finalize (finalize means the display is
    // about to blank — nothing should re-trigger until the next wake).
    property bool screenOffEffectActive: false

    // madalone (v1.4.43): pattern B — true when this open() was triggered by an actual user
    // dock event AND Config.dockChimeAfterScreensaver is on. Consumed in onOpened to fire
    // the dock chime AFTER the popup transitions in (clean audio thread, no contention).
    // Reset to false in onOpened after the chime fires.
    property bool _openedViaDock: false

    // madalone (v1.6.0): the main screen (MainContainer / NoPage) is loaded asynchronously once the core
    // has sent the configuration (main.qml containerMain), so the screensaver can open first: DEV opens it
    // at start-up, and a remote that starts on the dock can open it before the configuration arrives. The
    // container's own takeControl() in Component.onCompleted then lands on top of ours, and the buttons go
    // to the page behind the screensaver. Take the input back whenever the container ends up in front
    // while the screensaver is open: the guard help-overlay/Base.qml uses for the tips. Set by main.qml.
    property Item hostContainer: null

    function reclaimInput() {
        if (chargingScreenRoot.opened && hostContainer && ui.inputController.activeItem === hostContainer) {
            buttonNavigation.takeControl();
        }
    }

    Connections {
        target: ui.inputController

        // deferred: taking the input from inside the change notification would re-enter the
        // input controller
        function onActiveItemChanged() {
            Qt.callLater(chargingScreenRoot.reclaimInput);
        }
    }

    // madalone (2026-11-03): the timing comes from ScreenOffSync (src/hardware/screenOffSync.h). It knows when
    // the core's dim phase began (also when this popup opened in the middle of it) and how short it can be, and
    // it switches the display off when the animation has ended. Before, a wall-clock poller and the previous
    // cycle's measured dim phase started the animation 1-2 s early or late: a black but lit screen afterwards,
    // or an animation the switch-off cut short.

    // True iff the currently-selected screen-off style needs to sample the
    // underlying theme's rendering (genie, pixelate, dissolve). Used to gate
    // the `themeCapture` ShaderEffectSource's `live` property so the offscreen
    // FBO render pass is only active during an effect that actually uses it.
    // For all non-sampling styles (fade, flash, vignette, wipe, sleepwave,
    // theme-native) this is false and the SES stays dormant.
    readonly property bool _needsThemeCapture:
        ScreensaverConfig.screenOffEffectStyle === "genie"
        || ScreensaverConfig.screenOffEffectStyle === "pixelate"
        || ScreensaverConfig.screenOffEffectStyle === "dissolve"

    // Forward runtime state changes to the loaded theme, AND dispatch the
    // shared screen-off lifecycle hooks when the display actually blanks / wakes.
    onIsClosingChanged: if (themeLoader.item && themeLoader.item.hasOwnProperty("isClosing")) themeLoader.item.isClosing = isClosing;
    onDisplayOffChanged: {
        if (themeLoader.item && themeLoader.item.hasOwnProperty("displayOff")) {
            themeLoader.item.displayOff = displayOff;
        }
        if (displayOff) {
            chargingScreenRoot.finalizeScreenOffEffect();
        } else {
            // True wake-from-displayOff transition — pass isWakeFromOff=true
            // so the theme's cancelScreenOff() runs to sidestep the
            // binding/scene-graph race that otherwise leaves the rain black.
            chargingScreenRoot.cancelScreenOffEffect(true);
        }
    }

    // =====================================================================
    // Screen-off animation dispatch (Tier 1 shared overlay + Tier 2 native)
    // =====================================================================
    // _scheduleScreenOff() (on entering the dim phase, or on opening / loading the theme during it) starts
    // dimPhaseDelayTimer so the animation ends just before the core's earliest switch-off; the timer fires
    // startScreenOffEffect(). The three helpers route to the theme's native protocol functions if the user
    // picked "theme-native" AND the theme declares providesNativeScreenOff; otherwise they drive the shared
    // ScreenOffOverlay.progress via screenOffAnim. When the animation has ended, screenOffEndTimer asks
    // ScreenOffSync to switch the display off.
    function _themeProvidesNative() {
        return themeLoader.status === Loader.Ready && themeLoader.item
            && themeLoader.item.hasOwnProperty("providesNativeScreenOff")
            && themeLoader.item.providesNativeScreenOff === true;
    }
    function _useNative() {
        return ScreensaverConfig.screenOffEffectStyle === "theme-native"
            && chargingScreenRoot._themeProvidesNative();
    }
    // A theme's own effect keeps its fixed length. The shared overlay plays the "Animation length" setting,
    // shortened so it still ends before the switch-off when less of the dim phase is left (ScreenOffSync.fitLeadMs)
    function _currentLeadMs() {
        if (chargingScreenRoot._useNative()
                && themeLoader.item.hasOwnProperty("screenOffLeadMs")) {
            return themeLoader.item.screenOffLeadMs;
        }
        return ScreenOffSync.fitLeadMs(chargingScreenRoot._sharedLengthMs());
    }
    function _sharedLengthMs() {
        return Math.max(chargingScreenRoot.screenOffMinLeadMs, ScreensaverConfig.screenOffEffectLengthMs);
    }

    // One reset for every path that starts a fresh screen-off cycle (open, theme load, cancel, dock change)
    function _resetScreenOffState() {
        chargingScreenRoot.screenOffEffectActive = false;
        dimPhaseDelayTimer.stop();
        screenOffEndTimer.stop();
        postAnimationSafetyTimer.stop();
        screenOffAnim.stop();
        screenOffOverlay.progress = 0.0;
    }

    // Start dimPhaseDelayTimer so the animation ends just before the earliest switch-off of the dim phase the
    // core is in. Nothing to do outside the dim phase (ScreenOffSync.startDelayMs returns -1).
    function _scheduleScreenOff() {
        if (!chargingScreenRoot.opened || chargingScreenRoot.isClosing) return;
        if (!ScreensaverConfig.screenOffEffectEnabled) return;
        if (!Battery.powerSupply && !ScreensaverConfig.screenOffEffectUndocked) return;
        if (chargingScreenRoot.screenOffEffectActive) return;
        var lead = chargingScreenRoot._currentLeadMs();
        var delay = ScreenOffSync.startDelayMs(lead);
        if (delay < 0) return;
        // Too little of the dim phase left for the shared animation (opened at its very end, or a display-off
        // timeout of 3 s or less): none, the core switches the display off by itself
        if (!chargingScreenRoot._useNative() && lead < chargingScreenRoot.screenOffMinLeadMs) {
            console.info("[screen-off] skipped: " + lead + " ms left before the switch-off");
            return;
        }
        dimPhaseDelayTimer.interval = Math.max(1, delay);
        dimPhaseDelayTimer.restart();
    }

    // The animation has ended: switch the display off now instead of leaving a black but lit screen until the
    // core does it. Only after a visible effect: the default "theme-native" draws nothing on a theme without
    // its own effect (Matrix, Starfield, Minimal), and there the core keeps its own timing.
    function _onScreenOffEffectEnd() {
        if (!chargingScreenRoot.screenOffEffectActive || chargingScreenRoot.displayOff) return;
        if (!chargingScreenRoot.opened || chargingScreenRoot.isClosing) return;
        if (!chargingScreenRoot._useNative() && ScreensaverConfig.screenOffEffectStyle === "theme-native") return;
        var sent = ScreenOffSync.requestDisplayOff();
        console.info("[screen-off] end: " + (sent ? "display off requested" : "display left to the core"));
    }

    function startScreenOffEffect() {
        if (chargingScreenRoot.screenOffEffectActive) return;   // already playing
        chargingScreenRoot.screenOffEffectActive = true;
        ScreenOffSync.markEffectStart();
        var themeEffect = chargingScreenRoot._useNative();
        var leadMs = chargingScreenRoot._currentLeadMs();
        var lengthNote = themeEffect ? "" : " (set " + ScreensaverConfig.screenOffEffectLengthMs + " ms)";
        console.info("[screen-off] start: " + ScreensaverConfig.screenOffEffectStyle + ", " + leadMs + " ms"
                     + lengthNote + ", " + ScreenOffSync.msSinceIdle() + " ms into the dim phase");
        screenOffEndTimer.interval = leadMs;
        screenOffEndTimer.restart();
        if (themeEffect) {
            if (themeLoader.item && themeLoader.item.startScreenOff) {
                themeLoader.item.startScreenOff();
            }
        } else {
            screenOffAnim.stop();
            screenOffOverlay.progress = 0.0;
            // The same length as the end timer and the safety timer
            screenOffAnim.duration = Math.max(1, leadMs);
            screenOffAnim.start();
        }
        // Safety: if the display still isn't off well after the animation
        // (the switch-off request failed, or the core never leaves Idle),
        // the display would stay on rendering a full-screen black overlay,
        // wasting battery and generating heat. Give the core a generous
        // grace window past the animation end; if displayOff is still false
        // by then, set it ourselves so the scene graph stops rendering.
        postAnimationSafetyTimer.interval = leadMs + chargingScreenRoot.safetyGraceMs;
        postAnimationSafetyTimer.restart();
    }

    function cancelScreenOffEffect(isWakeFromOff) {
        isWakeFromOff = isWakeFromOff || false;

        // Pending start, running animation, end request and the stuck-animation safety timer: the user is
        // awake (or otherwise cancelling)
        chargingScreenRoot._resetScreenOffState();

        // Theme reset (cancelScreenOff + scene-graph refresh) only runs on
        // a true wake-from-displayOff transition. That path needs the
        // defensive refresh to sidestep the QML running-binding /
        // scene-graph race (see MatrixTheme.qml fbf9028 comment and the
        // Matrix resetAfterScreenOff helper). On plain user interaction
        // (DPAD, touch, tap effects) OR dock/undock mid-cycle cleanup,
        // we must NOT call theme.cancelScreenOff() — for Matrix it chains
        // through resetAfterScreenOff() → initStreams() and respawns every
        // stream, which looks like a full reset instead of the smooth
        // gravity-lerp direction change the user expects.
        if (isWakeFromOff && themeLoader.item) {
            if (themeLoader.item.cancelScreenOff) {
                themeLoader.item.cancelScreenOff();
            }
            if (typeof themeLoader.item.update === "function") {
                themeLoader.item.update();
            }
        }
    }

    function finalizeScreenOffEffect() {
        // NOTE: leave screenOffEffectActive = true. The display is about to
        // blank; nothing should re-trigger. It will be cleared on the next
        // wake via cancelScreenOffEffect().
        // Display is physically blanking — the safety and end timers are no
        // longer needed; cancel them so they don't fire while the display is off.
        postAnimationSafetyTimer.stop();
        screenOffEndTimer.stop();
        dimPhaseDelayTimer.stop();
        if (chargingScreenRoot._themeProvidesNative()
                && themeLoader.item && themeLoader.item.finalizeScreenOff) {
            themeLoader.item.finalizeScreenOff();
        }
        screenOffAnim.stop();
        screenOffOverlay.progress = 1.0;
    }

    // Grace window past the animation end before we assume the core's
    // Low_power transition isn't coming and force-stop rendering.
    readonly property int safetyGraceMs: 1500

    // Safety timer — fires at (animation lead time + safetyGraceMs)
    // after startScreenOffEffect(). If displayOff is still false at
    // that point, the core never fired Low_power. In that case we set
    // displayOff=true ourselves — this pauses the sim and stops the
    // tick timer (running-binding propagation), saving battery while
    // keeping the popup open. The next Power→Normal wake still flips
    // displayOff=false via main.qml so the user sees rain again on
    // wake instead of being dumped to the home screen.
    //
    // Closing the popup here was the previous behaviour, but it left
    // undocked users staring at the home screen on every wake because
    // Low_power doesn't reliably fire when the device is on battery.
    Timer {
        id: postAnimationSafetyTimer
        repeat: false
        onTriggered: {
            if (!chargingScreenRoot.displayOff) {
                chargingScreenRoot.displayOff = true;
            }
        }
    }

    // One-shot: fires when the running screen-off animation has ended (the lead time of the shared overlay or
    // of the theme's own effect)
    Timer {
        id: screenOffEndTimer
        repeat: false
        onTriggered: chargingScreenRoot._onScreenOffEffectEnd()
    }

    // Persist direction between sessions (gated by dpadPersist setting, works for both DPAD and touch)
    function saveDirection(dir) { if (ScreensaverConfig.dpadPersist) ScreensaverConfig.lastDirection = dir; }
    function restoreDirection() {
        if (!ScreensaverConfig.dpadPersist) return;
        if (!ScreensaverConfig.dpadEnabled && !ScreensaverConfig.tapDirection) return;
        var dir = ScreensaverConfig.lastDirection;
        if (dir !== "" && themeLoader.item && themeLoader.item.interactiveInput)
            themeLoader.item.interactiveInput(dir);
    }

    // --- Touch-zone direction mapping ---
    // Map (x, y) pixel coordinates to one of 9 zones (3x3 grid)
    // Returns direction string or "enter" for center zone
    function zoneFromTap(x, y) {
        var col = Math.floor(x / (width / 3));
        var row = Math.floor(y / (height / 3));
        col = Math.min(Math.max(col, 0), 2);
        row = Math.min(Math.max(row, 0), 2);
        var map = [
            ["up-left",    "up",    "up-right"],
            ["left",       "enter", "right"],
            ["down-left",  "down",  "down-right"]
        ];
        return map[row][col];
    }

    // Kill theme rendering immediately when close starts
    onOpenedChanged: {
        if (!opened) {
            isClosing = true;
            if (matrixRainRef) matrixRainRef.resetEnterState();
            themeLoader.active = false;
        }
    }

    onOpened: {
        isClosing = false;
        themeLoader.active = true;
        // Belt-and-suspenders: reset the screen-off state on every popup open
        // so each dock cycle starts fresh regardless of whether the underlying
        // Loader destroyed/recreated the Popup. Opened in the middle of the dim
        // phase (idle screensaver, docked re-open): schedule from its real start.
        chargingScreenRoot._resetScreenOffState();
        chargingScreenRoot._scheduleScreenOff();
        // Always take button control on open. On first boot, onOpened can
        // fire before the async Loader has realized its child item — gating
        // takeControl() on themeLoader.item was silently skipping the push,
        // leaving main-app ButtonNavigation in control so no close handler
        // fired on any remote button. onLoaded re-calls takeControl() as
        // belt-and-suspenders; pushing twice with the same scope is a no-op.
        buttonNavigation.takeControl();
        if (themeLoader.item) {
            if (themeLoader.item.hasOwnProperty("isClosing")) themeLoader.item.isClosing = false;
            if (themeLoader.item.hasOwnProperty("displayOff")) themeLoader.item.displayOff = chargingScreenRoot.displayOff;
        }
        // madalone (v1.4.43): pattern B — fire the dock chime AFTER the popup is fully shown.
        // By here all heavy QML/GPU init is done; the audio thread has clean CPU and the chime
        // plays without ALSA buffer underruns. Gated by main.qml's _nextOpenViaDock flag (set
        // only on actual user dock events, never on idle-open / wake / boot-while-docked).
        if (chargingScreenRoot._openedViaDock) {
            chargingScreenRoot._openedViaDock = false;
            SoundEffects.play(SoundEffects.BatteryCharge);
        }
    }

    onClosed: {
        isClosing = false;
        holdSlowTimer.stop();
        holdPauseTimer.stop();
        doubleTapTimer.stop();
        centerTapTimer.stop();
        speedOverlayTimer.stop();
        if (buttonNavigation) buttonNavigation.releaseControl();
    }

    // --- Helper: send direction input to theme ---
    function sendDirection(dir) {
        if (themeLoader.item && themeLoader.item.interactiveInput)
            themeLoader.item.interactiveInput(dir);
        chargingScreenRoot.saveDirection(dir);
        chargingScreenRoot.cancelScreenOffEffect();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            // All physical buttons dismiss the screensaver unconditionally.
            // DPAD buttons: interactive when dpadEnabled, otherwise dismiss.
            "BACK": { "pressed": function() { chargingScreenRoot.close(); } },
            "HOME": { "pressed": function() { chargingScreenRoot.close(); } },
            "DPAD_UP":     { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("up");
                else chargingScreenRoot.close();
            }},
            "DPAD_DOWN":   { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("down");
                else chargingScreenRoot.close();
            }},
            "DPAD_LEFT":   { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("left");
                else chargingScreenRoot.close();
            }},
            "DPAD_RIGHT":  { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("right");
                else chargingScreenRoot.close();
            }},
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (ScreensaverConfig.dpadEnabled) {
                        if (matrixRainRef) matrixRainRef.enterPressed();
                    } else {
                        chargingScreenRoot.close();
                    }
                },
                "released": function() {
                    if (!matrixRainRef) return;
                    matrixRainRef.enterReleased();
                }
            },
            "VOICE":  { "pressed": function() { chargingScreenRoot.close(); } },
            "VOLUME_UP": { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("up-left");
                else chargingScreenRoot.close();
            }},
            "VOLUME_DOWN": { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("down-left");
                else chargingScreenRoot.close();
            }},
            "GREEN":  { "pressed": function() { chargingScreenRoot.close(); } },
            "YELLOW": { "pressed": function() { chargingScreenRoot.close(); } },
            "RED":    { "pressed": function() { chargingScreenRoot.close(); } },
            "BLUE":   { "pressed": function() { chargingScreenRoot.close(); } },
            "CHANNEL_UP": { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("up-right");
                else chargingScreenRoot.close();
            }},
            "CHANNEL_DOWN": { "pressed": function() {
                if (ScreensaverConfig.dpadEnabled) chargingScreenRoot.sendDirection("down-right");
                else chargingScreenRoot.close();
            }},
            "MUTE":   { "pressed": function() { chargingScreenRoot.close(); } },
            "PREV":   { "pressed": function() { chargingScreenRoot.close(); } },
            "PLAY":   { "pressed": function() { chargingScreenRoot.close(); } },
            "NEXT":   { "pressed": function() { chargingScreenRoot.close(); } },
            "POWER":  { "pressed": function() { chargingScreenRoot.close(); } },
            "STOP":   { "pressed": function() { chargingScreenRoot.close(); } },
            "RECORD": { "pressed": function() { chargingScreenRoot.close(); } },
            "MENU":   { "pressed": function() { chargingScreenRoot.close(); } }
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 50 }
    }

    background: Rectangle { color: "black" }

    // Only Matrix has interactive gestures (swipe speed, direction zones, hold-to-slow, tap effects).
    // All themes support double-tap-to-close.
    readonly property bool isInteractiveTheme: ScreensaverConfig.theme === "matrix"
    readonly property bool isSwipeableTheme: ScreensaverConfig.theme === "matrix" || ScreensaverConfig.theme === "starfield"

    // --- Gesture handling ---
    // Three gesture types distinguished by movement + duration:
    //   Tap: press+release with < swipeThreshold movement
    //   Swipe: press+drag with >= swipeThreshold vertical movement → speed adjust
    //   Hold: press without movement for holdSlowMs → slow, holdPauseMs → pause
    //
    // Tap behavior:
    //   Normal mode: single tap = effects, double tap = close
    //   Zone mode: edge tap = direction + effects, center multi-tap = glitch/restore/close

    readonly property int swipeThreshold: 30  // px movement to classify as swipe vs tap
    readonly property int holdSlowMs: 500     // hold duration to start slowing
    readonly property int holdPauseMs: 1500   // hold duration to pause completely

    property real lastTapX: 0
    property real lastTapY: 0
    property int centerTapCount: 0
    property real pressStartX: 0
    property real pressStartY: 0
    property bool isDragging: false
    property bool isHolding: false
    property int holdStage: 0  // 0=none, 1=slow, 2=paused

    MouseArea {
        anchors.fill: parent

        onPressed: {
            chargingScreenRoot.pressStartX = mouse.x;
            chargingScreenRoot.pressStartY = mouse.y;
            chargingScreenRoot.isDragging = false;
            chargingScreenRoot.isHolding = false;
            chargingScreenRoot.holdStage = 0;
            if (chargingScreenRoot.isInteractiveTheme) holdSlowTimer.restart();
        }

        onPositionChanged: {
            // Once in hold mode (slow/pause), ignore movement — user is committed to hold gesture
            if (chargingScreenRoot.holdStage > 0) return;

            var dy = Math.abs(mouse.y - chargingScreenRoot.pressStartY);
            var dx = Math.abs(mouse.x - chargingScreenRoot.pressStartX);
            if (dy > chargingScreenRoot.swipeThreshold || dx > chargingScreenRoot.swipeThreshold) {
                chargingScreenRoot.isDragging = true;
                holdSlowTimer.stop();
                holdPauseTimer.stop();
            }
        }

        onReleased: {
            holdSlowTimer.stop();
            holdPauseTimer.stop();

            // Release hold effect if active
            if (chargingScreenRoot.holdStage > 0) {
                chargingScreenRoot.holdStage = 0;
                chargingScreenRoot.isHolding = false;
                if (matrixRainRef) matrixRainRef.interactiveInput("slow:release");
                return;
            }

            if (chargingScreenRoot.isDragging) {
                // --- Swipe gesture: adjust speed (Matrix/Starfield, when enabled + tap direction on) ---
                if (chargingScreenRoot.isSwipeableTheme && ScreensaverConfig.tapSwipeSpeed && ScreensaverConfig.tapDirection) {
                    var deltaY = mouse.y - chargingScreenRoot.pressStartY;
                    var speedDelta = Math.round(-deltaY / 10);
                    if (speedDelta !== 0) {
                        if (ScreensaverConfig.theme === "starfield") {
                            var newStarSpeed = Math.min(100, Math.max(10, ScreensaverConfig.starfieldSpeed + speedDelta));
                            ScreensaverConfig.starfieldSpeed = newStarSpeed;
                        } else {
                            var newMatrixSpeed = Math.min(100, Math.max(10, ScreensaverConfig.matrixSpeed + speedDelta));
                            ScreensaverConfig.matrixSpeed = newMatrixSpeed;
                        }
                        var newSpeed = ScreensaverConfig.theme === "starfield" ? ScreensaverConfig.starfieldSpeed : ScreensaverConfig.matrixSpeed;
                        speedOverlay.text = "Speed: " + newSpeed;
                        speedOverlayTimer.restart();
                        speedOverlay.visible = true;
                    }
                }
                chargingScreenRoot.isDragging = false;
                return;
            }

            // --- Tap gesture ---
            chargingScreenRoot.isDragging = false;
            chargingScreenRoot.isHolding = false;
            chargingScreenRoot.lastTapX = mouse.x;
            chargingScreenRoot.lastTapY = mouse.y;

            if (chargingScreenRoot.isInteractiveTheme && ScreensaverConfig.tapDirection) {
                // --- Zone direction mode (Matrix only) ---
                var zone = chargingScreenRoot.zoneFromTap(mouse.x, mouse.y);

                if (zone !== "enter") {
                    // Edge zone — send direction + fire effects, reset center counter
                    chargingScreenRoot.centerTapCount = 0;
                    centerTapTimer.stop();
                    chargingScreenRoot.sendDirection(zone);
                    chargingScreenRoot.fireTapEffects();
                } else {
                    // Center zone — multi-tap state machine
                    chargingScreenRoot.centerTapCount++;
                    centerTapTimer.restart();

                    if (chargingScreenRoot.centerTapCount <= 2) {
                        // Tap 1-2: enter/glitch + effects
                        if (matrixRainRef) {
                            matrixRainRef.enterPressed();
                            matrixRainRef.enterReleased();
                        }
                        chargingScreenRoot.fireTapEffects();
                    } else if (chargingScreenRoot.centerTapCount === 3) {
                        // Tap 3: restore direction
                        if (themeLoader.item && themeLoader.item.interactiveInput)
                            themeLoader.item.interactiveInput("restore");
                        ScreensaverConfig.lastDirection = "";
                    } else if (chargingScreenRoot.centerTapCount >= 4) {
                        // Tap 4: close screensaver
                        centerTapTimer.stop();
                        chargingScreenRoot.centerTapCount = 0;
                        chargingScreenRoot.close();
                    }
                }
            } else {
                // --- Normal mode: tap-based dismissal ---
                // madalone (v1.4.43): semantics clarified.
                //   tapToClose ON  = require double-tap to close (anti-accidental dismissal).
                //                    Side-effect on Matrix-interactive themes: doubleTapTimer
                //                    fires fireTapEffects() 300 ms after a single tap (line 602).
                //   tapToClose OFF = single-tap closes immediately (responsive dismissal).
                //                    The doubleTapTimer is never started, so the Matrix
                //                    fireTapEffects() side-effect doesn't fire either.
                if (ScreensaverConfig.tapToClose) {
                    if (doubleTapTimer.running) {
                        doubleTapTimer.stop();
                        chargingScreenRoot.close();
                    } else {
                        doubleTapTimer.restart();
                    }
                } else {
                    chargingScreenRoot.close();
                }
            }
        }
    }

    // Hold stage 1 — slow to 25% after 500ms
    Timer {
        id: holdSlowTimer; interval: holdSlowMs
        onTriggered: {
            chargingScreenRoot.isHolding = true;
            chargingScreenRoot.holdStage = 1;
            if (matrixRainRef) matrixRainRef.interactiveInput("slow:hold");
            holdPauseTimer.restart();
        }
    }
    // Hold stage 2 — pause after 1500ms total
    Timer {
        id: holdPauseTimer; interval: holdPauseMs - holdSlowMs
        onTriggered: {
            chargingScreenRoot.holdStage = 2;
            // Use pauseTicks() instead of `running = false` — writing to
            // matrixRain.running from QML breaks the running binding
            // permanently, leaving the sim unable to auto-resume on wake.
            if (matrixRainRef) matrixRainRef.pauseTicks();
        }
    }

    // Connect C++ enter state machine signals to interactiveInput + direction persistence
    property var matrixRainRef: themeLoader.item && themeLoader.item.hasOwnProperty("matrixRainItem")
                                ? themeLoader.item.matrixRainItem : null
    Connections {
        target: matrixRainRef
        ignoreUnknownSignals: true
        function onEnterAction(action) {
            if (action === "restore") ScreensaverConfig.lastDirection = "";
            if (themeLoader.item && themeLoader.item.interactiveInput)
                themeLoader.item.interactiveInput(action);
        }
    }

    // Build tap effect flags string from current config
    function tapEffectFlags() {
        return (ScreensaverConfig.tapBurst ? "1" : "0") + "," +
               (ScreensaverConfig.tapFlash ? "1" : "0") + "," +
               (ScreensaverConfig.tapScramble ? "1" : "0") + "," +
               (ScreensaverConfig.tapSpawn ? "1" : "0") + "," +
               (ScreensaverConfig.tapMessage ? "1" : "0") + "," +
               (ScreensaverConfig.tapSquareBurst ? "1" : "0") + "," +
               (ScreensaverConfig.tapRipple ? "1" : "0") + "," +
               (ScreensaverConfig.tapWipe ? "1" : "0") +
               (ScreensaverConfig.tapRandomize ? ",R" + ScreensaverConfig.tapRandomizeChance : "");
    }

    // Fire tap effects at last recorded tap position
    function fireTapEffects() {
        // Master gate — user can disable all tap effects with one toggle.
        // Still cancel any active screen-off effect so a tap wakes the UI.
        if (!ScreensaverConfig.tapEnabled) {
            chargingScreenRoot.cancelScreenOffEffect();
            return;
        }
        if (themeLoader.item && themeLoader.item.interactiveInput)
            themeLoader.item.interactiveInput("tap:" + lastTapX + "," + lastTapY + "," + tapEffectFlags());
        chargingScreenRoot.cancelScreenOffEffect();
    }

    // Normal mode timer — single tap confirmed after 300ms
    Timer {
        id: doubleTapTimer; interval: doubleTapMs
        onTriggered: { if (chargingScreenRoot.isInteractiveTheme) chargingScreenRoot.fireTapEffects(); }
    }

    // Zone mode timer — reset center tap counter after 400ms of inactivity
    Timer {
        id: centerTapTimer; interval: zoneTapMs
        onTriggered: chargingScreenRoot.centerTapCount = 0;
    }

    // --- Mutual exclusion: tapDirection ON → dpadEnabled OFF ---
    Connections {
        target: ScreensaverConfig
        function onTapDirectionChanged() {
            if (ScreensaverConfig.tapDirection && ScreensaverConfig.dpadEnabled)
                ScreensaverConfig.dpadEnabled = false;
        }
        function onDpadEnabledChanged() {
            if (ScreensaverConfig.dpadEnabled && ScreensaverConfig.tapDirection)
                ScreensaverConfig.tapDirection = false;
        }
    }

    // --- Touchbar speed control ---
    // Active when DPAD direction is on + touchbar speed toggle is on.
    // Swipe left = faster, swipe right = slower. Shows speed overlay briefly.
    property real touchbarPrevX: -1
    readonly property bool touchbarSpeedActive: !chargingScreenRoot.isClosing
                                                && (  (ScreensaverConfig.theme === "matrix" && ScreensaverConfig.dpadTouchbarSpeed && ScreensaverConfig.dpadEnabled && !ScreensaverConfig.tapDirection)
                                                   || (ScreensaverConfig.theme === "starfield")  )
    Connections {
        target: TouchSliderProcessor
        ignoreUnknownSignals: true

        function onTouchPressed() {
            if (!chargingScreenRoot.touchbarSpeedActive) return;
            chargingScreenRoot.touchbarPrevX = TouchSliderProcessor.touchX;
        }
        function onTouchXChanged() {
            if (!chargingScreenRoot.touchbarSpeedActive) return;
            if (chargingScreenRoot.touchbarPrevX < 0) return;
            var delta = TouchSliderProcessor.touchX - chargingScreenRoot.touchbarPrevX;
            chargingScreenRoot.touchbarPrevX = TouchSliderProcessor.touchX;

            if (Math.abs(delta) < 3) return;  // minimum 3px movement

            // v1.4.8: tune sensitivity down (was 1:1 pixel-to-unit, now ~1:3)
            var scaledDelta = delta / 3;

            if (ScreensaverConfig.theme === "starfield") {
                // Starfield: touchbar adjusts density
                var newDensity = Math.round(ScreensaverConfig.starfieldDensity - scaledDelta);
                newDensity = Math.max(10, Math.min(100, newDensity));
                if (newDensity !== ScreensaverConfig.starfieldDensity) {
                    ScreensaverConfig.starfieldDensity = newDensity;
                    speedOverlay.text = "Density: " + newDensity;
                    speedOverlayTimer.restart();
                    speedOverlay.visible = true;
                }
            } else {
                // Matrix: touchbar adjusts speed
                var newSpeed = Math.round(ScreensaverConfig.matrixSpeed - scaledDelta);
                newSpeed = Math.max(10, Math.min(100, newSpeed));
                if (newSpeed !== ScreensaverConfig.matrixSpeed) {
                    ScreensaverConfig.matrixSpeed = newSpeed;
                    speedOverlay.text = "Speed: " + newSpeed;
                    speedOverlayTimer.restart();
                    speedOverlay.visible = true;
                }
            }
        }
        function onTouchReleased() {
            chargingScreenRoot.touchbarPrevX = -1;
        }
    }

    // Speed overlay label (shown briefly on touchbar speed change)
    Text {
        id: speedOverlay
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height - 60
        color: "#ff3333"
        font.pixelSize: 24
        font.bold: true
        visible: false
        z: 9999
    }
    Timer {
        id: speedOverlayTimer
        interval: 1200
        onTriggered: speedOverlay.visible = false;
    }

    Loader {
        id: themeLoader
        anchors.fill: parent
        source: {
            switch (ScreensaverConfig.theme) {
                case "matrix": return "qrc:/components/themes/MatrixTheme.qml";
                case "starfield": return "qrc:/components/themes/StarfieldTheme.qml";
                case "minimal": return "qrc:/components/themes/MinimalTheme.qml";
                case "analog": return "qrc:/components/themes/AnalogTheme.qml";
                case "tvstatic": return "qrc:/components/themes/TvStaticTheme.qml";
                default: return "qrc:/components/themes/MatrixTheme.qml";
            }
        }

        onLoaded: {
            if (!item) return;
            // Belt-and-suspenders re-push in case onOpened fired before the
            // Loader realized its item (first-boot race). Stack push is
            // idempotent for the same scope.
            if (chargingScreenRoot.visible) buttonNavigation.takeControl();
            // Runtime state — not config, must be set explicitly
            if (item.hasOwnProperty("isClosing")) item.isClosing = chargingScreenRoot.isClosing;
            if (item.hasOwnProperty("displayOff")) item.displayOff = chargingScreenRoot.displayOff;
            // Force-reset any theme-native screen-off state so a rapid dock
            // cycle can't leave a fresh theme instance inheriting u_tvOff=1.0
            // from whatever the previous sibling was doing.
            if (item.cancelScreenOff) item.cancelScreenOff();
            // Also reset the shared overlay and the screen-off timers, and schedule again during the dim phase:
            // the theme decides the lead time (its own effect or the shared overlay)
            chargingScreenRoot._resetScreenOffState();
            chargingScreenRoot._scheduleScreenOff();
            // Restore persisted DPAD direction from previous session
            chargingScreenRoot.restoreDirection();
        }
    }

    // ---- Theme capture for sampling-based overlay styles (Batch 2) ----
    // Mirrors the themeLoader's rendered output into an offscreen FBO so the
    // distortion-based shared overlay styles (genie, pixelate, dissolve) can
    // sample the theme as a texture. `hideSource: false` keeps the theme
    // visible in its normal position — the SES just mirrors it.
    //
    // `live` is gated so the offscreen pass only runs while an effect that
    // actually needs it is playing. For the 5 non-sampling styles (fade,
    // flash, vignette, wipe, sleepwave) and theme-native mode, the SES
    // stays dormant — zero GPU cost.
    //
    // FBO cost: ~1.6 MB VRAM on a 480x850 display. Allocated lazily on first
    // `live: true` transition and retained thereafter. Mali T-series handles
    // this trivially; see Qt 5.15 ShaderEffectSource docs.
    ShaderEffectSource {
        id: themeCapture
        anchors.fill: themeLoader
        sourceItem: themeLoader.item
        hideSource: false
        recursive: false
        wrapMode: ShaderEffectSource.ClampToEdge
        visible: false
        live: chargingScreenRoot.screenOffEffectActive
              && chargingScreenRoot._needsThemeCapture
    }

    // ---- Shared screen-off overlay (Tier 1) ----
    // Sits above the theme in document order so it draws on top. Hidden when
    // the user picks "theme-native" AND the theme implements the native
    // protocol — in that case the theme paints its own shutdown effect.
    Overlays.ScreenOffOverlay {
        id: screenOffOverlay
        anchors.fill: parent
        source: themeCapture       // texture input for sampling-based styles
        style: ScreensaverConfig.screenOffEffectStyle
        progress: 0.0
        // ScreenOffOverlay handles its own visibility (progress > 0 && not theme-native)
    }

    // ---- Animation that drives the shared overlay's progress ----
    NumberAnimation {
        id: screenOffAnim
        target: screenOffOverlay
        property: "progress"
        from: 0.0
        to: 1.0
        // Set from the fitted "Animation length" in startScreenOffEffect(), the same number the end timer
        // uses, so the overlay is complete when the display is switched off. Linear: the styles' phases are
        // fractions of the progress.
        duration: 800
        easing.type: Easing.Linear
    }

    // madalone (2026-11-03): no cancel on a dock / undock. A charger change does not restart the core's display-off
    // countdown (the owner's logs: IDLE 12.5 s and 14.2 s after a charger flip at a 30 s timeout, so it was not
    // counted as activity), and a Qi pad flaps on its own. Cancelling here stopped the switch-off request and, for
    // TV Static / Analog, left the theme's own effect black on a lit display. A real pickup wakes the core, and the
    // Idle -> Normal branch below cancels the animation and resets the theme.

    // ---- Trigger: core Power state transition ----
    // The core drives its own idle countdown based on system-level activity.
    // When that countdown reaches the display-off point, it transitions:
    //     Normal  -> Idle        (display dimmed, still visible)
    //     Idle    -> Low_power   (display physically off)
    //
    // The animation plays at the end of the dim phase: it starts so it ends
    // just before the core's earliest switch-off (ScreenOffSync knows when the
    // dim phase began and how short it can be), and then the display is
    // switched off right away. Firmware >= 2.12.0 (the bundle's requirement)
    // always passes through Idle on its timeout path.
    Connections {
        target: Power
        ignoreUnknownSignals: true
        function onPowerModeChanged(fromPowerMode, toPowerMode) {
            if (toPowerMode === PowerModes.Idle) {
                // Deferred: QML handlers of a C++ signal run before its C++ slots, so ScreenOffSync (a C++ slot
                // of the same signal) only knows about this dim phase once the emission is over
                Qt.callLater(chargingScreenRoot._scheduleScreenOff);
            } else if (fromPowerMode === PowerModes.Idle
                       && toPowerMode !== PowerModes.Low_power) {
                // Left the dim phase other than by the display switch-off: woken (touch, motion, a button: Normal),
                // or straight to standby (Suspend). Stop a pending start, and an animation already running (the
                // display stays on, so onDisplayOffChanged never fires for this wake). The non-wake cancel leaves the
                // theme alone (a Matrix reset respawns every stream), so a theme's own effect (TV Static, Analog) is
                // reset here.
                dimPhaseDelayTimer.stop();
                if (chargingScreenRoot.screenOffEffectActive && !chargingScreenRoot.displayOff) {
                    console.info("[screen-off] left the dim phase during the animation: cancelled");
                    var themeEffect = chargingScreenRoot._useNative();
                    chargingScreenRoot.cancelScreenOffEffect(false);
                    if (themeEffect && themeLoader.item && themeLoader.item.cancelScreenOff) {
                        themeLoader.item.cancelScreenOff();
                    }
                }
            }
        }
    }

    // One-shot delay Timer: fires startScreenOffEffect() so the animation ends
    // just before the core's earliest switch-off (see _scheduleScreenOff).
    Timer {
        id: dimPhaseDelayTimer
        repeat: false
        // A start that outlived its dim phase (the core left Idle without the handlers above stopping this timer)
        // must not play: the animation would end on a lit display that nothing switches off
        onTriggered: {
            if (ScreenOffSync.msSinceIdle() >= 0) {
                chargingScreenRoot.startScreenOffEffect();
            } else {
                console.info("[screen-off] start skipped: no longer in the dim phase");
            }
        }
    }
}
