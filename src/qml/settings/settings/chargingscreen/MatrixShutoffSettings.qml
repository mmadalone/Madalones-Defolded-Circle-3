// Copyright (c) 2026 madalone. Matrix shutdown animation settings panel.
// Conditionally shown in ChargingScreen settings page when theme === "matrix".
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components
import ScreensaverConfig 1.0

ColumnLayout {
    id: root

    required property Item settingsPage

    property Item navUpTarget
    property Item navDownTarget

    property alias firstFocusItem: durationSlider
    property alias lastFocusItem: durationSlider

    Layout.fillWidth: true
    Layout.leftMargin: 10; Layout.rightMargin: 10
    spacing: 20

    // ---- Section header ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Shutdown animation")
        font: fonts.primaryFont(30)
    }
    Text {
        Layout.fillWidth: true; color: colors.medium
        text: qsTr("A cascade sweep clears the rain before the display turns off")
        font: fonts.primaryFont(20)
        wrapMode: Text.WordWrap
    }

    // ---- Duration slider: 800-2000 ms ----
    Text {
        Layout.fillWidth: true; color: colors.offwhite
        text: qsTr("Duration")
        font: fonts.primaryFont(30)
    }
    Text {
        Layout.fillWidth: true; color: colors.medium
        text: qsTr("Total animation length in milliseconds (%1 ms)").arg(ScreensaverConfig.matrixShutoffDuration)
        font: fonts.primaryFont(20)
    }
    Components.Slider {
        id: durationSlider
        height: 60; Layout.fillWidth: true
        from: 800; to: 2000; stepSize: 100
        value: ScreensaverConfig.matrixShutoffDuration; live: true
        onMoved: ScreensaverConfig.matrixShutoffDuration = value
        onUserInteractionEnded: ScreensaverConfig.matrixShutoffDuration = value
        highlight: activeFocus && ui.keyNavigationEnabled
        onActiveFocusChanged: if (activeFocus) root.settingsPage.ensureVisible(this)
        KeyNavigation.up: root.navUpTarget
        KeyNavigation.down: root.navDownTarget
    }
}
