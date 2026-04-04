# Digital Avatar with Moods — UC Remote 3

## Context

Add a mood-reactive avatar system to the UC Remote 3, representing the currently active AI agent from the Project Fronkensteen ecosystem. The avatar reflects the agent's mood (derived from HA voice mood modulation instances, occupancy state, and system context), responds to touch interactions, and gracefully falls back to local-only mood when HA is unreachable.

The avatar appears in three surfaces:
1. **Voice overlay** — replaces the stock listening/response animations when pressing the mic button
2. **Push overlay** — pops up when HA sends banter, notifications, therapy, or theatrical events targeting the UC3
3. **Screensaver overlay** — optional layer on top of any screensaver theme (Matrix, Starfield, Minimal). Not a theme itself — a feature overlay, same pattern as clock/battery overlays. Configurable.

---

## Prototyping Results (2026-04-03)

### What was tested

Ran 6 prototype iterations on macOS via `qmlscene` at 480x850 (UC3 display size) using the Rick Braille portrait from `ASCII_Rick_base.md` (30 rows x 45 cols, ~1350 Braille cells = ~90x120 effective pixels).

### Approaches tested and rejected

| # | Approach | Result | Why it failed |
|---|---|---|---|
| v1 | Embed 4 lines of art, basic animation | Rendered at 15fps, Braille font works | Only proved rendering — too little art |
| v2 | Full portrait + cell-level bit masking for expressions | All expressions looked identical | Region coordinates were guesses; cell-level ops too coarse |
| v3 | Per-cell dot shifting (shiftDotsDown/Up/Left/Right) | Destroyed the art | Shifting dots within cells breaks the image at neighboring boundaries |
| v4 | Pixel-level bitmap (decode Braille->90x120 bitmap, animate, re-encode) | Mouth looked like South Park Canadians | Splitting/shifting pixel regions creates mechanical flap effects, not organic motion |
| v5 | Static portrait + pixel-animated overlays at mapped face regions | Slightly better but still crude | Even at pixel level, 24x28px eye and 38x24px mouth don't have enough resolution for smooth deformation |

### Key finding: procedural deformation fails, but cell-level sprite animation works

At ~90x120 effective pixels, **procedural deformation** fails — shifting, masking, and bitmap manipulation produce visible mechanical artifacts because neighboring pixels depend on each other. This is a hard physical limitation of the deformation approach, not the platform.

**However:** the v1-v5 prototypes all used QML Text (one big string element). They couldn't change individual characters independently. The Matrix screensaver's C++ renderer changes 4,000+ individual characters per frame for glitch/chaos effects at 40fps on the UC3 ARM SoC. The same per-cell rendering technology enables **sprite-style facial animation** — pre-drawn region variants swapped at the character cell level.

**Revised conclusion:** Procedural deformation is dead. Per-cell sprite animation via C++ character grid is the path forward.

### Approach: Character Grid Renderer — ACCEPTED

**Core insight:** The entire avatar display is a full-screen character grid (C++ QQuickItem), where each cell holds a Braille character with independent brightness and color. The portrait, glow, particles, voice visualizer, response text — everything is rendered as characters in a single GPU draw call via texture atlas. This is a simplified fork of the proven MatrixRainItem architecture.

**Ambient life effects (per-cell, all in C++):**

| Effect | Implementation |
|---|---|
| **Mood color** | Atlas built with mood color palette. Smooth transition = rebuild atlas with interpolated color. |
| **Breathing** | Per-cell brightness sine wave on portrait cells (`brightness[i] = base + sin(t) * 0.05`) |
| **Ambient glow** | Cells surrounding portrait get brightness falloff based on distance from portrait edge |
| **Voice visualizer** | Bottom row(s) of cells, brightness driven by voice activity state |
| **Mood flash** | All portrait cells spike to max brightness for 3-4 frames on mood transition |
| **Floating particles** | Random cells outside portrait area get brief brightness pulses, drift via cell coordinate changes |
| **Eye blinks** | Swap ~184 eye-region cells to "closed" variant, hold 150ms, swap back |
| **Talking** | Cycle mouth-region cells between talk-A and talk-B variants while voice is active |
| **Expression changes** | Swap eye + mouth region cells to mood-appropriate variants |

**Why this works when v1-v5 failed:**

| v1-v5 (QML Text, failed) | Character grid (C++ renderer) |
|---|---|
| One monolithic Text element | Per-cell independent control |
| Black rectangle overlay for blink | Swap eye cells to pre-drawn "closed eyes" Braille characters |
| Pixel-level bitmap manipulation for mouth | Swap mouth cells to pre-drawn variant |
| All-or-nothing portrait change | Any subset of cells can change independently |
| Procedural deformation | Pre-drawn region variants, cell-level swap |

### Face region mapping (ACTIVE — used for animation)

Built an interactive drag-to-select mapper tool (`test_braille_mapper.qml`). User mapped Rick's face regions:

```
LEFT EYE:  rows 12-19, cols 11-23  (~96 cells)
RIGHT EYE: rows 12-19, cols 23-34  (~88 cells)
MOUTH:     rows 22-28, cols 14-33  (~133 cells)
```

These coordinates drive the cell-level animation system. Each region can be independently swapped to a variant. A blink = 184 cell writes. The Matrix screensaver does 4,000+ cell writes per frame for chaos events — 184 is trivial.

### Multi-portrait + region variant approach — DECIDED

Animation uses two layers:

1. **Base portraits** — full 30x45 grid per mood (neutral, happy, angry, sleepy, etc.). Defines the overall expression and feel.
2. **Region variants** — small patches for eyes and mouth (10-20 cols x 7-8 rows). Define micro-expressions: blink, talk, smile, frown, look-left/right.

**Compositing per frame:**
```
Base portrait (full grid — mood-specific)
  + Eye state (open | closed | half | left | right)
  + Mouth state (neutral | smile | frown | open | talk-A | talk-B)
  = Composited grid cells (written per-frame by C++ renderer)
```

**What the artist draws:**
- 1 full base portrait per mood group (same as before: neutral, happy, negative, low, special)
- Eye region patches: open, closed, half-closed, looking-left, looking-right (~2 per expression)
- Mouth region patches: neutral, smile, frown, open, talk-A, talk-B (~6 variants)
- Total per character: ~5 base portraits + ~10-15 small region patches (vs. 5 complete portraits)

**Mood transitions:** Interpolate between two base portraits by progressively swapping cells from old to new grid, combined with brightness flash. Not instant — cells morph over ~300ms.

### Agent signature colors (confirmed)

| Agent | Color | Hex |
|---|---|---|
| Rick | Cyan | `#97d5e0` |
| Quark | Gold | `#ffd700` |
| Kramer | Warm brown | `#d4956a` |
| Deadpool | Red | `#ff3333` |
| Dr. Portuondo | Olive | `#8b7d3c` |

### Performance confirmed

- **QML Text prototype:** 1350 Braille chars at 15fps on macOS (v6 prototype). Proved the visual concept.
- **C++ character grid (production):** The MatrixRainItem renderer handles 4,000-16,000+ cells at 40fps on UC3 ARM SoC. The avatar grid at ~4,556 cells (68x67 at 14px) is well within budget. Single GPU draw call via texture atlas — proven architecture.
- **Font on UC3: RESOLVED.** GlyphAtlas pre-renders all Braille characters into a GPU texture at atlas build time. Font bundled in `deploy/config/BrailleFont.otf` (same pattern as `NotoSansMonoCJKjp.otf`, 23KB). After atlas build, font file is not needed — rendering is pure texture lookups. No runtime font dependency.
- Braille Unicode renders correctly in Menlo font at 14px; font.pixelSize 14, lineHeight 0.9 (macOS prototype). On UC3, the bundled font handles this.

---

## Architecture Overview

```
  HA Server                                            UC Remote 3
  +---------------------------------------------+     +------------------------------+
  |                                             |     |  remote-ui (custom binary)   |
  | voice_mood_modulation_{agent}               |     |                              |
  | (5 blueprint instances, hourly)             |     |  +------------------------+  |
  | +---------------------------------------+   |     |  | AvatarOverlay.qml      |  |
  | | Rick:  [slurring] stab=0.2  @21-5     |   |     |  | (unified popup)        |  |
  | | Quark: [whispers] stab=0.45 @22-5     |   |     |  |                        |  |
  | | Kramer:[whispers] stab=0.45 @21-5     |   |     |  | Mode A: VOICE          |  |
  | | Deadpl:[whispers] stab=0.45 @21-9     |   |     |  |  mic btn -> listen ->  |  |
  | | DrPort:[whispers] stab=0.5  @21-5     |   |     |  |  process -> response   |  |
  | +-----------+---------------------------+   |     |  |  (Voice singleton)      |  |
  |             | writes hourly                 |     |  |                        |  |
  |             v                               |     |  | Mode B: PUSH           |  |
  | sensor.ai_voice_mood_{agent}_tags           |     |  |  HA sensor trigger ->  |  |
  | input_number.ai_voice_mood_*_stability      |     |  |  show text -> dismiss  |  |
  |             |                               |     |  |  (entity valueChanged) |  |
  |             | consumed by                   |     |  |                        |  |
  |             v                               |     |  | Shared: AvatarDisplay  |  |
  | pyscript/avatar_mood.py                     |     |  |  portrait + ambient    |  |
  | +---------------------------------------+   |     |  |  life + voice viz      |  |
  | | READS existing mood infra:            |   |     |  +------------------------+  |
  | |  voice_mood_{agent}_tags              |   |     |  +------------------------+  |
  | |  voice_mood_{agent}_stability         |   |     |  | Screensaver Overlay    |  |
  | |  ai_last_interaction                  |   |     |  | (on any theme)         |  |
  | |  occupancy_mode                       |   |     |  +------------------------+  |
  | |  bedtime/winddown/theatrical          |   |     |  +------------------------+  |
  | |                                       |   |     |  | Config (QSettings)     |  |
  | | SERVICES:                             |   |     |  | All avatar prefs local |  |
  | |  avatar_notify(mode, text, ...)       |   |     |  +------------------------+  |
  | |  avatar_dismiss()                     |   |     |  +------------------------+  |
  | +---------------------------------------+   |     |  | Local Fallback Engine  |  |
  |             | state.set()                   |     |  | Battery + Time -> Mood |  |
  |             v                               |     |  +------------------------+  |
  | sensor.ai_avatar_character -----------------+---->|                              |
  | sensor.ai_avatar_mood      -----------------+---->| EntityController.get()       |
  | sensor.ai_avatar_energy    -----------------+---->| QML property bindings        |
  | sensor.ai_avatar_context   -----------------+---->|                              |
  | sensor.ai_avatar_active    -----------------+---->| TRIGGER: opens/closes popup  |
  | sensor.ai_avatar_text      -----------------+---->| TEXT: displayed under avatar  |
  +---------------------------------------------+     +------------------------------+
```

