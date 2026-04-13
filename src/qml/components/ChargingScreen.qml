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
import ScreensaverConfig 1.0
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

    // Wall-clock baseline for the shared screen-off countdown. Reset on open,
    // on interactiveInput dispatch, and on wake (displayOff -> false).
    property double countdownStartTime: 0.0
    // Lead time for whatever effect is currently active. Shared overlay uses
    // a fixed 800 ms; theme-native themes override via their screenOffLeadMs.
    readonly property int screenOffFallbackLeadMs: 800
    // True while an effect (either shared overlay OR theme-native) is playing.
    // Covers the theme-native path where screenOffOverlay.progress stays at 0
    // and the poller would otherwise re-fire every tick, restarting the theme
    // animation from 0 in a loop. Reset by cancel; NOT cleared by finalize
    // (finalize means the display is about to blank — no further poll should
    // re-trigger until the next wake).
    property bool screenOffEffectActive: false

    // True when the popup opened via the idle-screensaver timer on battery
    // — a legacy path where `idleTimeout` seconds of inactivity preceded
    // the open and the core's display-off counter has ALREADY been counting
    // during that window. When this flag is set, the screen-off countdown
    // poller subtracts idleTimeout from the effective window to align with
    // the core's internal counter. Any event that represents fresh user
    // activity (dock, undock, wake from displayOff) resets this flag — those
    // events reset the core's idle counter, giving us the full displayTimeout
    // window ahead. Signalled from main.qml via chargingScreenLoader
    // .onStatusChanged before item.open() fires.
    property bool _openedViaIdleTimer: false

    // Measured dim-phase duration — how long the core spends in the Idle
    // power mode (display dimmed) before transitioning to Low_power (display
    // physically off). Measured empirically on each cycle so we can delay
    // the animation start to land exactly at the Low_power transition.
    //
    // PERSISTED in ScreensaverConfig.measuredDimPhaseMs (QSettings-backed)
    // so the measurement survives Popup destruction on undock/redock cycles.
    // Without persistence, every redock would reset to the seeded default
    // (3000 ms), causing a visible "black screen but display still on" gap
    // whenever the actual dim phase diverges from the seed. See screensaverconfig.h.
    property double idleEnteredAtMs: 0.0

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
            // Measure the dim phase duration for next cycle's timing.
            // idleEnteredAtMs is set on the Normal -> Idle transition; if we
            // see Low_power without an Idle entry recorded (rare — direct
            // Normal -> Low_power transition), skip the measurement.
            // Writes to ScreensaverConfig.measuredDimPhaseMs persist to
            // QSettings so the value survives Popup destruction.
            if (chargingScreenRoot.idleEnteredAtMs > 0) {
                var dimMs = Date.now() - chargingScreenRoot.idleEnteredAtMs;
                // Clamp to a sane range so a weird measurement can't poison
                // the next cycle (minimum 500 ms, maximum 30 s).
                if (dimMs >= 500 && dimMs <= 30000) {
                    ScreensaverConfig.measuredDimPhaseMs = dimMs;
                }
                chargingScreenRoot.idleEnteredAtMs = 0;
            }
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
    // The poller Timer further down fires startScreenOffEffect() when the
    // idle threshold is crossed. The three helpers route to the theme's
    // native protocol functions if the user picked "theme-native" AND the
    // theme declares providesNativeScreenOff; otherwise they drive the
    // shared ScreenOffOverlay.progress via screenOffAnim.
    function _themeProvidesNative() {
        return themeLoader.status === Loader.Ready && themeLoader.item
            && themeLoader.item.hasOwnProperty("providesNativeScreenOff")
            && themeLoader.item.providesNativeScreenOff === true;
    }
    function _useNative() {
        return ScreensaverConfig.screenOffEffectStyle === "theme-native"
            && chargingScreenRoot._themeProvidesNative();
    }
    function _currentLeadMs() {
        if (chargingScreenRoot._useNative()
                && themeLoader.item.hasOwnProperty("screenOffLeadMs")) {
            return themeLoader.item.screenOffLeadMs;
        }
        return chargingScreenRoot.screenOffFallbackLeadMs;
    }

    function startScreenOffEffect() {
        if (chargingScreenRoot.screenOffEffectActive) return;   // already playing
        chargingScreenRoot.screenOffEffectActive = true;
        if (chargingScreenRoot._useNative()) {
            if (themeLoader.item && themeLoader.item.startScreenOff) {
                themeLoader.item.startScreenOff();
            }
        } else {
            screenOffAnim.stop();
            screenOffOverlay.progress = 0.0;
            screenOffAnim.start();
        }
        // Safety: if the core's Low_power transition never fires (e.g.
        // on battery the core's idle timer may not reliably transition
        // Idle → Low_power), the animation ends at shutoffDim=1.0 and
        // the display stays on rendering a full-screen black overlay
        // continuously, wasting battery and generating heat. Give the
        // core a generous grace window past the animation end; if
        // displayOff is still false by then, close the popup entirely
        // so the scene graph stops rendering the stuck screensaver.
        postAnimationSafetyTimer.interval = chargingScreenRoot._currentLeadMs()
                                          + chargingScreenRoot.safetyGraceMs;
        postAnimationSafetyTimer.restart();
    }

    function cancelScreenOffEffect(isWakeFromOff) {
        isWakeFromOff = isWakeFromOff || false;

        // Reset wall-clock baseline so the poller restarts from now.
        chargingScreenRoot.countdownStartTime = Date.now();
        chargingScreenRoot.screenOffEffectActive = false;
        // Wake is fresh user activity — the core's idle counter just reset,
        // so the next cycle uses the full displayTimeout window.
        chargingScreenRoot._openedViaIdleTimer = false;
        // Cancel any pending delayed start from the Idle-entry handler.
        dimPhaseDelayTimer.stop();
        // Cancel the stuck-animation safety timer — the user is awake
        // (or otherwise cancelling), no need to force-close the popup.
        postAnimationSafetyTimer.stop();
        screenOffAnim.stop();
        screenOffOverlay.progress = 0.0;

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
        // blank; we don't want the poller re-firing. It will be cleared on
        // the next wake via cancelScreenOffEffect().
        // Display is physically blanking — the safety timer is no longer
        // needed; cancel it so it doesn't fire while the display is off.
        postAnimationSafetyTimer.stop();
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
        // Belt-and-suspenders: reset the screen-off countdown baseline on
        // every popup open so each dock cycle starts fresh regardless of
        // whether the underlying Loader destroyed/recreated the Popup.
        chargingScreenRoot.countdownStartTime = Date.now();
        chargingScreenRoot.screenOffEffectActive = false;
        chargingScreenRoot.idleEnteredAtMs = 0;
        dimPhaseDelayTimer.stop();
        screenOffAnim.stop();
        screenOffOverlay.progress = 0.0;
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
                // --- Normal mode: double-tap to close ---
                if (doubleTapTimer.running) {
                    doubleTapTimer.stop();
                    if (ScreensaverConfig.tapToClose) chargingScreenRoot.close();
                } else {
                    doubleTapTimer.restart();
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

            if (ScreensaverConfig.theme === "starfield") {
                // Starfield: touchbar adjusts density
                var newDensity = Math.round(ScreensaverConfig.starfieldDensity - delta);
                newDensity = Math.max(10, Math.min(100, newDensity));
                if (newDensity !== ScreensaverConfig.starfieldDensity) {
                    ScreensaverConfig.starfieldDensity = newDensity;
                    speedOverlay.text = "Density: " + newDensity;
                    speedOverlayTimer.restart();
                    speedOverlay.visible = true;
                }
            } else {
                // Matrix: touchbar adjusts speed
                var newSpeed = Math.round(ScreensaverConfig.matrixSpeed - delta);
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
            // Also reset the shared overlay and the central countdown baseline.
            chargingScreenRoot.screenOffEffectActive = false;
            chargingScreenRoot.countdownStartTime = Date.now();
            screenOffAnim.stop();
            screenOffOverlay.progress = 0.0;
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
        // Duration matches the shared-overlay fallback lead time so the
        // animation completes exactly as the display actually blanks.
        duration: chargingScreenRoot.screenOffFallbackLeadMs
        easing.type: Easing.Linear
    }

    // Dock / undock are both fresh user activity from the core's perspective
    // — the core's display-off timer resets on either transition. Follow suit:
    // reset our countdown baseline and clear the idle-open flag so the next
    // screen-off cycle aligns with the core's fresh idle counter instead of a
    // stale pre-transition baseline. Without this, a popup that persists
    // through undock (via 'Close on wake' toggle OFF) uses a stale baseline
    // plus the subtracted idleTimeout formula, firing the animation seconds
    // before the core actually blanks the display.
    Connections {
        target: Battery
        ignoreUnknownSignals: true
        function onPowerSupplyChanged(value) {
            if (chargingScreenRoot.isClosing) return;
            chargingScreenRoot.countdownStartTime = Date.now();
            chargingScreenRoot._openedViaIdleTimer = false;
            // Cancel any in-flight animation from the pre-transition cycle
            // so it doesn't finish early against the fresh baseline.
            if (chargingScreenRoot.screenOffEffectActive
                    && !chargingScreenRoot.displayOff) {
                chargingScreenRoot.cancelScreenOffEffect();
            }
        }
    }

    // ---- Primary trigger: core Power state transition ----
    // The core drives its own idle countdown based on system-level activity.
    // When that countdown reaches the display-off point, it transitions:
    //     Normal  -> Idle        (display dimmed, still visible)
    //     Idle    -> Low_power   (display physically off)
    //
    // We want the animation to play DURING the dim phase and end exactly at
    // the Low_power transition. To do that we measure the dim phase duration
    // on each cycle (Idle entry -> Low_power entry) and, on subsequent cycles,
    // delay the animation start by (measuredDimPhaseMs - leadMs) so it ends
    // at the measured Low_power moment.
    //
    // First cycle uses a seeded default measurement; cycles 2+ are accurate.
    Connections {
        target: Power
        ignoreUnknownSignals: true
        function onPowerModeChanged(fromPowerMode, toPowerMode) {
            if (toPowerMode === PowerModes.Idle) {
                // Dim phase just started — record entry time for measurement
                // and schedule the animation to fire near the end of the dim
                // phase so it completes exactly as Low_power fires.
                chargingScreenRoot.idleEnteredAtMs = Date.now();
                if (!chargingScreenRoot.opened) return;
                if (chargingScreenRoot.isClosing) return;
                if (!ScreensaverConfig.screenOffEffectEnabled) return;
                if (!Battery.powerSupply && !ScreensaverConfig.screenOffEffectUndocked) return;
                if (chargingScreenRoot.screenOffEffectActive) return;
                var delay = ScreensaverConfig.measuredDimPhaseMs
                          - chargingScreenRoot._currentLeadMs();
                if (delay < 0) delay = 0;
                dimPhaseDelayTimer.interval = delay;
                dimPhaseDelayTimer.restart();
            } else if (fromPowerMode === PowerModes.Idle
                       && toPowerMode === PowerModes.Normal) {
                // User woke the remote during the dim phase (tap/motion).
                // Cancel any pending delayed animation start.
                dimPhaseDelayTimer.stop();
            }
        }
    }

    // One-shot delay Timer: fires startScreenOffEffect() at
    // (measuredDimPhaseMs - leadMs) after Normal -> Idle, so the animation
    // ends at the Low_power transition.
    Timer {
        id: dimPhaseDelayTimer
        repeat: false
        onTriggered: chargingScreenRoot.startScreenOffEffect()
    }

    // ---- Idle countdown poller (200 ms wall-clock tick) — FALLBACK ----
    // Central gate: effect master switch + (dock OR undocked-override) + live
    // display-off timeout sanity check. Polls elapsed wall-clock time against
    // the dynamic threshold (displayTimeout - leadMs) and fires the shared
    // dispatch when crossed. Repeating Timer with binding — the single-shot
    // + binding combo is broken per Qt QML property-binding rules.
    Timer {
        id: screenOffCountdownPoller
        interval: 200
        repeat: true
        running: chargingScreenRoot.opened
                 && !chargingScreenRoot.isClosing
                 && !chargingScreenRoot.displayOff
                 && ScreensaverConfig.screenOffEffectEnabled
                 && (Battery.powerSupply || ScreensaverConfig.screenOffEffectUndocked)
                 && Config.displayTimeout >= 5
        onRunningChanged: {
            if (running) {
                // Fresh start (or restart after a condition flip). Reset the
                // wall-clock baseline and clear any lingering overlay progress
                // and the active-effect flag.
                chargingScreenRoot.countdownStartTime = Date.now();
                chargingScreenRoot.screenOffEffectActive = false;
                screenOffAnim.stop();
                screenOffOverlay.progress = 0.0;
            }
            // When running -> false (display blanked, popup closed, undock),
            // state cleanup is handled elsewhere: onDisplayOffChanged fires
            // cancelScreenOffEffect() on wake, and onOpened re-initialises
            // on the next popup creation. No action needed here.
        }
        onTriggered: {
            // Guard: don't re-fire while the effect is already in progress.
            // This single flag covers BOTH the shared overlay path (where
            // progress would be > 0) AND the theme-native path (where the
            // shared overlay is bypassed and progress stays at 0).
            if (chargingScreenRoot.screenOffEffectActive) return;

            // Guard: themeLoader must be fully loaded before we dispatch to
            // native hooks — onLoaded may fire after a poll tick if a theme
            // is being reloaded.
            if (themeLoader.status !== Loader.Ready) return;

            var now = Date.now();
            // Self-heal: zero / future baseline means state drift — snap and
            // retry next tick.
            if (chargingScreenRoot.countdownStartTime <= 0
                    || chargingScreenRoot.countdownStartTime > now) {
                chargingScreenRoot.countdownStartTime = now;
                return;
            }

            var elapsed = now - chargingScreenRoot.countdownStartTime;

            // Effective remaining window until the core blanks the display.
            // - On dock / undock / wake / any user activity: the core's idle
            //   counter just reset, so the full displayTimeout is ahead.
            // - Opened via idleScreensaverTimer on battery (legacy path): the
            //   popup opened after `idleTimeout` seconds of inactivity during
            //   which the core's display-off counter was ALSO running, so
            //   only (displayTimeout - idleTimeout) remain. Tracked via the
            //   _openedViaIdleTimer flag (set by main.qml before item.open
            //   when the idle timer fired the open).
            var effectiveTimeoutMs = Config.displayTimeout * 1000;
            if (chargingScreenRoot._openedViaIdleTimer) {
                effectiveTimeoutMs -= ScreensaverConfig.idleTimeout * 1000;
            }

            // Floor to 500 ms so a mid-countdown slider drop or an idleTimeout
            // that exceeds displayTimeout can never produce a negative lead
            // window (which would either fire immediately or never fire).
            var threshold = Math.max(500,
                effectiveTimeoutMs - chargingScreenRoot._currentLeadMs());

            if (elapsed >= threshold) {
                chargingScreenRoot.startScreenOffEffect();
            }
        }
    }

}
