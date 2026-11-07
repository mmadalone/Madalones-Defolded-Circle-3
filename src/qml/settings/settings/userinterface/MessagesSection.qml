// Copyright (c) 2026 madalone. Messages settings: error pop-ups and banners. SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

import "qrc:/components" as Components
import Config 1.0

// Settings → User interface → Messages, at the end of Ui.qml.
// "Show error pop-ups" decides how failed commands, the touch slider's "not available" and integration errors are
// shown: upstream's pop-up, which takes the input until it is dismissed, or (off) a banner at the top that leaves the
// buttons working, or nothing. The banner of a tile or button pressed for an unavailable device gets a switch, and
// every banner but the error banner a duration (settings and setup banners always show). Notification (C++) applies
// the switches; Notification.qml the durations.
ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: errorPopupsSwitch
    readonly property Item lastFocusItem: otherBannerSlider
    property Item navUpTarget
    property Item navDownTarget

    spacing: 0

    // While the keypad is in use, show the whole row of the focused control: its title, help text and control.
    // UC's ensureVisible() reveals the direct child of the page layout around the control, which is this whole
    // section; with all its rows shown it is about as tall as the screen, so the control alone is revealed and the
    // title above a slider or the help text below a switch stays cut off. ChargingScreen.qml does the same for its
    // sections. Touch is left alone: scrolling to a title would move a slider under the finger.
    // UC's ButtonNavigation reveals the control on the same focus change (onWindowFocusItemChanged), after this
    // handler, and the page's contentY Behavior animates to whichever target was set last. Deferring the row reveal
    // to the end of the event makes it the last one (seen in the simulator: the help text of a switch reached with
    // DOWN from below the screen stayed cut off).
    readonly property Item keypadFocusItem: Window.activeFocusItem

    onKeypadFocusItemChanged: {
        if (ui.keyNavigationActive) {
            Qt.callLater(root.revealFocusedRow);
        }
    }

    function revealFocusedRow() {
        if (!ui.keyNavigationActive || !root.settingsPage) {
            return;
        }

        for (let item = keypadFocusItem; item && item.parent; item = item.parent) {
            if (item.parent === root) {
                root.settingsPage.ensureVisible(item);
                return;
            }
        }
    }

    // MESSAGES: the heading and the first switch, one row for the reveal
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 20
            Layout.topMargin: 20
            wrapMode: Text.WordWrap
            color: colors.textPrimary
            //: Heading in the user interface settings, above the settings for error pop-ups and banners
            text: qsTr("Messages")
            font: fonts.heading()
        }

        Components.SettingRow {
            //: Switch in the user interface settings: whether errors open a full-screen pop-up that has to be dismissed before the buttons work again
            title: qsTr("Show error pop-ups")
            //: Help text of the "Show error pop-ups" switch
            help: qsTr("For failed commands, an unavailable touch slider and integration connection errors. Other pop-ups, such as low battery, always show.")
            showDivider: Config.showErrorPopups

            Components.Switch {
                id: errorPopupsSwitch
                objectName: "errorPopupsSwitch"
                icon: "uc:check"
                checked: Config.showErrorPopups
                trigger: function() { Config.showErrorPopups = !Config.showErrorPopups; }
                highlight: activeFocus && ui.keyNavigationActive
                Accessible.name: "Show error pop-ups"
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: Config.showErrorPopups ? unavailableBannersSwitch : errorBannerSwitch
            }
        }
    }

    // --- with the error pop-ups off: the error banner and its duration ---
    Components.SettingRow {
        visible: !Config.showErrorPopups
        //: Switch in the user interface settings, shown when "Show error pop-ups" is off: a banner in place of the pop-up. Keep it short, the switch sits on the same line
        title: qsTr("Show a banner")
        //: Help text of the "Show a banner" switch
        help: qsTr("Instead of the pop-up: a short message at the top of the screen that closes by itself and leaves the buttons working. To try again, press the button again.")
        showDivider: !Config.showErrorBanner

        Components.Switch {
            id: errorBannerSwitch
            objectName: "errorBannerSwitch"
            icon: "uc:check"
            checked: Config.showErrorBanner
            trigger: function() { Config.showErrorBanner = !Config.showErrorBanner; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Show a banner"
            KeyNavigation.up: errorPopupsSwitch
            KeyNavigation.down: Config.showErrorBanner ? errorBannerSlider : unavailableBannersSwitch
        }
    }

    Components.SettingRow {
        visible: !Config.showErrorPopups && Config.showErrorBanner
        //: Slider in the user interface settings: how long the banner shown in place of an error pop-up stays
        title: qsTr("Error banner duration")
        value: Config.errorBannerSec + " s"
        controlBelow: true

        Components.Slider {
            id: errorBannerSlider
            objectName: "errorBannerSlider"
            height: 60
            // Config clamps the setting to the same range
            from: 1; to: 30; stepSize: 1; live: true; showLiveValue: false
            value: Config.errorBannerSec
            onMoved: Config.errorBannerSec = value
            onUserInteractionEnded: Config.errorBannerSec = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Error banner duration " + value + " s"
            KeyNavigation.up: errorBannerSwitch
            KeyNavigation.down: unavailableBannersSwitch
        }
    }

    // --- "Show offline banners" (a tile or button pressed for an unavailable device), and the duration of every banner
    // but the error banner ---
    Components.SettingRow {
        //: Switch in the user interface settings: whether a short message says so when a tile or button is pressed for a device that is offline (unavailable). Keep it short, the switch sits on the same line
        title: qsTr("Show offline banners")
        //: Help text of the "Show offline banners" switch
        help: qsTr("A short message at the top of the screen when you press a tile or button for a device that is unavailable. Banners about settings and setup always show.")

        Components.Switch {
            id: unavailableBannersSwitch
            objectName: "unavailableBannersSwitch"
            icon: "uc:check"
            checked: Config.showUnavailableBanners
            trigger: function() { Config.showUnavailableBanners = !Config.showUnavailableBanners; }
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Show offline banners"
            KeyNavigation.up: Config.showErrorPopups ? errorPopupsSwitch
                                                     : Config.showErrorBanner ? errorBannerSlider : errorBannerSwitch
            KeyNavigation.down: otherBannerSlider
        }
    }

    Components.SettingRow {
        //: Slider in the user interface settings: how long the remote's banners other than the error banner stay on screen
        title: qsTr("Other banner duration")
        //: Help text of the "Other banner duration" slider
        help: qsTr("For every banner except the error banner, such as a tile pressed for an unavailable device or a setting that could not be changed.")
        value: Config.otherBannerSec + " s"
        controlBelow: true
        showDivider: false

        Components.Slider {
            id: otherBannerSlider
            objectName: "otherBannerSlider"
            height: 60
            // Config clamps the setting to the same range
            from: 1; to: 30; stepSize: 1; live: true; showLiveValue: false
            value: Config.otherBannerSec
            onMoved: Config.otherBannerSec = value
            onUserInteractionEnded: Config.otherBannerSec = value
            highlight: activeFocus && ui.keyNavigationActive
            Accessible.name: "Other banner duration " + value + " s"
            KeyNavigation.up: unavailableBannersSwitch
            KeyNavigation.down: root.navDownTarget
        }
    }
}
