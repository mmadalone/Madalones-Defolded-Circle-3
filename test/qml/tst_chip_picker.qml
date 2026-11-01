// Copyright (c) 2026 madalone. v1.6.0 — QML tests for the shared ChipPicker (replaces the tests of the removed
// ChargingScreen.cycleOption, whose job the picker took over).
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import "qrc:/components" as Components

Item {
    id: testRoot
    width: 480
    height: 400

    Components.ChipPicker {
        id: picker
        width: 300
        model: [
            { value: "matrix", label: "Matrix" },
            { value: "starfield", label: "Starfield" },
            { value: "minimal", label: "Minimal" },
            { value: "analog", label: "Analog" },
            { value: "tvstatic", label: "TV Static" }
        ]
    }

    SignalSpy {
        id: pickedSpy
        target: picker
        signalName: "picked"
    }

    TestCase {
        name: "ChipPicker"
        when: windowShown

        function init() {
            picker.multiple = false;
            picker.wrap = false;
            picker.currentValues = [];
            picker.cursor = 0;
            picker.currentValue = "matrix";
            picker.forceActiveFocus();
            pickedSpy.clear();
        }

        function test_height_isRowsOfChips() {
            // 5 chips in 3 columns: 2 rows of 60 with 10 between them
            compare(picker.height, 130);
        }

        function test_right_picksTheNextValue() {
            keyClick(Qt.Key_Right);
            compare(pickedSpy.count, 1);
            compare(pickedSpy.signalArguments[0][0], "starfield");
        }

        function test_stopsAtTheEnds() {
            keyClick(Qt.Key_Left);
            compare(pickedSpy.count, 0, "LEFT on the first chip does nothing");
            picker.currentValue = "tvstatic";
            keyClick(Qt.Key_Right);
            compare(pickedSpy.count, 0, "RIGHT on the last chip does nothing");
        }

        function test_wrap_carriesOnAtTheEnds() {
            picker.wrap = true;
            keyClick(Qt.Key_Left);
            compare(pickedSpy.signalArguments[0][0], "tvstatic");
            picker.currentValue = "tvstatic";
            keyClick(Qt.Key_Right);
            compare(pickedSpy.signalArguments[1][0], "matrix");
        }

        function test_valueOfACompanionPicker_entersFromTheKeysSide() {
            // the value belongs to another picker of the same setting (solid colours vs gradients)
            picker.currentValue = "rainbow";
            compare(picker.currentIndex, -1);
            keyClick(Qt.Key_Right);
            compare(pickedSpy.signalArguments[0][0], "matrix", "RIGHT comes in at the first chip");
            keyClick(Qt.Key_Left);
            compare(pickedSpy.signalArguments[1][0], "tvstatic", "LEFT comes in at the last chip");
        }

        function test_ok_doesNothingInASinglePicker() {
            keyClick(Qt.Key_Return);
            compare(pickedSpy.count, 0);
        }

        function test_multiple_okTogglesTheChipInTheRing() {
            picker.multiple = true;
            picker.currentValues = ["matrix"];
            keyClick(Qt.Key_Right);
            keyClick(Qt.Key_Right);
            compare(picker.cursor, 2);
            compare(pickedSpy.count, 0, "moving the ring picks nothing");
            keyClick(Qt.Key_Return);
            compare(pickedSpy.count, 1);
            compare(pickedSpy.signalArguments[0][0], "minimal");
        }

        function test_multiple_ringStaysInsideTheGrid() {
            picker.multiple = true;
            keyClick(Qt.Key_Left);
            compare(picker.cursor, 0);
            for (var i = 0; i < 8; i++) {
                keyClick(Qt.Key_Right);
            }
            compare(picker.cursor, 4);
        }

        function test_tap_picksThatChip() {
            // second row, first column: "Analog"
            mouseClick(picker, picker.cellWidth / 2, picker.cellHeight + picker.cellSpacing + picker.cellHeight / 2);
            compare(pickedSpy.count, 1);
            compare(pickedSpy.signalArguments[0][0], "analog");
        }

        function test_checkMark_readsOnLightAndDarkSwatches() {
            compare(picker.markColor(Qt.rgba(1, 1, 1, 1)), colors.bg, "black on white");
            compare(picker.markColor(Qt.rgba(0, 0, 1, 1)), colors.textPrimary, "light grey on blue");
        }
    }
}
