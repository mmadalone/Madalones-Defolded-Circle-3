# Custom Files Manifest — UC Remote 3 Mods

Tracks every file that is custom (added by madalone) or modified from the upstream `unfoldedcircle/remote-ui` codebase. If a file is not listed here, it is upstream and should not be modified without explicit justification.

**Upstream base:** `v0.71.1`  
**Last updated:** 2026-04-24 (v1.4.9 — MediaBrowser thumbnail preview handoff)

---

## Shared Infrastructure (used by multiple mods)

### Custom Files
| File | Purpose |
|------|---------|
| `src/ui/glyphatlas.h` | Glyph atlas texture builder — shared GPU texture for all character grid renderers |
| `src/ui/glyphatlas.cpp` | Atlas build, UV lookup, brightness map, font loading |
| `src/ui/simcontext.h` | Simulation context struct (header-only) |

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/main.cpp` | Added `#include` for MatrixRain, ScreensaverConfig. `qmlRegisterType<MatrixRainItem>`. Instantiated `ScreensaverConfig` singleton. v1.4.0: added `<QSettings>` include + `migrateLegacySettings()` helper (carries v1.3.0's `ui/batteryOnDetailPages` forward into upstream's `ui/batteryEveryWhere`) called once at startup after `setApplicationName`. |
| `src/config/config.h` | Screensaver properties removed (moved to ScreensaverConfig). Only a redirect comment remains. v1.4.2: added `showVolumeOverlay` Q_PROPERTY + getter/setter decls + signal. |
| `src/config/config.cpp` | Mirror of config.h: screensaver impls removed, redirect comment for moved ScreensaverConfig. v1.4.2: added `getShowVolumeOverlay` / `setShowVolumeOverlay` (QSettings key `ui/showVolumeOverlay`, default `true`). |
| `src/logging.h` | Added `lcScreensaver` logging category declaration. |
| `src/logging.cpp` | Added `lcScreensaver` logging category definition (uc.ui.screensaver). |
| `src/hardware/battery.h` | Added `Q_INVOKABLE setPowerSupply()` + `instance()` getter for DEV-mode F12 dock-toggle. |
| `src/ui/inputController.h` | Added global event filter declaration + `touchDetected` signal (DEV F12 + idle-timer reset). |
| `src/ui/inputController.cpp` | DEV F12 dock-toggle event filter, `touchDetected` signal emit, idle-timer reset wiring. ~40 lines. |
| `src/ui/entity/mediaPlayer.cpp` | Bugfix: re-download image when URL is present but image data is empty (2-line change). |
| `src/qml/components/entities/media_player/ImageLoader.qml` | Same image-re-download bugfix on the QML side. |
| `remote-ui.pro` | Added custom HEADERS/SOURCES at end of lists (matrixrain, screensaverconfig, rainsimulation, gravitydirection, glitchengine, messageengine, glyphatlas, matrixrain/layerpipeline, matrixrain/atlasbuilder). |
| `resources/qrc/main.qrc` | Registered all custom QML files and settings sub-pages. |

---

## Mod 1: Screensaver System

### Custom C++ Files
| File | Purpose | ~Lines |
|------|---------|--------|
| `src/ui/matrixrain.h` | `MatrixRainItem` QQuickItem — GPU-accelerated matrix rain renderer | ~600 |
| `src/ui/matrixrain.cpp` | QSG rendering, vertex buffer, atlas upload, single-layer stream iteration, ScreensaverConfig binding orchestration | ~1430 |
| `src/ui/matrixrain/layerpipeline.h` | `LayerPipeline` — multi-layer rain pipeline (3 depth planes), shared render primitives (`MatrixRainVertex`, `emitQuad`, `packColor`, etc.) | ~245 |
| `src/ui/matrixrain/layerpipeline.cpp` | LayerPipeline build/sync/render implementation, multi-layer atlas cache | ~660 |
| `src/ui/matrixrain/atlasbuilder.h` | `AtlasBuilder` — single-layer atlas builder + canonical SHA-1 cache-key hashing (deduped across single + multi-layer paths). Defines shared `AtlasInputs` struct. | ~65 |
| `src/ui/matrixrain/atlasbuilder.cpp` | Class-static single-layer cache (`s_singleCacheKey`, `s_singleCacheAtlas`), `buildSingle`, `cacheKey` | ~50 |
| `src/ui/rainsimulation.h` | `RainSimulation` — stream-based rain simulation engine | ~150 |
| `src/ui/rainsimulation.cpp` | Stream lifecycle, head advance, float movement, density management | ~400 |
| `src/ui/glitchengine.h` | `GlitchEngine` — char swap, brightness flash, column flash, stutter, reverse glow | ~100 |
| `src/ui/glitchengine.cpp` | Per-tick glitch application, rate control | ~200 |
| `src/ui/messageengine.h` | `MessageEngine` — on-screen text rendering in character grid | ~80 |
| `src/ui/messageengine.cpp` | Text layout, character placement | ~150 |
| `src/ui/gravitydirection.h` | `GravityDirection` — direction system, auto-rotate | ~80 |
| `src/ui/gravitydirection.cpp` | Angle sweep, per-stream lerp, travel/spread axis math | ~200 |
| `src/ui/screensaverconfig.h` | `ScreensaverConfig` singleton — owns QSettings, SCRN_* macros, 114 properties (108 SCRN + 6 transformed) | ~220 |
| `src/ui/screensaverconfig.cpp` | QSettings init, transformed getters, Battery deferred connect | ~100 |
| `src/ui/screensaverconfig_macros.h` | SCRN_BOOL/INT/STRING read-write property macros | ~50 |

