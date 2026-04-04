// Copyright (c) 2024 madalone. Matrix effects settings component.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: root

    required property Item settingsPage

    property alias firstFocusItem: invertTrailSwitch
    property alias lastFocusItem: messagePulseSwitch
    property Item navUpTarget
    property Item navDownTarget

    spacing: 20

    // Leading separator
    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 10c. INVERT TRAIL
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Invert trail"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: invertTrailSwitch
                objectName: "invertTrailSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixInvertTrail
                trigger: function() { Config.chargingMatrixInvertTrail = !Config.chargingMatrixInvertTrail; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: root.navUpTarget
                KeyNavigation.down: glowSwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 11. HEAD GLOW
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Head glow"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: glowSwitch
                objectName: "glowSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlow
                trigger: function() { Config.chargingMatrixGlow = !Config.chargingMatrixGlow; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: invertTrailSwitch; KeyNavigation.down: glitchSwitch
            }
        }
    }

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 11. GLITCH EFFECT
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Glitch effect"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: glitchSwitch
                objectName: "glitchSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitch
                trigger: function() { Config.chargingMatrixGlitch = !Config.chargingMatrixGlitch; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glowSwitch; KeyNavigation.down: glitchRateSlider
            }
        }
    }

    // 12. GLITCH INTENSITY (visible when glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Glitch intensity"); font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: glitchRateSlider
            objectName: "glitchRateSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 80; stepSize: 5
            value: Config.chargingMatrixGlitchRate; live: true
            onValueChanged: Config.chargingMatrixGlitchRate = value
            onUserInteractionEnded: Config.chargingMatrixGlitchRate = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: glitchSwitch; KeyNavigation.down: glitchFlashSwitch
        }
    }

    // 12b. COLUMN FLASH (sub-toggle, visible when glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Column flash"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchFlashSwitch
                objectName: "glitchFlashSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchFlash
                trigger: function() { Config.chargingMatrixGlitchFlash = !Config.chargingMatrixGlitchFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glitchRateSlider; KeyNavigation.down: glitchStutterSwitch
            }
        }
    }

    // 12c. COLUMN STUTTER
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Column stutter"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchStutterSwitch
                objectName: "glitchStutterSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchStutter
                trigger: function() { Config.chargingMatrixGlitchStutter = !Config.chargingMatrixGlitchStutter; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glitchFlashSwitch; KeyNavigation.down: glitchReverseSwitch
            }
        }
    }

    // 12d. REVERSE GLOW
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Reverse glow"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchReverseSwitch
                objectName: "glitchReverseSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchReverse
                trigger: function() { Config.chargingMatrixGlitchReverse = !Config.chargingMatrixGlitchReverse; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glitchStutterSwitch; KeyNavigation.down: glitchDirectionSwitch
            }
        }
    }

    // 12e. DIRECTION CHANGE
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Direction change"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchDirectionSwitch
                objectName: "glitchDirectionSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchDirection
                trigger: function() { Config.chargingMatrixGlitchDirection = !Config.chargingMatrixGlitchDirection; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glitchReverseSwitch; KeyNavigation.down: glitchDirRateSlider
            }
        }
    }

    // 12f. DIRECTION GLITCH FREQUENCY (visible when direction glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.light
            text: qsTr("Frequency"); font: fonts.primaryFont(24)
        }
        Components.Slider {
            id: glitchDirRateSlider
            objectName: "glitchDirRateSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 80; stepSize: 5
            value: Config.chargingMatrixGlitchDirRate; live: true
            onValueChanged: Config.chargingMatrixGlitchDirRate = value
            onUserInteractionEnded: Config.chargingMatrixGlitchDirRate = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: glitchDirectionSwitch; KeyNavigation.down: glitchDirLengthSlider
        }
    }

    // 12f2. DIRECTION GLITCH LENGTH (visible when direction glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail length"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: glitchDirLengthSlider
            objectName: "glitchDirLengthSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 3; to: 30; stepSize: 1; live: true
            value: Config.chargingMatrixGlitchDirLength
            onValueChanged: Config.chargingMatrixGlitchDirLength = value
            onUserInteractionEnded: Config.chargingMatrixGlitchDirLength = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: glitchDirRateSlider; KeyNavigation.down: glitchDirCardinalRow
        }
    }

    // 12g. DIRECTION TOGGLES — 8 individual direction toggles (visible when direction glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10

        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Glitch directions"); font: fonts.primaryFont(24) }

        // Helper to toggle a bit in the mask with minimum-1 guard
        function toggleDirBit(bit) {
            var newMask = Config.chargingMatrixGlitchDirMask ^ (1 << bit);
            if (newMask > 0) {
                Config.chargingMatrixGlitchDirMask = newMask;
                Haptic.play(Haptic.Click);
            }
        }

        // Cardinal directions (bits 0-3: down, up, left, right)
        RowLayout {
            id: glitchDirCardinalRow
            objectName: "glitchDirCardinalRow"
            spacing: 8; focus: true
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: glitchDirLengthSlider; KeyNavigation.down: glitchDirDiagRow
            Repeater {
                model: [
                    { bit: 0, label: "\u2193 Down" },
                    { bit: 1, label: "\u2191 Up" },
                    { bit: 3, label: "\u2190 Left" },
                    { bit: 2, label: "\u2192 Right" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    property bool checked: (Config.chargingMatrixGlitchDirMask & (1 << modelData.bit)) !== 0
                    color: checked ? colors.offwhite : colors.dark
                    border { color: checked ? colors.offwhite : colors.medium; width: checked ? 3 : 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: parent.checked ? colors.black : colors.offwhite
                        font: fonts.primaryFont(20)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: parent.parent.parent.toggleDirBit(modelData.bit)
                    }
                }
            }
        }

        // Diagonal directions (bits 4-7: down-right, down-left, up-right, up-left)
        RowLayout {
            id: glitchDirDiagRow
            objectName: "glitchDirDiagRow"
            spacing: 8; focus: true
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: glitchDirCardinalRow; KeyNavigation.down: glitchDirFadeSlider
            Repeater {
                model: [
                    { bit: 4, label: "\u2198 D-R" },
                    { bit: 5, label: "\u2199 D-L" },
                    { bit: 6, label: "\u2197 U-R" },
                    { bit: 7, label: "\u2196 U-L" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    property bool checked: (Config.chargingMatrixGlitchDirMask & (1 << modelData.bit)) !== 0
                    color: checked ? colors.offwhite : colors.dark
                    border { color: checked ? colors.offwhite : colors.medium; width: checked ? 3 : 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: parent.checked ? colors.black : colors.offwhite
                        font: fonts.primaryFont(20)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: parent.parent.parent.toggleDirBit(modelData.bit)
                    }
                }
            }
        }
    }

    // 12g2. TRAIL FADE (visible when direction glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail fade"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: glitchDirFadeSlider
            objectName: "glitchDirFadeSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 0; to: 100; stepSize: 5; live: true
            value: Config.chargingMatrixGlitchDirFade
            onValueChanged: Config.chargingMatrixGlitchDirFade = value
            onUserInteractionEnded: Config.chargingMatrixGlitchDirFade = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: glitchDirDiagRow; KeyNavigation.down: glitchDirSpeedSlider
        }
    }

    // 12g3. TRAIL SPEED (visible when direction glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Trail speed"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: glitchDirSpeedSlider
            objectName: "glitchDirSpeedSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 100; stepSize: 5; live: true
            value: Config.chargingMatrixGlitchDirSpeed
            onValueChanged: Config.chargingMatrixGlitchDirSpeed = value
            onUserInteractionEnded: Config.chargingMatrixGlitchDirSpeed = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: glitchDirFadeSlider; KeyNavigation.down: glitchRandomColorSwitch
        }
    }

    // 12g4. RANDOM COLOR (visible when direction glitch is on)
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchDirection
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Random color"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: glitchRandomColorSwitch
                objectName: "glitchRandomColorSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchRandomColor
                trigger: function() { Config.chargingMatrixGlitchRandomColor = !Config.chargingMatrixGlitchRandomColor; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glitchDirSpeedSlider; KeyNavigation.down: glitchChaosSwitch
            }
        }
    }

    // 12h. CHAOS EVENTS
    ColumnLayout {
        visible: Config.chargingMatrixGlitch
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Chaos events"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: glitchChaosSwitch
                objectName: "glitchChaosSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchChaos
                trigger: function() { Config.chargingMatrixGlitchChaos = !Config.chargingMatrixGlitchChaos; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: glitchRandomColorSwitch; KeyNavigation.down: chaosFrequencySlider
            }
        }
    }

    // 12i. CHAOS FREQUENCY
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Frequency"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: chaosFrequencySlider
            objectName: "chaosFrequencySlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 100; stepSize: 5; live: true
            value: Config.chargingMatrixGlitchChaosFrequency
            onValueChanged: Config.chargingMatrixGlitchChaosFrequency = value
            onUserInteractionEnded: Config.chargingMatrixGlitchChaosFrequency = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: glitchChaosSwitch; KeyNavigation.down: chaosIntensitySlider
        }
    }

    // 12i2. CHAOS INTENSITY
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Intensity"); font: fonts.primaryFont(24) }
        Components.Slider {
            id: chaosIntensitySlider
            objectName: "chaosIntensitySlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 100; stepSize: 5; live: true
            value: Config.chargingMatrixGlitchChaosIntensity
            onValueChanged: Config.chargingMatrixGlitchChaosIntensity = value
            onUserInteractionEnded: Config.chargingMatrixGlitchChaosIntensity = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: chaosFrequencySlider; KeyNavigation.down: chaosSurgeSwitch
        }
    }

    // 12j. CHAOS SUB-TYPES
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 50; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Surge (flash)"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosSurgeSwitch
                objectName: "chaosSurgeSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchChaosSurge
                trigger: function() { Config.chargingMatrixGlitchChaosSurge = !Config.chargingMatrixGlitchChaosSurge; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: chaosIntensitySlider; KeyNavigation.down: chaosScrambleSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scramble (mutate)"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosScrambleSwitch
                objectName: "chaosScrambleSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchChaosScramble
                trigger: function() { Config.chargingMatrixGlitchChaosScramble = !Config.chargingMatrixGlitchChaosScramble; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: chaosSurgeSwitch; KeyNavigation.down: chaosFreezeSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Freeze (stutter)"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosFreezeSwitch
                objectName: "chaosFreezeSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchChaosFreeze
                trigger: function() { Config.chargingMatrixGlitchChaosFreeze = !Config.chargingMatrixGlitchChaosFreeze; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: chaosScrambleSwitch; KeyNavigation.down: chaosScatterSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scatter (burst)"); font: fonts.primaryFont(24) }
            Components.Switch {
                id: chaosScatterSwitch
                objectName: "chaosScatterSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixGlitchChaosScatter
                trigger: function() { Config.chargingMatrixGlitchChaosScatter = !Config.chargingMatrixGlitchChaosScatter; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: chaosFreezeSwitch; KeyNavigation.down: chaosScatterRateSlider
            }
        }
    }

    // 12k. SCATTER FREQUENCY
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos && Config.chargingMatrixGlitchChaosScatter
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 70; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scatter frequency"); font: fonts.primaryFont(22) }
        Components.Slider {
            id: chaosScatterRateSlider
            objectName: "chaosScatterRateSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 100; stepSize: 5; live: true
            value: Config.chargingMatrixGlitchChaosScatterRate
            onValueChanged: Config.chargingMatrixGlitchChaosScatterRate = value
            onUserInteractionEnded: Config.chargingMatrixGlitchChaosScatterRate = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: chaosScatterSwitch; KeyNavigation.down: chaosScatterLengthSlider
        }
    }

    // 12l. SCATTER TRAIL LENGTH
    ColumnLayout {
        visible: Config.chargingMatrixGlitch && Config.chargingMatrixGlitchChaos && Config.chargingMatrixGlitchChaosScatter
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 70; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scatter trail length"); font: fonts.primaryFont(22) }
        Components.Slider {
            id: chaosScatterLengthSlider
            objectName: "chaosScatterLengthSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 3; to: 40; stepSize: 1; live: true
            value: Config.chargingMatrixGlitchChaosScatterLength
            onValueChanged: Config.chargingMatrixGlitchChaosScatterLength = value
            onUserInteractionEnded: Config.chargingMatrixGlitchChaosScatterLength = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: chaosScatterRateSlider; KeyNavigation.down: tapBurstSwitch
        }
    }

    // ─────────────────────────────────────────
    // TAP EFFECTS (corruption burst)
    // ─────────────────────────────────────────

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 12m. TAP EFFECTS
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Tap effects"); font: fonts.primaryFont(30)
        }

        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Scatter burst"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapBurstSwitch
                objectName: "tapBurstSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapBurst
                trigger: function() { Config.chargingMatrixTapBurst = !Config.chargingMatrixTapBurst; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: chaosScatterLengthSlider; KeyNavigation.down: tapFlashSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Flash shockwave"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapFlashSwitch
                objectName: "tapFlashSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapFlash
                trigger: function() { Config.chargingMatrixTapFlash = !Config.chargingMatrixTapFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: tapBurstSwitch; KeyNavigation.down: tapScrambleSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Character scramble"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapScrambleSwitch
                objectName: "tapScrambleSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapScramble
                trigger: function() { Config.chargingMatrixTapScramble = !Config.chargingMatrixTapScramble; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: tapFlashSwitch; KeyNavigation.down: tapSpawnSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Stream spawn"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapSpawnSwitch
                objectName: "tapSpawnSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapSpawn
                trigger: function() { Config.chargingMatrixTapSpawn = !Config.chargingMatrixTapSpawn; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: tapScrambleSwitch; KeyNavigation.down: tapMessageSwitch
            }
        }
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Show message"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapMessageSwitch
                objectName: "tapMessageSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapMessage
                trigger: function() { Config.chargingMatrixTapMessage = !Config.chargingMatrixTapMessage; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: tapSpawnSwitch; KeyNavigation.down: tapRandomizeSwitch
            }
        }
    }

    // 12n. TAP RANDOMIZE
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Randomize"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: tapRandomizeSwitch
                objectName: "tapRandomizeSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixTapRandomize
                trigger: function() { Config.chargingMatrixTapRandomize = !Config.chargingMatrixTapRandomize; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: tapMessageSwitch
                KeyNavigation.down: Config.chargingMatrixTapRandomize ? tapRandomizeChanceSlider : subliminalSwitch
            }
        }
    }

    // 12o. TAP RANDOMIZE CHANCE
    ColumnLayout {
        visible: Config.chargingMatrixTapRandomize
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 70; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Chance") + " (" + Config.chargingMatrixTapRandomizeChance + "%)"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: tapRandomizeChanceSlider
            objectName: "tapRandomizeChanceSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 10; to: 90; stepSize: 5; live: true
            value: Config.chargingMatrixTapRandomizeChance
            onValueChanged: Config.chargingMatrixTapRandomizeChance = value
            onUserInteractionEnded: Config.chargingMatrixTapRandomizeChance = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: tapRandomizeSwitch; KeyNavigation.down: subliminalSwitch
        }
    }

    // ─────────────────────────────────────────
    // SUBLIMINAL MESSAGES
    // ─────────────────────────────────────────

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium; visible: Config.chargingMatrixMessages !== "" }

    // 12p. SUBLIMINAL MASTER TOGGLE
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== ""
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.offwhite; text: qsTr("Subliminal messages"); font: fonts.primaryFont(30) }
            Components.Switch {
                id: subliminalSwitch
                objectName: "subliminalSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixSubliminal
                trigger: function() { Config.chargingMatrixSubliminal = !Config.chargingMatrixSubliminal; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: Config.chargingMatrixTapRandomize ? tapRandomizeChanceSlider : tapRandomizeSwitch
                KeyNavigation.down: Config.chargingMatrixSubliminal ? subliminalStreamSwitch : messagesInput
            }
        }
    }

    // 12q. IN-STREAM MODE
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("In-stream injection"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: subliminalStreamSwitch
                objectName: "subliminalStreamSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixSubliminalStream
                trigger: function() { Config.chargingMatrixSubliminalStream = !Config.chargingMatrixSubliminalStream; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: subliminalSwitch; KeyNavigation.down: subliminalOverlaySwitch
            }
        }
    }

    // 12r. OVERLAY SPANNING MODE
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Overlay spanning"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: subliminalOverlaySwitch
                objectName: "subliminalOverlaySwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixSubliminalOverlay
                trigger: function() { Config.chargingMatrixSubliminalOverlay = !Config.chargingMatrixSubliminalOverlay; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: subliminalStreamSwitch; KeyNavigation.down: subliminalFlashSwitch
            }
        }
    }

    // 12s. FLASH MODE
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Flash mode"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: subliminalFlashSwitch
                objectName: "subliminalFlashSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixSubliminalFlash
                trigger: function() { Config.chargingMatrixSubliminalFlash = !Config.chargingMatrixSubliminalFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: subliminalOverlaySwitch; KeyNavigation.down: subliminalIntervalSlider
            }
        }
    }

    // 12t. SUBLIMINAL INTERVAL
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Interval") + " (" + Config.chargingMatrixSubliminalInterval + "s)"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: subliminalIntervalSlider
            objectName: "subliminalIntervalSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 1; to: 30; stepSize: 1; live: true
            value: Config.chargingMatrixSubliminalInterval
            onValueChanged: Config.chargingMatrixSubliminalInterval = value
            onUserInteractionEnded: Config.chargingMatrixSubliminalInterval = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: subliminalFlashSwitch; KeyNavigation.down: subliminalDurationSlider
        }
    }

    // 12u. SUBLIMINAL DURATION
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Duration") + " (" + Config.chargingMatrixSubliminalDuration + ")"; font: fonts.primaryFont(22) }
        Components.Slider {
            id: subliminalDurationSlider
            objectName: "subliminalDurationSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 2; to: 40; stepSize: 1; live: true
            value: Config.chargingMatrixSubliminalDuration
            onValueChanged: Config.chargingMatrixSubliminalDuration = value
            onUserInteractionEnded: Config.chargingMatrixSubliminalDuration = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: subliminalIntervalSlider; KeyNavigation.down: messagesInput
        }
    }

    // ─────────────────────────────────────────
    // HIDDEN MESSAGES
    // ─────────────────────────────────────────

    Rectangle { Layout.alignment: Qt.AlignCenter; width: parent.width - 20; height: 2; color: colors.medium }

    // 13. MESSAGES TEXT
    ColumnLayout {
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Hidden messages"); font: fonts.primaryFont(30)
        }

        Components.InputField {
            id: messagesInput
            objectName: "messagesInput"
            Layout.fillWidth: true
            inputField.text: Config.chargingMatrixMessages
            inputField.placeholderText: "HELLO, WORLD, WAKE UP"
            inputField.onTextChanged: Config.chargingMatrixMessages = inputField.text
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: (Config.chargingMatrixMessages !== "" && Config.chargingMatrixSubliminal) ? subliminalDurationSlider : subliminalSwitch
            KeyNavigation.down: messageIntervalSlider
        }

    }

    // 13b. MESSAGE INTERVAL
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== ""
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Message interval") + " (" + Config.chargingMatrixMessageInterval + "s)"
            font: fonts.primaryFont(30)
        }
        Components.Slider {
            id: messageIntervalSlider
            objectName: "messageIntervalSlider"
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            height: 60; Layout.fillWidth: true
            from: 5; to: 60; stepSize: 5
            value: Config.chargingMatrixMessageInterval; live: true
            onValueChanged: Config.chargingMatrixMessageInterval = value
            onUserInteractionEnded: Config.chargingMatrixMessageInterval = value
            highlight: activeFocus && ui.keyNavigationEnabled
            KeyNavigation.up: messagesInput; KeyNavigation.down: messageRandomSwitch
        }
    }

    // 13c. RANDOM ORDER
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== ""
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text {
                Layout.fillWidth: true; color: colors.offwhite
                text: qsTr("Random order"); font: fonts.primaryFont(30)
            }
            Components.Switch {
                id: messageRandomSwitch
                objectName: "messageRandomSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixMessageRandom
                trigger: function() { Config.chargingMatrixMessageRandom = !Config.chargingMatrixMessageRandom; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: messageIntervalSlider; KeyNavigation.down: messageDirRow
            }
        }
    }

    // 13d. MESSAGE DIRECTION
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== ""
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 10; Layout.rightMargin: 10
        spacing: 10

        Text {
            Layout.fillWidth: true; color: colors.offwhite
            text: qsTr("Message direction"); font: fonts.primaryFont(30)
        }

        RowLayout {
            id: messageDirRow
            objectName: "messageDirRow"
            spacing: 8; focus: true
            onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
            KeyNavigation.up: messageRandomSwitch; KeyNavigation.down: messageFlashSwitch
            Keys.onLeftPressed: root.settingsPage.cycleOption(["horizontal-lr","horizontal-rl","vertical-tb","vertical-bt","stream"], Config.chargingMatrixMessageDirection, function(v){ Config.chargingMatrixMessageDirection = v }, -1)
            Keys.onRightPressed: root.settingsPage.cycleOption(["horizontal-lr","horizontal-rl","vertical-tb","vertical-bt","stream"], Config.chargingMatrixMessageDirection, function(v){ Config.chargingMatrixMessageDirection = v }, 1)
            Repeater {
                model: [
                    { name: "horizontal-lr", label: "H \u2192" },
                    { name: "horizontal-rl", label: "\u2190 H" },
                    { name: "vertical-tb",   label: "V \u2193" },
                    { name: "vertical-bt",   label: "\u2191 V" },
                    { name: "stream",        label: "Rain" }
                ]
                Rectangle {
                    Layout.fillWidth: true; height: 46; radius: 8
                    color: Config.chargingMatrixMessageDirection === modelData.name ? colors.offwhite : colors.dark
                    border { color: colors.medium; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: modelData.label
                        color: Config.chargingMatrixMessageDirection === modelData.name ? colors.black : colors.offwhite
                        font: fonts.primaryFont(22)
                    }
                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: Config.chargingMatrixMessageDirection = modelData.name
                    }
                }
            }
        }
    }

    // 13e. SURROUNDING FLASH (visible when messages set)
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== ""
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Surrounding flash"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: messageFlashSwitch
                objectName: "messageFlashSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixMessageFlash
                trigger: function() { Config.chargingMatrixMessageFlash = !Config.chargingMatrixMessageFlash; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: messageDirRow; KeyNavigation.down: messagePulseSwitch
            }
        }
    }

    // 13f. BRIGHTNESS PULSE (visible when messages set)
    ColumnLayout {
        visible: Config.chargingMatrixMessages !== ""
        Layout.alignment: Qt.AlignCenter
        Layout.leftMargin: 30; Layout.rightMargin: 10
        spacing: 10
        RowLayout {
            spacing: 10
            Text { Layout.fillWidth: true; color: colors.light; text: qsTr("Brightness pulse"); font: fonts.primaryFont(26) }
            Components.Switch {
                id: messagePulseSwitch
                objectName: "messagePulseSwitch"
                icon: "uc:check"
                onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
                checked: Config.chargingMatrixMessagePulse
                trigger: function() { Config.chargingMatrixMessagePulse = !Config.chargingMatrixMessagePulse; }
                highlight: activeFocus && ui.keyNavigationEnabled
                KeyNavigation.up: messageFlashSwitch
                KeyNavigation.down: root.navDownTarget
            }
        }
    }
}