**Key design decisions:**
1. **No separate integration driver.** The existing HA integration bridges HA sensor entities to UC3 (entity_id passthrough, confirmed from integration source).
2. **avatar_mood.py is a CONSUMER, not a creator.** Reads existing `voice_mood_modulation_{agent}` output (tags + stability). Zero duplication.
3. **Real-time overlays on top of hourly base.** Hourly tags/stability = base mood arc. Event-driven triggers (bedtime, theatrical, agent change) = real-time context modifiers.
4. **Avatar is a feature overlay, not a theme.** The avatar is a configurable layer that can appear on top of ANY screensaver theme (Matrix, Starfield, Minimal), not a theme itself. Follows the same pattern as the existing clock and battery overlays. Toggled via `Config.avatarShowOnScreensaver`. The screensaver and avatar are separate systems that optionally coexist.
5. **Two-layer composited rendering (when on screensaver).** Two independent QQuickItems z-stacked in QML:
   - **[z:0] Active screensaver theme** — Matrix/Starfield/Minimal, unchanged.
   - **[z:1] AvatarGridItem** — renders portrait + ambient cells only. Background transparent — screensaver shows through. Portrait cells opaque, occlude theme beneath.
   - Two GPU draw calls when both active. No shared grid, no simulation entanglement.
   - **Screensaver reacts to avatar mood via QML property bindings** — mood changes drive rain color/speed/glitch on Matrix theme. Other themes get simpler reactions (color tint). Avatar controls the screensaver as a mood amplifier without sharing code.
6. **Cell-level sprite animation (AvatarGridItem).** Per-cell character/brightness/color control via GlyphAtlas texture atlas (shared with screensaver). Pre-drawn region variants (eyes, mouth) swapped at the character cell level. Eye blink = ~184 cell writes. Talking = mouth cell cycling. Expression changes = eye + mouth region swap.
7. **Unified overlay, two modes.** One AvatarOverlay component serves both voice (mic button) and push (HA-initiated) interactions. AvatarGridItem visual, different lifecycle drivers.
7. **Push channel via entity state.** HA triggers the UC3 avatar by writing to `sensor.ai_avatar_active`. The existing Core API WebSocket delivers entity changes in real time. No new protocol.
8. **Remote-only by design.** Voice mode is inherently UC3-only (physical mic button). Push mode is opt-in per blueprint instance (`show_on_uc3` input). Satellite interactions never trigger the avatar.

---

## Constraints (from research + review)

| Constraint | Impact | Mitigation |
|---|---|---|
| UC3 sensor entities only carry state/value/unit (sensor.h:260-283) | Can't pass custom attributes in one entity | Use 6 sensor entities (one per data dimension) |
| Entity ID format: `hass.main.{ha_entity_id}` (confirmed from integration source) | Plan must use full prefixed IDs | `hass.main.sensor.ai_avatar_mood` etc. |
| EntityController loads entities on demand (entityController.h:71) | Avatar entities not auto-available | Call `EntityController.load()` on component creation; listen for `entityLoaded` signal |
| Two-layer compositing = two GPU draw calls | Marginal perf cost vs. single renderer | Proven fine on ARM for two items. Avoids entangling rain simulation with portrait logic — clean separation worth the extra draw call. |
| Braille art ~90x120 effective pixels | Not enough resolution for procedural deformation | Cell-level sprite animation (pre-drawn region variants) instead |
| Sensor `getValue()` returns QString (sensor.h:280) | Energy is a string "0"-"100", not int | Use `parseInt()` in QML for numeric comparisons |
| `state.set()` sensors don't persist across HA restart | Sensors briefly absent after reboot | state_bridge.py seeds on startup; QML null-guards entity access |
| Menlo font is macOS-only | Braille may not render on UC3 via QML Text | Non-issue: GlyphAtlas pre-renders Braille into GPU texture at build time. Bundle font in `deploy/config/` (same pattern as `NotoSansMonoCJKjp.otf`). After atlas build, font availability doesn't matter. |

---

## Phase 1: QML Avatar Framework

### 1.1 New Files (UC-Remote-UI)

**C++ renderer — new QQuickItem, NOT a fork of MatrixRainItem:**

AvatarGridItem is a separate renderer that shares only GlyphAtlas with the screensaver. It does NOT inherit from, extend, or modify MatrixRainItem. The two renderers are independent and composed via QML z-stacking.

| File | Purpose | Est. lines |
|---|---|---|
| `src/ui/avatargrid.h` | `AvatarGridItem` — QQuickItem subclass. Owns grid model (`m_gridChar[]`, `m_gridBright[]`), animation timer, portrait compositing, region variant swapping. | ~150 |
| `src/ui/avatargrid.cpp` | Portrait loading, per-cell animation (breathing, glow, particles, blink/talk), QSG vertex buffer rendering via atlas texture. | ~400 |

**What's copied from MatrixRainItem (pattern, not code sharing):**
- `MatrixRainNode` destructor pattern (GPU texture cleanup on render thread) — ~10 lines
- Atlas texture upload in `updatePaintNode()` (`matrixrain.cpp:115-131`) — ~20 lines
- Quad vertex/index buffer construction (`matrixrain.cpp:266-273`) — ~10 lines
- Timer/tick/displayOff pattern — ~15 lines
- `componentComplete()` deferred init pattern — ~20 lines

**What's NOT copied — written from scratch:**
- Grid cell iteration (simpler than stream trail walk — just iterate all non-empty cells)
- Portrait zone management (which cells hold portrait chars)
- Region variant compositing (overlay eye/mouth patches onto base portrait)
- All per-cell animation (breathing sine, glow distance, particle pulses, blink timer, talk cycle)
- Manifest JSON loader

**Honest reuse estimate: ~60% copied patterns, ~40% new code.**

**Modified from Matrix screensaver (minor):**

| File | Change |
|---|---|
| `src/ui/glyphatlas.h/.cpp` | Add `"braille"` to `charsetString()` (1 line + charset string constant). Add `loadBrailleFont()` alongside existing `loadCJKFont()` (~15 lines, same pattern). |

**QML + settings:**

| File | Purpose |
|---|---|
| `src/qml/components/avatar/AvatarDisplay.qml` | Thin QML wrapper around `AvatarGridItem` C++ type. Exposes properties (mood, character, voiceActive, displayOff) and translates QML signals to C++ method calls. |
| `src/qml/components/avatar/MoodEngine.qml` | Mood state resolver: entity values -> resolved mood + energy + color. Uses QML States/Transitions pattern. |
| `src/qml/components/avatar/TouchHandler.qml` | Touch interaction controller (tap/swipe/hold -> mood reactions) |
| `src/qml/components/avatar/AvatarPushHandler.qml` | Entity watcher for push mode — monitors `sensor.ai_avatar_active`, controls popup show/hide lifecycle |
| `src/qml/components/AvatarOverlay.qml` | **Unified popup** — hosts AvatarDisplay + text area. Two modes: voice (driven by Voice singleton) and push (driven by entity changes). |
| `src/qml/components/avatar/AvatarScreensaverOverlay.qml` | Optional avatar layer on top of any screensaver theme (loaded by ChargingScreen.qml when `Config.avatarShowOnScreensaver` is true) |
| `src/qml/settings/settings/Avatar.qml` | Avatar settings page |

**Art assets (compiled into qrc):**

| File | Purpose |
|---|---|
| `src/qml/components/avatar/art/manifest.json` | Character -> mood -> art file mapping |
| `src/qml/components/avatar/art/*.txt` | Braille portrait text files + region variant patches |

**Bundled font:**

| File | Purpose |
|---|---|
| `deploy/config/BrailleFont.otf` | Monospace font with Braille Unicode block (U+2800-U+28FF) subset. ~30-50KB. Created via `pyftsubset` (same technique as `NotoSansMonoCJKjp.otf`). |

### 1.2 Modified Files (UC-Remote-UI)

