// Copyright (c) 2026 madalone. v1.6.3 — QML test for the order of the two reveals on Settings → User interface →
// Messages (MessagesSection.qml). On a keypad focus change UC's ButtonNavigation reveals the focused control and the
// section the control's whole row (title, help text, control); the page's contentY Behavior animates to whichever
// target was set last, so the section defers its reveal to run after UC's. tst_messages_section.qml records the reveal
// with a stub page and passes either way; this test scrolls a real page and checks what is on the screen.
// The page is a cut-down Ui.qml (Settings.Page with UC's ButtonNavigation, the Flickable with the 300 ms Behavior, the
// section last), created as in the app: through an asynchronous Loader while another page holds the input. The
// section's focus handler then runs first, as in the app (load() checks it); a page that runs UC's first lets a
// synchronous reveal pass.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtTest 1.2

import Config 1.0

import "qrc:/components" as Components
import "qrc:/settings" as Settings
import "qrc:/settings/settings/userinterface" as UserInterface

Item {
    id: testRoot
    width: 480
    height: 850

    Component {
        id: pageComponent

        Settings.Page {
            id: uiPage
            scrollTarget: flickable
            initialFocusItem: aboveSwitch

            property alias flickable: flickable
            property alias scrollAnimation: scrollAnimation
            property alias content: content
            property alias aboveSwitch: aboveSwitch
            property alias section: section

            // as Ui.qml's
            Flickable {
                id: flickable
                width: parent.width
                height: parent.height - topNavigation.height
                anchors.top: topNavigation.bottom
                contentWidth: content.width
                contentHeight: content.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                Behavior on contentY {
                    NumberAnimation {
                        id: scrollAnimation
                        duration: 300
                    }
                }

                ColumnLayout {
                    id: content
                    width: flickable.width
                    spacing: 0

                    // a screen of other settings
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: flickable.height
                    }

                    // the control above the section: Ui.qml's "Show source picker button"
                    Components.SettingRow {
                        title: "Above the section"

                        Components.Switch {
                            id: aboveSwitch
                            objectName: "aboveSwitch"
                            highlight: activeFocus && ui.keyNavigationActive
                            KeyNavigation.down: section.firstFocusItem
                        }
                    }

                    UserInterface.MessagesSection {
                        id: section
                        Layout.fillWidth: true
                        settingsPage: uiPage
                        navUpTarget: aboveSwitch
                    }
                }
            }
        }
    }

    // like the app's level-2 settings pages (Profile.qml's thirdPage)
    Loader {
        id: loader
        anchors.fill: parent
        asynchronous: true
        active: false
        sourceComponent: pageComponent
    }

    // The level-1 settings page that opens the page (Settings.qml) holds the input while the page is created, as in
    // the app, where MainContainer.qml also sets a base owner. With no owner (the mock's default), ButtonNavigation's
    // hasInputControl (activeItem === scope) is briefly true while its scope is still null, and the change back makes
    // it read Window.activeFocusItem during creation: its focus handler then runs before the section's.
    Item {
        id: settingsMenu
    }

    TestCase {
        name: "MessagesRevealOrder"
        when: windowShown

        property Item page: null

        // top to bottom, the controls each state shows
        readonly property var everyControl: ["errorPopupsSwitch", "errorBannerSwitch", "errorBannerSlider",
                                             "unavailableBannersSwitch", "otherBannerSlider"]
        readonly property var defaultControls: ["errorPopupsSwitch", "unavailableBannersSwitch", "otherBannerSlider"]

        function init() {
            Config.resetDefaults();
            ui.keyNavigationActive = true;
            ui.inputController.takeControl(settingsMenu);
        }

        function cleanup() {
            if (loader.item) {
                // hand the input back, and let a reveal still pending run while the page exists
                loader.item.buttonNavigation.releaseControl();
                wait(50);
            }
            loader.active = false;
            page = null;
            ui.inputController.releaseControl(settingsMenu);
            ui.keyNavigationActive = true;
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

        function control(objName) {
            var c = findByObjectName(page.section, objName);
            verify(c, objName + " found");
            return c;
        }

        // the direct child of the section around a control: the row the section reveals
        function rowOf(item) {
            for (var p = item; p && p.parent; p = p.parent) {
                if (p.parent === page.section) return p;
            }
            return null;
        }

        // the ColumnLayouts place their rows on the next polish: true once the shown rows are stacked
        function stacked(layout) {
            var y = 0;
            for (var i = 0; i < layout.children.length; i++) {
                var row = layout.children[i];
                if (!row.visible) continue;
                if (Math.abs(row.y - y) > 0.5) return false;
                y += row.height;
            }
            return true;
        }

        function load() {
            loader.active = true;
            tryCompare(loader, "status", Loader.Ready, 10000, "the page loaded");
            page = loader.item;
            verify(page.scrollAnimation, "the page's scroll animation");
            tryVerify(function() {
                return stacked(page.content) && stacked(page.section)
                       && page.flickable.contentHeight > page.flickable.height;
            }, 5000, "the page laid out");
            // the page takes the input, as Profile.qml's Loader has it do once loaded (here once laid out, so the first
            // reveal measures the final rows): its button navigation focuses the switch above the section. On that
            // focus change, note which focus handler runs first.
            var order = [];
            var sectionSaw = function() { order.push("MessagesSection"); };
            var pageSaw = function() { order.push("ButtonNavigation"); };
            page.section.keypadFocusItemChanged.connect(sectionSaw);
            page.buttonNavigation.windowFocusItemChanged.connect(pageSaw);
            page.buttonNavigation.takeControl();
            tryVerify(function() { return page.aboveSwitch.activeFocus; }, 5000,
                      "the switch above the section focused");
            page.section.keypadFocusItemChanged.disconnect(sectionSaw);
            page.buttonNavigation.windowFocusItemChanged.disconnect(pageSaw);
            compare(order.slice(0, 2).join(", "), "MessagesSection, ButtonNavigation",
                    "the section's focus handler runs before UC's, as in the app; otherwise the order is not tested");
            settle();
        }

        // the section's reveal runs on the next pass of the event loop, then the page scrolls for 300 ms to the target
        // set last
        function settle() {
            wait(100);
            tryVerify(function() { return !page.scrollAnimation.running; }, 5000, "the page's scroll animation ended");
            var y = page.flickable.contentY;
            wait(150);
            compare(page.flickable.contentY, y, "the page stays where the reveals put it");
        }

        function topOf(row) {
            return row.mapToItem(page.flickable.contentItem, 0, 0).y;
        }

        function rowShown(row) {
            var top = topOf(row);
            return top >= page.flickable.contentY - 0.5
                   && top + row.height <= page.flickable.contentY + page.flickable.height + 0.5;
        }

        // the focused control's whole row is on the screen: inside [contentY, contentY + height]
        function expectRowShown(item, message) {
            var row = rowOf(item);
            verify(row, message + ": the row around the control");
            var top = topOf(row);
            var first = page.flickable.contentY;
            verify(rowShown(row), message + ": its whole row on the screen (the row spans " + Math.round(top) + " to "
                   + Math.round(top + row.height) + ", the screen shows " + Math.round(first) + " to "
                   + Math.round(first + page.flickable.height) + ")");
        }

        // press the key once per control of the chain; after each, the focus is on it and its whole row is shown.
        // Returns the controls whose row was not on the screen before the key press.
        function walk(chain, key, from) {
            var offScreen = [];
            for (var i = 0; i < chain.length; i++) {
                var next = control(chain[i]);
                var step = (key === Qt.Key_Down ? "DOWN from " : "UP from ") + from + " to " + chain[i];
                if (!rowShown(rowOf(next))) offScreen.push(chain[i]);
                keyClick(key);
                tryVerify(function() { return next.activeFocus; }, 2000, step + ": focused");
                settle();
                expectRowShown(next, step);
                from = chain[i];
            }
            return offScreen;
        }

        function test_keypad_down_data() {
            return [
                { tag: "every row shown (error pop-ups off)", popups: false, chain: everyControl },
                { tag: "defaults", popups: true, chain: defaultControls }
            ];
        }

        // DOWN from the switch above the section, whose rows start below the screen, to the last control
        function test_keypad_down(data) {
            Config.showErrorPopups = data.popups;
            load();
            var offScreen = walk(data.chain, Qt.Key_Down, "aboveSwitch");
            verify(offScreen.indexOf(data.chain[0]) >= 0, "the first row started below the screen");
        }

        function test_keypad_up_data() {
            return test_keypad_down_data();
        }

        // UP from the last control, at the end of the page, to the first: the first row, its heading above the switch,
        // comes in from above the screen
        function test_keypad_up(data) {
            Config.showErrorPopups = data.popups;
            load();
            var chain = data.chain.slice().reverse();
            var last = control(chain[0]);
            last.forceActiveFocus();
            tryVerify(function() { return last.activeFocus; }, 2000, chain[0] + " focused");
            settle();
            fuzzyCompare(page.flickable.contentY, page.flickable.contentHeight - page.flickable.height, 0.5,
                         "the page scrolled to its end");
            expectRowShown(last, chain[0] + " focused");
            var offScreen = walk(chain.slice(1), Qt.Key_Up, chain[0]);
            verify(offScreen.indexOf(chain[chain.length - 1]) >= 0, "the first row started above the screen");
        }
    }
}
