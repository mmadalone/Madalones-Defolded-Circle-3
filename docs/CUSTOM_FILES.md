# Custom Files Manifest — UC Remote 3 Mods

Tracks every file that is custom (added by madalone) or modified from the upstream `unfoldedcircle/remote-ui` codebase. If a file is not listed here, it is upstream and should not be modified without explicit justification.

**Upstream base:** `v0.71.1`  
**Last updated:** 2026-04-04

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
| `src/config/config.h` | Screensaver properties removed (moved to ScreensaverConfig). Only a redirect comment remains. |
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
| `src/qml/components/themes/MatrixTheme.qml` | Matrix rain screensaver theme |
| `src/qml/components/themes/StarfieldTheme.qml` | Starfield screensaver theme |
| `src/qml/components/themes/MinimalTheme.qml` | Minimal digital clock theme with GradientText |
| `src/qml/components/themes/AnalogTheme.qml` | UC stock analog clock extracted as theme option |
| `src/qml/components/themes/MatrixTheme_canvas_backup.qml` | Archived Canvas-based prototype (not used) |
| `src/qml/components/overlays/GradientText.qml` | Reusable solid/rainbow gradient text component |
| `src/qml/components/overlays/ClockOverlay.qml` | Clock overlay with GradientText, date, 24h, color |
| `src/qml/components/overlays/BatteryOverlay.qml` | Battery overlay for screensaver |
| `src/qml/settings/settings/chargingscreen/ThemeSelector.qml` | Theme picker sub-page |
| `src/qml/settings/settings/chargingscreen/CommonToggles.qml` | Clock/battery/tap-to-close toggles |
| `src/qml/settings/settings/chargingscreen/MatrixAppearance.qml` | Matrix color, speed, density, charset settings |
| `src/qml/settings/settings/chargingscreen/MatrixEffects.qml` | Glitch effects, auto-rotate, trail bend settings |
| `src/qml/settings/settings/chargingscreen/GeneralBehavior.qml` | General screensaver behavior settings |

### Custom Assets
| File | Purpose |
|------|---------|
| `deploy/config/NotoSansMonoCJKjp.otf` | 23KB katakana font subset for Matrix rain |
| `deploy/config/charging_screen.json` | Default screensaver configuration |

### Modified Upstream QML
| File | Modification |
|------|-------------|
| `src/qml/components/ChargingScreen.qml` | Replaced stock analog clock with theme Loader system. Added ButtonNavigation for interactive screensaver controls. Added displayOff propagation. |
| `src/qml/settings/settings/ChargingScreen.qml` | Replaced stock charging settings with full screensaver configuration (theme selector + sub-pages). |
| `src/qml/settings/Settings.qml` | Added "Screensaver" menu entry. |
| `src/qml/components/Switch.qml` | Added `Keys.onReturnPressed`/`onEnterPressed` for DPAD center toggle. |
| `src/qml/components/TouchSliderVolume.qml` | Added `applicationWindow.screensaverActive` guard to suppress during screensaver. |
| `src/qml/components/TouchSliderSeek.qml` | Same screensaverActive guard. |
| `src/qml/components/TouchSliderBrightness.qml` | Same screensaverActive guard. |
| `src/qml/components/TouchSliderPosition.qml` | Same screensaverActive guard. |
| `src/qml/main.qml` | Added `screensaverActive` property, idle timer DEV mode bypass, ScreensaverConfig import. |

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
