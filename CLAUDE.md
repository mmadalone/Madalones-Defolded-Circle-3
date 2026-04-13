# CLAUDE.md — UC Remote 3 Custom Firmware

## Project Identity

Custom firmware mod project for the **Unfolded Circle Remote 3**. Fork of [`unfoldedcircle/remote-ui`](https://github.com/unfoldedcircle/remote-ui) (Qt 5.15 / QML / C++17 / GPL-3.0).

Multiple independent mods are built on top of the upstream UC codebase — some bridge to Home Assistant and [Project Fronkensteen](https://github.com/mmadalone/Project_Fronkensteen) (the HA voice AI ecosystem), some are purely UC3-native. Each mod is self-contained but may share infrastructure (GlyphAtlas, config macros, etc.).

**Owner:** madalone  
**Device:** UC Remote 3 at `${UC3_HOST}`, PIN `${UC3_PIN}`  
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

**Recent changes (2026-04-07):**
- **Per-cell residual glow** — Rezmason-inspired (3.7k stars) per-cell age tracking. Cells retain brightness independently after stream head passes, decaying via the same brightness map. Eliminates dark gaps between active trails, especially visible in horizontal directions.
- **Color layers** — custom `MatrixRainShader` (texture × per-vertex RGBA) for atmospheric color variation. Each stream's `depthFactor` (0.6–1.4) produces a unique tint via continuous lerp: dim teal (slow) → base green → bright chartreuse (fast). White atlas when enabled. Also: trail length scaling, async speed, spatial offset, depth-sorted occlusion. Settings: "Color layers" toggle, intensity slider, overlay mode.
- **Coprime gravity spawn** — `coprimeGoldenStep()` enforces `gcd(step, n) == 1`, guaranteeing all rows/columns visited during gravity mode. Previously `gcd(40,65)=5` left 80% of rows empty.
- **Full-screen grid** — grid covers entire screen at native glyph spacing. Density slider controls stream count (multiplier), not grid spacing.
- **Docker visual preview** — `test/matrixrain_preview/` with Dockerfile, TigerVNC on port 5909. Builds x86_64 native with Mesa software OpenGL.
- **Keyboard:** Arrow keys = direction, Enter = chaos, R = restore, G = toggle gravity, D = toggle color layers.

**Design doc:** `SCREENSAVER-IMPLEMENTATION.md`

### Mod 2: Avatar System 📦 Archived on `feature/avatar` branch
Braille character grid renderer with mood-reactive animation, per-cell sprite animation, voice overlay integration, HA entity bridge for push events. **Removed from `main` on 2026-04-08** (commit `c7e7a3a`) to keep release scope clean. Phase A was code-complete on the feature branch; no current merge plan. Both `AVATAR_PLAN.md` and avatar source (`src/ui/avatargrid.*`, `test/avatar_preview/`) are gitignored on `main`.

**Design doc:** `AVATAR_PLAN.md` — local reference only, gitignored on `main`. **To work on avatar code: `git checkout feature/avatar`.**

### Mod 3+: Future
TBD. New mods follow the established mod anatomy pattern (see `STYLE_GUIDE.md`).

---

## Build, Preview & Deploy

### Dev Workflow (MANDATORY)

1. **Edit** C++ / QML source
2. **Preview in Docker** — visually verify via VNC before deploying
3. **Cross-compile and deploy** to UC3

### Screensaver Preview (Docker — visual)
Linux x86_64 container with Mesa software OpenGL + TigerVNC. Renders the actual `QSGGeometryNode` scene graph.
```bash
cd "/Users/madalone/_Claude Projects/UC-Remote-UI/test/matrixrain_preview"
docker-compose up --build -d    # First time or after source edits
docker-compose down && docker-compose up --build -d  # Rebuild
docker-compose logs --tail 10   # Check for errors
```
**VNC:** Connect to `localhost:5909` (no password). Press any key to start rendering.
**Keyboard:** Arrow keys = direction, Enter = chaos, R = restore, G = toggle gravity.
**macOS native build:** Compiles but renders black (Qt 5.15 OpenGL 2.1 macOS limitation). Use Docker for visual testing.

### Full App (macOS — requires Core Simulator)
```bash
cd "/Users/madalone/_Claude Projects/UC-Remote-UI"
qmake && make
UC_MODEL=DEV ./binaries/osx-*/release/Remote\ UI
```
Needs UC's [core-simulator](https://github.com/unfoldedcircle/core-simulator) Docker container running to get past the loading screen. Only needed for testing non-screensaver mods (settings pages, entity bridge, etc.).

### Cross-compile for UC3 (ARM64)
```bash
cd "/Users/madalone/_Claude Projects/UC-Remote-UI"
docker run --rm --user=$(id -u):$(id -g) -v "$(pwd)":/sources \
    unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest
```
Output: `binaries/linux-arm64/release/remote-ui`

### Deploy to UC3
```bash
cp binaries/linux-arm64/release/remote-ui deploy/bin/remote-ui
cd deploy && tar czf /tmp/remote-ui-deploy.tar.gz release.json bin/ config/
curl --location "http://${UC3_HOST}/api/system/install/ui?void_warranty=yes" \
    --form "file=@/tmp/remote-ui-deploy.tar.gz" -u 'web-configurator:${UC3_PIN}'
```
Restarts the UI on the device (~10s). Revert to stock: `curl -X PUT "http://${UC3_HOST}/api/system/install/ui?enable=false" -u 'web-configurator:${UC3_PIN}'`

### QML Prototyping
```bash
qmlscene test_braille.qml          # Avatar ambient life prototype
qmlscene test_braille_mapper.qml   # Face region mapper
qmlscene test_themes.qml           # Theme testing
```

### Font Subsetting
```bash
pyftsubset DejaVuSansMono.ttf --unicodes="U+2800-28FF" --output-file=BrailleFont.ttf
```
TTF format for FreeType hinting quality on ARM. Subset fonts go in `deploy/config/`. Same pattern as `NotoSansMonoCJKjp.otf` (23KB katakana subset).

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
