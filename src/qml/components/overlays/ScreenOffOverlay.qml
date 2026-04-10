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
    // Live texture capture of the underlying theme (a ShaderEffectSource
    // from ChargingScreen.qml). Used by sampling-based styles — genie,
    // pixelate, dissolve — which distort / sample the theme's rendering.
    // Non-sampling styles ignore this. Null when no theme capture is wired.
    property variant source: null

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

    // ============================================================================
    // BATCH 2 STYLES (added 2026-04-10)
    // Sleep wave is non-sampling. Genie / Pixelate / Dissolve sample the theme
    // via `root.source` (ShaderEffectSource from ChargingScreen).
    // ============================================================================

    // ---- Style 5: Sleep wave (non-sampling) ----
    // Soft cyan glow band travels from top to bottom, leaving progressively
    // darkened space behind it. Pure shader output (no texture sampling), so
    // it doesn't activate the theme-capture path in ChargingScreen.
    ShaderEffect {
        anchors.fill: parent
        visible: root.style === "sleepwave"
        property real u_progress: root.progress
        blending: true
        fragmentShader: "
            uniform lowp float qt_Opacity;
            varying highp vec2 qt_TexCoord0;
            uniform lowp float u_progress;

            void main() {
                lowp float y = qt_TexCoord0.y;
                lowp float wavePos = u_progress;

                // Soft cyan band centred at wavePos with ~15% of screen height falloff.
                lowp float band = smoothstep(0.0, 0.15, wavePos - y + 0.15)
                                * (1.0 - smoothstep(0.0, 0.15, wavePos - y));

                // Darken everything above the wave crest progressively.
                lowp float darken = smoothstep(0.0, 0.1, wavePos - y);

                lowp vec3 glow = vec3(0.2, 0.55, 0.75) * band * 0.8;
                gl_FragColor = vec4(glow, darken) * qt_Opacity;
            }
        "
    }

    // ---- Style 6: Genie / zoom-to-corner (sampling) ----
    // Inverse-scale-around-moving-center transform: the theme content
    // visibly shrinks and slides toward a target corner (bottom centre).
    // Output black outside the shrinking rectangle.
    //
    // Math: at progress=0, scale=1 and center=(0.5,0.5) so we pass uv
    // through unchanged (content fills screen). At progress=1, scale->0
    // and center->target, so only one pixel sees the content and the
    // rest is masked to black.
    //
    // For each output pixel uv, sample FROM the original texture at
    // (uv - center) / scale + 0.5. If that lands outside [0,1], emit
    // black. This inversely scales the texture around the moving center.
    //
    // Limitation: Qt 5.15 ShaderEffect is fragment-shader-only (no vertex
    // shader access), so this is a uniform shrink, not a fluid mesh curl.
    // It looks like a proper "zoom to corner" rather than a genie ribbon.
    ShaderEffect {
        anchors.fill: parent
        visible: root.style === "genie"
        property variant source: root.source
        property real u_progress: root.progress
        blending: false
        fragmentShader: "
            uniform lowp float qt_Opacity;
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform mediump float u_progress;

            void main() {
                // Ease-in quadratic so the collapse accelerates.
                mediump float t = u_progress * u_progress;

                // Scale shrinks from 1 to (near) 0. Floor prevents div-by-zero.
                mediump float s = max(1.0 - t, 0.001);

                // Content center glides from screen centre to bottom-centre.
                mediump vec2 center = mix(vec2(0.5, 0.5), vec2(0.5, 1.0), t);

                // For this output pixel, compute which texture coord it
                // should sample from inside the inversely-scaled source.
                mediump vec2 sampleUv = (qt_TexCoord0 - center) / s + vec2(0.5);

                // Inside-box mask: 1 if sampleUv is within [0,1], else 0.
                mediump vec2 insideVec = step(vec2(0.0), sampleUv)
                                       * step(sampleUv, vec2(1.0));
                mediump float mask = insideVec.x * insideVec.y;

                // Dim the content as it collapses so the final frame is black.
                mediump float dim = 1.0 - t;

                lowp vec4 col = texture2D(source,
                                          clamp(sampleUv, vec2(0.0), vec2(1.0)))
                              * mask * dim;
                gl_FragColor = vec4(col.rgb, 1.0) * qt_Opacity;
            }
        "
    }

    // ---- Style 7: Pixelate (sampling) ----
    // Quantise UV to a grid, sample the theme texture at the grid centre.
    // Block size ramps 0.5% -> 8% of screen width across progress 0..0.8,
    // then fade the pixelated output to black in phase 0.8..1.0.
    //
    // Mali filter note: Qt 5.15 ShaderEffectSource is stuck on GL_LINEAR.
    // Sampling dead-centre in each block (+ blockSize * 0.5) keeps the
    // result crisp enough at 5-8% block sizes.
    ShaderEffect {
        anchors.fill: parent
        visible: root.style === "pixelate"
        property variant source: root.source
        property real u_progress: root.progress
        blending: false
        fragmentShader: "
            uniform lowp float qt_Opacity;
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform lowp float u_progress;

            void main() {
                // Phase 1 (0..0.8): growing pixelation.
                lowp float phase1 = clamp(u_progress / 0.8, 0.0, 1.0);
                lowp float blockSize = mix(0.005, 0.08, phase1);

                // Snap UV to block grid centre.
                lowp vec2 uv = floor(qt_TexCoord0 / blockSize) * blockSize
                             + blockSize * 0.5;

                // Phase 2 (0.8..1.0): fade the pixelated output to black.
                lowp float fade = 1.0 - clamp((u_progress - 0.8) / 0.2, 0.0, 1.0);

                lowp vec4 col = texture2D(source, uv) * fade;
                gl_FragColor = col * qt_Opacity;
            }
        "
    }

    // ---- Style 8: Dissolve to noise (sampling) ----
    // Blend sampled theme with per-pixel hash noise (same Inigo Quilez
    // hash12 we use in TV Static), progressively shifting toward pure
    // noise. Then fade to black in the last 35% of progress.
    ShaderEffect {
        anchors.fill: parent
        visible: root.style === "dissolve"
        property variant source: root.source
        property real u_progress: root.progress
        blending: false
        fragmentShader: "
            uniform lowp float qt_Opacity;
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform lowp float u_progress;

            highp float hash12(highp vec2 p) {
                highp vec3 p3 = fract(vec3(p.xyx) * 0.1031);
                p3 += dot(p3, p3.yzx + 33.33);
                return fract((p3.x + p3.y) * p3.z);
            }

            void main() {
                // Phase 1 (0..0.65): dissolve theme into white noise.
                lowp float diss = clamp(u_progress / 0.65, 0.0, 1.0);
                lowp vec3 theme = texture2D(source, qt_TexCoord0).rgb;
                lowp float n = hash12(qt_TexCoord0 * 1000.0
                                    + vec2(u_progress * 100.0));
                lowp vec3 noise = vec3(n);
                lowp vec3 mixed = mix(theme, noise, diss);

                // Phase 2 (0.65..1.0): fade to black.
                lowp float fade = 1.0
                                - clamp((u_progress - 0.65) / 0.35, 0.0, 1.0);

                gl_FragColor = vec4(mixed * fade, 1.0) * qt_Opacity;
            }
        "
    }
}
