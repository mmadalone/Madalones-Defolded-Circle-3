# Custom Files Manifest — UC Remote 3 Mods

Tracks every file that is custom (added by madalone) or modified from the upstream `unfoldedcircle/remote-ui` codebase. If a file is not listed here, it is upstream and should not be modified without explicit justification.

**Upstream base:** `v0.71.1`  
**Last updated:** 2026-04-22 (Mod 3 detail-page battery chip)

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
| `src/main.cpp` | Added `#include` for MatrixRain, ScreensaverConfig. `qmlRegisterType<MatrixRainItem>`. Instantiated `ScreensaverConfig` singleton. |
| `src/config/config.h` | Screensaver properties removed (moved to ScreensaverConfig). Only a redirect comment remains. Mod 3: added `Q_PROPERTY(bool showBatteryOnDetailPages …)` + getter/setter decl + signal at END of custom blocks. |
| `src/config/config.cpp` | Mirror of config.h: screensaver impls removed, redirect comment for moved ScreensaverConfig. Mod 3: added `getShowBatteryOnDetailPages()` / `setShowBatteryOnDetailPages()` reading QSettings key `ui/batteryOnDetailPages` (default `true`). |
| `src/logging.h` | Added `lcScreensaver` logging category declaration. |
| `src/logging.cpp` | Added `lcScreensaver` logging category definition (uc.ui.screensaver). |
| `src/hardware/battery.h` | Added `Q_INVOKABLE setPowerSupply()` + `instance()` getter for DEV-mode F12 dock-toggle. |
| `src/ui/inputController.h` | Added global event filter declaration + `touchDetected` signal (DEV F12 + idle-timer reset). |
| `src/ui/inputController.cpp` | DEV F12 dock-toggle event filter, `touchDetected` signal emit, idle-timer reset wiring. ~40 lines. |
| `src/ui/entity/mediaPlayer.cpp` | Bugfix: re-download image when URL is present but image data is empty (2-line change). |
| `src/qml/components/entities/media_player/ImageLoader.qml` | Same image-re-download bugfix on the QML side. |
| `remote-ui.pro` | Added custom HEADERS/SOURCES at end of lists (matrixrain, screensaverconfig, rainsimulation, gravitydirection, glitchengine, messageengine, glyphatlas). |
| `resources/qrc/main.qrc` | Registered all custom QML files and settings sub-pages. |

---

## Mod 1: Screensaver System

### Custom C++ Files
| File | Purpose | ~Lines |
|------|---------|--------|
| `src/ui/matrixrain.h` | `MatrixRainItem` QQuickItem — GPU-accelerated matrix rain renderer | ~200 |
| `src/ui/matrixrain.cpp` | QSG rendering, vertex buffer, atlas upload, stream iteration | ~500 |
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

## Mod 3: Detail Page Battery Chip

### Custom QML Files
| File | Purpose |
|------|---------|
| `src/qml/components/overlays/BatteryStatusChip.qml` | Compact battery chip mirroring StatusBar visual (bolt + percentage when charging, 16×30 bar + optional percentage otherwise). Touch-transparent (no MouseArea). Bound directly to `Battery` and `Config` singletons. |

### Modified Upstream Files
| File | Modification |
|------|-------------|
| `src/qml/components/entities/BaseDetail.qml` | Added `import Config 1.0`. Added `id: iconIntegrationDisconnected` to the existing disconnected-integration icon (no behavior change). Added `Loader` anchored `right: iconIntegrationDisconnected.visible ? iconIntegrationDisconnected.left : iconClose.left`, `z: 1001`, gated on `Config.showBatteryOnDetailPages`. **2026-04-22 Option B hotfix:** added `import Wifi 1.0` + `import Wifi.SignalStrength 1.0`; added `readonly property bool _wifiWarningActive` mirroring the StatusBar / BaseTitle / Activity WiFi-warning predicate; chip Loader `rightMargin` now conditional `(!iconIntegrationDisconnected.visible && _wifiWarningActive) ? 70 : 10` to avoid overlap with the WiFi warning icon in subclass title bars. |
| `src/qml/settings/settings/Ui.qml` | Added "Battery on detail pages" toggle row between `batteryPercentSwitch` and `activityBarSwitch`. Re-wired `KeyNavigation` chain through the new switch. Bumped `Flickable.contentY` clamp 1100 → 1260 to accommodate the added content. |
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
