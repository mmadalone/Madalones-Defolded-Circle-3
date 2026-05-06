# Custom Files Manifest — UC Remote 3 Mods

Tracks every file that is custom (added by madalone) or modified from the upstream `unfoldedcircle/remote-ui` codebase. If a file is not listed here, it is upstream and should not be modified without explicit justification.

**Upstream base:** `v0.71.1`  
**Last updated:** 2026-05-02 (v1.4.38 — test-only CI fix for v1.4.37 SignalSpy regression; binary identical to v1.4.37)

> **Note on currency:** the per-Mod sections below reflect state through v1.4.11. Full per-file detail for v1.4.12 → v1.4.36 lives in CHANGELOG.md and the "Mod 4 (WiFi UX)" + "Mod 5 (Active Session Keeper)" + "Mod 6 (Phantom-Wake Suppressor)" sections in `CLAUDE.md`. Quick summary at the bottom of this file ("v1.4.12+ deltas").

> **Note on size figures (v1.4.28):** The `~Lines` columns in the per-file tables below were mechanically synced against `wc -l` on 2026-05-01 per `audit-v1.4.26-thorough.md` Findings 7 + 11. Pre-v1.4.28 figures were stale — some by factors of 2-7x — because the manifest tracked file sizes by hand, not by re-counting. Going forward, run `wc -l <file>` whenever a file gets a substantive edit, and update the matching row here. The QML files (which still use `(~N)` parenthetical figures elsewhere) are not part of this sync.

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
| `remote-ui.pro` | Added custom HEADERS/SOURCES at end of lists (matrixrain, screensaverconfig, rainsimulation, gravitydirection, glitchengine, messageengine, glyphatlas, matrixrain/layerpipeline, matrixrain/atlasbuilder, matrixrain/singlelayerrenderer, matrixrain/inputhandler, matrixrain/bindinghelper). |
| `resources/qrc/main.qrc` | Registered all custom QML files and settings sub-pages. |

---

## Mod 1: Screensaver System