| File | Change |
|---|---|
| `src/main.cpp` | Add `#include "ui/avatargrid.h"` + `qmlRegisterType<AvatarGridItem>("AvatarGrid", 1, 0, "AvatarGrid")` + `GlyphAtlas::loadBrailleFont()` |
| `src/ui/glyphatlas.h/.cpp` | Add `"braille"` charset (U+2800-U+28FF). Add `loadBrailleFont()` (same pattern as `loadCJKFont()`). |
| `remote-ui.pro` | Add `avatargrid.h/.cpp` to HEADERS/SOURCES |
| `src/qml/components/VoiceOverlay.qml` | Add conditional: when `Config.avatarEnabled`, delegate to AvatarOverlay in voice mode instead of showing EQ bars/circles. Pass Voice singleton signals through. |
| `src/qml/components/ChargingScreen.qml` | Add AvatarScreensaverOverlay Loader (active when `Config.avatarShowOnScreensaver`). Sits above theme Loader at z:1. Passes `displayOff`/`isClosing` through. |
| `src/qml/main.qml` | Add AvatarOverlay Loader for push mode (alongside existing VoiceOverlay and ChargingScreen loaders, ~line 460 area). Wire `sensor.ai_avatar_active` entity changes to popup open/close. |
| `src/qml/settings/settings/ChargingScreen.qml` | Add "Show avatar on screensaver" toggle (below existing theme selector) |
| `src/qml/settings/Settings.qml` | Add "Avatar" menu entry |
| `src/config/config.h` | Add ~20 avatar Q_PROPERTY declarations (using CFG_BOOL/CFG_INT/CFG_STRING macros) |
| `src/config/config.cpp` | Add avatar getter/setter/defaults (via config_macros.h) |
| `resources/qrc/main.qrc` | Register new QML + art asset files |

### 1.3 Config Properties (QSettings)

```ini
[avatar]
enabled=false                # Master kill switch for all avatar features
character=auto               # auto | rick | quark | kramer | deadpool | portuondo
mood_source=hybrid           # ha | local | hybrid
ha_prefix=hass.main          # HA integration instance prefix for entity IDs (auto-discovered or manual)
show_on_screensaver=false    # Show avatar as overlay on active screensaver theme (any theme). Uses slightly more battery when undocked.
voice_overlay=true           # Replace stock voice listening/response with avatar
voice_local_audio=false      # Play TTS audio on UC3 speaker during voice mode (false = visual-only, audio on room speakers via reroute)
push_overlay=true            # Show avatar for HA-initiated push events
push_wake_display=false      # Wake UC3 display when push event arrives (requires Core API power command)
push_charging_inline=true    # When screensaver is active: absorb push events in-place (true) vs. popup overlay on top (false)
overlay_background=gradient  # black | gradient | mood (overlay background style)
overlay_opacity=90           # 10-100 (percent)
touch_enabled=true           # Respond to touch on screensaver
touch_reactions=true         # Poke reactions (surprise, annoy)
show_clock=false             # Clock overlay (screensaver only)
show_battery=true            # Battery overlay (screensaver only)
show_mood_label=true         # Text label of current mood
show_text=true               # Show response/banter text below avatar
text_mode=truncate           # truncate | scroll | paginate (response text overflow)
transition_style=flash       # flash | crossfade | slide (mood/character transition animation)
```

Each property gets a Q_PROPERTY + NOTIFY signal in config.h. Charging theme uses `hasOwnProperty` guards, so missing properties are safe.

**Dynamic integration prefix:** `ha_prefix` defaults to `"hass.main"` but can be changed if the user's HA integration instance has a different ID. Entity IDs are constructed at runtime as `Config.avatarHaPrefix + ".sensor.ai_avatar_" + name`. This avoids hardcoding and supports non-standard setups.

### 1.4 Two-Layer Composited Rendering

**Architecture:** Two independent QQuickItems z-stacked in QML. Each has its own simulation, its own timer, its own rendering. Connected only through QML property bindings.

```qml
// Charging theme: rain behind, avatar on top
Item {
    MatrixRain {
        id: rain
        anchors.fill: parent
        z: 0
        // Rain reacts to avatar mood via bindings (see 1.4.3)
    }
    AvatarGrid {
        id: avatar
        anchors.fill: parent
        z: 1
        // Portrait cells opaque, background transparent
        // Rain visible through empty cells
    }
}

// Voice/push overlay: avatar only (no rain), or avatar + rain (configurable)
```

#### 1.4.1 AvatarGridItem (new C++ class)

Renders portrait + ambient cells. Background is transparent — non-portrait cells are empty (no quad emitted), so the layer behind (rain or black) shows through.

**Grid model:**
```cpp
int m_gridCols, m_gridRows;  // ~68 x 67 at 14px font on 480x850

// Per-cell state (only portrait + ambient cells are populated)
QVector<int>   m_gridChar;    // Braille character index (into atlas). -1 = empty/transparent.
QVector<int>   m_gridBright;  // brightness level (0 = brightest, blevels-1 = dimmest). Ignored if char is -1.

// Portrait placement (centered in grid)
int m_portraitOffsetRow, m_portraitOffsetCol;
int m_portraitRows, m_portraitCols;  // 30 x 45 for Rick

// Region definitions (loaded from manifest)
struct Region { int row, col, rows, cols; };
Region m_leftEye, m_rightEye, m_mouth;
```

**Simulation tick (~20fps):**
```cpp
void AvatarGridItem::tick() {
    if (m_displayOff) return;
    float t = m_tickCount * 0.05f;  // elapsed seconds at 20fps

    // 1. Breathing — portrait cells brightness sine wave
    float breathOffset = sin(t * 1.2f) * 0.8f;  // ±0.8 brightness levels
    applyBreathing(breathOffset);

    // 2. Ambient glow — cells surrounding portrait, brightness falloff with distance
    applyGlow(t);

    // 3. Floating particles — random non-portrait cells pulse briefly
    advanceParticles();

    // 4. Voice visualizer — bottom rows brightness driven by m_voiceActive
    if (m_voiceActive) advanceVoiceViz(t);

    // 5. Eye blink — periodic region swap (every 3-6s, hold 150ms)
    advanceBlink();

    // 6. Talk animation — mouth region cycles talkA/talkB while m_voiceActive
    if (m_voiceActive) advanceTalk();

    m_tickCount++;
    update();  // triggers updatePaintNode()
}
```

**Rendering (`updatePaintNode`):**
- Copies atlas texture upload and `MatrixRainNode` cleanup pattern from `matrixrain.cpp:105-131`
- Iterates all cells, skips where `m_gridChar[i] == -1` (transparent)
- Per-cell: atlas UV lookup from `(glyphIdx, brightLevel)`, quad vertex construction
- Copies quad vertex/index pattern from `matrixrain.cpp:266-273`
- Single `QSGGeometryNode` draw call with `QSGTextureMaterial` (alpha blending enabled for transparency)

**Key Q_PROPERTYs:**
```cpp
// Lifecycle
Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
Q_PROPERTY(bool displayOff READ displayOff WRITE setDisplayOff NOTIFY displayOffChanged)

// Avatar state (driven by QML MoodEngine / VoiceOverlay / PushHandler)
Q_PROPERTY(bool voiceActive READ voiceActive WRITE setVoiceActive NOTIFY voiceActiveChanged)
Q_PROPERTY(QString mood READ mood WRITE setMood NOTIFY moodChanged)
Q_PROPERTY(QString character READ character WRITE setCharacter NOTIFY characterChanged)
Q_PROPERTY(QColor moodColor READ moodColor WRITE setMoodColor NOTIFY moodColorChanged)
Q_PROPERTY(QString eyeState READ eyeState WRITE setEyeState NOTIFY eyeStateChanged)
Q_PROPERTY(QString mouthState READ mouthState WRITE setMouthState NOTIFY mouthStateChanged)

// Response text (rendered as characters in grid rows below portrait)
Q_PROPERTY(QString responseText READ responseText WRITE setResponseText NOTIFY responseTextChanged)
Q_PROPERTY(QString textMode READ textMode WRITE setTextMode NOTIFY textModeChanged)

// Visual config
Q_PROPERTY(QString transitionStyle READ transitionStyle WRITE setTransitionStyle NOTIFY transitionStyleChanged)
Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
```

**Font handling:** GlyphAtlas loads bundled `BrailleFont.otf` from `deploy/config/` at startup (same `loadCJKFont()` pattern). All 256 Braille characters pre-rendered into atlas texture. After build, font is no longer needed — pure GPU texture. Font problem resolved.

**displayOff:** Timer stops when `m_displayOff` is true. Zero CPU/GPU when screen is off. Same pattern as MatrixRainItem.

**Response text:** Long text rendered as characters in grid rows below portrait. `textMode` controls overflow: `truncate` (elide), `scroll` (auto-scroll ~2 cells/sec), `paginate` (page on timer).

#### 1.4.2 Screensaver themes (existing, unchanged)

MatrixRainItem, StarfieldTheme, MinimalTheme — all used as-is. No modifications. The avatar overlay sits on top via QML z-stacking. When `Config.avatarShowOnScreensaver` is false, screensaver runs exactly as before. When true, the avatar overlay loads on top and optionally drives mood-reactive properties on the theme (see 1.4.3).

#### 1.4.3 QML Mood-to-Rain Bindings

The avatar doesn't modify rain code. It controls rain behavior through QML property bindings — the same way the settings page already controls the screensaver:

```qml
// In AvatarScreensaverOverlay.qml
MatrixRain {
    id: rain
    // Base rain config from settings
    color: Config.chargingMatrixColor
    speed: Config.chargingMatrixSpeed / 50.0

    // Mood-reactive overrides (when avatar is active)
    Behavior on color { ColorAnimation { duration: 1000 } }
}

AvatarGrid {
    id: avatar

    onMoodChanged: {
        // Rain reacts to avatar mood
        switch (mood) {
            case "angry":
            case "annoyed":
                rain.glitchRate = 80;
                rain.speed = 2.0;
                rain.color = avatar.moodColor;
                break;
            case "sleepy":
            case "sad":
                rain.speed = 0.3;
                rain.density = 0.4;
                rain.color = Qt.darker(avatar.moodColor, 1.5);
                break;
            case "dramatic":
                rain.triggerChaosBurst();  // existing Q_INVOKABLE
                rain.color = avatar.moodColor;
                break;
            default:
                rain.speed = Config.chargingMatrixSpeed / 50.0;  // back to user setting
                rain.color = avatar.moodColor;
                break;
        }
    }

    onCharacterChanged: {
        // Character entrance: chaos burst, rain goes agent color
        rain.triggerChaosBurst();
        rain.color = avatar.moodColor;
    }
}
```

