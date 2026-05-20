// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Adds "Screen off animations" section (shared screensaver shutdown effect) + Active Session Keeper section.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import HwInfo 1.0
import Haptic 1.0
import Config 1.0
import ScreensaverConfig 1.0
import Wifi 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: powerPageContent

    function secondsToTime(e){
        let m = Math.floor(e % 3600 / 60).toString();
        let s = Math.floor(e % 60).toString();

        let mDisplay = m > 0 ? m + "m" : "";
        let sDisplay = s > 0 ? s + "s" : "";

        return mDisplay + sDisplay;
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        onContentYChanged: {
            if (contentY < 0) {
                contentY = 0;
            }
            var maxY = contentHeight - height;
            if (maxY > 0 && contentY > maxY) {
                contentY = maxY;
            }
        }

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 20
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            /** PHANTOM-WAKE SUPPRESSOR (madalone, v1.4.20, Mod 6) **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Suppress phantom wake-ups")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: phantomWakeSwitch
                        icon: "uc:check"
                        checked: Config.phantomWakeSuppressEnabled
                        trigger: function() { Config.phantomWakeSuppressEnabled = !Config.phantomWakeSuppressEnabled; }
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.down: Config.phantomWakeSuppressEnabled ? phantomWakeGraceSlider : sessionKeeperSwitch
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Forces the device back to standby if a wake event happens with no user input within %1 ms.").arg(Config.phantomWakeSuppressGraceMs)
                    font: fonts.secondaryFont(24)
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    visible: Config.phantomWakeSuppressEnabled

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: qsTr("Grace window after wake: %1 ms").arg(Config.phantomWakeSuppressGraceMs)
                        font: fonts.secondaryFont(24)
                    }

                    Components.Slider {
                        id: phantomWakeGraceSlider
                        height: 60
                        Layout.fillWidth: true
                        from: 100
                        to: 5000
                        stepSize: 100
                        value: Config.phantomWakeSuppressGraceMs
                        live: true
                        onValueChanged: { Config.phantomWakeSuppressGraceMs = value; }
                        onUserInteractionEnded: { Config.phantomWakeSuppressGraceMs = value; }
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.up: phantomWakeSwitch
                        KeyNavigation.down: phantomWakeLookbackSlider
                    }

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: Config.phantomWakeSuppressInputLookbackMs > 0
                              ? qsTr("Recent-input lookback: %1 ms").arg(Config.phantomWakeSuppressInputLookbackMs)
                              : qsTr("Recent-input lookback: disabled")
                        font: fonts.secondaryFont(24)
                    }

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: qsTr("Skip the grace timer when a button-press arrived this recently before a wake.")
                        font: fonts.secondaryFont(24)
                    }

                    Components.Slider {
                        id: phantomWakeLookbackSlider
                        height: 60
                        Layout.fillWidth: true
                        from: 0
                        to: 2000
                        stepSize: 50
                        value: Config.phantomWakeSuppressInputLookbackMs
                        live: true
                        onValueChanged: { Config.phantomWakeSuppressInputLookbackMs = value; }
                        onUserInteractionEnded: { Config.phantomWakeSuppressInputLookbackMs = value; }
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.up: phantomWakeGraceSlider
                        KeyNavigation.down: sessionKeeperSwitch
                    }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** ACTIVE SESSION KEEPER (madalone) **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Keep awake while watching/listening")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: sessionKeeperSwitch
                        icon: "uc:check"
                        checked: Config.sessionKeeperEnabled
                        trigger: function() { Config.sessionKeeperEnabled = !Config.sessionKeeperEnabled; }
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.up: Config.phantomWakeSuppressEnabled ? phantomWakeLookbackSlider : phantomWakeSwitch
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Resets the device's sleep countdown every 4.5 min while media is playing or you've recently pressed a button.")
                    font: fonts.secondaryFont(24)
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    visible: Config.sessionKeeperEnabled

                    Text {
                        id: idleTimeoutLabel
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: qsTr("Idle timeout after last button: %1 seconds").arg(Config.sessionKeeperIdleSec)
                        font: fonts.secondaryFont(24)
                    }

                    Components.Slider {
                        height: 60   // matches GeneralBehavior idleTimeoutSlider; no low/high labels (value shown in title above)
                        Layout.fillWidth: true
                        from: 30
                        to: 300
                        stepSize: 30
                        value: Config.sessionKeeperIdleSec
                        live: true
                        onValueChanged: { Config.sessionKeeperIdleSec = value; }
                        onUserInteractionEnded: { Config.sessionKeeperIdleSec = value; }
                        highlight: activeFocus && ui.keyNavigationEnabled
                    }
                }

                RowLayout {
                    spacing: 10
                    visible: Config.sessionKeeperEnabled

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Only when on charger or dock")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: sessionKeeperRequireAcSwitch
                        icon: "uc:check"
                        checked: Config.sessionKeeperRequireAc
                        trigger: function() { Config.sessionKeeperRequireAc = !Config.sessionKeeperRequireAc; }
                        highlight: activeFocus && ui.keyNavigationEnabled
                    }
                }

            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            //** WAKE ON WLAN **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10
                visible: HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled

                RowLayout {
                    spacing: 10

                    Text {
                        id: wowlanText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        //: Title for indication of wifi always on functionality
                        text: qsTr("Keep WiFi connected in standby")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: wowlanSwitch
                        icon: "uc:check"
                        checked: Config.wowlanEnabled
                        trigger: function() {
                            Config.wowlanEnabled = !Config.wowlanEnabled;
                        }

                        /** KEYBOARD NAVIGATION **/
                        KeyNavigation.down: resumeTimeoutValueSlider
                        highlight: activeFocus && ui.keyNavigationEnabled

                        Component.onCompleted: {
                            if (Wifi.wowlanEnabled) {
                                wowlanSwitch.forceActiveFocus();
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Faster reconnect after wake; minor battery cost.")
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** RESUME TIMEOUT WINDOW **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height + 40

                Text {
                    id: resumeTimeoutValueText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Retry commands after wakeup")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    id: resumeTimeoutValueSmallText
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Retry commands within %1 second(s) after wakeup.").arg(Config.resumeTimeoutWindowSec)
                    anchors { left: parent.left; top:resumeTimeoutValueText.bottom; topMargin: 5 }
                    font: fonts.secondaryFont(24)
                }

                Components.Slider {
                    id: resumeTimeoutValueSlider
                    height: 60
                    from: 0
                    to: 10
                    stepSize: 1
                    value: Config.resumeTimeoutWindowSec
                    lowValueText: qsTr("Disabled")
                    highValueText: qsTr("%1 seconds").arg(to)
                    live: true
                    anchors { top: resumeTimeoutValueSmallText.bottom; topMargin: 10 }

                    onValueChanged: {
                        Config.resumeTimeoutWindowSec = value;
                    }

                    onUserInteractionEnded: {
                        Config.resumeTimeoutWindowSec = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: (HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled) ? wowlanSwitch : undefined
                    KeyNavigation.down: wakeupSensitivitySlider
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
                visible: HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled
            }

            /** WAKEUP SENSITIVITY **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height + 40

                Text {
                    id: wakeupSensitivityText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    //: Movement the remote reacts to wake up
                    text: qsTr("Wakeup sensitivity")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    id: wakeupSensitivitySmallText
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Amount of movement needed to wake up the remote.")
                    anchors { left: parent.left; top:wakeupSensitivityText.bottom; topMargin: 5 }
                    font: fonts.secondaryFont(24)
                }

                Components.Slider {
                    id: wakeupSensitivitySlider
                    height: 60
                    from: 0
                    to: 3
                    stepSize: 1
                    value: Config.wakeupSensitivity
                    showLiveValue: false
                    showTicks: true
                    //: Wakeup is turned off
                    lowValueText: qsTr("Off")
                    //: More sensitive wakeup setting, as in the remote will be more sensitive to movement
                    highValueText: qsTr("Sensitivity")
                    anchors { top: wakeupSensitivitySmallText.bottom; topMargin: 10 }

                    onUserInteractionEnded: {
                        Config.wakeupSensitivity = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: resumeTimeoutValueSlider
                    KeyNavigation.down: displayoffTimeoutSlider
                    highlight: activeFocus && ui.keyNavigationEnabled

                    Component.onCompleted: {
                        if (HwInfo.modelNumber != "UCR3") {
                            wakeupSensitivitySlider.forceActiveFocus();
                        }
                    }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** DISPLAY TIMEOUT **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height + 40

                Text {
                    id: displayTimeoutText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    //: How much time the display will turn off after
                    text: qsTr("Display off timeout")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    color: colors.light
                    text: Config.displayTimeout + "s"
                    anchors { right: parent.right; baseline: displayTimeoutText.baseline }
                    font: fonts.secondaryFont(24)
                }

                Components.Slider {
                    id: displayoffTimeoutSlider
                    height: 60
                    from: 10
                    to: 60
                    stepSize: 1
                    live: true
                    value: Config.displayTimeout
                    lowValueText: qsTr("%1 seconds").arg(from)
                    highValueText: qsTr("%1 seconds").arg(to)
                    anchors { top: displayTimeoutText.bottom; topMargin: 10 }

                    onValueChanged: {
                        valueDisplayText = value + "s"
                    }

                    onUserInteractionEnded: {
                        Config.displayTimeout = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: wakeupSensitivitySlider
                    KeyNavigation.down: screenOffEnabledSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** SCREEN OFF ANIMATIONS (madalone) **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                // Explicit height based on the last child's bottom. childrenRect
                // doesn't reliably include a GridLayout's implicit height with
                // anchor-based positioning, causing the style picker to overlap
                // the next section when it has more than one row of buttons.
                height: screenOffStyleRow.y + screenOffStyleRow.height + 20

                Text {
                    id: screenOffTitle
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Screen off animations")
                    anchors { left: parent.left; top: parent.top }
                    font: fonts.primaryFont(30)
                }

                /** MASTER ENABLE **/
                Text {
                    id: screenOffEnabledLabel
                    color: colors.light
                    text: qsTr("Enabled")
                    anchors { left: parent.left; top: screenOffTitle.bottom; topMargin: 14 }
                    font: fonts.secondaryFont(24)
                }
                Components.Switch {
                    id: screenOffEnabledSwitch
                    icon: "uc:check"
                    checked: ScreensaverConfig.screenOffEffectEnabled
                    trigger: function() { ScreensaverConfig.screenOffEffectEnabled = !ScreensaverConfig.screenOffEffectEnabled; }
                    anchors { right: parent.right; verticalCenter: screenOffEnabledLabel.verticalCenter }
                    KeyNavigation.up: displayoffTimeoutSlider
                    KeyNavigation.down: screenOffUndockedSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }

                /** FIRE WHEN UNDOCKED **/
                Text {
                    id: screenOffUndockedLabel
                    color: colors.light
                    text: qsTr("Fire when undocked")
                    anchors { left: parent.left; top: screenOffEnabledLabel.bottom; topMargin: 34 }
                    font: fonts.secondaryFont(24)
                    opacity: ScreensaverConfig.screenOffEffectEnabled ? 1.0 : 0.4
                }
                Components.Switch {
                    id: screenOffUndockedSwitch
                    icon: "uc:check"
                    checked: ScreensaverConfig.screenOffEffectUndocked
                    enabled: ScreensaverConfig.screenOffEffectEnabled
                    opacity: enabled ? 1.0 : 0.4
                    trigger: function() { ScreensaverConfig.screenOffEffectUndocked = !ScreensaverConfig.screenOffEffectUndocked; }
                    anchors { right: parent.right; verticalCenter: screenOffUndockedLabel.verticalCenter }
                    KeyNavigation.up: screenOffEnabledSwitch
                    KeyNavigation.down: screenOffStyleRow
                    highlight: activeFocus && ui.keyNavigationEnabled
                }

                /** STYLE PICKER (fade / flash / vignette / wipe / theme-native) **/
                Text {
                    id: screenOffStyleLabel
                    color: colors.light
                    text: qsTr("Style")
                    anchors { left: parent.left; top: screenOffUndockedLabel.bottom; topMargin: 34 }
                    font: fonts.secondaryFont(24)
                    opacity: ScreensaverConfig.screenOffEffectEnabled ? 1.0 : 0.4
                }
                // v1.4.26: replaced GridLayout with Item + computed-position Repeater children.
                // GridLayout ran Qt's constraint solver on every child added during instantiation
                // (~30 ms × 9 = ~270 ms visible cascade on ARM). Explicit x/y math is O(1) per
                // child — instantiation drops below one frame. Same visual output, no change to
                // selection logic or KeyNavigation.
                Item {
                    id: screenOffStyleRow
                    // 9 buttons in 3 cols × 3 rows × 44 px tall + 2 × 6 px spacing.
                    height: 3 * 44 + 2 * 6
                    focus: true
                    enabled: ScreensaverConfig.screenOffEffectEnabled
                    opacity: enabled ? 1.0 : 0.4
                    anchors { left: parent.left; right: parent.right; top: screenOffStyleLabel.bottom; topMargin: 8 }

                    readonly property real cellSpacing: 6
                    readonly property real cellWidth:  (width - 2 * cellSpacing) / 3
                    readonly property real cellHeight: 44

                    KeyNavigation.up: screenOffUndockedSwitch
                    KeyNavigation.down: sleepTimeoutSlider
                    Keys.onLeftPressed: {
                        var styles = ["fade","flash","vignette","wipe","sleepwave","genie","pixelate","dissolve","theme-native"];
                        var cur = ScreensaverConfig.screenOffEffectStyle;
                        for (var i = 0; i < styles.length; i++) {
                            if (styles[i] === cur) {
                                var next = i > 0 ? i - 1 : styles.length - 1;
                                ScreensaverConfig.screenOffEffectStyle = styles[next];
                                Haptic.play(Haptic.Click);
                                return;
                            }
                        }
                    }
                    Keys.onRightPressed: {
                        var styles = ["fade","flash","vignette","wipe","sleepwave","genie","pixelate","dissolve","theme-native"];
                        var cur = ScreensaverConfig.screenOffEffectStyle;
                        for (var i = 0; i < styles.length; i++) {
                            if (styles[i] === cur) {
                                var next = (i + 1) % styles.length;
                                ScreensaverConfig.screenOffEffectStyle = styles[next];
                                Haptic.play(Haptic.Click);
                                return;
                            }
                        }
                    }
                    Repeater {
                        model: [
                            { name: "fade",         label: "Fade" },
                            { name: "flash",        label: "Flash" },
                            { name: "vignette",     label: "Iris" },
                            { name: "wipe",         label: "Wipe" },
                            { name: "sleepwave",    label: "Wave" },
                            { name: "genie",        label: "Genie" },
                            { name: "pixelate",     label: "Pixels" },
                            { name: "dissolve",     label: "Dissolve" },
                            { name: "theme-native", label: "Theme" }
                        ]
                        Rectangle {
                            // Explicit grid math — 3 cols, index → (col, row).
                            x: (index % 3) * (screenOffStyleRow.cellWidth + screenOffStyleRow.cellSpacing)
                            y: Math.floor(index / 3) * (screenOffStyleRow.cellHeight + screenOffStyleRow.cellSpacing)
                            width: screenOffStyleRow.cellWidth
                            height: screenOffStyleRow.cellHeight
                            radius: 8
                            // v1.4.26 polish: single shared binding for selected-state instead of
                            // two parallel string-compares (color + text color). 50% fewer compares
                            // when ScreensaverConfig.screenOffEffectStyle changes.
                            readonly property bool selected: ScreensaverConfig.screenOffEffectStyle === modelData.name
                            color: selected ? colors.offwhite : colors.dark
                            border { color: colors.medium; width: 1 }
                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                color: parent.selected ? colors.black : colors.offwhite
                                font: fonts.primaryFont(18)
                                elide: Text.ElideRight
                                width: parent.width - 8
                                horizontalAlignment: Text.AlignHCenter
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    ScreensaverConfig.screenOffEffectStyle = modelData.name;
                                    Haptic.play(Haptic.Click);
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** SLEEP TIMEOUT **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height

                Text {
                    id: sleepTimeoutText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    //: How much time the remote will enter sleep mode after
                    text: qsTr("Sleep timeout")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    color: colors.light
                    text:  secondsToTime(Config.sleepTimeout)
                    anchors { right: parent.right; baseline: sleepTimeoutText.baseline }
                    font: fonts.secondaryFont(24)
                }

                Components.Slider {
                    id: sleepTimeoutSlider
                    height: 60
                    from: 10
                    to: 300
                    stepSize: 1
                    live: true
                    value: Config.sleepTimeout
                    lowValueText: qsTr("%1 seconds").arg(from)
                    highValueText: qsTr("%1 minutes").arg(5)
                    anchors { top: sleepTimeoutText.bottom; topMargin: 10 }

                    onValueChanged: {
                        valueDisplayText = secondsToTime(value);
                    }

                    onUserInteractionEnded: {
                        Config.sleepTimeout = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: screenOffStyleRow
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Item { Layout.preferredHeight: 40 }
        }
    }
}
