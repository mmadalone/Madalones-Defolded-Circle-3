// Copyright (c) 2026 madalone. Base theme interface contract. SPDX-License-Identifier: GPL-3.0-or-later
//
// Defines the property and function interface that all screensaver themes must implement.
// ChargingScreen.qml loads themes via Loader by source URL, so themes do NOT inherit from
// this type. Each theme declares these properties independently. This file serves as the
// single-source-of-truth contract definition.
//
// Required interface:
//   property bool displayOff    — true when the physical display is off (gate animations)
//   property bool isClosing     — true during exit animation (stop rendering)
//   property bool showClock     — whether clock overlay is visible
//   property bool showBattery   — whether battery overlay is visible
//   function interactiveInput(action) — handle DPAD/touch input from ChargingScreen
//
// Themes may satisfy the showClock/showBattery contract in two ways:
//   1. Declare these as properties (optionally with custom logic, e.g. MatrixTheme
//      adds extra conditions beyond the config value).
//   2. Bind ScreensaverConfig directly on overlay visibility within the theme,
//      bypassing these properties entirely (e.g. StarfieldTheme, MatrixTheme).
// Both patterns are valid. The defaults here bind directly to ScreensaverConfig.
//
// ============================================================================
// OPTIONAL: Screen-off animation protocol (Tier 2 native override)
// ============================================================================
// ChargingScreen owns a shared countdown + ScreenOffOverlay system. By default,
// any theme inherits the user-selected shared overlay style (fade / flash /
// vignette / wipe) from ScreensaverConfig.screenOffEffectStyle.
//
// A theme can OPT IN to provide its own native shutdown animation by declaring:
//
//   readonly property bool providesNativeScreenOff: true
//   readonly property int  screenOffLeadMs: 800   // ms — how early to start
//
//   function startScreenOff()    { /* begin the animation */ }
//   function cancelScreenOff()   { /* reset on wake. ALSO the wake-refresh hook for any
//                                    theme with stateful rendering (e.g. MatrixTheme
//                                    calls matrixRain.resetAfterScreenOff() here to
//                                    sidestep the QML running-binding vs scene-graph
//                                    race — see MatrixTheme.qml / fbf9028). Gated in
//                                    ChargingScreen.cancelScreenOffEffect(isWakeFromOff)
//                                    so user interaction (DPAD/touch) doesn't fire it. */ }
//   function finalizeScreenOff() { /* snap to final state — display physically off */ }
//
// When providesNativeScreenOff === true AND the user-selected effect style is
// "theme-native", ChargingScreen will:
//   - Call startScreenOff() when the idle poller crosses (displayTimeout - leadMs)
//   - Call cancelScreenOff() on any interactiveInput and on displayOff → false (wake)
//   - Call finalizeScreenOff() on displayOff → true (hardware display blanked)
//
// The shared ScreenOffOverlay is hidden for native-capable themes (unless the
// user explicitly picks a shared style like "fade", in which case the theme's
// native hooks are bypassed and the overlay animates instead).
//
// Themes that do NOT declare these (Matrix, Starfield, Minimal, Analog today)
// automatically inherit the shared overlay behaviour when the user picks a
// shared style, and get no animation when "theme-native" is selected (since
// they have no native implementation).
//
// Dispatch convention (per ChargingScreen.qml existing style):
//   - Property checks use hasOwnProperty("providesNativeScreenOff")
//   - Function checks use truthy && (item && item.startScreenOff)
// See STYLE_GUIDE.md §592 for the documented codebase convention.

import QtQuick 2.15
import ScreensaverConfig 1.0

Item {
    id: baseTheme
    anchors.fill: parent

    // Runtime state — set by ChargingScreen, not config
    property bool displayOff: false
    property bool isClosing: false

    // Overlay visibility — bound to ScreensaverConfig by default
    property bool showClock: ScreensaverConfig.showClock
    property bool showBattery: ScreensaverConfig.showBattery

    // Interactive input handler — no-op by default, override for themes with
    // DPAD/touch interaction (e.g. MatrixTheme gravity control)
    function interactiveInput(action) {}
}