This gives the "dope effects" — rain reacting to mood — without the two renderers sharing any code. The rain is a mood amplifier controlled through the same property interface the settings page uses.

### 1.5 Portrait Art Requirements

**Two types of art assets:**

**A. Base portraits** — full 30x45 Braille grid per mood group:

| Mood group | Moods covered | Portrait needed |
|---|---|---|
| neutral | neutral, thinking, curious | 1 portrait |
| happy | happy, amused, excited, greeting | 1 portrait |
| negative | angry, annoyed | 1 portrait |
| low | sleepy, sad | 1 portrait |
| special | mischievous, dramatic, surprised, confused | 1 portrait each or group as desired |

**B. Region variant patches** — small Braille grids covering eye/mouth regions only:

| Region | Size (approx) | Variants needed |
|---|---|---|
| Eyes (both) | ~24 cols x 8 rows | open (default in base), closed, half-closed, looking-left, looking-right |
| Mouth | ~20 cols x 7 rows | neutral (default in base), smile, frown, open, talk-A, talk-B |

Region patches overlay the base portrait at the mapped coordinates. Only the cells in the patch change — surrounding cells stay from the base portrait.

**Minimum viable:** 1 base portrait (neutral) + eyes-closed patch + talk-A/B mouth patches. Gives you blinks and talking animation. All mood conveyed through per-cell color/brightness.

**Full set:** 5 base portraits + 5 eye variants + 6 mouth variants per character. Full expression range.

Art format: plain text file, one portrait or region patch per file. Compiled into qrc for reliability.

### 1.5.1 Art Management System

Braille portraits are managed via a JSON manifest + text files, all compiled into qrc:

```
src/qml/components/avatar/art/
  manifest.json          # character -> mood -> file mapping
  rick_neutral.txt       # raw Braille text, one portrait per file
  rick_happy.txt
  rick_negative.txt
  rick_low.txt
  rick_mischievous.txt
  rick_dramatic.txt
  quark_neutral.txt      # added later as characters are created
  ...
```

**Manifest format (`manifest.json`):**
```json
{
  "characters": {
    "rick": {
      "display_name": "Rick",
      "color": "#97d5e0",
      "portrait_rows": 30,
      "portrait_cols": 45,
      "moods": {
        "neutral":     { "file": "rick_neutral.txt",     "groups": ["neutral", "thinking", "curious"] },
        "happy":       { "file": "rick_happy.txt",       "groups": ["happy", "amused", "excited", "greeting"] },
        "negative":    { "file": "rick_negative.txt",    "groups": ["angry", "annoyed"] },
        "low":         { "file": "rick_low.txt",         "groups": ["sleepy", "sad"] },
        "mischievous": { "file": "rick_mischievous.txt", "groups": ["mischievous"] },
        "dramatic":    { "file": "rick_dramatic.txt",    "groups": ["dramatic", "surprised", "confused"] }
      },
      "regions": {
        "left_eye":  { "row": 12, "col": 11, "rows": 8, "cols": 12 },
        "right_eye": { "row": 12, "col": 23, "rows": 8, "cols": 11 },
        "mouth":     { "row": 22, "col": 14, "rows": 7, "cols": 19 }
      },
      "eye_variants": {
        "open":    null,
        "closed":  { "file": "rick_eyes_closed.txt" },
        "half":    { "file": "rick_eyes_half.txt" },
        "left":    { "file": "rick_eyes_left.txt" },
        "right":   { "file": "rick_eyes_right.txt" }
      },
      "mouth_variants": {
        "neutral": null,
        "smile":   { "file": "rick_mouth_smile.txt" },
        "frown":   { "file": "rick_mouth_frown.txt" },
        "open":    { "file": "rick_mouth_open.txt" },
        "talk_a":  { "file": "rick_mouth_talk_a.txt" },
        "talk_b":  { "file": "rick_mouth_talk_b.txt" }
      },
      "fallback": "neutral"
    }
  },
  "font": {
    "family": "monospace",
    "pixelSize": 14,
    "lineHeight": 0.9
  }
}
```

**How it works:**
1. AvatarDisplay loads `manifest.json` at startup, caches all portrait text in a JS object
2. MoodEngine resolves mood name (e.g., "amused") -> manifest walks `groups` arrays to find the matching mood entry -> loads that portrait's text
3. If no mood matches, falls back to the character's `fallback` entry
4. If character not in manifest, shows nothing (or a placeholder)

**Adding a new character:** Create text files, add a character entry to `manifest.json`, rebuild. No QML code changes.

**Adding a mood variant:** Add a text file, add/update the mood entry in the character's `moods` object, rebuild. No QML code changes.

**Hot-reload for development:** During prototyping on macOS, art files can be loaded from filesystem instead of qrc by setting `Config.avatarArtPath` to a local directory. For production (UC3), always use qrc.

### 1.6 Unified AvatarOverlay — Two Modes

AvatarOverlay.qml is a `Popup` (follows VoiceOverlay pattern: full-screen, `modal: false`). It hosts AvatarDisplay and a text area, and operates in two modes:

```qml
Popup {
    id: avatarOverlay
    property string mode: ""   // "voice" | "banter" | "therapy" | "theatrical" | "notification"

    // Background — configurable style
    background: Item {
        Rectangle {
            anchors.fill: parent
            visible: Config.avatarOverlayBackground === "black"
            color: "black"
        }
        // Gradient: transparent top -> black bottom (matches stock VoiceOverlay)
        Item {
            anchors.fill: parent
            visible: Config.avatarOverlayBackground === "gradient"
            LinearGradient { /* top half: transparent -> black */ }
            Rectangle { /* bottom half: solid black */ }
        }
        // Mood-colored: subtle tinted background
        Rectangle {
            anchors.fill: parent
            visible: Config.avatarOverlayBackground === "mood"
            color: Qt.rgba(moodEngine.moodColor.r, moodEngine.moodColor.g, moodEngine.moodColor.b, 0.15)
        }
    }

    // Shared visual — portrait centered, text below
    AvatarDisplay {
        id: display
        anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; verticalCenterOffset: -60 }
        displayOff: false
        voiceActive: avatarOverlay.mode === "voice" && voiceState.listening
        currentMood: moodEngine.resolvedMood
    }

    // Text area (below avatar)
    Text {
        id: responseText
        visible: Config.avatarShowText && text !== ""
        text: ""   // set by voice mode (STT/response) or push mode (entity value)
        anchors { top: display.bottom; topMargin: 20; horizontalCenter: parent.horizontalCenter }
        width: parent.width - 80
        wrapMode: Text.WordWrap
        maximumLineCount: 6
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        color: colors.offwhite
        font: fonts.primaryFont(24)
    }

    // Mode A: Voice — driven by Voice singleton
    // Mode B: Push — driven by AvatarPushHandler (entity watcher)
}
```

**Mode A — Voice (mic button):**

VoiceOverlay.qml delegates to AvatarOverlay when `Config.avatarEnabled && Config.avatarVoiceOverlay`:

```
VoiceOverlay.start() → avatarOverlay.mode = "voice" → avatarOverlay.open()
Voice.assistantEventReady     → voiceActive = true (viz animates)
Voice.assistantEventSttResponse → responseText = transcription
Voice.assistantEventTextResponse → responseText = agent response
Voice.assistantEventSpeechResponse → voiceActive = true (during TTS)
Voice.assistantAudioSpeechResponseEnd → voiceActive = false
Voice.assistantEventFinished  → success animation → close
Voice.assistantEventError     → error flash → close
VoiceOverlay.stop()           → voiceActive = false, processing mood
```

The Voice singleton signals provide the voice visualizer's data source — no HA sensor needed for "is talking."

**Local audio toggle:** When `Config.avatarVoiceLocalAudio` is true, the UC3 plays TTS audio through its own speaker via `Voice.playSpeechResponse()` (stock behavior). When false (recommended — `assist_tts_reroute` already sends audio to room speakers), the speech response URL is captured but not played locally. The `assistantAudioSpeechResponseEnd` signal still fires for popup auto-close timing regardless of this setting. This avoids double-audio (UC3 speaker + room speaker) when reroute is active.

**Mode B — Push (HA-initiated):**

AvatarPushHandler.qml watches `sensor.ai_avatar_active` for value changes:

```
Entity value: "idle" -> "banter"  → avatarOverlay.mode = "banter" → open()
Entity value: text updates        → responseText = sensor.ai_avatar_text value
Entity value: "banter" -> "idle"  → close animation → close()
```

Push mode behavior per event type:

| Mode | Show duration | Text updates | Character changes | Voice viz |
|---|---|---|---|---|
| `banter` | Until HA sends `idle` (or 30s timeout) | Single text | No | Brief pulse |
| `notification` | Until HA sends `idle` (or 15s timeout) | Single text | No | No |
| `therapy` | Until HA sends `idle` (session end) | Per-turn updates | No | Per-turn pulse |
| `theatrical` | Until HA sends `idle` (debate end) | Per-turn updates | Yes (agents swap) | Per-turn pulse |

**Timeout fallback:** If HA never sends `idle` (crash, network drop), QML auto-closes after mode-specific timeout. Prevents stuck popup.

**Push event collision:** Not handled in QML. The HA-side `tts_queue` already serializes audio delivery, and blueprints call `avatar_notify`/`avatar_dismiss` in sync with their TTS flow. Sequential blueprint execution means the sensor values are naturally serialized. The QML simply displays whatever the latest entity state says.

