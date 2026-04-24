# Custom Files Manifest — UC Remote 3 Mods

Tracks every file that is custom (added by madalone) or modified from the upstream `unfoldedcircle/remote-ui` codebase. If a file is not listed here, it is upstream and should not be modified without explicit justification.

**Upstream base:** `v0.71.1`  
**Last updated:** 2026-04-24 (v1.4.2 — Config.showVolumeOverlay toggle for volume OSD suppression)

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
| `src/qml/components/TouchSlider.qml` | Added `applicationWindow.screensaverActive` guard to suppress during screensaver (base shared component; the 4 specific variants below have the same guard). |
| `src/qml/components/TouchSliderVolume.qml` | Added `applicationWindow.screensaverActive` guard to suppress during screensaver. |
| `src/qml/components/TouchSliderSeek.qml` | Same screensaverActive guard. |
| `src/qml/components/TouchSliderBrightness.qml` | Same screensaverActive guard. |
| `src/qml/components/TouchSliderPosition.qml` | Same screensaverActive guard. |
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
