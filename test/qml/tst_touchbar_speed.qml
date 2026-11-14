// Copyright (c) 2026 madalone. v1.6.4 — the touch slider on the screensaver: Matrix speed and Starfield star density
// (ChargingScreen.qml, the rule in src/ui/screensaverTouchbar.h). Runs the REAL ChargingScreen.qml, loaded
// asynchronously like main.qml's chargingScreenLoader, with the real seek overlay (TouchSliderSeek.qml) on a fake Kodi
// player behind it, and drives the slider through MockTouchSlider (press, move, release). Every raw unit of travel
// counts, one step per 3, however the remote groups the movement; up to v1.6.3 a slow slide changed nothing.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Config 1.0
import ScreensaverConfig 1.0
import TouchSlider 1.0
import TestLog 1.0

import "qrc:/components" as Components

Item {
    id: root
    width: 480
    height: 850

    // stands in for main.qml's ApplicationWindow: screensaverActive follows the screensaver's Loader
    Item {
        id: applicationWindow
        property bool screensaverActive: ssLoader.active
    }

    // a Kodi player behind the screensaver, with its seek overlay (the touch slider of the owner's Kodi activity)
    QtObject {
        id: kodi
        property bool enabled: true
        property string name: "Kodi"
        property int mediaPosition: 600
        property int mediaDuration: 3600
        property int sent: 0
        signal entityEnabledChanged()
        function seek(v) { sent++; mediaPosition = v; }
    }

    Components.TouchSliderSeek {
        id: seekSlider
        entityObj: kodi
    }

    Loader {
        id: ssLoader
        anchors.fill: parent
        active: false
        asynchronous: true
        source: "qrc:/components/ChargingScreen.qml"
        onStatusChanged: if (status === Loader.Ready) item.open()

        Connections {
            target: ssLoader.item
            ignoreUnknownSignals: true
            function onClosed() { ssLoader.active = false; }
        }
    }

    SignalSpy { id: labelSpy; signalName: "visibleChanged" }
    SignalSpy { id: speedSpy; target: ScreensaverConfig; signalName: "matrixSpeedChanged" }

    TestCase {
        name: "TouchbarSpeed"
        when: windowShown

        property var cs: null
        property var label: null

        // the red "Speed: N" / "Density: N" label of ChargingScreen.qml
        function findLabel(popup) {
            var kids = popup.contentItem.children;
            var found = null;
            for (var i = 0; i < kids.length; ++i) {
                var k = kids[i];
                if (k.z === 9999 && k.text !== undefined && k.font !== undefined && Qt.colorEqual(k.color, "#ff3333")) {
                    verify(found === null, "only one Text matches the speed label");
                    found = k;
                }
            }
            return found;
        }

        function openScreensaver() {
            ssLoader.active = true;
            tryVerify(function() { return ssLoader.item !== null && ssLoader.item.opened; }, 5000, "screensaver open");
            cs = ssLoader.item;
            label = findLabel(cs);
            verify(label !== null, "found the speed label");
            compare(label.visible, false, "the speed label starts hidden");
            labelSpy.target = label;
            labelSpy.clear();
        }

        function labelShown() { return labelSpy.count > 0 || label.visible; }

        // slider positions from `from` in moves of the given signed sizes, repeated `times`
        function path(from, sizes, times) {
            var xs = [];
            var x = from;
            for (var t = 0; t < times; ++t) {
                for (var i = 0; i < sizes.length; ++i) {
                    x += sizes[i];
                    xs.push(x);
                }
            }
            return xs;
        }

        function moveSeq(xs) {
            for (var i = 0; i < xs.length; ++i) {
                TouchSliderProcessor.move(xs[i]);
                wait(2);
            }
        }

        // one slide on the open screensaver: touch down at pressX, the moves, lift; returns what changed
        function slide(pressX, xs) {
            var rec = {
                touchbarSpeedActive: cs.touchbarSpeedActive,
                net: xs.length ? xs[xs.length - 1] - pressX : 0
            };
            var typeErrorsBefore = TestLog.typeErrors();
            labelSpy.clear();
            speedSpy.clear();
            TouchSliderProcessor.press(pressX);
            moveSeq(xs);
            rec.speedAfter = ScreensaverConfig.matrixSpeed;
            rec.densityAfter = ScreensaverConfig.starfieldDensity;
            rec.speedWrites = speedSpy.count;
            rec.labelShown = labelShown();
            rec.labelText = label.text;
            TouchSliderProcessor.release();
            compare(TestLog.typeErrors() - typeErrorsBefore, 0, "no TypeError");
            return rec;
        }

        function init() {
            // Matrix, speed 50, density 50, DPAD interactive on, Touchbar speed on, Touch directions off
            ScreensaverConfig.resetDefaults();
            Config.resetDefaults();   // the global "Touch slider" switch on
            kodi.mediaPosition = 600;
            kodi.sent = 0;
        }

        function cleanup() {
            TouchSliderProcessor.release();
            if (ssLoader.item) ssLoader.item.close();
            else ssLoader.active = false;
            tryVerify(function() { return !ssLoader.active; }, 3000, "screensaver closed");
            labelSpy.target = null;
            cs = null;
            label = null;
            wait(30);
        }

        // 60 raw units toward a higher slider value (slower), however the remote groups them: always 20 steps
        function test_sixty_units_any_grouping_data() {
            return [
                { tag: "60 x 1 (slow)", sizes: [1], times: 60 },
                { tag: "30 x 2", sizes: [2], times: 30 },
                { tag: "20 x 3", sizes: [3], times: 20 },
                { tag: "1,2,1 x 15", sizes: [1, 2, 1], times: 15 },
                { tag: "4,2 x 10", sizes: [4, 2], times: 10 },
                { tag: "15 x 4", sizes: [4], times: 15 },
                { tag: "12 x 5", sizes: [5], times: 12 },
                { tag: "6 x 10 (quick)", sizes: [10], times: 6 },
                { tag: "1 x 60 (flick)", sizes: [60], times: 1 }
            ];
        }
        function test_sixty_units_any_grouping(data) {
            openScreensaver();
            var r = slide(200, path(200, data.sizes, data.times));
            compare(r.net, 60);
            compare(r.speedAfter, 30, data.tag + ": one step per 3 raw units");
            compare(r.labelShown, true);
            compare(r.labelText, "Speed: 30");
        }

        // the other way (the volume-down end) is faster
        function test_slow_slide_other_way() {
            openScreensaver();
            var r = slide(260, path(260, [-1], 60));
            compare(r.speedAfter, 70);
            compare(r.labelText, "Speed: 70");
        }

        // a resting finger: jitter changes nothing and writes nothing
        function test_jitter_data() {
            return [ { tag: "+1/-1 x 50", sizes: [1, -1] }, { tag: "+2/-2 x 50", sizes: [2, -2] } ];
        }
        function test_jitter(data) {
            openScreensaver();
            var r = slide(150, path(150, data.sizes, 50));
            compare(r.speedAfter, 50);
            compare(r.speedWrites, 0);
            compare(r.labelShown, false);
        }

        // a slow drift with jitter counts its net travel: (+2, -1) x 30 = 30 raw units = 10 steps slower
        function test_slow_drift() {
            openScreensaver();
            var r = slide(150, path(150, [2, -1], 30));
            compare(r.speedAfter, 40);
        }

        // v1.4.8's rate: the full 100 -> 10 sweep is 270 raw units, also when slow
        function test_full_sweep_slow() {
            ScreensaverConfig.matrixSpeed = 100;
            openScreensaver();
            var r = slide(100, path(100, [1], 270));
            compare(r.speedAfter, 10);
        }

        // at an end stop the travel is used up, so turning back responds at once
        function test_end_stop() {
            ScreensaverConfig.matrixSpeed = 98;
            openScreensaver();
            labelSpy.clear();
            TouchSliderProcessor.press(150);
            moveSeq(path(150, [-1], 30));
            var atStop = ScreensaverConfig.matrixSpeed;
            moveSeq(path(120, [1], 3));
            var after = ScreensaverConfig.matrixSpeed;
            TouchSliderProcessor.release();
            compare(atStop, 100, "pinned at 100");
            compare(after, 99, "3 units back: one step slower");
        }

        // each slide starts from zero: 2 units, lift, 2 units make no step
        function test_each_slide_starts_from_zero() {
            openScreensaver();
            slide(150, [152]);
            var b = slide(152, [154]);
            compare(b.speedAfter, 50);
            compare(b.speedWrites, 0);
        }

        // Starfield: the same rule on the star density
        function test_starfield_slow_slide() {
            ScreensaverConfig.theme = "starfield";
            openScreensaver();
            var r = slide(200, path(200, [1], 60));
            compare(r.densityAfter, 30);
            compare(r.speedAfter, 50);
            compare(r.labelText, "Density: 30");
        }

        // the switches still decide; Settings lets Touch directions be on with DPAD interactive still on
        function test_switches_turn_it_off_data() {
            return [ { tag: "Touch directions on", obj: ScreensaverConfig, key: "tapDirection", value: true },
                     { tag: "DPAD interactive off", obj: ScreensaverConfig, key: "dpadEnabled", value: false },
                     { tag: "Touchbar speed off", obj: ScreensaverConfig, key: "dpadTouchbarSpeed", value: false },
                     { tag: "Touch slider off (Settings > Touch Slider)", obj: Config, key: "touchSliderEnabled",
                       value: false } ];
        }
        function test_switches_turn_it_off(data) {
            data.obj[data.key] = data.value;
            openScreensaver();
            var r = slide(200, path(200, [10], 6));
            compare(r.touchbarSpeedActive, false);
            compare(r.speedAfter, 50);
            compare(r.labelShown, false);
        }

        // the global "Touch slider" switch off also turns off Starfield's density control
        function test_touch_slider_off_starfield() {
            ScreensaverConfig.theme = "starfield";
            Config.touchSliderEnabled = false;
            openScreensaver();
            var r = slide(200, path(200, [10], 6));
            compare(r.touchbarSpeedActive, false);
            compare(r.densityAfter, 50);
        }

        // a slide that began before the screensaver opened stays ended (the seek overlay sends nothing more); a new
        // touch then works, also slowly
        function test_slide_started_before_then_new_slow_slide() {
            verify(!ssLoader.active, "no screensaver");
            TouchSliderProcessor.press(200);
            moveSeq(path(200, [1], 10));
            wait(250);
            ssLoader.active = true;
            tryVerify(function() { return ssLoader.item !== null && ssLoader.item.opened; }, 5000, "screensaver open");
            cs = ssLoader.item;
            label = findLabel(cs);
            labelSpy.target = label;
            labelSpy.clear();
            var seeksBefore = kodi.sent;
            moveSeq(path(210, [10], 6));
            wait(250);
            var speedSameTouch = ScreensaverConfig.matrixSpeed;
            TouchSliderProcessor.release();
            var r = slide(270, path(270, [1], 60));
            compare(speedSameTouch, 50, "the slide under way when it opened changes nothing");
            compare(r.speedAfter, 30, "a new slow slide does");
            compare(kodi.sent - seeksBefore, 0, "no seek under the screensaver");
        }
    }
}