**Display wake:** When `Config.avatarPushWakeDisplay` is true and the UC3 is sleeping/idle, a push event triggers a Core API power command to wake the display before opening the popup. When false, push events arriving while the screen is off are silently ignored (the entity state updates, but no popup opens — MoodEngine still absorbs the mood update for next wake).

**Charging theme inline mode:** When `Config.avatarPushChargingInline` is true and the avatar screensaver is active, push events update the screensaver's avatar in-place (text appears below portrait, mood/character update, no separate popup). When false, the standard AvatarOverlay popup opens on top of the screensaver. Configurable because some users may prefer the seamless in-place update while others want the distinct popup to clearly signal an active event.

### 1.7 Voice Overlay Integration

The existing `VoiceOverlay.qml` remains the entry point for mic button presses. When avatar is enabled, it conditionally delegates to AvatarOverlay:

```qml
// In VoiceOverlay.qml — modified start() function
function start(entityId, profileId) {
    // ... existing entity setup ...

    if (Config.avatarEnabled && Config.avatarVoiceOverlay) {
        // Delegate to avatar overlay in voice mode
        avatarOverlay.startVoiceMode(entityId, profileId);
        return;
    }

    // ... existing EQ/circle flow (fallback when avatar disabled) ...
    voice.open();
}
```

All existing Voice singleton Connections stay in VoiceOverlay.qml. When in avatar mode, they forward events to AvatarOverlay. When avatar is disabled, the stock EQ bars and circle animations work exactly as before. **Zero breaking changes to non-avatar users.**

### 1.8 Screensaver Avatar Overlay

The avatar is NOT a screensaver theme. It's a feature overlay that sits on top of whatever theme is active, same pattern as the clock and battery overlays. Toggled via `Config.avatarShowOnScreensaver`.

**ChargingScreen.qml changes:**

```qml
// Existing theme Loader (unchanged)
Loader {
    id: themeLoader
    anchors.fill: parent
    z: 0
    source: {
        switch (Config.chargingTheme) {
            case "matrix": return "qrc:/components/themes/MatrixTheme.qml";
            case "starfield": return "qrc:/components/themes/StarfieldTheme.qml";
            case "minimal": return "qrc:/components/themes/MinimalTheme.qml";
            default: return "qrc:/components/themes/MatrixTheme.qml";
        }
    }
}

// NEW: Avatar overlay (loads on top of any theme when enabled)
Loader {
    id: avatarOverlayLoader
    anchors.fill: parent
    z: 1
    active: Config.avatarEnabled && Config.avatarShowOnScreensaver
    source: "qrc:/components/avatar/AvatarScreensaverOverlay.qml"

    onLoaded: {
        if (item) {
            item.displayOff = Qt.binding(function() { return chargingScreenRoot.displayOff; });
            item.isClosing = Qt.binding(function() { return chargingScreenRoot.isClosing; });
            // Pass theme item ref so avatar can drive mood reactions on it
            item.themeItem = Qt.binding(function() { return themeLoader.item; });
        }
    }
}
```

**AvatarScreensaverOverlay.qml:**

```qml
import AvatarGrid 1.0

Item {
    id: root
    anchors.fill: parent
    property bool displayOff: false
    property bool isClosing: false
    property var themeItem: null   // ref to active theme (MatrixRain, etc.)

    AvatarGrid {
        id: avatar
        anchors.fill: parent
        running: root.visible && !root.isClosing && !root.displayOff
        displayOff: root.displayOff
        // Portrait cells opaque, background transparent — theme shows through
    }

    // Mood-to-screensaver bindings (only when theme supports it)
    onThemeItemChanged: {
        if (!themeItem) return;
        // Matrix theme has color, speed, glitchRate, etc.
        // Starfield/Minimal may not — use hasOwnProperty guards
    }

    Connections {
        target: avatar
        function onMoodChanged() {
            if (!root.themeItem) return;
            // Drive screensaver properties based on mood
            if (root.themeItem.hasOwnProperty("color"))
                root.themeItem.color = avatar.moodColor;
            if (root.themeItem.hasOwnProperty("glitchRate") && avatar.mood === "angry")
                root.themeItem.glitchRate = 80;
        }
        function onCharacterChanged() {
            // Chaos burst on character entrance (Matrix theme only)
            if (root.themeItem && root.themeItem.hasOwnProperty("triggerChaosBurst"))
                root.themeItem.triggerChaosBurst();
        }
    }
}
```

This works with any current or future theme. Matrix gets full mood-reactive rain. Starfield/Minimal get color tinting at most. No theme needs to know the avatar exists.

### 1.9 MoodEngine

Resolves mood from entity values + local fallback. Uses QML `States` + `Transitions` pattern (same as `MainContainer.qml`), not `QtQml.StateMachine`.

```qml
Item {
    id: moodEngine
    property string resolvedMood: "neutral"
    property int energy: 50
    property color moodColor: "#97d5e0"
    property string resolvedCharacter: "rick"

    // Entity inputs (null until loaded)
    property var moodEntity: null
    property var energyEntity: null
    property var characterEntity: null
    property var contextEntity: null

    // Resolve mood from entity or local fallback
    function resolveMood() {
        if (moodEntity && moodEntity.value !== "") {
            resolvedMood = moodEntity.value;
        } else {
            resolvedMood = localFallback();
        }
    }

    // Energy: parse string to int
    function resolveEnergy() {
        if (energyEntity && energyEntity.value !== "") {
            energy = parseInt(energyEntity.value) || 50;
        }
    }

    // Local fallback: battery + time of day -> mood
    function localFallback() {
        var hour = new Date().getHours();
        var batt = Battery.level;
        if (batt < 15) return "sleepy";
        if (hour >= 23 || hour < 6) return "sleepy";
        if (hour >= 6 && hour < 9) return "neutral";
        return "neutral";
    }
}
```

### 1.10 TouchHandler

Responds to touch on the avatar (primarily for screensaver; disabled during voice/push modes to avoid conflicts).

- **Single tap:** Brief surprise reaction (flash + particle burst)
- **Double tap:** Cycle mood label display
- **Long press:** Show mood/character info
- **Swipe:** Dismiss (screensaver only)

**Conflict avoidance:** When AvatarOverlay is in voice or push mode, TouchHandler is disabled. For screensaver, ChargingScreen's close-on-tap MouseArea must cooperate: if avatar theme is active and `Config.avatarTouchEnabled`, touch events go to TouchHandler first; ChargingScreen close is triggered by swipe or button press instead.

### 1.11 Settings Page

`settings/settings/Avatar.qml` — standard settings list page:

- **Entity status banner:** If avatar is enabled but any of the 6 HA entities fail to load, show a warning: "Avatar entities not found. Add sensor.ai_avatar_* entities in Web Configurator -> HA Integration." Uses entity load success/failure tracking from the entity watcher. Dismissible, re-checks on page open.
- **Master toggle:** Enable/disable avatar (maps to `Config.avatarEnabled`)
- **Character:** Auto / Rick / Quark / Kramer / Deadpool / Dr. Portuondo
- **Mood source:** HA / Local / Hybrid
- **HA integration prefix:** Text field, default "hass.main" (maps to `Config.avatarHaPrefix`)
- **Voice overlay:** Replace stock listening/response (toggle)
- **Voice local audio:** Play TTS on UC3 speaker during voice mode (toggle, default off)
- **Push overlay:** Show for HA events (toggle)
- **Wake display on push:** Wake UC3 screen for push events (toggle, default off)
- **Charging theme inline:** Absorb push events in screensaver vs. popup overlay (toggle)
- **Overlay background:** Black / Gradient / Mood-colored (select)
- **Show on screensaver:** Overlay avatar on active screensaver theme (toggle, default off). Note: uses slightly more battery when undocked.
- **Touch reactions:** Enable/disable
- **Show text:** Show response text below avatar (toggle)
- **Show mood label:** Show current mood name (toggle)

---

## Phase 2: HA Mood Bridge (pyscript)

Connects the avatar to live HA state by **consuming the existing voice mood modulation infrastructure**. The 5 `voice_mood_modulation_{agent}` blueprint instances already compute per-agent, per-time-block mood data (tags + stability) every hour. `avatar_mood.py` reads this output and maps it to visual mood states. Zero duplication.

### 2.1 Existing Infrastructure (consumed, not modified)

**Blueprint:** `blueprints/automation/madalone/voice_mood_modulation.yaml` (306 lines)

**5 Instances** (automations.yaml):

| Agent | Time Blocks (start hours) | Sample Night Tags | Night Stability |
|---|---|---|---|
| Rick | 5->9->12->17->21->5 | `[slurring]` | 0.20 |
| Quark | 5->9->12->17->22->5 | `[whispers]` | 0.45 |
| Kramer | 5->9->12->17->21->5 | `[whispers]` | 0.45 |
| Deadpool | 9->12->17->21->9 | `[whispers]` | 0.45 |
| Dr. Portuondo | 5->9->13->17->21->5 | `[thick Cuban accent] [whispers]` | 0.50 |

### 2.2 New File: `pyscript/avatar_mood.py`

Pure translator — reads existing mood data + context sensors, maps to visual mood states for UC3.

**Output sensors** (via `state.set()`):

