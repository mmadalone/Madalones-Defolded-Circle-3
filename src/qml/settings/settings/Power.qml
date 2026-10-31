// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Phantom-wake suppressor (Mod 6), keep awake while watching (Mod 5), screen-off animations.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import HwInfo 1.0
import Haptic 1.0
import Config 1.0
import Wifi 1.0
import ScreensaverConfig 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: powerPageContent
    scrollTarget: flickable
    initialFocusItem: phantomWakeSwitch

    // the wowlan switch is hidden on some models
    readonly property bool wowlanShown: HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled
    // madalone: the last visible row of the keep-awake section, where the keypad chain continues
    readonly property Item sessionKeeperLast: Config.sessionKeeperEnabled ? sessionKeeperRequireAcSwitch
                                                                          : sessionKeeperSwitch

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
        boundsBehavior: Flickable.StopAtBounds

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 0
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            /** PHANTOM-WAKE SUPPRESSOR (madalone, Mod 6) **/
            Components.SettingRow {
                title: qsTr("Suppress phantom wake-ups")
                help: qsTr("Forces the device back to standby if a wake event happens with no user input within %1 ms.").arg(Config.phantomWakeSuppressGraceMs)
                showDivider: !Config.phantomWakeSuppressEnabled

                Components.Switch {
                    id: phantomWakeSwitch
                    icon: "uc:check"
                    checked: Config.phantomWakeSuppressEnabled
                    trigger: function() {
                        Config.phantomWakeSuppressEnabled = !Config.phantomWakeSuppressEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: Config.phantomWakeSuppressEnabled ? phantomWakeGraceSlider : sessionKeeperSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                visible: Config.phantomWakeSuppressEnabled
                title: qsTr("Grace window after wake")
                value: Config.phantomWakeSuppressGraceMs + " ms"
                controlBelow: true
                controlBottomSpace: 40
                showDivider: false

                Components.Slider {
                    id: phantomWakeGraceSlider
                    height: 60
                    from: 100
                    to: 5000
                    stepSize: 100
                    live: true
                    showLiveValue: false
                    value: Config.phantomWakeSuppressGraceMs
                    lowValueText: from + " ms"
                    highValueText: to + " ms"

                    onValueChanged: { Config.phantomWakeSuppressGraceMs = value; }
                    onUserInteractionEnded: { Config.phantomWakeSuppressGraceMs = value; }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: phantomWakeSwitch
                    KeyNavigation.down: phantomWakeLookbackSlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                visible: Config.phantomWakeSuppressEnabled
                title: qsTr("Recent-input lookback")
                value: Config.phantomWakeSuppressInputLookbackMs > 0 ? Config.phantomWakeSuppressInputLookbackMs + " ms"
                                                                     : qsTr("Disabled")
                help: qsTr("Skip the grace timer when a button-press arrived this recently before a wake.")
                controlBelow: true
                controlBottomSpace: 40

                Components.Slider {
                    id: phantomWakeLookbackSlider
                    height: 60
                    from: 0
                    to: 2000
                    stepSize: 50
                    live: true
                    showLiveValue: false
                    value: Config.phantomWakeSuppressInputLookbackMs
                    lowValueText: qsTr("Disabled")
                    highValueText: to + " ms"

                    onValueChanged: { Config.phantomWakeSuppressInputLookbackMs = value; }
                    onUserInteractionEnded: { Config.phantomWakeSuppressInputLookbackMs = value; }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: phantomWakeGraceSlider
                    KeyNavigation.down: sessionKeeperSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** KEEP AWAKE WHILE WATCHING (madalone, Mod 5) **/
            Components.SettingRow {
                title: qsTr("Keep awake while watching/listening")
                help: qsTr("Keeps the remote from going to sleep while media is playing, and for the time below after you press a media button.")
                showDivider: !Config.sessionKeeperEnabled

                Components.Switch {
                    id: sessionKeeperSwitch
                    icon: "uc:check"
                    checked: Config.sessionKeeperEnabled
                    trigger: function() {
                        Config.sessionKeeperEnabled = !Config.sessionKeeperEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: Config.phantomWakeSuppressEnabled ? phantomWakeLookbackSlider : phantomWakeSwitch
                    KeyNavigation.down: Config.sessionKeeperEnabled ? sessionKeeperIdleSlider
                                                                    : (powerPageContent.wowlanShown ? wowlanSwitch : resumeTimeoutValueSlider)
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                visible: Config.sessionKeeperEnabled
                title: qsTr("Idle timeout after last button")
                value: Config.sessionKeeperIdleSec + "s"
                controlBelow: true
                controlBottomSpace: 40
                showDivider: false

                Components.Slider {
                    id: sessionKeeperIdleSlider
                    height: 60
                    from: 30
                    to: 300
                    stepSize: 30
                    live: true
                    showLiveValue: false
                    value: Config.sessionKeeperIdleSec
                    lowValueText: qsTr("%1 seconds").arg(from)
                    highValueText: qsTr("%1 minutes").arg(5)

                    onValueChanged: { Config.sessionKeeperIdleSec = value; }
                    onUserInteractionEnded: { Config.sessionKeeperIdleSec = value; }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: sessionKeeperSwitch
                    KeyNavigation.down: sessionKeeperRequireAcSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                visible: Config.sessionKeeperEnabled
                title: qsTr("Only when on charger or dock")

                Components.Switch {
                    id: sessionKeeperRequireAcSwitch
                    icon: "uc:check"
                    checked: Config.sessionKeeperRequireAc
                    trigger: function() {
                        Config.sessionKeeperRequireAc = !Config.sessionKeeperRequireAc;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: sessionKeeperIdleSlider
                    KeyNavigation.down: powerPageContent.wowlanShown ? wowlanSwitch : resumeTimeoutValueSlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            //** WAKE ON WLAN **/
            Components.SettingRow {
                visible: HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled
                //: Title for indication of wifi always on functionality
                title: qsTr("Keep WiFi connected in standby")
                help: qsTr("Keeps WiFi always connected, even when the device is sleeping. Allows for faster reconnect after wakeup. Please note that enabling this feature slightly decreases battery life.")

                Components.Switch {
                    id: wowlanSwitch
                    icon: "uc:check"
                    checked: Config.wowlanEnabled
                    trigger: function() {
                        Config.wowlanEnabled = !Config.wowlanEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: powerPageContent.sessionKeeperLast
                    KeyNavigation.down: resumeTimeoutValueSlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** RESUME TIMEOUT WINDOW **/
            Components.SettingRow {
                title: qsTr("Retry commands after wakeup")
                help: qsTr("Retry commands within %1 second(s) after wakeup.").arg(Config.resumeTimeoutWindowSec)
                controlBelow: true
                controlBottomSpace: 40

                Components.Slider {
                    id: resumeTimeoutValueSlider
                    height: 60
                    from: 0
                    to: 10
                    stepSize: 1
                    value: Config.resumeTimeoutWindowSec
                    //: The feature is switched off. Not a person with a disability.
                    lowValueText: qsTr("Disabled")
                    highValueText: qsTr("%1 seconds").arg(to)
                    live: true

                    onValueChanged: {
                        Config.resumeTimeoutWindowSec = value;
                    }

                    onUserInteractionEnded: {
                        Config.resumeTimeoutWindowSec = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: powerPageContent.wowlanShown ? wowlanSwitch : powerPageContent.sessionKeeperLast
                    KeyNavigation.down: wakeupSensitivitySlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** WAKEUP SENSITIVITY **/
            Components.SettingRow {
                //: Movement the remote reacts to wake up
                title: qsTr("Wakeup sensitivity")
                help: qsTr("Amount of movement needed to wake up the remote.")
                controlBelow: true
                controlBottomSpace: 40

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

                    onUserInteractionEnded: {
                        Config.wakeupSensitivity = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: resumeTimeoutValueSlider
                    KeyNavigation.down: displayoffTimeoutSlider
                    highlight: activeFocus && ui.keyNavigationActive

                }
            }

            /** DISPLAY TIMEOUT **/
            Components.SettingRow {
                //: How much time the display will turn off after
                title: qsTr("Display off timeout")
                value: Config.displayTimeout + "s"
                controlBelow: true
                controlBottomSpace: 40

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

                    onValueChanged: {
                        valueDisplayText = value + "s"
                    }

                    onUserInteractionEnded: {
                        Config.displayTimeout = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: wakeupSensitivitySlider
                    KeyNavigation.down: screenOffEnabledSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** SCREEN OFF ANIMATIONS (madalone, Mod 1) **/
            Components.SettingRow {
                title: qsTr("Screen off animations")
                help: qsTr("Plays an animation on the screensaver just before the display turns off.")
                showDivider: false

                Components.Switch {
                    id: screenOffEnabledSwitch
                    icon: "uc:check"
                    checked: ScreensaverConfig.screenOffEffectEnabled
                    trigger: function() {
                        ScreensaverConfig.screenOffEffectEnabled = !ScreensaverConfig.screenOffEffectEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: displayoffTimeoutSlider
                    KeyNavigation.down: screenOffUndockedSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                title: qsTr("Fire when undocked")
                showDivider: false

                Components.Switch {
                    id: screenOffUndockedSwitch
                    icon: "uc:check"
                    checked: ScreensaverConfig.screenOffEffectUndocked
                    enabled: ScreensaverConfig.screenOffEffectEnabled
                    opacity: enabled ? 1.0 : 0.4
                    trigger: function() {
                        ScreensaverConfig.screenOffEffectUndocked = !ScreensaverConfig.screenOffEffectUndocked;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: screenOffEnabledSwitch
                    KeyNavigation.down: screenOffStyleRow
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                title: qsTr("Style")
                controlBelow: true

                // 9 chips in 3 columns. LEFT/RIGHT walk the styles, a tap picks one. Explicit x/y math instead of a
                // GridLayout (v1.4.26: the layout solver cost ~270 ms on the device when the page opened).
                Item {
                    id: screenOffStyleRow
                    width: parent.width
                    height: 3 * cellHeight + 2 * cellSpacing
                    enabled: ScreensaverConfig.screenOffEffectEnabled
                    opacity: enabled ? 1.0 : 0.4

                    readonly property real cellSpacing: 10
                    readonly property real cellWidth: (width - 2 * cellSpacing) / 3
                    readonly property real cellHeight: 60
                    readonly property var styles: ["fade", "flash", "vignette", "wipe", "sleepwave", "genie",
                                                   "pixelate", "dissolve", "theme-native"]

                    function step(delta) {
                        var i = styles.indexOf(ScreensaverConfig.screenOffEffectStyle);
                        ScreensaverConfig.screenOffEffectStyle = styles[(Math.max(i, 0) + delta + styles.length) % styles.length];
                        Haptic.play(Haptic.Click);
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: screenOffUndockedSwitch
                    KeyNavigation.down: sleepTimeoutSlider
                    Keys.onLeftPressed: step(-1)
                    Keys.onRightPressed: step(1)

                    Components.Selectable {
                        anchors.margins: -6
                        selected: screenOffStyleRow.activeFocus
                    }

                    Repeater {
                        model: [
                            { name: "fade",         label: qsTr("Fade") },
                            { name: "flash",        label: qsTr("Flash") },
                            { name: "vignette",     label: qsTr("Iris") },
                            { name: "wipe",         label: qsTr("Wipe") },
                            { name: "sleepwave",    label: qsTr("Wave") },
                            { name: "genie",        label: qsTr("Genie") },
                            { name: "pixelate",     label: qsTr("Pixels") },
                            { name: "dissolve",     label: qsTr("Dissolve") },
                            { name: "theme-native", label: qsTr("Theme") }
                        ]

                        Rectangle {
                            x: (index % 3) * (screenOffStyleRow.cellWidth + screenOffStyleRow.cellSpacing)
                            y: Math.floor(index / 3) * (screenOffStyleRow.cellHeight + screenOffStyleRow.cellSpacing)
                            width: screenOffStyleRow.cellWidth
                            height: screenOffStyleRow.cellHeight
                            radius: ui.cornerRadiusSmall
                            readonly property bool chosen: ScreensaverConfig.screenOffEffectStyle === modelData.name
                            color: chosen ? colors.textPrimary : colors.surface
                            border { color: colors.divider; width: 2 }

                            Text {
                                anchors.centerIn: parent
                                width: parent.width - 12
                                text: modelData.label
                                color: parent.chosen ? colors.bg : colors.textPrimary
                                font: fonts.caption()
                                elide: Text.ElideRight
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

            /** SLEEP TIMEOUT **/
            Components.SettingRow {
                showDivider: false
                //: How much time the remote will enter sleep mode after
                title: qsTr("Sleep timeout")
                value: secondsToTime(Config.sleepTimeout)
                controlBelow: true
                controlBottomSpace: 40

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

                    onValueChanged: {
                        valueDisplayText = secondsToTime(value);
                    }

                    onUserInteractionEnded: {
                        Config.sleepTimeout = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: screenOffStyleRow
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }
        }
    }
}
