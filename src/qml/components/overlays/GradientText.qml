// Copyright (c) 2026 madalone. Reusable text element supporting solid color or rainbow gradient.
// colorValue accepts a hex string ("#ffffff") or a gradient mode ("rainbow", "rainbow_gradient", "neon").
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtGraphicalEffects 1.0

import Palettes 1.0

Item {
    id: root

    property string colorValue: "#ffffff"
    property alias text: sourceText.text
    property alias font: sourceText.font

    // Expose content dimensions for parent anchoring
    implicitWidth: sourceText.contentWidth
    implicitHeight: sourceText.contentHeight

    function isGradient(v) {
        return v === "rainbow" || v === "rainbow_gradient" || v === "neon";
    }

    // Source text — visible when solid color, hidden (layer source) when gradient
    Text {
        id: sourceText
        visible: !root.isGradient(root.colorValue)
        color: root.isGradient(root.colorValue) ? "#ffffff" : root.colorValue
        layer.enabled: root.isGradient(root.colorValue)
    }

    // Gradient overlays — gradient stops shared via Palettes singleton.
    // Visibility-gated by colorValue; only one renders at a time.
    LinearGradient {
        visible: root.colorValue === "rainbow"
        anchors.fill: sourceText
        source: sourceText
        start: Qt.point(0, 0)
        end: Qt.point(sourceText.contentWidth, 0)
        gradient: Palettes.rainbow
    }
    LinearGradient {
        visible: root.colorValue === "rainbow_gradient"
        anchors.fill: sourceText
        source: sourceText
        start: Qt.point(0, 0)
        end: Qt.point(sourceText.contentWidth, 0)
        gradient: Palettes.rainbowPlus
    }
    LinearGradient {
        visible: root.colorValue === "neon"
        anchors.fill: sourceText
        source: sourceText
        start: Qt.point(0, 0)
        end: Qt.point(sourceText.contentWidth, 0)
        gradient: Palettes.neon
    }
}
