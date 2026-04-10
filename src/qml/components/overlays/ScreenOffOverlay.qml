// Copyright (c) 2026 madalone. Screen-off animation overlay.
// Shared overlay composed above the active theme. Driven by `progress: 0..1`
// (animated externally by ChargingScreen.qml) and `style: string` (user-selected
// via ScreensaverConfig.screenOffEffectStyle).
//
// Each style is a self-contained sub-item gated on `style === "..."`. All styles
// are driven by the single `progress` property so ChargingScreen only needs one
// NumberAnimation to control any of them.
//
// Supported styles: "fade", "flash", "vignette", "wipe".
// ("theme-native" is handled by ChargingScreen dispatching to the theme's own
// protocol functions — this overlay is simply not visible in that case.)
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Item {
    id: root
    anchors.fill: parent

    // Driven by ChargingScreen.qml via NumberAnimation.
    property real progress: 0.0
    // Which visual to render. Unknown values default to "fade".
    property string style: "fade"

    // Visible whenever progress > 0 and a valid shared style is selected.
    visible: progress > 0.001 && style !== "theme-native"

    // ---- Style 1: Fade ----
    // Simple monotonic black ramp. Always-correct fallback.
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: root.progress
        visible: root.style === "fade"
    }

    // ---- Style 2: Flash ----
    // Two phases driven from the same progress:
    //   [0.0 .. 0.4]  white rectangle opacity ramps up then drops
    //   [0.4 .. 1.0]  black rectangle opacity ramps up to full
    // Feels like an old TV "zap" cut-off.
    Item {
        anchors.fill: parent
        visible: root.style === "flash"
        // White flash phase — peaks around progress=0.2, vanishes by 0.4
        Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: {
                var p = root.progress;
                if (p <= 0.0) return 0.0;
                if (p >= 0.4) return 0.0;
                // Triangle wave: 0 → 1 (at p=0.2) → 0 (at p=0.4)
                return p < 0.2 ? (p / 0.2) : (1.0 - (p - 0.2) / 0.2);
            }
        }
        // Black cut-off phase — starts at p=0.4, ends at 1.0
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: root.progress < 0.4 ? 0.0 : (root.progress - 0.4) / 0.6
        }
    }

    // ---- Style 3: Vignette iris close ----
    // Circular black mask closing from edges to centre. ShaderEffect because
    // per-pixel radial masking with soft edges is clumsy in plain QML.
    // Radius shrinks from 0.72 (covers diagonal of 1.0×1.0 UV = ~0.71) to 0.
    ShaderEffect {
        id: vignette
        anchors.fill: parent
        visible: root.style === "vignette"

        property real u_progress: root.progress

        // GLSL ES 2.0 inline shader — same pattern as TvStaticTheme.
        // IMPORTANT: no double-quotes inside the string literal (qmlcachegen
        // parses it as a QML JS string and will bail on embedded quotes).
        fragmentShader: "
            uniform lowp float qt_Opacity;
            varying highp vec2 qt_TexCoord0;
            uniform lowp float u_progress;

            void main() {
                // Distance from centre in UV space. Max distance to a corner
                // is sqrt(0.5) ~= 0.7071. We start the radius just above that
                // and shrink it to 0 as progress goes 0 -> 1.
                highp vec2 c = qt_TexCoord0 - vec2(0.5);
                lowp float d = length(c);
                lowp float radius = mix(0.72, 0.0, u_progress);
                // Soft edge: 0 inside the iris, 1 outside.
                lowp float mask = smoothstep(radius - 0.03, radius, d);
                gl_FragColor = vec4(0.0, 0.0, 0.0, mask) * qt_Opacity;
            }
        "
    }

    // ---- Style 4: Wipe ----
    // Black rectangle anchored to the top whose height ramps from 0 to parent.height.
    // Top-down film-projector wipe. Clean and cheap.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        color: "black"
        height: parent.height * root.progress
        visible: root.style === "wipe"
    }
}