### Custom C++ Files
| File | Purpose | Lines |
|------|---------|-------|
| `src/ui/matrixrain.h` | `MatrixRainItem` QQuickItem — GPU-accelerated matrix rain renderer | 603 |
| `src/ui/matrixrain.cpp` | QSG rendering orchestrator, vertex buffer, atlas upload, multi-layer dispatch. Single-layer rendering, input dispatch, and ScreensaverConfig wiring delegated to helper classes (v1.4.36) | 746 |
| `src/ui/matrixrain/layerpipeline.h` | `LayerPipeline` — multi-layer rain pipeline (3 depth planes), shared render primitives (`MatrixRainVertex`, `emitQuad`, `packColor`, etc.) | 244 |
| `src/ui/matrixrain/layerpipeline.cpp` | LayerPipeline build/sync/render implementation, multi-layer atlas cache | 661 |
| `src/ui/matrixrain/atlasbuilder.h` | `AtlasBuilder` — single-layer atlas builder + canonical SHA-1 cache-key hashing (deduped across single + multi-layer paths). Defines shared `AtlasInputs` struct. | 65 |
| `src/ui/matrixrain/atlasbuilder.cpp` | Class-static single-layer cache (`s_singleCacheKey`, `s_singleCacheAtlas`), `buildSingle`, `cacheKey` | 47 |
| `src/ui/matrixrain/singlelayerrenderer.h` | `SingleLayerRenderer` — single-layer render path (countVisibleQuads + 5 render helpers extracted from MatrixRainItem in v1.4.36 Phase A). Stateless — all state passed by parameter. By-value member of MatrixRainItem. | 87 |
| `src/ui/matrixrain/singlelayerrenderer.cpp` | Stateless render implementation: stream trails → residual cells → glitch trails → message flash → message overlay. Mirrors LayerPipeline pattern (pure C++, no Qt object system). | 379 |
| `src/ui/matrixrain/inputhandler.h` | `InputHandler` (QObject) — input dispatch + enter-button state machine (extracted in v1.4.36 Phase B). Owns 2 QTimers (300 ms double-tap, 500 ms hold). Friend of MatrixRainItem. Forwards `enterAction(QString)` signal to parent for QML contract preservation. | 74 |
| `src/ui/matrixrain/inputhandler.cpp` | EnterIdle/Pressed/Held state machine + interactiveInput dispatch + handleDirection/Enter/Slow/Restore/Tap implementations. | 254 |
| `src/ui/matrixrain/bindinghelper.h` | `BindingHelper` (all-static utility class) — 8 ScreensaverConfig binding helpers (extracted in v1.4.36 Phase C): bindAppearance, bindDirectionAndGravity, bindGlitch, bindChaos, bindTap, bindMessages, bindSubliminal, bindDepthAndLayers. Mirrors AtlasBuilder shape (no instances, no QObject). | 38 |
| `src/ui/matrixrain/bindinghelper.cpp` | 8 binding helper implementations. Wrapped in `#ifndef MATRIX_RAIN_TESTING` so test builds compile to empty TU. Caller (`MatrixRainItem::bindToScreensaverConfig`) owns QSignalBlocker batching scope. | 199 |
| `src/ui/rainsimulation.h` | `RainSimulation` — stream-based rain simulation engine | 419 |
| `src/ui/rainsimulation.cpp` | Stream lifecycle, head advance, float movement, density management | 706 |
| `src/ui/glitchengine.h` | `GlitchEngine` — char swap, brightness flash, column flash, stutter, reverse glow | 197 |
| `src/ui/glitchengine.cpp` | Per-tick glitch application, rate control | 394 |
| `src/ui/messageengine.h` | `MessageEngine` — on-screen text rendering in character grid | 168 |
| `src/ui/messageengine.cpp` | Text layout, character placement | 358 |
| `src/ui/gravitydirection.h` | `GravityDirection` — direction system, auto-rotate (most logic now inline in header — `.cpp` is a near-empty shell after the inline-implementation refactor) | 50 |
| `src/ui/gravitydirection.cpp` | Angle sweep, per-stream lerp, travel/spread axis math (most code moved into the header during a prior refactor; this `.cpp` is now a thin shell) | 28 |
| `src/ui/screensaverconfig.h` | `ScreensaverConfig` singleton — owns QSettings, SCRN_* macros, **136 properties total** (60 `SCRN_BOOL` + 47 `SCRN_INT` + 18 `SCRN_STRING` macros = 125 macro-emitted Q_PROPERTYs, plus 5 hand-written raw-int dual-emit setters and 6 transformed read-only properties = 11 explicit `Q_PROPERTY` declarations) | 313 |
| `src/ui/screensaverconfig.cpp` | QSettings init, transformed getters, Battery deferred connect | 142 |
| `src/ui/screensaverconfig_macros.h` | SCRN_BOOL/INT/STRING read-write property macros | 58 |

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
| `deploy/config/click.wav` `click_lo.wav` `confirm.wav` `error.wav` `zap_future.wav` | **v1.4.41.** CC0 substitute UI sound effects, 16-bit PCM stereo at 44.1 kHz, ~140 KB total. Bundled because firmware 2.9.2 dropped `UC_SOUND_EFFECTS_PATH`; `uiController.cpp` falls back to `$UC_CONFIG_HOME` (= this dir) when the env is empty. Stereo required — UCR3's ALSA backend rejects mono with `snd_pcm_hw_params_set_channels: err = -22`. Generated by `tools/gen_sounds.py` (regen any time; replaceable with stock UC originals if obtained, same filenames). Flat layout — `./config/` cannot contain sub-directories per the install endpoint's archive validator. |
| `deploy/config/zap_arpeggio.wav` `zap_bell.wav` `zap_dyad.wav` `zap_synthwave.wav` `zap_strike.wav` | **v1.4.42.** Five additional CC0 synth dock-chime variants (`Ascend`, `Bell`, `Chord`, `Pulse`, `Zap`). Same 44.1 kHz stereo format as v1.4.41 chimes. Selected via `Settings → Sound → Dock chime` picker (variants 2-6; variant 1 is `zap_future.wav`). Generated by `tools/gen_sounds.py` `chime_arpeggio()` / `chime_bell()` / `chime_dyad()` / `chime_synthwave()` / `chime_strike()` functions. ~315 KB total. |
| `deploy/config/power_down.wav` `power_hold_and_off.wav` `power_up1_clean.wav` `power_up2_clean.wav` `tos_bridge_loss_power.wav` `tos_bridge_loss_power_shorter.wav` | **v1.4.42 (TOS short refreshed v1.4.43).** Six user-curated dock-chime sources processed through `tools/gen_sounds.py::process_user_wav()` for loudness normalization. 16-bit stereo at 48 kHz (Qt 5.15 QSoundEffect handles mixed rates fine on UCR3 — empirically verified post-deploy). Selected as variants 7-12 in the picker. Pipeline: `pre_gain=1.5 → tanh(drive=3.5) → peak-normalize-to-1.0` (more aggressive than synth-chime pipeline at 1.2/2.2 — user-curated audio tolerates harder saturation without harmonic-distortion artifacts). ~2.2 MB total. |
| `chimes/*.wav` (6 source files) | **v1.4.42.** Pristine source wavs from which `deploy/config/power_*.wav` and `deploy/config/tos_*.wav` are processed. Tracked in git as the canonical chime sources — re-run `python tools/gen_sounds.py` to regenerate processed copies if these are updated. |
| `tools/gen_sounds.py` | **v1.4.41 (extended v1.4.42 + v1.4.43).** Numpy-based wav generator + processor. Synth chimes 1-6 generated from scratch. User wavs (variants 7-12) read from `chimes/` and processed via `process_user_wav()` (stereo de-interleave → mono mix → pre-gain → tanh saturation → peak-normalize → re-interleave to stereo at source rate). Output goes directly to `deploy/config/`. ~250 LOC. v1.4.43: refreshed TOS-short source (1.86 → 1.24 s). |

### Modified Upstream QML
| File | Modification |
|------|-------------|
| `src/qml/components/ChargingScreen.qml` | Replaced stock analog clock with theme Loader system. Added ButtonNavigation for interactive screensaver controls. Added displayOff propagation. |
| `src/qml/settings/settings/ChargingScreen.qml` | Replaced stock charging settings with full screensaver configuration (theme selector + sub-pages). 2026-04-14: theme sub-pages wrapped in deferred Loaders via `sourceComponent:` + inline `Component { }` to fix settings-open freeze. |
| `src/qml/settings/Settings.qml` | Added "Screensaver" menu entry. |
| `src/qml/components/Switch.qml` | Added `Keys.onReturnPressed`/`onEnterPressed` for DPAD center toggle. |
| `src/qml/components/Slider.qml` | **v1.4.33:** tick `Repeater.model` gated by `showTicks` (`showTicks ? to-from+1 : 0`) — the wrapping Row's `visible: showTicks` only gated rendering, not instantiation, causing the upstream Slider to create thousands of invisible tick Rectangles when `to-from` was large. Power.qml's 100–5000 grace slider alone produced ~4,900 invisible Rectangles + O(N²) Row positioner reflows, blocking Settings → Power open for ~70 s on UC3 ARM. One-line change. |
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

