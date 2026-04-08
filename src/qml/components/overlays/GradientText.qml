// Copyright (c) 2024 madalone. Reusable text element supporting solid color or rainbow gradient.
// colorValue accepts a hex string ("#ffffff") or a gradient mode ("rainbow", "rainbow_gradient", "neon").
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtGraphicalEffects 1.0

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

    // Rainbow gradient (5 stops)
    LinearGradient {
        visible: root.colorValue === "rainbow"
        anchors.fill: sourceText
        source: sourceText
        start: Qt.point(0, 0)
        end: Qt.point(sourceText.contentWidth, 0)
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#ff0000" }
            GradientStop { position: 0.25; color: "#ffbf00" }
            GradientStop { position: 0.5; color: "#00ff41" }
            GradientStop { position: 0.75; color: "#0000ff" }
            GradientStop { position: 1.0; color: "#ff0000" }
        }
    }

    // Rainbow+ gradient (11 stops, smoother)
    LinearGradient {
        visible: root.colorValue === "rainbow_gradient"
        anchors.fill: sourceText
        source: sourceText
        start: Qt.point(0, 0)
        end: Qt.point(sourceText.contentWidth, 0)
        gradient: Gradient {
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
    }

    // Neon gradient (11 stops, pastel)
    LinearGradient {
        visible: root.colorValue === "neon"
        anchors.fill: sourceText
        source: sourceText
        start: Qt.point(0, 0)
        end: Qt.point(sourceText.contentWidth, 0)
        gradient: Gradient {
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
    }
}
