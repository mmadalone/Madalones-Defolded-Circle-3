// Copyright (c) 2026 madalone. v1.6.4 — the touch slider overlays (components/TouchSlider{Volume,Brightness,Position,
// Seek}.qml) and the screensaver, which takes the touch slider over while it shows: a slide under way when it opens
// ends there (the value the slider showed is sent once, only if the device doesn't have it yet, then nothing more), a
// slide under it never reaches the device, and a lift with no slide under way sends nothing but still closes the
// overlay. Also upstream's Math.rodun typo in the brightness overlay.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import TouchSlider 1.0
import TestLog 1.0

// the four slider overlays under test
import "qrc:/components" as Sliders

Item {
    id: testRoot
    width: 480
    height: 850

    // stands in for main.qml's ApplicationWindow (id: applicationWindow): its screensaverActive is true while the
    // screensaver's Loader is active
    Item {
        id: applicationWindow
        property bool screensaverActive: false
    }

    // a device: counts every command and takes the value it was sent at once
    Component {
        id: deviceComponent
        QtObject {
            property bool enabled: true
            property string name: "Test device"
            property int volume: 20
            property int brightness: 60
            property int position: 30
            property int mediaPosition: 100
            property int mediaDuration: 1000
            property int sent: 0
            // entity::Base announces availability changes as entityEnabledChanged (src/ui/entity/entity.h)
            signal entityEnabledChanged()
            onEnabledChanged: entityEnabledChanged()
            function setVolume(v) { sent++; volume = v; }
            function setBrightness(v) { sent++; brightness = v; }
            function setPosition(v) { sent++; position = v; }
            function seek(v) { sent++; mediaPosition = v; }
        }
    }

    Component { id: volumeComponent; Sliders.TouchSliderVolume {} }
    Component { id: brightnessComponent; Sliders.TouchSliderBrightness {} }
    Component { id: positionComponent; Sliders.TouchSliderPosition {} }
    Component { id: seekComponent; Sliders.TouchSliderSeek {} }

    SignalSpy { id: openSpy; signalName: "open" }
    SignalSpy { id: closeSpy; signalName: "close" }

    TestCase {
        name: "TouchSliderScreensaver"
        when: windowShown

        // The devices outlive the sliders: temporary objects are deleted in creation order, and a slider whose device
        // went first re-evaluates its bindings on a null entityObj (TypeError warnings at teardown).
        property var devices: []

        function init() {
            applicationWindow.screensaverActive = false;
        }

        function cleanup() {
            applicationWindow.screensaverActive = false;
        }

        function cleanupTestCase() {
            for (var i = 0; i < devices.length; i++) {
                devices[i].destroy();
            }
            devices = [];
        }

        function variants() {
            return [
                { tag: "volume", component: volumeComponent, prop: "volume", target: "targetVolume" },
                { tag: "brightness", component: brightnessComponent, prop: "brightness", target: "targetBrightness" },
                { tag: "position", component: positionComponent, prop: "position", target: "targetPosition" },
                { tag: "seek", component: seekComponent, prop: "mediaPosition", target: "targetPosition" },
            ];
        }

        function make(data) {
            var device = deviceComponent.createObject(testRoot);
            devices.push(device);
            var slider = createTemporaryObject(data.component, testRoot, { "entityObj": device });
            verify(slider, "slider created");
            openSpy.target = slider;
            openSpy.clear();
            closeSpy.target = slider;
            closeSpy.clear();
            return { device: device, slider: slider };
        }

        function test_slide_ends_when_screensaver_opens_data() { return variants(); }

        // The fix: the screensaver ends a slide that is under way, and the device is left alone afterwards
        function test_slide_ends_when_screensaver_opens(data) {
            var o = make(data);
            var e = o.device, s = o.slider;

            // a slide before the screensaver changes the device, as everywhere
            TouchSliderProcessor.press(200);
            compare(openSpy.count, 1, "the overlay opened");
            TouchSliderProcessor.move(400);
            tryVerify(function() { return e.sent > 0; }, 2000, "the slide changed the device before the screensaver");

            // one more move, then the screensaver opens mid-slide
            TouchSliderProcessor.move(500);
            var reached = s[data.target];
            var sentBeforeOpen = e.sent;   // no event loop runs before the opening, so the 200 ms timer can't send
            applicationWindow.screensaverActive = true;
            compare(e[data.prop], reached, "the value the slider showed was sent when the screensaver opened");
            compare(e.sent, sentBeforeOpen + 1, "and sent once");
            compare(s.touchSliderActive, false, "the slide ended when the screensaver opened");
            compare(closeSpy.count, 1, "the overlay closed when the screensaver opened");
            var sentAtOpen = e.sent;

            // the rest of that slide, under the screensaver
            TouchSliderProcessor.move(700);
            TouchSliderProcessor.move(900);
            wait(300);
            compare(e.sent, sentAtOpen, "the rest of the slide sent nothing");
            compare(e[data.prop], reached, "the device kept the value the slider showed");
            TouchSliderProcessor.release();
            wait(300);
            compare(e.sent, sentAtOpen, "the lift under the screensaver sent nothing");

            // the device is changed by other means: nothing sets it back
            e[data.prop] = 10;
            wait(300);
            compare(e.sent, sentAtOpen, "a change by other means is left alone");
            compare(e[data.prop], 10, "the device keeps the other change");

            // a new slide under the screensaver belongs to the screensaver
            TouchSliderProcessor.press(800);
            TouchSliderProcessor.move(300);
            TouchSliderProcessor.move(100);
            wait(300);
            TouchSliderProcessor.release();
            wait(300);
            compare(e.sent, sentAtOpen, "a slide under the screensaver sent nothing");
            compare(openSpy.count, 1, "and opened no overlay");

            // after the screensaver the slider works as before
            applicationWindow.screensaverActive = false;
            TouchSliderProcessor.press(200);
            compare(openSpy.count, 2, "the overlay opens again");
            TouchSliderProcessor.move(600);
            tryVerify(function() { return e.sent > sentAtOpen; }, 2000, "the slider changes the device again");
            TouchSliderProcessor.release();
            compare(s.touchSliderActive, false, "the slide ended on the lift");
            tryCompare(closeSpy, "count", 2, 2000, "the overlay closed 1 s after the lift");
        }

        function test_slide_started_under_screensaver_data() { return variants(); }

        // v1.4.7's case: a slide that starts while the screensaver shows never reaches the device
        function test_slide_started_under_screensaver(data) {
            var o = make(data);
            var e = o.device, s = o.slider;
            applicationWindow.screensaverActive = true;
            TouchSliderProcessor.press(200);
            TouchSliderProcessor.move(600);
            TouchSliderProcessor.move(900);
            wait(300);
            TouchSliderProcessor.release();
            wait(300);
            compare(e.sent, 0, "nothing was sent");
            compare(openSpy.count, 0, "no overlay opened");
            compare(s.touchSliderActive, false, "no slide started");
        }

        function test_device_unavailable_mid_slide_data() { return variants(); }

        // The device became unavailable during the slide: the screensaver sends it nothing and still ends the slide
        function test_device_unavailable_mid_slide(data) {
            var o = make(data);
            var e = o.device, s = o.slider;
            TouchSliderProcessor.press(200);
            TouchSliderProcessor.move(400);
            tryVerify(function() { return e.sent > 0; }, 2000, "the slide changed the device");
            TouchSliderProcessor.move(500);
            e.enabled = false;
            var sentBefore = e.sent;
            applicationWindow.screensaverActive = true;
            compare(e.sent, sentBefore, "nothing was sent to the unavailable device");
            compare(s.touchSliderActive, false, "the slide ended");
            compare(closeSpy.count, 1, "the overlay closed");
            e.enabled = true;
            e[data.prop] = 10;
            wait(300);
            compare(e.sent, sentBefore, "the 200 ms updates stopped");
        }

        function test_screensaver_closes_before_lift_data() { return variants(); }

        // The screensaver opens and closes again while the finger stays down (e.g. a dock and a lift with Close on
        // wake set to Undocked); the device is changed by other means; then the finger lifts
        function test_screensaver_closes_before_lift(data) {
            var o = make(data);
            var e = o.device, s = o.slider;
            TouchSliderProcessor.press(200);
            TouchSliderProcessor.move(400);
            tryVerify(function() { return e.sent > 0; }, 2000, "the slide changed the device");
            TouchSliderProcessor.move(500);
            applicationWindow.screensaverActive = true;
            applicationWindow.screensaverActive = false;
            e[data.prop] = 10;
            var sentBefore = e.sent;
            TouchSliderProcessor.move(600);
            TouchSliderProcessor.release();
            wait(300);
            compare(e.sent, sentBefore, "the late lift sent nothing");
            compare(e[data.prop], 10, "the other change stays");
            compare(openSpy.count, 1, "no overlay opened again");
        }

        function test_screensaver_without_slide_data() { return variants(); }

        // No slide under way: opening the screensaver does nothing to the slider
        function test_screensaver_without_slide(data) {
            var o = make(data);
            var e = o.device, s = o.slider;
            TouchSliderProcessor.press(200);
            TouchSliderProcessor.move(600);
            TouchSliderProcessor.release();
            tryCompare(closeSpy, "count", 1, 2000, "the overlay closed after the lift");
            var sentAfterSlide = e.sent;
            e[data.prop] = 10;
            applicationWindow.screensaverActive = true;
            wait(300);
            compare(e.sent, sentAfterSlide, "opening the screensaver sent nothing");
            compare(closeSpy.count, 1, "and closed nothing more");
            compare(e[data.prop], 10, "the device's value is untouched");
        }

        function test_release_after_refused_press_data() { return variants(); }

        // The press came while the device was unavailable (the slider refused it), and the device came back before
        // the lift: the lift sends nothing. Upstream's lift sent the slider's last target again, 0 before any slide.
        function test_release_after_refused_press(data) {
            var o = make(data);
            var e = o.device, s = o.slider;
            var initial = e[data.prop];
            var popUpsBefore = ui.actionableNotifications;
            e.enabled = false;
            TouchSliderProcessor.press(200);
            compare(ui.actionableNotifications, popUpsBefore + 1, "the slider said the device is not available");
            compare(s.touchSliderActive, false, "the press started no slide");
            compare(openSpy.count, 0, "and opened no overlay");
            e.enabled = true;
            TouchSliderProcessor.move(600);
            TouchSliderProcessor.release();
            wait(300);
            compare(e.sent, 0, "the lift sent nothing");
            compare(e[data.prop], initial, "the device keeps its value");
        }

        function test_screensaver_opens_when_device_holds_target_data() { return variants(); }

        // The device already has the value the slider shows: the screensaver ends the slide and sends nothing
        function test_screensaver_opens_when_device_holds_target(data) {
            var o = make(data);
            var e = o.device, s = o.slider;
            TouchSliderProcessor.press(200);
            TouchSliderProcessor.move(400);
            tryVerify(function() { return e.sent > 0 && e[data.prop] === s[data.target]; }, 2000,
                      "the device took the value the slider shows");
            var sentBefore = e.sent;
            applicationWindow.screensaverActive = true;
            compare(e.sent, sentBefore, "nothing sent: the device already has the value");
            compare(s.touchSliderActive, false, "the slide ended");
            compare(closeSpy.count, 1, "the overlay closed");
        }

        function test_release_by_overlay_loaded_after_press_data() { return variants(); }

        // The overlay was loaded while the finger was already down (the device's features arriving after the press,
        // a screen opened meanwhile): it never saw the press, so its lift sends nothing (upstream's sent its initial 0),
        // and it still closes 1 s after the lift, like a release
        function test_release_by_overlay_loaded_after_press(data) {
            TouchSliderProcessor.press(200);
            var o = make(data);
            var e = o.device, s = o.slider;
            var initial = e[data.prop];
            TouchSliderProcessor.move(600);
            TouchSliderProcessor.release();
            wait(300);
            compare(e.sent, 0, "the lift sent nothing");
            compare(e[data.prop], initial, "the device keeps its value");
            tryCompare(closeSpy, "count", 1, 2000, "the overlay closes 1 s after the lift");
        }

        function test_release_after_press_refused_under_screensaver_data() { return variants(); }

        // The press came while the screensaver showed, and the screensaver closed before the lift (a tap, a button,
        // Close on wake): the lift sends nothing (v1.6.3 sent the slider's last target, 0 before any slide)
        function test_release_after_press_refused_under_screensaver(data) {
            var o = make(data);
            var e = o.device, s = o.slider;
            var initial = e[data.prop];
            applicationWindow.screensaverActive = true;
            TouchSliderProcessor.press(200);
            applicationWindow.screensaverActive = false;
            TouchSliderProcessor.move(600);
            TouchSliderProcessor.release();
            wait(300);
            compare(e.sent, 0, "the lift sent nothing");
            compare(e[data.prop], initial, "the device keeps its value");
            compare(openSpy.count, 0, "no overlay opened");
        }

        // Upstream's Math.rodun typo: a light's brightness changing while no slide is under way threw a TypeError
        // in the brightness overlay
        function test_brightness_change_without_slide() {
            var e = deviceComponent.createObject(testRoot);
            devices.push(e);
            var s = createTemporaryObject(brightnessComponent, testRoot, { "entityObj": e });
            verify(s, "slider created");
            var before = TestLog.typeErrors();
            e.brightness = 128;
            compare(TestLog.typeErrors(), before, "no TypeError when the light's brightness changed");
        }
    }
}
