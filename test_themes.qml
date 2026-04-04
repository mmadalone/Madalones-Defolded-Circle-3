// Standalone theme previewer — runs without the remote-core backend
import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: window
    width: 480
    height: 850
    visible: true
    title: "Charging Screen Theme Preview"
    color: "black"

    property string currentTheme: "matrix"

    // Mock objects that the themes expect from the remote-ui context
    QtObject {
        id: mockColors
        property color offwhite: "#F5F5F5"
        property color black: "#000000"
        property color red: "#FF0000"
    }

    QtObject {
        id: mockUi
        property var time: new Date()
        property int width: 480
        property int height: 850
    }

    // Update time every second
    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: mockUi.time = new Date()
    }

    // ===== Matrix Theme (inline for standalone testing) =====
    Item {
        id: matrixTheme
        anchors.fill: parent
        visible: currentTheme === "matrix"

        property color matrixColor: "#00ff41"
        property real speed: 1.0
        property real density: 0.7

        readonly property string charset: "\u30A0\u30A1\u30A2\u30A3\u30A4\u30A5\u30A6\u30A7\u30A8\u30A9\u30AA\u30AB\u30AC\u30AD\u30AE\u30AF\u30B0\u30B1\u30B2\u30B3\u30B4\u30B5\u30B6\u30B7\u30B8\u30B9\u30BA\u30BB\u30BC\u30BD\u30BE\u30BF\u30C0\u30C1\u30C2\u30C3\u30C4\u30C5\u30C6\u30C7\u30C8\u30C9\u30CA\u30CB\u30CC\u30CD\u30CE\u30CF\u30D0\u30D1\u30D2\u30D3\u30D4\u30D5\u30D6\u30D7\u30D8\u30D9\u30DA\u30DB\u30DC\u30DD\u30DE\u30DF\u30E0\u30E1\u30E2\u30E3\u30E4\u30E5\u30E6\u30E7\u30E8\u30E9\u30EA\u30EB\u30EC\u30ED\u30EE\u30EF0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

        readonly property int fontSize: 16
        readonly property int colWidth: fontSize + 2

        Canvas {
            id: matrixCanvas
            anchors.fill: parent
            renderTarget: Canvas.FramebufferObject
            renderStrategy: Canvas.Cooperative

            property var drops: []
            property bool initialized: false
            property int numCols: Math.floor(width / matrixTheme.colWidth)

            onPaint: {
                var ctx = getContext("2d");

                if (!initialized) {
                    ctx.fillStyle = "black";
                    ctx.fillRect(0, 0, width, height);
                    drops = [];
                    for (var i = 0; i < numCols; i++) {
                        drops.push(Math.random() * (-height / matrixTheme.fontSize));
                    }
                    initialized = true;
                }

                ctx.fillStyle = "rgba(0, 0, 0, 0.05)";
                ctx.fillRect(0, 0, width, height);

                ctx.font = matrixTheme.fontSize + "px monospace";

                for (var col = 0; col < drops.length; col++) {
                    if (col / drops.length > matrixTheme.density && col % 3 !== 0) continue;

                    var charIndex = Math.floor(Math.random() * matrixTheme.charset.length);
                    var ch = matrixTheme.charset[charIndex];
                    var x = col * matrixTheme.colWidth;
                    var y = drops[col] * matrixTheme.fontSize;

                    ctx.fillStyle = Qt.lighter(matrixTheme.matrixColor, 1.8);
                    ctx.fillText(ch, x, y);

                    if (drops[col] > 1) {
                        var prevChar = matrixTheme.charset[Math.floor(Math.random() * matrixTheme.charset.length)];
                        ctx.fillStyle = "" + matrixTheme.matrixColor;
                        ctx.fillText(prevChar, x, y - matrixTheme.fontSize);
                    }

                    drops[col] += matrixTheme.speed * (0.5 + Math.random() * 0.5);

                    if (y > matrixCanvas.height && Math.random() > 0.975) {
                        drops[col] = 0;
                    }
                }
            }
        }

        Timer {
            interval: 55
            running: matrixTheme.visible
            repeat: true
            onTriggered: matrixCanvas.requestPaint()
        }
    }

    // ===== Starfield Theme (inline) =====
    Item {
        id: starfieldTheme
        anchors.fill: parent
        visible: currentTheme === "starfield"

        property int starCount: 200
        property real speed: 1.0

        Canvas {
            id: starCanvas
            anchors.fill: parent
            renderTarget: Canvas.FramebufferObject
            renderStrategy: Canvas.Cooperative

            property var stars: []
            property bool initialized: false
            property real cx: width / 2
            property real cy: height / 2

            onPaint: {
                var ctx = getContext("2d");

                if (!initialized) {
                    ctx.fillStyle = "black";
                    ctx.fillRect(0, 0, width, height);
                    stars = [];
                    for (var i = 0; i < starfieldTheme.starCount; i++) {
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

                ctx.fillStyle = "rgba(0, 0, 0, 0.15)";
                ctx.fillRect(0, 0, width, height);

                for (var s = 0; s < stars.length; s++) {
                    var star = stars[s];
                    star.z -= starfieldTheme.speed * 4;

                    if (star.z <= 0) {
                        star.x = Math.random() * width - cx;
                        star.y = Math.random() * height - cy;
                        star.z = width;
                        star.pz = star.z;
                        continue;
                    }

                    var sx = (star.x / star.z) * width + cx;
                    var sy = (star.y / star.z) * height + cy;
                    var px = (star.x / star.pz) * width + cx;
                    var py = (star.y / star.pz) * height + cy;
                    star.pz = star.z;

                    if (sx < 0 || sx > width || sy < 0 || sy > height) continue;

                    var brightness = 1 - star.z / width;
                    var size = brightness * 3;

                    ctx.beginPath();
                    ctx.moveTo(px, py);
                    ctx.lineTo(sx, sy);
                    ctx.strokeStyle = "rgba(255, 255, 255, " + (brightness * 0.8) + ")";
                    ctx.lineWidth = size * 0.5;
                    ctx.stroke();

                    ctx.beginPath();
                    ctx.arc(sx, sy, size * 0.5, 0, Math.PI * 2);
                    ctx.fillStyle = "rgba(255, 255, 255, " + brightness + ")";
                    ctx.fill();
                }
            }
        }

        Timer {
            interval: 55
            running: starfieldTheme.visible
            repeat: true
            onTriggered: starCanvas.requestPaint()
        }
    }

    // ===== Minimal Theme (inline) =====
    Item {
        id: minimalTheme
        anchors.fill: parent
        visible: currentTheme === "minimal"

        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -60
            color: "#F5F5F5"
            font.pixelSize: 96
            font.family: "Helvetica"

            text: {
                var h = mockUi.time.getHours();
                var m = mockUi.time.getMinutes();
                var hStr = h < 10 ? "0" + h : "" + h;
                var mStr = m < 10 ? "0" + m : "" + m;
                return hStr + ":" + mStr;
            }
        }

        Text {
            anchors {
                bottom: parent.verticalCenter
                bottomMargin: 60
                horizontalCenter: parent.horizontalCenter
            }
            color: Qt.rgba(1, 1, 1, 0.4)
            font.pixelSize: 20
            font.family: "Helvetica"
            text: {
                var d = mockUi.time;
                var days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
                var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
                return days[d.getDay()] + ", " + months[d.getMonth()] + " " + d.getDate();
            }
        }
    }

    // ===== Shared Overlays =====

    // Digital clock overlay (for matrix and starfield)
    Text {
        visible: currentTheme !== "minimal"
        anchors {
            top: parent.top
            topMargin: parent.height * 0.15
            horizontalCenter: parent.horizontalCenter
        }
        color: Qt.rgba(1, 1, 1, 0.85)
        font.pixelSize: 48
        font.family: "Helvetica"

        text: {
            var h = mockUi.time.getHours();
            var m = mockUi.time.getMinutes();
            var hStr = h < 10 ? "0" + h : "" + h;
            var mStr = m < 10 ? "0" + m : "" + m;
            return hStr + ":" + mStr;
        }
    }

    // Battery indicator (all themes)
    Row {
        anchors {
            bottom: parent.bottom
            bottomMargin: 40
            horizontalCenter: parent.horizontalCenter
        }
        spacing: 10

        Text {
            text: "\u26A1"
            font.pixelSize: 28
            color: "#F5F5F5"
        }

        Text {
            color: "#F5F5F5"
            font.pixelSize: 24
            font.family: "Helvetica"
            text: "78% - Charging"
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // ===== Theme Switcher (bottom buttons) =====
    Row {
        anchors {
            bottom: parent.bottom
            bottomMargin: 5
            horizontalCenter: parent.horizontalCenter
        }
        spacing: 8

        Repeater {
            model: ["matrix", "starfield", "minimal"]
            Rectangle {
                width: 80; height: 26
                radius: 4
                color: currentTheme === modelData ? "#00ff41" : "#333333"

                Text {
                    anchors.centerIn: parent
                    text: modelData
                    color: currentTheme === modelData ? "black" : "white"
                    font.pixelSize: 11
                    font.family: "Helvetica"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        // Reset canvas state when switching themes
                        if (modelData === "matrix") matrixCanvas.initialized = false;
                        if (modelData === "starfield") starCanvas.initialized = false;
                        currentTheme = modelData;
                    }
                }
            }
        }
    }
}
