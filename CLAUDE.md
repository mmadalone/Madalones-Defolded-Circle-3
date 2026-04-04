# CLAUDE.md — UC Remote 3 Custom Firmware

## Project Identity

Custom firmware mod project for the **Unfolded Circle Remote 3**. Fork of [`unfoldedcircle/remote-ui`](https://github.com/unfoldedcircle/remote-ui) (Qt 5.15 / QML / C++17 / GPL-3.0).

Multiple independent mods are built on top of the upstream UC codebase — some bridge to Home Assistant and [Project Fronkensteen](https://github.com/mmadalone/Project_Fronkensteen) (the HA voice AI ecosystem), some are purely UC3-native. Each mod is self-contained but may share infrastructure (GlyphAtlas, config macros, etc.).

**Owner:** madalone  
**Device:** UC Remote 3 at `192.168.2.204`, PIN `6984`  
**Display:** 480 × 850px IPS LCD  
**SoC:** Quad-core ARM64 1.8 GHz, 4 GB RAM, 32 GB eMMC  
**Repo:** Private — `mmadalone/Madalones-Defolded-Circle-3`  
**Upstream:** `unfoldedcircle/remote-ui` (tracked as `upstream` remote)

---

## Git & Remotes

| Remote | URL | Purpose |
|--------|-----|---------|
| `origin` | `github.com/mmadalone/Madalones-Defolded-Circle-3` | Our private repo — push here |
| `upstream` | `github.com/unfoldedcircle/remote-ui` | UC's official repo — pull updates from here |

Upstream base: `v0.71.1`. All custom work lives on `main`. To pull upstream updates: `git fetch upstream && git merge upstream/main` (resolve conflicts manually — our modified files will conflict).

---

## Current Mods

### Mod 1: Screensaver System ✅ Complete
GPU-accelerated screensaver replacing UC's stock analog clock. Matrix rain renderer, Starfield, Minimal themes, clock/battery overlays, full settings UI with 5 sub-pages. Zero HA dependency.

**Design doc:** `SCREENSAVER-IMPLEMENTATION.md`

### Mod 2: Avatar System 🔧 In Progress
Braille character grid renderer with mood-reactive animation, per-cell sprite animation (eye blinks, talking, expressions), voice overlay integration, HA entity bridge for push events. Bridges to Project Fronkensteen voice personas but works standalone via local fallback.

**Design doc:** `AVATAR_PLAN.md` — **READ THIS BEFORE TOUCHING AVATAR CODE.** All architecture decisions are resolved. Implementation order is defined (Phase A→B→C→D).

### Mod 3+: Future
TBD. New mods follow the established mod anatomy pattern (see `STYLE_GUIDE.md`).

---

## Build & Run

### Desktop (macOS)
```bash
cd "/Users/madalone/_Claude Projects/UC-Remote-UI"
qmake && make
# Run with Core Simulator:
UC_MODEL=DEV ./binaries/osx-*/release/Remote\ UI
```

### QML Prototyping
```bash
qmlscene test_braille.qml          # Avatar ambient life prototype
qmlscene test_braille_mapper.qml   # Face region mapper
qmlscene test_themes.qml           # Theme testing
```

### Cross-compile for UC3 (ARM64)
Docker-based cross-compilation targeting the UC3's Buildroot environment. Deploy via SCP to device.

### Font Subsetting
```bash
pyftsubset InputFont.ttf --unicodes="U+2800-28FF" --output-file=BrailleFont.otf
```
Subset fonts go in `deploy/config/`. Same pattern as `NotoSansMonoCJKjp.otf` (23KB katakana subset).

---

## Architecture Quick Reference

### Layer Split
- **C++** (`src/ui/`, `src/config/`): Renderers, config, singletons, GPU pipeline
- **QML** (`src/qml/`): UI components, settings pages, overlays, theme files
- **Bridge:** QML accesses C++ via `qmlRegisterType` (custom renderers) and singleton context properties (`Config`, `ScreensaverConfig`, `EntityController`, `Voice`, `Battery`)

### Key Singletons
| Singleton | Access from QML | Purpose |
|-----------|----------------|---------|
| `Config` | `import Config 1.0` | All QSettings-backed properties |
| `ScreensaverConfig` | `import ScreensaverConfig 1.0` | Screensaver domain config bridge |
| `EntityController` | `import Entity.Controller 1.0` | HA entity loading + access |
| `Voice` | `import Voice 1.0` | Voice assistant session signals |
| `Battery` | `import Battery 1.0` | Battery level, charging state |

### Config Pattern
Properties in `config.h` via macros defined in `config_macros.h`:
```cpp
// In config.h — Q_PROPERTY declaration
Q_PROPERTY(bool chargingMatrixGlow READ getChargingMatrixGlow WRITE setChargingMatrixGlow NOTIFY chargingMatrixGlowChanged)

// In config.cpp — implementation via macro
CFG_BOOL(ChargingMatrixGlow, "charging/matrixGlow", true, chargingMatrixGlowChanged)
```

### Entity Access Pattern
```qml
Component.onCompleted: EntityController.load(entityId)
Connections {
    target: EntityController
    ignoreUnknownSignals: true
    function onEntityLoaded(success, entityId) {
        if (success) myEntity = EntityController.get(entityId);
    }
}
```
Entity IDs on UC3: `{integration_prefix}.{ha_entity_id}` → e.g., `hass.main.sensor.ai_avatar_mood`

### GPU Renderer Pattern
Custom renderers are `QQuickItem` subclasses using `GlyphAtlas` texture atlas:
1. Pre-render glyphs at multiple brightness levels into a single `QImage` atlas
2. Upload once as `QSGTexture` in `updatePaintNode()`
3. Per frame: update vertex buffer UV coordinates (pure float math)
4. Single `QSGGeometryNode` draw call with `QSGTextureMaterial`
5. `displayOff` property gates the timer — zero CPU/GPU when screen is off

---

## File Placement Rules

| What | Where |
|------|-------|
| C++ renderers / logic | `src/ui/` |
| C++ config bridge singletons | `src/ui/` (alongside its renderer) |
| QML components (reusable) | `src/qml/components/{feature}/` |
| QML themes | `src/qml/components/themes/` |
| QML overlays | `src/qml/components/overlays/` |
| Settings pages | `src/qml/settings/settings/{feature}/` |
| Settings sub-pages | `src/qml/settings/settings/{feature}/` (subfolder) |
| Bundled fonts | `deploy/config/` |
| Art assets (compiled to qrc) | `src/qml/components/{feature}/art/` |
| QML prototypes / test files | Project root (`test_*.qml`) — NOT for production |
| Design docs | Project root (`*_PLAN.md`, `*-IMPLEMENTATION.md`) |

Register new files in: `remote-ui.pro` (HEADERS/SOURCES) + `resources/qrc/main.qrc` (QML/assets).

---

## Mandatory Rules for Claude Code

1. **Read the design doc first.** Before touching screensaver code → read `SCREENSAVER-IMPLEMENTATION.md`. Before touching avatar code → read `AVATAR_PLAN.md`. No exceptions.

2. **Verify from source.** Don't assume how Entity API, Core API, Config, or Voice works. Check the actual headers (`src/ui/entity/sensor.h`, `src/config/config.h`, `src/voice.h`). The AVATAR_PLAN was written after reading the actual integration source — follow that standard.

3. **Know what's custom vs upstream.** See `docs/CUSTOM_FILES.md` for the full manifest. Don't modify upstream files unless the design doc calls for it. If you're unsure whether a file is custom or upstream, check the manifest.

4. **Follow the mod pattern.** New features follow the established anatomy: C++ renderer + config bridge singleton + QML wrapper + settings page + qrc registration. See `STYLE_GUIDE.md` for the full template.

5. **HA is optional.** Any mod that bridges to HA must work without it. Local fallbacks. Null guards on entities. HA sensors populated via `state.set()` + `state_bridge.py` seeding. The UC3 is a standalone device — HA enriches it, doesn't define it.

6. **Don't break upstream compatibility.** All mods are additive. Stock behavior is preserved when features are disabled. New Config properties default to `false`/safe values. VoiceOverlay falls back to stock EQ when avatar is off. ChargingScreen falls back to stock behavior when screensaver mods are off.

7. **Copyright headers.** Custom files: `// Copyright (c) {year} madalone. {description}. SPDX-License-Identifier: GPL-3.0-or-later`. Modified upstream files: add `// Copyright (c) {year} madalone. {description}.` below the UC copyright line.

8. **Formatting.** Run `clang-format` (`.clang-format` in project root — Google base, 4-space indent, 120 col). Run `cpplint.sh` before committing C++. See `STYLE_GUIDE.md` for QML conventions.

9. **Performance awareness.** UC3 is an ARM64 embedded device with a 480×850 display. Keep GPU draw calls minimal. Gate animations with `displayOff`. Precompute where possible (brightness maps, atlas lookups). Test on device — macOS perf is not representative.

10. **No gratuitous refactoring.** Don't reorganize upstream code to be "cleaner." Don't rename upstream variables. Don't reformat upstream files with clang-format. Minimize the diff against upstream to ease future merges.