### Custom QML Files
| File | Purpose |
|------|---------|
| `src/qml/components/themes/BaseTheme.qml` | Base theme class — common properties + protocol for all screensaver themes |
| `src/qml/components/themes/MatrixTheme.qml` | Matrix rain screensaver theme |
| `src/qml/components/themes/StarfieldTheme.qml` | Starfield screensaver theme |
| `src/qml/components/themes/MinimalTheme.qml` | Minimal digital clock theme with GradientText |
| `src/qml/components/themes/AnalogTheme.qml` | UC stock analog clock extracted as theme option |
| `src/qml/components/themes/TvStaticTheme.qml` | TV static CRT-shader theme with channel-flash + chroma noise |
| `src/qml/components/themes/Palettes.qml` | QML singleton — single source of truth for shared gradient stops (rainbow / rainbow+ / neon) and battery-level color tiers. Registered via `qmlRegisterSingletonType` in `main.cpp` as `Palettes 1.0`. |
| `src/qml/components/themes/MatrixTheme_canvas_backup.qml` | Archived Canvas-based prototype (not used) |
| `src/qml/components/overlays/GradientText.qml` | Reusable solid/rainbow gradient text component |
| `src/qml/components/overlays/ClockOverlay.qml` | Clock overlay with GradientText, date, 24h, color |
| `src/qml/components/overlays/BatteryOverlay.qml` | Battery overlay for screensaver |
| `src/qml/components/overlays/ScreenOffOverlay.qml` | Shared screen-off animation overlay (fade/flash/vignette/wipe/theme-native) |
| `src/qml/settings/settings/chargingscreen/ThemeSelector.qml` | Theme picker sub-page |
| `src/qml/settings/settings/chargingscreen/CommonToggles.qml` | Clock/battery/tap-to-close toggles |
| `src/qml/settings/settings/chargingscreen/MatrixAppearance.qml` | Matrix color, speed, density, charset settings |
| `src/qml/settings/settings/chargingscreen/MatrixEffects.qml` | Container for matrix effect sub-sections (delegates to ChaosSection, DirectionGlitchSection, TapSection, MessageSection) |
| `src/qml/settings/settings/chargingscreen/ChaosSection.qml` | Chaos burst effects (surge, scramble, freeze, square burst, ripple, wipe, scatter) |
| `src/qml/settings/settings/chargingscreen/DirectionGlitchSection.qml` | Direction flip + glitch (rate, flash, stutter, reverse) settings |
| `src/qml/settings/settings/chargingscreen/TapSection.qml` | Tap/touch effect settings (burst, flash, scramble, spawn, message, ripple, wipe) |
| `src/qml/settings/settings/chargingscreen/MessageSection.qml` | Message/subliminal text settings |
| `src/qml/settings/settings/chargingscreen/StarfieldSettings.qml` | Starfield theme sliders + color pickers (extracted from inline 2026-04-14) |
| `src/qml/settings/settings/chargingscreen/MinimalSettings.qml` | Minimal theme clock/date sizes + font + color pickers (extracted from inline 2026-04-14) |
| `src/qml/settings/settings/chargingscreen/TvStaticSettings.qml` | TV static theme intensity/scanline/chroma/tracking + channel-flash sliders |
| `src/qml/settings/settings/chargingscreen/AnalogSettings.qml` | Analog theme shutdown-hands picker |
| `src/qml/settings/settings/chargingscreen/GeneralBehavior.qml` | General screensaver behavior settings (idle timer, tap-to-close, DPAD interactive, debug overlay toggle) |

### Custom Assets
| File | Purpose |
|------|---------|
| `deploy/config/NotoSansMonoCJKjp.otf` | 23KB katakana font subset for Matrix rain |
| `deploy/config/charging_screen.json` | Default screensaver configuration |

### Modified Upstream QML
| File | Modification |
|------|-------------|
| `src/qml/components/ChargingScreen.qml` | Replaced stock analog clock with theme Loader system. Added ButtonNavigation for interactive screensaver controls. Added displayOff propagation. |
| `src/qml/settings/settings/ChargingScreen.qml` | Replaced stock charging settings with full screensaver configuration (theme selector + sub-pages). 2026-04-14: theme sub-pages wrapped in deferred Loaders via `sourceComponent:` + inline `Component { }` to fix settings-open freeze. |
| `src/qml/settings/Settings.qml` | Added "Screensaver" menu entry. |
| `src/qml/components/Switch.qml` | Added `Keys.onReturnPressed`/`onEnterPressed` for DPAD center toggle. |
| `src/qml/components/TouchSlider.qml` | Added `applicationWindow.screensaverActive` guard to suppress during screensaver (base shared component; the 4 specific variants below have the same guard). **v1.4.5:** null-guard added at top of `startSetup()` — if `entityObj` is null, log warn + set `active=false` + clear `sliderLoader.source` + return (prevents TypeError at line 44). Loader `y:` binding at line 161 now uses ternary `sliderLoader.item ? ui.height - sliderLoader.item.height : 0` to guard against null `item` during source="" transitions. Same null-guard recipe as v1.4.3 MediaBrowser. |
| `src/qml/components/TouchSliderVolume.qml` | Added `applicationWindow.screensaverActive` guard to suppress during screensaver. **v1.4.7:** guard extended to `onTouchXChanged` and `onTouchReleased` (was press-only — XChanged was accumulating stale `targetVolume`, Released was committing it via `entityObj.setVolume()`, so Kodi volume got overwritten every time the user adjusted screensaver speed). |
| `src/qml/components/TouchSliderSeek.qml` | Same screensaverActive guard. **v1.4.7:** extended to XChanged + Released, same reason. |
| `src/qml/components/TouchSliderBrightness.qml` | Same screensaverActive guard. **v1.4.7:** extended to XChanged + Released, same reason. |
| `src/qml/components/TouchSliderPosition.qml` | Same screensaverActive guard. **v1.4.7:** extended to XChanged + Released, same reason. |
| `src/qml/components/LoadingFirst.qml` | Skip splash animation in DEV mode (UC_MODEL=DEV). ~8 lines. |
| `src/qml/MainContainer.qml` | Retry timer for ButtonNavigation on startup (~3 lines) — workaround for first-boot focus race. |
| `src/qml/main.qml` | Added `screensaverActive` property, idle timer DEV mode bypass, ScreensaverConfig import. |
| `src/qml/settings/settings/Power.qml` | Added "Screen off animations" settings section (~164 lines) — style picker, master toggle, undocked-fire toggle, measured-dim-phase display. |

