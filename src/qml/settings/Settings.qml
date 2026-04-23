// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Added "Screensaver" menu entry; made the
// 10-item menu scrollable on touch and auto-scrolling on DPAD so the
// bottom entry ("Factory reset") isn't clipped off the viewport.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0
import Config 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: settingsPage

    function loadPage(page) {
        parentSwipeView.thirdPage.setSource("qrc:/settings/settings/" + menu.model[page].page + ".qml", { parentSwipeView: profileRoot, topNavigationText: Qt.binding(function(){ return qsTr(menu.model[page].itemTitle); }) });

        parentSwipeView.thirdPage.active = true;
        settingsSwipeView.incrementCurrentIndex();
    }

    Component.onCompleted: {
        Config.getConfig();

        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         menu.incrementCurrentIndex();
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         menu.decrementCurrentIndex();
                                                     }
                                                 },
                                                 "DPAD_MIDDLE": {
                                                     "pressed": function() {
                                                         loadPage(menu.currentIndex);
                                                     }
                                                 }
                                             });
    }

    // Direct ListView (no Flow wrapper). Bound to the viewport so the 10-item
    // menu scrolls instead of clipping the bottom ("Factory reset") item;
    // interactive: true gives touch-scroll; highlightRangeMode + preferredHighlight
    // band keeps the DPAD-selected item in the middle band so the view auto-scrolls
    // as DPAD navigation moves through items.
    ListView {
        id: menu
        anchors { top: topNavigation.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        clip: true

        interactive: true
        boundsBehavior: Flickable.StopAtBounds
        highlightMoveDuration: 200
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: height / 3
        preferredHighlightEnd: 2 * height / 3

        model: [
                {
                    itemTitle: QT_TR_NOOP("Display & Brightness"),
                    page: "Display",
                    icon: "uc:tv"
                },
                {
                    itemTitle: QT_TR_NOOP("User interface"),
                    page: "Ui",
                    icon: "uc:list"
                },
                {
                    itemTitle: QT_TR_NOOP("Screensaver"),
                    page: "ChargingScreen",
                    icon: "uc:bolt"
                },
                //                {
                //                    itemTitle: QT_TR_NOOP("Colors"),
                //                    page: "Color",
                //                    icon: "uc:color"
                //                },
                {
                    itemTitle: QT_TR_NOOP("Sound & Haptic"),
                    page: "Sound",
                    icon: "uc:volume"
                },
                {
                    itemTitle: QT_TR_NOOP("Voice Control"),
                    page: "Voice",
                    icon: "uc:microphone"
                },
                {
                    itemTitle: QT_TR_NOOP("Power Saving"),
                    page: "Power",
                    icon: "uc:battery-full"
                },
                {
                    itemTitle: QT_TR_NOOP("Wifi & Bluetooth"),
                    page: "Wifi",
                    icon: "uc:wifi"
                },
                {
                    itemTitle: QT_TR_NOOP("Localisation"),
                    page: "Localisation",
                    icon: "uc:language"
                },
                {
                    itemTitle: QT_TR_NOOP("Administrator PIN"),
                    page: "AdminPin",
                    icon: "uc:lock"
                },
                {
                    itemTitle: QT_TR_NOOP("Factory reset"),
                    page: "Reset",
                    icon: "uc:triangle-exclamation"
                }
            ]

        delegate: menuItem
    }

    Component {
        id: menuItem

        Rectangle {
            id: menuItemBg
            width: ui.width
            height: 80
            color: ListView.isCurrentItem && ui.keyNavigationEnabled ? colors.dark : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                color: Qt.lighter(menuItemBg.color, 1.3)
                width: 1
            }

            Components.Icon {
                id: icon
                color: colors.offwhite
                icon: menu.model[index].icon
                anchors { left: parent.left; verticalCenter: parent.verticalCenter; }
                size: 60
            }

            Text {
                id: menuItemText
                width: parent.width - 80
                elide: Text.ElideRight
                color: colors.offwhite
                text: qsTr(menu.model[index].itemTitle)
                anchors { left: icon.right; leftMargin: 20; verticalCenter: parent.verticalCenter; }
                font: fonts.primaryFont(30)
            }

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    menu.currentIndex = index;
                    loadPage(index);
                }
            }
        }
    }
}