| HA Sensor | UC3 Entity ID | State Value | Source |
|---|---|---|---|
| `sensor.ai_avatar_character` | `hass.main.sensor.ai_avatar_character` | agent name | `sensor.ai_last_interaction` state |
| `sensor.ai_avatar_mood` | `hass.main.sensor.ai_avatar_mood` | one of 15 mood names | mapped from voice mood tags |
| `sensor.ai_avatar_energy` | `hass.main.sensor.ai_avatar_energy` | `0`-`100` (int string) | `input_number.ai_voice_mood_{agent}_stability` x 100 |
| `sensor.ai_avatar_context` | `hass.main.sensor.ai_avatar_context` | context name | context sensor aggregation |
| `sensor.ai_avatar_active` | `hass.main.sensor.ai_avatar_active` | `idle` / `banter` / `therapy` / `theatrical` / `notification` | `avatar_notify` / `avatar_dismiss` services |
| `sensor.ai_avatar_text` | `hass.main.sensor.ai_avatar_text` | response text string | `avatar_notify` service |

**Entity ID format confirmed:** The HA integration driver passes entity_ids through unchanged (source: `integration-home-assistant/src/client/get_states.rs`). The UC3 Core prepends `{integration_id}.` So `sensor.ai_avatar_mood` in HA becomes `hass.main.sensor.ai_avatar_mood` on UC3.

### 2.3 ElevenLabs Audio Tag -> Visual Mood Mapping

```python
_TAG_TO_MOOD = {
    "groaning": "annoyed", "tired": "sleepy", "sighs": "thinking",
    "slurring": "mischievous", "excited": "excited", "mischievously": "mischievous",
    "whispers": "sleepy", "laughs": "amused",
    "happy": "happy", "sad": "sad", "angry": "angry", "annoyed": "annoyed",
    "sarcastic": "mischievous", "curious": "curious", "surprised": "surprised",
    "thoughtful": "thinking", "crying": "sad", "chuckles": "amused", "muttering": "annoyed",
}

# Priority order for multi-tag resolution.
# Voice mood modulation can produce multiple tags (e.g., "[thick Cuban accent] [whispers]").
# Non-mood tags (accents, speaking styles) are ignored. If multiple mood tags match,
# the FIRST match in priority order wins.
_MOOD_PRIORITY = [
    "excited", "angry", "surprised", "mischievous", "amused", "happy",
    "annoyed", "sad", "curious", "thinking", "sleepy",
]

def _resolve_mood_from_tags(tags_str):
    """Extract mood from tag string like '[slurring] [whispers]'. First priority match wins."""
    import re as _re
    tags = _re.findall(r'\[([^\]]+)\]', tags_str)
    matched_moods = []
    for tag in tags:
        tag_lower = tag.lower().strip()
        if tag_lower in _TAG_TO_MOOD:
            matched_moods.append(_TAG_TO_MOOD[tag_lower])
    if not matched_moods:
        return "neutral"
    # Return highest-priority mood
    for mood in _MOOD_PRIORITY:
        if mood in matched_moods:
            return mood
    return matched_moods[0]
```

### 2.4 Context Override Logic

| Priority | Condition | Context | Mood Override |
|---|---|---|---|
| 1 | `sensor.ai_theatrical_mode_active == "on"` | `theatrical` | -> `dramatic` |
| 2 | `sensor.ai_bedtime_active == "on"` | `bedtime` | -> `sleepy` if energy < 30 |
| 3 | `sensor.ai_winddown_active == "on"` | `winddown` | energy -= 20 |
| 4 | `sensor.occupancy_mode == "away"` | `away` | -> `sleepy` (idle) |
| 5 | (none) | `normal` | keep base mood |

All context sensors confirmed migrated to `state.set()` sensors (state_bridge.py seeds them on startup).

### 2.5 Update Triggers

```python
@state_trigger("sensor.ai_last_interaction")
@state_trigger("sensor.ai_voice_mood_rick_tags")
@state_trigger("sensor.ai_voice_mood_quark_tags")
@state_trigger("sensor.ai_voice_mood_kramer_tags")
@state_trigger("sensor.ai_voice_mood_deadpool_tags")
@state_trigger("sensor.ai_voice_mood_doctor_portuondo_tags")
@state_trigger("input_number.ai_voice_mood_rick_stability")
@state_trigger("input_number.ai_voice_mood_quark_stability")
@state_trigger("input_number.ai_voice_mood_kramer_stability")
@state_trigger("input_number.ai_voice_mood_deadpool_stability")
@state_trigger("input_number.ai_voice_mood_doctor_portuondo_stability")
@state_trigger("sensor.ai_bedtime_active")
@state_trigger("sensor.ai_winddown_active")
@state_trigger("sensor.ai_theatrical_mode_active")
@state_trigger("sensor.occupancy_mode")
```

**Active character filter:** When a tag/stability trigger fires, check whether the changed entity belongs to the currently active character (from `sensor.ai_last_interaction`). If Rick's tags change hourly but the user last spoke to Quark, skip the mood update — Quark's mood shouldn't change because Rick's hourly cron fired. Only update when: (a) the trigger matches the active character, or (b) the trigger is a context sensor (bedtime, occupancy, etc.), or (c) `ai_last_interaction` itself changed (character switch).

### 2.6 Push Services

```python
@service
def avatar_notify(mode, text="", character=None, mood=None, duration_s=0):
    """Show avatar on UC3 with text. Resets to idle after duration_s (0 = manual reset via avatar_dismiss)."""
    if character:
        state.set("sensor.ai_avatar_character", character,
                  icon="mdi:account-star", friendly_name="AI Avatar Character")
    if mood:
        state.set("sensor.ai_avatar_mood", mood,
                  icon="mdi:emoticon", friendly_name="AI Avatar Mood")
    state.set("sensor.ai_avatar_text", text,
              icon="mdi:message-text", friendly_name="AI Avatar Text")
    state.set("sensor.ai_avatar_active", mode,
              icon="mdi:drama-masks", friendly_name="AI Avatar Active")

    if duration_s > 0:
        task.sleep(duration_s)
        # Only reset if still in same mode (another event may have taken over)
        if state.get("sensor.ai_avatar_active") == mode:
            state.set("sensor.ai_avatar_active", "idle",
                      icon="mdi:drama-masks", friendly_name="AI Avatar Active")
            state.set("sensor.ai_avatar_text", "",
                      icon="mdi:message-text", friendly_name="AI Avatar Text")

@service
def avatar_dismiss():
    """Hide avatar on UC3."""
    state.set("sensor.ai_avatar_active", "idle",
              icon="mdi:drama-masks", friendly_name="AI Avatar Active")
    state.set("sensor.ai_avatar_text", "",
              icon="mdi:message-text", friendly_name="AI Avatar Text")
```

### 2.7 Startup Seeding (state_bridge.py)

```python
("sensor.ai_avatar_character", "rick", "mdi:account-star", "AI Avatar Character"),
("sensor.ai_avatar_mood", "neutral", "mdi:emoticon", "AI Avatar Mood"),
("sensor.ai_avatar_energy", "50", "mdi:lightning-bolt", "AI Avatar Energy"),
("sensor.ai_avatar_context", "normal", "mdi:home", "AI Avatar Context"),
("sensor.ai_avatar_active", "idle", "mdi:drama-masks", "AI Avatar Active"),
("sensor.ai_avatar_text", "", "mdi:message-text", "AI Avatar Text"),
```

### 2.8 Blueprint Integration Points

Each blueprint that should trigger the UC3 avatar gets a new optional boolean input:

```yaml
input:
  show_on_uc3:
    name: "Show on UC Remote"
    description: "Display avatar on UC Remote 3 when this event fires"
    default: false
    selector:
      boolean:
```

When `show_on_uc3` is true, the blueprint calls `avatar_notify` after `tts_queue_speak`:

```yaml
- alias: "Trigger UC3 avatar"
  if: "{{ show_on_uc3 }}"
  then:
    - action: pyscript.avatar_notify
      data:
        mode: "banter"           # or therapy, theatrical, notification
        text: "{{ response }}"
        character: "{{ agent }}"
        mood: "{{ resolved_mood }}"
        duration_s: 0            # 0 = manual dismiss; set >0 for auto-dismiss
```

And calls `avatar_dismiss` when the interaction ends:

```yaml
- alias: "Dismiss UC3 avatar"
  if: "{{ show_on_uc3 }}"
  then:
    - action: pyscript.avatar_dismiss
```

**Blueprints to modify:**

| Blueprint | Mode | Notes |
|---|---|---|
| `reactive_banter.yaml` | `banter` | Call after Step 7 (TTS). Auto-dismiss after TTS completes. |
| `notification_follow_me.yaml` | `notification` | Call alongside TTS delivery. Auto-dismiss with duration_s. |
| `therapy_session.yaml` | `therapy` | Call per turn. Dismiss at session end. |
| `theatrical_mode.yaml` | `theatrical` | Call per turn with character swap. Dismiss at debate end. |

---

## Phase 3: UC3 <-> HA Entity Wiring

### 3.1 Entity Flow

```
HA sensor.ai_avatar_mood (state="amused")
  |  (existing HA integration — entity_id passthrough)
UC3 Core (entity_id: "hass.main.sensor.ai_avatar_mood", value: "amused")
  |  (Core API WebSocket — entity_changed event)
EntityController -> Sensor QObject (value: "amused") -> emit valueChanged()
  |  (QML Connections binding)
AvatarDisplay -> MoodEngine -> portrait swap + ambient color change
```

### 3.2 QML Entity Access Pattern

