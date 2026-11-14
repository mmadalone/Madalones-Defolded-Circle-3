// Copyright (c) 2026 madalone. v1.6.4 — the REAL MatrixRain item keeps drawing while its speed keeps changing.
// A slide on the touch slider changes the screensaver's speed every 3 raw units, so the item gets a speed write every
// few tens of milliseconds. Up to v1.6.3 each write restarted the tick timer, and while the writes came faster than
// one tick the item drew no frame at all (1 frame in 1.2 s instead of about 14, measured in the v1.6.4 pre-release
// review). The item asks for a frame only from its tick, so the window's frames count the ticks.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtTest 1.2
import MatrixRain 1.0

Item {
    id: root
    width: 480
    height: 850

    MatrixRain {
        id: rain
        anchors.fill: parent
        running: true
        color: "#00ff41"
        colorMode: "green"
        speed: 1.0
        density: 0.7
        trailLength: 25
        fontSize: 16
        charset: "ascii"
    }

    property int frames: 0
    Connections {
        target: root.Window.window
        function onFrameSwapped() { root.frames++; }
    }

    // one speed write per interval, one step of the screensaver's Animation speed (matrixSpeed N -> speed N / 50,
    // what BindingHelper passes on), always in the same direction, as a slide does
    Timer {
        id: writer
        repeat: true
        property int n: 50
        property int step: -1
        property int left: 0
        onTriggered: {
            if (left <= 0) {
                stop();
                return;
            }
            n = Math.max(10, Math.min(100, n + step));
            rain.speed = n / 50.0;
            left--;
        }
    }

    TestCase {
        name: "SpeedWrites"
        when: windowShown

        function framesDuring(ms) {
            var f0 = root.frames;
            wait(ms);
            return root.frames - f0;
        }

        function cleanup() {
            writer.stop();
        }

        function test_frames_keep_coming_data() {
            return [
                { tag: "slower, a write every 30 ms", step: -1, every: 30, writes: 40 },
                { tag: "faster, a write every 30 ms", step: 1, every: 30, writes: 40 },
                { tag: "slower, a write every 60 ms", step: -1, every: 60, writes: 20 }
            ];
        }

        // the tick interval at Animation speed n, as MatrixRainItem::speedIntervalMs() has it (50 ms / (n / 50))
        function intervalMs(n) { return Math.max(25, Math.min(300, Math.floor(2500 / n))); }

        // the ticks a slide should give if each new speed simply took over from the next tick on
        function expectedTicks(n0, step, writes, everyMs) {
            var ticks = 0;
            var n = n0;
            for (var i = 0; i < writes; ++i) {
                n = Math.max(10, Math.min(100, n + step));
                ticks += everyMs / intervalMs(n);
            }
            return ticks;
        }

        // while the writes come, the item draws about as many frames as the changing speed asks for. The frames per
        // tick are measured first at a steady speed 50, so a slow machine lowers both sides.
        function test_frames_keep_coming(data) {
            rain.speed = 1.0;
            wait(500);
            var span = data.writes * data.every;
            var steady = framesDuring(span);
            verify(steady >= 8, "the rain draws at a steady speed (" + steady + " frames in " + span + " ms)");
            var framesPerTick = steady / (span / intervalMs(50));

            writer.n = 50;
            writer.step = data.step;
            writer.left = data.writes;
            writer.interval = data.every;
            var f1 = root.frames;
            writer.start();
            wait(span);
            var during = root.frames - f1;
            writer.stop();

            var expected = expectedTicks(50, data.step, data.writes, data.every) * framesPerTick;
            verify(during >= expected * 0.7,
                   "frames keep coming while the speed changes every " + data.every + " ms: " + during + " in "
                   + span + " ms, about " + Math.round(expected) + " expected (" + steady + " at a steady speed 50)");
        }
    }
}
