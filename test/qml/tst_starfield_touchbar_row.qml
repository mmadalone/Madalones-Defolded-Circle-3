// Copyright (c) 2026 madalone. The "Touchbar" row of the Starfield screensaver settings (v1.6.4): three chips Speed /
// Density / Off over ScreensaverConfig.starfieldTouchbar right below "Star density", a help line for each choice, the
// keypad path Star density -> Touchbar -> Star size and back, and the page's reveal after a keypad pick.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

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

    ChargingScreen.StarfieldSettings {
        id: starfieldSettings
        width: 480
        settingsPage: stubSettingsPage
    }

    // stands in for the Screensaver settings page (settings/settings/ChargingScreen.qml, a Settings.Page): records what
    // the section asks it to bring into view
    Item {
        id: revealPage
        property int reveals: 0
        property Item revealed: null
        function ensureVisible(item) {
            reveals++;
            revealed = item;
        }
    }

    ChargingScreen.StarfieldSettings {
        id: revealSection
        x: 480
        width: 480
        settingsPage: revealPage
    }

    TestCase {
        name: "StarfieldTouchbarRow"
        when: windowShown

        property var picker: null
        property var row: null

        readonly property string speedHelp: "Sliding along the touch slider changes the Animation speed."
        readonly property string densityHelp: "Sliding along the touch slider changes the Star density."
        readonly property string offHelp: "The touch slider does nothing while the screensaver shows."

        function init() {
            ScreensaverConfig.resetDefaults();
            ScreensaverConfig.theme = "starfield";
            picker = findByObjectName(starfieldSettings, "starfieldTouchbarPicker");
            row = findByObjectName(starfieldSettings, "starfieldTouchbarRow");
            verify(picker !== null && row !== null, "the Touchbar row and its picker");
        }

        // the setting row (a child of the section) that holds `item`
        function rowOf(item) {
            for (var it = item; it; it = it.parent) {
                if (it.parent === starfieldSettings) return it;
            }
            return null;
        }

        // the centre of chip i (0-2) in the picker's own coordinates
        function chipCentreX(i) {
            return i * (picker.cellWidth + picker.cellSpacing) + picker.cellWidth / 2;
        }

        function test_chips() {
            compare(row.title, "Touchbar");
            compare(picker.model.length, 3);
            compare(picker.model[0].value, "speed");
            compare(picker.model[0].label, "Speed");
            compare(picker.model[1].value, "density");
            compare(picker.model[1].label, "Density");
            compare(picker.model[2].value, "off");
            compare(picker.model[2].label, "Off");
            compare(picker.columns, 3, "the three chips in one line");
        }

        // the row comes right after Star density and right before Star size, in the section and on the screen
        function test_rowBelowStarDensity() {
            var density = rowOf(findByObjectName(starfieldSettings, "starfieldDensitySlider"));
            var size = rowOf(findByObjectName(starfieldSettings, "starfieldStarSizeSlider"));
            verify(density !== null && size !== null, "the Star density and Star size rows");
            compare(density.title, "Star density");
            compare(size.title, "Star size");
            verify(rowOf(picker) === row, "the picker sits in the Touchbar row");
            var kids = starfieldSettings.children;
            var i = -1;
            for (var k = 0; k < kids.length; k++) {
                if (kids[k] === row) i = k;
            }
            verify(i > 0 && i < kids.length - 1, "the row is inside the section");
            verify(kids[i - 1] === density, "Star density right above, found: " + kids[i - 1].title);
            verify(kids[i + 1] === size, "Star size right below, found: " + kids[i + 1].title);
            tryVerify(function() { return row.height > 0 && size.y > row.y && row.y > density.y; }, 2000,
                      "laid out in that order");
            fuzzyCompare(row.y, density.y + density.height, 1, "directly under Star density");
            fuzzyCompare(size.y, row.y + row.height, 1, "Star size directly under it");
        }

        function test_showsTheStoredChoice() {
            compare(picker.currentIndex, 1, "Density by default");
            ScreensaverConfig.starfieldTouchbar = "speed";
            compare(picker.currentIndex, 0);
            ScreensaverConfig.starfieldTouchbar = "off";
            compare(picker.currentIndex, 2);
            ScreensaverConfig.starfieldTouchbar = "density";
            compare(picker.currentIndex, 1);
        }

        // ScreensaverConfig reads any value but "speed" or "off" as "density" (src/ui/screensaverTouchbar.h; the mock
        // uses the same rule), so the row shows Density, which is also what the screensaver applies
        function test_unknownValueShowsDensity() {
            ScreensaverConfig.starfieldTouchbar = "speed";
            ScreensaverConfig.starfieldTouchbar = "warp";
            compare(ScreensaverConfig.starfieldTouchbar, "density", "an unknown value reads as density");
            compare(picker.currentIndex, 1, "the Density chip");
            compare(row.help, densityHelp);
        }

        // a pick may change the help line's height: wait for the frame before the next tap, as a finger would
        function tapChip(i) {
            mouseClick(picker, chipCentreX(i), picker.cellHeight / 2);
            waitForRendering(starfieldSettings);
        }

        function test_tapStoresTheChoice() {
            tapChip(0);
            compare(ScreensaverConfig.starfieldTouchbar, "speed", "tap on Speed");
            tapChip(2);
            compare(ScreensaverConfig.starfieldTouchbar, "off", "tap on Off");
            tapChip(1);
            compare(ScreensaverConfig.starfieldTouchbar, "density", "tap on Density");
            compare(ScreensaverConfig.starfieldDensity, 50, "no tap reached the Star density slider above");
            compare(ScreensaverConfig.starfieldStarSize, 50, "no tap reached the Star size slider below");
        }

        function test_leftRightStepTheChoice() {
            picker.forceActiveFocus();
            tryVerify(function() { return picker.activeFocus; }, 2000, "Touchbar focused");
            keyClick(Qt.Key_Left);
            compare(ScreensaverConfig.starfieldTouchbar, "speed", "LEFT from Density picks Speed");
            keyClick(Qt.Key_Left);
            compare(ScreensaverConfig.starfieldTouchbar, "speed", "LEFT stops at Speed");
            keyClick(Qt.Key_Right);
            compare(ScreensaverConfig.starfieldTouchbar, "density", "RIGHT from Speed picks Density");
            keyClick(Qt.Key_Right);
            compare(ScreensaverConfig.starfieldTouchbar, "off", "RIGHT from Density picks Off");
            keyClick(Qt.Key_Right);
            compare(ScreensaverConfig.starfieldTouchbar, "off", "RIGHT stops at Off");
        }

        function test_helpFollowsTheChoice() {
            compare(row.help, densityHelp, "Density by default");
            ScreensaverConfig.starfieldTouchbar = "speed";
            compare(row.help, speedHelp);
            ScreensaverConfig.starfieldTouchbar = "off";
            compare(row.help, offHelp);
            ScreensaverConfig.starfieldTouchbar = "density";
            compare(row.help, densityHelp);
        }

        // DOWN from Star density to the picker and on to Star size, then UP back; UP and DOWN never change the choice
        function test_keypadChain() {
            var density = findByObjectName(starfieldSettings, "starfieldDensitySlider");
            var size = findByObjectName(starfieldSettings, "starfieldStarSizeSlider");
            verify(density !== null && size !== null, "the Star density and Star size sliders");
            density.forceActiveFocus();
            tryVerify(function() { return density.activeFocus; }, 2000, "Star density focused");
            keyClick(Qt.Key_Down);
            tryVerify(function() { return picker.activeFocus; }, 2000, "DOWN from Star density reaches Touchbar");
            keyClick(Qt.Key_Down);
            tryVerify(function() { return size.activeFocus; }, 2000, "DOWN from Touchbar reaches Star size");
            keyClick(Qt.Key_Up);
            tryVerify(function() { return picker.activeFocus; }, 2000, "UP from Star size back to Touchbar");
            keyClick(Qt.Key_Up);
            tryVerify(function() { return density.activeFocus; }, 2000, "UP from Touchbar back to Star density");
            compare(ScreensaverConfig.starfieldTouchbar, "density", "UP and DOWN leave the choice");
            compare(ScreensaverConfig.starfieldDensity, 50, "and the sliders");
            compare(ScreensaverConfig.starfieldStarSize, 50);
        }
    }

    // The help line sits above the chips and follows the choice: with the keypad, a pick asks the page to bring the row
    // back into view once the layout has followed; a touch pick leaves the page where it is (a finger stays where it
    // tapped).
    TestCase {
        name: "StarfieldTouchbarReveal"
        when: windowShown

        property var picker: null
        property var row: null

        function init() {
            ScreensaverConfig.resetDefaults();
            ScreensaverConfig.theme = "starfield";
            ui.keyNavigationActive = true;
            revealPage.reveals = 0;
            revealPage.revealed = null;
            picker = findByObjectName(revealSection, "starfieldTouchbarPicker");
            row = findByObjectName(revealSection, "starfieldTouchbarRow");
            verify(picker !== null && row !== null, "the Touchbar row and its picker");
        }

        function cleanup() {
            ui.keyNavigationActive = true;
        }

        function test_keypadPick_revealsTheRow() {
            picker.forceActiveFocus();
            tryVerify(function() { return picker.activeFocus; }, 2000, "Touchbar focused");
            keyClick(Qt.Key_Left);
            compare(ScreensaverConfig.starfieldTouchbar, "speed", "Speed picked");
            tryCompare(revealPage, "reveals", 1, 2000, "the page asked once to bring something into view");
            verify(revealPage.revealed === row, "the Touchbar row");
        }

        function test_touchPick_leavesThePage() {
            ui.keyNavigationActive = false;
            mouseClick(picker, picker.cellWidth / 2, picker.cellHeight / 2);
            compare(ScreensaverConfig.starfieldTouchbar, "speed", "Speed picked by touch");
            wait(100);
            compare(revealPage.reveals, 0, "a touch pick asks the page for nothing");
        }
    }
}
