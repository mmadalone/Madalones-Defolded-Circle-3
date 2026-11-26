# Upstream merge playbook

This fork of [unfoldedcircle/remote-ui](https://github.com/unfoldedcircle/remote-ui) adds custom screensaver mods on top of the upstream UC codebase. The `upstream` remote points at the UC repo; merges from upstream are manual and require conflict resolution on the files listed below. This doc is the reference for how to do those merges without breaking the custom work.

## Remote setup

Run once on a fresh clone:

```shell
git remote add upstream https://github.com/unfoldedcircle/remote-ui.git
git fetch upstream
```

Verify with `git remote -v` — you should see both `origin` (this fork) and `upstream` (UC's repo).

## Upstream files we've modified (conflict surface)

These upstream files carry our changes and will conflict, or need a look, on every upstream merge. The exact list
for the current tree is one command away (`M` = upstream file we modify, `A` = our file, `D` = upstream file we removed):

```shell
git fetch upstream refs/tags/vX.Y.Z:refs/upstream-public/vX.Y.Z   # the upstream release we are on
git diff --name-status refs/upstream-public/vX.Y.Z HEAD
```

As of v1.6.5 (upstream v0.84.0): 67 upstream files modified, 3 removed (`.github/dependabot.yml`,
`.github/workflows/openspec-version.yml`, the `CLAUDE.md` symlink), the rest of the tree is upstream's or ours. v1.6.3's
Messages settings modified four of them for the first time (`src/ui/notification.{h,cpp}`,
`src/qml/components/Notification.qml`, `src/qml/components/entities/Base.qml`). v1.6.4 modifies none for the first
time: the upstream files it touches (`src/qml/main.qml`, `src/qml/components/ChargingScreen.qml`, the four
`TouchSlider*.qml`, `remote-ui.pro`, `test/CMakeLists.txt`, `resources/translations/en_US.ts`, `README.md`,
`CHANGELOG.md`) were already on the list. v1.6.5 modifies none for the first time either, so the count
stays 67: its 13 upstream files (`src/core/core.{h,cpp}`, `src/hardware/hardwareController.{h,cpp}`, `src/main.cpp`,
`src/qml/main.qml`, `src/qml/components/ChargingScreen.qml`, `src/ui/inputController.{h,cpp}`, `remote-ui.pro`,
`resources/translations/en_US.ts`, `README.md`, `CHANGELOG.md`) were already on the list (counted on 2026-11-20:
each changed path of the working tree that exists in `refs/upstream-public/v0.84.0` already differed from it at
v1.6.4).

| Area | Upstream files | What we keep | Conflict risk |
|---|---|---|---|
| Build | `remote-ui.pro`, `resources/qrc/main.qrc`, `.gitignore` | Our sources and QML in HEADERS / SOURCES / qrc; `VERSION` in sync with `deploy/release.json`; extra warning flags; ignore entries (incl. `CLAUDE.md`) | High |
| CI | `.github/workflows/build.yml` | Version-sync + CHANGELOG gates, UCR3 install-bundle staging (every file of `deploy/config/`, no sub-dirs), GPG signing, digest-pinned 5.15.19 toolchain (upstream uses the same image unpinned) | Medium |
| Start-up | `src/main.cpp` | MatrixRain / Palettes registration; `ScreensaverConfig` built after `hwController` and before `uiController`; keeper + suppressor wiring; `ScreenOffSync` input; `DevDockToggle` in DEV; the Messages switches pushed into `Notification::setErrorFeedback` and `Notification::setUnavailableBanners` before the UI loads and on every change; Close on wake and the Undock delay pushed into `UndockCheck` at start-up and on every change, and the charger once at start-up (v1.6.5; without them it stays at Never and never runs) | High |
| Settings store | `src/config/config.{h,cpp}` | Our Q_PROPERTYs: phantom wake, keep awake, dock chime, media buttons (v1.4.8), volume overlay, WiFi chip, Messages (`showErrorPopups`, `showErrorBanner`, `errorBannerSec`, `showUnavailableBanners`, `otherBannerSec`, keys `ui/<name>`) | High |
| Core API | `src/core/core.{h,cpp}`, `src/core/structs.h` | `setPowerMode` (Mod 6, body field `mode`), REST helpers + standby-inhibitor calls (Mod 5), `getBatteryRest` / `respBatteryRest` (v1.6.5, the quick undock check), `Inhibitor` struct. Keep our methods above upstream's `private:` and our `resp*` signals in its `signals:` block | High |
| Hardware | `src/hardware/hardwareController.{h,cpp}`, `src/system/battery.h`, `src/system/wifi.{h,cpp}` | Keeper / suppressor construction and wiring; `ScreenOffSync` (v1.6.2) and `UndockCheck` (v1.6.5), built after them: `ScreenOffSync` connected to Power and the touch slider, `UndockCheck` to Power, Battery and the core's `disconnected`, and its `closeRequested` to `PhantomWakeSuppressor::onQuickUndock` (v1.6.5: a close by the quick undock check counts as activity for the phantom-wake check), both registered as the QML singletons `ScreenOffSync 1.0` / `UndockCheck 1.0` that the screensaver's QML imports, with `getScreenOffSync()` / `getUndockCheck()`; `Battery::instance()`; WiFi W1 (WoWLAN shown), W2 link fields, W4 reassociate, W6 30 s poll, W13 diagnostics | High: upstream reworks Wi-Fi often |
| Entities | `src/ui/entity/entityController.{h,cpp}`, `src/ui/entity/mediaPlayer.{h,cpp}` | Mod 5 hook in `connectLazySignals` (only players a screen asked for), `entityCommandIssued`, `LOW_POWER` in the wake flag; `hide_volume_overlay`; browse-thumbnail preview on the artwork pipeline; the Messages block in `handleCommandFailure` (after the wake-up resend, the `voice_start` hand-off, the repeat drop and the entity-name lookup, just before `switch (code)`: with the pop-ups off, remove the pending command and show the error banner instead); in `refuseUnavailableEntity` (an activity's button mapping to an unavailable entity) `Notification::createUnavailableBanner` instead of `createNotification`, so "Show offline banners" can hide the "%1 is unavailable" toast | **High: conflicted on every merge so far.** Upstream keeps reworking wake/standby and artwork; the failure path is where upstream would land non-blocking command errors (see below) |
| Notifications | `src/ui/notification.{h,cpp}`, `src/qml/components/Notification.qml` | Messages: static `setErrorFeedback` / `errorPopupsEnabled` / `createErrorBanner` and the `errorBannerCreated` signal, `Q_INVOKABLE bannerInsteadOfPopUp()` for QML; static `setUnavailableBanners` / `createUnavailableBanner` (the "%1 is unavailable" toast through `createNotification`, only while its switch is on) and `Q_INVOKABLE showUnavailableBanner()` for QML; the toast's handler moved into `show(message, warning, durationMs)` with a per-message `displayMs` (was a fixed 4000): `onNotificationCreated` with "Other banner duration", the new `onErrorBannerCreated` with "Error banner duration". No filter in QML: whether a toast is created is decided in C++ | Medium: our hunks replace upstream's whole toast handler |
| Input, sound, UI | `src/ui/inputController.{h,cpp}`, `src/ui/soundEffects.{h,cpp}`, `src/ui/uiController.{h,cpp}`, `src/logging.{h,cpp}` | `touchDetected` (the idle screensaver's timer, Mod 6 and, since v1.6.2, the screen-off sync); since v1.6.5 `Q_INVOKABLE dropPendingKeys(QObject* owner)` and its block in the KeyPress branch of `eventFilter()`, between `cancelDeferredRelease(key);` and `const bool activated` (the keys a closing screensaver still has down; a block of ours in the middle of an upstream function, a deliberate exception to STYLE_GUIDE §3.5, because it has to run after that cancel and before the keypad state; it relies on upstream's `m_keyOwner[key] = owner;`, `m_keyOwner.take(key)` and `emit keyReleasedFor(owner.data(), mappedKey);`: see "Key navigation (v1.6.5)" below for the departures and the checks); bundled sounds, dock-chime variants, `UC_SOUND_EFFECTS_PATH` fallback; EntityController accessor; the Messages block in `onIntegrationError` (the error banner instead of the "%1 error" pop-up); `lcScreensaver` | Medium |
| Main window | `src/qml/main.qml`, `src/qml/components/ChargingScreen.qml` | The screensaver Loader block: dock chime (`_nextOpenViaDock`, set only while no screensaver item exists; `dockChimeGraceTimer` / `_chimeGraceForVariant`), boot grace, docked rearm (`dockedRearmTimer`; since v1.6.5 it and `dockChimeGraceTimer` restart instead of opening the screensaver while `UndockCheck.undockPending`), the idle screensaver (`idleScreensaverTimer` and its touch / key / touch-slider resets), Close on wake (where upstream closes the charging screen on every charger loss, ours asks `ScreensaverConfig.closesOnChargerLoss()`; while the screensaver is active a wake to Normal asks `closesOnWake()` first, and upstream's re-open on a wake on the charger became its `else` branch, which since v1.6.5 reads `else if (!chargingScreenLoader.active && ScreensaverConfig.reopensOnWake(fromPowerMode === PowerModes.Idle, Battery.isCharging, Battery.powerSupply, UndockCheck.undockPending))`: upstream's condition, asked in C++, plus no re-open while `undockPending`; both close through `_closeScreensaverNow()`, which also cancels a screensaver still loading and clears `_nextOpenViaDock`), the quick undock check (v1.6.5: `import UndockCheck 1.0`, a `Binding` of `UndockCheck.screensaverOpen` to the popup's `opened`, without which it never runs, and a `Connections` whose `onCloseRequested` is a third close through `_closeScreensaverNow()`), `displayOff` on Low_power / Suspend, `screensaverActive`, the `hostContainer` binding; the screensaver popup is a full replacement, whose `onClosed` calls `ui.inputController.dropPendingKeys(buttonNavigation.scope)` next to `releaseControl()` (v1.6.5) | Very high |
| Settings pages | `settings/Settings.qml`, `settings/settings/{Power,Sound,Ui,Wifi,WifiInfo}.qml`, `onboarding/Wifi.qml` | Screensaver menu entry; our setting rows and chip pickers; W10 scan gate on display off; Reconnect / Diagnostics buttons; in `Ui.qml` the Messages section as the last item (our `settings/settings/userinterface/MessagesSection.qml`), reached with DPAD_DOWN from `mediaSourceButtonSwitch`. The section reveals the focused row with `Qt.callLater` so it runs after `ButtonNavigation`'s own synchronous `onWindowFocusItemChanged` reveal of the control: if upstream defers that reveal, re-check the row reveal (`test/qml/tst_messages_reveal_order.qml`) | High: upstream restyles settings |
| Components | `Slider.qml`, `Switch.qml`, `StatusBar.qml`, `Page.qml`, `LoadingFirst.qml`, `VolumeOverlay.qml`, `TouchSlider{Volume,Seek,Brightness,Position}.qml`, `entities/Base.qml`, `entities/TitleStatus.qml`, `entities/activity/MediaComponent.qml`, `entities/media_player/MediaBrowser.qml`, `media_player/deviceclass/*.qml` | Tick gate (v1.4.33), DPAD centre toggles a switch, W3 signal bar, volume feature guards, DEV skips the splash, volume overlay toggles, screensaver touch-slider guards (press and release; v1.6.4: a handler that ends a slide under way when the screensaver opens, and a no-slide guard on the release; `tools/check_touchslider_guards.py` names a dropped one) plus `Math.round` for upstream's `Math.rodun` in `TouchSliderBrightness.qml`, and the Messages error banner for an unavailable entity (inserted before upstream's untouched "Touch slider is not available." block), Messages in `entities/Base.qml`: its 3 "%1 is unavailable" calls through `ui.notification.showUnavailableBanner()` instead of `ui.createNotification()` (two in `handleActivityOpen()`, one in `commandAllowed()`), title-row mirrors + WiFi chip, browse plumbing + MUTE/STOP/NEXT/PREV/CHANNEL keys, v1.4.8 media-button toggles | Medium; higher for `entities/Base.qml`: 11 upstream commits after v0.74.5, 3 of them on the "%1 is unavailable" lines our calls replace |
| Other | `resources/icons/icon-fallback.json`, `tools/design-check-allow.txt`, `README.md`, `CONTRIBUTING.md`, `SECURITY.md`, `CHANGELOG.md`, `test/**`, `docs/**`, `resources/translations/en_US.ts` | `wave-pulse` fallback; screensaver artwork on the allow-list; fork docs; our test harness and suites | Low |

For the authoritative custom-file manifest (purely custom, not modified upstream), see [CUSTOM_FILES.md](CUSTOM_FILES.md).

## Divergence from upstream's specs

Upstream describes what the app does in `openspec/specs/`. This section lists, for the Messages settings, the
screensaver, key navigation and the touch slider, where our app behaves differently from a requirement there, by default or only with one of our settings
changed, so a merge can check whether upstream changed that requirement in the meantime. Not listed yet: LOW_POWER in
the wake flag of "Command retry window after wakeup" (the spec says Suspend; see the Entities row above), and the Power
Saving page ("Power saving settings"): "Keep WiFi connected in standby" shown on a Remote 3 without `UC_WOWLAN=true`,
plus our own rows there.

### Messages settings (v1.6.3)

With the defaults ("Show error pop-ups" on, "Show offline banners" on, "Other banner duration" 4 s)
the app does what upstream's specs say. Only a changed setting departs from them:

| Spec | Requirement | With the setting changed |
|---|---|---|
| [`entity-commands`](../openspec/specs/entity-commands/spec.md) | "Command failure feedback": 408 / 503 show the prompt "%1 is not responding" with "Try again" and "Cancel"; any other code shows the warning notification "Error sending the command" | "Show error pop-ups" off: the error banner, "%1 is not responding" (neutral) or "%1 is not responding. Error code: %2" (warning), or nothing with "Show a banner" off. Removing the failed command from the pending commands first, the `voice_start` hand-off and the silent drop of a failed repeat are unchanged. |
| `entity-commands` | "Try again"; "Identical prompts are not stacked" | No prompt, so no "Try again": the failed command is no longer pending, so pressing the control again sends it ("An earlier failure never blocks the next press" still holds). Banners replace each other instead of stacking. |
| `entity-commands` | "No commands to an unavailable entity from its controls": a command refused there (an entity tile, an activity's button mapping) shows the warning notification "%1 is unavailable" naming the entity; the same in the scenarios "Unavailable entity on an awake remote" and "Retry after wakeup disabled" of "Commands are accepted while the remote is waking up". The same notification is required by [`activities`](../openspec/specs/activities/spec.md) ("Turning an activity on from its tile", "Physical button mapping in an activity"), [`entity-detail-controls`](../openspec/specs/entity-detail-controls/spec.md) ("Opening an entity control screen", "Inverted button behaviour setting") and [`pages`](../openspec/specs/pages/spec.md) ("Tile selection by d-pad"), and `notifications` lists it under "Which events produce which kind" | "Show offline banners" off: the command is still refused and no screen opens, without the notification (`EntityController::refuseUnavailableEntity` and the 3 calls in `entities/Base.qml` go through `Notification::createUnavailableBanner`). Every other toast these specs require still shows. |
| [`notifications`](../openspec/specs/notifications/spec.md) | "Which events produce which kind": entity command errors (408 / 503 "%1 is not responding", other codes "Error sending the command"), "Touch slider is not available." and integration connection errors ("%1 error") are actionable notifications | "Show error pop-ups" off: toasts (the error banner) instead, or nothing. |
| `notifications` | "Toast presentation and duration"; "Toasts replace each other without a queue": a toast closes 4000 ms after it opened, and a replacement restarts the 4000 ms | Each toast closes after its own duration: "Other banner duration" (1–30 s, default 4) or "Error banner duration" (1–30 s, default 5); a replacement restarts the timer with its own duration. |
| [`touch-slider`](../openspec/specs/touch-slider/spec.md) | "Unavailable entity": the actionable notification "Touch slider is not available." / "%1 is not available. Please check your configuration." once | "Show error pop-ups" off: the warning toast "%1 is unavailable" (the tiles' text) once, with the same latch, or nothing. |
| [`remote-entity`](../openspec/specs/remote-entity/spec.md) | "Command failures" (scenario "Dock offline"): 408 / 503 show "%1 is not responding" with a "Try again" action; any other error "Error sending the command" | "Show error pop-ups" off: the error banner, as for `entity-commands` above, or nothing. A failed repeat is still dropped silently. |
| [`core-connection`](../openspec/specs/core-connection/spec.md), [`integrations`](../openspec/specs/integrations/spec.md) | "Integration connection indicator" (scenario "Integration error") and "Integration and driver state follow core events" (scenario "Driver enters an error state"): the actionable warning "%1 error" / "Error while connecting to %1, with id %2", not during the onboarding | "Show error pop-ups" off: the warning toast "Error while connecting to %1, with id %2" (the error banner), or nothing; still nothing during the onboarding. |

Upstream's tracker has two open feature requests in this area:
[#783](https://github.com/unfoldedcircle/feature-and-bug-tracker/issues/783) "Non-blocking error feedback for failed
commands in a running activity", which is what "Show error pop-ups" off does for single commands, and
[#802](https://github.com/unfoldedcircle/feature-and-bug-tracker/issues/802) "Error feedback and recovery options when
an activity sequence fails", for failed sequences, which our switch leaves alone. If upstream ships non-blocking
command errors, drop or revisit this block: take upstream's behaviour and keep only what it doesn't cover (the banner
durations, "Show offline banners").

### Screensaver (charging screen)

Our screensaver replaces the charging screen of [`power-and-battery`](../openspec/specs/power-and-battery/spec.md),
requirement "Charging screen". It departs from it with the default settings too:

| Requirement | Ours |
|---|---|
| Opens when a power supply becomes connected, playing the BatteryCharge sound (scenario "Docked") | Opens with the "Dock chime" picked in Settings → Sound & Haptic (12 choices). With "Chime after screensaver" on (the default) the chime plays once the screensaver has faded in, and not at all when the screensaver is already showing; with it off the chime plays at the dock and the screensaver starts loading 350 ms to 3.03 s later (`_chimeGraceForVariant`), or one such grace later again for as long as `UndockCheck.undockPending` (v1.6.5; from the code: no test, simulator run or remote has exercised it). |
| "Reboot on charger": the screen appears and the sound plays once | In the first 3 s after the UI starts, a charger report opens the screensaver 2 s later, without a chime (`_bootGraceActive`). |
| Opens when the power mode changes to Normal from any mode but Idle while charging with a power supply connected (scenario "Wake on charger") | The same while the screensaver is closed, with one exception since v1.6.5: for up to 6 s after the quick undock check closed the screensaver, until the core reports the charger gone (`UndockCheck.undockPending`), the wake opens nothing (`reopensOnWake()` in `src/ui/screensaverClosePolicy.h`, asked through `ScreensaverConfig.reopensOnWake()`; checked in tests and in the desktop simulator, not yet on a remote). Battery still says "on the charger" then, and on test build 1.6.4-keys1 such a wake opened the screensaver again in the hand that had lifted the remote (seen once on the owner's remote, 2026-11-15, after "Suppress phantom wake-ups" had switched the display off and the core had woken it again). If the remote did stay on the charger, "Run after dismissal while docked" opens the screensaver later. While it is open or loading, "Close on wake" is asked first (`closesOnWake`; the re-open is its `else` branch): Always closes a loaded one at every wake to Normal, on the charger too; Undocked keeps it while a charger is reported; Never always keeps it. In the 13 lifts recorded on the owner's Remote 3 the remote woke 2.3–4.6 s before the core reported the charger gone, so a lift that wakes a charging remote whose display is off opens a closed screensaver; with Never it then stays. |
| An analogue clock (12 dots, hour, minute and second hands) and a bolt with `<level>% - Charging`, or `<level>%` when supplied without charging | Five themes: Matrix (the default), Starfield, Minimal, Analog (UC's clock as a theme, with its own screen-off effect) and TV Static. A clock overlay on Matrix, Starfield and TV Static, off by default. The battery overlay: a bolt with "%1% - Charging", "100% - Fully charged" or "%1%", shown by default only while a charger is reported ("Charging only"). |
| Closes when the power supply is removed | Only with "Close on wake" set to Undocked or Always (`closesOnChargerLoss`); Never, the default, keeps it showing. Since v1.6.5, while the screensaver shows lit or dimmed on the charger, the quick undock check asks `GET /api/system/power/battery` every 250 ms and closes it once N answers in a row say the charger is gone, N = "Undock delay" ÷ 250 ms + 1 (3 at the default 500 ms, the first and the last at least 500 ms apart; `src/hardware/undockCheck.h`). The delay rules only this check: the core's own report closes the screensaver at once. On 7 lifts with the screensaver lit or dimmed (2026-11-15) that endpoint first said so 1.4–4.5 s before the core's report. On the owner's remote, test build 1.6.4-keys1, whose rule needed one answer fewer, closed it 0.23–4.50 s before the core's report on the 7 lifts the core confirmed (2026-11-15 to 2026-11-19); with the three-answer rule (test build 1.6.4-keys2, 2026-11-26) the core's report followed the close by 0.31–4.15 s on 8 of 9 closes. Undocked closes it, one still loading included, so it meets this requirement. Always closes only one that has finished loading; one still loading opens afterwards, as up to v1.6.3. The core also reports the power supply removed when a Qi pad stops charging (seen at 80 % and when full; on the owner's remote with test build 1.6.4-keys1, set to Undocked, that closed the dark screensaver six times). |
| Closes when the screen is tapped | "Double-tap to close" on (the default): two taps within 300 ms; off: one tap. With it on, a single tap plays Matrix's tap effects, or TV Static's channel flash with "Flash on tap" on, 300 ms later (v1.6.5; `ScreensaverConfig.loneTapAction()`, `src/ui/screensaverInputPolicy.h`). On Matrix with "Touch directions" on, the 8 outer zones steer the rain and the fourth quick tap in the centre closes it. A theme name the app doesn't know loads Matrix and gets Matrix's taps (v1.6.5, `matrixLoaded`). |
| Closes on the press of 24 keys (BACK, HOME, VOICE, VOLUME_UP/DOWN, GREEN, YELLOW, RED, BLUE, DPAD_UP/DOWN/LEFT/RIGHT/MIDDLE, CHANNEL_UP/DOWN, MUTE, PREV, PLAY, NEXT, POWER, STOP, RECORD, MENU) | On Starfield, Minimal, Analog and TV Static all 24 close it, on their release (next row). On Matrix with "DPAD interactive" on (the default) the 9 keys DPAD_UP/DOWN/LEFT/RIGHT/MIDDLE, VOLUME_UP/DOWN and CHANNEL_UP/DOWN don't: the 8 direction keys steer the rain, and DPAD_MIDDLE fires a flash, or a chaos burst with "Chaos events" on (nothing with "Glitch effect" off; pressed twice: back to the set direction; held: slower rain); the other 15 close it, and all 24 do with DPAD interactive off (`ScreensaverConfig.keyAction()`, `src/ui/screensaverInputPolicy.h`). Up to v1.6.4 DPAD interactive, although shown only for Matrix, kept those 9 keys from closing every theme (on TV Static the direction keys flashed it). |
| (the press closes it) | The release closes it, and only the release of a key pressed while it showed (v1.6.5); a held key keeps it until it is let go, and a lone release whose press never reached the UI (the input was blocked at that moment, or the firmware didn't pass a waking press on as a key, as seen once on firmware 2.9.1) does nothing; when such a key is held, the screensaver takes its first auto-repeat for a press and closes at the release (see the limits under "Key navigation (v1.6.5)"). Up to v1.6.4 the press closed it: the popup was gone about 60 ms later (the 50 ms exit, then `main.qml` switched its Loader off), and UC's InputController sent the key's release, and the repeats of a held key, to the item that had the input then, the page behind (`inputController.cpp`), where a cover page acts on the release. UC's 300 ms fade hides a quick press, not a held key. Pinned by `test/qml/tst_screensaver_keys.qml`, which runs the real InputController (since the held-key change of v1.6.5, next row). Checked in tests, in the desktop simulator (2026-11-15) and on a remote on Starfield (test build 1.6.4-keys2, 2026-11-26); not on Minimal, Analog or TV Static. |
| (none; by UC's routing, once the item that got a key's press is destroyed, the key's repeats and its release go to the item in front) | When something else closes it while a key whose press it got is still down (the wake that key caused, a lift, a tap, another key), the page behind acts on neither that key's repeats nor its release (v1.6.5): `onClosed` hands the keys it still has down to `InputController::dropPendingKeys()`. Seen in the desktop simulator (2026-11-19/20): without it a blind's page moved its blind and Settings walked back under a held key; with it nothing reached the page in the same five runs. On a remote (test build 1.6.4-keys2, 2026-11-26): a held BACK and POWER with Always, and the close on release on Starfield. This departs from six requirements of `key-navigation` and one of `entity-detail-controls`: see "Key navigation (v1.6.5)" below. |
| (none) | A wake can close it (`closesOnWake` in `src/ui/screensaverClosePolicy.h`): with "Close on wake" set to Always, every wake to Normal closes one that has finished loading; with Undocked, a wake while no charger is reported closes it, one still loading included. |
| (none) | It opens by itself. On the charger, "Run after dismissal while docked" reopens it that long after it closes (5–120 s, default 60, no off position), even while the remote is in use, unless the time runs out while a page is being reordered; since v1.6.5, when it runs out while `UndockCheck.undockPending` (a close by the quick undock check that the core's charger report hasn't followed yet, at most 6 s), it runs another round, so at 5 s a close the core never confirms (a lift and put-back, a bump that cut the charging for longer than the delay) brings the screensaver back about 10.5 s later (10.5–10.7 s after each of the four such closes on the owner's remote with test build 1.6.4-keys1). Off the charger, with "Idle screensaver" on (off by default), it opens after "Idle timeout" (15–55 s, default 45) without a key press, a touch or the touch slider. |
| Open and close fade over 300 ms | Opens over 300 ms, closes in 50 ms (`ChargingScreen.qml` `enter` / `exit`). |

Up to v1.6.4 a key that closed the screensaver on its press passed its release, and the repeats of a held key, to the
screen underneath: the screensaver was already gone, and UC's InputController sent them to the item that had the input
then (`inputController.cpp`). Since v1.6.5 the release closes it, and a key that is still down when something else
closes it is dropped (the table above). So on the input-ownership path the scoped handlers of the screen underneath
get nothing of a key the screensaver got; handlers that listen to every owner (`overrideActive`: a light's brightness
or colour page, the VOICE handler in `main.qml`) get its press, its repeats until the screensaver has closed and its
release, and act on them as upstream says. On the QML focus chain ours does what upstream's scenario "Key press on
the charging screen" says (`openspec/specs/power-and-battery/spec.md`: DPAD_MIDDLE closes the charging screen, and
the key also reaches the keyboard focus chain of the screen underneath) for the press and for its repeats until the
screensaver has closed: neither popup takes the keyboard focus, and the controller passes a plain key press on as
upstream's does. So a control of the page behind that holds the keyboard focus gets the key there (for example the
source list of a media player screen, `entities/media_player/SourceList.qml`, which calls `forceActiveFocus()` when
it opens: in a probe with the real files an arrow pressed on the screensaver moved its selection; not tried in the
simulator or on a remote), while a page that parks its focus when it doesn't own the input, as the Settings pages
do, gets nothing. Seen in the desktop simulator (2026-11-15, Minimal): OK on the screensaver over a Settings page
whose keypad focus was on a switch left the switch alone, while OK on the page itself toggled it. Besides Matrix,
where OK and the steering keys don't close it at all while "DPAD interactive" is on (the table above), ours differs
from that scenario in two things: the screensaver closes at the key's release, not at its press, and after a close
under a held key that key's repeats reach neither path (next section).

Checks after merging `src/qml/main.qml`, `src/main.cpp` or `src/hardware/hardwareController.cpp` (v1.6.5):
`python3 tools/check_undock_wiring.py`. No test loads those three files, and a line dropped there breaks the quick
undock check without a failing test. It pins, as text:

- `main.qml`: `import UndockCheck 1.0`; the `Binding` of `UndockCheck.screensaverOpen` to the screensaver's `opened`;
  the `Connections` whose `onCloseRequested` calls `root._closeScreensaverNow();`; `dockedRearmTimer` and
  `dockChimeGraceTimer` waiting (`restart(); return;`) while `UndockCheck.undockPending`, before they open the
  screensaver; the whole condition of the wake branch (see the Main window row above), with a body that opens the
  screensaver and stops `dockedRearmTimer`; exactly five assignments to `chargingScreenLoader.active` that are not
  `= false` (the idle timer, the two timers above, the dock handler, the wake branch), because a sixth place that
  opens the screensaver would need the same guard; and the Loader's own binding, `active: HwInfo.modelNumber === "DEV"`.
- `main.cpp`: the handlers of ScreensaverConfig's `closeOnWakeChanged` and `undockCloseMsChanged`, and the three
  pushes at start-up in this order: `setUndockCloseMs`, `onPowerSupplyChanged`, `setCloseOnWake` (the last one can
  start the check, so it must find the delay and the charger already set).
- `hardwareController.cpp`: `m_undockCheck = new UndockCheck(m_core, this);`, its connections to
  `Power::powerModeChanged`, `Battery::powerSupplyChanged` and `core::Api::disconnected`,
  `UndockCheck::closeRequested` connected to `PhantomWakeSuppressor::onQuickUndock`, the order (Power, Battery and
  the phantom-wake check are created before it, and it before those four connects) and the QML singleton
  registration.

A statement that is commented out counts as missing. It is a text check, so a harmless reformat of these lines trips
it too: then compare with the list at the top of the script and update the script. What it cannot see, because it
doesn't run the app: a statement that is there but not in force (`enabled: false` on the `Connections`, another
condition around the close in `onCloseRequested`, C++ lines inside `#if 0`); a `!` in front of one of the two pieces
of the `Binding`'s value; an opener written another way (a `Binding` or a `State` on the Loader's `active`,
`chargingScreenLoader["active"]`, the Loader reached under another name or from another file); and anything in other
files. That `ScreensaverConfig.reopensOnWake()` can be called from QML at all is a unit test
(`test/screensaver/test_screensaver_config.cpp` calls it from JavaScript).

After a merge that touches these lines, a run on a remote, or in the desktop simulator behind a relay that plays the
charger reading, is still the proof, and it takes more than a lift. A lift shows the `Binding` and the close, with
the pushes and connects they need. The wake branch needs a wake from the dark while that lift is unconfirmed
(`Wake on the charger: not re-opening the screensaver, the quick undock check is waiting for the core's charger report`).
The connect to `PhantomWakeSuppressor::onQuickUndock` shows in a pick-up from the dark with "Suppress phantom
wake-ups" on and "Grace window after wake" longer than the close takes (2000 ms in the simulator run):
`PhantomWakeSuppressor cancelled: off the charger (quick undock check)`. `dockedRearmTimer`'s wait needs a close the
core never confirms (for example a lift and quick put-back) with "Run after dismissal while docked" at 5 s, and a
remote: that timer never starts with `UC_MODEL=DEV`. `dockChimeGraceTimer`'s wait has been exercised by no test,
simulator run or remote.

`python3 tools/check_undock_wiring.py --self-test` shows that each of its checks can fail (51 broken pieces, over
the 22 kinds of message it can give); CI runs both (job "Quick undock check wiring", see the validation checklist).

### Key navigation (v1.6.5)

In upstream's [`key-navigation`](../openspec/specs/key-navigation/spec.md) spec the input layer consumes no key
event, and a key's release goes to the owner that got its press. Upstream's code sends a key's auto-repeat presses to
that owner too, and both to the item in front once that owner is destroyed; `main.qml` destroys our screensaver right
after it closes. So when something else closed the screensaver under a key that was still down, the page behind got
the rest of that key. Since v1.6.5 `ChargingScreen.qml`'s `onClosed` hands the keys the screensaver still has down to
`InputController::dropPendingKeys()` (the Input row above), and for those keys, until their release, ours departs
from these requirements. Every other key event is routed as upstream routes it.

| Spec | Requirement | For a key that was down when the screensaver closed |
|---|---|---|
| `key-navigation` | "Physical button set and key mapping", scenario "Mapped key press": the button name is emitted together with the current input owner, and the event is never consumed by that layer | An auto-repeat press is announced to no owner and is consumed. The release is announced with the controller itself, which is no input owner. |
| `key-navigation` | "Every key press travels two independent paths": the input-ownership path and the QML focus chain, and a handler on the first shall not suppress the second | An auto-repeat press travels neither: without that, the focused control of the page behind takes it for a press (a Switch, Button or Checkbox acts on every Return that reaches it). The release's Qt event still travels the focus chain. |
| `key-navigation` | "Button-navigation handler dispatch", scenario "Override active": the handlers of an `overrideActive` instance run regardless of which layer owns the input | No handler runs for an auto-repeat press, those of `overrideActive` instances included. They do get the release (`main.qml`'s VOICE handler has to see it). |
| `key-navigation` | "Short press, repeat, long press and release semantics": `pressed` runs on the first press and on every auto-repeat press | Not on these auto-repeat presses. |
| `key-navigation` | "Key release routing and deferred auto-repeat release": the release is delivered to the owner that received its press, even when the input ownership changed in between | It goes to no owner: the one that got the press is the closing screensaver. The 150 ms deferral of a release flagged as auto-repeat is unchanged. |
| `key-navigation` | "Keypad-active state and selection rendering": the keypad is active from the first mapped key press until the next touch | An auto-repeat press after a touch doesn't make it active again: after a tap closed the screensaver under a held key, the selection of the page behind stays hidden and doesn't move. |
| [`entity-detail-controls`](../openspec/specs/entity-detail-controls/spec.md) | "Light keys on the control screen": they stay active while a popup is open on top of the screen; scenario "Holding DPAD_UP": the brightness rises by 1 per key repeat | While the screensaver shows over a light's page, as upstream says. When it closes under the held key, the rise stops; the next press steps again. |
| [`power-and-battery`](../openspec/specs/power-and-battery/spec.md) | "Charging screen", scenario "Key press on the charging screen": the key also reaches the keyboard focus chain of the screen underneath | The press does (see the screensaver section above); the auto-repeat presses after a close under the held key don't. |

Upstream's `docs/key-navigation.md` (§1: the controller "never consumes the event") and ADR 0007 say the same as the
first two rows; those files stay upstream's, so this table is where the departure is recorded.

Limits, by design: a key pressed once the screensaver is gone is the page's in full; a key held on the page since
before the screensaver opened stays the page's; a hold whose plain press the controller never got (input blocked at
that moment, or a press the firmware didn't pass to the UI) is not recorded, so the screensaver takes its first
auto-repeat for a press, nothing is dropped at the close and the page gets the rest; VOICE held on the screensaver
over an activity screen starts no assistant: `main.qml`'s handler stands back while an activity is open, and the
activity's own VOICE handler is scoped, so it gets nothing of that hold (pinned by
`test_heldVoiceKeyStartsNothingOnAPageWithItsOwnVoiceHandler`; up to v1.6.4 its assistant started about 1.4 s after
the press, worked out from the code; at a merge re-read upstream's `voice-assistant` requirement "Push-to-talk");
and a mark left by a release that was lost consumes the repeats of a later hold of that key whose press was lost
too, until its release (two lost events in a row).

Checks after merging `src/ui/inputController.{h,cpp}` or `src/qml/components/ChargingScreen.qml`:
`python3 tools/check_dropped_keys_guard.py`. A merge that drops all of this change there, or only the block in
`eventFilter()` or the `onClosed` line, still builds. Without `dropPendingKeys()` or without the `onClosed` line the
page behind then gets the rest of a held key again; with only the block gone its scoped handlers still get nothing,
but the handlers that listen to every owner take each repeat for a press and the repeats go on to the QML focus
chain. It pins, as text:

- `inputController.h`: `Q_INVOKABLE void dropPendingKeys(QObject* owner);` in the public part of the class (QML
  calls it, and QML doesn't see a private method);
- `inputController.cpp`: `dropPendingKeys()` pointing the entries of `owner` in `m_keyOwner` at the controller; in
  the KeyPress branch of `eventFilter()`, the block
  `if (keyEvent->isAutoRepeat() && m_keyOwner.value(key).data() == this) {` after `cancelDeferredRelease(key);` and
  before `setKeypadActive(true);`, with `emit keyPressed(mappedKey);`, `event->accept();` and `return true;`, and
  without a `keyPressedFor`; and upstream's own lines the design relies on: `m_keyOwner[key] = owner;` (a new press
  overwrites the mark), `QPointer<QObject> owner = m_keyOwner.take(key);` and
  `emit keyReleasedFor(owner.data(), mappedKey);` (the release takes the mark and goes out with it), and
  `timer->setInterval(150);` (the C++ test has a copy of that time);
- `ChargingScreen.qml`: the `onClosed` that gives the input back, with
  `if (buttonNavigation) buttonNavigation.releaseControl();`, also holds
  `if (buttonNavigation) ui.inputController.dropPendingKeys(buttonNavigation.scope);` (the order of the two isn't
  checked: the keys stay the popup's until it is destroyed).

A statement counts only as live code at the start of a line, and a reformat that breaks one of these lines up trips
the check too: then compare with the two tests and update the script. What it doesn't look at, because the tests
do: the first lines of `dropPendingKeys()` (a null owner and the controller itself are left alone), the rest of
`emitKeyRelease()`, the two log lines and the 3 s hold of POWER. The tests are
`test/screensaver/test_input_controller_dropped_keys.cpp` (`testInputControllerDroppedKeys`, the real controller, in
CI's CMake job) and `test/qml/tst_screensaver_keys.qml` (the real `ChargingScreen.qml` and `ButtonNavigation.qml`
over the real controller, in CI's QML job). In the first, `destroyedOwner_upstreamSendsItsKeysToTheItemInFront`
states upstream's own routing, the rule the drop exists for: if a merge makes it fail, upstream changed that rule,
and `dropPendingKeys()` and this section need a new look.

`python3 tools/check_dropped_keys_guard.py --self-test` shows that each of its checks can fail (23 doctored copies of
the three texts, each one reported, on the tree as it is and on a copy with harmless edits, which the check accepts);
CI runs both (job "Dropped keys guard", see the validation checklist).

### Touch slider (v1.2.0, v1.4.7, v1.6.4)

Upstream's [`touch-slider`](../openspec/specs/touch-slider/spec.md) spec knows no screensaver. With the defaults
the screensaver opens on the dock, so this applies with the default settings:

| Requirement | Ours |
|---|---|
| "Gesture to value mapping": while pressed the target is sent every 200 ms if it differs from the entity's value; on release once more if it still differs. "Slider overlay": it opens on press and closes 1 s after release | While the screensaver shows (`applicationWindow.screensaverActive`, from the moment its Loader is active), the touch slider belongs to it (the Matrix speed; on Starfield the Animation speed, the Star density or nothing, as its Touchbar row picks, since v1.6.4). A press there starts no gesture: nothing is sent and no overlay opens (v1.4.7). A gesture under way when the screensaver opens ends at once: the target is sent once if it differs and the entity is available, the 200 ms sending stops, and the overlay closes without the 1 s wait (v1.6.4). |
| "Gesture to value mapping": on release the target is sent once more if it still differs | Only for a gesture that is still under way (v1.6.4). The release sends nothing after a gesture the screensaver ended, after a refused press (an entity that was unavailable at the press and available at the release, or a press under the screensaver that closed before the release), and in an overlay loaded after the press (the entity's features arriving late); the overlay still closes 1 s after the release. Upstream's code sends the slider's last target then (0 if the slider never had one). |

Checks after merging `TouchSlider{Volume,Seek,Brightness,Position}.qml`: `python3 tools/check_touchslider_guards.py`
and `test/qml/tst_touch_slider_screensaver.qml` (see the validation checklist).

## Deviations from upstream's ADRs

Upstream records its architecture decisions in [`docs/adr/`](adr/). Where our code departs from one, as listed so far.
Not listed yet: [0005](adr/0005-core-api-over-websocket-ui-holds-no-business-logic.md) (Mod 5's REST calls to
`/api/system/power/standby_inhibitors`, and since v1.6.5 the quick undock check's `GET /api/system/power/battery`, a
REST read of state the battery_status event reports up to 5 s later),
[0016](adr/0016-the-ui-touches-as-little-hardware-as-possible.md) (`activitySessionKeeper`, `phantomWakeSuppressor`,
`screenOffSync` and `undockCheck` in `src/hardware/`), and 0012 for the
screensaver's own decisions in `main.qml` and `ChargingScreen.qml` (among them the touch slider control: the slide
position and rest, the 10..100 clamp, the switches that enable it and which value Starfield's Touchbar choice
changes; its travel-to-steps rule, and the rule that a stored Touchbar value other than "speed" and "off" reads as
"density", are in C++, `screensaverTouchbar.h`). What a lone tap does, what each key does, the direction the
screensaver opens with and which theme names count as Matrix moved to C++ in v1.6.5 (`screensaverInputPolicy.h`, asked
through `ScreensaverConfig.loneTapAction()`, `keyAction()`, `keyDirection()`, `directionToRestore()` and
`matrixLoaded`). Also since v1.6.5, whether a wake on the charger opens the screensaver again is asked in C++
(`reopensOnWake()` in `screensaverClosePolicy.h`, through `ScreensaverConfig.reopensOnWake()`), while the two QML
timer guards for the same wait decide in QML: `dockedRearmTimer` and `dockChimeGraceTimer` read
`UndockCheck.undockPending` themselves and restart instead of opening the screensaver. What happens to a key that is
still down when the screensaver closes (v1.6.5) is decided in C++ as well: `ChargingScreen.qml`'s `onClosed` only
calls `InputController::dropPendingKeys()`.

| ADR | Ours | Why |
|---|---|---|
| [0012](adr/0012-qml-is-presentation-only-logic-and-models-in-cpp.md) QML is presentation only | The touch slider's screensaver rules decide in QML: the press guard (since v1.2.0), the release guards (since v1.4.7; the no-slide guard since v1.6.4) and the handler that ends a slide under way when the screensaver opens (since v1.6.4), in the 4 `TouchSlider{Volume,Seek,Brightness,Position}.qml` | They extend upstream's own slider logic, which lives in those 4 upstream QML files; moving it to C++ would rewrite them and conflict at every upstream change to them. Pinned by `test/qml/tst_touch_slider_screensaver.qml` and `tools/check_touchslider_guards.py`. Better offered upstream. |
| [0007](adr/0007-one-input-idiom-per-screen.md) Two input paths, input ownership stack | "The design does not try to make one path consume the other": since v1.6.5 the input controller consumes the auto-repeat presses of a key that was still down when the screensaver closed (`InputController::dropPendingKeys()` and its block in `eventFilter()`), so they reach neither path; see "Key navigation (v1.6.5)" above | Without it the page behind acts on the rest of a key that was pressed on the screensaver: its scoped handlers get the repeats and the release, and its focused control takes a repeat for a press. Only those repeats are consumed; no other key event is kept from either path (the release of such a key is announced with the controller as its owner, see the table above). A rule for every destroyed owner would change upstream's own flows and is better offered upstream. Pinned by `test/screensaver/test_input_controller_dropped_keys.cpp`, `test/qml/tst_screensaver_keys.qml` and `tools/check_dropped_keys_guard.py`. |
| [0011](adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md) CMake builds the tests | `test/qml` (the QML suite), `test/integration` and our six `test/hardware/*_test` programs build with qmake; `test/matrixrain` builds with both (qmake in our job, CMake in the upstream job) | Their qmake projects predate the ADR (2026-09-24). Programs and tests added since joined them: `power_test` (v1.5.3), `screenoff_test` (v1.6.2), `undock_test` (v1.6.5) and new QML tests such as `tst_touch_slider_screensaver.qml`. CI builds them with qmake as before (`test.yml`). `test/screensaver` is our only suite built with CMake alone. |

## Merge rehearsal procedure

Never merge upstream directly into `main`. Always rehearse first:

```shell
git fetch upstream
git checkout -b rehearsal/upstream-$(date +%Y-%m-%d)
git merge upstream/main
```

### When conflicts hit

For **every** file in the conflict-surface table above:

1. **Upstream side (theirs):** new logic, bug fixes, API changes — preserve.
2. **Our side (ours):** all custom screensaver code — preserve.
3. **Mutual-exclusion cases** (e.g. upstream renamed a property we're using): inspect carefully, migrate our usage to the new API, update CUSTOM_FILES.md entry if the file path changed.
4. **Never reformat upstream code** in the resolve. Match upstream's indentation/style inside their blocks even if it disagrees with our `.clang-format`. Minimize the diff against upstream.

### When a file in the table doesn't conflict

Still inspect it — upstream may have touched nearby code that subtly breaks our custom logic without triggering a textual conflict. Example: upstream changes a signal signature, our custom handler keeps compiling but silently ignores the new argument.

### New upstream files we haven't touched

Pass through cleanly — no action needed.

### When upstream has rewritten its history

Upstream rewrote its history at v0.74.5: none of its commits after that point descend from the commit our previous
merge used (v0.73.5, `3b46959`), so `git merge-base` falls back to v0.38.4 and a merge reports thousands of conflicts.
What worked for v0.84.0 (v1.6.0):

```shell
# 1. Find upstream's rewritten counterpart of our previous base: the same release subject in the new history.
#    For v0.84.0: 4077183 "v0.73.5 changes" (parent bc72137, the rewritten v0.73.4); our previous base is the
#    old 3b46959 "v0.73.5 changes". The trees differ: the rewrite changed files as well.
git log --format="%h %p %s" refs/upstream-public/v0.84.0 | grep "v0.73.5 changes"
# 2. Graft our previous base onto it as an extra parent, so the merge base becomes our previous base
git replace --graft 4077183 bc72137 3b46959     # <counterpart> <its own parent> <our previous base>
git merge-base HEAD refs/upstream-public/v0.84.0  # must print 3b46959 (without the graft: f06f0f0, v0.38.4)
#    Upstream's changes between its v0.73.5 and the new release then arrive as one ordinary merge; differences the
#    rewrite made inside v0.73.5 itself show up as changes too, so review the conflicts with that in mind.
# 3. Merge, resolve, commit
git merge refs/upstream-public/v0.84.0
# 4. Delete the graft right after the merge commit; it must not stay in the repository
git replace -d 4077183
```

After the merge commit the merge base with upstream is upstream's own commit (`781882a` for v0.84.0), so the next
merge needs no graft, unless upstream rewrites its history again.

## Validation checklist

After the merge, before merging the rehearsal branch back into `main`:

- [ ] **ARM64 build** with the digest-pinned toolchain (`BUILD.md`): 0 errors; the QML compiler runs on every file
      (delete the `qrc` caches of an incremental build first)
- [ ] **Tests**, all in a Qt 5.15 container (`test.yml` has the commands): `test/hardware` (keeper, suppressor,
      entity controller, power, screen-off sync, quick undock check), `test/matrixrain`, `test/integration`, `test/qml`, and the CMake
      suite in `test/` (upstream's programs plus our `test/matrixrain` and `test/screensaver`); CI's four quick text
      checks, `python3 tools/check_setPowerMode_drift.py`, `python3 tools/check_touchslider_guards.py`,
      `python3 tools/check_undock_wiring.py` followed by `python3 tools/check_undock_wiring.py --self-test`, and
      `python3 tools/check_dropped_keys_guard.py` followed by `python3 tools/check_dropped_keys_guard.py --self-test`
- [ ] **Code guidelines** as CI runs them: `python3 tools/design-check.py --self-test && python3 tools/design-check.py`,
      `python3 tools/icon-font.py check --require-free`, `python3 tools/icon-font.py check-mapping`, cpplint
      (filters in `code_guidelines.yml`); the docs link check (`docs-links.yml`)
- [ ] **Desktop simulator** (`UC_MODEL=DEV`, `docs/static-compile.md`): every settings page walked with the keys,
      top to bottom and back; QML log free of binding loops and TypeErrors from our files
- [ ] **On the remote** via `scripts/deploy-canary.sh` (auto-revert on failure):
  - screensaver: docking opens it and plays the dock chime once; a double tap and BACK / HOME close it, also when the
    UI starts on the dock (it then opens after 2 s, without a chime); "Close on wake": Never keeps it when you lift
    the remote, Undocked keeps it on a wake on the dock and closes it soon after a lift (since v1.6.5, while it is lit
    or dimmed, the quick undock check closes it before the core's report and logs
    `Close on wake (Undocked): off the charger for at least N ms, 3 "off" answers in a row (quick undock check, Undock delay 500 ms), closes the screensaver`,
    N being 500 or more; at the default delay about 1–2 lifts in 10, for a lift at a random moment, close at the
    core's own report instead, when its battery check comes first (worked out: the quick check's 0.55–0.85 s over
    the core's 5.004 s; not counted on a remote), but if every lift closes only at the core's report, up to 5 s after
    the lift, that check isn't working), Always closes it on a wake; all five themes render
  - settings: Screensaver, Power, Sound, User interface, WiFi pages walked with the d-pad
  - first button press after a wake works (no backlight blinking); keep awake holds during playback
  - a button held when something else closes the screensaver (v1.6.5): "Close on wake" on Always, the remote on the
    charger with a Settings sub-page behind the screensaver; within a minute of the display going off, hold BACK for
    about 2 s: the screensaver closes, the page stays where it was, and the log has
    `Key(s) still down when their owner closed: BACK; the rest of each press is ignored (owner …)`; holding POWER
    for 3 s on the dark screensaver still brings the power-off dialog (both run on a remote with test build
    1.6.4-keys2, 2026-11-26)
  - touch slider: on the dock, with "Run after dismissal while docked" at 5 s, slide slowly until the screensaver
    comes back: the device stops changing as it appears, and slides under it change only the screensaver
  - media browser: play an item, its artwork shows at once
  - Messages (Settings → User interface): with the defaults a failed command opens upstream's pop-up; with "Show
    error pop-ups" off it shows the banner for the "Error banner duration" and the buttons keep working; with "Show
    offline banners" off a tile of an unavailable device shows no banner, while other banners (a wrong admin PIN,
    for one) still show; walked with the d-pad, each row's title and help text are fully on screen
- [ ] **Translations**: `en_US.ts` updated with lupdate; the other `.ts` files keep upstream's text (ADR 0006)

## Rollback procedure

### Rollback the merge locally (before pushing)

```shell
git checkout main
git branch -D rehearsal/upstream-<date>
# merge never happened — main is untouched
```

### Rollback after pushing to `origin/main`

```shell
# Find the last known-good commit (before the merge)
git log --oneline

# Hard reset main
git checkout main
git reset --hard <last-good-sha>

# Force-push — ONLY if no one else has pulled the bad merge
git push --force-with-lease origin main
```

**Do not** force-push main without first confirming nothing downstream has pulled the bad merge. `--force-with-lease` protects against overwriting unknown concurrent pushes but doesn't protect against collaborators who already pulled.

### Rollback on a deployed UC3

If the bad build is already on a device:

```shell
source .env.local
curl -X PUT "http://${UC3_HOST}/api/system/install/ui?enable=false" \
    -u "${UC3_USER}:${UC3_PIN}"
```

This disables our custom UI and the device reverts to the stock UC firmware — full backstop.

## Maintenance rules

Update this doc whenever:
- A new upstream file is modified for a new mod (add row to the table)
- A mod makes the app behave differently from a requirement in upstream's specs (add it to "Divergence from
  upstream's specs")
- A custom file is deleted or replaced (remove/update row)
- The validation checklist changes (new regression tests, new features)
- The rollback procedure changes (new API endpoint, new branch structure)

Keep the conflict-surface table in sync with [CUSTOM_FILES.md](CUSTOM_FILES.md) — that file lists all custom files; this doc lists only the subset that overlap with upstream and will conflict on merge.

## Known upstream base

Current fork base: **upstream `v0.84.0`** (`781882a`, the UI of firmware 2.12.0), merged in v1.6.0. Upstream tags
are fetched into `refs/upstream-public/` (the `upstream` remote fetches branches only). Next upstream release bump
requires walking through this doc end-to-end.

## Rehearsal history

Log each rehearsal here so "last known merge state" is always at a glance.

### 2026-10-31 — v0.84.0 merge (UI of firmware 2.12.0) → shipped as v1.6.0

- **Trigger:** the owner's remote runs firmware 2.12.0 with UI v0.84.0; the fork was on v0.73.5 (v1.5.2) plus five
  backports (v1.5.3).
- **History rewrite:** upstream rewrote its history at v0.74.5; merged with a temporary graft (see "When upstream has
  rewritten its history"). Merge commit `94c4006` (parents: our v1.5.4 `6649035`, upstream v0.84.0 `781882a`).
- **Conflicts:** resolved file by file from four conflict maps (C++ platform, entity layer, settings / WiFi QML,
  main / build / icons). Kept: every mod. Dropped as superseded or dead: Wake-replay HUD (owner: UC's retry
  spinners), our battery chip (UC's `TitleStatus`), WiFi W5/W9 (upstream fixed the causes), our WebSocket keep-alive
  (upstream's spec forbids it), the REST auth probe (it logged the PIN), `migrateLegacySettings()` (never worked).
  Adopted upstream's power-mode contract (an unchanged mode is not announced again).
- **After the merge:** our screens moved onto upstream's design system (screensaver settings, WiFi diagnostics,
  shared `ChipPicker`); browse-thumbnail preview re-implemented on the 0.74 artwork pipeline; keep awake follows only
  players a screen asked for (since 0.74 every configured entity is loaded at start-up); `MainContainer.qml` back to
  upstream (our start-up retry timer took the buttons from the screensaver).
- **Validation:** 0 errors on the ARM64 build; all suites pass (hardware 106, matrix rain 106, integration 41, QML 212,
  upstream 31 programs); design check 0 problems; desktop simulator walks of every theme; on-device check by the owner.

### 2026-04-23 — v0.72.0 merge (first real upstream-advance under this playbook) → shipped as v1.4.0

- **Trigger:** upstream `unfoldedcircle/remote-ui` advanced `main` with commit `c76ff05` (subject: `v0.72.0 changes` — UC no longer git-tags releases; version lives in commit subject only).
- **Fetch result:** `git fetch upstream --tags` — 1 new commit on `upstream/main` since the 2026-04-13 rehearsal.
- **Divergence (pre-merge):**
  - `git rev-list --count HEAD..upstream/main` → **1 commit behind upstream**
  - `git rev-list --count upstream/main..HEAD` → **135 commits ahead of upstream**
  - Merge base: `0586d45` (our old upstream pin = `v0.71.1`)
- **Upstream scope:** 12 files, +433/-93 lines. Three functional changes: (a) media browse press-and-hold + new shuffle/repeat/browse/sources controls row in `MediaComponent.qml`, (b) smarter media-browse error handling in `MediaBrowser.qml`, (c) **UC independently shipped the same feature as our Mod 3** ("Show battery indicator everywhere") with different property name + layout approach.
- **CI red-herring:** upstream's own aarch64 CI for `c76ff05` failed — investigation revealed it was a GitHub Actions deprecation (`actions/upload-artifact@v3` sunset), not a code defect. Our workflows already use `@v4`. Source compiles cleanly on our Docker toolchain.
- **Conflict surface:** 5 files with real conflicts (`config.h`, `config.cpp`, `BaseTitle.qml`, `Activity.qml deviceclass`, `Ui.qml`, `en_US.ts`). `config.h` auto-merged with both our `showBatteryOnDetailPages` AND upstream's `showBatteryEveryWhere` coexisting — required manual removal of our leftover decls.
- **Resolution strategy (Option B rebase):** adopt upstream's public API (`showBatteryEveryWhere`, QSettings key `ui/batteryEveryWhere`, upstream's Settings toggle wording) while keeping our superior Option A chain-anchoring `RowLayout` in `BaseDetail.qml`. Reject upstream's inline battery `Row` in `BaseTitle.qml` / `Activity.qml` (would duplicate the chip we already render via `BatteryStatusChip.qml` Loader). Accept-theirs on MediaComponent, MediaBrowser, SelectWidget, SensorWidget, icons, translations.
- **QSettings migration:** one-shot `migrateLegacySettings()` helper added to `src/main.cpp` — carries legacy `ui/batteryOnDetailPages` value (v1.3.0 default `true`) forward into `ui/batteryEveryWhere` (upstream default `false`). Ensures v1.3.0 users who accepted the default chip-visible state don't silently lose it on upgrade.
- **Icons:** initial Explore-agent read flagged that upstream removed `uc:heat` / `uc:brightness` / `uc:list` / `uc:bluetooth` / `uc:battery-low`. Independent verification showed this was a dedup — upstream's JSON had duplicate keys across primary + aliases sections; only the aliases-section copies were removed. All 5 icon names still resolve post-merge. Two icons (`uc:list` / `uc:heat`) get different glyphs (visual change only).
- **Branch workflow:** merged on `rehearsal/upstream-2026-04-23`; rollback tag `pre-v0.72.0-merge-2026-04-23` set on main pre-merge. Promoted via `git checkout main && git merge --no-ff rehearsal/upstream-2026-04-23` + `git tag v1.4.0` + `git push origin main v1.4.0`.
- **Validation checklist outcome:** canary deploy via `scripts/deploy-canary.sh` healthy at elapsed 0s. On-device smoke test: media browser works, battery chip works (confirming Mod 3 Option B rebase + migration), screensaver works, no errors in Logdy or `/api/system/logs`. About-screen UI version display shows stale `0.38.4-32-g1266974` instead of `1.4.0` — pre-existing upstream quirk unrelated to this merge (binary correctly compiled with `DAPP_VERSION=\"1.4.0\"`, but `softwareUpdate.h`'s inline `getUiVersion()` apparently isn't the source the screen reads from — out-of-scope investigation).
- **Divergence (post-merge):** 135 commits ahead (our custom work preserved) + the merge commit itself. Next upstream-advance will restart the counter.
- **Follow-up:** `v1.4.1` shipped same day — fixed 7 unguarded `volume.start()` call sites upstream (unblocks per-device `suppress_volume_overlay` toggles in integration drivers). See `_build_logs/2026-04-24_v1.4.1_osd_guards.md` for that log.
- **Conclusion:** first successful non-trivial upstream merge under this playbook. Option B rebase pattern established for future merges where upstream and our fork collide on feature semantics.

### 2026-04-13 — safety re-check before broader distribution

- **Trigger:** Maintainer asked "should we contrast our build with upstream before anyone else's UC3 installs it?" — quick re-check to confirm upstream hasn't moved since the Batch G rehearsal earlier in the day.
- **Fetch result:** `git fetch upstream` — no new commits on `upstream/main` since the earlier rehearsal.
- **Divergence:**
  - `git rev-list --count HEAD..upstream/main` → **0 commits behind upstream** (unchanged)
  - `git rev-list --count upstream/main..HEAD` → **91 commits ahead of upstream** (+9 from this session's batch 0 fixes, avatar cleanup, button-lockout fix, doc refreshes, CI tidy fix, test determinism fix)
  - `git merge-base HEAD upstream/main` → `0586d45b3ee7a04d2f1a15d9e4b2606c24d7ae08` (unchanged)
  - Upstream `HEAD` commit: `0586d45 v0.71.1 changes` (unchanged since the earlier rehearsal)
- **Conclusion:** Fork is still a strict superset of `upstream/main`. No new signal renames, no new upstream bug fixes missing from our tree. Safe to distribute from the "upstream deltas" angle. Real blast-radius concerns for non-maintainer devices (firmware/hardware revision mismatch, untested code paths) are orthogonal to this check and are documented in the new Install warnings in `README.md` and `SCREENSAVER-README.md`.

### 2026-04-13 — Batch G #17 initial rehearsal

- **Trigger:** Batch G #17 of the path-to-A plan (first rehearsal under this playbook).
- **Fetch result:** `git fetch upstream` — no new commits on `upstream/main` since our fork point.
- **Divergence:**
  - `git rev-list --count HEAD..upstream/main` → **0 commits behind upstream**
  - `git rev-list --count upstream/main..HEAD` → **82 commits ahead of upstream**
  - `git merge-base HEAD upstream/main` → `0586d45b3ee7a04d2f1a15d9e4b2606c24d7ae08`
  - Upstream `HEAD` commit: `0586d45 v0.71.1 changes` (same commit as the merge-base)
- **Upstream tag state:** Latest upstream tag is `v0.38.4` (`f06f0f0`), which lags the upstream `main` commit. Upstream's `main` commit message claims "v0.71.1 changes" but they haven't pushed a `v0.71.1` tag yet — a mild inconsistency in the upstream repo, not our problem.
- **Dry merge:** `git merge --no-commit --no-ff upstream/main` on a throwaway `rehearsal/upstream-2026-04-13` branch → **"Already up to date."** — zero conflicts, zero files touched.
- **Branch cleanup:** rehearsal branch deleted cleanly (`git branch -D rehearsal/upstream-2026-04-13`).
- **Validation checklist outcome:** not run — there's nothing to validate when the merge is a no-op. When the next rehearsal has real upstream commits, work through the Validation checklist section above.
- **Conclusion:** Our fork is a strict superset of `upstream/main`. Next rehearsal should happen after upstream advances `main`; watch the upstream repo or their release cadence for triggering events.

### Template for future entries

```
### YYYY-MM-DD — <trigger description>

- **Trigger:** <why we ran the rehearsal>
- **Fetch result:** `git fetch upstream` — <N new commits / no change>
- **Divergence:** HEAD is <N commits behind> / <M commits ahead>
- **Conflicts:** <N files / none>
  - <file 1>: <conflict type + resolution>
  - <file 2>: ...
- **Dry merge / resolution:** <what happened>
- **Validation checklist outcome:** <pass / fail + notes>
- **Conclusion:** <merged to main / held / rolled back>
```
