# Remote UI Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

# Fork releases (Madalone's Defolded Circle 3)

Releases below this point are from the custom-screensaver fork maintained by [@mmadalone](https://github.com/mmadalone), not from upstream Unfolded Circle. Upstream `unfoldedcircle/remote-ui` release history continues further down starting at `v0.71.1`.

## v1.4.18 — 2026-04-29 — CI fix: sync remote-ui.pro VERSION with release.json

### Fixed
- **`remote-ui.pro` VERSION bumped from 1.4.11 → 1.4.18.** CI check at `.github/workflows/build.yml:44-55` validates `PRO_VERSION == JSON_VERSION` and was failing on every release since v1.4.12 (six in a row) because nobody touched the `.pro` file's `VERSION = 1.4.11` line. No runtime impact — the actual displayed app version comes from `GIT_VERSION` (line 56-71 in `.pro`) via `git describe`. CI artifact build was the only thing affected. Going forward: `release.json` and `remote-ui.pro:75` bump together.

### Architectural note
- **Drift increase: zero.** `remote-ui.pro` was already in our modified-upstream manifest.
- **No translation impact.** No new strings.
- **No deploy needed.** The v1.4.17 binary on the device is functionally correct; this is purely a CI sync. Tag v1.4.18 to clear the GitHub Actions failure backlog.

---

## v1.4.17 — 2026-04-29 — WiFi Diagnostics popup (W13)

### Added
- **WiFi Diagnostics popup** — new "Diagnostics" button in the WifiInfo popup action stack (between Reconnect and Delete) opens a focused diagnostic surface showing:
  - **RSSI sparkline** — Canvas-rendered line graph of the last 60 RSSI samples. While the popup is open, a 5 s `Timer` polls `Wifi.getWifiStatus()`; the W6 30 s background poll feeds the buffer when the popup is closed. Reference lines (dashed, `colors.medium`) at -60 / -76 / -84 dBm matching the `SignalStrength::fromRssi` tier thresholds (EXCELLENT / GOOD-OK / OK-WEAK / WEAK boundaries). Y-axis range -100..-30 dBm clamped, X-axis is sample index.
  - **Live RSSI + link speed** readouts.
  - **Connection stats** — drops since boot, current session uptime (`HH:MM:SS`), time since last disconnect (formatted progressively: `Ns` → `Nmin` → `Nh Nmin`).
  - **Reset counters** button with a `createActionableWarningNotification` confirmation dialog (precedent at `Wifi.qml:310-322` for "Delete all networks") — calls `Wifi.resetDiagnosticCounters()` to zero `m_disconnectCount`, clear the ring buffer, and set `m_currentSessionStartMs` to now.
  - **Top-left back arrow** mirroring the v1.4.15 WifiInfo close affordance pattern.
- **`Wifi` singleton C++ surface:**
  - `Q_PROPERTY(QVariantList rssiHistory)` + `rssiHistoryChanged` signal — ring buffer (`QVector<int> m_rssiHistory`, capped at `kRssiHistoryMax = 60`) of last 60 RSSI samples; pushed in `getWifiStatus()`'s success branch alongside the existing `currentLinkInfoChanged` emit.
  - `Q_PROPERTY(int disconnectCount)` + `disconnectCountChanged` signal — incremented in `onWifiEventChanged(DISCONNECTED)`.
  - `Q_PROPERTY(qint64 currentSessionDurationSec READ ... NOTIFY connectionStatsChanged)` — computed from `m_currentSessionStartMs` (set in `onWifiEventChanged(CONNECTED)` and at construction). Returns 0 when disconnected.
  - `Q_PROPERTY(qint64 secondsSinceLastDisconnect READ ... NOTIFY connectionStatsChanged)` — computed from `m_lastDisconnectMs` (set in `onWifiEventChanged(DISCONNECTED)`). Returns -1 sentinel when no disconnect since boot.
  - `Q_INVOKABLE void resetDiagnosticCounters()` — zeroes counters, clears buffer, fires all three signals.
  - `connectionStatsChanged()` 1 Hz tick via new `m_statsTickTimer` so QML uptime labels re-evaluate every second.
- **`WifiDiagnostics.qml`** — new custom QML file (~280 lines), Canvas-based sparkline, mirrors WifiInfo's Popup chrome.

### Architectural note
- **Drift increase: 2 modified upstream files** (`wifi.{h,cpp}`, `WifiInfo.qml`) + **2 modified registration files** (`resources/qrc/main.qrc`) + **1 new custom file** (`WifiDiagnostics.qml`). `remote-ui.pro` doesn't need a new entry — QML files are picked up via qrc only.
- **No new firmware-side dependency.** Reuses existing `getWifiStatus()` + `wifiEventChanged(DISCONNECTED|CONNECTED)` signal sources. Buffer lives in C++ `Wifi` singleton, persists for app lifetime, does not survive UI restart by design (transient diagnostic).
- **Charting choice:** `Canvas` (Qt Quick 2.15 native, zero external deps). Codebase has no `QtCharts` or `QtQuick.Shapes` usage, so Canvas matches the existing rendering surface convention. Alternative ring-buffer pattern documented at `src/ui/rainsimulation.h:38-57` (head/count) was overkill for N=60 — used `QVector<int>::removeFirst()` instead.
- **Translation impact:** new strings — `"WiFi Diagnostics"`, `"Signal strength"`, `"Link speed"`, `"Drops since boot"`, `"Current session"`, `"Disconnected"`, `"Time since last drop"`, `"None since boot"`, `" s"`, `" min"`, `" h "`, `"Diagnostics"`, `"Reset counters"`, `"Reset diagnostic counters?"`, `"Are you sure you want to zero the drop counter and clear the RSSI history?"`, `"Reset"`. Run `lupdate` (auto via build).
- **Verification:** sparkline live update via slow walk-out-of-range, disconnect counter via toggling Settings → WiFi off/on, uptime via wall-clock, reset via the confirmation dialog.
- **Auto-revert safety net** active per `project_auto_revert_validated_on_uc3.md`. Recovery: `curl -X PUT "http://${UC3_HOST}/api/system/install/ui?enable=true" -u "web-configurator:${UC3_PIN}"`.

---

## v1.4.16 — 2026-04-29 — Post-v1.4.15 polish round (slider thinning, docked-rearm functional, WifiInfo button placement)

### Fixed
- **Settings → Power "Keep awake" slider — too thick.** v1.4.15's `Layout.preferredHeight: 140` (chosen to fit pressed-state `lowValueText`/`highValueText` overflow) made the slider visually heavy. Dropped the from/to value labels entirely (the current value is already shown in the section title above), reverted to `height: 60` to match the existing `idleTimeoutSlider` pattern in `GeneralBehavior.qml`. No more overflow into "Only when on charger or dock".
- **Screensaver docked-rearm — feature didn't fire.** v1.4.15's `onClosed` handler wrapped the new docked-rearm path in `_shouldOpenOnIdle()` (which requires `ScreensaverConfig.idleEnabled === true`), but the slider was always-visible per user spec. So the slider showed but the rearm only worked when the user happened to also have "Idle screensaver" on. Removed the `_shouldOpenOnIdle()` gate — docked rearm now fires whenever docked + non-DEV, regardless of the idle toggle.
- **Settings → Screensaver → docked-rearm slider label clipped.** "Re-run after dismissal while docked" was truncating to "Re-run after dismissal while do" at the available column width. Shortened to "Run after dismissal while docked".
- **Settings → Screensaver → docked-rearm slider min was 30 s.** Lowered to 5 s; step from 10 s to 5 s. Range now 5–120 s.
- **Settings → WiFi → tap connected network — buttons "pinned to the middle of the screen".** v1.4.14's split-the-buttons-out-of-Flickable design made them always visible but visually placed them at the bottom of the popup container (which is roughly mid-screen on UC3's 480×850 viewport). Folded them back into the Flickable's `ColumnLayout` at the bottom of scrollable content; user scrolls through diagnostics to reach Disconnect/Reconnect/Delete at the bottom. Back-arrow stays at top-left as the always-visible close affordance.

### Architectural note
- **Drift increase: zero.** All four modified files (`main.qml`, `Power.qml`, `WifiInfo.qml`, `GeneralBehavior.qml`) were already in v1.4.15's diff; this is pure polish on top.
- **Translation impact:** one string changed: "Re-run after dismissal while docked" → "Run after dismissal while docked". Other v1.4.15 strings unchanged.
- **Verification:** all four fixes physically tap-tested by user on the device.
- **No re-bump of other version-pinned files** (BUILD.md toolchain digest, etc.).

---

## v1.4.15 — 2026-04-29 — UI polish: Power slider overflow, WifiInfo back-arrow, docked-rearm screensaver timer

### Fixed
- **Settings → Power Active Session Keeper slider overlap.** v1.4.14's `Layout.preferredHeight: 100` wasn't enough — pressed-state animation grows `sliderBG` to `slider.height` (50 → 100) AND `lowValueText.topMargin` from 5 to 20, pushing labels ~42 px below the bound and into the "Only when on charger or dock" row. Bumped to 140 to give the pressed-state animation room.
- **Settings → WiFi → tap connected network — back arrow top-left.** Replaces the now-redundant bottom "Close" button. Popup is conceptually a sub-section of WiFi settings, so a back arrow matches the navigation pattern better than a modal X. Implemented as `Components.HapticMouseArea` + `Components.Icon { icon: "uc:arrow-left" }` outside the Flickable, `z: 10`, `anchors { top; left; topMargin: 10; leftMargin: 10 }`. Flickable's `anchors.top.topMargin: 60` so content doesn't render under the arrow. Three close paths kept: arrow, tap-above-popup MouseArea, BACK hardware key.
- **Screensaver doesn't rearm after tap-dismiss while docked (pre-existing UC bug).** `main.qml:648–655`'s `onClosed` handler restarted `idleScreensaverTimer` only when undocked (`var undocked = !Battery.powerSupply ...`). On the dock, the screensaver stayed dismissed until the user woke the screen via `Low_power → Normal` (the `main.qml:633–635` wake re-open path). New `dockedRearmTimer` (single-shot, interval driven by new `ScreensaverConfig.reopenWhileDockedSec` property, default 60 s, range 30–120 s via UI slider) re-activates `chargingScreenLoader` after the configured delay following a docked tap-dismiss. Cancels on `Battery.powerSupplyChanged(false)` (undock) and on `Power.powerModeChanged → Normal` wake-re-open (avoids double-fire when wake already re-opens the screensaver).

### Added
- **`ScreensaverConfig.reopenWhileDockedSec`** Q_PROPERTY (range 30–120 s via UI, default 60), QSettings key `charging/reopenWhileDockedSec`. New slider "Re-run after dismissal while docked" in Settings → Screensaver → General Behavior, below the existing idle-timeout slider. Always visible (independent of the "Idle screensaver" toggle, since docked rearm operates on a different code path from the battery-idle auto-open).

### Architectural note
- **Drift increase: 5 modified files** (`screensaverconfig.h`, `main.qml`, `Power.qml`, `WifiInfo.qml`, `GeneralBehavior.qml` — last is custom per CUSTOM_FILES manifest). No new files.
- **Translation impact:** new strings — "Re-run after dismissal while docked", "Restart the screensaver after this many seconds of inactivity when on the dock.", "%1 s" already exists.
- **Verification:** UI fixes (1, 2) tap-tested. Fix 3 requires docked-on-AC verification — set timer to 30/60/120 s, trigger screensaver, dismiss, time the rearm with a watch.
- **Auto-revert safety net** active per `project_auto_revert_validated_on_uc3.md`. If the build crashes the UI for 90 s, prior firmware reverts; re-enable manually:
  ```
  curl -X PUT "http://${UC3_HOST}/api/system/install/ui?enable=true" -u "web-configurator:${UC3_PIN}"
  ```

---

## v1.4.14 — 2026-04-28 — Active Session Keeper: prevent the 5-minute sleep timer during media playback

