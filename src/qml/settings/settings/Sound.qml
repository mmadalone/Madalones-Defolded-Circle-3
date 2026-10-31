// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Dock chime picker and chime timing.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

 
import Haptic 1.0
import Config 1.0
import SoundEffects 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: soundPageContent
    scrollTarget: flickable
    initialFocusItem: soundEffectsSwitch

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

            /** SOUND EFFECTS **/
            Components.SettingRow {
                title: qsTr("Sound effects")

                Components.Switch {
                    id: soundEffectsSwitch
                    icon: "uc:check"
                    checked: Config.soundEnabled
                    trigger: function() {
                        Config.soundEnabled = !Config.soundEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: soundEffectsVolumeSlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** SOUND EFFECTS VOLUME **/
            Components.SettingRow {
                title: qsTr("Sound effects volume")
                controlBelow: true

                Components.Slider {
                    id: soundEffectsVolumeSlider
                    height: 60
                    from: 0
                    to: 100
                    stepSize: 1
                    value: Config.soundVolume
                    live: true

                    onUserInteractionEnded: {
                        Config.soundVolume = value;
                        SoundEffects.play(SoundEffects.Click);
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: soundEffectsSwitch
                    KeyNavigation.down: dockChimePicker
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** DOCK CHIME (madalone, v1.4.41 / v1.4.42) **/
            Components.SettingRow {
                title: qsTr("Dock chime")
                help: qsTr("Plays when you place the remote on the dock. Choosing a variant plays it as a preview.")
                controlBelow: true

                // 12 chips in 3 columns, explicit x/y math (a GridLayout costs ~30 ms per child on the device).
                Item {
                    id: dockChimePicker
                    width: parent.width
                    height: 4 * cellHeight + 3 * cellSpacing

                    readonly property real cellSpacing: 10
                    readonly property real cellWidth: (width - 2 * cellSpacing) / 3
                    readonly property real cellHeight: 60
                    readonly property int variantCount: 12

                    // Config.dockChimeVariant previews the chime when it changes (SoundEffects::setDockChimeVariant)
                    function step(delta) {
                        Config.dockChimeVariant = ((Config.dockChimeVariant - 1 + delta + variantCount) % variantCount) + 1;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: soundEffectsVolumeSlider
                    KeyNavigation.down: dockChimeAfterSwitch
                    Keys.onLeftPressed: step(-1)
                    Keys.onRightPressed: step(1)

                    Components.Selectable {
                        anchors.margins: -6
                        selected: dockChimePicker.activeFocus
                    }

                    Repeater {
                        model: [
                            { variant: 1,  label: qsTr("Warp") },
                            { variant: 2,  label: qsTr("Ascend") },
                            { variant: 3,  label: qsTr("Bell") },
                            { variant: 4,  label: qsTr("Chord") },
                            { variant: 5,  label: qsTr("Pulse") },
                            { variant: 6,  label: qsTr("Zap") },
                            { variant: 7,  label: qsTr("Pwr Down") },
                            { variant: 8,  label: qsTr("Pwr Hold") },
                            { variant: 9,  label: qsTr("Pwr Up 1") },
                            { variant: 10, label: qsTr("Pwr Up 2") },
                            { variant: 11, label: qsTr("TOS") },
                            { variant: 12, label: qsTr("TOS Long") }
                        ]

                        Rectangle {
                            x: (index % 3) * (dockChimePicker.cellWidth + dockChimePicker.cellSpacing)
                            y: Math.floor(index / 3) * (dockChimePicker.cellHeight + dockChimePicker.cellSpacing)
                            width: dockChimePicker.cellWidth
                            height: dockChimePicker.cellHeight
                            radius: ui.cornerRadiusSmall
                            readonly property bool chosen: Config.dockChimeVariant === modelData.variant
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

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onClicked: Config.dockChimeVariant = modelData.variant
                            }
                        }
                    }
                }
            }

            /** DOCK CHIME TIMING (madalone, v1.4.43) **/
            Components.SettingRow {
                title: qsTr("Chime after screensaver")
                help: qsTr("On: screensaver appears immediately, chime plays once it's visible (best for long chimes). Off: chime plays first, screensaver appears after the chime ends.")

                Components.Switch {
                    id: dockChimeAfterSwitch
                    icon: "uc:check"
                    checked: Config.dockChimeAfterScreensaver
                    trigger: function() {
                        Config.dockChimeAfterScreensaver = !Config.dockChimeAfterScreensaver;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: dockChimePicker
                    KeyNavigation.down: buttonBacklightSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** HAPTIC FEEDBACK **/
            Components.SettingRow {
                showDivider: false
                title: qsTr("Haptic feedback")

                Components.Switch {
                    id: buttonBacklightSwitch
                    icon: "uc:check"
                    checked: Config.hapticEnabled
                    trigger: function() {
                        Config.hapticEnabled = !Config.hapticEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: dockChimeAfterSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }
        }
    }
}
