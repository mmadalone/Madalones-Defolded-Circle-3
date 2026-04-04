# STYLE_GUIDE.md — UC Remote 3 Coding & Architecture Standards

## C++ Conventions

### Formatting
- **Tool:** clang-format with `.clang-format` in project root
- **Base style:** Google
- **Indent:** 4 spaces
- **Column limit:** 120
- **Linting:** `cpplint.sh` before committing

### Copyright Headers
```cpp
// Custom file:
// Copyright (c) 2024 madalone. Brief description of what this file does.
// SPDX-License-Identifier: GPL-3.0-or-later

// Modified upstream file — add below existing UC header:
// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2024 madalone. Brief description of modifications.
// SPDX-License-Identifier: GPL-3.0-or-later
```

### Include Ordering
1. Own header (`#include "ui/matrixrain.h"`)
2. Qt headers (`#include <QObject>`, `#include <QQuickItem>`)
3. Project headers (`#include "../config/config.h"`)

### Namespace
All custom code lives in the `uc` namespace (matches upstream pattern).

### Q_PROPERTY Declaration Order
```cpp
Q_PROPERTY(type name READ getter WRITE setter NOTIFY signal)
// Group by domain: lifecycle → state → visual config → computed
```

### Signal Naming
`{propertyName}Changed` — matches the Q_PROPERTY NOTIFY convention. No other pattern.

---

## Config Property Pattern

### Adding a New Config Property

**Step 1:** Declare Q_PROPERTY in `config.h`:
```cpp
Q_PROPERTY(bool avatarEnabled READ getAvatarEnabled WRITE setAvatarEnabled NOTIFY avatarEnabledChanged)
```

**Step 2:** Implement in `config.cpp` via macro:
```cpp
CFG_BOOL(AvatarEnabled, "avatar/enabled", false, avatarEnabledChanged)
// Args: FuncSuffix, QSettings key, default value, signal name
```

**Step 3:** Emit signal declaration in `config.h` signals section:
```cpp
signals:
    void avatarEnabledChanged();
```

Available macros (defined in `config_macros.h`):
| Macro | Type | QSettings read | QSettings write |
|-------|------|---------------|----------------|
| `CFG_BOOL` | `bool` | `.toBool()` | direct |
| `CFG_INT` | `int` | `.toInt()` | direct |
| `CFG_STRING` | `QString` | `.toString()` | direct |

### QSettings Key Namespacing
Group keys by mod: `"charging/"` for screensaver, `"avatar/"` for avatar. Keeps QSettings organized and prevents collisions.

### When to Use a Config Bridge Singleton
If a mod's QML needs **transformed** config values (speed/50.0, density/100.0, conditional logic), create a dedicated bridge singleton (like `ScreensaverConfig`) instead of putting transforms in QML. The bridge:
1. Forwards `Config::*Changed` signals as domain-specific signals
2. Applies value transforms (math, conditionals)
3. Exposes read-only properties to QML via `SC_BOOL`/`SC_INT`/`SC_STRING` macros

If the QML just reads raw config values with no transforms, use `Config` directly.

---

## QML Conventions

### Property Declaration Order
```qml
Item {
    id: myComponent                          // 1. id (always first)

    // 2. Anchors & geometry
    anchors.fill: parent
    width: 480; height: 850

    // 3. Visual properties
    opacity: 1.0
    visible: true

    // 4. Custom properties
    property bool isActive: false
    property string currentMood: "neutral"
    property var entityRef: null

    // 5. Readonly / computed properties
    readonly property string _prefix: Config.avatarHaPrefix + ".sensor.ai_avatar_"

    // 6. Signals
    signal moodChanged(string newMood)

    // 7. Signal handlers
    onIsActiveChanged: { ... }

    // 8. Child components & Loaders

    // 9. Connections blocks

    // 10. Functions
    function resolveMood() { ... }

    // 11. Component.onCompleted (always last)
    Component.onCompleted: { ... }
}
```

