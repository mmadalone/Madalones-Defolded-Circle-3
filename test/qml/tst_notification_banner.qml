// Copyright (c) 2026 madalone. v1.6.3 — QML tests for the banner (components/Notification.qml): the remote's other
// banners and the error banner (in place of an error pop-up), each with its duration from Settings → User interface →
// Messages; a banner shown whatever "Show offline banners" says (Notification applies it, in C++); a banner replacing
// the one on screen.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Config 1.0

import "qrc:/components" as Components

Item {
    id: testRoot
    width: 480
    height: 850

    // when the banner finished opening (its display time starts there), when it last started closing, and how often
    // it did. The durations are measured from these signals, not with wait(), so a wait that runs long doesn't skew
    // them; the waits before a replacement leave the first banner at least 0.6 s.
    property real openedAt: 0
    property real closingAt: 0
    property int closings: 0

    // as main.qml has it
    Components.Notification {
        id: banner
    }

    Connections {
        target: banner
        function onOpened() { testRoot.openedAt = Date.now(); }
        function onAboutToHide() {
            testRoot.closingAt = Date.now();
            testRoot.closings++;
        }
    }

    TestCase {
        name: "NotificationBanner"
        when: windowShown

        function init() {
            Config.resetDefaults();
            testRoot.openedAt = 0;
            testRoot.closingAt = 0;
            testRoot.closings = 0;
        }

        function cleanup() {
            banner.close();
            tryCompare(banner, "visible", false, 3000, "the banner closed after the test");
            Config.resetDefaults();
        }

        // what Notification::createNotification() and Notification::createErrorBanner() emit
        function showOther(message, warning) {
            ui.notification.emitNotificationCreated(message, warning);
        }
        function showError(message, warning) {
            ui.notification.emitErrorBannerCreated(message, warning);
        }

        function messageText() {
            var kids = banner.contentItem.children;
            for (var i = 0; i < kids.length; i++) {
                if (kids[i].text !== undefined) return kids[i].text;
            }
            return undefined;
        }

        function colour() {
            return String(banner.contentItem.color);
        }

        function waitOpened(message) {
            tryCompare(banner, "opened", true, 3000, message || "the banner opened");
        }

        // how long the banner stayed after the given moment: waits until it starts closing, which it does only once
        function shownSince(since, timeout, message) {
            tryCompare(banner, "opened", false, timeout, message);
            compare(testRoot.closings, 1, "the banner closed once");
            return testRoot.closingAt - since;
        }

        // The lower bound is the "still open" check: the banner did not start closing before about expectedMs.
        // Its timer never fires early; the software backend under xvfb can make it late, hence the wide upper margin.
        function verifyAbout(actualMs, expectedMs, message) {
            verify(actualMs >= expectedMs - 150 && actualMs <= expectedMs + 1500,
                   message + ": " + actualMs + " ms, expected about " + expectedMs);
        }

        // ── message and colour ──────────────────────────────────────

        // the neutral and the warning colour
        function colourRows() {
            return [
                { tag: "neutral", warning: false, colour: "buttonPrimary" },
                { tag: "warning", warning: true, colour: "red" }
            ];
        }

        function test_otherBanner_showsTheMessage_data() {
            return colourRows();
        }

        function test_otherBanner_showsTheMessage(data) {
            showOther("Living room TV is unavailable", data.warning);
            verify(banner.visible, "opens at once");
            waitOpened();
            compare(messageText(), "Living room TV is unavailable");
            compare(colour(), String(colors[data.colour]), "the " + data.tag + " colour");
        }

        function test_errorBanner_showsTheMessage_data() {
            return colourRows();
        }

        function test_errorBanner_showsTheMessage(data) {
            showError("Kodi is not responding", data.warning);
            verify(banner.visible, "opens at once");
            waitOpened();
            compare(messageText(), "Kodi is not responding");
            compare(colour(), String(colors[data.colour]), "the " + data.tag + " colour");
        }

        // ── durations ───────────────────────────────────────────────

        // upstream's fixed 4 s stays the other banners' default; the error banner starts at 5 s
        function test_defaults() {
            showOther("Other", false);
            waitOpened();
            compare(banner.displayMs, 4000, "other banners: 4 s, as before");
            showError("Error", true);
            compare(banner.displayMs, 5000, "the error banner: 5 s");
        }

        function test_otherBanner_closesAfterItsDuration() {
            Config.otherBannerSec = 1;
            Config.errorBannerSec = 30;   // the error banner's duration must not apply
            showOther("One second", false);
            waitOpened();
            compare(banner.displayMs, 1000);
            verifyAbout(shownSince(testRoot.openedAt, 4000, "closes after about 1 s"), 1000, "shown");
            tryCompare(banner, "visible", false, 2000, "gone after the exit transition");
        }

        function test_errorBanner_closesAfterItsDuration() {
            Config.errorBannerSec = 2;
            Config.otherBannerSec = 30;   // the other banners' duration must not apply
            showError("Two seconds", true);
            waitOpened();
            compare(banner.displayMs, 2000);
            // still open at 1.4 s and closed by about 3 s: it starts closing about 2 s after it opened
            verifyAbout(shownSince(testRoot.openedAt, 5000, "closed by about 3 s"), 2000, "shown");
        }

        // ── "Show offline banners" ──────────────────────────────────

        // The switch is applied in C++: Notification::createUnavailableBanner() emits nothing while it is off, and every
        // other banner always shows. So the banner shows whatever it is sent with the switch either way, and a change of
        // the switch leaves the banner on screen alone; the error banner doesn't read it either.
        function test_banner_showsWhateverTheUnavailableSwitch_data() {
            return [
                { tag: "unavailable banners on", unavailable: true },
                { tag: "unavailable banners off", unavailable: false }
            ];
        }

        function test_banner_showsWhateverTheUnavailableSwitch(data) {
            Config.showUnavailableBanners = data.unavailable;
            Config.otherBannerSec = 7;
            showOther("The setting could not be changed", true);
            verify(banner.visible, "a banner opens at once");
            waitOpened();
            compare(messageText(), "The setting could not be changed");
            compare(colour(), String(colors.red), "in the warning colour");
            compare(banner.displayMs, 7000, "with the other banners' duration");

            Config.showUnavailableBanners = !data.unavailable;
            verify(banner.opened, "still open after the switch changed");
            compare(messageText(), "The setting could not be changed", "keeps its text");
            compare(banner.displayMs, 7000, "and its duration");

            showError("Kodi is not responding", false);
            compare(messageText(), "Kodi is not responding", "the error banner replaces it");
            compare(colour(), String(colors.buttonPrimary), "in the neutral colour");
            compare(banner.displayMs, 5000, "with the error banner's duration");
            verify(banner.opened, "the banner stays open");
        }

        // ── a banner replacing the one on screen ────────────────────

        // the error banner's shorter duration replaces what was left of the other banner's
        function test_replacing_errorOverOther_takesTheErrorDuration() {
            Config.otherBannerSec = 5;
            Config.errorBannerSec = 1;
            showOther("First", false);
            waitOpened();
            wait(300);
            verify(banner.opened, "the first banner still open");

            var replacedAt = Date.now();
            showError("Second", true);
            compare(messageText(), "Second", "the text is replaced at once");
            compare(colour(), String(colors.red), "in the error colour");
            compare(banner.displayMs, 1000, "with the error banner's duration");
            verify(banner.opened, "the banner stays open");
            verifyAbout(shownSince(replacedAt, 4000, "closes about 1 s after the replacement"), 1000,
                        "closed after the replacement (not the 4.7 s left of the first)");
        }

        // the other banner's longer duration restarts the timer, past the end of the error banner's
        function test_replacing_otherOverError_restartsWithTheOtherDuration() {
            Config.errorBannerSec = 1;
            Config.otherBannerSec = 2;
            showError("First", true);
            waitOpened();
            wait(400);
            verify(banner.opened, "the first banner still open");

            var replacedAt = Date.now();
            showOther("Second", false);
            compare(messageText(), "Second", "the text is replaced at once");
            compare(colour(), String(colors.buttonPrimary), "in the neutral colour");
            compare(banner.displayMs, 2000, "with the other banners' duration");
            // the first had about 0.6 s left: the banner stays for the other banners' full 2 s from the replacement
            verifyAbout(shownSince(replacedAt, 5000, "closes about 2 s after the replacement"), 2000,
                        "closed after the replacement");
        }

        // upstream's rule, now per duration: the same kind again gets its full time, not what was left
        function test_replacing_sameKind_restartsTheTimer() {
            Config.otherBannerSec = 2;
            showOther("First", false);
            waitOpened();
            wait(800);
            verify(banner.opened, "the first banner still open");

            var replacedAt = Date.now();
            showOther("Second", false);
            compare(messageText(), "Second", "the text is replaced at once");
            // the first had about 1.2 s left: the second gets its full 2 s
            verifyAbout(shownSince(replacedAt, 5000, "closes about 2 s after the replacement"), 2000,
                        "closed after the replacement");
        }
    }
}
