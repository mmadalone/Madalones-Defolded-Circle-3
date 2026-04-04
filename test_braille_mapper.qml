// Face Region Mapper — Drag rectangles around eyes and mouth. Press U to undo.
import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: win
    width: 700; height: 950
    visible: true; title: "Drag to select face regions — U to undo"
    color: "#111111"

    property var labelSequence: ["LEFT EYE", "RIGHT EYE", "MOUTH"]
    property int labelIndex: 0
    property string currentLabel: labelSequence[0]
    property var regions: []  // [{name, row0, col0, row1, col1}]

    property real charW: 0
    property real charH: 0
    property bool dragging: false
    property real dragStartX: 0
    property real dragStartY: 0

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

    Component.onCompleted: {
        var lines = rickArt.split('\n');
        var cols = Array.from(lines[0]).length;
        console.log("Grid: " + lines.length + " rows x " + cols + " cols");
    }

    // Keyboard: U = undo last region
    Item {
        focus: true
        Keys.onPressed: {
            if (event.key === Qt.Key_U && regions.length > 0) {
                var removed = regions.pop();
                console.log("Undid: " + removed.name);
                labelIndex = Math.max(0, labelIndex - 1);
                currentLabel = labelSequence[labelIndex];
                // Remove visual rect
                if (rectVisuals.length > 0) {
                    var r = rectVisuals.pop();
                    r.destroy();
                }
            }
        }
    }
    property var rectVisuals: []

    Text {
        id: brailleText
        x: 40; y: 70
        text: rickArt
        color: "#97d5e0"
        font.family: "Menlo"; font.pixelSize: 14
        lineHeight: 0.9; lineHeightMode: Text.ProportionalHeight

        Component.onCompleted: {
            var lines = rickArt.split('\n');
            var cols = Array.from(lines[0]).length;
            charW = width / cols;
            charH = height / lines.length;
            console.log("Char: " + charW.toFixed(1) + "x" + charH.toFixed(1) + "px");
        }

        // Drag selection rectangle
        Rectangle {
            id: selRect
            visible: dragging
            color: "transparent"
            border.color: currentLabel.indexOf("EYE") >= 0 ? "#ff4444" : "#44ff44"
            border.width: 2
        }

        MouseArea {
            anchors.fill: parent

            onPressed: {
                if (labelIndex >= labelSequence.length) return;
                dragging = true;
                dragStartX = mouse.x;
                dragStartY = mouse.y;
                selRect.x = mouse.x;
                selRect.y = mouse.y;
                selRect.width = 0;
                selRect.height = 0;
            }

            onPositionChanged: {
                if (!dragging) return;
                var x0 = Math.min(dragStartX, mouse.x);
                var y0 = Math.min(dragStartY, mouse.y);
                var x1 = Math.max(dragStartX, mouse.x);
                var y1 = Math.max(dragStartY, mouse.y);
                selRect.x = x0; selRect.y = y0;
                selRect.width = x1 - x0; selRect.height = y1 - y0;
            }

            onReleased: {
                if (!dragging || labelIndex >= labelSequence.length) { dragging = false; return; }
                dragging = false;

                var x0 = Math.min(dragStartX, mouse.x);
                var y0 = Math.min(dragStartY, mouse.y);
                var x1 = Math.max(dragStartX, mouse.x);
                var y1 = Math.max(dragStartY, mouse.y);

                var col0 = Math.floor(x0 / charW);
                var row0 = Math.floor(y0 / charH);
                var col1 = Math.ceil(x1 / charW);
                var row1 = Math.ceil(y1 / charH);

                var label = labelSequence[labelIndex];
                var region = {name: label, row0: row0, col0: col0, row1: row1, col1: col1};
                regions.push(region);

                console.log(label + ": rows " + row0 + "-" + row1 + ", cols " + col0 + "-" + col1);

                // Persistent visual rectangle
                var color = label.indexOf("EYE") >= 0 ? "#ff444466" : "#44ff4466";
                var borderColor = label.indexOf("EYE") >= 0 ? "#ff4444" : "#44ff44";
                var rect = Qt.createQmlObject(
                    'import QtQuick 2.15; Rectangle { ' +
                    'x: ' + (col0 * charW) + '; y: ' + (row0 * charH) + '; ' +
                    'width: ' + ((col1 - col0) * charW) + '; height: ' + ((row1 - row0) * charH) + '; ' +
                    'color: "' + color + '"; border.color: "' + borderColor + '"; border.width: 2; ' +
                    'Text { anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 2; ' +
                    'text: "' + label + '"; color: "' + borderColor + '"; font.pixelSize: 9; font.family: "Menlo" } }',
                    brailleText, "regionRect");
                rectVisuals.push(rect);

                labelIndex++;
                if (labelIndex < labelSequence.length) {
                    currentLabel = labelSequence[labelIndex];
                } else {
                    currentLabel = "DONE";
                    var output = "\n=== FACE REGIONS ===\n";
                    for (var i = 0; i < regions.length; i++) {
                        var r = regions[i];
                        output += r.name + ": rows " + r.row0 + "-" + r.row1 + ", cols " + r.col0 + "-" + r.col1 + "\n";
                    }
                    output += "====================\n";
                    console.log(output);
                    resultText.text = output;
                }

                selRect.width = 0; selRect.height = 0;
            }
        }
    }

    // Instructions
    Rectangle {
        anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter; anchors.topMargin: 10
        color: "#cc000000"; radius: 6; width: instrText.width + 30; height: instrText.height + 14
        Text {
            id: instrText; anchors.centerIn: parent
            text: labelIndex < labelSequence.length
                ? "DRAG AROUND: " + currentLabel + "  (U = undo)"
                : "DONE — Copy the output below"
            color: labelIndex < labelSequence.length
                ? (currentLabel.indexOf("EYE") >= 0 ? "#ff4444" : "#44ff44")
                : "#ffdd00"
            font.pixelSize: 20; font.family: "Menlo"; font.bold: true
        }
    }

    // Results
    Text {
        id: resultText
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 15
        text: "Drag rectangles around:\n1. Left eye\n2. Right eye\n3. Mouth\n\nPress U to undo"
        color: "#aaaaaa"; font.pixelSize: 13; font.family: "Menlo"
        horizontalAlignment: Text.AlignHCenter
    }
}
