// Braille Avatar v6 — Living portrait through light, color, and atmosphere
import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: win
    width: 480; height: 850
    visible: true; title: "Braille Avatar — Ambient Life"
    color: "black"

    property bool talking: true
    property string mood: "neutral" // neutral, happy, angry, sleepy, mischievous, excited
    property var moods: ["neutral", "happy", "angry", "sleepy", "mischievous", "excited"]
    property int moodIndex: 0
    property int fpsCount: 0
    property int lastFps: 0

    // Mood color map
    function moodColor(m) {
        switch(m) {
            case "happy": return "#4de8b0";
            case "angry": return "#ff4444";
            case "sleepy": return "#6a6aaa";
            case "mischievous": return "#d4a04e";
            case "excited": return "#44ddff";
            default: return "#97d5e0";
        }
    }
    function moodGlow(m) {
        switch(m) {
            case "happy": return "#1a4de8b0";
            case "angry": return "#22ff2222";
            case "sleepy": return "#0d4444aa";
            case "mischievous": return "#18d4a04e";
            case "excited": return "#2244ddff";
            default: return "#1297d5e0";
        }
    }

    property string rickArt: [
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2840\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28ff\u28c6\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28e0\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28ff\u28ff\u28e6\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2880\u28fc\u28ff\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28ff\u28ff\u28ff\u28f7\u2844\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2880\u28f4\u28ff\u28ff\u28ff\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28ff\u28ff\u28ff\u28ff\u28ff\u28c6\u2800\u2800\u2800\u2800\u2800\u2880\u28f4\u28ff\u28ff\u28ff\u28ff\u28ff\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28a0\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28e7\u2800\u2800\u2800\u28f4\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2847\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28b8\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28e7\u28c0\u28fe\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2801\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u28b9\u28ff\u28f6\u28e6\u28e4\u28c0\u2840\u2800\u2800\u2800\u2800\u2800\u28fc\u28ff\u28ff\u28ff\u287f\u283f\u281f\u281b\u281b\u283f\u283f\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u285f\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u28bf\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28f6\u28f6\u28e4\u28e4\u287f\u281f\u2809\u28b4\u28f6\u28ff\u28ff\u28ff\u28ff\u28ff\u28f7\u28e6\u28cd\u283b\u28ff\u28ff\u28ff\u2847\u2800\u2800\u2800\u2800\u2800\u28c0\u28c0\u28e0\u28e4\u28f6\u2876",
        "\u2800\u2800\u2800\u2800\u2800\u2808\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u287f\u281f\u28cb\u28c0\u28d9\u287b\u28b6\u28dd\u28bf\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28cc\u283b\u28ff\u28f7\u28f6\u28f6\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u280f\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2818\u28ff\u28ff\u28ff\u28ff\u28ff\u280f\u28f4\u28ff\u287f\u283f\u28bf\u28ff\u28e6\u2859\u28a6\u28fd\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2867\u2839\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u287f\u2801\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28b9\u28ff\u28ff\u28ff\u28ff\u2846\u2889\u28e5\u28f6\u28fe\u28f6\u28cc\u283b\u28ff\u28ce\u283b\u28ff\u28ff\u28ff\u287f\u281f\u28cb\u28ed\u28f4\u28f6\u2844\u28b9\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u285f\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28bf\u28ff\u28ff\u28ff\u2883\u28ff\u28ff\u287f\u283f\u283f\u283f\u28e7\u2859\u28bf\u28f7\u28f6\u28f6\u28f6\u28f6\u28ff\u283f\u281f\u280b\u28e9\u28f4\u284c\u28ff\u28ff\u28ff\u28ff\u28ff\u285f\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2880\u28f8\u28ff\u28ff\u285f\u28b8\u281f\u28e1\u28f6\u28fe\u28ff\u28ff\u28f6\u28cc\u2832\u28ec\u28c9\u2809\u28c9\u28e5\u28f4\u28fe\u28ff\u28f7\u28e6\u2859\u28e7\u28b9\u28ff\u28ff\u28ff\u281f\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2880\u28e0\u28f4\u28fe\u28ff\u28ff\u28ff\u28ff\u2847\u284e\u28fc\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2809\u28a2\u28b9\u287f\u28b0\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2809\u28f3\u2808\u28b8\u28ff\u28ff\u284b\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2820\u28f4\u28fe\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2881\u2847\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28b8\u2847\u28fe\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2800\u28b8\u28ff\u28ff\u28ff\u28f7\u28f6\u28e4\u28c4\u28c0\u28c0\u2800",
        "\u2800\u2800\u2809\u283b\u28bf\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28b8\u2847\u28bf\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2807\u28fc\u28e7\u2838\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u287f\u28a0\u28b8\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u281f\u2801",
        "\u2800\u2800\u2800\u2800\u2800\u2808\u281b\u28bf\u28ff\u28ff\u28ff\u28b8\u28ff\u28cc\u283b\u28bf\u28ff\u28ff\u28ff\u287f\u288b\u28fc\u28ff\u28ff\u28e7\u2859\u283f\u28ff\u28ff\u28ff\u287f\u281f\u28e1\u28ff\u28b8\u28ff\u28ff\u28ff\u28ff\u28ff\u287f\u280b\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u28e0\u28fe\u28ff\u28ff\u28ff\u28fe\u28ff\u28ff\u28ff\u28f6\u28e4\u28e4\u28e4\u28f6\u28ff\u280b\u28ff\u28ff\u28bb\u28ff\u28f7\u28f6\u28e4\u28f4\u28f6\u28ff\u28ff\u28ff\u28b8\u28ff\u28ff\u28ff\u287f\u280b\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u28e0\u28fe\u28ff\u28ff\u28ff\u28ff\u28ff\u28b9\u28ff\u28f7\u28ec\u28db\u28db\u281b\u28db\u28e9\u28fd\u2800\u28ff\u28ff\u2880\u28f7\u28ec\u28d9\u285b\u281b\u28db\u28eb\u28f4\u28ff\u28b8\u28ff\u28ff\u285f\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2820\u28fe\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u281f\u28b8\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2847\u28ff\u28ff\u28b8\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2818\u28bf\u28ff\u28f7\u2840\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2808\u2819\u283b\u28bf\u28ff\u28ff\u2883\u28fe\u2818\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28cc\u28e1\u28fe\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u2800\u28e6\u2859\u28ff\u28ff\u28f7\u28e4\u28c0\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2808\u28ff\u28b8\u28ff\u2847\u28ff\u28ff\u28ff\u28ff\u287f\u28bf\u28ff\u287f\u28bb\u28df\u28b9\u285f\u28bb\u28df\u283b\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28ff\u28b8\u28ff\u2847\u28ff\u28ff\u28ff\u283f\u281f\u2801\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28b0\u28ff\u28e6\u2859\u2807\u28b8\u28ff\u28ff\u285f\u2870\u2801\u2808\u2801\u2800\u2801\u2800\u2800\u2800\u2801\u2800\u2809\u2800\u2819\u28cc\u28bb\u28ff\u28ff\u2818\u28cb\u28f4\u2809\u2801\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2880\u28ff\u28ff\u28ff\u28ff\u28f7\u284c\u28ff\u28ff\u28b0\u2847\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2818\u284c\u28ff\u2847\u28fc\u28ff\u28ff\u2846\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2818\u281b\u281b\u281b\u281b\u283b\u28f7\u2839\u28ff\u2838\u28e7\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28a0\u2847\u28ff\u28a1\u28ff\u28ff\u28ff\u28f7\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2880\u28ff\u28c7\u28bb\u28e7\u2859\u283f\u2836\u2834\u28a6\u2840\u2836\u28f6\u28f6\u2876\u2806\u28a0\u28e4\u2834\u288f\u28f4\u2883\u284e\u2800\u2808\u2809\u2809\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2818\u281f\u281b\u2800\u283b\u28ff\u28f7\u28f6\u28fe\u28ff\u28ff\u2847\u28b9\u280f\u28f4\u28f6\u28f6\u28f6\u28f6\u287f\u2803\u281a\u2807\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2808\u283b\u28ff\u28ff\u28ff\u28ff\u28ff\u28e4\u28fe\u28ff\u28ff\u28ff\u28ff\u281f\u2801\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800",
        "\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2808\u2819\u281b\u281b\u283b\u283f\u283f\u281b\u281b\u2809\u2801\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800"
    ].join("\n")

    Timer { interval: 1000; running: true; repeat: true; onTriggered: { lastFps = fpsCount; fpsCount = 0 } }

    // Cycle moods every 6s
    Timer {
        interval: 6000; running: true; repeat: true
        onTriggered: {
            moodIndex = (moodIndex + 1) % moods.length;
            mood = moods[moodIndex];
            moodTransition.restart();
        }
    }

    // Toggle talking every 4-7s
    Timer {
        interval: 5000; running: true; repeat: true
        onTriggered: { talking = !talking; interval = talking ? (3000+Math.random()*4000) : (2000+Math.random()*3000) }
    }

    // ---- Background glow ----
    Rectangle {
        id: bgGlow
        anchors.centerIn: portrait
        width: portrait.width + 80; height: portrait.height + 80
        radius: 40; color: "transparent"
        border.width: 0

        Rectangle {
            anchors.fill: parent; anchors.margins: -20; radius: 50
            color: moodGlow(mood); opacity: 0.8
            Behavior on color { ColorAnimation { duration: 1500 } }
        }

        // Breathing glow
        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation { to: 0.6; duration: 2500; easing.type: Easing.InOutSine }
            NumberAnimation { to: 1.0; duration: 2500; easing.type: Easing.InOutSine }
        }
    }

    // ---- The portrait (static, untouched) ----
    Text {
        id: portrait
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -40
        text: rickArt
        font.family: "Menlo"; font.pixelSize: 14
        lineHeight: 0.9; lineHeightMode: Text.ProportionalHeight

        // Base color animates with mood
        color: moodColor(mood)
        Behavior on color { ColorAnimation { duration: 1500; easing.type: Easing.InOutQuad } }

        // Subtle breathing scale
        transform: Scale {
            id: breathScale
            origin.x: portrait.width / 2; origin.y: portrait.height / 2
            xScale: 1.0; yScale: 1.0

            SequentialAnimation on yScale {
                loops: Animation.Infinite
                NumberAnimation { to: 1.008; duration: 3000; easing.type: Easing.InOutSine }
                NumberAnimation { to: 0.992; duration: 3000; easing.type: Easing.InOutSine }
            }
        }
    }

    // Eye blinks removed — looked awful

    // ---- Voice visualizer (below portrait) ----
    Row {
        id: voiceViz
        anchors.top: portrait.bottom; anchors.topMargin: 15
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 3
        opacity: talking ? 1.0 : 0.15
        Behavior on opacity { NumberAnimation { duration: 300 } }

        Repeater {
            model: 16
            Rectangle {
                id: bar
                width: 4; radius: 2
                color: moodColor(mood)
                Behavior on color { ColorAnimation { duration: 1500 } }

                property real phase: index * 0.4
                property real barHeight: talking
                    ? 6 + Math.abs(Math.sin(voiceTimer.elapsed * 0.008 + phase)) * 22
                    : 3

                height: barHeight
                anchors.bottom: parent ? undefined : undefined
                y: 30 - height

                Behavior on height { NumberAnimation { duration: 80 } }
            }
        }
    }

    // Voice animation driver
    Item {
        id: voiceTimer
        property real elapsed: 0
        Timer {
            interval: 50; running: true; repeat: true
            onTriggered: { voiceTimer.elapsed += 50; fpsCount++ }
        }
    }

    // ---- Mood transition flash ----
    Rectangle {
        id: moodFlash
        anchors.fill: portrait
        color: moodColor(mood); opacity: 0; radius: 8
    }
    SequentialAnimation {
        id: moodTransition
        NumberAnimation { target: moodFlash; property: "opacity"; to: 0.4; duration: 150 }
        NumberAnimation { target: moodFlash; property: "opacity"; to: 0; duration: 600; easing.type: Easing.OutQuad }
    }

    // ---- Ambient particles (subtle floating dots) ----
    Repeater {
        model: 12
        Rectangle {
            id: particle
            property real startX: Math.random() * 480
            property real startY: Math.random() * 850
            x: startX; y: startY
            width: 2; height: 2; radius: 1
            color: moodColor(mood); opacity: 0.15 + Math.random() * 0.15
            Behavior on color { ColorAnimation { duration: 2000 } }

            SequentialAnimation on y {
                loops: Animation.Infinite
                NumberAnimation { to: particle.startY - 40 - Math.random() * 60; duration: 4000 + Math.random() * 4000; easing.type: Easing.InOutSine }
                NumberAnimation { to: particle.startY; duration: 4000 + Math.random() * 4000; easing.type: Easing.InOutSine }
            }
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { to: 0.05; duration: 3000 + Math.random() * 3000 }
                NumberAnimation { to: 0.2; duration: 3000 + Math.random() * 3000 }
            }
        }
    }

    // ---- Status display ----
    Text {
        anchors.bottom: voiceViz.bottom; anchors.bottomMargin: -35
        anchors.horizontalCenter: parent.horizontalCenter
        text: mood.toUpperCase() + (talking ? "  •  TALKING" : "")
        color: moodColor(mood); opacity: 0.6
        Behavior on color { ColorAnimation { duration: 1500 } }
        font.pixelSize: 14; font.family: "Menlo"; font.bold: true
    }

    // FPS
    Text {
        anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 10
        text: "FPS: " + lastFps
        color: "#333"; font.pixelSize: 11; font.family: "Menlo"
    }
}