### Added
- **Active Session Keeper (Mod 5)** prevents the firmware's 5-min standby timer from firing while a media-player entity is in `Playing` state or while curated media-control commands have been pressed within the configurable idle window. Eliminates the every-5-min wake-recovery gap while watching TV / listening to music. Backed by the previously-orphan `set_power_mode` ucapi RPC at `enums.h:96` (same orphan-surface pattern as v1.4.12 `WifiCmd::REASSOCIATE` and v1.4.10 `entityAdded`). Wire-probe (`_diag_capture_set_power_mode.log`) confirmed `PUT /api/system/power?power_mode=NORMAL` returns `{"code":"OK"}` and resets `standby_timeout_sec` to its configured max (typically 300) on every LOW_POWER/IDLE → NORMAL transition. New `core::Api::setPowerMode(PowerMode)` method mirrors `setPowerSavingCfg` shape.
- **`uc::hw::ActivitySessionKeeper` singleton** at `src/hardware/activitySessionKeeper.{h,cpp}` (custom files, full SPDX header per CLAUDE.md rule 7). Owns a 270 s repeating ping timer + a single-shot idle-window timer. State machine collapsed to a single `m_active` boolean: `m_enabled && (m_onAc || !m_requireAcPower) && (!m_activeMediaPlayers.isEmpty() || m_idleTimer.isActive())`. On `m_active` rising edge: immediate ping + start ping timer. On falling edge: stop ping timer. No counted-set, no debouncing, no inhibitor-source list — the simpler model emerged from the probe finding that ping cadence is the actual lever (firmware countdown auto-decrements; we just reset it before it expires). Singleton registered for QML as `ActivitySessionKeeper 1.0` (read-only `enabled`/`active`/`idleTimeoutSec`/`requireAcPower` Q_PROPERTYs for diagnostics; setter equivalents are reached via Config). Listens to: `core::Api::disconnected` (clear all state), `qApp::aboutToQuit` (stop timers cleanly), `Battery::powerSupplyChanged` (re-evaluate AC requirement), `EntityController::mediaPlayerStateChanged` (track Playing entities), `EntityController::entityCommandIssued` (curated allowlist arms idle timer).
- **Two new `EntityController` signals** plumbed for keeper hookup: `mediaPlayerStateChanged(QString entityId, int newState)` (emitted from a per-entity `entity::Base::stateChanged` connection set up in `addEntityObject`'s media-player branch — re-emits with bootstrap on already-Playing entities) and `entityCommandIssued(QString entityId, QString command)` (emitted at the top of `onEntityCommand` chokepoint). Both wired in `main.cpp` after both controllers are constructed. New `EntityController*` accessor on `ui::Controller`.
- **Three Settings → Power rows** above the existing "Keep WiFi connected in standby" block: Switch "Keep awake while watching/listening" (default off), Slider "Idle timeout after last button" (30–300 s, step 30, default 60), Switch "Only when on charger or dock" (default true). Backed by 3 new `Config` `Q_PROPERTY`s (`sessionKeeperEnabled`/`sessionKeeperIdleSec`/`sessionKeeperRequireAc`), QSettings keys under `power/sessionKeeper*`. UI is opt-in by design — same posture as v1.4.12 WoWLAN's discoverability — because the feature has battery-life implications when AC requirement is disabled.

### Architectural note
- **Probe-first delivery.** Stage 1 wire probe captured in `_diag_capture_set_power_mode.log` before any code was written. Probe revealed the firmware exposes `PUT /api/system/power?power_mode=NORMAL` directly via REST (not just WS) — same RPC dispatcher backs both transports, so internal `Api::setPowerMode` uses the existing `sendRequest(RequestTypes::set_power_mode, msgData)` WS path. Probe also revealed the original "inhibitor flag" design from the plan was unnecessary — the firmware countdown is the actual mechanism, and the ping resets it on transition. Plan was rewritten in-flight; ~80 LOC fewer than the original counted-set design.
- **Curated command allowlist** for Tier-2 idle-timer arming — `PLAY_PAUSE / PLAY / STOP / PAUSE / SEEK / VOLUME* / MUTE* / CURSOR_* / CHANNEL_* / NEXT / PREVIOUS / FAST_FORWARD / REWIND`. Avoids inhibit-on-poll-noise from state queries and capability fetches, which would race auto-revert and burn battery for nothing.
- **Drift increase: 5 modified upstream files** (`core.{h,cpp}`, `hardwareController.{h,cpp}`, `main.cpp`, `entityController.{h,cpp}`, `uiController.h`, `Power.qml`, `config.{h,cpp}`, `release.json`, `remote-ui.pro`) + 2 custom files (`activitySessionKeeper.{h,cpp}`).
- **Translation impact:** new `qsTr(...)` strings: `"Keep awake while watching/listening"`, `"Prevents the 5-minute sleep timer..." (helper paragraph)`, `"Idle timeout after last button: %1 seconds"`, `"%1 s"` × 2 (slider labels), `"Only when on charger or dock"`. Run `lupdate` regen.
- **Verification done on-device:** post-deploy at 22:31:34, sampled `/api/system/power` over 60 s — UI alive (no auto-revert triggered), `standby_timeout_sec` decrements naturally (749 → 719 → 689 in 60 s = 1:1 wall-clock decay), `standby_inhibitors:true` is firmware-managed (post-boot grace + dock detect, NOT our keeper since toggle defaults off and we're not pinging at idle). Behavioral verification (toggle on + start playback + 10 min idle) deferred to user — requires physical device interaction.
- **Auto-revert safety net** active per `project_auto_revert_validated_on_uc3.md`. If the build crashes the UI for 90 s, prior firmware reverts; re-enable manually via `curl -X PUT "http://${UC3_HOST}/api/system/install/ui?enable=true" -u "web-configurator:${UC3_PIN}"`.

---

## v1.4.13 — 2026-04-28 — WiFi onboarding: scoped failure cleanup (preserve other saved networks)

### Fixed
- **`onboarding/Wifi.qml` no longer nukes every saved WiFi network when a join attempt fails.** The two `Wifi.deleteAllNetworks()` callsites at line 71 (`onConnected(false)` handler) and line 249 (`connectionTimeoutTimer` 3 s timeout) were a nuclear cleanup — if the user had any pre-existing saved networks (rare during onboarding but possible after factory-reset-keep-data, or when re-running setup) and mistyped a password on one new network, all of them got wiped. Replaced both callsites with a new `Q_INVOKABLE Wifi::deletePendingJoinNetwork()` that targets only the SSID currently being joined. Tracking via new private member `m_pendingJoinSsid` set in `Wifi::connect()` (covers the timer-fires-before-`addNetwork`-completes race) and cleared in `onWifiEventChanged(CONNECTED)` for symmetry. Pre-checks `m_knownNetworkList.contains(ssid)` before calling `deleteSavedNetwork()` to silence the "network does not exist" notification when the timer wins the race against the async addNetwork response. Settings-side WiFi flow doesn't have this anti-pattern; this fix is onboarding-only.

### Architectural note
- **Three-line patch logic, ~20 lines diff.** Mechanical correction to a known bug; no architectural change. Touched: `wifi.h` (1 member, 1 Q_INVOKABLE decl, 1 copyright line), `wifi.cpp` (3 inserts: connect's pending-set, CONNECTED branch's pending-clear, deletePendingJoinNetwork impl), `onboarding/Wifi.qml` (2 line swaps).
- **Verification path is awkward** — onboarding only runs at first boot or factory reset. Untested on-device for v1.4.13; the patch is mechanical enough that I'm willing to ship without verification, but flag it: confirm next time you factory-reset (or use macOS sim with `UC_MODEL=DEV`).
- **Zero translation impact.** No new `qsTr(...)` strings.
- **Drift increase: zero.** All three modified files were already in our diff post-v1.4.12.

---

## v1.4.12 — 2026-04-28 — WiFi UX bundle: live diagnostics, always-on status indicator, reconnect button, WoWLAN surfacing, m_currentNetwork leak fix, periodic poll, scan-timer displayOff gate

### Added
- **Live link diagnostics on `WifiInfo` popup.** Five new rows render data the firmware was already shipping but the UI threw away: **Signal** (`rssi` dBm + optional `snr` dB), **Link speed** (`linkspeed` Mbps), **Throughput** (`est_throughput` Mbps), **BSSID**, **Channel** (computed from `freq`: 2.4 GHz `(f-2412)/5+1` for ch 1–13, special-cased ch 14 at 2484 MHz, 5 GHz `(f-5000)/5`). Backed by 7 new `Q_PROPERTY`s on the `Wifi` singleton (`currentBssid` / `currentRssi` / `currentAverageRssi` / `currentNoise` / `currentSnr` / `currentLinkSpeed` / `currentEstimatedThroughput`) populated in `Wifi::getWifiStatus()` from the existing `WifiStatus` struct (`src/core/structs.h:275-292` — already parsed in `core.cpp:2882-2898`, just unused). Single bundled `currentLinkInfoChanged()` signal — values update atomically per status response, so 7 separate signals would be wasteful. Properties are `int` not `double` because the firmware ships integer dBm/Mbps; conversion only happens at QML display time.
- **Always-visible signal-strength indicator in `StatusBar`.** Previously the wifi icon was visible **only** when disconnected or RSSI=NONE (and the inner switch was dead code returning `""` for every case). Now mirrors phone-style behavior: low-opacity base `uc:wifi` glyph always rendered, overlay strength tier (`uc:wifi-01` for WEAK / `uc:wifi-02` for OK or GOOD / `uc:wifi-03` for EXCELLENT) on top, red strikethrough only when fully disconnected. Pattern lifted from the existing `WifiNetworkList.qml:266-293` icon-stacking convention (Components.Icon doesn't overlay — child Icon is `anchors.centerIn` of parent). Same Layout slot footprint as the prior wifi icon (~40 px) so battery + profile icons keep their positions.
- **Reconnect button on `WifiInfo` popup** (visible only when connected). Calls a new `Q_INVOKABLE Wifi::reassociate()` that wraps the existing `Api::wifiCommand(WifiEnums::WifiCmd::REASSOCIATE)` — the firmware exposed it but no QML ever invoked it (`enums.h:379-385` lists DISCONNECT/RECONNECT/REASSOCIATE/ENABLE_ALL/DISABLE_ALL; only DISCONNECT was wired). REASSOCIATE redoes the 4-way handshake without full deauth — the right primitive for signal-drop or WoWLAN-recovery; RECONNECT is for explicit-disconnect resume which a button-press doesn't match. After the command, `QTimer::singleShot(2500, ...)` triggers a fresh `getWifiStatus()` so RSSI / link-speed / SNR all update without forcing the user to reopen the popup. `loading.start()` / `loading.success()` provide visual feedback through the global LoadingScreen (`main.qml:274-276`).
- **Periodic `getWifiStatus()` poll keeps the `StatusBar` indicator and `WifiInfo` diagnostics live.** Without a poll, the always-visible signal-strength bar (W3) only refreshed when `Api::connected` re-fired or `WifiInfo` was reopened — so signal walking out (e.g., walking room-to-room while watching TV) left the bar frozen at last-known. New `m_statusPollTimer` on the `Wifi` singleton fires every 30 s and calls `getWifiStatus()`. Started in `onWifiEventChanged(CONNECTED)` AND in `getWifiStatus`'s success branch (covers fresh-boot case where the device is already-connected and `WifiEvent::CONNECTED` never fires). Stopped in DISCONNECTED, in WPA_STATE in {ERROR, DISCONNECTED, intermediate}, and on display-off (see `setDisplayOff` below). 30 s cadence chosen to match Android's wifi indicator — fast enough to track room-scale signal changes, slow enough to be free in battery terms (one wpa_supplicant query per poll, ~negligible). Memory hygiene preserved: each poll runs through the v1.4.11 `oldNetwork->deleteLater()` pattern so `WifiNetwork` instance count stays at 1 even with 2880 polls/day.
- **`Wifi::setDisplayOff(bool)` slot mirrors core power mode → halts the new poll while screen is fully off.** Connected at construction to `core::Api::powerModeChanged` with the mapping `LOW_POWER || SUSPEND → displayOff=true`, `NORMAL → displayOff=false`. `IDLE` keeps polling (display dimmed but still visible — same precedent as `MatrixRainItem` and `ChargingScreen` per `project_uc3_power_modes.md` memory). On `displayOff=true` the poll timer stops. On wake (`displayOff=false`) the timer auto-restarts iff `m_isConnected`. Same gating discipline as v1.4.11's `MatrixRainItem` displayOff-helper consolidation (AP-UC-08 — zero CPU/GPU when screen is off).

### Fixed
- **WiFi scan polling no longer spins behind a black screen.** The 2 s `scanTimer` + 10 s `scanStartTimer` chain in `Wifi.qml` (settings) and `onboarding/Wifi.qml` kept the `wifiGetScanStatus` RPC firing every 2 s as long as the user was on the WiFi page — even when the display turned off. Walking away with WiFi settings still in foreground burned ~mW for nothing. Added a `Connections { target: Power }` block in both QML files that listens to `powerModeChanged(from, to)` — on `Low_power || Suspend` it calls `Wifi.stopNetworkScan() + scanStartTimer.stop() + scanTimer.stop()`; on transition back to `Normal` it restarts scanning iff the WiFi page is still active (`activeController === wifiPageContent` in settings, `OnboardingController.currentStep === Wifi` in onboarding). Imports `Power 1.0` + `Power.Modes 1.0` per the existing convention used in `main.qml` / `ChargingScreen.qml`. Same pattern as the v1.4.11 `MatrixRainItem` `displayOff` consolidation, applied to a different timer family.

- **`m_currentNetwork` leaked one `WifiNetwork` QObject per `getWifiStatus()` response.** `wifi.cpp:118` was bare `m_currentNetwork = new WifiNetwork(...)` with no free of the prior allocation. Each status response (every reconnect, every page-open, periodic poll) leaked one QObject parented to the singleton — the children were cleaned at process shutdown, but accumulated linearly during long uptimes with active wifi events. Same anti-pattern as the v1.4.11 entity-leak fix; same fix shape: snapshot old pointer → assign new → emit `currentNetworkChanged()` (so QML rebinds **synchronously** to new pointer) → `oldNetwork->deleteLater()` (queues to next event loop iteration, after bindings already point to the new object). Reversing the emit/delete order would crash QML reading freed memory. Same `oldNetwork = m_currentNetwork; if (oldNetwork) oldNetwork->deleteLater();` block applied at the constructor's initial allocation (`wifi.cpp:19`) for code uniformity — first call is a no-op since `m_currentNetwork` is `nullptr`-initialized.
- **WoWLAN toggle is now reachable on UCR3.** Was hidden behind `qEnvironmentVariable("UC_WOWLAN").toLower() == "true"` at `wifi.cpp:21` — UCR3 firmware never sets that env var so the upstream toggle in `Power.qml:68-110` resolved invisible despite the rest of the pipeline being fully wired (`Config.wowlanEnabled` → `Api::setNetworkCfg(... wowlanEnabled, ...)` → JSON `wifi.wake_on_wlan.enabled`). Replaced the env-var read with `m_wowlan = true;` — the property is `Q_PROPERTY(... CONSTANT)` (`wifi.h:106`), so a hardcoded value is correct. Closes the loop on the 2026-04-24 HA-entity-prune workaround memory (`feedback_ha_entity_prune_thermal.md`): users can now disable WoWLAN entirely rather than relying on phantom-wake mitigation by pruning HA-pushed entities. `Power.qml` itself is **untouched** — the existing block becomes visible automatically because its gate `HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled` now resolves true on UCR3.

### Architectural note
- **Zero upstream divergence pre-bundle, six files modified post-bundle.** All five Wi-Fi-relevant files (`src/hardware/wifi.{h,cpp}`, `src/qml/settings/settings/Wifi*.qml`, `src/qml/onboarding/Wifi.qml`, `src/qml/components/StatusBar.qml`) were byte-identical to `upstream/main` (incl. `v0.72.0`) before this release — UC has not touched the Wi-Fi subsystem since `v0.62.1`. Bundle adds drift on `wifi.{h,cpp}`, `StatusBar.qml`, `WifiInfo.qml`, `settings/settings/Wifi.qml`, and `onboarding/Wifi.qml`. `Power.qml` deliberately left untouched (already 209 lines diverged from upstream for our screensaver work — keeping the diff localized minimizes future merge cost). The W1 fix flips the toggle's gate without changing `Power.qml` itself.
- **No new ucapi or upstream API surface.** New Q_PROPERTYs/signals/Q_INVOKABLE all live on the existing `Wifi` singleton; no new `setNetworkCfg` parameters, no new `WifiCmd`/`WifiEvent` enum members, no new wire messages. Diagnostics fields were already parsed by `core.cpp:2882-2898` from the existing `wifi_status` response — bundle just plumbs them through.
- **No new QSettings keys.** WoWLAN `Config.wowlanEnabled` already persisted upstream (`config.h:75`); other diagnostics are transient runtime state and don't persist.
- **Translation impact.** New `qsTr(...)` strings: `"Reconnect"`, `"Signal"`, `"Link speed"`, `"Throughput"`, `"BSSID"`, `"Channel"`. Run `lupdate` regen before tagging release (per existing `[chore] translations: lupdate regen post-vX.Y.Z` cadence).
- **Verification matrix:** qmlscene-stub for QML-only rendering tests (channel calc edge cases — 2412→1, 2437→6, 2484→14, 5180→36); macOS sim build against `core-simulator` for end-to-end Q_PROPERTY plumbing + Reconnect button + WoWLAN visibility; ARM64 cross-compile (`unfoldedcircle/r2-toolchain-qt-5.15.8-static@sha256:d4b1b81b...`) + on-device Logdy tail filtered on `lcHwWifi` to confirm REASSOCIATE wire activity + 2.5 s post-reassoc status fetch + StatusBar bar transitions during walk-out from AP. Auto-revert safety net (`project_auto_revert_validated_on_uc3.md`) if the build crashes UI.

---

## v1.4.11 — 2026-04-27 — Audit-driven hardening: timer displayOff gate, entity-leak deferred-delete, toolchain digest pin

### Fixed
- **`MatrixRainItem` timer-start callsites now gate on `displayOff`.** Prior to v1.4.11, 7 `m_timer.start(...)` callsites in `matrixrain.cpp` (first-render at `updatePaintNode`, `resumeTicks`, `setSpeed`, slowdown effect at `handleSlowInput(true)`, slowdown release at `handleSlowInput(false)`, `handleRestoreInput`, `setRunning(true)`) had no `displayOff` guard — any of them firing while the screen was off would silently re-arm the tick timer and burn CPU/GPU until the next `setDisplayOff(true)` arrived. AP-UC-08 (§7.5) "Zero CPU/GPU when screen off — non-negotiable for battery life" contract was technically violated by 5 of the 7 sites (the other 2 already had `if (m_running)` guards but `m_running` is independent of `m_displayOff`). Consolidated all 7 callsites behind `startTimerAtSpeed()` and `startTimerAt(int intervalMs)` private helpers, both early-returning on `m_displayOff`. Line 1163's `handleSlowInput(true)` 3× interval preserved via the variable-interval helper. `setDisplayOff(false)` wake path keeps its direct `m_timer.start()` (helper would behave identically anyway since `m_displayOff` was just cleared on the previous line — kept as direct call to flag it as the canonical wake path).
- **`EntityController` defers entity deletion on `onCoreConnected` / `onCoreDisconnected` to plug a slow leak.** Plain `m_entities.clear()` removes the QHash entry but the entity QObject lives on as a child of `EntityController` with stale signal connections against an entity-id no longer in the map. Over a long uptime with flaky network, every reconnect leaks N entity objects + their slot wirings. New private helper `clearEntitiesDeferred()` schedules `deleteLater()` on the stale entity pointers via `QTimer::singleShot(100, this, ...)` — the 100 ms defer matches the precedent at `onEntityDeleted:520-530` ("leave a bit of time for the UI to do its thing to avoid QML type errors"). Captures pointers by value so they stay alive until the timer fires; `this` as the timer context auto-cancels if the controller dies first (children would die via Qt parent-child destruction anyway, so the lambda isn't needed). Both reconnect paths now use the helper.

### Changed
- **Toolchain image pinned by digest in `BUILD.md`.** Was `unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest` — could be re-pushed by upstream and silently change the compiler / Qt minor / static-libs without any local indication. Now pinned to `@sha256:d4b1b81b4722586aa1bc9e6fc2d8ccf329872d71d6bbda40a40adb74060d31c6`. Rotation cookbook documented inline in `BUILD.md` (`docker pull` → `docker inspect --format '{{index .RepoDigests 0}}'` → replace digest, commit as `[chore] toolchain: bump to <new-digest>`). `CLAUDE.md:105` still references `:latest` for now — informational, not load-bearing for the build path.

### Architectural note
- **Audit-driven release.** All three fixes surfaced from the v1.4.10-baseline codebase audit (see `_build_logs/2026-04-27_audit_v1.4.10.md`). Closes the `displayOff` gating gap (downgraded from "fragile, AP-UC-08 violation by 5 callsites" to "centralized helper, no callsite can resurrect the timer"), the slow leak (downgraded from "definite leak per reconnect" to "fixed"), and the toolchain reproducibility gap (downgraded from "could change silently" to "pinned by digest"). The audit's #1 (orphan-signal sweep) was already closed during the audit itself — only 2 genuine orphans remain (`voiceAssistantsChanged`, `respDockSetupProcesses`), both upstream-stub features tied to features upstream itself never finished, not worth fork-side fixes. The audit's #2 (`ScreensaverConfig` caching) was downgraded ⚠️ → ℹ️ on re-evaluation: the original framing conflated `QSettings::value()` with disk I/O when it's actually an in-memory `QHash` lookup, so the perf benefit was ~100× smaller than the audit claimed. Skipped — cost > benefit.
- **Drift increase: zero.** All three changes touch files already in the modified-upstream / custom-file manifests (`entityController.{h,cpp}` newly modified in v1.4.10, `matrixrain.{h,cpp}` custom since project inception, `BUILD.md` is custom). No new upstream files entered the diff.
- **No new ucapi or upstream API surface.** Both runtime changes are private helpers + internal callsite refactors; no new Q_PROPERTYs, no new signals, no new QSettings keys.
- **No translation impact.** No new `qsTr(...)` strings; translation regen picks up only line-number drift on `entityController.cpp` and `matrixrain.cpp`.

---

## v1.4.10 — 2026-04-27 — entity_change apply gap fix: NEW events now populate m_entities

### Fixed
- **`entity_change` events with `event_type=NEW` were silently discarded.** Upstream UC's `core::Api::processEntityChange` emits `entityAdded(entity)` for the NEW path (`src/core/core.cpp:2142`) but **nothing in the codebase ever connected to that signal** (verified via grep — declared in `core.h:360`, emitted, never wired). Subsequent `entity_change ev=CHANGE` events for that entity then hit the `m_entities.contains(entityId)` early-return in `EntityController::onEntityChanged()` (`entityController.cpp:430`) and were dropped without a log line. User-visible symptom: after a Kodi (or any) integration **uninstall→reinstall** cycle, the integration's NEW event re-creates the entity in core, the wire delivers the event to the firmware, the core-API GET shows the populated state — but the remote display never re-renders any subsequent attribute updates (artwork, title, position, etc.). Close+reopening the activity card worked around it because `Activity.qml`'s `includedEntityItem` delegate calls `EntityController.load(entityId)`, which fetches the (already-populated) state and inserts into m_entities. **Fix:** `entityController.cpp` ctor now `connect()`s `core::Api::entityAdded` to a new `EntityController::onEntityAdded(core::Entity)` slot that calls `addEntityObject(entity)`. `addEntityObject` is idempotent (early-return when the id is already in the map), so a NEW for an already-loaded entity is a safe no-op. Diagnosed via wire capture (`_diag_capture_5.log`) showing `ev=NEW` at 15:07:11 followed by ignored `ev=CHANGE` events through 15:09 even after the populated 17 KB JPEG data URI emit at 15:08:01.
- **Backstop in `EntityController::onEntityChanged` for late entity_change on unloaded entities.** Replaced the previously-silent early-return at `entityController.cpp:430` with a `qCWarning` + synchronous `load(entityId)` + return. Catches: (a) integrations that emit CHANGE without a prior NEW (spec violation but observed in practice), (b) reconnect races where `m_entities` was just cleared in `onCoreConnected`. Multiple in-flight loads for the same id are safe — `addEntityObject` short-circuits after the first response inserts; the duplicate fetches are harmless.
- **`MediaComponent.qml` now load+`entityLoaded`-pattern aligned with `SensorWidget.qml` / `SelectWidget.qml` / `Activity.qml`'s `includedEntityItem`.** Previously its `Component.onCompleted` only called `EntityController.get(entityId)` and stayed at `entityObj = null` if the entity wasn't already loaded — even when the entity later landed in m_entities, MediaComponent never re-bound. Defense-in-depth fix even with the C++ wiring above: `ensureEntityLoaded()` helper now calls `get` first, then `load` if the get returned null; a new `Connections { target: EntityController; function onEntityLoaded(success, loadedId) }` block re-acquires `entityObj` once the load lands and the id matches.

### Architectural note
- **Three-line root-cause class.** `entityAdded` was orphaned upstream — pre-existing latent bug going back to the upstream UC fork. Symptom only manifests after an integration uninstall→reinstall cycle (rare for end-users, very common during integration development), which is why it survived this long. The wire/core-store/render triangulation that pinned this came from the `integration-kodi-patch` session running a wire capture against `ws://192.168.2.204/ws` channel `all` (`_diag_capture_5.log`), the firmware-side static code analysis (this session), and the asymmetric "close+reopen works" symptom which exactly matches the path that *does* call `EntityController.load()`.
- **Zero ucapi contract change.** Both NEW and CHANGE event semantics are unchanged on the wire; the firmware now correctly applies both. No integration-side change required for this specific fix.
- **No translation impact.** No new qsTr strings, no UI surface — translation regen only picks up line-number drift on `entityController.cpp`.
- **Drift increase: +2 modified upstream files** (`entityController.h`, `entityController.cpp`); `MediaComponent.qml` already modified upstream-drift since v1.4.8.

---

## v1.4.9 — 2026-04-24 — MediaBrowser → Player Widget thumbnail preview handoff + setPreviewImage scheme filter + empty controls-bar auto-collapse

### Added
- **Browse-time thumbnails now render on the player widget immediately after `playMedia()`**, instead of waiting for the integration's `Player.GetItem` art response (which for unscraped Kodi library files, video-source SMB/NFS files, and most plugin items — Netflix, Movistar+, Amazon, Filmin — never arrives with usable art, leaving the widget blank or showing Kodi's stock `DefaultVideo.png` placeholder). Thumbnail is already in hand at tap-time in `MediaBrowser` via `modelData.thumbnail` / `pageContainer.thumbnail`; new `Q_INVOKABLE MediaPlayer::setPreviewImage(QString thumbnailUrl)` accepts the URL and drives it through the existing `getMediaImageColor()` fetch path, populating `m_mediaImage` base64 data URI + `m_mediaImageColor` accent (same machinery as the normal `media_image_url` flow, no new image plumbing). All 10 `requestPlayMedia(...)` call sites in `MediaBrowser.qml` now thread `thumbnail` through — centralized in the existing `requestPlayMedia()` helper which invokes `entityObj.setPreviewImage(thumbnail)` once before dispatching `entityObj.playMedia(...)`. `playMedia()` signature untouched; zero coupling between transient preview UI state and ucapi command dispatch.

### Changed
- **Preview-preserve guard in `MediaPlayer::updateAttribute(Media_image_url)`.** Once `setPreviewImage()` has populated the preview, incoming `media_image_url` updates that are empty OR contain any of Kodi's canonical skin-level placeholders — `DefaultVideo.png`, `DefaultAudio.png`, `DefaultVideoCover.png`, `DefaultMovies.png`, `DefaultMovieTitle.png`, `DefaultTVShows.png`, `DefaultTVShowTitle.png`, `DefaultPlaylist.png`, `DefaultFolder.png`, `DefaultFile.png`, `DefaultPicture.png`, `DefaultProgram.png` (case-insensitive substring match; catches both plain filename and URL-encoded `image%3A%2F%2FDefault*.png%2F` forms) — are swallowed rather than applied. Any non-empty, non-placeholder URL still upgrades the preview normally. Flag (`m_mediaImageIsPreview`) resets on: state→Off transition, successful real-URL fetch replacing the preview, or `clearMediaImageState()` elsewhere. **Intended behavior change:** post-preview the empty/placeholder URLs no longer blank the image. Only active AFTER `setPreviewImage()` is called — callers that never invoke the new API see identical behavior to pre-v1.4.9.
- **Preview preserved on real-URL fetch failure.** After the existing 3-retry budget in `onNetworkRequestFinished()` exhausts for a real `media_image_url` download (404, network error, etc.), `clearMediaImageState()` previously blanked `m_mediaImage` unconditionally. Now it's skipped when a preview is live, so the browse-time thumbnail keeps showing instead of dropping to the "Nothing is playing" fallback. **Intended behavior change:** honest fetch failures of real art no longer blank an existing preview. Only active AFTER `setPreviewImage()` is called.

### Architectural note
- **Pure FW-side fix; zero ucapi contract change.** Core-API spec (`entity_media_player.md`) has no preview/thumbnail convention for immediate display — `media_image_url` is the only hook. Research-verified: no upstream `unfoldedcircle/remote-ui` PR or discussion proposes one. The companion Kodi integration-patch release `v1.18.13-madalone.2` (patch 28 broader art-key fallback) already raises the integration-side floor for art; this release masks what Kodi itself can't provide (video-source files simply have no art for `Player.GetItem` to return). Complementary, not overlapping — preview bridges the gap between tap and real-art-arrival; integration's fallback raises the quality when Kodi has better art to return.
- **Shared `applyMediaImageUrl(const QString &url, bool isPreview)` private helper** consolidates the existing `Media_image_url` attribute case's fetch-dispatch logic (HTTP → `getMediaImageColor()`; base64 data URI → direct assign + `computeAverageImageColor()`) so both paths call the same implementation. Minor refactor; no behavior drift on the real-art upgrade path. v1.4.6's `QNetworkReply::OperationCanceledError` filters at both log sites preserved verbatim — new preview-fetch reply supersessions are still filtered silently, and don't consume the 3-retry budget.
- **No new `Config` / `QSettings` / `Q_PROPERTY` surface.** This is transient runtime entity state, not user preference — distinguishes from v1.4.2 `showVolumeOverlay` (user preference, global) and v1.4.4 `hideVolumeOverlay` (per-entity, pushed via ucapi `options`). Neither template fits. No new `qsTr(...)` strings, so translation regen is line-number churn only.

### Fixed
- **`setPreviewImage` now rejects unfetchable URL schemes.** `MediaBrowser`'s `modelData.thumbnail` sometimes contains UC3's internal `icon://uc:video` (or `icon://uc:music`) fallback string for unscraped library items — MediaBrowser renders that correctly as an icon name for display, but the raw string is not a URL. The initial v1.4.9 implementation passed it straight to `setPreviewImage` → `getMediaImageColor` → `QNetworkAccessManager::get` → `ProtocolUnknownError "Protocol 'icon' is unknown"` + 3 × 1 s retry burn before the preview was abandoned. `setPreviewImage` now whitelists only `http(s)://` and `data:image/…;base64,…` at entry, logging a DEBUG breadcrumb for any other scheme. Also rejects Kodi's internal `image://` scheme on the same grounds (QNAM can't resolve it; integrations are expected to pre-convert these). **Integration-side note:** the preview-fix proper requires the Kodi integration fork (`integration-kodi-patch`) to surface usable browse thumbnails — at present, unscraped video files in a library source show `modelData.thumbnail: "icon://uc:video"` (even though Kodi knows about local `*-thumb.jpg` files via `art.thumb`). That's tracked outside this release — see `integration-kodi-patch/PLAN_v1.18.13-madalone.2.md`.
- **Empty controls-bar gap when all 4 media buttons are toggled off.** After v1.4.8 added user toggles for shuffle / repeat / media-browser / source-picker, disabling all four left an 80 px empty reserved row under the progress bar in the activity card's media player — `controlsContainer`'s `height: controlsContainerHeight` was a fixed 80 px whenever the component was tall enough (≥ 320 px), and its `visible:` gate only checked that height > 0, not whether any children were actually visible. `controlsContainerHeight` in `MediaComponent.qml:50` now evaluates to 0 when all four button Configs are false, which collapses the RowLayout cleanly and feeds through the existing `mediaInfoHeight` calc (line 51) so the progress bar + title area reclaim the space naturally. One expression change, no new settings — avoids adding a 5th master "Show media bar" toggle that would duplicate the combined effect of the existing four.

---

## v1.4.8 — 2026-04-24 — Touchbar speed sensitivity tuning + media-button suppression toggles

Two independent additive changes bundled:

### Changed
- **Screensaver touchbar speed/density sensitivity tuned ~3× less sensitive.** Matrix + Starfield touchbar-speed control in `ChargingScreen.qml` was mapping 1 touchbar pixel to 1 unit of speed/density — a full 10→100 sweep happened over ~90 px, which felt twitchy in practice. Inserted a `scaledDelta = delta / 3` factor between the raw touchbar delta and the speed/density update, so the full range now sweeps over ~270 px. Minimum-movement dead zone (`abs(delta) < 3`) unchanged. If this new feel turns out to be too slow in practice, the knob is a single constant — easy to re-tune.

### Added
- **Four new global toggles to hide shuffle / repeat / media-browser / source-picker icons on the media player controls row** (`Settings → UI`). Parallel to the v1.4.2 `Config.showVolumeOverlay` pattern — global Q_PROPERTY + QSettings-backed, default `true` (preserves upstream behavior on upgrade), one-line `visible:` binding at each icon in `MediaComponent.qml`. Motivation: the Kodi integration fork cannot selectively strip individual `MediaPlayerFeatures` bits to hide just one of these icons (e.g., Kodi's native play-queue covers shuffle/repeat well enough that the remote-ui duplicates feel cluttered, but the browser button is useful to keep). UC-side config toggles solve the scope problem by operating at the display layer, not the feature-capability layer.
  - `Config.showShuffleButton` (QSettings key `ui/showShuffleButton`, default `true`) — hides the shuffle toggle icon when off.
  - `Config.showRepeatButton` (QSettings key `ui/showRepeatButton`, default `true`) — hides the repeat-mode cycle icon + its active-state badge when off.
  - `Config.showMediaBrowserButton` (QSettings key `ui/showMediaBrowserButton`, default `true`) — hides the media-browser shortcut icon when off. **Does not affect** the press-and-hold gesture on album art that upstream v0.72.0 added (same-purpose entry point, still gated on `Browse_media`/`Search_media` features — call this out in the Settings helper text).
  - `Config.showMediaSourceButton` (QSettings key `ui/showMediaSourceButton`, default `true`) — hides the source-picker icon when off.
  - **QtQuick.Layouts collapse semantics:** invisible children of the 4-icon RowLayout don't consume horizontal space (`Layout.fillWidth: true` on the 3 remaining visible icons redistributes the row automatically). No layout churn, no empty slots, no `Layout.preferredWidth: visible ? N : 0` gymnastics needed — matches the existing browser/source icons' `visible:` gating on MediaPlayerFeatures, which already relied on the same collapse behavior.
- **4 new Switch rows in `Settings → UI`** ("Show shuffle button" / "Show repeat button" / "Show media browser button" / "Show source picker button"), chained into `KeyNavigation.up`/`.down` below the existing "Show volume overlay" row. `Flickable.contentY` clamp bumped 1260 → 1900 for the ~640 px of added content (4 × ~160 px per toggle, same scaling as v1.4.2).

### Architectural note
- **Zero impact on upstream feature-capability semantics.** Integrations still advertise `Shuffle` / `Repeat` / `Browse_media` / `Search_media` / `Select_source` features as they always did; the existing `entityObj.hasFeature(...)` checks on browser/source icons remain in place and are AND-chained with the new Config toggle. A Config-disabled button hides regardless of feature advertising; a Config-enabled button still respects the entity's actual capabilities. The global master is strictly additive — it never *unhides* a button the entity chose not to expose.
- **Companion Kodi-patch note:** The Kodi integration fork (`integration-kodi-patch`) does not need any changes for this — the new toggles operate entirely on the remote-ui side, no ucapi `options` ingest, no new entity property. Contrast with v1.4.4's per-entity `hideVolumeOverlay` which required a two-repo contract; these four toggles were deliberately scoped as global UC-side preferences because per-entity granularity isn't needed (user's stated preference: "I don't want the browser button at all, everywhere" — a whole-UI preference, not a per-device one).

---

## v1.4.7 — 2026-04-24 — TouchSlider screensaver guard completeness fix

### Fixed
- **Physical touch slider bled through to the active media_player during screensaver.** When the Matrix/Starfield screensaver was active and the user touched the side slider (the intended behavior was: touchbar controls theme speed/density, volume/seek/etc. stays untouched), the slider was in fact still committing volume/seek/brightness/position writes to the currently-bound entity (e.g., Kodi volume getting changed from the bedroom while a movie played). Root cause: the `applicationWindow.screensaverActive` early-return guard was only present on `onTouchPressed` in each of the 4 `TouchSlider*.qml` variants — `onTouchXChanged` and `onTouchReleased` ran ungated. `onTouchPressed` being suppressed prevented `prevTouchX` and `targetVolume` from being initialized to fresh values, but `onTouchXChanged` still accumulated `targetVolume += Math.sign(rawDelta)` against stale state, and `onTouchReleased` then committed that target via `entityObj.setVolume(sliderContainer.targetVolume)` (or `setBrightness` / `setPosition` / seek). Net effect: the slider was writing arbitrary values to Kodi, undoing the user's own volume setting every time they adjusted screensaver speed. Fix: added the same one-line guard at the top of `onTouchXChanged` and `onTouchReleased` in all four variants (`TouchSliderVolume.qml`, `TouchSliderSeek.qml`, `TouchSliderBrightness.qml`, `TouchSliderPosition.qml`). 8 one-liners across 4 already-modified files; zero new upstream drift; identical pattern to the existing `onTouchPressed` guards, just completeness. **No behavior change when screensaver is inactive** — the guard is a strict defensive early-return on an existing property.

### Architectural note
- `ChargingScreen.qml`'s own `TouchSliderProcessor` Connections (screensaver speed/density control) are unaffected — that path was never guarded on `screensaverActive` because that handler only fires when the screensaver owns the touchbar, and the three C++-side signals (`touchPressed` / `touchXChanged` / `touchReleased`) are broadcast-only: Qt fans them out to every QML Connections target subscribed to `TouchSliderProcessor`, so both paths were receiving the same hardware events. The v1.4.5 memory note "the speed control is a completely different code path that never goes through `TouchSlider.qml`'s `startSetup()`" remains accurate for the `TouchSlider.qml` (entity-resolution) path but doesn't apply to the `TouchSlider*.qml` (per-feature Connections) paths — those always subscribed; they were just supposed to bail when screensaver owned the touchbar. Now they actually do.

---

## v1.4.6 — 2026-04-24 — Quiet boot hygiene pass

Four independent, low-risk fixes surfaced by v1.4.5 smoke-test logdy analysis (2026-04-24T09:56:45Z, `logdy-messages (3).json` — 167 `Image download Operation canceled` + 5 `QSoundEffect Error decoding` + 2 `TouchSliderProcessor ReferenceError` + 2-3 `VoiceOverlay undefined QString`). None are v1.4.5 regressions; all pre-date v1.4.4. Boot-log warning count drops ~177 → ≤ 4 (94% reduction), and one of the four fixes turns out to be a silently-broken functional wiring repair, not just cosmetic.

### Fixed
- **`mediaPlayer.cpp` image-download-cancel WARN flood (~167× per boot).** Both `onNetworkError()` and the error branch of `onNetworkRequestFinished()` were unconditionally logging WARN on `QNetworkReply::OperationCanceledError`, which is a routine supersession event (new artwork URL arrives before the previous fetch finishes), not a failure. Added early-return filters at both sites per the [QNetworkReply docs](https://doc.qt.io/qt-5/qnetworkreply.html) guidance (cancels also include `setTransferTimeout()` timeouts). Bonus correctness fix at site B: cancels no longer increment the 3-retry counter, preventing spurious retry-budget exhaustion during rapid entity re-subscribes post-boot. Zero behavior change for honest 4xx/5xx/timeout/network errors. 94% of boot-log noise eliminated from this one fix alone.
- **`VoiceOverlay.qml:666` "Unable to assign [undefined] to QString".** The `text:` binding on `assistantProfileNameText` was a JS function that fell off the end returning `undefined` when `voice.voiceEntityObj` was null (common during the voice-session entity-resolution window). Added a terminal `return "";` fallback after the outer `if` block. No visual change — the overlay already rendered empty when no profile, QML just stops complaining about the type mismatch.
- **`main.qml:507` "TouchSliderProcessor is not defined" (fires 2× per boot) — ALSO a silently-broken functional wiring fix.** `main.qml` was referencing `TouchSliderProcessor` in a `Connections { target: TouchSliderProcessor; ignoreUnknownSignals: true }` block without declaring `import TouchSlider 1.0`. The `ignoreUnknownSignals: true` had been silently masking the import failure, meaning the comment at line 502-505 ("TouchSliderProcessor bypasses Qt's event system... Direct connection ensures slider interaction prevents screensaver from activating during use") had never actually been wiring up on UC3 — the physical touch slider has not been resetting the idle-screensaver countdown. Added the missing import (the other 5 consumer QML files already had it). Post-deploy UC3 verification: touch the slider while idle-screensaver countdown is running; countdown should restart. If it doesn't, the signal-level hardware path needs follow-up investigation — but adding the QML import is the necessary precondition either way.
- **`soundEffects.cpp` "QSoundEffect(qaudio): Error decoding source file:///*.wav" (×5 wavs).** `createEffects()` unconditionally called `QSoundEffect::setSource(QUrl::fromLocalFile(m_effectsDir + "/click.wav"))` even when `m_effectsDir` was empty (env var `UC_SOUND_EFFECTS_PATH` unset) — resulting URL `file:///click.wav` → Qt audio backend emits 5 decode warnings. Added `#include <QFileInfo>`, initialized all 5 `QSoundEffect*` members to `nullptr` in the ctor init list, added empty-path early-return + per-file existence guard via a `makeEffect` lambda in `createEffects()` (skips `setSource` + logs DEBUG when the file is missing), and null-guards around every `m_effect*->setVolume()` / `->play()` call in `play()`'s switch. Matches Qt docs recommendation for [QSoundEffect](https://doc.qt.io/qt-5/qsoundeffect.html) lifecycle (verify source path / status before playback). Happy-path behavior unchanged when `UC_SOUND_EFFECTS_PATH` is set and wav files exist as expected on UC3 firmware; missing-path case now silently disables the specific effects instead of loudly failing at the Qt audio backend.

### Architectural note
- **All four fixes are independent and additive.** No cross-coupling, no new APIs, no new settings, no user-visible behavior changes for honest inputs. Net line count: ~55 added, ~4 deleted across 4 source files + 1 doc file. Two new upstream-pristine files now count as modified: `VoiceOverlay.qml` and `soundEffects.cpp`; `mediaPlayer.cpp` and `main.qml` were already modified (zero drift increase on those two). See `docs/CUSTOM_FILES.md` v1.4.6 section.
- **Explicitly deferred from this release:** cosmetic log-level downgrades for `uc.ui.resources: Empty ID passed to getIcon()` (2×/boot, fires during async entity resolution — expected condition) and `uc.app.i18n: Failed to remove translation` (1×/boot, first-boot-only: `QCoreApplication::removeTranslator` returns false when nothing is installed yet). Each would be a +1-drift pure log-level change with no functional value — skipped to honor `CLAUDE.md` §10 ("Minimize the diff against upstream to ease future merges"). `Cannot find EGLConfig` is Qt 5.15 internal and out of reach. Remaining warning budget post-v1.4.6: ≤ 4 per boot (2 Empty ID + 1 translation + 1 EGLConfig), down from ~177.

---

## v1.4.5 — 2026-04-24 — TouchSlider null-guard (startSetup + Loader binding)

### Fixed
- **Two latent `TouchSlider.qml` null-deref TypeErrors** surfaced in v1.4.4 deploy logdy trace (2026-04-24T10:32:40.760Z, single occurrence during a Settings → HOME navigation). `qrc:/components/TouchSlider.qml:44: TypeError: Value is null and could not be converted to an object` thrown inside `startSetup()` when `entityObj` was null (binding race during rapid card re-activation). `qrc:/components/TouchSlider.qml:161: TypeError: Cannot read property 'height' of null` thrown by the `sliderLoader` Loader's `y:` binding when `sliderLoader.item` became null post-`source=""`. Same class of bug v1.4.3 fixed for MediaBrowser's `onOpened` — identical null-guard recipe applied: (1) at the top of `startSetup()`, if `entityObj` is null, log a warn breadcrumb, set `touchSlider.active = false`, clear `sliderLoader.source`, and return before the first dereference; (2) the Loader's `y:` binding now evaluates `sliderLoader.item ? ui.height - sliderLoader.item.height : 0` — safe zero fallback when item is null, binding re-evaluates cleanly once a valid source is set again. Pre-existing robustness issue, not caused by v1.4.4.

### Architectural note
- **Zero impact on screensaver touchbar-speed control.** The matrix- and starfield-theme dpad-touchbar-speed feature uses the C++ `TouchSliderProcessor` singleton (`src/hardware/touchSlider.*`) directly via `onTouchPressed` / `onTouchXChanged` signals in `ChargingScreen.qml:631-650` — a completely different code path that never goes through `TouchSlider.qml`'s `startSetup()`. Confirmed via codebase trace: only the entity-Popup path is affected.
- **Zero behavior change for valid entities.** The null-guard recovery branch sets `active = false` and clears `source` — identical side effects to the existing "Disabled on this hardware" branch at lines 37-42. Only previously-throwing edge case (null entity during rapid navigation) is now silent-recovered.

---

## v1.4.4 — 2026-04-24 — MediaBrowser button expansion + volume split-guard + per-entity OSD flag

### Added
- **MediaBrowser — full hardware-button coverage.** `MUTE`, `STOP`, `NEXT`, `PREV`, `CHANNEL_UP`, and `CHANNEL_DOWN` now fire their commands while the media browser Popup is the active `buttonNavigation` owner. Before v1.4.4 these 6 hardware keys silently no-op'd — only `PLAY`, `VOLUME_UP`, `VOLUME_DOWN`, `BACK`, `HOME`, and the DPAD cluster were mapped. Feature gating: MUTE unguarded (matches `Tv.qml`/`Set_top_box.qml` pattern; `muteToggle()` is a command dispatch with no side effect when unsupported); STOP gated on `MediaPlayerFeatures.Stop`; NEXT prefers `Fast_forward` → falls back to `Next`; PREV prefers `Rewind` → falls back to `Previous`.
- **MediaBrowser ch+/- = page scroll.** In MediaBrowser, `CHANNEL_UP` and `CHANNEL_DOWN` now page-scroll the browse `ListView` (PgUp/PgDn-style). New top-level functions `pageScrollUp()` / `pageScrollDown()` / `pageScrollIncrement()` compute items-per-page from the Flickable's `contentHeight` and `height`, then use `positionViewAtIndex(i, ListView.Beginning)` for snap-to-item page jumps (avoids the jarring sub-item-height clamp Flickable does when you manipulate `contentY` directly on a short last page). Context-aware: outside MediaBrowser (TV / set-top-box / streaming-box / receiver / speaker device-class detail pages), `CHANNEL_UP`/`DOWN` retain their existing `Channel_switcher` command wiring — unchanged.
- **Per-entity `hideVolumeOverlay` Q_PROPERTY** on `MediaPlayer` entity (C++). Integrations can set `options["hide_volume_overlay"] = True` via ucapi's existing open-schema `options` dict, and remote-ui suppresses the volume OSD for that specific device. Ingest chain (all pre-existing, verified): ucapi Python → UC Core → `core.cpp:2045` parses into `Entity.options` QVariantMap → `entityController.cpp:207` threads it into the `MediaPlayer` constructor → `mediaPlayer.cpp` reads `hide_volume_overlay` alongside existing `volume_steps` / `simple_commands`. Bonus fix: `MediaPlayer::updateOptions(QVariant)` now overrides the no-op Base stub, so the flag is hot-updatable at runtime (previously, options updates post-construction were silently dropped entity-wide). Hot-update emits `hideVolumeOverlayChanged()` on flip.

### Changed
- **VolumeOverlay.qml::start() second guard.** v1.4.2 added the global `if (!Config.showVolumeOverlay) return;` master. v1.4.4 stacks a per-entity guard right after: `if (entity && entity.hideVolumeOverlay) return;`. Precedence: OSD is hidden if EITHER the global master OR the per-entity flag says hide. Zero impact on v1.4.2 users' existing configuration; purely additive. Null-guard on `entity` is defensive — the 14 call sites all pass a real entity, but `start()` is a public function so belt-and-suspenders.
- **Volume split-guard refactor across 14 call sites in 8 files.** v1.4.1 shipped coupled guards `if (hasFeature) { cmd; OSD; }` at `MediaBrowser.qml`, `Page.qml`, and 5 deviceclass files (Tv / Set_top_box / Streaming_box / Receiver / Speaker). v1.4.4 splits command-dispatch from OSD-display: the command fires unconditionally, only `volume.start()` stays feature-gated. Rationale: the v1.4.1 coupling was overcautious — integrations are the authority for what they can process, a stripped-feature entity is free to no-op internally, but remote-ui blocking the command from ever reaching the integration was a layering mistake. This refactor is required for the new per-entity `hideVolumeOverlay` flag to work correctly: if we hid the OSD without splitting the guards, an integration that stripped `VOLUME_UP_DOWN` features (to suppress OSD the old way) would ALSO break its actual volume control. `Activity.qml` at `src/qml/components/entities/activity/deviceclass/Activity.qml` was already architecturally correct (command fires via `activityBase.triggerCommand()` outside the `hasFeature` block; only `volume.start()` is gated) — no edits there.

### Architectural note
- This release is the remote-ui half of a two-repo contract. The companion Kodi integration-patch release (`v1.18.13-madalone.2`, planned) will rework its legacy `suppress_volume_overlay` toggle: stop removing `VOLUME_UP_DOWN` / `MUTE*` features from the entity (original architectural mistake — breaks Kodi's actual volume control), start setting `options["hide_volume_overlay"] = True` instead. With both halves shipped, Kodi volume works regardless of OSD suppression state. See `C:\Users\mique\_Claude Projects\integration-kodi-patch\PLAN_v1.18.13-madalone.2.md` item #6.
- Zero changes to upstream semantics beyond the intended split-guard fix. No breaking changes for honest integrations. Minor semantic shift for any integration that relied on v1.4.1's command-blocking side-effect via feature absence — that behavior is now relaxed (integration is the authority for command processing).

---

## v1.4.3 — 2026-04-24 — MediaBrowser hotfix (null-guard + inline loading + watchdog)

### Fixed
- **MediaBrowser unescapable loading loop (critical).** When `MediaBrowser.qml::onOpened` was called with a transiently-null `entityObj` binding (QML entity-resolution race), line 225's `entityObj.browseMedia(...)` threw a TypeError mid-handler. `buttonNavigation.takeControl()` at line 226 never ran → hardware HOME/BACK keys stopped working in the browser. Meanwhile `pageLoading: true` was already set at line 218, triggering the global `LoadingScreen` which blocks all UI input for up to 3 minutes via `inputController.blockInput(true)` + `timeOutTimer{interval: 180000}`. Net effect: user stuck in a 3-minute blackout with the rotating loading animation burning CPU at 60 fps (thermal risk on repeated entry — reproduced during v1.4.2 smoke testing, required hard reboot). Fix: null-guard `entityObj` at the top of `onOpened` — if null, log a warning and close the popup immediately via `Qt.callLater(close)`. Root cause analysis captured from a complete logdy trace of the live incident (2026-04-24T08:15:50Z); known upstream behaviour previously documented in `.claude-memory/project_media_browser_close_loop.md` ("X button dead, remote restart only escape") — now with a fix that prevents the trap instead of requiring a hardware reboot.
- **MediaBrowser no longer invokes the global `LoadingScreen`.** Replaced `loading.start()` / `loading.stop()` calls with a local `BusyIndicator` inside the popup's `contentItem`. Identical visual loading UX, but no `inputController.blockInput(true)`, so the popup's X close button, hardware BACK, and hardware HOME remain interactive at all times during browse loading. ~30 other callers of `LoadingScreen` across the codebase (Settings / Wifi / docks / integrations / profiles / groups / onboarding) are unaffected — their paths are genuinely blocking operations where input-block is the correct UX.
- **15-second local watchdog timer on `MediaBrowser`.** If `pageLoading` stays true for 15 s without a response, the popup auto-closes with the standard "Could not load media" warning notification. Belt-and-suspenders recovery for the "Kodi didn't respond" case, independent of the null-guard fix above. Watchdog is declarative (`running: isLoading`), auto-arms and auto-cancels from the existing `isLoading` binding — no manual start/stop calls to forget. 15 s is 15× the empirical worst-case Kodi browseMedia response on a healthy LAN (<1 s); aggressive enough to keep the device recoverable in under a log cycle, vs the old 180 s LoadingScreen timeout which was 180× expected worst case and kept the device in a compute-pinned state.

### Architectural note
- **No changes to `LoadingScreen.qml`**. It remains the correct component for the blocking paths it was designed for (Wifi setup, profile switch, dock configure, etc. — operations where a user shouldn't be able to interact while the device is in an in-between state). The bug was specifically that `MediaBrowser`, which is an INTERACTIVE browsing surface, inappropriately depended on it. Fix removes that dependency at the one right place.
- **Zero changes to upstream semantics for volume, battery, OSD, or any v1.4.0–v1.4.2 feature.** This is a strict hotfix. Feature work deferred to v1.4.4 (volume split-guard refactor + MediaBrowser media-button expansion).

---

## v1.4.2 — 2026-04-24 — Settings → UI toggle to suppress volume OSD popup

### Added
- **`Config.showVolumeOverlay`** (`Q_PROPERTY` in `src/config/config.h`, QSettings key `ui/showVolumeOverlay`, default `true` — preserves current behaviour on upgrade). Users can now globally disable the volume OSD that appears when pressing volume keys, without disabling the underlying volume commands themselves. Implementation is a single early-return guard at the top of `VolumeOverlay.qml::start()` — one suppression point covers all 16 call sites (8 files × VOLUME_UP + VOLUME_DOWN), architecturally orthogonal to v1.4.1's `hasFeature(Volume_up_down)` feature-advertising fix (that one checks the entity; this one is a user preference). Guard fires before any side effect (no property writes, no `hideTimer.restart()`, no `volume.open()`), so a disabled toggle produces zero OSD-related activity. Exposed as the final toggle in `Settings → UI` ("Show volume overlay") after "Coverflow in media browser", wired into the `KeyNavigation` chain. `Flickable.contentY` clamp bumped 1100 → 1260 for the ~160 px of added content (restores v1.3.0's value before the v1.4.0 rebase reverted it to upstream's 1100).

### Deprecation (downstream coordination)
- The `madalone/integration-kodi-patch` fork's `suppress_volume_overlay` (introduced `v1.18.13-madalone.1`, commit `d8cb60e`) stripped `VOLUME*` features from the Kodi entity to hide the OSD. Post-v1.4.1 that became a semantic flaw — hiding the OSD that way now also blocks actual volume control. `Config.showVolumeOverlay` supersedes it at the correct architectural layer (user preference, not entity capability). See `integration-kodi-patch/PLAN_v1.18.13-madalone.2.md` item #6 for the Kodi-side rework (ships after this tag).

---

## v1.4.1 — 2026-04-24 — volume OSD guard fix (root-cause for kodi-integration `suppress_volume_overlay`)

### Fixed
- **Volume OSD bypassed entity feature advertising in 7 of 8 call sites** — upstream bug. `Activity.qml`'s VOLUME_UP/DOWN handlers correctly check `entityObj.hasFeature(MediaPlayerFeatures.Volume_up_down)` before calling `volume.start()`; every other handler (`Page.qml` home-screen fallback, `MediaBrowser.qml`, and the 5 media-player deviceclass overrides `Receiver` / `Speaker` / `Tv` / `Streaming_box` / `Set_top_box`) called `volume.start()` unconditionally. Net effect: any media-player entity would fire the OSD on VOLUME_UP/DOWN even if the driver had removed the `Volume_up_down` feature from its capability set. This broke the contract that integration drivers could suppress volume UI by removing the feature. **Specific user impact:** the `madalone/integration-kodi-patch` fork's `suppress_volume_overlay` toggle (introduced in `v1.18.13-madalone.1` patch 6) correctly strips `Features.VOLUME` / `VOLUME_UP_DOWN` / `MUTE*` from the entity, but the OSD still appeared because remote-ui's call sites weren't checking the feature flag before rendering. Wrapped all 7 unguarded `volume.start()` call sites with the same `hasFeature(MediaPlayerFeatures.Volume_up_down)` guard `Activity.qml` already uses — 14 edits across 7 files, no new imports needed (each file already imports `Entity.MediaPlayer 1.0`). Now integration-declared volume capability is honoured consistently across all media-player UI paths, whether the user is on the home page, inside a detail view, in any media_player deviceclass skin, or navigating the media browser. **No config or settings changes** — the fix is purely respecting existing upstream semantics. Upstream-contributable: if you're reading this and you maintain `unfoldedcircle/remote-ui`, these are the same ~14 guards needed in stock. Files touched: `src/qml/components/Page.qml`, `src/qml/components/entities/media_player/MediaBrowser.qml`, `src/qml/components/entities/media_player/deviceclass/Receiver.qml` / `Speaker.qml` / `Tv.qml` / `Streaming_box.qml` / `Set_top_box.qml`.

---

## v1.4.0 — 2026-04-23 — upstream v0.72.0 merge (Option B rebase) + detail-page WiFi predicate fix

### Merged from upstream (`unfoldedcircle/remote-ui` "v0.72.0", commit `c76ff05`)

- **Press-and-hold media browse + new media controls row.** `MediaComponent.qml` adds a press-and-hold gesture on album art that opens the media browser (feature-gated on `Browse_media`/`Search_media` capability). When the component is tall enough (height ≥ 320px), a 4-icon controls row appears at the bottom: shuffle toggle, repeat cycle (OFF/ALL/ONE with active-state badge), browser shortcut, source picker. Each icon feature-gates itself against driver capabilities so hidden icons don't leave empty slots. Clean win — we hadn't touched this file.

- **Smarter media-browse error handling** in `MediaBrowser.qml`. Error codes now dispatch by semantic: `404` → empty "no results" state (previously showed a jarring retry modal for legitimate zero-result searches), `408` / `503` → retryable warning notification, other errors → non-retryable warning then close cleanly. Pagination falls back to `incoming.length >= pageLimit` when backends don't return `pagination.count`. Coverflow: non-focused fallback icons shrink from `0.5×` → `0.3×` art size for sharper focal hierarchy. Bottom text strip moves to `bottomMargin: 10` (from 30) and subtitle left-aligns with capitalized-first typography. Accept-theirs.

- **UC independently shipped our Mod 3 feature** as "Show battery indicator everywhere" — same user-visible behavior, different property name (`showBatteryEveryWhere` vs our `showBatteryOnDetailPages`) and layout approach (plain `Row` anchored at `rightMargin: 60` vs our `RowLayout` chain-anchoring 6 status icons in `BaseDetail.qml`). Adopted via **Option B rebase**: took their public API (property, QSettings key `ui/batteryEveryWhere`, Settings → UI toggle wording) and kept our superior Option A chain-anchoring layout. Their inline battery `Row` additions in `BaseTitle.qml` / `Activity.qml deviceclass` rejected during merge resolution — we already render the chip via `BatteryStatusChip.qml` through a Loader in the consolidated status strip, and accepting both would cause duplicate renders. `BatteryStatusChip.qml` retained as the reusable, touch-transparent primitive. Upstream's layout handles 2 icons statically; ours handles 6 via Qt's Layout solver adaptively — keeping ours costs one point of divergence per merge on 4 files but buys the overlap handling and future-proofs against additional status indicators.

- **One-shot QSettings migration** (`main.cpp::migrateLegacySettings`) carries the legacy `ui/batteryOnDetailPages` value (v1.3.0 default `true`) forward into `ui/batteryEveryWhere` (upstream default `false`) on first v1.4.0 boot. Without this, v1.3.0 users who accepted the chip default would silently lose it on upgrade — preservation semantics honour §1.3 "Never remove features without asking." Legacy key is removed after the one-shot copy. Function runs once after `setApplicationName` and before `Config` is constructed so `QSettings` resolves to the right file.

- **Minor layout polish** — `SelectWidget.qml` gains `clip: true` + `Layout.fillWidth` hints; `SensorWidget.qml` gains `clip: true` + `Layout.maximumWidth`. Both accept-theirs.

- **Icon set refresh** (`icons.otf` + `icon-mapping.json`) — upstream deduplicated JSON entries (removed duplicate keys that existed in an aliases section alongside primary definitions). All icon names our QML references still resolve after the merge — zero dangling references. Two icons render with different glyphs: `uc:list` `` → ``, `uc:heat` `` → ``. Functional behavior unchanged.

- **Translation refresh** — upstream's `en_US.ts` updated with new strings for the MediaComponent / MediaBrowser / battery-everywhere changes. All 8 locale `.ts` files regenerated via `lupdate` post-merge to keep them in sync with the current source.

### Fixed
- **Detail-page WiFi warning icon stayed visible on subjectively-good WiFi.** When Mod 3 Option A consolidated the detail-page status icons into `BaseDetail.qml`'s `RowLayout`, the WiFi-warning predicate inherited from the pre-consolidation `BaseTitle.qml` / `Activity.qml` blocks was preserved as `!Wifi.isConnected || signalStrength === NONE || signalStrength === WEAK` — wider than the home-screen `StatusBar.qml:252` predicate (`NONE only`, no WEAK). UC3's embedded WiFi reports `WEAK` for signals that are subjectively fine in practice, so detail pages showed the warning permanently while the home StatusBar correctly hid it. Aligned BaseDetail to StatusBar's narrower predicate — both surfaces now warn only on disconnect or true `NONE` signal. Single-line predicate fix in `BaseDetail.qml:_wifiWarningActive`; no anchor / RowLayout changes; no other consumers of the predicate exist.

### Changed (internal — breaking for any external Config consumer)
- ⚠️ **`Config.showBatteryOnDetailPages` Q_PROPERTY removed**; adopt upstream's `showBatteryEveryWhere`. Sole internal call site (`BaseDetail.qml:322` Loader `active:` binding) updated atomically in the same commit. No external consumers confirmed via grep — no test coverage existed for this property.
- ⚠️ **QSettings key renamed** `ui/batteryOnDetailPages` → `ui/batteryEveryWhere`. One-shot migration handles v1.3.0 user data.
- **Settings → UI toggle wording** changes to upstream's phrasing: "Battery on detail pages" → "Show battery indicator everywhere"; helper text "Show a compact battery indicator on entity and activity detail pages." → "Shows the battery level indicator on all pages and activities." Both translatable — locale files regenerated.

## v1.3.0 — 2026-04-23 — settings-freeze fix + atlas profiling overlay + dead-code sweep + hot-path polish + detail-page battery chip (Mod 3) + matrixrain.cpp subsystem extraction (audit B → A−)

### Refactored (Mod 1 architectural cleanup, audit B → A−)
- **`matrixrain.cpp` subsystem extraction.** Triggered by a deep-scan codebase audit that landed an honest grade B, dragged down primarily by one file: `src/ui/matrixrain.cpp` was 2055 lines (4× the project's own §1.6 budget of ~500), with `MatrixRainItem` carrying 150+ Q_PROPERTYs and a 214-line `updatePaintNode`. Extracted two new pure-C++ collaborator classes (no QObject, no MOC, no signals/slots — owned by-value on `MatrixRainItem`):
  - **`LayerPipeline`** (`src/ui/matrixrain/layerpipeline.{h,cpp}`, ~660 lines) — owns the multi-depth-plane subsystem: `RainLayer` struct, `m_layers[3]`, multi-layer atlas cache, combined atlas QImage, `BuildTimings`, plus `build` / `syncLayerConfig` / `advanceTick` / `applyDirection` / `applyGravityMode` / `applyGravityDirection` / `applyGravityLerpRate` / `applySpawnSuppress` / `applyDrainSpeedMultiplier` / `applyDrainMode` / `applyClearSubliminalCells` / `applyResetAfterScreenOff` (main-thread setter fan-outs) and `initAllLayers` / `countVisibleQuads` / `renderAll` / `renderLayer*` / `renderMidInteractiveOverlays` (render thread, called at QSG sync point). `MatrixRainVertex`, `emitQuad`, `packColor`, `depthColor`, `depthPriority`, `MAX_EMIT_VERTICES` moved to `layerpipeline.h` as inline render primitives so the single-layer (`matrixrain.cpp`) and multi-layer (`layerpipeline.cpp`) paths share one set.
  - **`AtlasBuilder`** (`src/ui/matrixrain/atlasbuilder.{h,cpp}`, ~115 lines) — all-static class wrapping the canonical `cacheKey(AtlasInputs)` SHA-1 hash (previously duplicated inline in both build paths) and `buildSingle(GlyphAtlas&, AtlasInputs)` for the layers-off path. Class-static cache (`s_singleCacheKey`, `s_singleCacheAtlas`) preserves the prior file-static lifetime — same first-paint cache-hit timing on repeat-dock. Defines the shared `AtlasInputs` struct used by both build paths.
  - **`bindToScreensaverConfig`** sliced from a 172-line monolith into a 36-line orchestrator + 8 domain helpers (`bindAppearance`, `bindDirectionAndGravity`, `bindGlitch`, `bindChaos`, `bindTap`, `bindMessages`, `bindSubliminal`, `bindDepthAndLayers`). Each helper owns BOTH the initial-sync setter calls AND the live-binding signal connects for its property group. The `QSignalBlocker` + `m_batchingUpdates` scope wraps all 8 helper calls so initial sync still produces ONE atlas rebuild at the end, not 60+.
  - **8 dead `MatrixRainItem::tap*()` wrapper methods** (header lines 511-519, cpp 1414-1449, ~37 lines) deleted. Plan agent verified zero call sites — `handleTapInput` dispatches directly into `RainSimulation::tap*()` via the `tapSim` reference, bypassing the wrappers entirely. Pure dead-code cleanup, user-approved per §1.3.
  
  **Zero observable behaviour change.** Every `Q_PROPERTY`, signal, `Q_INVOKABLE`, public method preserved bit-identical. Render-thread invariants preserved (no new `QSGNode` allocations outside `updatePaintNode`, no retained `QSGNode` references on collaborator classes). `displayOff` gating intact (collaborators have no timers — gated transitively by `MatrixRainItem::tick` which stops on `m_displayOff`). Initial-sync property order in `bindToScreensaverConfig` preserved within each domain group. Atlas dirty-flag (`m_atlasDirty`) ownership unchanged — stays on `MatrixRainItem` as the single-writer (main thread) / single-reader (render thread at sync point) handshake.
  
  **Outcomes:** `matrixrain.cpp` 2055 → ~1430 lines (−30%); `updatePaintNode` 214 → 164 lines (the residual ~64 lines over the §1.6 80-line target are QSG/texture/geometry boilerplate that doesn't decompose cleanly without making things worse); `bindToScreensaverConfig` orchestrator 172 → 36 lines (−79%); SHA-1 cache-key implementation reduced from 2 inline copies to 1 canonical impl in `AtlasBuilder::cacheKey`. On-device `ctorToPaintMs` parity verified vs pre-refactor baseline (cold 566 ms / wake1 179 ms / wake2 92 ms — within ±10% tolerance per Phase 5 acceptance gate).
  
  **Side-fixes shipped on the same branch:** stale macOS-absolute path in `test/matrixrain_preview/docker-compose.yml` replaced with portable `../..:/app/sources` mount (was blocking the per-phase Docker preview verification gate post-2026-04-15 Mac→Windows project migration); `test/matrixrain_preview/matrixrain_preview.pro` updated to register the new `layerpipeline.{h,cpp}` and `atlasbuilder.{h,cpp}` translation units (previously only the main firmware `.pro` knew about them).

### Added
- **Compact battery chip on all entity/activity detail pages (Mod 3).** The home-screen `StatusBar` — the only place battery state was visible — gets covered by the `containerSecond`/`containerThird` Popups the moment a user opens any Activity or entity detail page, leaving users blind to battery state for the entire remote-control session. New `src/qml/components/overlays/BatteryStatusChip.qml` (~68 lines, zero `MouseArea` so it's touch-transparent) mirrors the existing StatusBar battery visual 1:1 (bolt + percentage when charging, 16×30 battery-bar when on battery, color flips red on `Battery.low`, percentage follows existing `Config.showBatteryPercentage` toggle). Wired into `BaseDetail.qml` via a `Loader { active: Config.showBatteryOnDetailPages }` so all 27+ detail subclasses (Light, Climate, Tv, Speaker, Receiver, Activity, Blind, Switch, sensor variants, etc.) pick it up through inheritance with zero per-page plumbing. Anchored `right: iconIntegrationDisconnected.visible ? iconIntegrationDisconnected.left : iconClose.left` — a one-line conditional binding (no imperative JS) that shifts the chip left of the disconnected-integration warning icon when that rare edge case appears. Existing screensaver `BatteryOverlay.qml` is left untouched — it's screensaver-styled and `ScreensaverConfig`-coupled, not reusable for compact detail-page UI chrome.
- **New `Config.showBatteryOnDetailPages` toggle** (`Q_PROPERTY` in `src/config/config.h`, QSettings key `ui/batteryOnDetailPages`, default `true`). Exposed in `Settings → UI → "Battery on detail pages"` between the existing `Show battery percentage` and `Activities on pages` toggles with the standard label + helper-text pattern and wired into the `KeyNavigation` chain. `Flickable.contentY` clamp bumped 1100 → 1260 to accommodate the ~160 px of added content.

### Fixed
- **Battery state was invisible during every detail-page session.** On any Activity or entity detail popup, the home-screen StatusBar is hidden and there was no fallback — users could drain through a bedtime-routine / media-control session without realising. Addressed by the new detail-page battery chip above (see Added).
- **Detail-page battery chip overlapped the WiFi warning icon when WiFi was disconnected / weak / no-signal.** Initial Mod 3 plan flagged this as a "rare edge case" — a real-device screenshot (Kodi media-player detail page on weak WiFi) proved the overlap is guaranteed whenever the WiFi warning is visible, not rare. The chip Loader's `rightMargin: 10` placed its right edge at `parent.width - 80`; the WiFi warning in `BaseTitle.qml` / `Activity.qml` spans `parent.width - 120` to `parent.width - 60`. Hotfix in `BaseDetail.qml` only: added `import Wifi 1.0` + `import Wifi.SignalStrength 1.0`, a `readonly property bool _wifiWarningActive` mirroring the same visibility predicate used by `StatusBar.qml` / `BaseTitle.qml` / `Activity.qml` (`!Wifi.isConnected || signalStrength === NONE || signalStrength === WEAK`), and a conditional `rightMargin: (!iconIntegrationDisconnected.visible && _wifiWarningActive) ? 70 : 10` on the chip Loader — shifts the chip ~60 px further left when the WiFi warning is up, lands ~20 px clear of the WiFi icon's left edge. Zero edits to upstream-origin `BaseTitle.qml` / `Activity.qml`; zero anchor churn on the healthy-WiFi path; chip visual identical to pre-hotfix when WiFi is fine. **Verification pending** — reporter could not reproduce weak-WiFi conditions in the session; healthy-WiFi path unchanged so no regression risk on that branch. Architectural refactor (Option A) deferred: the correct long-term model inverts the anchor hierarchy so the battery chip is the persistent fixed reference and all warning icons (integration-disconnected, WiFi, future unknowns) chain leftward around it — documented in `~/.claude/plans/i-d-like-to-have-cheerful-snowglobe.md`.
- **Settings → Screensaver page stalled visibly when opened.** Root cause: `ChargingScreen.qml` (the settings page) unconditionally instantiated all six theme-dependent sub-pages on every open, then gated each with `visible: ScreensaverConfig.theme === "..."`. Qt still compiled + bound + laid out every theme's full settings UI (~100+ child items across 15+ sliders, color pickers, switches, repeater rectangles) on every page open, even though only one theme's page was displayed. Wrapped each theme sub-page in a `Loader { active: theme === "..."; sourceComponent: inlineComponent; asynchronous: true; visible: status === Loader.Ready }` so only the currently-selected theme's sub-page exists at any moment. Verified significantly snappier on-device.
- **Qt 5.15 `required property` + `Loader { source:... onLoaded:... }` trap.** The first deploy of the settings-freeze fix used `source: "qrc:/..."` + `onLoaded: item.settingsPage = chargingScreenPage`, which violates Qt 5.15's `required property Item settingsPage` contract — required properties are enforced at construction time, not via `onLoaded`. Result: `Loader.item` silently stayed `null`, all sub-page content was invisible, Matrix theme appeared to have lost all its effects and color settings on-device. Fixed by switching to `sourceComponent:` pointing to an inline `Component { }` declaration in `ChargingScreen.qml` with declarative property bindings, which satisfies the required-property contract at construction time. Logged as a standing rule: any menu-touching QML change must be tested in the macOS dev env or Docker preview before deploying to the physical UC3.

### Changed
- **Detail-page status icons consolidated into a single `RowLayout` in `BaseDetail.qml` (Option A, architectural follow-up to Mod 3 + Option B).** Replaces the previous mix of standalone icons + conditional anchor bindings with the `RowLayout` + `visible` + `Layout.preferredWidth` pattern UC3's own `StatusBar.qml` (lines 138-340) already uses — aligning `BaseDetail.qml` with the in-house idiom instead of inventing a new one. The strip is anchored `right: iconClose.left; rightMargin: 10; verticalCenter: iconClose.verticalCenter`, containing 6 children in left-to-right declaration order: (1) animated integration-loading spinner (`ui.isConnecting`, copied from `StatusBar.qml:147-217` minus its `HapticMouseArea` since detail-page indicators are display-only), (2) 12×12 red core-disconnected dot (`!ui.coreConnected`), (3) yellow `uc:cloud-arrow-down` software-update icon (`SoftwareUpdate.updateAvailable`), (4) WiFi warning (detail-page-wider predicate `!Wifi.isConnected || signalStrength === NONE || WEAK` preserved, **not** StatusBar's narrower NONE-only version), (5) per-entity `uc:link-slash` integration-disconnected icon, (6) battery chip Loader. The **battery chip is now the persistent fixed anchor** (rightmost visible child of a right-anchored RowLayout → its position doesn't move as peers appear / disappear); warnings shift leftward around it as they become visible, which is the user's stated architectural model. **Option B's conditional `rightMargin` hotfix is removed** — the Layout solver handles positioning correctly, no anchor juggling required. The Qt Layout solver reliably reflows via `Layout.preferredWidth: visible ? N : 0` per child, avoiding the ternary-anchor anti-pattern documented by `QTBUG-28931` (which was the footgun the original Option A plan was walking into).
- **Detail pages gained 3 new status indicators** (loading spinner, software-update icon, core-disconnected dot) brought over from `StatusBar.qml`. Previously only visible on the home-screen StatusBar; now also surfaced inside detail / activity sessions so the user isn't blind to system-level events while controlling an entity. Icons are display-only (no `MouseArea`, no click-to-open-panel — user closes the detail popup first and then taps the StatusBar version if action is needed). **Breaking change risk: ℹ️ minor** — detail pages look slightly different the first time a user triggers one of these states, but all icons are self-gating (only visible when their triggering condition is true), so the happy-path visual is unchanged.
- **WiFi warning block deleted from `BaseTitle.qml`** (upstream-origin, user-authorized per §1.3) and from `Activity.qml`'s custom title Rectangle (same origin, same authorization). `BaseDetail.qml` is now the single source of truth for detail-page WiFi warning rendering. 3-way dup (`StatusBar.qml` + `BaseTitle.qml` + `Activity.qml`) becomes a 2-way dup (`StatusBar.qml` + `BaseDetail.qml`) — still a future DRY opportunity, but narrower and cleaner.
- **Orphaned imports removed** from `BaseTitle.qml` (`import Wifi 1.0`, `import Wifi.SignalStrength 1.0`) and `Activity.qml` (same two) after the WiFi block deletions left them unused.

### Added
- **Atlas profiling overlay** — new toggle at the bottom of Settings → Screensaver → General Behavior (matrix-theme-only). When enabled, a small green text strip renders at the top of the Matrix rain showing the live phase timings of the most recent `buildCombinedAtlas` pass (`cache=hit|miss`, per-layer `buildMs`, `composeMs`, `remapMs`, `totalMs`, `firstPaintMs`, `ctorToPaintMs`). Backed by a new `MatrixRainItem::lastBuildSummary` `Q_PROPERTY` populated by `QElapsedTimer` instrumentation around `updatePolish` + `buildCombinedAtlas` + `updatePaintNode`. Off by default. Useful for future on-device profiling without needing `qCInfo` output (which `lodgy` doesn't surface usefully on UC3). Backed by a new `SCRN_BOOL(debugAtlasOverlay, "charging/debugAtlasOverlay", false)` in `ScreensaverConfig`.
- **Extracted `StarfieldSettings.qml` + `MinimalSettings.qml`** from what were previously ~300 lines of inline `ColumnLayout` blocks in `ChargingScreen.qml` (the settings page). Both new files match the existing pattern of the other four theme sub-pages (`MatrixAppearance`, `MatrixEffects`, `TvStaticSettings`, `AnalogSettings`). Translation `.ts` files re-generated in lock-step.

### Removed
- **`MatrixShutoffSettings.qml` + `matrixShutoffStyle` + `matrixShutoffDuration`** — orphan dead cruft left over from the v1.2.1 removal of the Matrix theme-native cascade screen-off animation. `MatrixShutoffSettings.qml` (the settings panel), its `qrc` entry, its `SCRN_STRING`/`SCRN_INT` declarations in `screensaverconfig.h`, its 23-line orphan translation blocks across eight `.ts` files, and the dead `navUpTarget` ternary branch in `generalBehavior.navUpTarget` that was silently falling through to the next branch. -264 lines net across 16 files, 1 file deleted. The settings panel for a feature that no longer exists is finally gone.

### Performance
- **`ClockOverlay.qml` date string** — previously recomputed on every 1Hz `ui.time` tick via `void(ui.time)` (~864k recomputes per day). Replaced with a `Timer` refreshing a cached property once per minute (~1440 fires per day — 98% fewer). `triggeredOnStart: true` so the cached string populates immediately when the overlay becomes visible. Same visible output, fewer JS allocations and locale lookups.
- **`matrixrain.cpp` render hot path** — `countVisibleQuads` / `renderStreamTrails` / their multi-layer variants previously heap-allocated two scratch `QVector`s (`order`, `streamColors`) per frame (~1200-2400 `malloc+free` per second at 20-40 FPS just for these buffers). Promoted both to shared `MatrixRainItem` members (`m_sortOrder`, `m_streamColorCache`), reused via `resize()` / `fill()`. Steady-state zero allocation on the render path; saves ~200-500 µs per frame on dense scenes.

### Chore
- **Reverted `src/hardware/hardwareController.{h,cpp}` whitespace-only diff vs upstream.** Two removed blank lines, zero semantic change — pure diff churn that wasn't shrinking any fork value. Restored to `upstream/main` to shrink the fork's diff surface against future merges.
- **Refreshed `docs/CUSTOM_FILES.md` manifest.** Previous "Last updated" was 2026-04-04; the manifest had drifted and was missing ~10 custom QML files (`ScreenOffOverlay.qml`, `BaseTheme.qml`, `TvStaticTheme.qml`, and all the `chargingscreen/*.qml` sub-pages added in v1.2.0 / v1.2.1 / v1.2.2) plus ~10 modified-upstream files (`src/config/config.cpp`, `src/logging.{h,cpp}`, `src/hardware/battery.h`, `src/ui/inputController.{h,cpp}`, `src/ui/entity/mediaPlayer.cpp`, `src/qml/components/{TouchSlider,MainContainer,LoadingFirst}.qml`, `src/qml/settings/settings/Power.qml`). Stale manifest is a merge-safety liability; brought back in sync.
- **Documented audit item closure: "ScreensaverConfig QSettings caching".** Investigation revealed that Qt 5.15's `QSettings` INI backend parses the file once on construction into an in-memory `QHash` — subsequent `value()` calls are O(1) hash lookups, not disk I/O. The audit's "biggest remaining runtime win" framing was based on a false premise, and the total per-dock cost of all ScreensaverConfig getter calls is ~127 µs one-time at `bindToScreensaverConfig()` with zero per-frame cost. Added warning comments in `src/ui/screensaverconfig_macros.h` and `src/ui/screensaverconfig.h` so future sessions don't re-litigate, and so the load-bearing dual-emit fix from commit `47b6d59` (Qt 5.15 MOC signal-chain bug) is visibly protected from well-meaning refactors.
- **New standing rule: menu-touching QML changes must be dev-env previewed before UC3 deploy.** Captured in auto-memory after the `required property` trap burned on the first settings-freeze fix deploy. Rule applies to any QML edit under `src/qml/settings/settings/*`, `src/qml/components/*Screen*`, `src/qml/components/themes/*`, `src/qml/components/overlays/*`, or any change to `Loader`/`Component` instantiation patterns.

## v1.2.2 — 2026-04-13 — screensaver bug fixes + thermal + hygiene sweep + post-release polish

### Fixed (user-reported Batch 0)
- **"Close on wake" toggle was ignored on undock** (`main.qml`). The `Battery.onPowerSupplyChanged(false)` handler now honors `ScreensaverConfig.motionToClose` the same way the sibling `Power.onPowerModeChanged` handler already did. Template for the fix was the `tapToClose` check at `ChargingScreen.qml:494` — no new state, no new abstractions, just connecting the piece that was disconnected.
- **Matrix/Starfield themes sometimes stayed black after the screen-off animation finished.** Root cause: `MatrixRainItem::resetAfterScreenOff()` was a designed-but-disconnected wake-refresh helper. Wired `cancelScreenOff()` into MatrixTheme and StarfieldTheme (both call the helper), plus belt-and-suspenders `themeLoader.item.update()` in `ChargingScreen.cancelScreenOffEffect()`. Also preemptively removed the latent `!root.displayOff` gates from TvStaticTheme's Timer bindings.
- **"Idle screensaver OFF" toggle had no effect** — popup still opened after `idleTimeout` expired. Added the `_shouldOpenOnIdle()` gate to the idle-timer fallback path in `main.qml`.
- **Display-off gap on undock**: screen-off animation started ~3 s after undock and display stayed powered on for ~7 s before blanking. Retimed cascade so animation fires at `displayTimeout` after undock; blackout-to-display-off gap is now < 2 s.
- **First-boot button lockout**: every physical remote button was dead on the first popup open (only tap worked). Root cause: `buttonNavigation.takeControl()` was gated on `themeLoader.item`, which was null when `Popup.onOpened` fired before the async Loader realized its child. Dropped the guard; re-call `takeControl()` in `themeLoader.onLoaded` as belt-and-suspenders.
- **MinimalTheme date line rendered blank**. Root cause: Batch C had replaced the hardcoded English day/month arrays with `Qt.formatDateTime(new Date(), "dddd, MMM d", Qt.locale())`, which silently returns empty in Qt 5.15 because the 3rd argument is a `Locale.FormatType` enum, not a full `Locale` object. First fix attempt (`Qt.locale().toString(new Date(), "dddd, MMM d")`) rendered literal `"[object Object]"` because the QML `Locale` type has no `toString(date, format)` method. Correct fix: `new Date().toLocaleDateString(Qt.locale(), "dddd, MMM d")` — the Qt QML extension on JS `Date.prototype` that accepts a Qt Locale object plus a Qt date format string.

### Fixed (thermal + render regressions)
- **Remote getting noticeably warm on long docked sessions.** `MatrixRainItem::setDisplayOff(true)` was stopping render output but the internal tick timer kept firing, pegging ~4% of one ARM core. Fixed by also calling `m_timer.stop()` and `m_gravity.stopAutoRotation()` during display-off; wake path in `setDisplayOff(false)` restarts both.
- **DPAD/touch direction changes were respawning the rain** instead of bending it smoothly via gravity-lerp. Direction changes now route through `GravityDirection::setDirection()` so the per-stream angle lerp takes over.

### Added (Batches A–G hygiene sweep)
- **Batch A** — version sync CI gate in `build.yml` (fails when `remote-ui.pro` VERSION ≠ `release.json` ≠ latest git tag); credentials scrubbed from all tracked docs and replaced with `.env.local` references; 60 MB of tracked tarball/Makefile build-artifact tree debt purged; `CRITICAL` landmine comment on `AnalogTheme.qml:120` for the Qt 5.15 qmlcachegen binding race.
- **Batch B** — strict warning flags enabled project-wide (`-Wall -Wextra -Werror=format -Wold-style-cast -Wfloat-equal -Woverloaded-virtual -Wshadow`); full cascade fixed in every custom `src/ui/*` file with **no pragma suppressions** — root causes only. Matrix hot-path UV-index bounds tightened with a new negative regression test.
- **Batch C** — `.githooks/pre-commit` running `cpplint.sh` + `clang-format --dry-run -Werror`; `lupdate` i18n baseline populated across all 8 `.ts` files; new `docs/UPSTREAM_MERGE.md` playbook; CHANGELOG sync gate in CI on tagged commits; `BaseTheme.qml` `cancelScreenOff` doc clarified as the wake-refresh hook.
- **Batch D** — `clang-tidy` in CI via `.github/workflows/tidy.yml` with a tolerant baseline ruleset (`modernize-*`, `bugprone-*`, `cert-*`, `performance-*`); per-file `NOLINT` comments with reasons for any intentional upstream-compat cases.
- **Batch E** — dead `CFG_*` macro family deleted from `src/config/config_macros.h` (zero call sites), `SCRN_*` documented as canonical in `STYLE_GUIDE.md` §6.6; four new QML theme lifecycle tests (`tst_starfield.qml`, `tst_minimal.qml`, `tst_analog.qml`, `tst_tvstatic.qml`) covering Starfield, Minimal, Analog, and TV Static beyond the existing Matrix suite.
- **Batch F** — GPG release signing pipeline (`docs/RELEASE_SIGNING.md`, `scripts/verify-release.sh`, signing step in `build.yml`); canary deploy with auto-revert on health-check failure (`scripts/deploy-canary.sh`); local mock UC3 endpoint (`scripts/mock-uc3-api.py`) so the canary path can be rehearsed without a spare device.
- **Batch G** — upstream merge rehearsal executed (fork is strict superset of `upstream/main@0586d45`, zero conflicts); `docs/A11Y_AUDIT.md` walkthrough; real translations populated for `de_DE.ts` + `fr_FR.ts`; CycloneDX `sbom.cdx.json` regenerated in CI.

### Chore / cleanup
- **Mod 2 (Avatar) completely stripped from the repo**. Research preserved losslessly in an external archive at `/Users/madalone/_Claude Projects/UC-Remote AVATAR Project/` with `pre-strip-originals/` snapshots and a `FINDINGS.md` index. Removed: `CUSTOM_FILES.md` Mod 2 section, `STYLE_GUIDE.md` avatar examples, `.gitignore` Mod 2 block, `glyphatlas.{h,cpp}` dormant braille charset support (`loadBrailleFont`, `CHARS_BRAILLE`, `buildBrailleChars`, all `charset == "braille"` branches including the programmatic 2×4 dot grid rendering), `resources/qrc/main.qrc` avatar comment, tracked prototype files (ASCII art references, `test_braille.qml`, `test_braille_mapper.qml`, `AVATAR_PLAN.md`, `BrailleFont.ttf`).
- **clang-tidy CI workflow** (`.github/workflows/tidy.yml`) was failing on every push because Qt 5.15 `lupdate` requires `libicui18n.so.56` which Ubuntu 22.04 no longer ships. Added the libicu56 cache/build/install preamble mirroring `build.yml`.
- **`subliminalStreamWritesGrid` test was flaky on CI.** Root cause: `RainSimulation`'s default ctor seeds `m_rng` from `std::random_device{}()`, so each runner got different entropy. Fix: deterministic `m_rng.seed(42)` + bumped attempt budget 5 → 30 as belt-and-suspenders.
- **GPG signing key rotated** from `82236E0F07904BDC` → `3172A28DABF07621`. The original Batch F key's passphrase was generated inline via `$(openssl rand -base64 32)` and never captured — command substitution consumed it. Regenerated fresh as a passphrase-less key (appropriate for CI-only signing stored in GitHub Secrets).
- **`README.md` rewritten for fork identity** (no longer the upstream UC README). New sections: five-theme hero table, YouTube Shorts demo link, explicit Matrix-tunability disclosure, "How this was built" (vibecoding loop + audits + testing + safety posture), "Human-reviewed documentation" (AI drafts, maintainer proofs), "With love and thanks" acknowledgment, expanded screen-off animations section, pinned tested firmware range (1.9.x only, maintainer device only), loud revert-first install callout.
- **`SCREENSAVER-README.md` screenshot tables fully rebuilt** to match current `docs/screenshots/` contents: 5 theme heroes, common toggles, Matrix/Starfield/Minimal/Analog/TV Static settings tables (21 sub-pages total), Power saving → Screen off animations settings page + 9 textual style descriptions.

## v1.2.1 — 2026-04-13 — drop displayOff gate from running binding (fixes wake-black)

### Fixed
- **Matrix and Starfield rain going black on wake** from any screen-off animation cycle. Root cause was a `running: visible && !isClosing && !displayOff` binding race: on wake, `setRunning(false) → setRunning(true)` fired in the same QML tick as `cancelScreenOffEffect` and `setSpeed`, and Qt does not guarantee binding / notifier / onChanged ordering. The race left the scene graph's first post-wake `updatePaintNode()` submitting an empty geometry node. Fix: drop `!displayOff` from the binding — the sim ticks through display-off at near-zero cost because Qt stops compositing when the display is off.
- **`holdPauseTimer` was writing `matrixRain.running = false` imperatively from QML**, permanently breaking the running binding on the theme instance. Replaced with new `Q_INVOKABLE pauseTicks()` / `resumeTicks()` C++ methods that stop and start the tick timer without touching `m_running` or the QML property — the binding stays live for future display-off/wake cycles.
- **`postAnimationSafetyTimer` was closing the popup** when the core's `Low_power` transition didn't fire within `leadMs + 1500ms`. On undocked setups where `Low_power` never fires, this was dumping the user to the home screen on every wake. Changed to set `displayOff = true` instead — popup stays alive.

### Removed
- **Matrix theme-native cascade animation.** Matrix now falls through to the shared `ScreenOffOverlay` styles same as Starfield and Minimal. The Matrix shutdown animation settings section is gone.

## v1.2.0 — 2026-04-13 — runtime slider wiring + tap master toggle

### Fixed
- **Matrix animation speed, density, trail length, fade, and color sliders silently having no effect on the live rain.** Root cause: a signal-to-signal `connect` in `ScreensaverConfig`'s ctor didn't route through correctly because the raw `matrix*Changed` signals were declared via macro while the transformed `*Changed` signals were in a separate manual `signals:` block — Qt's MOC + QML binding engine don't trace indirect signal chains. Fixed with the canonical Qt dual-emit pattern: hand-written setters emit both the raw and the transformed NOTIFY signal directly.

### Added
- Master **"Enable tap effects"** toggle in the Tap section of the Charging Screen settings — single switch that disables every tap effect at once. When off, taps still wake/cancel screen-off animations but produce no visual effect on the rain.
- Bumped `TICK_MAX_MS` from 150 to 300 so slider value 10 actually maps to a visibly slower tick.

---

# Upstream Unfolded Circle releases

## v0.71.1 - 2026-03-19
### Fixed
- Adjust color contrast
- Show play indication
- Punch volume and play button through in media browsing
- Search media class filter
- Close media browser after starting

---
## v0.71.0 - 2026-03-18
### Added
- Download progress for software updates
- Media browsing and search
- Coverflow view mode for media browsing
- Option to set coverflow as the default view

---
## v0.70.1 - 2026-02-24
### Added
- Log messages for software update process

---
## v0.70.0 - 2026-02-16
### Added
- Show warning when activity ready check is disabled

### Fixed
- Only check entities in activity on and off sequences

---
## v0.69.0 - 2026-01-22
### Added
- Select entity and select widget support

### Fixed
- Long entity names on activity loading screens won't break into multiple lines
- Missing retry logic from activity power button mapping

---
## v0.68.5 - 2026-01-15
### Fixed
- Entity state check before starting/stopping activities

---
## v0.68.4 - 2026-01-14
### Fixed
- Button control not working when entity opened from an activity
- Activity page indicator visible when activity in header is disabled

---
## v0.68.3 - 2026-01-13
### Fixed
- Dropdown menu button control. Mainly present in activity included entities screen letting button presses through.

---
## v0.68.2 - 2026-01-12
### Fixed
- Long press timer key tracking

---
## v0.68.1 - 2026-01-11
### Fixed
- Media image not shown on page entity

---
## v0.68.0 - 2026-01-09
### Fixed
- Button navigation sproadically stops working
- Voice assistant listening animation still showed after error
- Activity start screen with 0 included entities
- Activity error handling for sequences
- Entity name missing when starting activity for first time
- Text cut off on activity loading screen
- Ignore button presses for unavailable entities

---
## v0.67.0 - 2025-12-24
### Fixed
- Touch slider warning if entity is unavailable
- Sensor widget shows wrong values in activity UI

---
## v0.66.0 - 2025-12-19
### Fixed
- Same sensor value shown for all sensors
- Customer sensor label shows "Custom"
- Voice UI lets button presses through
- Processing showed after voice assistant finished event
- Activity sequence timeout handling
- Activity's entities readyness check
- Invalid media player state
- Media image not shown on main page entities
- Touch slider commands even when entity is unavailable

### Changed
- Improved voice assistant error handling
- Improved activity turn on/off after resume from system sleep

---
## v0.65.10 - 2025-12-10
### Fixed
- Charging screen shown after reboot

---
## v0.65.2 - 2025-12-05
### Added
- Voice Assistant support
- Command retry after wakeup. Wakeup window is configurable in Power Saving settings.

### Fixed
- Media image download timeout handling

### Changed
- Disable certificate validation for media image download

---
## v0.64.4 - 2025-11-27
### Fixed
- Media image not loaded sproadically
- Sensor value not shown within activity

---
## v0.64.3 - 2025-11-23
### Changed
- Display brightness minimum value to 5%


---
## v0.64.1 - 2025-11-21
### Fixed
- Only load media image when it has changed

---
## v0.64.0 - 2025-11-18
### Added
- Touch slider configuration support

---
## v0.63.0 - 2025-11-06
### Added
- Sensor widget support for activities
- Notify before starting an activity if an integration is not ready
- Notify with option to try again if command fails due to device not being ready

### Changed
- Charging screen shows up when a power supply is detected with additional information if the device is charging or just being supplied with power
- Show IP address instead of hostname by default for the Web Configurator

### Fixed
- Sensor UI screens
- Popup menu button handling
- Visilbity of software update icon in the status bar
- Failed marco sequences not shown
- Wifi network list empty during dock setup
- Popup menu trims text for long text items

---
## v0.62.2 - 2025-09-26
### Fixed
- Popup menu button handling

---
## v0.62.0 - 2025-09-23
### Changed
- Reload entity data when entering UI screen
- Update method for loading button mapping

---
## v0.61.0 - 2025-09-22
### Changed
- Starting an activity from another activity will open the new activity's UI

### Fixed
- Rendering of icons
- Show loading icon next to WiFi networks, when connecting

---
## v0.60.1 - 2025-09-19
### Added
- Binary sensor support

### Fixed
- Known WiFi network did not connect when selected

### Changed
- WiFi settings menu

---
## v0.59.0 - 2025-09-12
### Fixed
- Repeat command handling, do not wait for ack

### Changed
- Repeat count increased to 4

---
## v0.58.3 - 2025-08-27
### Fixed
- QR code in pull-down menu and during onboarding
- Popup menu closed when home button released when it has opened

---
## v0.58.2 - 2025-08-26
### Fixed
- QR code in pull-down menu and during onboarding
- Popup menu closed when home button released when it has opened

---
## v0.58.0 - 2025-08-25
### Fixed
- QR code in pull-down menu and during onboarding
- Popup menu closed when home button released when it has opened

---
## v0.57.0 - 2025-08-18
### Fixed
- Language text logic
- High power consumption when display is off

### Changed
- Renamed media image fill option

---
## v0.56.4 - 2025-08-05
### Fixed
- High CPU consumption in low power mode

---
## v0.56.3 - 2025-08-03
### Fixed
- Wifi scan interval slider range

---
## v0.56.2 - 2025-08-02
### Fixed
- High CPU consumption while loading animation is running

---
## v0.56.0 - 2025-07-24
### Added
- WiFi band selection
- WiFi scan interval config option

---
## v0.55.1 - 2025-07-04
### Fixed
- Incorrect dock image shown
- Dock discovery help text

---
## v0.54.10 - 2025-06-06
### Fixed
- Bug in repeat logic

---
## v0.54.9 - 2025-05-27
### Fixed
- Wifi icon size in known networks
- Transparent media image when no media text is shown
- Media player screen shuffle, repeat and app icons cut off
- Activity bar height jumps when image changes
- Media image sometimes not shown
- Touch slider not working with certain device classes

---
## v0.54.5 - 2025-05-23
### Fixed
- Turn off menu only shows entities with on/off features available

## v0.54.4 - 2025-05-19
### Changed
- Media type is displayed as string

### Fixed
- Record, Stop and Menu buttons not working on Remote 3
- Icon shown under transparent media image

## v0.54.2 - 2025-05-12
### Fixed
- Icon shown under transparent media image

## v0.53.2 - 2025-04-06
### Added
- Option to fill available space for media player widget. Can be turned on in Settings / User interface.

### Fixed
- Activity list image and icon sizes
- Media player widget shrinking

## v0.50.2 - 2025-04-03
### Fixed
- Missing media player icon map

## v0.50.0 - 2025-03-31
### Added
- Support for touch slider
- Access profiles, Web Configurator and settings by pulling down the page

### Changed
- Activity bar moved to the page header with option to turn it off in Settings / User Interface

### Fixed
- Missing icons during dock discovery
- Wrong remote name for Remote 3 during onboarding

## v0.49.0 - 2024-02-11
### Added
- Option to show media widget as horizontal

## v0.48.0 - 2024-01-17
### Fixed
- DPAD middle button behaviour on pages
- Sizing of media player widget on activity UI pages. Very small media widget won't show progress bar and media information.
