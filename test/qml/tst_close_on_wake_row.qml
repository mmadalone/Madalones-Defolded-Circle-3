// Copyright (c) 2026 madalone. The "Close on wake" row of the screensaver's General behavior (v1.6.4, GitHub issue #3):
// three chips Never / Undocked / Always over ScreensaverConfig.closeOnWake, a help line for each choice, and the keypad
// path through the row on Matrix and on the other themes.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtTest 1.2

import ScreensaverConfig 1.0

import "qrc:/settings" as Settings
import "qrc:/settings/settings/chargingscreen" as ChargingScreen

Item {
    id: testRoot
    width: 960
    height: 2400

    function findByObjectName(item, name) {
        if (item.objectName === name) return item;
        if (!item.children) return null;
        for (var i = 0; i < item.children.length; i++) {
            var found = findByObjectName(item.children[i], name);
            if (found) return found;
        }
        return null;
    }

    Item { id: stubSettingsPage }

    ChargingScreen.GeneralBehavior {
        id: generalBehavior
        width: 480
        settingsPage: stubSettingsPage
    }

    // A settings page as the Screensaver page builds it (settings/settings/ChargingScreen.qml: Settings.Page, the
    // Flickable under the title bar, the sections in a ColumnLayout), with a screen of other rows above the section,
    // for the reveal after a pick. Placed to the right of the stub section above.
    Settings.Page {
        id: revealPage
        x: 480
        width: 480
        height: 850
        scrollTarget: revealFlickable

        property alias flickable: revealFlickable
        property alias section: revealSection

        Flickable {
            id: revealFlickable
            width: parent.width
            height: parent.height - revealPage.topNavigation.height
            anchors.top: revealPage.topNavigation.bottom
            contentWidth: revealContent.width
            contentHeight: revealContent.height
            clip: true

            ColumnLayout {
                id: revealContent
                width: revealFlickable.width
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: revealFlickable.height
                }

                ChargingScreen.GeneralBehavior {
                    id: revealSection
                    Layout.fillWidth: true
                    settingsPage: revealPage
                }
            }
        }
    }

    TestCase {
        name: "CloseOnWakeRow"
        when: windowShown

        property var picker: null
        property var row: null

        function init() {
            ScreensaverConfig.resetDefaults();
            picker = findByObjectName(generalBehavior, "closeOnWakePicker");
            row = findByObjectName(generalBehavior, "closeOnWakeRow");
            verify(picker !== null && row !== null, "the Close on wake row and its picker");
        }

        // the centre of chip i (0-2) in the picker's own coordinates
        function chipCentreX(i) {
            return i * (picker.cellWidth + picker.cellSpacing) + picker.cellWidth / 2;
        }

        function test_chips() {
            compare(row.title, "Close on wake");
            compare(picker.model.length, 3);
            compare(picker.model[0].value, 0);
            compare(picker.model[0].label, "Never");
            compare(picker.model[1].value, 1);
            compare(picker.model[1].label, "Undocked");
            compare(picker.model[2].value, 2);
            compare(picker.model[2].label, "Always");
            compare(picker.columns, 3, "the three chips in one line");
        }

        function test_showsTheStoredChoice() {
            compare(picker.currentIndex, 0, "Never by default");
            ScreensaverConfig.closeOnWake = 1;
            compare(picker.currentIndex, 1);
            ScreensaverConfig.closeOnWake = 2;
            compare(picker.currentIndex, 2);
            ScreensaverConfig.closeOnWake = 0;
            compare(picker.currentIndex, 0);
        }

        // A pick changes the help line's height: the rows below move only when the section's layout runs before the
        // next frame. Wait for that frame before the next tap, as a finger would, or the tap lands on the row below.
        function tapChip(i) {
            mouseClick(picker, chipCentreX(i), picker.cellHeight / 2);
            waitForRendering(generalBehavior);
        }

        function test_tapStoresTheChoice() {
            tapChip(1);
            compare(ScreensaverConfig.closeOnWake, 1, "tap on Undocked");
            tapChip(2);
            compare(ScreensaverConfig.closeOnWake, 2, "tap on Always");
            tapChip(0);
            compare(ScreensaverConfig.closeOnWake, 0, "tap on Never");
            compare(ScreensaverConfig.dpadEnabled, true, "no tap reached the DPAD interactive row below");
        }

        function test_leftRightStepTheChoice() {
            picker.forceActiveFocus();
            tryVerify(function() { return picker.activeFocus; }, 2000, "Close on wake focused");
            keyClick(Qt.Key_Right);
            compare(ScreensaverConfig.closeOnWake, 1, "RIGHT from Never picks Undocked");
            keyClick(Qt.Key_Right);
            compare(ScreensaverConfig.closeOnWake, 2, "RIGHT from Undocked picks Always");
            keyClick(Qt.Key_Right);
            compare(ScreensaverConfig.closeOnWake, 2, "RIGHT stops at Always");
            keyClick(Qt.Key_Left);
            compare(ScreensaverConfig.closeOnWake, 1, "LEFT from Always picks Undocked");
            keyClick(Qt.Key_Left);
            keyClick(Qt.Key_Left);
            compare(ScreensaverConfig.closeOnWake, 0, "LEFT stops at Never");
        }

        function test_helpFollowsTheChoice() {
            compare(row.help, "Stays until you close it.");
            ScreensaverConfig.closeOnWake = 1;
            compare(row.help,
                    "Stays on the dock. Closes a few seconds after you lift the remote, or when it wakes off the dock.");
            ScreensaverConfig.closeOnWake = 2;
            compare(row.help, "Closes whenever the remote wakes or leaves the dock.");
        }

        // DOWN from "Double-tap to close" to the picker and on to the next row, then UP back
        function keypad(nextName) {
            var first = findByObjectName(generalBehavior, "tapToCloseSwitch");
            var next = findByObjectName(generalBehavior, nextName);
            verify(first !== null && next !== null, "Double-tap to close and " + nextName);
            tryVerify(function() { return next.visible; }, 2000, nextName + " shown");
            first.forceActiveFocus();
            tryVerify(function() { return first.activeFocus; }, 2000, "Double-tap to close focused");
            keyClick(Qt.Key_Down);
            tryVerify(function() { return picker.activeFocus; }, 2000, "DOWN from Double-tap to close reaches Close on wake");
            keyClick(Qt.Key_Down);
            tryVerify(function() { return next.activeFocus; }, 2000, "DOWN from Close on wake reaches " + nextName);
            keyClick(Qt.Key_Up);
            tryVerify(function() { return picker.activeFocus; }, 2000, "UP back to Close on wake");
            keyClick(Qt.Key_Up);
            tryVerify(function() { return first.activeFocus; }, 2000, "UP back to Double-tap to close");
        }

        function test_keypad_matrix() {
            ScreensaverConfig.theme = "matrix";
            keypad("dpadEnabledSwitch");
        }

        function test_keypad_otherThemes() {
            ScreensaverConfig.theme = "minimal";
            keypad("idleEnabledSwitch");
        }

        // DOWN and UP inside the picker never change the choice; only LEFT and RIGHT do
        function test_upDownLeaveTheChoice() {
            ScreensaverConfig.closeOnWake = 1;
            picker.forceActiveFocus();
            tryVerify(function() { return picker.activeFocus; }, 2000, "Close on wake focused");
            keyClick(Qt.Key_Down);
            keyClick(Qt.Key_Up);
            compare(ScreensaverConfig.closeOnWake, 1);
        }
    }

    // Undocked's help line is two lines longer than Never's and pushes the chips down. With the keypad, the page
    // brings them back into view after the pick; with touch it doesn't scroll (a finger stays where it tapped).
    TestCase {
        name: "CloseOnWakeReveal"
        when: windowShown

        property var picker: null
        property var row: null

        function init() {
            ScreensaverConfig.resetDefaults();
            ScreensaverConfig.theme = "minimal";
            ui.keyNavigationActive = true;
            picker = findByObjectName(revealPage.section, "closeOnWakePicker");
            row = findByObjectName(revealPage.section, "closeOnWakeRow");
            verify(picker !== null && row !== null, "the Close on wake row and its picker on the page");
        }

        function cleanup() {
            ui.keyNavigationActive = true;
        }

        // the picker's bottom edge in the Flickable's content
        function pickerBottom() {
            return picker.mapToItem(revealPage.flickable.contentItem, 0, picker.height).y;
        }

        // scrolls the page so that the picker's bottom edge sits `gap` px above the bottom of the viewport
        function placePickerAtTheBottom(gap) {
            var f = revealPage.flickable;
            f.contentY = Math.max(0, pickerBottom() - f.height + gap);
            tryVerify(function() { return pickerBottom() <= f.contentY + f.height; }, 2000, "picker on the screen");
        }

        function test_keypadPick_bringsTheChipsBackIntoView() {
            var f = revealPage.flickable;
            picker.forceActiveFocus();
            tryVerify(function() { return picker.activeFocus; }, 2000, "Close on wake focused");
            placePickerAtTheBottom(10);
            var before = f.contentY;
            keyClick(Qt.Key_Right);
            compare(ScreensaverConfig.closeOnWake, 1, "Undocked picked");
            // the longer help moved the chips below the viewport; the deferred reveal scrolls them back
            tryVerify(function() { return pickerBottom() <= f.contentY + f.height; }, 3000,
                      "the chips are on the screen again after the pick");
            verify(f.contentY > before, "the page scrolled down to show them");
        }

        function test_touchPick_leavesThePage() {
            var f = revealPage.flickable;
            placePickerAtTheBottom(10);
            var before = f.contentY;
            ui.keyNavigationActive = false;
            // the Undocked chip: tap it where it is on the screen
            mouseClick(picker, picker.cellWidth * 1.5 + picker.cellSpacing, picker.cellHeight / 2);
            compare(ScreensaverConfig.closeOnWake, 1, "Undocked picked by touch");
            wait(300);
            compare(f.contentY, before, "a touch pick does not scroll the page");
        }
    }
}