```qml
import Entity.Controller 1.0

Item {
    id: entityWatcher

    // Entity IDs — built from configurable HA integration prefix
    // Format: {prefix}.sensor.ai_avatar_{name}
    // Default prefix "hass.main" -> "hass.main.sensor.ai_avatar_mood" etc.
    readonly property string _p: Config.avatarHaPrefix + ".sensor.ai_avatar_"
    readonly property string characterEntityId: _p + "character"
    readonly property string moodEntityId: _p + "mood"
    readonly property string energyEntityId: _p + "energy"
    readonly property string contextEntityId: _p + "context"
    readonly property string activeEntityId: _p + "active"
    readonly property string textEntityId: _p + "text"

    property var characterEntity: null
    property var moodEntity: null
    property var energyEntity: null
    property var contextEntity: null
    property var activeEntity: null
    property var textEntity: null

    Component.onCompleted: {
        if (Config.avatarMoodSource !== "local") {
            EntityController.load(characterEntityId)
            EntityController.load(moodEntityId)
            EntityController.load(energyEntityId)
            EntityController.load(contextEntityId)
            EntityController.load(activeEntityId)
            EntityController.load(textEntityId)
        }
    }

    Connections {
        target: EntityController
        ignoreUnknownSignals: true

        function onEntityLoaded(success, entityId) {
            if (!success) return;
            switch (entityId) {
                case entityWatcher.characterEntityId:
                    entityWatcher.characterEntity = EntityController.get(entityId); break;
                case entityWatcher.moodEntityId:
                    entityWatcher.moodEntity = EntityController.get(entityId); break;
                case entityWatcher.energyEntityId:
                    entityWatcher.energyEntity = EntityController.get(entityId); break;
                case entityWatcher.contextEntityId:
                    entityWatcher.contextEntity = EntityController.get(entityId); break;
                case entityWatcher.activeEntityId:
                    entityWatcher.activeEntity = EntityController.get(entityId); break;
                case entityWatcher.textEntityId:
                    entityWatcher.textEntity = EntityController.get(entityId); break;
            }
        }
    }

    // Watch for push trigger changes
    Connections {
        target: entityWatcher.activeEntity
        ignoreUnknownSignals: true

        function onValueChanged() {
            var mode = entityWatcher.activeEntity ? entityWatcher.activeEntity.value : "idle";
            if (mode !== "idle" && mode !== "") {
                avatarOverlay.startPushMode(mode);
            } else {
                avatarOverlay.closePushMode();
            }
        }
    }

    // Watch for text updates during push mode
    Connections {
        target: entityWatcher.textEntity
        ignoreUnknownSignals: true

        function onValueChanged() {
            if (avatarOverlay.mode !== "voice") {
                avatarOverlay.updateText(entityWatcher.textEntity.value);
            }
        }
    }
}
```

### 3.3 User Setup Steps

1. HA: `avatar_mood.py` auto-creates sensors on startup via state_bridge.py seeds (no user action)
2. UC3: Web Configurator -> HA Integration -> Add the 6 `sensor.ai_avatar_*` entities
3. UC3: Settings -> Avatar -> Enable avatar, set mood source to "Hybrid" or "HA"
4. UC3: Settings -> Avatar -> Enable voice overlay and/or push overlay as desired
5. HA: Enable `show_on_uc3` on desired blueprint instances (banter, notifications, etc.)

---

## Breaking Changes

**None.** All changes are additive.

- VoiceOverlay falls back to stock EQ/circles when avatar is disabled
- ChargingScreen adds a new theme option without affecting existing themes
- All new Config properties default to off/safe values
- Blueprints gain an optional input that defaults to false

**Binary size impact:** ~150-200KB (AvatarGridItem C++ ~550 lines compiled + QML files + Braille art text files + BrailleFont.otf ~30-50KB subset). Negligible relative to existing binary (MatrixRainItem alone is ~1800 lines across 4 files).

---

## Implementation Order

**Phase A — Renderer foundation (C++, testable on desktop):**
1. **Braille font subset** — `pyftsubset` DejaVu Sans Mono for U+2800-U+28FF. Add to `deploy/config/`. Add `loadBrailleFont()` + `"braille"` charset to GlyphAtlas.
2. **AvatarGridItem scaffold** — New QQuickItem: grid model, QSG renderer (copy atlas upload + quad vertex patterns from MatrixRainItem). Register in `main.cpp`. Static display only — hardcoded test grid.
3. **Portrait loader** — Art manifest JSON parser. Load base portrait from qrc text file into grid cells. Verify portrait renders centered on screen.
4. **Region variant compositing** — Load eye/mouth patches. Overlay onto base portrait at mapped coordinates. Verify swap works.
5. **Per-cell animation** — Breathing, glow, particles, mood flash in tick(). Timer + displayOff pattern.
6. **Facial animation** — Blink timer, talk cycle. Driven by `eyeState`/`mouthState` properties.
7. **Response text** — Render text as characters in grid rows below portrait. Truncate/scroll/paginate modes.

**Phase B — QML integration (connecting renderer to UI):**
8. **Config properties** — ~20 avatar Q_PROPERTYs in config.h/.cpp (CFG macros).
9. **AvatarDisplay.qml** — Thin wrapper around AvatarGridItem. Translates QML signals to C++ properties.
10. **MoodEngine.qml** — Mood state resolver with local fallback.
11. **AvatarScreensaverOverlay.qml** — Optional avatar layer on any screensaver theme. Mood-to-screensaver bindings (Matrix gets full reactions, others get color tinting). Wire into ChargingScreen.qml Loader.
12. **AvatarOverlay.qml** — Unified popup: voice mode + push mode.
13. **VoiceOverlay integration** — Conditional delegation when avatar enabled.
14. **TouchHandler** — Charging theme interaction.
15. **Settings page** — Full configuration UI with entity status warning.

**Phase C — HA bridge (pyscript + blueprints):**
16. **avatar_mood.py** — Mood bridge + push services (`avatar_notify`, `avatar_dismiss`).
17. **state_bridge.py seeds** — 6 sensor startup seeds.
18. **Blueprint mods** — `show_on_uc3` input on banter, notification, therapy, theatrical.

**Phase D — Art + end-to-end:**
19. **Portrait art** — Rick neutral base + eyes-closed + talk-A/B patches (minimum viable).
20. **Wiring & testing** — Voice mode (mic button) + push mode (HA events) + screensaver + rain mood reactions.

---

## Verification

### Local Testing (Desktop)

1. Build: `qmake && make` -> run with Core Simulator (`UC_MODEL=DEV`)
2. **Voice mode:** Simulate mic button press -> verify avatar replaces EQ bars -> mock response -> verify text + success animation
3. **Push mode:** Mock entity value change on `ai_avatar_active` -> verify popup opens with text -> mock idle -> verify popup closes
4. **Charging theme:** Trigger with simulated battery event -> verify avatar theme
5. **Fallback:** Disable avatar in settings -> verify stock VoiceOverlay EQ/circles work unchanged
6. **Touch:** Tap/double-tap/swipe on avatar screensaver -> verify reactions
7. **Settings:** All toggles persist across restarts
8. **displayOff:** Simulate power mode change -> verify animations pause

### Integration Testing (Device)

1. Cross-compile via Docker -> deploy to UC3 (192.168.2.204)
2. **Font check:** Verify Braille rendering on device display
3. **Voice mode:** Press mic button -> speak -> verify avatar + response on UC3
4. **Push mode:** Trigger banter from HA with `show_on_uc3` enabled -> verify avatar pops up on UC3 with banter text
5. **Therapy/theatrical:** Start session from HA -> verify multi-turn text updates + character swaps
6. **Charging theme:** Dock remote -> verify avatar theme activates
7. **Disconnect:** Remove HA connection -> verify graceful fallback to local mood
8. **Timeout:** Kill HA during push mode -> verify popup auto-closes after timeout

---

## Open Decisions

None. All decisions resolved.

## Resolved Decisions

**(Font)** UC3 Braille font: RESOLVED. GlyphAtlas pre-renders Braille into GPU texture at build time. Bundled font in `deploy/config/BrailleFont.otf` (same pattern as NotoSansMonoCJKjp.otf). No runtime font dependency. No device test needed.

**(Text scrolling)** All three modes implemented and configurable via `Config.avatarTextMode`: `truncate` (elide at grid edge), `scroll` (auto-scroll at ~2 cells/sec), `paginate` (advance on timer). All rendered as character cells in the grid. AvatarGridItem C++ property `textMode`.

**(Theatrical transition)** All three styles implemented and configurable via `Config.avatarTransitionStyle`: `flash` (mood flash covers swap), `crossfade` (progressive cell morph over ~300ms), `slide` (portrait slides out, new slides in). AvatarGridItem C++ property `transitionStyle`.

**(ARM performance)** RESOLVED — with caveat. AvatarGridItem alone uses the same GPU texture atlas pattern proven at 40fps on ARM. When composited with MatrixRainItem (screensaver), that's two draw calls and two textures. Expected to be fine — the avatar grid has far fewer quads than the rain (only portrait + ambient cells emit quads, not the full 4,556 cell grid). But must verify on device that two concurrent renderers don't exceed frame budget. Degradation path if needed: reduce rain density when avatar is active.

**(Portrait art volume)** Rick only to start. 1 base portrait (neutral) + eyes-closed patch + talk-A/B mouth patches. Minimum viable set for blinks + talking animation. Mood conveyed through per-cell color/brightness. More base portraits and region variants added incrementally as art is created.

6. **Push event collision:** Not handled in QML. The HA-side `tts_queue` already serializes audio delivery, and blueprints call `avatar_notify`/`avatar_dismiss` in sync with TTS execution. The QML simply displays whatever the latest entity state says.

7. **Device wake on push:** Both options, configurable via `Config.avatarPushWakeDisplay` (default off). When on, push events trigger a Core API power command to wake the display.

8. **Speech playback during voice mode:** Configurable via `Config.avatarVoiceLocalAudio` (default off). When off, voice mode is visual-only on UC3 — audio plays on room speakers via `assist_tts_reroute`. When on, UC3 plays TTS locally (stock behavior).

9. **Charging theme + push events:** Both options, configurable via `Config.avatarPushChargingInline` (default on). When on, the screensaver absorbs push events in-place. When off, the standard AvatarOverlay popup opens on top.