---

## v1.4.1 upstream bug-fix: volume OSD feature-check guards

Root-cause fix for a bug where the volume OSD (`VolumeOverlay.qml`) would fire even when the media-player entity had removed `MediaPlayerFeatures.Volume_up_down` from its advertised capability set. `Activity.qml` correctly gated its VOLUME_UP/DOWN handlers; the 7 other call sites did not. Fix wraps each handler with the same `hasFeature()` pattern — no new imports, no behaviour change for entities that legitimately advertise volume control.

### Modified Upstream Files (bug-fix only)
| File | Modification |
|------|-------------|
| `src/qml/components/Page.qml` | Wrapped home-screen fallback VOLUME_UP + VOLUME_DOWN `volume.start(mediaComponentEntity, ...)` call sites with `if (mediaComponentEntity.hasFeature(MediaPlayerFeatures.Volume_up_down)) { ... }`. |
| `src/qml/components/entities/media_player/MediaBrowser.qml` | Same guard wrapping VOLUME_UP + VOLUME_DOWN handlers in the media browser's own `defaultConfig`. |
| `src/qml/components/entities/media_player/deviceclass/Receiver.qml` | Same guard on VOLUME_UP + VOLUME_DOWN overrides. |
| `src/qml/components/entities/media_player/deviceclass/Speaker.qml` | Same. |
| `src/qml/components/entities/media_player/deviceclass/Tv.qml` | Same. |
| `src/qml/components/entities/media_player/deviceclass/Streaming_box.qml` | Same. |
| `src/qml/components/entities/media_player/deviceclass/Set_top_box.qml` | Same. |

---

## v1.4.2: Volume OSD suppression toggle

User-facing complement to v1.4.1's feature-check fix. Adds a `Config.showVolumeOverlay` QSettings-backed toggle (`ui/showVolumeOverlay`, default `true`) so users can globally suppress the volume OSD popup regardless of entity feature advertising. Implementation is a single early-return guard in `VolumeOverlay.qml::start()` — one suppression point for all 16 volume-key call sites. Settings exposed in `Settings → UI → "Show volume overlay"`.

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/qml/components/VolumeOverlay.qml` | Added `import Config 1.0`. Added one-line early-return guard at the top of `start(entity, up = true)`: `if (!Config.showVolumeOverlay) return;`. Short-circuits before any side-effect (no property writes, no `hideTimer.restart()`, no `volume.open()`). Rest of the 199-line Popup unchanged. |
| `src/qml/settings/settings/Ui.qml` | Re-added to modified-upstream after v1.4.0's rebase had it byte-identical to upstream. Appended new "Show volume overlay" toggle block below "Coverflow in media browser"; added `KeyNavigation.down: volumeOverlaySwitch` to the previously-dangling `mediaCoverflowSwitch`; bumped `Flickable.contentY` clamp 1100 → 1260 for the extra ~160 px (restores v1.3.0 value). |

---

## v1.4.3: MediaBrowser robustness hotfix

Fixes a latent upstream bug where a null `entityObj` binding race at open-time caused the MediaBrowser Popup to enter an unescapable loading loop (3-minute global input block via `LoadingScreen`'s `inputController.blockInput(true)` + `timeOutTimer{180 s}`, with continuous 60 fps animation = thermal risk). Root cause captured from live logdy trace 2026-04-24T08:15:50Z; symptom previously known in memory as `project_media_browser_close_loop.md` ("X button dead, remote restart only escape"). v1.4.3 fixes all three failure modes at the right layer.

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/qml/components/entities/media_player/MediaBrowser.qml` | Three targeted changes: **(1) null-guard in `onOpened`** — if `entityObj` is null, log a warning and `Qt.callLater(close)` before touching `browseNav` / `pageLoading` state (prevents the TypeError that triggered the original stuck-state). **(2) Replace `loading.start()` / `loading.stop()` calls with a local `BusyIndicator`** — standard `QtQuick.Controls 2.15`, id `inlineLoading`, centered in `contentItem`, `running: <flag>`. Popup no longer invokes `inputController.blockInput(true)` — X close button / hardware HOME / hardware BACK stay responsive at all times during browse loading. **(3) 15-second `loadingWatchdog` Timer** (`running: isLoading`, declarative property binding) — auto-closes with the standard "Could not load media" warning notification if browse stays pending past the watchdog window. Zero changes to `LoadingScreen.qml` (still used correctly by ~30 unrelated callers across the codebase: Settings / Wifi / docks / integrations / profiles / groups / onboarding). |

---

## v1.4.4: MediaBrowser button expansion + volume split-guard + per-entity OSD flag