### Component File Naming
PascalCase: `AvatarDisplay.qml`, `MoodEngine.qml`, `TouchHandler.qml`.

### Connections Pattern
Always use `ignoreUnknownSignals: true` when connecting to entities or dynamically loaded objects:
```qml
Connections {
    target: entityWatcher.moodEntity
    ignoreUnknownSignals: true
    function onValueChanged() { ... }
}
```

### Loader Pattern for Conditional Components
```qml
Loader {
    id: featureLoader
    anchors.fill: parent
    active: Config.featureEnabled  // only loads when enabled
    source: "qrc:/components/feature/FeatureComponent.qml"
    onLoaded: {
        if (item) {
            item.displayOff = Qt.binding(function() { return root.displayOff; });
        }
    }
}
```

### hasOwnProperty Guards
When driving properties on dynamically loaded theme items, always guard:
```qml
if (themeItem && themeItem.hasOwnProperty("color"))
    themeItem.color = newColor;
```
This prevents errors when themes don't support all mood-reactive properties.

### QML Popup Pattern
Voice/avatar overlays follow the established Popup pattern:
```qml
Popup {
    width: ui.width; height: ui.height
    opacity: 0; modal: false; closePolicy: Popup.NoAutoClose; padding: 0
    onOpened: { buttonNavigation.takeControl(); }
    onClosed: { buttonNavigation.releaseControl(); }
    Components.ButtonNavigation { id: buttonNavigation; defaultConfig: { ... } }
}
```

---

## GPU Renderer Pattern (QQuickItem)

### Anatomy of a Custom Renderer

```
src/ui/myrenderer.h      — QQuickItem subclass declaration
src/ui/myrenderer.cpp    — Grid model, simulation tick, QSG rendering
```

