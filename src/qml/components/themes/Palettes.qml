// Copyright (c) 2026 madalone. Shared QML palettes — gradient stops + battery-level color tiers.
//
// QML singleton (registered via qmlRegisterSingletonType in main.cpp at startup).
// Consumers: `import Palettes 1.0` then read `Palettes.rainbow` / `Palettes.batteryColor(level)`.
//
// This is the single source of truth for:
//   - Matrix theme color-mode gradients (rainbow / rainbowPlus / neon) — previously
//     duplicated inline across GradientText.qml and the MatrixAppearance settings page
//     color-mode picker.
//   - Battery overlay color tiers — previously hardcoded inline in BatteryOverlay.qml.
//
// Adding a new gradient: drop a new `readonly property Gradient` block here, then
// reference it from any consumer via `Palettes.<name>`. Adding a new theme palette:
// follow the same pattern. No need to update consumers if they take `gradient:` /
// `color:` bindings.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton

import QtQuick 2.15

QtObject {

    // --- Matrix theme color-mode gradients ---------------------------------
    // colorMode strings ("rainbow" / "rainbow_gradient" / "neon") map to these.

    // All gradients use horizontal orientation (the MatrixAppearance settings-page
    // Rectangle.gradient consumers expect left-to-right colors). LinearGradient
    // overlays in GradientText.qml ignore Gradient.orientation entirely and route
    // direction through their own start/end Points, so this setting is harmless
    // there.

    // Rainbow — 5 stops, primary colors, returns to red for seamless loop.
    readonly property Gradient rainbow: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0;  color: "#ff0000" }
        GradientStop { position: 0.25; color: "#ffbf00" }
        GradientStop { position: 0.5;  color: "#00ff41" }
        GradientStop { position: 0.75; color: "#0000ff" }
        GradientStop { position: 1.0;  color: "#ff0000" }
    }

    // Rainbow+ — 11 stops, smooth full-spectrum sweep, returns to red.
    readonly property Gradient rainbowPlus: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#ff0000" }
        GradientStop { position: 0.1; color: "#ff8000" }
        GradientStop { position: 0.2; color: "#ffff00" }
        GradientStop { position: 0.3; color: "#80ff00" }
        GradientStop { position: 0.4; color: "#00ff80" }
        GradientStop { position: 0.5; color: "#00ffff" }
        GradientStop { position: 0.6; color: "#0080ff" }
        GradientStop { position: 0.7; color: "#0000ff" }
        GradientStop { position: 0.8; color: "#8000ff" }
        GradientStop { position: 0.9; color: "#ff00ff" }
        GradientStop { position: 1.0; color: "#ff0000" }
    }

    // Neon — 11 stops, pastel sweep, returns to soft red.
    readonly property Gradient neon: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#ff8080" }
        GradientStop { position: 0.1; color: "#ffd080" }
        GradientStop { position: 0.2; color: "#ffff80" }
        GradientStop { position: 0.3; color: "#d0ff80" }
        GradientStop { position: 0.4; color: "#80ffd0" }
        GradientStop { position: 0.5; color: "#80ffff" }
        GradientStop { position: 0.6; color: "#80d0ff" }
        GradientStop { position: 0.7; color: "#8080ff" }
        GradientStop { position: 0.8; color: "#d080ff" }
        GradientStop { position: 0.9; color: "#ff80ff" }
        GradientStop { position: 1.0; color: "#ff8080" }
    }

    // --- Battery overlay color tiers --------------------------------------
    // Five-tier mapping of charge level → overlay color. Caller passes
    // `Battery.level` (0-100); function returns a color string.
    function batteryColor(level) {
        if (level >= 86) return "#00ff41";   // Full       — green
        if (level >= 61) return "#7fff00";   // Good       — light green
        if (level >= 31) return "#ffd700";   // Medium     — yellow
        if (level >= 16) return "#ff8c00";   // Low        — orange
        return "#ff3333";                     // Critical   — red
    }
}
