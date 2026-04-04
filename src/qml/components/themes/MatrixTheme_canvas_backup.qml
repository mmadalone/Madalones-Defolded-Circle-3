// Copyright (c) 2024 madalone. Matrix rain charging screen theme.
// Optimized Canvas implementation for embedded ARM.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import "qrc:/components/overlays" as Overlays

Item {
    id: root
    anchors.fill: parent

    // Configurable properties (set from ChargingScreen applyConfig)
    property color matrixColor: "#00ff41"
    property string colorMode: "green"
    property real speed: 1.0
    property real density: 0.7
    property int trailValue: 50
    property int matrixFontSize: 16
    property string charset: "katakana"
    property bool glow: true
    property bool showClock: false
    property bool showBattery: true
    property bool isClosing: false

    readonly property var colorPresets: ({
        "green":  "#00ff41",
        "blue":   "#00b4d8",
        "red":    "#ff0040",
        "amber":  "#ffbf00",
        "white":  "#ffffff",
        "purple": "#bf00ff"
    })

    readonly property var charsets: ({
        "katakana": "\u30A0\u30A1\u30A2\u30A3\u30A4\u30A5\u30A6\u30A7\u30A8\u30A9\u30AA\u30AB\u30AC\u30AD\u30AE\u30AF\u30B0\u30B1\u30B2\u30B3\u30B4\u30B5\u30B6\u30B7\u30B8\u30B9\u30BA\u30BB\u30BC\u30BD\u30BE\u30BF\u30C0\u30C1\u30C2\u30C3\u30C4\u30C5\u30C6\u30C7\u30C8\u30C9\u30CA\u30CB\u30CC\u30CD\u30CE\u30CF\u30D0\u30D1\u30D2\u30D3\u30D4\u30D5\u30D6\u30D7\u30D8\u30D9\u30DA\u30DB\u30DC\u30DD\u30DE\u30DF\u30E0\u30E1\u30E2\u30E3\u30E4\u30E5\u30E6\u30E7\u30E8\u30E9\u30EA\u30EB\u30EC\u30ED\u30EE\u30EF0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "ascii": "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
        "binary": "01",
        "digits": "0123456789"
    })

    readonly property int colWidth: matrixFontSize + 2
    readonly property int numCols: Math.floor(width / colWidth)

    // Trail alpha: slider 100 = long trails (low alpha), slider 10 = short trails (high alpha)
    // Range: 0.01 (very long ghostly trails) to 0.25 (short sharp trails)
    readonly property real fadeAlpha: 0.25 - (trailValue - 10) * 0.00267

    Rectangle { anchors.fill: parent; color: "black" }

    Canvas {
        id: canvas
        anchors.fill: parent
        renderTarget: Canvas.Image
        renderStrategy: Canvas.Threaded

        property var drops: []
        property bool initialized: false
        property var charArray: []

        // Pre-computed cache
        property string cachedFont: ""
        property string cachedFadeStyle: ""
        property var cachedColColors: []
        property string cachedHeadColor: ""
        property string cachedTrailColor: ""

        function recomputeCache() {
            var cs = root.charsets[root.charset] || root.charsets["katakana"];
            charArray = cs.split("");
            cachedFont = root.matrixFontSize + "px monospace";
            cachedFadeStyle = "rgba(0, 0, 0, " + root.fadeAlpha + ")";

            var cols = root.numCols;
            cachedColColors = [];

            if (root.colorMode === "rainbow") {
                for (var i = 0; i < cols; i++) {
                    var hue = i / cols;
                    var c = (1 - Math.abs(2 * 0.5 - 1)) * 1.0;
                    var x = c * (1 - Math.abs((hue * 6) % 2 - 1));
                    var m = 0.5 - c / 2;
                    var r, g, b, hh = hue * 6;
                    if (hh < 1) { r = c; g = x; b = 0; }
                    else if (hh < 2) { r = x; g = c; b = 0; }
                    else if (hh < 3) { r = 0; g = c; b = x; }
                    else if (hh < 4) { r = 0; g = x; b = c; }
                    else if (hh < 5) { r = x; g = 0; b = c; }
                    else { r = c; g = 0; b = x; }
                    cachedColColors.push("rgb(" + Math.round((r+m)*255) + "," + Math.round((g+m)*255) + "," + Math.round((b+m)*255) + ")");
                }
                cachedHeadColor = "rgba(255,255,255,0.95)";
                cachedTrailColor = "";
            } else {
                var base = root.colorPresets.hasOwnProperty(root.colorMode)
                    ? root.colorPresets[root.colorMode] : "" + root.matrixColor;
                cachedTrailColor = base;
                cachedHeadColor = root.glow ? ("" + Qt.lighter(base, 1.8)) : base;
            }
        }

        onWidthChanged: initialized = false
        onHeightChanged: initialized = false
        Component.onCompleted: recomputeCache()

        Connections {
            target: root
            function onColorModeChanged() { canvas.recomputeCache(); }
            function onMatrixColorChanged() { canvas.recomputeCache(); }
            function onGlowChanged() { canvas.recomputeCache(); }
            function onFadeAlphaChanged() { canvas.recomputeCache(); }
            function onMatrixFontSizeChanged() { canvas.initialized = false; canvas.recomputeCache(); }
            function onCharsetChanged() { canvas.recomputeCache(); }
            function onNumColsChanged() { canvas.initialized = false; canvas.recomputeCache(); }
        }

        onPaint: {
            var ctx = getContext("2d");
            var cols = root.numCols;
            var fs = root.matrixFontSize;
            var cw = root.colWidth;
            var chars = charArray;
            var charLen = chars.length;
            var dens = root.density;
            var spd = root.speed;
            var isRainbow = (root.colorMode === "rainbow");
            var headCol = cachedHeadColor;
            var trailCol = cachedTrailColor;
            var h = canvas.height;
            var w = canvas.width;

            if (!initialized || drops.length !== cols) {
                ctx.fillStyle = "black";
                ctx.fillRect(0, 0, w, h);
                drops = [];
                for (var i = 0; i < cols; i++) {
                    drops.push(Math.random() * (-h / fs));
                }
                initialized = true;
            }

            ctx.fillStyle = cachedFadeStyle;
            ctx.fillRect(0, 0, w, h);
            ctx.font = cachedFont;

            for (var col = 0; col < cols; col++) {
                if (dens < 0.99 && Math.random() > dens) continue;

                var x = col * cw;
                var y = drops[col] * fs;

                // Head character
                ctx.fillStyle = isRainbow ? headCol : headCol;
                ctx.fillText(chars[(Math.random() * charLen) | 0], x, y);

                // Trail character one row above
                if (drops[col] > 1) {
                    ctx.fillStyle = isRainbow ? cachedColColors[col] : trailCol;
                    ctx.fillText(chars[(Math.random() * charLen) | 0], x, y - fs);
                }

                drops[col] += spd * (0.5 + Math.random() * 0.5);

                if (y > h && Math.random() > 0.975) {
                    drops[col] = 0;
                }
            }
        }
    }

    Timer {
        interval: 65
        running: root.visible && !root.isClosing
        repeat: true
        onTriggered: canvas.requestPaint()
    }

    Overlays.ClockOverlay {
        visible: root.showClock
        anchors {
            top: parent.top
            topMargin: parent.height * 0.15
            horizontalCenter: parent.horizontalCenter
        }
    }

    Overlays.BatteryOverlay {
        visible: root.showBattery
        anchors {
            bottom: parent.bottom
            bottomMargin: 40
            horizontalCenter: parent.horizontalCenter
        }
    }
}