## v1.4.11: Audit-driven hardening — timer displayOff gate + entity-leak deferred-delete + toolchain digest pin

Three audit-driven fixes from the v1.4.10-baseline codebase audit. **Drift increase: zero** — all three changes touch files already in the modified-upstream / custom-file manifests.

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/ui/entity/entityController.h` | **Further extended by v1.4.11** (in addition to v1.4.10's `onEntityAdded` slot decl). Adds `void clearEntitiesDeferred();` private helper declaration alongside existing `onEntityChanged` / `onEntityAdded` / `onEntityDeleted`. Doc comment notes the per-reconnect leak the helper plugs and the 100 ms defer rationale (matches `onEntityDeleted:520-530` precedent). |
| `src/ui/entity/entityController.cpp` | **Further extended by v1.4.11** (in addition to v1.4.10's `entityAdded` connect + late-load fallback). New `EntityController::clearEntitiesDeferred()` (~10 lines) iterates `m_entities.values()`, captures pointers by value into a lambda, schedules `deleteLater()` on each via `QTimer::singleShot(100, this, ...)`, then `m_entities.clear()`. Both `onCoreConnected()` and `onCoreDisconnected()` updated to call the helper instead of bare `m_entities.clear()`. Plain clear was the bug — entity QObjects are children of `EntityController` so they survive the map clear with stale signal connections; over long uptime with flaky network, every reconnect leaked N entity QObjects + their slot wirings. `this` as the timer's context object auto-cancels the lambda if the controller dies first (children would die via Qt parent-child destruction anyway). No other behavior changes; v1.4.10's `onEntityAdded` and `onEntityChanged` backstop preserved verbatim. |

### Custom Files (further modified)
| File | Modification |
|------|-------------|
| `src/ui/matrixrain.h` | Adds two private helper declarations (`void startTimerAtSpeed()` and `void startTimerAt(int intervalMs)`) just above `QTimer m_timer;` member. Block comment notes the AP-UC-08 (§7.5 — zero CPU/GPU when screen off) rationale and that the only direct `m_timer.start()` left is `setDisplayOff(false)`'s wake path. |
| `src/ui/matrixrain.cpp` | Two helper implementations near `resumeTicks()` (~6 lines each), both early-return on `m_displayOff` then call `m_timer.start(...)` with either the sim-speed-derived interval or a caller-provided one. Replaced 7 direct `m_timer.start(...)` callsites: `updatePaintNode` first-render guard (line 565), `resumeTicks()`, `setSpeed()`, `handleSlowInput(true)` slowdown (uses `startTimerAt(normalInterval * 3)` to preserve the 3× interval), `handleSlowInput(false)` slowdown release, `handleRestoreInput()`, `setRunning(true)`. The `setDisplayOff(false)` wake path at line ~1367 keeps its direct `m_timer.start()` (helper would behave identically since `m_displayOff` was just cleared the previous line — kept as direct call to flag it as the canonical wake path). Existing `if (m_running)` guards at the per-callsite level preserved verbatim — helper adds a second gate, doesn't replace the first. |

### Build / Tooling
| File | Modification |
|------|-------------|
| `BUILD.md` | Toolchain image pinned by digest. Was `unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest`; now `unfoldedcircle/r2-toolchain-qt-5.15.8-static@sha256:d4b1b81b4722586aa1bc9e6fc2d8ccf329872d71d6bbda40a40adb74060d31c6`. Added an inline rotation cookbook (`docker pull` → `docker inspect --format '{{index .RepoDigests 0}}'` → replace digest, commit as `[chore] toolchain:`). `CLAUDE.md:105` still references `:latest` — informational divergence flagged in the v1.4.11 CHANGELOG entry. |

**Intentionally NOT modified:** `setDisplayOff(false)`'s direct `m_timer.start()` call at the wake path — the helper would no-op because `m_displayOff` was just cleared on the previous line, so behavior is identical either way; kept direct as documentation of the canonical wake transition. `CLAUDE.md` toolchain reference — the actual build path uses `BUILD.md`; if the user wants the doc synced, that's a one-line follow-up.

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
| `src/ui/soundEffects.cpp` | **v1.4.6 (path/file guards) + v1.4.41 (diagnostic loglevel bump) + v1.4.42 (chime-variant picker).** v1.4.6: added `#include <QFileInfo>`; initialized all 5 `QSoundEffect*` members to `nullptr` in the ctor init list; added empty-path guard + file-existence guard in `createEffects()` (new `makeEffect` lambda skips `setSource` when the path is empty or the file doesn't exist) and null-guards around every `m_effect*->setVolume()` / `->play()` call in `play()`'s switch. **v1.4.41**: rerouted the 5 diagnostic `qCDebug(lcUi())` calls to `qCWarning(lcCore())` — bare `uc.ui` is silenced before journald in firmware 2.9.x, and `qCDebug` is dropped entirely; only WARN+ uc.core surfaces. **v1.4.42**: added 12-variant dock-chime picker — new `chimeFileName(int variant)` static (cases 1-12 mapping to `zap_*.wav` / `power_*.wav` / `tos_*.wav` filenames), `setDockChimeVariant(int)` runtime swap method (deletes old `m_effectBatteryCharge` via `deleteLater()`, creates fresh QSoundEffect from chosen wav, calls `play(BatteryCharge)` for immediate audio preview), `m_dockChimeVariant` member tracking current variant. Constructor extended with `int initialChimeVariant = 1` parameter for boot-time variant selection. |
| `src/ui/uiController.cpp` | **v1.4.41 (audio path fallback) + v1.4.42 (chime variant wiring).** v1.4.41: added `UC_SOUND_EFFECTS_PATH` empty-fallback at the `SoundEffects` constructor call site. When the env var is empty (firmware 2.9.2 regression), reads `qgetenv("UC_CONFIG_HOME")` instead — same env our font (`glyphatlas.cpp:51`) and screensaver json (`screensaverconfig.cpp:21`) already use. Env still wins if non-empty, so future firmware that restores the env is forward-compatible. **v1.4.42**: passes `m_config->getDockChimeVariant()` to SoundEffects ctor as initial variant; connects `Config::dockChimeVariantChanged → SoundEffects::setDockChimeVariant` for runtime variant swap on user picker change. |
| `src/qml/components/ChargingScreen.qml` | **v1.3.0 baseline (theme Loader + ButtonNavigation + displayOff propagation) + v1.4.43 (pattern B chime + single-tap dismiss).** v1.4.43 additions: new `_openedViaDock` property (set by main.qml's `Loader.onStatusChanged` from `_nextOpenViaDock` flag, gated on `Battery.powerSupply` to ignore stale flags from quick dock-then-undock during async load); in `onOpened`, if `_openedViaDock` is true, fires `SoundEffects.play(BatteryCharge)` and resets the flag — by this point all heavy QML/GPU init is done so the audio thread has clean CPU and the chime plays without ALSA buffer underruns. Also `onReleased` else-branch now branches on `tapToClose`: ON keeps double-tap-required (anti-accidental) behavior, OFF closes immediately on single tap (responsive dismissal — also bug-fixes the line-602 doubleTapTimer Matrix-glitch side-effect since the timer never starts). New import: `SoundEffects 1.0`. |
| `src/qml/main.qml` | **v1.4.41 (boot-while-docked grace + chime grace timer) + v1.4.42 (variant-aware grace) + v1.4.43 (pattern B toggle).** Latest additions: `_nextOpenViaDock` one-shot flag (set in normal-dock branch when `Config.dockChimeAfterScreensaver` is true, propagated to `ChargingScreen.qml::_openedViaDock` via `Loader.onStatusChanged`); `_chimeDurationsMs[]` table (12 entries) + `_chimeGraceForVariant(v)` helper for the legacy chime-first path; three-way branch in `onPowerSupplyChanged(true)` (boot-while-docked → 2 s grace, no chime; pattern B → immediate Loader activation, chime fires in popup.onOpened; legacy → chime + variant-aware grace). |
| `src/config/config.{h,cpp}` | **v1.4.42 (`dockChimeVariant`) + v1.4.43 (`dockChimeAfterScreensaver`).** Two QSettings-backed Q_PROPERTYs for the dock-chime feature. `dockChimeVariant` (int, 1-12, default 1, key `sound/dockChimeVariant`) selects which wav file `m_effectBatteryCharge` loads. `dockChimeAfterScreensaver` (bool, default true, key `sound/dockChimeAfterScreensaver`) selects pattern B vs legacy timing. Bounds-clamped on read and write. |
| `src/qml/settings/settings/Sound.qml` | **v1.4.42 (12-variant picker grid) + v1.4.43 (description text refinements + chime-timing toggle).** Added "Dock chime" section (4×3 grid of variant tiles, tap-to-preview-and-select pattern matching `ThemeSelector.qml`). Added "Chime after screensaver" toggle below the picker (v1.4.43) with description text. `contentY` cap raised 1100 → 1450 → 1600 → 1750 across releases to accommodate the growing settings page. |
| `src/qml/settings/settings/chargingscreen/GeneralBehavior.qml` | **v1.4.43 (description text under "Double-tap to close" toggle).** Single-line description clarifying both states: "On: tap twice to dismiss the screensaver (anti-accidental). Off: a single tap dismisses." Pairs with the v1.4.43 `ChargingScreen.qml` semantics fix (single-tap dismissal when toggle is OFF). |

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

---

## v1.4.12+ deltas (summary table)

Full prose lives in `CHANGELOG.md` and the per-Mod sections in `CLAUDE.md`. Use this table as a "where did this file change" lookup.

| Release | Custom files added | Upstream files modified |
|---------|--------------------|-------------------------|
| **v1.4.12** Mod 4 WiFi UX bundle (W1–W6, W10) | — | `src/hardware/wifi.{h,cpp}`, `src/qml/components/StatusBar.qml`, `src/qml/settings/settings/WifiInfo.qml`, `src/qml/settings/settings/Wifi.qml`, `src/qml/onboarding/Wifi.qml` |
| **v1.4.13** W9 onboarding nuclear-cleanup fix | — | `src/hardware/wifi.{h,cpp}`, `src/qml/onboarding/Wifi.qml` |
| **v1.4.14** Mod 5 Active Session Keeper | `src/hardware/activitySessionKeeper.{h,cpp}` (new custom files) | `src/core/core.{h,cpp}` (added `setPowerMode`), `src/hardware/hardwareController.{h,cpp}` (singleton wiring), `src/main.cpp` (signal hookup), `src/ui/entity/entityController.{h,cpp}` (2 new signals + chokepoint emit + media-player state forward), `src/ui/uiController.h` (`getEntityController()` accessor), `src/qml/settings/settings/Power.qml` (3 new rows), `src/config/config.{h,cpp}` (3 new Q_PROPERTYs), `remote-ui.pro` (register new custom files) |
| **v1.4.15** UI polish (Power slider overflow, WifiInfo back-arrow, screensaver docked-rearm) | — | `src/qml/settings/settings/Power.qml` (preferredHeight fix), `src/qml/settings/settings/WifiInfo.qml` (Flickable + back arrow + drop bottom Close), `src/ui/screensaverconfig.h` (new `reopenWhileDockedSec` SCRN_INT macro), `src/qml/main.qml` (new `dockedRearmTimer` + 3 handler edits), `src/qml/settings/settings/chargingscreen/GeneralBehavior.qml` (new slider section). Also: backfilled `src/config/config.h` Q_PROPERTYs for v1.4.14's `sessionKeeper*` (had landed in source after the v1.4.14 commit was tagged). |
| **v1.4.16** Post-v1.4.15 polish round (slider thinning, docked-rearm gate fix, WifiInfo button placement, label clipping fix) | — | `src/qml/settings/settings/Power.qml` (revert preferredHeight 140 → height 60, drop low/high labels), `src/qml/settings/settings/WifiInfo.qml` (fold buttons back into Flickable), `src/qml/main.qml` (drop `_shouldOpenOnIdle()` gate from docked rearm path), `src/qml/settings/settings/chargingscreen/GeneralBehavior.qml` (label clip fix, min 30→5s, slider thinning) |
| **v1.4.17** WiFi Diagnostics popup (W13) | `src/qml/settings/settings/WifiDiagnostics.qml` (~280 lines) | `src/hardware/wifi.{h,cpp}` (4 new Q_PROPERTYs: `rssiHistory` QVariantList, `disconnectCount`, `currentSessionDurationSec`, `secondsSinceLastDisconnect` + ring buffer `m_rssiHistory` cap 60 + `Q_INVOKABLE resetDiagnosticCounters()` + 1 Hz `m_statsTickTimer`), `src/qml/settings/settings/WifiInfo.qml` (Diagnostics button + popup instance), `resources/qrc/main.qrc` (register new file) |
| **v1.4.18** CI sync fix (chore) | — | `remote-ui.pro:75` (`VERSION = 1.4.11` → `1.4.18`). No runtime change — CI artifact-build version-consistency check between `.pro` and `release.json` was failing for six releases v1.4.12-v1.4.17 silently. App-version display continues to come from `GIT_VERSION` (`git describe --tags`). |
| **v1.4.19** Wake-replay HUD + LOW_POWER wake-trigger fix | `src/qml/components/overlays/ReconnectingHUD.qml` (~75 lines) | `src/ui/entity/entityController.cpp` (3-line wake-trigger expansion at line 757: `wasAsleep = SUSPEND \|\| LOW_POWER` — fixes a latent upstream bug where the existing `m_pendingCommands` retry loop never engaged on UCR3 because daily standby uses LOW_POWER, not SUSPEND), `src/qml/main.qml` (HUD instantiation + `import "qrc:/components/overlays" as Overlays` alias), `resources/qrc/main.qrc` (register new file). Closes the `EntityController.resumeWindow` Q_PROPERTY visibility loop — the property has existed since upstream but was never surfaced to the user. |
| **v1.4.20** Mod 6 Phantom-Wake Suppressor | `src/hardware/phantomWakeSuppressor.{h,cpp}` (~75 / ~110 lines) | `src/hardware/hardwareController.{h,cpp}` (singleton construction + Battery/Power/TouchSlider bridges + qmlRegisterSingletonType), `src/main.cpp` (Config + InputController wiring + initial-state push), `src/config/config.{h,cpp}` (2 new Q_PROPERTYs `phantomWakeSuppressEnabled`/`phantomWakeSuppressGraceMs` with QSettings keys `power/phantomWakeSuppress*`), `src/qml/settings/settings/Power.qml` (new top section above Mod 5 with Switch + helper Text + conditional Slider, KeyNav chain integrated), `remote-ui.pro` (HEADERS/SOURCES + VERSION 1.4.19 → 1.4.20). Inverse-symmetric to Mod 5 (`activitySessionKeeper.cpp`'s NORMAL-pinging) — same C++ singleton + QTimer + setPowerMode pattern, but forces LOW_POWER instead of NORMAL when grace expires without user input. |
| **v1.4.21** Reconnect HUD overhaul + WiFi-everywhere toggle | `src/qml/components/overlays/WifiStatusChip.qml` (~50 lines, sibling to Mod 3's `BatteryStatusChip.qml`) | `src/qml/components/overlays/ReconnectingHUD.qml` (height 60 → 120, font 24 → 32, spinner 36 → 56, `colors.dark` → `colors.medium`, added 3-second opacity pulse), `src/qml/components/entities/BaseDetail.qml` (new WiFi chip Loader at position 6 just-left-of-battery; warning icon predicate now also gated on `!Config.showWifiEveryWhere`), `src/qml/settings/settings/Ui.qml` (new "Show WiFi indicator everywhere" Switch row + KeyNav chain extension + Flickable contentY clamp 1900 → 2080), `src/config/config.{h,cpp}` (1 new Q_PROPERTY `showWifiEveryWhere`, QSettings key `ui/showWifiEveryWhere`, default ON), `resources/qrc/main.qrc` (register WifiStatusChip), `remote-ui.pro` (VERSION 1.4.20 → 1.4.21). Mirrors Mod 3 Battery Chip pattern; chip carries full WiFi info (signal strength + disconnected red-X) so the existing warning-only icon hides when the everywhere toggle is on. |
| **v1.4.22** Mod 5 + Mod 6 force-back API fix + wake-press detection | `test/probe_logdy_persist.py` (test infra — persistent reconnecting Logdy WS capture for soak observation, gitignored output dir) | `src/core/core.cpp:815` (one-line fix: `Api::setPowerMode` body field name `power_mode` → `mode`; firmware's serde rejected every prior call with HTTP 400 since v1.4.14 for Mod 5 and v1.4.20 for Mod 6, both Mods were silent no-ops), `src/hardware/phantomWakeSuppressor.{h,cpp}` (added `onEntityCommandIssued(QString,QString)` slot with curated allowlist mirroring Mod 5's; promoted log levels qCDebug/qCInfo → qCWarning at the three state-machine transition sites for Logdy visibility), `src/main.cpp` (wire `EntityController::entityCommandIssued → suppressor::onEntityCommandIssued`), `CHANGELOG.md`, `deploy/release.json`, `remote-ui.pro` (1.4.21 → 1.4.22). Empirical confirmation captured 2026-05-01 via Logdy WS — 3 Mod 6 force-backs + 2 Mod 5 pings all logged the same `missing field 'mode'` error. |
| **v1.4.23** Mod 6 wake-press timing fix + grace range expansion | — | `src/config/config.{h,cpp}` (new Q_PROPERTY `phantomWakeSuppressInputLookbackMs`, QSettings key `power/phantomWakeSuppressInputLookbackMs`, default 500 ms, range 0–2000 ms), `src/hardware/phantomWakeSuppressor.{h,cpp}` (new `m_lastInputTimer` QElapsedTimer + `m_inputLookbackMs` member; updated in `onUserInput`/`onEntityCommandIssued` regardless of timer state; consulted in `onPowerModeChanged` to skip arming when input arrived within lookback window — handles firmware delivery ordering where wake-press signals fire BEFORE `Power::powerModeChanged` on wake-from-LOW_POWER; setGraceMs clamp 2000 → 5000 ms), `src/main.cpp` (config wiring + initial state push), `src/qml/settings/settings/Power.qml` (grace slider `to: 2000` → `to: 5000`, new "Recent-input lookback" slider 0–2000 ms with KeyNav chain extension), `resources/translations/*.ts` (lupdate regen, 3 new qsTr strings), `CHANGELOG.md`, `deploy/release.json`, `remote-ui.pro` (1.4.22 → 1.4.23). |
| **v1.4.24** Power.qml settings copy fixes | — | `src/qml/settings/settings/Power.qml` (lookback help text font 20 → 24, `colors.medium` → `colors.light`, shortened to "Skip the grace timer when a button-press arrived this recently before a wake."; Mod 5 subtitle rewritten from "Prevents the 5-minute sleep timer..." to "Resets the device's sleep countdown every 4.5 min while media is playing or you've recently pressed a button." — accurate to `activitySessionKeeper.cpp::ping()` mechanism), `resources/translations/*.ts` (lupdate regen, 2 changed qsTr strings), `CHANGELOG.md`, `deploy/release.json`, `remote-ui.pro` (1.4.23 → 1.4.24). Copy-only; no logic, no new files. |
| **v1.4.25** release.json description fix + Mod 5/6 history note | — | `deploy/release.json` (Mod 5 description carried the same misleading "5-minute sleep timer" wording v1.4.24 fixed in Power.qml — publicly visible on GitHub release page + install-API responses, so updated to match the corrected QML wording. Mod 6 description gained a brief one-line history note: "(v1.4.22 fixed the API call format that had silently broken Mod 5/6 since their respective releases; v1.4.23 added recent-input lookback to handle wake-press timing)"), `CHANGELOG.md`, `remote-ui.pro` (1.4.24 → 1.4.25). Copy-only; no QML, no code, no logic. |
| **v1.4.26** Power.qml screen-off-style grid render fix | — | `src/qml/settings/settings/Power.qml` (replaced the screen-off-style picker's `GridLayout` with `Item` + computed-position `Repeater` children; explicit `x/y` math skips Qt's constraint solver during instantiation — eliminates the ~270 ms cascade when entering Settings → Power → Screen off animations. Bundled polish: per-delegate `readonly property bool selected` consolidates the previous two parallel `ScreensaverConfig.screenOffEffectStyle === modelData.name` bindings into one — halves the binding cost on selection change), `resources/translations/*.ts` (lupdate regen, line-number-only shifts), `CHANGELOG.md`, `deploy/release.json`, `remote-ui.pro` (1.4.25 → 1.4.26). No new code, no new qsTr strings, single-file logic change. |
| **v1.4.27 → v1.4.35** Test infrastructure + minor fixes | `test/hardware/keeper_test/`, `test/hardware/suppressor_test/`, `test/hardware/mock_core_api.{h,cpp}` (v1.4.27 unit tests for Mod 5 + Mod 6); `test/qml/MockBattery.h`, `MockConfig.h`, `MockEntityController.h`, `MockSignalStrength.h`, `MockWifi.h` + `tst_battery_status_chip.qml`, `tst_reconnecting_hud.qml`, `tst_wifi_status_chip.qml` (v1.4.34 QML tests for chips + HUD); `tools/check_setPowerMode_drift.py` (v1.4.29 audit-path drift check) | `src/ui/screensaverconfig.{h,cpp}` (v1.4.28 audit quick wins), `src/qml/components/Slider.qml` (v1.4.33 Repeater `model:` gate fix — Settings → Power 70 s open delay), `src/qml/components/overlays/{ReconnectingHUD,BatteryStatusChip,WifiStatusChip}.qml` (v1.4.34 testability + v1.4.35 HUD value-source-aware test fix), `.github/workflows/{test,tidy,build}.yml` (v1.4.28 / v1.4.29 / v1.4.30 CI green), `remote-ui.pro` (per-release VERSION sync). No new compiled binary files in this nine-release window. |
| **v1.4.36** B1 matrixrain decomposition + C1 ctor reorder + AP-UC-13 ext + settings-text consistency | `src/ui/matrixrain/singlelayerrenderer.{h,cpp}` (Phase A — 87 + 379 LOC, stateless render path), `src/ui/matrixrain/inputhandler.{h,cpp}` (Phase B — 74 + 254 LOC, QObject-with-timers; enter-button state machine + dispatch), `src/ui/matrixrain/bindinghelper.{h,cpp}` (Phase C — 38 + 199 LOC, all-static helpers; 8 ScreensaverConfig binding helpers; .cpp wrapped in `#ifndef MATRIX_RAIN_TESTING`) | `src/ui/matrixrain.{cpp,h}` (1445 → 746 LOC, -48%; orchestrator + QSGNode + multi-layer dispatch retained; QML contract preserved verbatim), `src/main.cpp` (C1 — ScreensaverConfig construction reordered to AFTER hwController so Battery is live when its ctor wires Battery signals), `src/ui/screensaverconfig.cpp` (C1 — drops `QTimer::singleShot(500, ...)` Battery deferred-connect retry hack, `Q_ASSERT(batt && ...)` regression net), `src/qml/settings/settings/chargingscreen/{AnalogSettings,GeneralBehavior}.qml` (3× `colors.medium` → `colors.light` for description-text consistency), `STYLE_GUIDE.md` (AP-UC-13 extension — Repeater `model:` gating cheap-hygiene combo), `.gitignore` (untrack `.claude/settings.local.json`), `remote-ui.pro` + 5 test/CMake build files (register 3 new helper modules; `MATRIX_RAIN_TESTING` filter for tidy + tests), `CHANGELOG.md`, `ENGINEERING_LOG.md`, `docs/CUSTOM_FILES.md`, `SCREENSAVER-IMPLEMENTATION.md`, `README.md`, `CLAUDE.md`, `deploy/release.json`, `remote-ui.pro` (1.4.35 → 1.4.36). |
| **v1.4.37** Audit-driven hygiene cleanup (path to A−) — N4 fake tests + N6 BUILD.md + N9 auto-revert doc + N7 a11y static pre-pass | — (no new compiled files; doc/test only) | `test/integration/tst_matrixrain_lifecycle.qml` (3 `verify(true)` → 6 real assertions: `compare(rain.direction/charset/colorMode, last)` + `verify(rain.running)`), `test/integration/tst_config_propagation.qml` (3 fakes → 8 real: 4× verify(prop>0) post-extreme + 4× round-trip compare; SignalSpy on `enterAction`), `test/integration/tst_settings_navigation.qml` (3 fakes → 5 real: SignalSpy `compare(spy.count, 1)` for autoRepeatIgnored — converts a comment claim into a checked invariant; `compare(spy.count, 0)` for releaseWithoutPress; `verify(spy.count <= 20)` for rapid cycles), `BUILD.md` (drop hardcoded macOS path; add Linux/macOS + Windows+Git-Bash Docker invocations matching CLAUDE.md), `STYLE_GUIDE.md` (new §1.11 "Auto-revert is a crash safety net, not a validation review" — documents the v1.4.22 setPowerMode silent failure as case study), `docs/A11Y_AUDIT.md` (§4 first audit pass — static pre-pass: page-existence ✓, KeyNavigation chain count per file, font.pixelSize sweep, touch-target dimension check; manual on-device items explicitly deferred to a future release with UCR3 device time, with a checklist), `CHANGELOG.md`, `ENGINEERING_LOG.md`, `docs/CUSTOM_FILES.md`, `deploy/release.json`, `remote-ui.pro` (1.4.36 → 1.4.37). **Net: -9 fake assertions, +19 real assertions, +10 net real. Zero binary changes — install bundle ships byte-identical binary to v1.4.36 with updated release.json.** |
| **v1.4.38** Test-only CI fix for v1.4.37 SignalSpy regression (binary identical to v1.4.37) | — | `test/integration/tst_config_propagation.qml` (drop wrong SignalSpy assumption from `test_interactiveInputEnterAndSlow` — `interactiveInput()` is imperative-dispatch path, doesn't go through `enterAction` signal; replace `verify(spy.count >= 1)` with `verify(rain.running)` + inline comment flagging dispatch-vs-state-machine distinction), `CHANGELOG.md`, `ENGINEERING_LOG.md`, `docs/CUSTOM_FILES.md`, `deploy/release.json`, `remote-ui.pro` (1.4.37 → 1.4.38). **Zero binary changes — exists solely to attach a green CI run to the latest tag.** |

### Cumulative drift since v1.4.11 baseline

**Custom files added (compiled):** 12 (6 → 12 in v1.4.36; v1.4.22 → v1.4.35 modified existing files, did not add new compiled units).
- `src/hardware/activitySessionKeeper.{h,cpp}` — Mod 5 (v1.4.14)
- `src/qml/settings/settings/WifiDiagnostics.qml` — Mod 4 W13 (v1.4.17)
- `src/qml/components/overlays/ReconnectingHUD.qml` — Wake-replay HUD (v1.4.19)
- `src/hardware/phantomWakeSuppressor.{h,cpp}` — Mod 6 (v1.4.20)
- `src/qml/components/overlays/WifiStatusChip.qml` — WiFi-everywhere chip (v1.4.21)
- `src/ui/matrixrain/singlelayerrenderer.{h,cpp}` — Mod 1 single-layer render path (v1.4.36 Phase A)
- `src/ui/matrixrain/inputhandler.{h,cpp}` — Mod 1 input dispatch + enter-button state machine (v1.4.36 Phase B)
- `src/ui/matrixrain/bindinghelper.{h,cpp}` — Mod 1 ScreensaverConfig binding helpers (v1.4.36 Phase C)

**Test infrastructure added (not compiled into binary):**
- `test/probe_logdy_persist.py` — persistent reconnecting Logdy WS capture for multi-hour soak observation (v1.4.22)
- `test/hardware/keeper_test/` + `test/hardware/suppressor_test/` + `test/hardware/mock_core_api.{h,cpp}` — Mod 5 / Mod 6 unit tests (v1.4.27)
- `tools/check_setPowerMode_drift.py` — audit-path drift check (v1.4.29)
- `test/qml/Mock{Battery,Config,EntityController,SignalStrength,Wifi}.h` + `tst_{battery_status_chip,reconnecting_hud,wifi_status_chip}.qml` — chip + HUD QML tests (v1.4.34 / v1.4.35)

**Upstream files now modified (cumulative):**
- `src/hardware/wifi.{h,cpp}` — Mod 4 (W1-W6, W9, W10) + Mod 4 W13 v1.4.17 (ring buffer + counters)
- `src/core/core.{h,cpp}` — Mod 5 (`setPowerMode`)
- `src/hardware/hardwareController.{h,cpp}` — Mod 5 keeper singleton wiring
- `src/main.cpp` — Mod 5 keeper hookup + previous mods
- `src/ui/entity/entityController.{h,cpp}` — Mod 5 keeper hookup signals; v1.4.19 LOW_POWER wake-trigger expansion
- `src/ui/uiController.h` — Mod 5 `getEntityController()` accessor
- `src/qml/main.qml` — Mod 1 docked-rearm timer; v1.4.19 ReconnectingHUD instantiation + `import "qrc:/components/overlays" as Overlays`
- `src/qml/components/StatusBar.qml` — Mod 4 always-visible bar
- `src/qml/settings/settings/Power.qml` — Mod 5 keeper UI
- `src/qml/settings/settings/WifiInfo.qml` — Mod 4 diagnostics + back arrow + v1.4.17 Diagnostics button
- `src/qml/settings/settings/Wifi.qml` — Mod 4 displayOff gate
- `src/qml/onboarding/Wifi.qml` — Mod 4 displayOff gate + W9 onboarding fix
- `src/qml/settings/settings/chargingscreen/GeneralBehavior.qml` — Mod 1 docked-rearm slider
- `src/ui/screensaverconfig.h` — Mod 1 `reopenWhileDockedSec` SCRN_INT
- `src/config/config.{h,cpp}` — Mod 5 keeper QSettings preferences + Mod 6 suppressor QSettings preferences (v1.4.20) + showWifiEveryWhere (v1.4.21)
- `remote-ui.pro` — Mod 5 custom-file registration; v1.4.18 VERSION sync; bumped per-release v1.4.19+; Mod 6 custom-file registration (v1.4.20)
- `resources/qrc/main.qrc` — registers v1.4.17 WifiDiagnostics.qml + v1.4.19 ReconnectingHUD.qml + v1.4.21 WifiStatusChip.qml
- `src/qml/components/entities/BaseDetail.qml` — Mod 3 chip Loader + v1.4.21 WiFi chip Loader + warning predicate tweak
- `src/qml/components/overlays/ReconnectingHUD.qml` — v1.4.19 initial impl + v1.4.21 prominence overhaul
- `src/qml/settings/settings/Ui.qml` — v1.4.2 showVolumeOverlay toggle + v1.4.8 media button toggles + v1.4.21 showWifiEveryWhere toggle
