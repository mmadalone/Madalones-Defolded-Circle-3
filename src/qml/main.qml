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
import TouchSlider 1.0

import "qrc:/components" as Components
import "qrc:/settings/softwareupdate" as Softwareupdate
import "qrc:/components/entities/activity" as ActivityComponents
import ScreensaverConfig 1.0

ApplicationWindow {
    id: applicationWindow
    objectName : "applicationWindow"
    title: "Remote Two simulator"
    // hidden while the display is off, so that nothing is rendered (on a remote only)
    visible: Power.windowShown

    // Global flag: true when the screensaver is covering the UI.
    // Used by TouchSlider components to suppress volume/seek during screensaver.
    property bool screensaverActive: chargingScreenLoader.active
    // DEV-only dock fake: F12 toggles Battery.powerSupply (src/ui/devDockToggle.cpp, an application
    // event filter, so it works even under modal focus capture).

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
        // Without this the simulator window takes the window manager focus, the main window never
        // gets an active focus item, and every key-navigated settings page appears dead on the
        // desktop simulator even though it works on the device (which has no second window).
        flags: Qt.Window | Qt.WindowDoesNotAcceptFocus
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
        // an entity without a detail screen: EntityController.screenUrl() returns an empty URL
        if (String(source) === "") {
            return;
        }

        if (!containerSecond.loader.active) {
            console.debug("Loading second container", source);
            containerSecond.loader.openAfterLoad = openAfterLoad;
            containerSecond.loader.active = true;
            containerSecond.loader.setSource(source, parameters);
        }
    }

    function loadActivityToSecondContainer(entityObj) {
        ui.setTimeOut(1000, () => {
                          loadSecondContainer(EntityController.screenUrl(entityObj), { "entityId": entityObj.id, "entityObj": entityObj });
                      });
    }

    property bool isSecondContainerLoaded: containerSecond.loader.source != ""

    /**
      Drops whatever the second and third containers are showing, without playing their close
      animations. Used when a screen has to be replaced by something else right away.
      */
    function unloadSecondAndThirdContainer() {
        if (containerThird.loaderThird.active) {
            containerThird.close();
            containerThird.loaderThird.source = "";
            containerThird.loaderThird.active = false;
        }

        if (containerSecond.loader.active) {
            containerSecond.close();
            containerSecond.loader.source = "";
            containerSecond.loader.active = false;
            root.isActivityOpen = false;
        }
    }

    /**
      Brings up an activity screen on top of everything else, replacing the currently open screens.
      */
    function openActivity(entityObj) {
        // already showing this activity and nothing on top of it: leave it alone
        if (containerSecond.loader.item && containerSecond.loader.item.entityId === entityObj.id
                && !containerThird.loaderThird.active) {
            return;
        }

        unloadSecondAndThirdContainer();
        loadSecondContainer(EntityController.screenUrl(entityObj), { "entityId": entityObj.id, "entityObj": entityObj });
    }

    function loadThirdContainer(source, parameters = {}, openAfterLoad = true) {
        // an entity without a detail screen: EntityController.screenUrl() returns an empty URL
        if (String(source) === "") {
            return;
        }

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

    // Runs the readiness check of a sequence command in the core and calls callback(summary).
    //
    // summary is null when the check could not be made at all - no connection to the core, a core that does not
    // know the request, an error or an answer that takes too long: the check tells the user what is wrong, it
    // never decides whether the command may run.
    function requestSequenceReadiness(entityId, cmdId, callback) {
        const checkId = EntityController.checkSequenceReadiness(entityId, cmdId);

        if (checkId < 0) {
            callback(null);
            return;
        }

        function handler(id, result) {
            // several checks can be in flight at once, e.g. when turning off every activity at once
            if (id !== checkId) {
                return;
            }

            EntityController.sequenceReadinessResult.disconnect(handler);
            callback(result.supported ? result : null);
        }

        EntityController.sequenceReadinessResult.connect(handler);
    }

    // keyed by activity id and direction: the readiness check of a sequence that is already waiting
    property var activityReadinessWaits: ({})

    // Runs the readiness check of an activity and calls proceed(activityObj) once its sequence may run.
    //
    // A wakeup brings the integrations back one by one and the core reports it only once it is through, so
    // an activity started by the very button press that woke the remote regularly finds all of its devices
    // still disconnected. The sequence waits for them for as long as the resume window configured under
    // Power lasts - the same budget EntityController gives a single entity command - and only asks the user
    // whether to proceed anyway once that budget is spent.
    function checkActivityReadiness(activityObj, onSequence, proceed) {
        if (!activityObj) {
            return;
        }

        const activityId = activityObj.id;
        const cmdId = onSequence ? "activity.on" : "activity.off";
        const key = activityId + (onSequence ? ".on" : ".off");

        // a second press while the first one is still waiting would wait on its own and run the sequence
        // one more time once the devices are back
        if (applicationWindow.activityReadinessWaits[key]) {
            return;
        }

        // Eligibility is sampled once, when the sequence is requested: the wait exists for the devices that
        // a wakeup took away, not for the ones that are down for good.
        const waitForDevices = EntityController.resumePending;
        // provisional as long as the remote is still waking up, extended below once the window opens
        let deadline = Date.now() + EntityController.resumeTimeout;
        let windowSeen = EntityController.resumeWindow;

        function step() {
            const obj = EntityController.get(activityId);

            if (!obj) {
                // the activity is gone, there is nothing left to start
                delete applicationWindow.activityReadinessWaits[key];
                return;
            }

            // nothing is waking up and the user turned the check off for this activity: don't spend a request
            // on a report that would not be shown
            if (!obj.readyCheck && !waitForDevices) {
                delete applicationWindow.activityReadinessWaits[key];
                proceed(obj);
                return;
            }

            requestSequenceReadiness(activityId, cmdId, function(summary) {
                const current = EntityController.get(activityId);

                if (!current) {
                    delete applicationWindow.activityReadinessWaits[key];
                    return;
                }

                if (summary && summary.verdict === "ready") {
                    delete applicationWindow.activityReadinessWaits[key];
                    proceed(current);
                    return;
                }

                // the wakeup is reported after the fact, so the window opens while we are already waiting: give
                // the sequence the full window from that point, the way EntityController extends its commands
                if (!windowSeen && EntityController.resumeWindow) {
                    windowSeen = true;
                    deadline = Math.max(deadline, Date.now() + EntityController.resumeTimeout);
                }

                // Not ready, or the check itself failed - right after a wakeup that usually means the connection
                // to the core is still coming back. Both are worth waiting out, for as long as the resume budget
                // lasts: waiting past a lost connection is pointless, nothing can be sent over it and the devices
                // cannot come back before it does.
                if (waitForDevices && EntityController.resumePending && ui.coreConnected && Date.now() < deadline) {
                    applicationWindow.activityReadinessWaits[key] = true;
                    ui.setTimeOut(1000, step);
                    return;
                }

                delete applicationWindow.activityReadinessWaits[key];

                // no report to show, or the user turned the check off: the check never holds the sequence back
                if (!summary || !current.readyCheck) {
                    proceed(current);
                    return;
                }

                readinessCheck.show(summary, function() {
                    // the activity may be gone by the time the user answers
                    const target = EntityController.get(activityId);

                    if (target) {
                        proceed(target);
                    }
                });
            });
        }

        step();
    }

    Connections {
        target: ui.inputController
        ignoreUnknownSignals: true

        function onGlobalPowerLongPressed() {
            if (!SoftwareUpdate.updateInProgress) {
                powerOffLoader.active = true;
            }
        }
    }

    // madalone: the remote woke up (power mode back to Normal) = user activity, so the idle screensaver
    // countdown starts again. The window's visibility follows Power.windowShown (upstream 42f9888).
    Connections {
        target: Power
        ignoreUnknownSignals: true

        function onPowerModeChanged(fromPowerMode, toPowerMode) {
            if (toPowerMode == PowerModes.Normal && idleScreensaverTimer.running) {
                idleScreensaverTimer.restart();
            }
        }
    }

    Components.ButtonNavigation {
        overrideActive: true
        defaultConfig: {
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

    Item {
        id: root
        objectName: "root"
        width: ui.width
        height: ui.height

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
                if (!ui.isOnboarding) {
                    loadingFirst.stop();

                    containerSecond.close();
                    containerThird.close();

                    if (ui.profiles.count === 0) {
                        containerMain.setSource("qrc:/components/ProfileAdd.qml", { state: "visible", noProfile: true })
                    } else {
                        containerMain.setSource("qrc:/components/ProfileSwitch.qml", { state: "visible", noProfile: true })
                    }
                }
            }
        }

        Connections {
            target: EntityController
            ignoreUnknownSignals: true

            // an activity turned on without being started here, e.g. through the API: optionally
            // bring its screen up, replacing whatever is currently open
            function onActivityStartedExternally(entityId) {
                if (!Config.openActivityOnApiStart || ui.isOnboarding) {
                    return;
                }

                const entityObj = EntityController.get(entityId);

                if (!entityObj) {
                    return;
                }

                console.debug("Opening activity started via the API:", entityId);
                openActivity(entityObj);
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
                    } else if (status == Loader.Error) {
                        // Nothing was loaded, so there is no item that could ever emit closed() - the only
                        // thing that releases the container again. Without this the container stays active
                        // and open, and loadSecondContainer() refuses every screen from then on.
                        // Entity screens come from EntityController.screenUrl(), which only returns screens the
                        // testEntityScreens unit test found in the qrc: this is the safety net if one is missing.
                        console.warn("Second container failed to load:", loader.source);
                        loader.source = "";
                        loader.active = false;
                        containerSecond.close();
                        root.isActivityOpen = false;
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
                    } else if (status == Loader.Error) {
                        // Same as for the second container: without an item nothing ever emits closed(), and
                        // loadThirdContainer() would refuse every further screen. The exit transition resets
                        // the loader, so this leaves exactly the state a regular close does. The hide
                        // animation of the second container has not been started, nothing was shown yet.
                        console.warn("Third container failed to load:", loaderThird.source);
                        containerThird.close();
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

        // madalone (v1.4.43): one-shot — set true when the next open is triggered by an
        // actual user dock event (not boot-while-docked, not idle-open, not wake). When
        // Config.dockChimeAfterScreensaver is on, ChargingScreen.qml's onOpened uses this
        // flag to fire the dock chime AFTER the popup is fully visible (pattern B).
        // Cleared by the popup's onOpened or by a competing trigger before it fires.
        property bool _nextOpenViaDock: false

        // madalone (v1.4.40): startup-grace flag for boot-while-docked detection.
        // When the device boots while already on the dock, Battery.powerSupply pumps
        // its initial state (true) before InputController / ButtonNavigation /
        // QML Popup parenting are fully wired. The screensaver opens but its tap +
        // button-press dismiss handlers don't route — leaving the user with an
        // unclearable screen until force-restart. _bootGraceActive=true during the
        // first 3 s after QML load; powerSupplyChanged uses a longer activation
        // delay (2 s) and skips the audio chime when this flag is set.
        property bool _bootGraceActive: true
        Timer {
            id: bootGraceTimer
            interval: 3000
            repeat: false
            running: true
            onTriggered: root._bootGraceActive = false
        }

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

        // madalone (v1.4.40): grace delay between dock chime and screensaver async load.
        // ChargingScreen.qml is heavyweight (Matrix rain shader compile, glyph atlas build,
        // GPU buffer upload). Loading it on the same JS tick as SoundEffects.play() saturates
        // the main thread, starves the ALSA audio thread, and produces audible buffer
        // underruns ("choppy" chime). The original 350 ms covered all 6 synth chimes (max
        // ~500 ms duration). v1.4.42 added 6 user-curated wavs with durations up to ~3 s,
        // so the grace duration is now chime-variant-aware: max(350, chimeMs + 100).
        // Boot-while-docked branch overrides this to 2000 ms (no chime fires there anyway).
        Timer {
            id: dockChimeGraceTimer
            repeat: false
            running: false
            interval: 350
            onTriggered: {
                if (Battery.powerSupply && !chargingScreenLoader.active) {
                    chargingScreenLoader.active = true;
                }
            }
        }

        // Per-variant chime duration in ms. Indexed by Config.dockChimeVariant - 1.
        // Updated when gen_sounds.py regenerates wavs; values match what the wav writer
        // produces for synth chimes (1-6) and the source duration of user wavs (7-12).
        readonly property var _chimeDurationsMs: [
             340,  360,  500,  280,  400,  250,    // 1-6: synth (Warp/Ascend/Bell/Chord/Pulse/Zap)
            1750,  940, 2930, 1800, 1240, 2490     // 7-12: user (PwrDown/PwrHold/Up1/Up2/TOS/TOSLong)
        ]
        function _chimeGraceForVariant(v) {
            if (v < 1 || v > 12) v = 1;
            return Math.max(350, _chimeDurationsMs[v - 1] + 100);
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
                    // madalone (v1.4.43): propagate dock-event flag for pattern B (chime fires
                    // in popup.onOpened). Gated on Battery.powerSupply so a stale flag from
                    // a quick dock-then-undock during the load doesn't fire the chime.
                    item._openedViaDock = root._nextOpenViaDock && Battery.powerSupply;
                    root._nextOpenViaDock = false;
                    item.open();
                }
            }

            Connections {
                target: Battery
                ignoreUnknownSignals: true

                function onPowerSupplyChanged(value) {
                    if (value) {
                        idleScreensaverTimer.stop();
                        if (root._bootGraceActive) {
                            // madalone (v1.4.40): boot-while-docked. Initial Battery state pump
                            // happens before InputController / ButtonNavigation / Popup parenting
                            // are fully wired — opening the screensaver here leaves it undismissible.
                            // Defer 2 s instead of 350 ms (gives QML/input subsystems time to
                            // settle), and skip the chime since this isn't a user-initiated dock.
                            dockChimeGraceTimer.interval = 2000;
                            dockChimeGraceTimer.restart();
                        } else if (Config.dockChimeAfterScreensaver) {
                            // madalone (v1.4.43, default): pattern B — open the screensaver
                            // immediately (no grace timer), and fire the chime AFTER the popup
                            // transitions in. ChargingScreen.qml's onOpened consumes the
                            // _openedViaDock flag and calls SoundEffects.play() then. Net: visual
                            // and audio both arrive at ~450 ms after dock, no contention chopping
                            // because the audio thread fires after all heavy QML/GPU init is done.
                            root._nextOpenViaDock = true;
                            chargingScreenLoader.active = true;
                        } else {
                            // Legacy / opt-out: play chime FIRST (audio thread gets head-start),
                            // then defer screensaver load by chime-variant-aware grace duration
                            // so ChargingScreen.qml's heavyweight QML/GPU init doesn't starve the
                            // ALSA audio thread mid-playback. Synth chimes (1-6) are short enough
                            // for the 350 ms floor; user wavs (7-12) are 1-3 s long and need a
                            // proportionally longer grace.
                            SoundEffects.play(SoundEffects.BatteryCharge);
                            dockChimeGraceTimer.interval = root._chimeGraceForVariant(Config.dockChimeVariant);
                            dockChimeGraceTimer.restart();
                        }
                    } else {
                        dockedRearmTimer.stop();   // madalone (v1.4.15): undocked → no docked rearm
                        dockChimeGraceTimer.stop(); // madalone (v1.4.40): undocked before grace fired
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
        Components.ReadinessCheck {
            id: readinessCheck
        }

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

        // not an OpacityAnimator: an animator runs on the render thread and cannot finish while the window is hidden
        // with the display off, and until it does Qt wakes the main thread at the display refresh rate. The brightness
        // is set at start-up, also when the UI starts with the display off.
        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }
    }

    Component.onCompleted: {
        ui.inputController.setSource(applicationWindow);
        VirtualKeyboardSettings.locale = Qt.binding(function() { return Config.language })

        if (ui.isOnboarding) {
            loadingFirst.stop();
            containerMain.source = "qrc:/OnboardingContainer.qml";
        }
    }
}