10. **Integration instance ID:** Dynamic, not hardcoded. `Config.avatarHaPrefix` (default `"hass.main"`) used to construct entity IDs at runtime: `prefix + ".sensor.ai_avatar_" + name`. Supports non-standard HA integration instances.

11. **HA entity not configured guard:** Settings page shows warning banner when any of the 6 avatar entities fail to load. Guides user to Web Configurator. Avatar degrades gracefully to local fallback when entities are missing.

12. **Overlay layout/background:** Three background styles, configurable via `Config.avatarOverlayBackground`: `"black"` (solid), `"gradient"` (transparent->black, matches stock VoiceOverlay), `"mood"` (subtle mood-colored tint). Default: `"gradient"`.

---

## Research Notes: Prior Art (2026-04-03)

Surveyed 20+ open-source projects. No drop-in solution exists for an embeddable Qt scene graph character grid renderer with per-cell animation. Our `QQuickItem` + `GlyphAtlas` texture atlas is the industry-standard pattern (same as Alacritty, WezTerm, Contour). Two projects have API design patterns worth studying:

### FTXUI Canvas abstraction
**Repo:** https://github.com/ArthurSonzogni/FTXUI (C++, 9.9k stars)

FTXUI's `Canvas` class has a Braille resolution mode (2x4 sub-pixels per cell) with per-cell color via `Stylizer` functions. Its API for cell operations is clean:

- `canvas.DrawPoint(x, y, color)` — maps pixel coordinates to the Braille cell that contains that sub-pixel, sets the appropriate dot bit
- `canvas.DrawText(x, y, text, decorator)` — writes text at cell coordinates with styling
- Functional composition: `canvas | color(Color::Cyan)` applies color to a region

**What to borrow for AvatarGridItem:**
- The pixel-to-Braille-cell mapping math (trivial but well-tested in FTXUI)
- The idea of a `DrawRegion(row, col, patch)` method that stamps a pre-drawn region variant onto the grid — cleaner API than raw `m_gridChar[]` index math scattered across animation code
- Separating the "what to draw" (portrait data, region patches) from "how to draw it" (atlas UV lookup, vertex buffer) — FTXUI's Canvas is a logical buffer, not a renderer

**What NOT to borrow:** FTXUI renders to terminal escape sequences via a functional DOM diffing system. Its output pipeline is fundamentally incompatible with Qt scene graph. Only the Canvas cell abstraction is useful.

### Contour double-buffered RenderBuffer
**Repo:** https://github.com/contour-terminal/contour (C++, 2.9k stars)

Contour's `RenderBuffer` decouples simulation from rendering with double-buffering and RAII lock guards:

- Two `RenderBuffer` instances (front/back). Simulation writes to back buffer, renderer reads front buffer.
- Swap is an atomic pointer exchange — zero copy, zero lock contention during rendering.
- Each `RenderCell` holds: codepoint(s), foreground/background color, attributes (bold, italic, etc.), image fragment reference.
- `RenderBuffer::at(line, col)` returns a `RenderCell&` for mutation.

**What to borrow for AvatarGridItem (if needed later):**
- If avatar simulation grows complex enough that it can't complete within a single frame tick (unlikely at 4,556 cells, but possible if procedural effects get heavy), double-buffering the grid arrays (`m_gridChar`, `m_gridBright`, `m_gridColor`) would let simulation run on a worker thread while the render thread reads the stable front buffer.
- The `RenderCell` struct pattern — bundling char + brightness + colorVariant into a single struct instead of parallel arrays — improves cache locality for per-cell iteration.

**What NOT to borrow:** Contour is a full terminal emulator (C++23, Qt6). Its rendering pipeline is deeply coupled to VT terminal semantics (cursor, scrollback, selection, hyperlinks). Only the double-buffer pattern and cell struct design are transferable.

**Current assessment:** Our screensaver uses Qt's sync-point guarantee (main thread blocked during `updatePaintNode`), which is correct and simpler. Double-buffering is a future optimization if needed, not a launch requirement.

---

## Architecture Review Findings (2026-04-03 — 2026-04-04)

### UC3 Hardware (researched)
- CPU: Quad-core ARM64, 1.8 GHz (exact SoC undisclosed)
- GPU: Undisclosed (embedded in SoC, runs Qt 5.15 QML/QSG rendering)
- RAM: 4 GB
- Storage: 32 GB eMMC
- Display: 3.2" IPS LCD, 480 x 850px (no burn-in risk — IPS, not OLED)
- Battery: ~8.88 Wh Li-ion, 5W Qi wireless charging

### Entity ID Format (verified via UC3 REST API + integration source)
- HA integration driver passes entity_ids through unchanged (source: `integration-home-assistant/src/client/get_states.rs`)
- UC3 Core prepends `{integration_id}.` → `hass.main.sensor.ai_avatar_mood`
- Confirmed by querying live UC3 API: `curl -u "web-configurator:6984" http://192.168.2.204/api/entities`
- Only HA entity currently configured: `hass.main.assist` (voice_assistant)

### Codebase Reuse Assessment (verified by reading actual source)

**GlyphAtlas (~90% reusable):**
- Add `"braille"` charset to `charsetString()` (1 line + string constant)
- Add `loadBrailleFont()` alongside `loadCJKFont()` (same pattern, ~15 lines)
- Atlas build, UV lookup, brightness map, font loading — all work as-is
- Atlas rebuild is ~50-150ms on ARM — fine for mood transitions, not per-frame color fading

**MatrixRainItem (pattern reuse, ~60%):**
- Copy verbatim: `MatrixRainNode` destructor, atlas texture upload (`matrixrain.cpp:115-131`), quad vertex construction (`266-273`), timer/tick/displayOff, `componentComplete()` deferred init
- NOT reusable: stream rendering loop (iterates streams, not cells), `RainSimulation`, `GlitchEngine` (stream-scoped)
- AvatarGridItem iterates all non-empty cells directly — simpler loop than stream trail walking

**Why "extend MatrixRainItem" was rejected:**
- Render loop iterates STREAMS not cells — portrait cells with no stream never render
- `m_charGrid` ownership conflict — streams overwrite portrait chars as they pass through
- `ChaosScramble` destroys portrait (randomizes charGrid without portrait awareness)
- Glitch effects are per-`StreamState` — portrait cells have no stream
- Would require modifying RainSimulation, GlitchEngine, AND render loop — not extending, rewriting

**Two-layer compositing chosen instead:**
- MatrixRainItem at z:0 (unchanged), AvatarGridItem at z:1 (transparent background)
- Rain reacts to avatar mood via QML property bindings (same interface settings page uses)
- No shared grid, no simulation entanglement, no code changes to screensaver
- Two GPU draw calls — expected fine with 4GB RAM, quad-core 1.8GHz, but must verify on device

### Feasibility Assessment (per surface)

**Voice overlay — HIGH confidence:**
- VoiceOverlay.qml delegates to AvatarOverlay when avatar enabled
- Voice singleton Connections forward events to avatar (STT, text, speech, finished, error)
- Stock EQ bars/circles preserved as fallback when avatar disabled
- Standard QML component swapping inside existing Popup

**Push overlay — HIGH confidence:**
- Copies exact pattern from SensorWidget.qml (`EntityController.load()` → `onEntityLoaded` → `onValueChanged`)
- Popup opens/closes from entity state change — same mechanism ChargingScreen uses for battery events
- Manual entity setup step required (6 entities via Web Configurator)

**Screensaver overlay — HIGH confidence with one caveat:**
- QML z-stacking, alpha transparency verified in code (`QSGTextureMaterial` + `TextureHasAlphaChannel` + `ARGB32_Premultiplied` atlas)
- AvatarGridItem skips empty cells (no quad emitted) — theme shows through naturally
- Caveat: dual-renderer GPU frame rate unverified on device. Degradation path: reduce rain density when avatar active.

### Edge Cases Requiring Careful Implementation

1. **Atlas build timing** — first avatar open could flash blank for 1-2 frames. Mitigation: copy MatrixRainItem's 2-second safety retry.
2. **Mode collision** — avatar in screensaver idle mode, then mic pressed → mode transition without close/reopen. Needs testing that Qt ButtonNavigation re-take works.
3. **Push during voice** — QML guard: if `mode === "voice"`, ignore push. HA side handles audio via tts_queue.
4. **Entity load delay** — first push after boot may hit before entities load (~100-500ms). Mitigation: load entities eagerly at startup when avatar enabled.
5. **Voice session timing** — `assistantAudioSpeechResponseEnd` controls popup close. If avatar and VoiceOverlay disagree on timing, UX breaks. Testable on desktop but real TTS only fires with live HA.
6. **VoiceOverlay delegation pattern** — VoiceOverlay becomes headless session manager when avatar active. Awkward but functional. Cleaner refactor (extract VoiceSessionManager) possible later.

### One Avatar, Multiple Triggers (architectural clarification)

The avatar is NOT three separate things. It's one AvatarOverlay component at `main.qml` level with three trigger sources:
- Mic button → voice mode
- HA entity change → push mode
- Screensaver active + config enabled → idle mode (ambient, no text)

Same AvatarGridItem instance, same MoodEngine. Different reasons for appearing, different things driving it while visible. The screensaver is a separate system that happens to be visible behind the avatar when both are active.

---

## Test Files (in project root, not for production)

| File | Purpose |
|---|---|
| `test_braille.qml` | Current v6 ambient life prototype — run with `qmlscene` |
| `test_braille_mapper.qml` | Interactive drag-to-select face region mapper |
| `ASCII_Rick_base.md` | Rick Braille portrait (30x45 grid) used for all prototyping |
| `ASCII Source.md` | Collection of 6 Rick Braille art examples (reference quality samples) |
