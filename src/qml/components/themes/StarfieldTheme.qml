// Copyright (c) 2024 madalone. Starfield warp charging screen theme.
// Implements BaseTheme interface — see BaseTheme.qml for contract
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import ScreensaverConfig 1.0

import "qrc:/components/overlays" as Overlays

Item {
    id: root
    anchors.fill: parent

    // Runtime state (set by ChargingScreen, not config)
    property bool isClosing: false
    property bool displayOff: false

    // Configurable properties
    property int starCount: 200
    property real speed: 1.0

    // No-op stub — Starfield has no interactive input, but ChargingScreen
    // calls this unconditionally on all themes.
    function interactiveInput(action) {}

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
                var size = brightness * 3;

                // Draw streak line
                ctx.beginPath();
                ctx.moveTo(px, py);
                ctx.lineTo(sx, sy);
                ctx.strokeStyle = "rgba(255, 255, 255, " + (brightness * 0.8) + ")";
                ctx.lineWidth = size * 0.5;
                ctx.stroke();

                // Draw star point
                ctx.beginPath();
                ctx.arc(sx, sy, size * 0.5, 0, Math.PI * 2);
                ctx.fillStyle = "rgba(255, 255, 255, " + brightness + ")";
                ctx.fill();
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
        visible: ScreensaverConfig.showClock
        anchors {
            top: parent.top
            topMargin: parent.height * 0.15
            horizontalCenter: parent.horizontalCenter
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
