// Standalone MatrixRain preview — keyboard arrows change direction, Enter triggers chaos.
import QtQuick 2.15
import QtQuick.Window 2.15
import MatrixRain 1.0

Window {
    id: root
    width: 480; height: 850
    visible: true
    title: "Matrix Rain Preview"
    color: "black"

    property bool gravityActive: false

    MatrixRain {
        id: rain
        anchors.fill: parent
        running: true
        displayOff: false
        gravityMode: root.gravityActive

        // Defaults — tweak here to test different configs
        speed: 1.0
        density: 0.7
        trailLength: 25
        charset: "ascii"
        colorMode: "classic"
        color: "#00ff41"
        glow: true
        glitch: true
        glitchRate: 15
        glitchDirection: true
        glitchDirRate: 8
        glitchChaos: true
        depthIntensity: 100
        depthOverlay: true
    }

    // Direction label
    Text {
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 10 }
        color: "#00ff4180"
        font.pixelSize: 14
        text: "Arrows=dir | Enter=chaos | R=restore | G=gravity"
        z: 10
    }

    Text {
        id: dirLabel
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 10 }
        color: "#00ff41"
        font.pixelSize: 18
        font.bold: true
        z: 10
    }

    // Keyboard input — focus must be on an Item, not Window
    Item {
        id: inputHandler
        anchors.fill: parent
        focus: true

        Keys.onPressed: {
            var action = "";
            if (event.key === Qt.Key_Up)    action = "up";
            if (event.key === Qt.Key_Down)  action = "down";
            if (event.key === Qt.Key_Left)  action = "left";
            if (event.key === Qt.Key_Right) action = "right";

            if (action !== "") {
                root.gravityActive = true;
                rain.interactiveInput(action);
                dirLabel.text = "Direction: " + action + " [gravity]";
                event.accepted = true;
                return;
            }

            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                rain.interactiveInput("enter");
                dirLabel.text = "CHAOS!";
                event.accepted = true;
            }
            if (event.key === Qt.Key_R) {
                rain.interactiveInput("restore");
                root.gravityActive = false;
                dirLabel.text = "Restored default direction";
                event.accepted = true;
            }
            if (event.key === Qt.Key_G) {
                root.gravityActive = !root.gravityActive;
                dirLabel.text = "Gravity: " + (root.gravityActive ? "ON" : "OFF");
                event.accepted = true;
            }
            if (event.key === Qt.Key_D) {
                rain.depthEnabled = !rain.depthEnabled;
                dirLabel.text = "Depth: " + (rain.depthEnabled ? "ON" : "OFF");
                event.accepted = true;
            }
            if (event.key === Qt.Key_L) {
                rain.layersEnabled = !rain.layersEnabled;
                dirLabel.text = "Layers: " + (rain.layersEnabled ? "ON" : "OFF");
                event.accepted = true;
            }
        }
    }
}
