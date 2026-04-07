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
