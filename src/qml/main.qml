// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.VirtualKeyboard 2.3
import QtQuick.VirtualKeyboard.Settings 2.3
import QtQuick.Window 2.2

import Haptic 1.0
import HwInfo 1.0
import Config 1.0
import Battery 1.0
import Power 1.0
import Power.Modes 1.0
import SoundEffects 1.0
import SoftwareUpdate 1.0
import Entity.Controller 1.0
import Integration.Controller 1.0
import TouchSlider 1.0

import "qrc:/components" as Components
import "qrc:/settings/softwareupdate" as Softwareupdate
import "qrc:/components/entities/activity" as ActivityComponents
import ScreensaverConfig 1.0

ApplicationWindow {
    id: applicationWindow
    objectName : "applicationWindow"
    title: "Remote Two simulator"
    visible: true

    // Global flag: true when the screensaver is covering the UI.
    // Used by TouchSlider components to suppress volume/seek during screensaver.
    property bool screensaverActive: chargingScreenLoader.active
    // DEV-only dock fake: F12 → InputController::eventFilter toggles Battery.powerSupply
    // (handled in C++ so it works even under modal focus capture).

    minimumWidth: ui.width * ui.ratio
    maximumWidth: minimumWidth
    minimumHeight: ui.height * ui.ratio
    maximumHeight: minimumHeight
    color: colors.black

    Window {
        id: buttonSimulator
        visible: !ui.showRegulatoryInfo
        title: "Button simulator"
        color: colors.black
        minimumWidth: ui.width * ui.ratio
        maximumWidth: minimumWidth
        minimumHeight: ui.width * ui.ratio * 0.95
        maximumHeight: minimumHeight
        x: applicationWindow.height + buttonSimulator.height > Screen.desktopAvailableHeight ? applicationWindow.x+applicationWindow.width : applicationWindow.x
        y: applicationWindow.height + buttonSimulator.height > Screen.desktopAvailableHeight ? applicationWindow.y : applicationWindow.y + applicationWindow.height + 60

        Loader {
            anchors.fill: parent
            source: "qrc:/button-simulator/Buttons.qml"
            active: buttonSimulator.visible
        }
    }

    function loadSecondContainer(source, parameters = {}, openAfterLoad = true) {
        if (!containerSecond.loader.active) {
            console.debug("Loading second container", source);
            containerSecond.loader.openAfterLoad = openAfterLoad;
            containerSecond.loader.active = true;
            containerSecond.loader.setSource(source, parameters);
        }
    }

    function loadActivityToSecondContainer(entityObj) {
        ui.setTimeOut(1000, () => {
                          loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityObj.id, "entityObj": entityObj });
                      });
    }

    property bool isSecondContainerLoaded: containerSecond.loader.source != ""

    function loadThirdContainer(source, parameters = {}, openAfterLoad = true) {
        if (!containerThird.loaderThird.active) {
            console.debug("Loading third container", source);
            containerThird.loaderThird.openAfterLoad = openAfterLoad;
            containerThird.loaderThird.active = true;
            containerThird.loaderThird.setSource(source, parameters);
        }
    }

    function connectSignalSlot(sig, slot) {
        let slotConn = (...args) => {
            slot(...args);
            sig.disconnect(slotConn);
        }
        sig.connect(slotConn)
    }

    function checkActivityIncludedEntities(activityObj, onSequence = true) {
        // check if all entities in the activity has a connected integraiton
        let entityListToCheck = onSequence ? activityObj.onSequenceEntities : activityObj.offSequenceEntities;
        let allIncludedEntitiesConnected = true;
        let notReadyEntities = "";
        let notReadyEntityQty = 0;

        if (entityListToCheck.length === 0) {
            return {
                allIncludedEntitiesConnected: true,
                notReadyEntities: notReadyEntities,
                notReadyEntityQty: notReadyEntityQty
            }
        }

        for (let i = 0; i < entityListToCheck.length; i++) {
            const includedEntityObj = EntityController.get(entityListToCheck[i]);

            if (includedEntityObj) {
                const includedEntityIntegrationObj = IntegrationController.getModelItem(includedEntityObj.integrationId);
                if (includedEntityIntegrationObj) {
                    if (includedEntityIntegrationObj.state !== "connected") {
                        allIncludedEntitiesConnected = false;
                        notReadyEntities += includedEntityObj.name + ",  ";
                        notReadyEntityQty++;
                    } else if (includedEntityIntegrationObj.state === "connected") {
                        if (!includedEntityObj.enabled) {
                            allIncludedEntitiesConnected = false;
                            notReadyEntities += includedEntityObj.name + ",  ";
                            notReadyEntityQty++;
                        }
                    }
                }
            }
        }

        // chop the last comma
        notReadyEntities = notReadyEntities.slice(0, -3);

        return {
            allIncludedEntitiesConnected: allIncludedEntitiesConnected,
            notReadyEntities: notReadyEntities,
            notReadyEntityQty: notReadyEntityQty
        }
    }

    Connections {
        target: Power
        ignoreUnknownSignals: true

        function onPowerModeChanged(fromPowerMode, toPowerMode) {
            if (toPowerMode == PowerModes.Low_power && fromPowerMode == PowerModes.Idle) {
                applicationWindow.visible = false;
            }

            if (toPowerMode == PowerModes.Normal) {
                if (!applicationWindow.visible) {
                    applicationWindow.visible = true;
                }
                // Reset idle screensaver timer on ANY activity (power mode to Normal = user interacted)
                if (idleScreensaverTimer.running) {
                    idleScreensaverTimer.restart();
                }
            }
        }
    }

    Components.ButtonNavigation {
        overrideActive: true
        defaultConfig: {
            "POWER": {
                "pressed": function() {
                    if (!SoftwareUpdate.updateInProgress) {
                        powerOffButtonTimer.start();
                    }
                },
                "released": function() {
                    if (!SoftwareUpdate.updateInProgress) {
                        powerOffButtonTimer.stop();
                    }
                }
            },
            "VOICE": {
                "long_press": function() {
                    if (!isSecondContainerLoaded || (isSecondContainerLoaded && !root.isActivityOpen)) {
                        voice.start(Config.voiceAssistantId, Config.voiceAssistantProfileId);
                    }
                },
                "released": function() {
                    if (!isSecondContainerLoaded || (isSecondContainerLoaded && !root.isActivityOpen)) {
                        voice.stop();
                    }
                }
            }
        }
    }

    Timer {
        id: powerOffButtonTimer
        running: false
        repeat: false
        interval: 3000
        onTriggered: {
            powerOffLoader.active = true;
        }
    }


    Item {
        id: root
        objectName: "root"
        width: ui.width
        height: ui.height
        layer.enabled: true

        anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
        transformOrigin: Item.Center

        Behavior on anchors.verticalCenterOffset {
            PropertyAnimation { duration: 300; easing.type: Easing.OutExpo }
        }

        property alias containerMain: containerMain
        property alias containerMainItem: containerMain.item
        property alias activityLoading: activityLoading
        property alias loading: loading
        property alias volume: volume
        property alias keyboard: keyboard
        property alias keyboardInputField: keyboardInputField
        property bool isActivityOpen: false

        Loader {
            id: containerMain
            anchors.fill: parent
            asynchronous: true
        }

        Connections {
            target: ui
            ignoreUnknownSignals: true

            function onConfigLoaded() {
                if (!ui.isOnboarding) {
                    if (ui.pages.count === 0) {
                        containerMain.source = "qrc:/NoPage.qml";
                    } else {
                        containerMain.source = "qrc:/MainContainer.qml";
                    }
                }
            }

            function onIsOnboardingChanged() {
                if (!ui.isOnboarding) {
                    if (ui.pages.count === 0) {
                        containerMain.source = "qrc:/NoPage.qml";
                    } else {
                        containerMain.source = "qrc:/MainContainer.qml";
                    }
                } else {
                    containerMain.source = "qrc:/OnboardingContainer.qml";
                }
            }

            function onIsNoProfileChanged() {
                if (ui.isNoProfile && !ui.isOnboarding) {
                    loadingFirst.stop();

                    containerSecond.close();
                    containerThird.close();

                    if (ui.profiles.count === 0) {
                        containerMain.setSource("qrc:/components/ProfileAdd.qml", { state: "visible", noProfile: true })
                    } else {
                        containerMain.setSource("qrc:/components/ProfileSwitch.qml", { state: "visible", noProfile: true })
                    }
                }  else if (!ui.isNoProfile && !ui.isOnboarding) {
                    if (ui.pages.count === 0) {
                        containerMain.source = "qrc:/NoPage.qml";
                    } else {
                        containerMain.source = "qrc:/MainContainer.qml";
                    }
                }
            }
        }

        Connections {
            target: ui.pages
            ignoreUnknownSignals: true

            function onCountChanged() {
                if (!ui.isOnboarding) {
                    if (ui.pages.count === 0) {
                        containerMain.source = "qrc:/NoPage.qml";
                    } else {
                        containerMain.source = "qrc:/MainContainer.qml";
                    }
                }
            }
        }

        Popup {
            id: containerSecond
            objectName: "containerSecond"
            width: parent.width; height: parent.height
            modal: false
            closePolicy: Popup.NoAutoClose
            padding: 0

            property alias loader: loader

            Behavior on y {
                PropertyAnimation { duration: 300; easing.type: Easing.OutExpo }
            }

            background: Item {}

            SequentialAnimation {
                id: containerSecondHideAnimation
                running: false
                alwaysRunToEnd: true

                ParallelAnimation {
                    NumberAnimation { target: containerSecond; properties: "scale"; from: 1.0; to: 0.7; easing.type: Easing.OutExpo; duration: 300 }
                    NumberAnimation { target: containerSecond; properties: "x"; from: 0; to: -ui.width; easing.type: Easing.InExpo; duration: 300 }
                }
            }

            SequentialAnimation {
                id: containerSecondShowAnimation
                running: false
                alwaysRunToEnd: true

                PauseAnimation { duration: 200 }
                ParallelAnimation {
                    NumberAnimation { target: containerSecond; properties: "scale"; from: 0.7; to: 1.0; easing.type: Easing.InExpo; duration: 300 }
                    NumberAnimation { target: containerSecond; properties: "x"; from: -ui.width; to: 0; easing.type: Easing.OutExpo; duration: 300 }
                }
            }

            Loader {
                id: loader
                anchors.fill: parent
                asynchronous: true
                active: false

                property bool openAfterLoad: false

                onActiveChanged: {
                    if (active) {
                        containerSecond.open();
                    }
                }

                onStatusChanged: {
                    if (status == Loader.Ready && loader.openAfterLoad) {
                        loader.item.open();
                    }
                }
            }

            Connections {
                target: loader.item
                ignoreUnknownSignals: true

                function onClosed() {
                    console.debug("Second container closed signal called");
                    loader.source = "";
                    loader.active = false
                    containerSecond.close();
                    root.isActivityOpen = false;
                }
            }
        }

        Popup {
            id: containerThird
            objectName: "containerThird"
            width: parent.width; height: parent.height
            modal: false
            closePolicy: Popup.NoAutoClose
            padding: 0
            x: ui.width
            scale: 0.7

            property alias loaderThird: loaderThird

            Behavior on y {
                PropertyAnimation { duration: 300; easing.type: Easing.OutExpo }
            }

            background: Item {}

            enter: Transition {
                SequentialAnimation {
                    PauseAnimation { duration: 200 }
                    ParallelAnimation {
                        NumberAnimation { properties: "scale"; from: 0.7; to: 1.0; easing.type: Easing.InExpo; duration: 300 }
                        NumberAnimation { properties: "x"; from: ui.width; to: 0; easing.type: Easing.OutExpo; duration: 300 }
                    }
                }
            }

            exit: Transition {
                SequentialAnimation {
                    ParallelAnimation {
                        NumberAnimation { properties: "scale"; from: 1.0; to: 0.7; easing.type: Easing.OutExpo; duration: 300 }
                        NumberAnimation { properties: "x"; from: 0; to: ui.width; easing.type: Easing.InExpo; duration: 300 }
                    }
                    PropertyAction { target: loaderThird; property: "source"; value: "" }
                    PropertyAction { target: loaderThird; property: "active"; value: false }
                }
            }

            Loader {
                id: loaderThird
                anchors.fill: parent
                asynchronous: true
                active: false

                property bool openAfterLoad: false

                onActiveChanged: {
                    if (active) {
                        containerThird.open();
                    }
                }

                onStatusChanged: {
                    if (status == Loader.Ready && loaderThird.openAfterLoad) {
                        loaderThird.item.open(true);
                        containerSecondHideAnimation.start();
                    }
                }
            }

            Connections {
                target: loaderThird.item
                ignoreUnknownSignals: true

                function onClosed() {
                    console.debug("Third container closed signal called");
                    containerThird.close();
                    containerSecondShowAnimation.start();
                }
            }
        }

        NoProfile {
            visible: ui.profile.id === "" && !ui.isOnboarding
        }

        ActivityComponents.LoadingScreen {
            id: activityLoading
        }

        Components.LoadingScreen {
            id: loading
            objectName: "loading"
        }

        Components.LoadingFirst {
            id: loadingFirst
        }

        Components.VolumeOverlay {
            id: volume
            anchors.centerIn: parent
        }

        Components.VoiceOverlay {
            id: voice
            anchors.centerIn: parent
        }

        // Idle screensaver: touch-anywhere detection resets the idle timer
        MouseArea {
            anchors.fill: parent
            z: 9999
            enabled: idleScreensaverTimer.running && !chargingScreenLoader.active
            propagateComposedEvents: true
            onPressed: {
                idleScreensaverTimer.restart();
                mouse.accepted = false;
            }
            onReleased: { mouse.accepted = false; }
            onClicked: { mouse.accepted = false; }
        }

        // Hardware button + touch detection resets idle screensaver timer
        // (keyPressed fires for ALL 24 buttons including DPAD, independent of ButtonNavigation stack)
        // (touchDetected fires from C++ event filter — reliable over Flickables unlike QML MouseArea)
        Connections {
            target: ui.inputController
            function onKeyPressed() {
                if (idleScreensaverTimer.running) {
                    idleScreensaverTimer.restart();
                }
            }
            function onTouchDetected() {
                if (idleScreensaverTimer.running) {
                    idleScreensaverTimer.restart();
                }
            }
        }

        // Touchbar (physical slider) resets idle timer.
        // TouchSliderProcessor bypasses Qt's event system — its signals don't reach
        // InputController.touchDetected(). Direct connection ensures slider interaction
        // prevents screensaver from activating during use.
        Connections {
            target: TouchSliderProcessor
            ignoreUnknownSignals: true
            function onTouchPressed() {
                if (idleScreensaverTimer.running) {
                    idleScreensaverTimer.restart();
                }
            }
        }

        // One-shot: set true when the next chargingScreenLoader.active = true
        // is triggered by idleScreensaverTimer firing (legacy idle-open path).
        // Consumed by chargingScreenLoader.onStatusChanged when the popup
        // becomes Ready, propagated to the popup's _openedViaIdleTimer flag
        // to drive the screen-off countdown's subtraction rule.
        property bool _nextOpenViaIdleTimer: false

        // Helper: whether the screensaver should auto-open on battery idle.
        // Strictly gated by the 'Idle screensaver' toggle — when off, no
        // auto-open regardless of other settings. The screen-off animation
        // system is a separate feature that applies when the screensaver is
        // already open; it does not trigger auto-open on its own.
        function _shouldOpenOnIdle() {
            return ScreensaverConfig.idleEnabled;
        }

        // Idle screensaver timer — activates screensaver after N seconds of inactivity when undocked
        Timer {
            id: idleScreensaverTimer
            repeat: false
            running: false
            interval: ScreensaverConfig.idleTimeout * 1000
            onTriggered: {
                var undocked = !Battery.powerSupply || HwInfo.modelNumber === "DEV";
                if (root._shouldOpenOnIdle() && undocked && !chargingScreenLoader.active) {
                    root._nextOpenViaIdleTimer = true;
                    chargingScreenLoader.active = true;
                }
            }
        }

        // madalone (v1.4.15): rearm screensaver after tap-dismiss while docked.
        // Pre-existing UC bug — onClosed handler at the bottom of this Loader skipped
        // idleScreensaverTimer.restart() when docked, so screensaver stayed dismissed
        // until the user woke the screen from Low_power. This timer fires on tap-dismiss
        // while docked and re-activates chargingScreenLoader after a configurable delay.
        Timer {
            id: dockedRearmTimer
            repeat: false
            running: false
            interval: ScreensaverConfig.reopenWhileDockedSec * 1000
            onTriggered: {
                if (Battery.powerSupply && !chargingScreenLoader.active && !ui.editMode) {
                    chargingScreenLoader.active = true;
                }
            }
        }

        Connections {
            target: ScreensaverConfig
            ignoreUnknownSignals: true

            function onIdleEnabledChanged() {
                root._refreshIdleTimer();
            }

            function onIdleTimeoutChanged() {
                idleScreensaverTimer.interval = ScreensaverConfig.idleTimeout * 1000;
                if (idleScreensaverTimer.running) idleScreensaverTimer.restart();
            }

            // madalone (v1.4.15): keep dockedRearmTimer's interval in sync with the slider.
            function onReopenWhileDockedSecChanged() {
                dockedRearmTimer.interval = ScreensaverConfig.reopenWhileDockedSec * 1000;
                if (dockedRearmTimer.running) dockedRearmTimer.restart();
            }
        }

        function _refreshIdleTimer() {
            var undocked = !Battery.powerSupply || HwInfo.modelNumber === "DEV";
            if (root._shouldOpenOnIdle() && undocked) {
                idleScreensaverTimer.restart();
            } else {
                idleScreensaverTimer.stop();
            }
        }

        Loader {
            id: chargingScreenLoader
            anchors.fill: parent
            asynchronous: true
            active: HwInfo.modelNumber === "DEV"  // Auto-open screensaver in DEV mode
            source: "qrc:/components/ChargingScreen.qml"

            onStatusChanged: {
                if (status == Loader.Ready) {
                    // Propagate "open reason" to the popup so the screen-off
                    // countdown knows whether to subtract idleTimeout. AND
                    // with Battery state to cover the race where the user
                    // docks while the async loader is still loading.
                    item._openedViaIdleTimer = root._nextOpenViaIdleTimer && !Battery.powerSupply;
                    root._nextOpenViaIdleTimer = false;
                    item.open();
                }
            }

            Connections {
                target: Battery
                ignoreUnknownSignals: true

                function onPowerSupplyChanged(value) {
                    if (value) {
                        idleScreensaverTimer.stop();
                        chargingScreenLoader.active = true;
                        SoundEffects.play(SoundEffects.BatteryCharge);
                    } else {
                        dockedRearmTimer.stop();   // madalone (v1.4.15): undocked → no docked rearm
                        // Honor 'Close on wake' toggle — keeps undock consistent with motion wake.
                        if (ScreensaverConfig.motionToClose && chargingScreenLoader.active && chargingScreenLoader.item) {
                            chargingScreenLoader.item.close();
                        }
                        if (root._shouldOpenOnIdle()) {
                            idleScreensaverTimer.restart();
                        }
                    }
                }
            }

            Connections {
                target: Power
                ignoreUnknownSignals: true

                function onPowerModeChanged(fromPowerMode, toPowerMode) {
                    // Pause screensaver rendering when display is actually off (Low_power/Standby).
                    // Idle = dimmed but still visible — screensaver should keep animating.
                    if (toPowerMode === PowerModes.Low_power
                            && chargingScreenLoader.active && chargingScreenLoader.item) {
                        chargingScreenLoader.item.displayOff = true;
                    }

                    if (toPowerMode === PowerModes.Normal) {
                        // Resume screensaver rendering
                        if (chargingScreenLoader.active && chargingScreenLoader.item) {
                            chargingScreenLoader.item.displayOff = false;
                        }
                        // Motion/pickup detected while screensaver is showing — close it if enabled
                        if (ScreensaverConfig.motionToClose && chargingScreenLoader.active && chargingScreenLoader.item) {
                            chargingScreenLoader.item.close();
                        }
                        // Re-open screensaver when waking from suspend while charging (if not motion-closed)
                        else if (fromPowerMode !== PowerModes.Idle && Battery.isCharging && Battery.powerSupply) {
                            chargingScreenLoader.active = true;
                            dockedRearmTimer.stop();   // madalone (v1.4.15): wake re-opened it; cancel pending rearm
                        }
                        // Reset idle timer on activity
                        if (root._shouldOpenOnIdle() && !Battery.powerSupply) {
                            idleScreensaverTimer.restart();
                        }
                    }
                }
            }

            Connections {
                target: chargingScreenLoader.item ? chargingScreenLoader.item : null
                ignoreUnknownSignals: true

                function onClosed() {
                    chargingScreenLoader.active = false;
                    // Restart idle timer after user dismisses screensaver (if enabled and undocked)
                    var undocked = !Battery.powerSupply || HwInfo.modelNumber === "DEV";
                    if (root._shouldOpenOnIdle() && undocked) {
                        idleScreensaverTimer.restart();
                    }
                    // madalone (v1.4.15): rearm screensaver while docked after configurable timeout.
                    // NOT gated on _shouldOpenOnIdle() — the docked-rearm slider is always-visible
                    // and operates independently of the idleEnabled toggle (per user feedback v1.4.15a).
                    if (Battery.powerSupply && HwInfo.modelNumber !== "DEV") {
                        dockedRearmTimer.interval = ScreensaverConfig.reopenWhileDockedSec * 1000;
                        dockedRearmTimer.restart();
                    }
                }
            }
        }

        Loader {
            id: powerOffLoader
            anchors.fill: parent
            asynchronous: true
            active: false
            source: "qrc:/components/Poweroff.qml"

            onStatusChanged: {
                if (status == Loader.Ready) {
                    powerOffLoader.item.open();
                }
            }

            Connections {
                target: powerOffLoader.item
                ignoreUnknownSignals: true

                function onClosed() {
                    powerOffLoader.active = false;
                }
            }
        }

        Softwareupdate.UpdateProgress {
            id: updateProgress

            Connections {
                target: SoftwareUpdate
                ignoreUnknownSignals: true

                function onUpdateStarted() {
                    updateProgress.open();
                }
            }
        }

        Components.Notification {}
        Components.ActionableNotification {}

        Loader {
            id: remoteOpenLoader
            objectName: "remoteOpenLoader"
            anchors.fill: parent
            asynchronous: true
            active: false
            source: "qrc:/components/RemoteOpen.qml"

            onStatusChanged: {
                if (status == Loader.Ready) {
                    remoteOpenLoader.item.open();
                }
            }

            onActiveChanged: {
                if (active) {
                    keyboard.hide();
                }
            }
        }

        Rectangle {
            parent:keyboard
            width: keyboard.width
            height: keyboard.height
            color: colors.black
            opacity: ui.globalBrightness
            z: 5000
        }

        Rectangle {
            id: keyboardInputField
            parent: Overlay.overlay
            width: parent.width
            height: parent.height - keyboard.height + 10
            color: colors.black
            state: "hidden"
            z: 10000
            anchors.top: parent.top

            property QtObject originObj
            property alias keyboardInput: keyboardInput

            function show(obj, label = "") {
                if (keyboardInputField.state === "hidden") {
                    console.debug("Show input", obj, label);
                    keyboardInputField.originObj = obj;

                    keyboardInput.inputValue = obj.inputValue;
                    keyboardInput.inputField.placeholderText = obj.inputField.placeholderText;
                    keyboardInput.inputField.inputMethodHints = obj.inputField.inputMethodHints;
                    keyboardInput.errorMsg = obj.errorMsg;
                    keyboardInput.password = obj.password;

                    if (keyboardInput.password) {
                        keyboardInput.inputField.passwordMaskDelay = obj.inputField.passwordMaskDelay;

                    }

                    if (label !== "") {
                        keyboardInputLabel.text = label;
                        keyboardInputLabel.visible = true;
                    }

                    keyboardInputField.state = "visible";
                    ui.setTimeOut(200, () => { keyboardInput.focus(); });
                }
            }

            function hide() {
                if (keyboardInputField.state === "visible") {
                    console.debug("Hide input");
                    keyboardInputField.originObj.inputValue = keyboardInput.inputValue;

                    keyboardInputField.state = "hidden";
                    keyboardInputLabel.visible = false;
                }
            }

            states: [
                State {
                    name: "hidden"
                    PropertyChanges { target: keyboardInputField; anchors.topMargin: -keyboardInputField.height; opacity: 0; visible: false }
                },
                State {
                    name: "visible"
                    PropertyChanges { target: keyboardInputField; anchors.topMargin: 0; opacity: 1; visible: true }
                }
            ]
            transitions: [
                Transition {
                    from: "visible"
                    to: "hidden"
                    SequentialAnimation {
                        PropertyAnimation { target: keyboardInputField; properties: "anchors.topMargin, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        PropertyAnimation { target: keyboardInputField; properties: "visible"; duration: 0 }
                        ScriptAction { script: buttonNavigation.releaseControl() }
                    }
                },
                Transition {
                    from: "hidden"
                    to: "visible"
                    SequentialAnimation {
                        PropertyAnimation { target: keyboardInputField; properties: "visible"; duration: 0 }
                        PropertyAnimation { target: keyboardInputField; properties: "anchors.topMargin, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        ScriptAction { script: buttonNavigation.takeControl() }
                    }
                }
            ]

            Components.ButtonNavigation {
                id: buttonNavigation
                defaultConfig: {
                    "HOME": {
                        "pressed": function() {
                            keyboard.hide();
                        }
                    },
                    "BACK": {
                        "pressed": function() {
                            keyboard.hide();
                        }
                    },
                    "DPAD_MIDDLE": {
                        "pressed": function() {
                            keyboard.hide();
                        }
                    }
                }
            }

            Text {
                id:  keyboardInputLabel
                width: parent.width
                height: visible ? implicitHeight : 0
                color: colors.offwhite
                anchors { top: parent.top; topMargin: 10 }
                maximumLineCount: 1
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                font: fonts.primaryFont(30)
                visible: false
            }

            Components.InputField {
                id: keyboardInput
                width: parent.width - 40
                anchors { top: keyboardInputLabel.bottom; topMargin: 10; horizontalCenter: parent.horizontalCenter }
            }

            Components.Button {
                text: qsTr("Done")
                width: parent.width - 40
                anchors { top: keyboardInput.bottom; topMargin: 20; horizontalCenter: parent.horizontalCenter }
                trigger: function() {
                    keyboard.hide();
                }
            }

            Rectangle {
                anchors.fill: parent
                color: colors.black
                opacity: ui.globalBrightness
            }
        }

        InputPanel {
            id: keyboard
            objectName: "keyboard"
            width: ui.width
            x: hiddenX
            y: hiddenY

            property int hiddenX
            property int hiddenY
            property int visibleX
            property int visibleY

            signal opened()
            signal closed()

            transformOrigin: Item.Center

            function show() {
                keyboard.active = true;
                keyboard.opened();
            }

            function hide() {
                if (keyboard.active) {
                    keyboard.active = false;
                    keyboard.closed();
                    keyboardInputField.hide();
                }
            }

            Connections{
                target: Qt.inputMethod

                function onVisibleChanged(){
                    if(!Qt.inputMethod.visible){
                        keyboard.hide();
                    } else {
                        keyboard.show();
                    }
                }
            }

            states: State {
                name: "visible"
                when: keyboard.active
                PropertyChanges {
                    target: keyboard
                    x: visibleX
                    y: visibleY
                }
            }
            transitions: Transition {
                id: inputPanelTransition
                from: ""; to: "visible"
                reversible: true
                ParallelAnimation {
                    NumberAnimation {
                        properties: "x, y"
                        duration: 300
                        easing.type: Easing.InOutExpo
                    }
                }
            }
        }

    } // end root

    Rectangle {
        anchors.fill: parent
        parent: Overlay.overlay
        color: colors.black
        opacity: ui.globalBrightness
        layer.enabled: true
        z: 4000

        Behavior on opacity {
            OpacityAnimator { duration: 300 }
        }
    }

    Component.onCompleted: {
        ui.inputController.setSource(applicationWindow);
        ui.inputController.activeController = containerMain;
        VirtualKeyboardSettings.locale = Qt.binding(function() { return Config.language })

        if (ui.isOnboarding) {
            loadingFirst.stop();
            containerMain.source = "qrc:/OnboardingContainer.qml";
        }
    }
}
