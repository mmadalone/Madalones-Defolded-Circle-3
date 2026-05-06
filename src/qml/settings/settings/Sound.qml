// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
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
            // madalone: cap raised to 1750 to accommodate the new "Chime after screensaver"
            // toggle + description (was 1600 with just the 12-variant grid).
            if (contentY > 1750) {
                contentY = 1750;
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

            /** SOUND EFFECTS **/
            RowLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                Text {
                    id: soundEffectsText
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Sound effects")
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: soundEffectsSwitch
                    icon: "uc:check"
                    checked: Config.soundEnabled
                    trigger: function() {
                        Config.soundEnabled = !Config.soundEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: soundEffectsVolumeSlider
                    highlight: activeFocus && ui.keyNavigationEnabled

                    Component.onCompleted: {
                        soundEffectsSwitch.forceActiveFocus();
                    }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** SOUND EFFECTS VOLUME **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height

                Text {
                    id: soundEffectsVolumeText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Sound effects volume")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Components.Slider {
                    id: soundEffectsVolumeSlider
                    height: 60
                    from: 0
                    to: 100
                    stepSize: 1
                    value: Config.soundVolume
                    live: true
                    anchors { top: soundEffectsVolumeText.bottom; topMargin: 10 }

                    onUserInteractionEnded: {
                        Config.soundVolume = value;
                        SoundEffects.play(SoundEffects.Click);
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: soundEffectsSwitch
                    KeyNavigation.down: buttonBacklightSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** DOCK CHIME VARIANT (madalone) **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                Layout.fillWidth: true
                spacing: 10

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Dock chime")
                    font: fonts.primaryFont(30)
                }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Tap a variant to preview and select. The chime plays when you place the remote on the dock.")
                    font: fonts.primaryFont(22)
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    rowSpacing: 10
                    columnSpacing: 10

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
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            radius: 8
                            color: Config.dockChimeVariant === modelData.variant ? colors.offwhite : colors.dark
                            border { color: colors.medium; width: 1 }
                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                color: Config.dockChimeVariant === modelData.variant ? colors.black : colors.offwhite
                                font: fonts.primaryFont(22)
                                elide: Text.ElideRight
                                width: parent.width - 12
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

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** DOCK CHIME TIMING (madalone v1.4.43) **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                Layout.fillWidth: true
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Chime after screensaver")
                        font: fonts.primaryFont(30)
                    }
                    Components.Switch {
                        id: dockChimeAfterSwitch
                        icon: "uc:check"
                        checked: Config.dockChimeAfterScreensaver
                        trigger: function() { Config.dockChimeAfterScreensaver = !Config.dockChimeAfterScreensaver; }
                        highlight: activeFocus && ui.keyNavigationEnabled
                    }
                }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("On: screensaver appears immediately, chime plays once it's visible (best for long chimes). Off: chime plays first, screensaver appears after the chime ends.")
                    font: fonts.primaryFont(22)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** HAPTIC FEEDBACK **/
            RowLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                Text {
                    id: hapticFeedbackText
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Haptic feedback")
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: buttonBacklightSwitch
                    icon: "uc:check"
                    checked: Config.hapticEnabled
                    trigger: function() {
                        Config.hapticEnabled = !Config.hapticEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: soundEffectsVolumeSlider
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }
        }
    }
}
