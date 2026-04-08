// Copyright (c) 2026 madalone. Starfield warp charging screen theme.
// Implements BaseTheme interface — see BaseTheme.qml for contract
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

    // Starfield-specific config (independent from Matrix)
    property int starCount: Math.round(100 + ScreensaverConfig.starfieldDensity * 10) // 0→100, 50→600, 100→1100
    property real speed: ScreensaverConfig.starfieldSpeed / 10.0                      // 0→0, 50→5.0, 100→10.0
    property real starSize: 1.0 + ScreensaverConfig.starfieldStarSize * 0.08          // 0→1.0, 50→5.0, 100→9.0
    property real trailFactor: 0.2 + ScreensaverConfig.starfieldTrailLength * 0.018   // 0→0.2, 50→1.1, 100→2.0

    // No-op stub — Starfield has no interactive input, but ChargingScreen
    // calls this unconditionally on all themes.
    function interactiveInput(action) {}

    // Star color helper — returns {r,g,b} for a given star index
    property string starColor: ScreensaverConfig.starfieldColor
    function isGradient(v) { return v === "rainbow" || v === "rainbow_gradient" || v === "neon"; }

    // Parse hex "#rrggbb" → {r, g, b} (0-255)
    function hexToRgb(hex) {
        var r = parseInt(hex.substring(1, 3), 16);
        var g = parseInt(hex.substring(3, 5), 16);
        var b = parseInt(hex.substring(5, 7), 16);
        return { r: r, g: g, b: b };
    }

    // HSL to RGB (h: 0-1, s: 0-1, l: 0-1) → {r,g,b} 0-255
    function hslToRgb(h, s, l) {
        var r, g, b;
        if (s === 0) { r = g = b = l; }
        else {
            function hue2rgb(p, q, t) {
                if (t < 0) t += 1; if (t > 1) t -= 1;
                if (t < 1/6) return p + (q - p) * 6 * t;
                if (t < 1/2) return q;
                if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
                return p;
            }
            var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            var p = 2 * l - q;
            r = hue2rgb(p, q, h + 1/3);
            g = hue2rgb(p, q, h);
            b = hue2rgb(p, q, h - 1/3);
        }
        return { r: Math.round(r * 255), g: Math.round(g * 255), b: Math.round(b * 255) };
    }

    // Get star color for index i out of total n stars
    function starRgb(i, n) {
        if (!isGradient(starColor)) return hexToRgb(starColor);
        var hue = (i / Math.max(1, n)) % 1.0;
        var lightness = starColor === "neon" ? 0.75 : 0.5;
        return hslToRgb(hue, 1.0, lightness);
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        renderTarget: Canvas.FramebufferObject
        renderStrategy: Canvas.Cooperative

        property var stars: []
        property bool initialized: false

        // Reset when star count changes so the array is rebuilt on next paint
        Connections {
            target: root
            function onStarCountChanged() { canvas.initialized = false; }
        }

        readonly property real cx: width / 2
        readonly property real cy: height / 2

        onPaint: {
            var ctx = getContext("2d");

            if (!initialized) {
                stars = [];
                for (var i = 0; i < root.starCount; i++) {
                    stars.push({
                        x: Math.random() * width - cx,
                        y: Math.random() * height - cy,
                        z: Math.random() * width,
                        pz: 0
                    });
                    stars[i].pz = stars[i].z;
                }
                initialized = true;
            }

            // Fade background
            ctx.fillStyle = "rgba(0, 0, 0, 0.15)";
            ctx.fillRect(0, 0, width, height);

            for (var s = 0; s < stars.length; s++) {
                var star = stars[s];

                // Move star closer (decrease z)
                star.z -= root.speed * 4;

                // Reset if past camera
                if (star.z <= 0) {
                    star.x = Math.random() * width - cx;
                    star.y = Math.random() * height - cy;
                    star.z = width;
                    star.pz = star.z;
                    continue;
                }

                // Project to 2D
                var sx = (star.x / star.z) * width + cx;
                var sy = (star.y / star.z) * height + cy;

                // Previous position for streak line
                var px = (star.x / star.pz) * width + cx;
                var py = (star.y / star.pz) * height + cy;

                star.pz = star.z;

                // Skip if off screen
                if (sx < 0 || sx > width || sy < 0 || sy > height) continue;

                // Star brightness based on distance (closer = brighter)
                var brightness = 1 - star.z / width;
                var size = brightness * root.starSize;

                // Star color
                var sc = root.starRgb(s, stars.length);

                // Draw streak line (trail length amplifies the previous position offset)
                var tpx = sx + (px - sx) * root.trailFactor;
                var tpy = sy + (py - sy) * root.trailFactor;
                ctx.beginPath();
                ctx.moveTo(tpx, tpy);
                ctx.lineTo(sx, sy);
                ctx.strokeStyle = "rgba(" + sc.r + "," + sc.g + "," + sc.b + "," + brightness + ")";
                ctx.lineWidth = size;
                ctx.lineCap = "round";  // rounded endcap IS the star head — no gap
                ctx.stroke();
            }
        }
    }

    Timer {
        id: animTimer
        interval: 55
        running: root.visible && !root.isClosing && !root.displayOff
        repeat: true
        onTriggered: canvas.requestPaint()
    }

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
