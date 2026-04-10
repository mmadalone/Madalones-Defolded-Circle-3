// Copyright (c) 2026 madalone. TV Static charging screen theme.
// Implements BaseTheme interface — see BaseTheme.qml for contract.
// GPU path: Qt 5.15 ShaderEffect with inline GLSL ES 2.0 fragment shader.
// Composes luma snow + chroma bleed + CRT scanlines + rolling tracking bar
// + channel-flash bursts in a single full-frame pass.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import Battery 1.0
import ScreensaverConfig 1.0

import "qrc:/components/overlays" as Overlays

Item {
    id: root
    anchors.fill: parent

    // Runtime state (set by ChargingScreen, not config)
    property bool isClosing: false
    property bool displayOff: false

    // ---- Config-derived uniforms (0..1 normalized) ----
    property real u_intensity:        ScreensaverConfig.tvStaticIntensity        / 100.0
    property real u_snowSize:         Math.max(1, ScreensaverConfig.tvStaticSnowSize)
    property real u_scanlineStrength: ScreensaverConfig.tvStaticScanlineStrength / 100.0
    property real u_scanlineSpeed:    ScreensaverConfig.tvStaticScanlineSpeed    / 100.0
    property real u_chromaAmount:     ScreensaverConfig.tvStaticChromaAmount     / 100.0
    property real u_trackingEnable:   ScreensaverConfig.tvStaticTrackingEnable ? 1.0 : 0.0
    property real u_trackingSpeed:    ScreensaverConfig.tvStaticTrackingSpeed * 1.0  // px/s
    property real u_flashBrightness:  ScreensaverConfig.tvStaticFlashBrightness  / 100.0
    property vector3d u_tint:         hexToVec3(ScreensaverConfig.tvStaticTint)

    // Monotonic time + channel-flash envelope (driven imperatively)
    property real u_time: 0.0
    property real u_flash: 0.0

    // TV-off collapse progress (0 = normal, 1 = fully black dot).
    // Driven by tvOffAnim. The countdown that triggers the animation lives in
    // ChargingScreen.qml (shared screen-off system); this theme just exposes
    // the protocol hooks below.
    property real u_tvOff: 0.0
    readonly property int tvOffDuration: 800   // ms — collapse duration
    readonly property int tvOffHoldMs:   500   // ms — pure-black hold after collapse

    // ============================================================================
    // Screen-off animation protocol (Tier 2 native override)
    // See BaseTheme.qml for full documentation of this optional protocol.
    // ChargingScreen's screen-off poller will call these when the user has
    // selected "theme-native" as the effect style.
    // ============================================================================
    readonly property bool providesNativeScreenOff: true
    // Lead time = collapse + black hold. The hold gives the core's idle timer
    // a buffer window to fire displayOff=true during the black phase so the
    // transition "animation finished" -> "display off" is visually seamless
    // even with ±200 ms polling jitter.
    readonly property int screenOffLeadMs: root.tvOffDuration + root.tvOffHoldMs  // 1300 ms

    function startScreenOff() {
        tvOffAnim.complete();   // flush any pending animation frames
        tvOffAnim.stop();
        root.u_tvOff = 0.0;
        tvOffAnim.start();
    }
    function cancelScreenOff() {
        tvOffAnim.complete();
        tvOffAnim.stop();
        root.u_tvOff = 0.0;
    }
    function finalizeScreenOff() {
        tvOffAnim.complete();
        tvOffAnim.stop();
        root.u_tvOff = 1.0;
    }

    // Belt-and-suspenders: hard-reset u_tvOff on load so stale state can never
    // leave the theme stuck in a collapsed/black state.
    Component.onCompleted: {
        tvOffAnim.stop();
        root.u_tvOff = 0.0;
    }

    // Defensive: if displayOff goes false (wake) and u_tvOff is still at a
    // collapsed value for any reason (lost dispatch, race with finalize),
    // force a reset here. This is redundant with ChargingScreen.cancelScreenOffEffect
    // calling theme.cancelScreenOff() — intentionally so, to paper over any
    // race where ChargingScreen's dispatch missed.
    onDisplayOffChanged: {
        if (!root.displayOff && root.u_tvOff > 0.0) {
            root.cancelScreenOff();
        }
    }

    // Parse hex "#rrggbb" → Qt.vector3d(r,g,b) in 0..1
    function hexToVec3(hex) {
        var r = parseInt(hex.substring(1, 3), 16) / 255.0;
        var g = parseInt(hex.substring(3, 5), 16) / 255.0;
        var b = parseInt(hex.substring(5, 7), 16) / 255.0;
        return Qt.vector3d(r, g, b);
    }

    // Trigger a channel-flash burst
    function triggerFlash() {
        flashAnim.stop();
        root.u_flash = 1.0;
        flashAnim.start();
    }

    NumberAnimation {
        id: flashAnim
        target: root
        property: "u_flash"
        from: 1.0
        to: 0.0
        duration: Math.max(80, ScreensaverConfig.tvStaticFlashDuration)
        easing.type: Easing.OutCubic
    }

    // DPAD / touch dispatch. Fires a channel-flash burst IF tvStaticFlashOnTap
    // is true. ChargingScreen separately handles cancelling the screen-off
    // countdown on every interactiveInput call (no per-theme countdown state).
    function interactiveInput(action) {
        // Defensive: any user activity should clear a partial collapse.
        if (root.u_tvOff > 0.0) root.cancelScreenOff();
        if (ScreensaverConfig.tvStaticFlashOnTap) {
            root.triggerFlash();
        }
    }

    // Black background — prevents flashes of previous theme while shader warms up
    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    // ---- Full-frame ShaderEffect ----
    ShaderEffect {
        id: staticShader
        anchors.fill: parent

        // Property-to-uniform bindings — ShaderEffect auto-maps by name
        property real u_time:             root.u_time
        property vector2d u_resolution:   Qt.vector2d(width, height)
        property real u_intensity:        root.u_intensity
        property real u_snowSize:         root.u_snowSize
        property real u_scanlineStrength: root.u_scanlineStrength
        property real u_scanlineSpeed:    root.u_scanlineSpeed
        property real u_chromaAmount:     root.u_chromaAmount
        property vector3d u_tint:         root.u_tint
        property real u_trackingEnable:   root.u_trackingEnable
        property real u_trackingSpeed:    root.u_trackingSpeed
        property real u_flash:            root.u_flash
        property real u_flashBrightness:  root.u_flashBrightness
        property real u_tvOff:            root.u_tvOff

        // GLSL ES 2.0 fragment shader — Qt 5.15 ShaderEffect inline source.
        // `qt_TexCoord0` and `qt_Opacity` are provided by Qt's default vertex shader.
        // Hash noise form: Inigo Quilez / Hugo Elias standard (fract-sin free variant).
        fragmentShader: "
            uniform lowp float qt_Opacity;
            varying highp vec2 qt_TexCoord0;

            uniform highp float u_time;
            uniform highp vec2  u_resolution;
            uniform lowp  float u_intensity;
            uniform lowp  float u_snowSize;
            uniform lowp  float u_scanlineStrength;
            uniform lowp  float u_scanlineSpeed;
            uniform lowp  float u_chromaAmount;
            uniform lowp  vec3  u_tint;
            uniform lowp  float u_trackingEnable;
            uniform lowp  float u_trackingSpeed;
            uniform lowp  float u_flash;
            uniform lowp  float u_flashBrightness;
            uniform lowp  float u_tvOff;

            highp float hash12(highp vec2 p) {
                highp vec3 p3 = fract(vec3(p.xyx) * 0.1031);
                p3 += dot(p3, p3.yzx + 33.33);
                return fract((p3.x + p3.y) * p3.z);
            }

            void main() {
                highp vec2 pixel = qt_TexCoord0 * u_resolution;
                // Quantized pixel for snow/chroma -- gives blocky cells (snow size).
                highp vec2 qPixel = floor(pixel / u_snowSize) * u_snowSize;
                highp float seed = u_time * 60.0;

                // 1. Luma snow (quantized grain)
                lowp float snow = hash12(qPixel + vec2(seed, seed * 1.3));
                snow = mix(0.08, 1.0, snow);

                // 2. Chroma bleed (VHS) — also uses quantized grain so cells stay coherent
                lowp float rN = hash12(qPixel + vec2(seed + 7.0, 0.0));
                lowp float bN = hash12(qPixel + vec2(0.0, seed + 13.0));
                lowp vec3 rgb = vec3(
                    mix(snow, rN, u_chromaAmount),
                    snow,
                    mix(snow, bN, u_chromaAmount)
                );

                // 3. CRT scanlines — hard alternating rows (period = 2 px).
                // Scanlines always use raw pixel coords (not qPixel) so they stay crisp.
                highp float scanY = pixel.y + u_time * u_scanlineSpeed * 120.0;
                lowp float scan = mod(floor(scanY), 2.0);
                rgb *= mix(1.0, scan, u_scanlineStrength);

                // 4. Rolling tracking bar (VHS vertical-hold drift)
                if (u_trackingEnable > 0.5) {
                    highp float barY = mod(u_time * u_trackingSpeed, u_resolution.y + 80.0) - 40.0;
                    highp float d = (pixel.y - barY) / 30.0;
                    lowp float bar = exp(-d * d);
                    rgb += bar * 0.4 * vec3(hash12(qPixel + vec2(seed * 2.0, 0.0)));
                }

                rgb *= u_tint * u_intensity;

                // 5. Channel-flash envelope — applied AFTER intensity multiply so the
                // flash is always visible regardless of the snow brightness setting.
                // u_flashBrightness scales the peak (0..1).
                rgb = mix(rgb, vec3(1.0), u_flash * u_flashBrightness);

                // 6. TV-off collapse — classic CRT shutdown.
                //    Phase 1 (0.00 -> 0.44): vertical collapse to a line (quadratic ease-in)
                //    Phase 2 (0.44 -> 0.81): line collapses horizontally to a dot
                //    Phase 3 (0.81 -> 1.00): dot fades to black
                if (u_tvOff > 0.001) {
                    lowp float p1 = clamp(u_tvOff / 0.44, 0.0, 1.0);
                    lowp float p2 = clamp((u_tvOff - 0.44) / 0.37, 0.0, 1.0);
                    lowp float p3 = clamp((u_tvOff - 0.81) / 0.19, 0.0, 1.0);

                    lowp float winH = mix(1.0, 0.003, p1 * p1);
                    lowp float winW = mix(1.0, 0.002, p2);

                    highp vec2 c = qt_TexCoord0 - vec2(0.5);
                    lowp float insideY = step(abs(c.y), winH * 0.5);
                    lowp float insideX = step(abs(c.x), winW * 0.5);
                    lowp float mask = insideY * insideX;

                    // Bright white glow as the window shrinks (the classic CRT 'line')
                    lowp float glow = clamp(smoothstep(0.5, 1.0, p1) + p2, 0.0, 1.0) * (1.0 - p3);
                    rgb = mix(rgb, vec3(1.0), glow * 0.8);

                    rgb *= mask * (1.0 - p3);
                }

                gl_FragColor = vec4(rgb, 1.0) * qt_Opacity;
            }
        "
    }

    // ---- Animation driver: advance u_time ----
    // ~30 FPS is plenty for noise; zero cost when displayOff.
    Timer {
        id: animTimer
        interval: 33
        repeat: true
        running: root.visible && !root.isClosing && !root.displayOff
        onTriggered: root.u_time += 0.033
    }

    // ---- Automatic channel-flash cadence ----
    // Central cadence from ScreensaverConfig.tvStaticFlashInterval (seconds),
    // with ±50% jitter on each tick so it doesn't feel metronomic.
    function _pickFlashInterval() {
        var base = Math.max(3, ScreensaverConfig.tvStaticFlashInterval) * 1000;
        var jitter = base * 0.5;
        return Math.max(1000, Math.floor(base - jitter + Math.random() * jitter * 2));
    }
    Timer {
        id: autoFlashTimer
        repeat: true
        running: root.visible && !root.isClosing && !root.displayOff
                 && ScreensaverConfig.tvStaticChannelFlashAuto
        interval: root._pickFlashInterval()
        onTriggered: {
            root.triggerFlash();
            interval = root._pickFlashInterval();
        }
    }

    // ---- CRT TV-off collapse animation ----
    // Phase 1 (0 -> tvOffDuration): linear collapse 0..1 (vertical -> line -> dot)
    // Phase 2 (tvOffHoldMs): hold u_tvOff=1.0 (pure black) while waiting for the
    //         core's idle timer to blank the display. This buffer absorbs the
    //         ±200 ms polling jitter and any drift between my wall-clock
    //         baseline and the core's idle-timer baseline.
    SequentialAnimation {
        id: tvOffAnim
        NumberAnimation {
            target: root
            property: "u_tvOff"
            from: 0.0
            to: 1.0
            duration: root.tvOffDuration
            easing.type: Easing.Linear
        }
        PauseAnimation { duration: root.tvOffHoldMs }
    }

    // NOTE: The screen-off countdown poller, displayOff dispatch, and wake
    // handling all live in ChargingScreen.qml now. This theme just provides
    // the native protocol hooks (startScreenOff / cancelScreenOff /
    // finalizeScreenOff) above and lets ChargingScreen drive them.

    // Clock overlay
    Overlays.ClockOverlay {
        visible: ScreensaverConfig.showClock && (!ScreensaverConfig.clockDockedOnly || Battery.powerSupply)
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: ScreensaverConfig.clockPosition === "top" ? parent.top : undefined
            topMargin: ScreensaverConfig.clockPosition === "top" ? parent.height * 0.15 : 0
            verticalCenter: ScreensaverConfig.clockPosition === "center" ? parent.verticalCenter : undefined
            bottom: ScreensaverConfig.clockPosition === "bottom" ? parent.bottom : undefined
            bottomMargin: ScreensaverConfig.clockPosition === "bottom" ? parent.height * 0.15 : 0
        }
    }

    // Battery overlay
    Overlays.BatteryOverlay {
        visible: ScreensaverConfig.showBattery
        anchors {
            bottom: parent.bottom
            bottomMargin: 40
            horizontalCenter: parent.horizontalCenter
        }
    }
}
