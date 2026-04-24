# Remote UI Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

# Fork releases (Madalone's Defolded Circle 3)

Releases below this point are from the custom-screensaver fork maintained by [@mmadalone](https://github.com/mmadalone), not from upstream Unfolded Circle. Upstream `unfoldedcircle/remote-ui` release history continues further down starting at `v0.71.1`.

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