Three coupled behavior changes: full hardware-button coverage in MediaBrowser, split-guard refactor of the v1.4.1 volume call sites, and a new per-entity `hideVolumeOverlay` flag that integrations can set via ucapi `options` to suppress the volume OSD on a device-by-device basis. This is the remote-ui half of a two-repo contract — the companion Kodi integration-patch release (`v1.18.13-madalone.2`) reworks `suppress_volume_overlay` to set the new flag instead of stripping `VOLUME_UP_DOWN` features (the original architectural mistake that broke Kodi's actual volume control).

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/ui/entity/mediaPlayer.h` | **Newly modified by this fork as of v1.4.4.** Adds `Q_PROPERTY(bool hideVolumeOverlay ...)` (in the `// options` block alongside existing `volumeSteps`), the companion getter `getHideVolumeOverlay()`, a `hideVolumeOverlayChanged()` signal, an `m_hideVolumeOverlay = false` member, and declares `bool updateOptions(QVariant data) override;` to enable runtime option hot-updates (Base class stub was a no-op — this override was missing entity-wide, bonus fix). |
| `src/ui/entity/mediaPlayer.cpp` | **Newly modified by this fork as of v1.4.4** (in addition to the existing 2-line image-redownload bugfix already listed in Shared Infrastructure). Adds constructor options ingest (`m_hideVolumeOverlay = options.value("hide_volume_overlay", false).toBool();` alongside the existing `volume_steps` / `simple_commands` reads) and the `MediaPlayer::updateOptions(QVariant)` implementation that emits `hideVolumeOverlayChanged()` on flip. |
| `src/qml/components/VolumeOverlay.qml` | Second guard added inside `start(entity, up)`: `if (entity && entity.hideVolumeOverlay) return;` — placed after v1.4.2's `if (!Config.showVolumeOverlay) return;` global master. Precedence: OSD is hidden if EITHER says hide. Null-guard on `entity` is defensive (start() is a public function). |
| `src/qml/components/entities/media_player/MediaBrowser.qml` | Four additions. (1) Six new action-map entries in `buttonNavigation.defaultConfig`: `MUTE` (unguarded), `STOP` (gated on `MediaPlayerFeatures.Stop`), `NEXT` (prefers `Fast_forward` → falls back to `Next`), `PREV` (prefers `Rewind` → falls back to `Previous`), `CHANNEL_UP` / `CHANNEL_DOWN` (with `pressed` + `pressed_repeat`, delegating to the new helpers below). (2) Three helper functions near `loadMore` — `pageScrollIncrement(lv)` computes items-per-page from `contentHeight` / item count, `pageScrollUp()` / `pageScrollDown()` call `positionViewAtIndex(newIndex, ListView.Beginning)` for snap-to-item page jumps. (3) Split-guard of existing `VOLUME_UP` / `VOLUME_DOWN` action-map entries: command (`volumeUp/Down()`) extracted outside the `hasFeature` block; only `volume.start()` stays gated. (4) No null-guard edits — v1.4.3's `onOpened` null-guard ensures `takeControl()` never runs when `entityObj` is null, so all new handlers inherit that protection transitively. |
| `src/qml/components/Page.qml` | Split-guard refactor at the two volume call sites (`VOLUME_UP` / `VOLUME_DOWN` in the Media_player branch ~lines 337-342 and 357-362): `mediaComponentEntity.volumeUp/Down()` extracted outside the `hasFeature` block; `volume.start(mediaComponentEntity)` stays gated. Activity-branch path (via `triggerCommand`) unchanged — it was already unconditional. |
| `src/qml/components/entities/media_player/deviceclass/Tv.qml` | Split-guard refactor of the `VOLUME_UP` / `VOLUME_DOWN` entries in `overrideConfig` (~lines 203-218). Same pattern as Page.qml. |
| `src/qml/components/entities/media_player/deviceclass/Set_top_box.qml` | Same split-guard refactor (~lines 203-218). |
| `src/qml/components/entities/media_player/deviceclass/Streaming_box.qml` | Same split-guard refactor (~lines 203-218). |
| `src/qml/components/entities/media_player/deviceclass/Receiver.qml` | Same split-guard refactor (~lines 203-218). |
| `src/qml/components/entities/media_player/deviceclass/Speaker.qml` | Same split-guard refactor (~lines 203-218). |

**Intentionally NOT modified:** `src/qml/components/entities/activity/deviceclass/Activity.qml` — already architecturally correct (`activityBase.triggerCommand()` fires unconditionally outside the `hasFeature` block wrapping only `volume.start()`). Verified by direct read during v1.4.4 implementation; research agent's earlier "structurally identical to v1.4.1 additions" classification was incorrect for this specific file.

**Intentionally preserved:** `Config.showVolumeOverlay` (v1.4.2 — `src/config/config.{h,cpp}`, `Settings → UI` toggle in `Ui.qml`). Per-entity `hideVolumeOverlay` is an ADDITIVE layer, not a replacement. Owner confirmed: global master stays as a catch-all coarse control; per-entity flag is for surgical control when only specific devices should skip the OSD (e.g., Kodi has its own on-screen OSD while Sonos/LG want UC's).

---

## v1.4.10: entity_change apply gap fix — NEW events now populate m_entities

Pre-existing latent upstream bug: `core::Api::entityAdded` signal is emitted by `processEntityChange` for `MsgEventTypes::NEW` events at `core.cpp:2142`, but no consumer was ever connected to it (verified via grep). After an integration uninstall→reinstall cycle, the integration's NEW events re-create entities in the core, the wire delivers them to the firmware, the core-API GET shows the populated state — but `EntityController::onEntityChanged()` then hits the `m_entities.contains(entityId)` early-return at `entityController.cpp:430` for every subsequent CHANGE and silently drops the update. User-visible: blank artwork / stale title / frozen state on the activity card until close+reopen (which works because `Activity.qml`'s `includedEntityItem` delegate calls `EntityController.load(entityId)`).

Three-fix bundle: connect the orphan signal (primary), add a load-fallback in the silent-return path (backstop), and align `MediaComponent.qml` with the load+`entityLoaded` pattern already used by `SensorWidget` / `SelectWidget` / `Activity.qml.includedEntityItem` (defense-in-depth).

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/ui/entity/entityController.h` | **Newly modified by v1.4.10.** Adds `void onEntityAdded(core::Entity entity);` slot declaration in the `public slots:` block alongside existing `onEntityChanged` / `onEntityDeleted`. |
| `src/ui/entity/entityController.cpp` | **Newly modified by v1.4.10.** Three coherent additions: **(1)** ctor `connect()` for `core::Api::entityAdded → EntityController::onEntityAdded` (alongside existing `entityChanged` / `entityDeleted` connects). Closes the orphan-signal hole — NEW events now populate `m_entities` via `addEntityObject` (which is idempotent on already-loaded entities). **(2)** New `EntityController::onEntityAdded(core::Entity entity)` slot — single-line implementation that delegates to `addEntityObject(entity)` with a `qCDebug` breadcrumb. **(3)** `EntityController::onEntityChanged()` early-return at line 430 replaced with `qCWarning` + synchronous `load(entityId)` + return. Defends against integrations emitting CHANGE without prior NEW (spec violation observed in practice) and against reconnect races where `m_entities` was just cleared in `onCoreConnected`. Multiple in-flight loads for the same id are safe — `addEntityObject` short-circuits on the second response. No other changes; no new strings; no behavior drift on the happy path. |
| `src/qml/components/entities/activity/MediaComponent.qml` | **Further extended by v1.4.10** (in addition to v1.4.8 `Config.show*Button` bindings and v1.4.9 `controlsContainerHeight` collapse expression). `Component.onCompleted` and `onEntityIdChanged` refactored into a shared `ensureEntityLoaded()` helper that calls `EntityController.get(entityId)` first, then `EntityController.load(entityId)` if `get` returned null and the id is non-empty. New `Connections { target: EntityController; ignoreUnknownSignals: true; function onEntityLoaded(success, loadedId) }` block re-acquires `entityObj` once the load lands, gated on `loadedId === mediaComponent.entityId` so the single global `entityLoaded` signal doesn't cross-fire across MediaComponent instances. Pattern matches `SensorWidget.qml:38-46` + `SensorWidget.qml:122-134`. Existing `Connections { target: entityObj }` block (state-changed scale animation) preserved verbatim — added the new block as a sibling. |

**Intentionally NOT modified:** `src/core/core.cpp` (the orphan emit at line 2142 is upstream-correct — the consumer was just missing). `src/core/core.h` (`entityAdded` signal declaration is fine as-is). The 5 device-class detail pages (`Tv.qml` / `Set_top_box.qml` / `Streaming_box.qml` / `Receiver.qml` / `Speaker.qml`) — they receive `entityObj` directly from `Activity.qml`'s `loadThirdContainer({entityObj: entity})` push, never look up entities by id, so they were never affected by this gap. `Page.qml` (`get`-only at line 298 is fine for the home-screen activities list — those entities are loaded via the activities-bar pre-load path). `SensorWidget.qml` / `SelectWidget.qml` — already correctly patterned, used as the reference.

---

## v1.4.9: MediaBrowser → Player Widget thumbnail preview handoff + setPreviewImage scheme filter + empty controls-bar auto-collapse

Browse-time thumbnails now render on the player widget immediately after `playMedia()`, bridging the gap before the integration's `Player.GetItem` art response (which often returns nothing useful for unscraped library files, video-source SMB/NFS content, or plugin items — Netflix / Movistar+ / Amazon / Filmin). Pure FW-side; zero ucapi contract change; companion to but independent from Kodi integration-patch `v1.18.13-madalone.2`'s patch 28 broader art-key fallback. Bundled with: the activity-card media-player controls `RowLayout` now auto-collapses when all four v1.4.8 button toggles are off, eliminating the empty 80 px reserved row (no new master toggle — reuses the existing four).

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/qml/components/entities/activity/MediaComponent.qml` | **Further extended by v1.4.9** (in addition to v1.4.8 `visible:` bindings on shuffle / repeat and `Config.show*Button` gate additions on browser / source-picker). `controlsContainerHeight` readonly property (line 50) now evaluates to 0 when all four `Config.show{Shuffle,Repeat,MediaBrowser,MediaSource}Button` are false (even if the component height is ≥ 320 px), so the whole controls `RowLayout` collapses cleanly instead of reserving an empty 80 px row below the progress bar. Flows naturally through the existing `mediaInfoHeight` calc at line 51 (auto-reclaims the space for title / progress block). Single-expression change — no new settings, no new qsTr strings. |
| `src/ui/entity/mediaPlayer.h` | **Newly extended by v1.4.9** (in addition to v1.4.4 `hideVolumeOverlay`). Adds `Q_INVOKABLE void setPreviewImage(const QString &thumbnailUrl)` public declaration alongside existing `playMedia` / `clearPlaylist`; three private additions adjacent to existing `m_mediaImageDownloadTries`: `static bool isKodiDefaultPlaceholder(const QString &url)`, `void applyMediaImageUrl(const QString &newImageUrl, bool isPreview)`, and `bool m_mediaImageIsPreview = false` member. `playMedia()` signature unchanged — no ucapi contract impact. |
| `src/ui/entity/mediaPlayer.cpp` | **Newly extended by v1.4.9** (in addition to the Shared-Infrastructure image-redownload bugfix, the v1.4.4 `hideVolumeOverlay` options ingest, and the v1.4.6 `OperationCanceledError` filters). Five coherent additions: **(1)** `isKodiDefaultPlaceholder(url)` static helper — case-insensitive substring match against 12 canonical Kodi skin default PNG filenames; handles both plain (`DefaultVideo.png`) and URL-encoded (`image%3A%2F%2FDefaultVideo.png%2F`) forms. **(2)** `applyMediaImageUrl(url, isPreview)` private helper — refactors the existing `Media_image_url` attribute case's HTTP/base64 dispatch into a shared path; `Media_image_url` case now calls this with `isPreview=false`. **(3)** `setPreviewImage(url)` public Q_INVOKABLE — whitelists fetchable schemes (`http(s)://`, `data:image/…;base64,…`) and rejects UC3 `icon://` / Kodi `image://` / non-image data URIs early (prevents `ProtocolUnknownError` + 3-retry burn on MediaBrowser thumbnail fallbacks), skips empty/placeholder inputs, skips no-op if same URL already loaded (but still marks the preview flag so a later placeholder swap is guarded), otherwise calls `applyMediaImageUrl(url, true)`. **(4)** Preview-preserve guard at top of `Media_image_url` attribute case — early-returns (leaves preview intact) when `m_mediaImageIsPreview && (newImageUrl.isEmpty() \|\| isKodiDefaultPlaceholder(newImageUrl))`. **(5)** Retry-exhausted branch of `onNetworkRequestFinished()` now skips `clearMediaImageState()` when a preview is still showing. `m_mediaImageIsPreview = false` added to state→Off block and at the end of `clearMediaImageState()` to keep the flag coherent with its image state. v1.4.6 `OperationCanceledError` filters preserved verbatim. |
| `src/qml/components/entities/media_player/MediaBrowser.qml` | **Further extended by v1.4.9** (in addition to v1.4.3 null-guard + BusyIndicator + 15 s watchdog, and v1.4.4 full button coverage + page-scroll helpers). `buildPlayMenu(mediaId, mediaType, actions)` now accepts an optional 4th `thumbnail` param — closes over `th` in each of its 3 callback IIFEs and forwards as the 4th arg to `requestPlayMedia`. `requestPlayMedia(mediaId, mediaType, action)` now accepts an optional 4th `thumbnail` param — before dispatching `entityObj.playMedia(...)`, calls `entityObj.setPreviewImage(thumbnail)` if both sides are non-null. 9 call-site updates (post-edit lines 362, 952, 964, 982, 1071, 1105, 1318 + 2 `buildPlayMenu` build sites at 952 / 1318) thread `modelData.thumbnail` / `pageContainer.thumbnail` / `item.thumbnail` into the new 4th param. Zero impact on v1.4.4's 6 new button bindings or v1.4.3's open-time guards; zero new `qsTr(...)` strings. |

**Intentionally NOT modified:** `src/qml/components/entities/media_player/ImageLoader.qml` (already correct — `cache: false` + dual-Image opacity fade handles transient preview overwrite safely; no code path assumes URL stability). `src/qml/components/entities/activity/MediaComponent.qml` and the 5 device-class detail pages (`Tv.qml` / `Set_top_box.qml` / `Streaming_box.qml` / `Receiver.qml` / `Speaker.qml`) — all bind `url: entityObj.mediaImage` directly, so a single C++ `m_mediaImage` write propagates to every consumer automatically. **Not extended:** ucapi spec (`entity_media_player.md`) — no `preview_image` field added; this is a pure FW-side fix layered on top of the existing `media_image_url` path.

---

## v1.4.8: Touchbar sensitivity tuning + media-button suppression toggles

Two independent additive changes bundled.

**Touchbar sensitivity** (1 file, 2 edits): `src/qml/components/ChargingScreen.qml` screensaver touchbar speed/density control scaled by `/ 3` — was 1:1 pixel-to-unit (twitchy at the physical slider size), now ~1:3 so the full 10→100 sweep happens over ~270 px instead of ~90 px.

**Media-button suppression toggles** (parallel to v1.4.2 `Config.showVolumeOverlay` pattern): 4 new global Q_PROPERTYs in `Config` (`showShuffleButton` / `showRepeatButton` / `showMediaBrowserButton` / `showMediaSourceButton`), default `true` — one-line `visible:` bindings at each icon in `MediaComponent.qml`, four `Settings → UI` Switch rows for discoverability. Motivation: Kodi integration fork can't selectively strip individual `MediaPlayerFeatures` bits to hide just one of the 4 controls-row icons — UC-side config toggles solve it at the display layer. Invisible children of the RowLayout collapse automatically via `Layout.fillWidth: true` (same mechanism upstream's browser/source `visible:` gates relied on). Global master is strictly additive to `entityObj.hasFeature(...)` checks on browser/source icons — never unhides a button the entity chose not to expose.

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/qml/components/ChargingScreen.qml` | **Extended by v1.4.8.** Touchbar speed/density handler (lines ~645-667, previously added in v1.2.0 and refined through v1.4.x) now scales `delta` by `/ 3` before applying to `ScreensaverConfig.matrixSpeed` / `ScreensaverConfig.starfieldDensity`. Minimum-movement 3 px dead zone unchanged. One new local `scaledDelta` var, one new comment — zero architectural impact. |
| `src/config/config.h` | **Extended by v1.4.8** (in addition to v1.4.2 `showVolumeOverlay`). 4 new `Q_PROPERTY(bool show{Shuffle,Repeat,MediaBrowser,MediaSource}Button ...)` declarations adjacent to `showVolumeOverlay`, 4 getter + 4 setter method decls in the Q_PROPERTY methods block, 4 NOTIFY signal decls. Follows identical naming/placement pattern as the v1.4.2 additions. |
| `src/config/config.cpp` | **Extended by v1.4.8.** 4 getter (`m_settings->value("ui/show*Button", true).toBool()`) + 4 setter (`m_settings->setValue(...); emit ...Changed();`) implementations appended directly after `setShowVolumeOverlay()`. Identical shape to the v1.4.2 `getShowVolumeOverlay` / `setShowVolumeOverlay` pair. |
| `src/qml/components/entities/activity/MediaComponent.qml` | **Newly modified by v1.4.8.** Added `import Config 1.0` alongside existing imports. Added `visible: Config.showShuffleButton` to the shuffle `Components.Icon` and `visible: Config.showRepeatButton` to the repeat `Components.Icon` (neither had a `visible:` binding previously — upstream's 4-icon row relied on the parent Rectangle's `visible: controlsContainerHeight > 0 && mediaTitle.visible` gate). Modified the existing `visible:` bindings on the browser and source-picker icons: browser now reads `Config.showMediaBrowserButton && (entityObj.hasFeature(Browse_media) \|\| entityObj.hasFeature(Search_media))`; source-picker now reads `Config.showMediaSourceButton && entityObj.hasFeature(Select_source) && entityObj.sourceList.length !== 0`. Global toggle AND-chains with existing feature-capability checks — never forces a hidden button visible. |
| `src/qml/settings/settings/Ui.qml` | **Extended by v1.4.8** (in addition to v1.4.2 `showVolumeOverlay` toggle row). 4 new `ColumnLayout` toggle rows appended below the volumeOverlay row, each preceded by a divider `Rectangle` and built on the exact volumeOverlay template — label + Switch + helper Text, wired to the 4 new Config properties. `volumeOverlaySwitch` given a `KeyNavigation.down: shuffleButtonSwitch` (previously terminal); KeyNavigation chain extends through shuffle → repeat → mediaBrowser → mediaSource (terminal). `Flickable.contentY` clamp bumped 1260 → 1900 for the ~640 px of added content (4 × ~160 px per toggle, same scaling as v1.4.2's 1100 → 1260). |

---

## v1.4.6: Quiet boot hygiene pass

Four independent, low-risk fixes surfaced by v1.4.5 smoke-test logdy analysis. None are v1.4.5 regressions — all pre-date v1.4.4 and had gone uncaptured due to logdy reconnection timing on UI restarts. Boot-log warning count drops from ~177 to ≤ 4 (94% reduction); one of the four fixes is also a silently-broken functional wiring repair, not just cosmetic.

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/ui/entity/mediaPlayer.cpp` | **Newly extended by v1.4.6** (in addition to the Shared-Infrastructure image-redownload bugfix and the v1.4.4 `hideVolumeOverlay` options ingest). Added `QNetworkReply::OperationCanceledError` filter at **two** log sites: (a) early-return in `onNetworkError()` before the WARN log (this is the source of the 167× per-boot "Image download network error: 0 QNetworkReply::OperationCanceledError" flood), and (b) early-return in `onNetworkRequestFinished()` error branch before the retry-counter bump + WARN log (cancels are supersession events, not failures — counting them against the 3-retry budget was a latent correctness bug). Idiomatic Qt handling per the QNetworkReply docs (cancels also include `setTransferTimeout()` timeouts). No behavior change for honest 4xx/5xx/timeout/network errors. |
| `src/qml/components/VoiceOverlay.qml` | **Newly modified by v1.4.6.** Added terminal `return "";` fallback after the outer `if` in `assistantProfileNameText.text:` JS binding (around line 677). Without it, when `voice.voiceEntityObj` is null (common during the voice-session entity-resolution window), the function fell off the end returning `undefined`, producing a `qrc:/components/VoiceOverlay.qml:666: Unable to assign [undefined] to QString` warning on every voice session open. Defensive — visual output was already empty (undefined rendered as "") so user-visible behavior is unchanged. |
| `src/qml/main.qml` | **Extended by v1.4.6** (in addition to the existing screensaver plumbing). Added `import TouchSlider 1.0` to the singleton-imports block. Line 507's `Connections { target: TouchSliderProcessor }` had been referencing a singleton whose module import was missing — `ignoreUnknownSignals: true` silently masked the failure, meaning the "physical slider resets idle timer" wiring (comment at line 502-505) had never actually been connected on UC3. Fixing the import should surface `TouchSlider::touchPressed` signal delivery; if the signal doesn't fire on physical slider contact at the hardware level, the feature will remain silently broken (follow-up investigation needed at that point). |
| `src/ui/soundEffects.cpp` | **Newly modified by v1.4.6.** Added `#include <QFileInfo>`; initialized all 5 `QSoundEffect*` members to `nullptr` in the ctor init list; added empty-path guard + file-existence guard in `createEffects()` (new `makeEffect` lambda skips `setSource` when the path is empty or the file doesn't exist, logs at DEBUG) and null-guards around every `m_effect*->setVolume()` / `->play()` call in `play()`'s switch. Prevents 5× "QSoundEffect(qaudio): Error decoding source file:///*.wav" warnings when `UC_SOUND_EFFECTS_PATH` env var is unset (dev env) or firmware-provided sound files are missing. Matches Qt docs recommendation for QSoundEffect lifecycle (verify source path / status before playback). Happy-path behavior unchanged when the env var is set and wav files exist. |

**Out of scope (explicitly deferred):** Warnings 5 (`uc.ui.resources: Empty ID passed to getIcon()`, 2×/boot, cosmetic) and 6 (`uc.app.i18n: Failed to remove translation`, 1×/boot, first-boot-only) are pure log-level-downgrade candidates with +1 upstream drift each and no functional value — skipped to honor `CLAUDE.md` §10 ("Minimize the diff against upstream to ease future merges"). Warning 7 (`Cannot find EGLConfig`) is Qt 5.15 internal and out of reach.

---

## Mod 3: Detail Page Battery Chip

**v1.4.0 update (2026-04-23 — upstream v0.72.0 merge):** UC independently shipped the same feature as "Show battery indicator everywhere" with a different property name and layout approach. **Option B rebase** applied: adopted upstream's public API (`Config.showBatteryEveryWhere`, QSettings key `ui/batteryEveryWhere`, Settings → UI toggle wording) while keeping our superior Option A chain-anchoring `RowLayout` in `BaseDetail.qml`. One-shot migration helper (`main.cpp::migrateLegacySettings`) preserves v1.3.0 user state. Upstream's inline battery `Row` additions in `BaseTitle.qml` / `Activity.qml` were rejected during merge — we already render the chip via `BatteryStatusChip.qml` through a Loader in the consolidated status strip, accepting both would duplicate renders. Post-merge, `Ui.qml` is now byte-identical to upstream (our toggle was replaced by upstream's; dropped from the table below). Config property / QSettings key / toggle row no longer count as our custom additions on `config.h` / `config.cpp` / `Ui.qml`.

### Custom QML Files
| File | Purpose |
|------|---------|
| `src/qml/components/overlays/BatteryStatusChip.qml` | Compact battery chip mirroring StatusBar visual (bolt + percentage when charging, 16×30 bar + optional percentage otherwise). Touch-transparent (no MouseArea). Bound directly to `Battery` and `Config` singletons. Post-v1.4.0 loaded via `BaseDetail.qml:322` with `active: Config.showBatteryEveryWhere`. |

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/qml/components/entities/BaseDetail.qml` | Added `import Config 1.0`, `import QtQuick.Layouts 1.15`, `import Wifi 1.0`, `import Wifi.SignalStrength 1.0`, `import SoftwareUpdate 1.0`. Added `readonly property bool _wifiWarningActive` (single source of truth for the WiFi-warning predicate). **2026-04-22 Option A consolidation:** standalone `iconIntegrationDisconnected` + `batteryChipLoader` blocks replaced by a single `RowLayout` (`id: titleStatusStrip`) anchored `right: iconClose.left; rightMargin: 10; verticalCenter: iconClose.verticalCenter`, `spacing: 5`, `z: 1001`, containing 6 children in L-to-R declaration order: integration loading spinner (animated, `ui.isConnecting`), 12×12 red core-disconnected dot (`!ui.coreConnected`), yellow `uc:cloud-arrow-down` software-update icon (`SoftwareUpdate.updateAvailable`), WiFi warning (detail-page-wider predicate), per-entity `uc:link-slash`, battery chip Loader. All children use `Layout.alignment: Qt.AlignVCenter` + `Layout.preferredWidth` that collapses to 0 when hidden. The battery chip is now the persistent fixed rightmost anchor; warnings shift around it via the Qt Layout solver. **v1.4.0:** Loader `active:` binding renamed `Config.showBatteryOnDetailPages` → `Config.showBatteryEveryWhere`. |
| `src/qml/components/entities/BaseTitle.qml` | **2026-04-22 Option A:** deleted the 34-line WiFi warning block (outer `Components.Icon` at lines 39-72 + inner weak-icon overlay + red strikethrough `Rectangle`) and the now-orphaned `import Wifi 1.0` + `import Wifi.SignalStrength 1.0`. `BaseDetail.qml` is now the single source of truth for detail-page WiFi warning rendering. User-authorized deletion per §1.3. **v1.4.0:** upstream added a battery `Row` here for their "Show battery everywhere" feature — rejected during merge (we render via `BaseDetail.qml`'s Loader; accepting would duplicate). |
| `src/qml/components/entities/activity/deviceclass/Activity.qml` | **2026-04-22 Option A:** deleted the 34-line duplicate WiFi warning block (lines 445-478) from inside the custom title Rectangle + the now-orphaned `import Wifi 1.0` + `import Wifi.SignalStrength 1.0`. Rest of the title block (activity icon, name text, "Tap for more" subtitle, menu-toggle `HapticMouseArea`) unchanged. User-authorized deletion per §1.3. **v1.4.0:** upstream added a battery `Row` here too — rejected during merge (same reason as `BaseTitle.qml`). |
| `resources/qrc/main.qrc` | Registered `components/overlays/BatteryStatusChip.qml`. |

---

## Documentation & Test Files (not compiled)

| File | Purpose |
|------|---------|
| `CLAUDE.md` | Claude Code operations manual |
| `STYLE_GUIDE.md` | Coding and architecture standards |
| `docs/CUSTOM_FILES.md` | This file — custom vs upstream manifest |
| `SCREENSAVER-IMPLEMENTATION.md` | Screensaver system design document |
| `test_themes.qml` | Theme testing harness |
| `matrix-charging-screen.tar.gz` | Archived screensaver build |
| `matrix-charging-screen-nofont.tar.gz` | Archived build (no font) |
| `matrix-charging-screen-pre-refactor.tar.gz` | Archived pre-refactor build |
| `matrix-nofont.tar.gz` | Archived matrix build |
