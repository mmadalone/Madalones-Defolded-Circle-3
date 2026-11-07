// Copyright (c) 2026 madalone. v1.6.3 — QML tests for Settings → User interface → Messages (MessagesSection.qml): the
// rows each switch shows ("Show offline banners" and the other banner duration always show), the dividers, the keypad
// chain through the rows and its two ends, OK on the switches, the two duration sliders, and the reveal of the row
// around the focused control.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import Config 1.0

import "qrc:/settings/settings/userinterface" as UserInterface

Item {
    id: testRoot
    width: 480
    height: 850

    // stands in for Ui.qml's Settings.Page: records what the section asks it to reveal
    Item {
        id: stubSettingsPage
        property var revealed: []
        function ensureVisible(item) {
            revealed.push(item);
        }
    }

    // holds the focus between tests, outside the section
    Item {
        id: parking
        width: 10; height: 10
    }

    Column {
        width: parent.width

        // the control above the section: Ui.qml's "Show source picker button", whose DOWN is firstFocusItem
        Item {
            id: stubAbove
            objectName: "stubAbove"
            width: 40; height: 10
            KeyNavigation.down: section.firstFocusItem
        }

        UserInterface.MessagesSection {
            id: section
            width: parent.width
            settingsPage: stubSettingsPage
            navUpTarget: stubAbove
            navDownTarget: stubBelow
        }

        // a control below the section; Ui.qml has none (test_nothingBelow covers that), this one checks navDownTarget
        // and lastFocusItem
        Item {
            id: stubBelow
            objectName: "stubBelow"
            width: 40; height: 10
            KeyNavigation.up: section.lastFocusItem
        }
    }

    TestCase {
        name: "MessagesSection"
        when: windowShown

        // top to bottom
        readonly property var controlNames: ["errorPopupsSwitch", "errorBannerSwitch", "errorBannerSlider",
                                             "unavailableBannersSwitch", "otherBannerSlider"]

        // the settings the section writes, for "only its own setting changed"
        readonly property var settingNames: ["showErrorPopups", "showErrorBanner", "errorBannerSec",
                                             "showUnavailableBanners", "otherBannerSec"]

        function init() {
            Config.resetDefaults();
            ui.keyNavigationActive = true;
            section.navDownTarget = stubBelow;
            parking.forceActiveFocus();
            stubSettingsPage.revealed = [];
        }

        function cleanup() {
            ui.keyNavigationActive = true;
            section.navDownTarget = stubBelow;
            Config.resetDefaults();
        }

        function findByObjectName(item, objName) {
            if (item.objectName === objName) return item;
            var kids = item.children || [];
            for (var i = 0; i < kids.length; i++) {
                var found = findByObjectName(kids[i], objName);
                if (found) return found;
            }
            return null;
        }

        function findText(item, text) {
            if (item.text === text) return item;
            var kids = item.children || [];
            for (var i = 0; i < kids.length; i++) {
                var found = findText(kids[i], text);
                if (found) return found;
            }
            return null;
        }

        function control(objName) {
            var c = objName === "stubAbove" ? stubAbove : objName === "stubBelow" ? stubBelow
                                                                                  : findByObjectName(section, objName);
            verify(c, objName + " found");
            return c;
        }

        // the SettingRow around a control
        function rowOf(item) {
            for (var p = item.parent; p; p = p.parent) {
                if (p.showDivider !== undefined && p.controlBelow !== undefined) return p;
            }
            return null;
        }

        // the direct child of the section around an item: what the section reveals
        function sectionChildOf(item) {
            for (var p = item; p && p.parent; p = p.parent) {
                if (p.parent === section) return p;
            }
            return null;
        }

        function apply(data) {
            Config.showErrorPopups = data.popups;
            Config.showErrorBanner = data.banner;
            Config.showUnavailableBanners = data.unavailable;
        }

        function shownControls() {
            var shown = [];
            for (var i = 0; i < controlNames.length; i++) {
                if (control(controlNames[i]).visible) shown.push(controlNames[i]);
            }
            return shown;
        }

        function settingsSnapshot() {
            var s = {};
            for (var i = 0; i < settingNames.length; i++) s[settingNames[i]] = Config[settingNames[i]];
            return s;
        }

        function lastRevealed() {
            var r = stubSettingsPage.revealed;
            return r.length > 0 ? r[r.length - 1] : null;
        }

        function focusOn(item, message) {
            item.forceActiveFocus();
            tryVerify(function() { return item.activeFocus; }, 2000, message);
        }

        // The section is a ColumnLayout: it moves its rows on the next polish, not when a row is shown or hidden, so
        // positions read straight after a switch change belong to the previous state. Wait until the shown rows stack.
        function waitForLayout() {
            tryVerify(function() {
                var y = 0;
                for (var i = 0; i < section.children.length; i++) {
                    var row = section.children[i];
                    if (!row.visible) continue;
                    if (Math.abs(row.y - y) > 0.5) return false;
                    y += row.height;
                }
                return true;
            }, 2000, "the section's rows laid out");
        }

        // the focus reached item; inside the section, the keypad also revealed the row around it (deferred to the end
        // of the event, after UC's own reveal of the control)
        function expectFocus(item, message) {
            tryVerify(function() { return item.activeFocus; }, 2000, message);
            var row = sectionChildOf(item);
            if (row) {
                tryVerify(function() { return lastRevealed() === row; }, 2000, message + ": the row around it revealed");
            }
            // the next step has to reveal its row again; it can't pass on this step's record
            stubSettingsPage.revealed = [];
        }

        // ── rows and dividers ───────────────────────────────────────

        // "Show offline banners" hides no row: it and the other banner duration always show
        function test_rows_data() {
            return [
                { tag: "defaults", popups: true, banner: true, unavailable: true,
                  shown: ["errorPopupsSwitch", "unavailableBannersSwitch", "otherBannerSlider"] },
                { tag: "error pop-ups off", popups: false, banner: true, unavailable: true,
                  shown: ["errorPopupsSwitch", "errorBannerSwitch", "errorBannerSlider", "unavailableBannersSwitch",
                          "otherBannerSlider"] },
                { tag: "error pop-ups off, banner off", popups: false, banner: false, unavailable: true,
                  shown: ["errorPopupsSwitch", "errorBannerSwitch", "unavailableBannersSwitch", "otherBannerSlider"] },
                { tag: "banner off with the pop-ups on", popups: true, banner: false, unavailable: true,
                  shown: ["errorPopupsSwitch", "unavailableBannersSwitch", "otherBannerSlider"] },
                { tag: "unavailable banners off", popups: true, banner: true, unavailable: false,
                  shown: ["errorPopupsSwitch", "unavailableBannersSwitch", "otherBannerSlider"] },
                { tag: "error pop-ups off, unavailable banners off", popups: false, banner: true, unavailable: false,
                  shown: ["errorPopupsSwitch", "errorBannerSwitch", "errorBannerSlider", "unavailableBannersSwitch",
                          "otherBannerSlider"] },
                { tag: "everything off", popups: false, banner: false, unavailable: false,
                  shown: ["errorPopupsSwitch", "errorBannerSwitch", "unavailableBannersSwitch", "otherBannerSlider"] }
            ];
        }

        function test_rows(data) {
            apply(data);
            compare(shownControls().join(", "), data.shown.join(", "));
            // the whole row hides with its control: title and help text go with it
            for (var i = 0; i < controlNames.length; i++) {
                var c = control(controlNames[i]);
                compare(rowOf(c).visible, c.visible, controlNames[i] + ": its row and the control agree");
            }
        }

        // one divider closes the error group, under its last shown row; "Show offline banners" has its own; the other
        // banner duration, last on the page, has none
        function test_dividers_data() {
            return [
                { tag: "defaults", popups: true, banner: true, unavailable: true,
                  dividers: ["errorPopupsSwitch", "unavailableBannersSwitch"] },
                { tag: "error pop-ups off", popups: false, banner: true, unavailable: true,
                  dividers: ["errorBannerSlider", "unavailableBannersSwitch"] },
                { tag: "error pop-ups off, banner off", popups: false, banner: false, unavailable: true,
                  dividers: ["errorBannerSwitch", "unavailableBannersSwitch"] },
                { tag: "banner off with the pop-ups on", popups: true, banner: false, unavailable: true,
                  dividers: ["errorPopupsSwitch", "unavailableBannersSwitch"] },
                { tag: "unavailable banners off", popups: true, banner: true, unavailable: false,
                  dividers: ["errorPopupsSwitch", "unavailableBannersSwitch"] },
                { tag: "everything off", popups: false, banner: false, unavailable: false,
                  dividers: ["errorBannerSwitch", "unavailableBannersSwitch"] }
            ];
        }

        function test_dividers(data) {
            apply(data);
            var withDivider = [];
            for (var i = 0; i < controlNames.length; i++) {
                var row = rowOf(control(controlNames[i]));
                if (row.visible && row.showDivider) withDivider.push(controlNames[i]);
            }
            compare(withDivider.join(", "), data.dividers.join(", "));
        }

        // ── keypad ──────────────────────────────────────────────────

        function test_keypadChain_data() {
            return [
                { tag: "defaults", popups: true, banner: true, unavailable: true,
                  chain: ["stubAbove", "errorPopupsSwitch", "unavailableBannersSwitch", "otherBannerSlider",
                          "stubBelow"] },
                { tag: "error pop-ups off", popups: false, banner: true, unavailable: true,
                  chain: ["stubAbove", "errorPopupsSwitch", "errorBannerSwitch", "errorBannerSlider",
                          "unavailableBannersSwitch", "otherBannerSlider", "stubBelow"] },
                { tag: "error pop-ups off, banner off", popups: false, banner: false, unavailable: true,
                  chain: ["stubAbove", "errorPopupsSwitch", "errorBannerSwitch", "unavailableBannersSwitch",
                          "otherBannerSlider", "stubBelow"] },
                { tag: "banner off with the pop-ups on", popups: true, banner: false, unavailable: true,
                  chain: ["stubAbove", "errorPopupsSwitch", "unavailableBannersSwitch", "otherBannerSlider",
                          "stubBelow"] },
                { tag: "unavailable banners off", popups: true, banner: true, unavailable: false,
                  chain: ["stubAbove", "errorPopupsSwitch", "unavailableBannersSwitch", "otherBannerSlider",
                          "stubBelow"] },
                { tag: "error pop-ups off, unavailable banners off", popups: false, banner: true, unavailable: false,
                  chain: ["stubAbove", "errorPopupsSwitch", "errorBannerSwitch", "errorBannerSlider",
                          "unavailableBannersSwitch", "otherBannerSlider", "stubBelow"] },
                { tag: "everything off", popups: false, banner: false, unavailable: false,
                  chain: ["stubAbove", "errorPopupsSwitch", "errorBannerSwitch", "unavailableBannersSwitch",
                          "otherBannerSlider", "stubBelow"] }
            ];
        }

        // DOWN from the control above through every shown control to the one below, then UP back
        function test_keypadChain(data) {
            apply(data);
            var chain = [];
            for (var k = 0; k < data.chain.length; k++) chain.push(control(data.chain[k]));

            focusOn(chain[0], data.chain[0] + " focused");
            for (var i = 1; i < chain.length; i++) {
                keyClick(Qt.Key_Down);
                expectFocus(chain[i], "DOWN from " + data.chain[i - 1] + " reaches " + data.chain[i]);
            }
            for (var j = chain.length - 2; j >= 0; j--) {
                keyClick(Qt.Key_Up);
                expectFocus(chain[j], "UP from " + data.chain[j + 1] + " reaches " + data.chain[j]);
            }
        }

        // the ends do not move with the switches: the other banner duration is always the last control
        function test_ends_data() {
            return [
                { tag: "defaults", popups: true, banner: true, unavailable: true },
                { tag: "error pop-ups off", popups: false, banner: true, unavailable: true },
                { tag: "error pop-ups off, banner off", popups: false, banner: false, unavailable: true },
                { tag: "unavailable banners off", popups: true, banner: true, unavailable: false },
                { tag: "everything off", popups: false, banner: false, unavailable: false }
            ];
        }

        function test_ends(data) {
            apply(data);
            compare(section.firstFocusItem, control("errorPopupsSwitch"), "firstFocusItem");
            compare(section.lastFocusItem, control("otherBannerSlider"), "lastFocusItem");
        }

        // Ui.qml leaves navDownTarget unset: the section is the last item of the page
        function test_nothingBelow_keepsTheFocus_data() {
            return test_ends_data();
        }

        function test_nothingBelow_keepsTheFocus(data) {
            apply(data);
            section.navDownTarget = null;
            var last = control("otherBannerSlider");
            focusOn(last, "otherBannerSlider focused");
            keyClick(Qt.Key_Down);
            verify(last.activeFocus, "DOWN from otherBannerSlider keeps the focus");
        }

        // the chain follows a switch toggled while it has the focus; "Show offline banners" moves nothing
        function test_keypad_followsTheSwitchJustToggled() {
            focusOn(control("errorPopupsSwitch"), "Show error pop-ups focused");
            keyClick(Qt.Key_Return);
            verify(!Config.showErrorPopups, "error pop-ups off");
            keyClick(Qt.Key_Down);
            expectFocus(control("errorBannerSwitch"), "DOWN reaches the banner switch that just appeared");
            keyClick(Qt.Key_Return);
            verify(!Config.showErrorBanner, "banner off");
            keyClick(Qt.Key_Down);
            expectFocus(control("unavailableBannersSwitch"), "DOWN skips the duration that just went");
            keyClick(Qt.Key_Up);
            expectFocus(control("errorBannerSwitch"), "UP skips it too");
            keyClick(Qt.Key_Return);
            verify(Config.showErrorBanner, "banner back on");
            keyClick(Qt.Key_Down);
            expectFocus(control("errorBannerSlider"), "DOWN reaches the duration that just came back");
            keyClick(Qt.Key_Down);
            expectFocus(control("unavailableBannersSwitch"), "DOWN reaches Show offline banners");
            keyClick(Qt.Key_Return);
            verify(!Config.showUnavailableBanners, "offline banners off");
            keyClick(Qt.Key_Down);
            expectFocus(control("otherBannerSlider"), "DOWN still reaches the other banner duration");
            keyClick(Qt.Key_Down);
            expectFocus(stubBelow, "DOWN leaves the section");
            keyClick(Qt.Key_Up);
            expectFocus(control("otherBannerSlider"), "UP comes back to the last control");
            keyClick(Qt.Key_Up);
            expectFocus(control("unavailableBannersSwitch"), "UP reaches Show offline banners");
            keyClick(Qt.Key_Up);
            expectFocus(control("errorBannerSlider"), "UP reaches the error banner duration");
        }

        // ── switches ────────────────────────────────────────────────

        function test_okTogglesTheSwitch_data() {
            return [
                { tag: "Show error pop-ups", objName: "errorPopupsSwitch", setting: "showErrorPopups", popups: true },
                { tag: "Show a banner", objName: "errorBannerSwitch", setting: "showErrorBanner", popups: false },
                { tag: "Show offline banners", objName: "unavailableBannersSwitch", setting: "showUnavailableBanners",
                  popups: true },
                { tag: "Show offline banners, error pop-ups off", objName: "unavailableBannersSwitch",
                  setting: "showUnavailableBanners", popups: false }
            ];
        }

        function test_okTogglesTheSwitch(data) {
            Config.showErrorPopups = data.popups;
            var sw = control(data.objName);
            focusOn(sw, data.objName + " focused");
            verify(Config[data.setting], data.setting + " starts on");
            verify(sw.checked, "the switch shows it on");
            var before = settingsSnapshot();

            keyClick(Qt.Key_Return);
            verify(!Config[data.setting], "OK (Return) turns " + data.setting + " off");
            verify(!sw.checked, "the switch follows");
            verify(sw.activeFocus, "the focus stays on the switch");
            for (var i = 0; i < settingNames.length; i++) {
                if (settingNames[i] !== data.setting) {
                    compare(Config[settingNames[i]], before[settingNames[i]], settingNames[i] + " unchanged");
                }
            }

            keyClick(Qt.Key_Enter);
            verify(Config[data.setting], "Enter turns it back on");
            verify(sw.checked, "the switch follows");
        }

        function test_switchesFollowTheSettings() {
            Config.showErrorPopups = false;
            Config.showErrorBanner = false;
            Config.showUnavailableBanners = false;
            verify(!control("errorPopupsSwitch").checked, "Show error pop-ups");
            verify(!control("errorBannerSwitch").checked, "Show a banner");
            verify(!control("unavailableBannersSwitch").checked, "Show offline banners");
            Config.showErrorBanner = true;
            Config.showUnavailableBanners = true;
            verify(control("errorBannerSwitch").checked, "Show a banner, back on");
            verify(control("unavailableBannersSwitch").checked, "Show offline banners, back on");
        }

        // ── duration sliders ────────────────────────────────────────

        // popups: a state that shows the slider (the other banner duration always shows) and keeps it inside the
        // 850 px window for a tap
        function test_slider_data() {
            return [
                { tag: "Error banner duration", objName: "errorBannerSlider", setting: "errorBannerSec", def: 5,
                  popups: false },
                { tag: "Other banner duration", objName: "otherBannerSlider", setting: "otherBannerSec", def: 4,
                  popups: true }
            ];
        }

        function test_slider(data) {
            Config.showErrorPopups = data.popups;
            var s = control(data.objName);
            verify(s.visible, data.objName + " shown");
            compare(s.from, 1, "from 1 s");
            compare(s.to, 30, "to 30 s");
            compare(s.stepSize, 1, "in 1 s steps");
            compare(s.value, data.def, "starts at the setting");
            compare(rowOf(s).value, data.def + " s", "the row shows the value");

            Config[data.setting] = 17;
            compare(s.value, 17, "follows the setting");
            compare(rowOf(s).value, "17 s");
        }

        // the keypad path: RIGHT / LEFT move the slider and write the setting
        function test_slider_keys_data() {
            return test_slider_data();
        }

        function test_slider_keys(data) {
            Config.showErrorPopups = data.popups;
            var s = control(data.objName);
            focusOn(s, data.objName + " focused");

            keyClick(Qt.Key_Right);
            compare(Config[data.setting], data.def + 1, "RIGHT writes the setting");
            compare(s.value, data.def + 1);
            compare(rowOf(s).value, (data.def + 1) + " s");
            keyClick(Qt.Key_Left);
            keyClick(Qt.Key_Left);
            compare(Config[data.setting], data.def - 1, "LEFT writes it");

            // the binding survives the keypad: a change of the setting still moves the slider
            Config[data.setting] = 20;
            compare(s.value, 20, "still follows the setting");

            // the ends
            Config[data.setting] = 30;
            keyClick(Qt.Key_Right);
            compare(Config[data.setting], 30, "RIGHT at 30 s stays at 30 s");
            compare(s.value, 30);
            Config[data.setting] = 1;
            keyClick(Qt.Key_Left);
            compare(Config[data.setting], 1, "LEFT at 1 s stays at 1 s");
            compare(s.value, 1);
        }

        // the touch path: a tap on the track moves the slider and writes the setting
        function test_slider_tap_data() {
            return test_slider_data();
        }

        function test_slider_tap(data) {
            Config.showErrorPopups = data.popups;
            var s = control(data.objName);
            waitForLayout();
            var right = s.mapToItem(testRoot, s.width - 2, s.height / 2);
            verify(right.y > 0 && right.y < testRoot.height, data.objName + " inside the window for the tap (y " +
                   right.y + ")");

            mouseClick(s, s.width - 2, s.height / 2);
            tryCompare(Config, data.setting, 30, 2000, "a tap at the right end writes 30 s");
            compare(s.value, 30);
            mouseClick(s, 2, s.height / 2);
            tryCompare(Config, data.setting, 1, 2000, "a tap at the left end writes 1 s");
            compare(s.value, 1);
        }

        // ── reveal ──────────────────────────────────────────────────

        function test_reveal_data() {
            return [
                { tag: "Show error pop-ups, with the heading", objName: "errorPopupsSwitch", row: 0 },
                { tag: "Show a banner", objName: "errorBannerSwitch", row: 1 },
                { tag: "Error banner duration", objName: "errorBannerSlider", row: 2 },
                { tag: "Show offline banners", objName: "unavailableBannersSwitch", row: 3 },
                { tag: "Other banner duration", objName: "otherBannerSlider", row: 4 }
            ];
        }

        // while the keypad is in use, the focused control's whole row is revealed: a direct child of the section
        function test_reveal(data) {
            Config.showErrorPopups = false;   // every row shown
            compare(section.children.length, 5, "the heading with the first switch, and four rows");
            stubSettingsPage.revealed = [];

            focusOn(control(data.objName), data.objName + " focused");
            tryVerify(function() { return stubSettingsPage.revealed.length > 0; }, 2000, "ensureVisible called");
            compare(stubSettingsPage.revealed.length, 1, "once");
            compare(lastRevealed(), section.children[data.row], "the row around " + data.objName);
            compare(lastRevealed().parent, section, "a direct child of the section");
            if (data.row === 0) {
                verify(findText(lastRevealed(), "Messages"), "the heading goes with the first switch");
            }
        }

        function test_reveal_onlyWhileTheKeypadIsInUse() {
            Config.showErrorPopups = false;
            ui.keyNavigationActive = false;
            stubSettingsPage.revealed = [];
            for (var i = 0; i < controlNames.length; i++) {
                focusOn(control(controlNames[i]), controlNames[i] + " focused");
            }
            wait(50);   // the reveal is deferred: give a pending one the chance to run
            compare(stubSettingsPage.revealed.length, 0, "touch: nothing revealed");

            ui.keyNavigationActive = true;
            focusOn(control("errorPopupsSwitch"), "Show error pop-ups focused with the keypad");
            tryVerify(function() { return stubSettingsPage.revealed.length > 0; }, 2000, "the keypad: revealed again");
            wait(50);
            compare(stubSettingsPage.revealed.length, 1, "the keypad: revealed once");
        }

        function test_reveal_notForControlsOutsideTheSection() {
            stubSettingsPage.revealed = [];
            focusOn(stubAbove, "the control above focused");
            wait(50);   // the reveal is deferred and merges focus changes of one event: let each one run on its own
            focusOn(stubBelow, "the control below focused");
            wait(50);
            compare(stubSettingsPage.revealed.length, 0, "nothing revealed for controls outside the section");
        }
    }
}