**Core lifecycle:**
1. `componentComplete()` — deferred init (don't do heavy work in constructor)
2. Timer fires `tick()` at target FPS (gated by `m_displayOff`)
3. `tick()` updates simulation state (cell values, animation timers)
4. `tick()` calls `update()` → Qt schedules `updatePaintNode()`
5. `updatePaintNode()` — builds/updates QSGGeometryNode with vertex buffer + atlas texture

**Copy these patterns from MatrixRainItem:**
- `MatrixRainNode` destructor (GPU texture cleanup on render thread) — ~10 lines
- Atlas texture upload in `updatePaintNode()` (`matrixrain.cpp:115-131`) — ~20 lines
- Quad vertex/index buffer construction (`matrixrain.cpp:266-273`) — ~10 lines
- Timer/tick/displayOff pattern — ~15 lines
- `componentComplete()` deferred init — ~20 lines

**GlyphAtlas integration:**
```cpp
// Register charset in glyphatlas.h charsetString()
// Add font loader: loadMyFont() alongside loadCJKFont()
// Atlas provides: UV lookup per (glyphIndex, brightnessLevel)
```

### displayOff Power Gating
Every renderer MUST stop its timer when `displayOff` is true. Zero CPU/GPU when the screen is off. This is critical for battery life on the UC3.

---

## Mod Anatomy — Template for New Features

A new UC3 mod typically consists of:

```
src/ui/{feature}.h/.cpp              C++ renderer or logic (QQuickItem subclass)
src/ui/{feature}config.h/.cpp        Config bridge singleton (if transforms needed)
src/qml/components/{feature}/        QML components
  {Feature}Display.qml                 Main visual wrapper
  {Feature}Engine.qml                  State/logic resolver
  {Feature}Overlay.qml                 Popup overlay (if applicable)
src/qml/components/overlays/         Shared overlay components
src/qml/settings/settings/
  {Feature}.qml                        Settings page entry
  {feature}/                           Settings sub-pages
deploy/config/                        Bundled assets (fonts, etc.)
src/qml/components/{feature}/art/    Art assets (compiled to qrc)
```

**Registration checklist for a new mod:**
- [ ] Add `.h` to HEADERS in `remote-ui.pro`
- [ ] Add `.cpp` to SOURCES in `remote-ui.pro`
- [ ] `qmlRegisterType<>()` in `main.cpp` (for C++ QML types)
- [ ] Instantiate config bridge singleton in `main.cpp` (if applicable)
- [ ] Register all QML files in `resources/qrc/main.qrc`
- [ ] Add Q_PROPERTYs to `config.h` / `config.cpp`
- [ ] Add settings page entry in `src/qml/settings/Settings.qml`
- [ ] Update `docs/CUSTOM_FILES.md` manifest

---

## Entity Integration Patterns (HA Bridge)

### UC3 Side (QML)
```qml
// Entity ID format: {prefix}.{ha_entity_id}
// Default prefix: "hass.main"
readonly property string entityId: Config.avatarHaPrefix + ".sensor.ai_avatar_mood"

Component.onCompleted: {
    if (Config.avatarMoodSource !== "local") {
        EntityController.load(entityId);
    }
}

Connections {
    target: EntityController
    ignoreUnknownSignals: true
    function onEntityLoaded(success, entityId) {
        if (!success) return;
        myEntity = EntityController.get(entityId);
    }
}
```

**Critical:** `sensor.getValue()` returns `QString`, not numeric types. Use `parseInt()` in QML for numeric comparisons. Always null-guard entity access.

### HA Side (pyscript)
```python
# Create sensors via state.set()
state.set("sensor.ai_avatar_mood", "neutral",
          icon="mdi:emoticon", friendly_name="AI Avatar Mood")

# Expose services
@service
def avatar_notify(mode, text="", character=None, mood=None, duration_s=0):
    ...
```

### state_bridge.py Seeding
All `state.set()` sensors are volatile — they disappear on HA restart. `state_bridge.py` seeds them on startup. Add new sensors to the seed list.

### Blueprint Integration
Optional HA bridge inputs use a boolean toggle defaulting to false:
```yaml
input:
  show_on_uc3:
    name: "Show on UC Remote"
    default: false
    selector:
      boolean:
```

---

## UC3 Hardware Constraints

| Spec | Value | Impact |
|------|-------|--------|
| CPU | ARM64 quad-core 1.8 GHz | Budget for simulation complexity |
| GPU | Embedded (in SoC) | Single draw call preferred; two max |
| RAM | 4 GB | Atlas textures live in GPU memory |
| Display | 480 × 850px IPS | No burn-in risk; 14px font → ~68×67 cell grid |
| Battery | ~8.88 Wh Li-ion | `displayOff` gating is mandatory |
| Storage | 32 GB eMMC | Binary size impact matters |

**Performance rules:**
- Precompute brightness maps, atlas lookups — no per-frame math
- Gate all rendering with `displayOff` — zero CPU/GPU when screen off
- Atlas rebuild is ~50-150ms on ARM — acceptable for transitions, not per-frame
- Two concurrent QQuickItem renderers (rain + avatar) expected fine, but verify on device
- Degradation path: reduce density/effects when dual-rendering if frame budget exceeded

---

## Upstream Management

### What's Upstream (don't touch unless necessary)
Everything not listed in `docs/CUSTOM_FILES.md`. Key upstream domains: `src/core/`, `src/dock/`, `src/hardware/`, `src/integration/`, `src/softwareupdate/`, `src/translation/`, most of `src/ui/entity/`, most QML components.

### Merging Upstream Updates
```bash
git fetch upstream
git merge upstream/main
# Conflicts will occur in: config.h, config.cpp, main.cpp, remote-ui.pro,
# ChargingScreen.qml, VoiceOverlay.qml (if avatar is implemented),
# main.qrc, Settings.qml
# Resolve manually — our additions are clearly separated.
```

### Minimizing Merge Pain
- Don't reformat upstream code
- Don't rename upstream variables or functions
- Add custom code in clearly delimited blocks (comment markers)
- Keep custom Q_PROPERTYs at the END of the property list in config.h
- Keep custom HEADERS/SOURCES at the END of the lists in remote-ui.pro
