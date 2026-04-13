# STYLE_GUIDE.md — UC Remote 3 Coding, Architecture & Operational Standards

> **Scope:** This guide governs all AI-assisted development on the UC Remote 3 custom firmware project. It covers coding conventions, architectural patterns, operational workflow, anti-patterns, quality gates, and session discipline. Adapted from the Project Fronkensteen HA Master Style Guide ("Rules of Acquisition") for the Qt 5.15 / QML / C++17 domain.
>
> **Reference docs:** [Qt 5.15 QML Coding Conventions](https://doc.qt.io/archives/qt-5.15/qml-codingconventions.html) · [Qt 5.15 Best Practices](https://doc.qt.io/archives/qt-5.15/qtquick-bestpractices.html) · [Qt Wiki Coding Conventions](https://wiki.qt.io/Coding_Conventions) · [Qt 5.15 Scene Graph](https://doc.qt.io/archives/qt-5.15/qtquick-visualcanvas-scenegraph.html) · [Qt 5.15 Scene Graph Renderer](https://doc.qt.io/archives/qt-5.15/qtquick-visualcanvas-scenegraph-renderer.html) · [Scene Graph Custom Geometry Example](https://doc.qt.io/qt-6/qtquick-scenegraph-customgeometry-example.html)
>
> **Verification status (2026-04-04):**
>
> | Layer | Source | Confidence |
> |---|---|---|
> | Qt 5.15 QML conventions | Official Qt docs (verified) | ✅ High |
> | Qt scene graph / QSG threading | Official Qt docs (verified) | ✅ High |
> | Qt Wiki C++ conventions | Qt Wiki (verified) | ✅ High |
> | UC `CONTRIBUTING.md` requirements | Upstream repo (verified) | ✅ High |
> | Embedded C++ / performance | Industry standards (verified) | ✅ High |
> | Vibe coding operational framework | Community best practices 2026 (verified) | ✅ High |
> | EntityController / Voice / Core API patterns | Read from UC source code — no official UC modding docs exist | ⚠️ Source-derived |
> | Entity ID format (`hass.main.{id}`) | Read from integration source + live API test | ⚠️ Source-derived |
> | Config / ScreensaverConfig bridge pattern | Our own custom code (established pattern) | ⚠️ Project-specific |
> | GPU perf numbers (atlas rebuild, dual renderer) | Empirical testing on device (AVATAR_PLAN) | ⚠️ Empirical |
> | UC3 SoC constraints | UC marketing materials + empirical | ⚠️ Partial |
>
> **What this means for Claude Code:** Items marked ✅ can be trusted as-is. Items marked ⚠️ were derived from reading the actual UC source code or from empirical device testing — they are the best available truth, but there are no official UC docs to cross-reference. If the upstream codebase changes significantly after a `git fetch upstream`, re-verify ⚠️ items by reading the updated source before relying on them. There is no UC3 firmware modding community or published modding guide — this project is pioneering that space.

---

## §1 CORE PHILOSOPHY

### §1.1 Modular over monolithic
- Prefer small, composable pieces over large all-in-one classes.
- If a C++ class is growing beyond ~500 lines of implementation, consider extracting subsystems (e.g., `RainSimulation` was extracted from `MatrixRainItem`).
- If a QML file exceeds ~300 lines, decompose into child components.
- When building something new, **always ask the user** whether the complexity warrants multiple components, a single class with extracted helpers, or a monolithic implementation. Never decide this silently.

### §1.2 Separation of concerns
- **C++ renderers** (`src/ui/`) = GPU pipeline, simulation logic, per-frame computation.
- **Config bridge singletons** (`src/ui/`) = value transforms, signal forwarding between Config and renderers.
- **QML components** (`src/qml/components/`) = UI composition, property bindings, visual layout.
- **QML state resolvers** (e.g., `MoodEngine.qml`) = state machines, entity-to-visual mapping, fallback logic.
- **Settings pages** (`src/qml/settings/`) = user-facing configuration UI.
- Never put simulation logic in QML. Never put UI layout in C++. The boundary is the Q_PROPERTY interface.

### §1.3 Never remove features without asking
- If something looks unnecessary or redundant, **ask the user** before removing it. Explain why you think it might not be needed and let them decide.
- If refactoring, ensure every behavior from the original is preserved unless explicitly told otherwise.
- **Applies to:** Functional code, user-written comments, disabled-but-preserved features, `// NOTE:` / `// HACK:` / `// FIXME:` / `// TODO:` comments.
- **Does NOT apply to:** AI-generated boilerplate comments, trailing whitespace, empty `description` strings. Clean these up silently.

### §1.4 Follow upstream patterns and official docs
- Always follow the existing UC codebase conventions for upstream code.
- **For Qt/QML:** Consult Qt 5.15 documentation. Don't use Qt 6 APIs.
- **For integrations:** Consult UC's Core API docs, integration driver source, and entity model headers before assuming behavior. Verify from actual source code — the AVATAR_PLAN was written after reading `integration-home-assistant/src/client/get_states.rs`. Follow that standard.
- Prefer established UC patterns over novel inventions:
  - `ButtonNavigation` for input handling (not raw `Keys.onPressed`).
  - `Popup` for overlays (not custom `Item` with manual z-ordering).
  - `Loader` for conditional components (not `visible: false` with full instantiation).
  - `EntityController.load()` → `onEntityLoaded` for entity access (not raw WebSocket).
- **Prefer declarative property bindings over imperative JavaScript.** QML is a declarative language — use bindings for reactive UI updates. Use imperative JS only for complex logic that can't be expressed declaratively (multi-step calculations, network calls, state machine transitions). If a binding expression exceeds ~3 lines, extract it into a JS function — but the function should still *return* a value for binding, not imperatively set properties.

### §1.5 Uncertainty signals — stop and ask, don't guess (MANDATORY)
- If you are **unsure about a Qt API, UC Core API, entity schema, or integration behavior**, STOP and tell the user. Do not guess.
- Specifically:
  - Don't invent Q_PROPERTY types you haven't verified in headers.
  - Don't assume an entity carries custom attributes — check `sensor.h`.
  - Don't fabricate QML import paths or singleton names.
  - Don't assume QSG rendering behavior — verify in Qt docs.
- Say: *"I'm not sure whether `EntityController.get()` returns null or throws when the entity isn't loaded — let me check `entityController.h` first."*
- **Never silently ship uncertain code.** Leave a `// TODO: verify this — not confirmed` comment AND flag it to the user.

#### §1.5.1 Requirement ambiguity
When the user's request is vague:

| User says... | Don't assume — ask instead |
|---|---|
| "Make it better" | "Better how? Performance, visual quality, code clarity, or fewer edge cases?" |
| "Add animations" | "Which animations? Breathing, particles, blinks, mood transitions, or all of them?" |
| "Fix the rendering" | "What's wrong — artifacts, wrong colors, performance, or crashes?" |
| "Clean this up" | "Restructure the logic, fix formatting, remove dead code, or all three?" |

### §1.6 Complexity budget — quantified limits

| Metric | Limit | What to do when exceeded |
|---|---|---|
| C++ class implementation | **~500 lines** | Extract subsystems (simulation, animation, rendering) |
| QML file | **~300 lines** | Decompose into child components |
| Nesting depth (if/switch/for) | **4 levels** | Extract into a helper function |
| Q_PROPERTYs on one class | **~25** | Consider a config bridge singleton |
| QML Connections blocks in one file | **5** | Extract entity watchers into a dedicated component |
| Settings toggles on one page | **~12** | Decompose into sub-pages (ChargingScreen pattern) |
| `updatePaintNode()` body | **~80 lines** | Extract vertex construction / atlas logic into helpers |

If a design naturally exceeds these limits, **stop and discuss** before generating. The answer is usually decomposition, not a bigger monolith.

### §1.7 Reasoning-first directive (MANDATORY)
Before generating **any** code (C++, QML, .pro changes, qrc registration), you MUST:

1. **State your understanding** of what's being asked. One or two sentences.
2. **Explain your approach** — which patterns you'll follow, which existing code you'll reference, which files you'll modify.
3. **Flag ambiguities or risks** — anything unclear, any Qt behavior you're unsure about (§1.5), any complexity budget concerns (§1.6), any upstream compatibility risks.
4. **Only then** generate the code.

**Claude Code workflow:** Use Plan Mode (Shift+Tab twice) for the reasoning step — it prevents accidental file writes while you're still designing. Only exit Plan Mode and write code after the user confirms the approach.

**Exceptions:** Trivial one-line fixes the user explicitly asked for. Or when the user says "skip the explanation."

**Anti-pattern:** Writing 400 lines of C++ first, then explaining it after. Flip the order.

### §1.8 Research-first mandate (MANDATORY)
Before proposing or generating ANY solution:

1. **Read the design doc** — AVATAR_PLAN.md or SCREENSAVER-IMPLEMENTATION.md, whichever applies.
2. **Read the actual source** — Don't assume API behavior. Check headers (`sensor.h`, `entityController.h`, `config.h`, `matrixrain.h`). Check QML files for existing patterns.
3. **Check for existing implementations** — Search the codebase for similar patterns before inventing new ones. The screensaver system is the reference implementation for the avatar system.
4. **No hacky workarounds** — If the only path forward is a workaround, say so explicitly with risks.
5. **Flag breaking changes** — Any change that alters existing behavior uses a `⚠️ BREAKING:` prefix.

### §1.9 Violation report severity taxonomy

All reviews, audits, and violation reports use exactly three severity tiers:

| Tier | Label | Meaning | Action required |
|---|---|---|---|
| ❌ | **ERROR** | Blocks effective use. Broken behavior, crash, memory leak, or spec violation. | Must fix before next build. **Stop and ask** — confirm fix approach. |
| ⚠️ | **WARNING** | Degrades quality or consistency. Works but wrong pattern, missing guard, tech debt. | Fix within current session. Fix silently and note. |
| ℹ️ | **INFO** | Nice-to-have. Style preference, future-proofing, documentation gap. | Fix if convenient. Log and move on otherwise. |

**Report output:** Violation reports are timestamped markdown files in the project root: `violations_report_YYYY-MM-DD_<scope>.md`. Commit alongside fixes.

### §1.10 Directive precedence — when MANDATORYs conflict

| Priority | Directive | Rationale |
|---|---|---|
| 1 | **Safety / memory** — No leaks, no crashes, no UB | Embedded device with no watchdog for the UI process |
| 2 | **Git checkpoint** (§3.3 pre-flight) | Uncommitted edits are at risk |
| 3 | **displayOff gating** — Zero CPU/GPU when screen off | Battery life on a portable device is sacred |
| 4 | **Upstream compatibility** — Don't break stock behavior | Non-avatar users must not be affected |
| 5 | **Reasoning-first** (§1.7) | Prevents hallucinated code |
| 6 | **Chunked generation** (§4.5) | Quality control for large files |
| 7 | **Anti-pattern scan** (§5) | Last gate before delivery |

**User override:** The user can say "skip X" for workflow preferences — but NOT for safety, git checkpoints, or displayOff gating. Push back on those.

---

## §2 OPERATIONAL MODES

At the start of every task, identify the mode:

### §2.1 BUILD mode
**Trigger:** User asks to create, implement, modify, or extend functionality.

**Workflow:**
1. Identify mode → load relevant design doc + this style guide
2. Run git pre-flight (§3.3)
3. Reasoning-first (§1.7) — explain approach
4. Generate code in chunks (§4.5) if >150 lines
5. Anti-pattern scan (§5) before presenting
6. Post-task state checkpoint (§4.3)

### §2.2 TROUBLESHOOT mode
**Trigger:** User reports a bug, crash, rendering artifact, or unexpected behavior.

**Workflow:**
1. Identify the symptom — ask for specifics if vague
2. Read relevant source files to understand current implementation
3. Form a hypothesis and explain it
4. If the fix requires editing files → **auto-escalate to BUILD mode** (run git pre-flight first)
5. If the fix is configuration or conceptual → explain without editing

### §2.3 AUDIT mode
**Trigger:** User asks to review, audit, check, or validate existing code.

**Workflow:**
1. Read the files under audit
2. Cross-reference against this style guide, design docs, and Qt 5.15 docs
3. Produce a violations report (§1.9 severity format)
4. If fixes are requested → **escalate to BUILD mode**

### §2.4 Auto-escalation
If a TROUBLESHOOT or AUDIT session requires editing files, escalate to BUILD mode BEFORE the first edit: run git pre-flight (§3.3), announce the mode switch, and follow BUILD workflow from that point.

---

## §3 GIT DISCIPLINE

### §3.1 Remotes & branches

| Remote | URL | Purpose |
|---|---|---|
| `origin` | `mmadalone/Madalones-Defolded-Circle-3` | Our private repo — push here |
| `upstream` | `unfoldedcircle/remote-ui` | UC's official repo — pull updates |

All custom work on `main`. Feature branches optional for large mods.

### §3.2 Commit message convention
```
[<type>] <scope>: <description>

Types: mod, renderer, qml, config, settings, build, test, docs, fix, audit
Scope: feature name or filename (short)

Examples:
[renderer] avatargrid: implement per-cell breathing animation
[settings] avatar: add voice overlay toggle and push event config
[fix] matrixrain: prevent atlas rebuild during displayOff
[audit] screensaver: 8 warnings fixed across 4 files
[docs] style guide v2: add operational framework
[mod] screensaver: initial implementation of Matrix theme system
```

### §3.3 Pre-flight checklist (MANDATORY — before first edit in any BUILD session)

**Stop. Before you edit any project file:**

1. ✅ Check `git status` — resolve any uncommitted changes from a previous session.
2. ✅ Commit or stash anything dirty with a descriptive message.
3. ✅ Only NOW may you edit files.

If you realize mid-edit that you forgot: don't panic — `git diff` shows what changed. But skipping this deliberately is a violation (AP-UC-03).

### §3.4 Atomic multi-file edits
When a task requires changes to multiple files (C++ + QML + config + .pro + .qrc):

1. **Single commit covers all files.**
2. **Edit in dependency order.** Headers before .cpp. Config before QML. .pro and .qrc before anything depending on compilation.
3. **If any edit fails mid-batch:** Stop. Report. Decide with user whether to revert or fix.

### §3.5 Upstream merge strategy
```bash
git fetch upstream && git merge upstream/main
```
Custom additions at END of lists in `config.h`, `remote-ui.pro`, `main.qrc` to minimize conflicts. Never reformat upstream files. Never rename upstream symbols. Never insert custom code in the middle of upstream functions.

---

## §4 BUILD WORKFLOW

### §4.1 Session start
1. Identify operational mode (§2)
2. Read relevant design doc
3. Load relevant style guide sections (don't load everything for a one-line fix)
4. Run git pre-flight (§3.3)

### §4.2 Build logs
For non-trivial builds (new component, multi-file change, >100 lines total), create a build log:

**File:** `_build_logs/YYYY-MM-DD_<task>.md`

```markdown
# Build Log: <task description>
**Date:** YYYY-MM-DD | **Mode:** BUILD | **Design doc:** <ref>

## Plan
<reasoning-first output from §1.7>

## Edit Log
| # | File | Change | Status |
|---|------|--------|--------|
| 1 | src/ui/avatargrid.h | Class declaration | ✅ |
| 2 | src/ui/avatargrid.cpp | Grid model + tick | 🔧 |

## Current State
<what exists on disk, committed vs pending>

## Decisions Made
<trade-offs, approaches chosen>

## Outstanding
<deferred items, open questions>
```

Update the edit log between consecutive edits — not batched at the end. Crash recovery depends on this.

### §4.3 Post-task checkpoint
After completing a deliverable: summarize decisions, current state, and outstanding items. Commit the build log alongside code.

### §4.4 Crash recovery
1. `git status` + `git diff` to see uncommitted changes.
2. Check `_build_logs/` for in-progress logs.
3. Decide with user: commit `[wip]`, revert, or continue.

### §4.5 Chunked generation (>150 lines)
1. **Chunk 1:** Declaration / scaffold / structure → write to disk
2. **Chunk 2:** Core logic → write to disk
3. **Chunk 3:** Animation / secondary features → write to disk
4. **Chunk 4:** Polish, edge cases, displayOff → write to disk

Write each chunk before proceeding. Don't stack in conversation.

### §4.6 Convergence
If ~15 exchanges pass without shipping: pause, summarize, ask whether to continue, decompose, or ship what exists.

---

## §5 ANTI-PATTERNS (NEVER DO THESE)

> **AI self-check:** Before presenting generated code, scan this table top to bottom. If output matches any trigger, fix it first.

### Core / General

| ID | Sev | Trigger pattern | Fix ref |
|---|---|---|---|
| AP-UC-01 | ❌ | C++ class modifies upstream renderer instead of composing via z-stack | §1.2 |
| AP-UC-02 | ❌ | QML Connections to entity without `ignoreUnknownSignals: true` | §8.3 |
| AP-UC-03 | ❌ | File edit without git pre-flight (§3.3) | §3.3 |
| AP-UC-04 | ⚠️ | Removed behavior without user confirmation | §1.3 |
| AP-UC-05 | ⚠️ | Code generated with no preceding reasoning | §1.7 |
| AP-UC-06 | ⚠️ | Hardcoded entity ID instead of configurable prefix | §8.4 |
| AP-UC-07 | ❌ | `updatePaintNode()` creates QSG objects outside render sync point, or modifies simulation state | §7.4 |
| AP-UC-08 | ❌ | Timer continues when `displayOff` is true | §7.5 |
| AP-UC-09 | ⚠️ | Qt 6 API used (project is Qt 5.15) | §1.4 |
| AP-UC-10 | ⚠️ | Missing `id:` on QML Loader | §8.2 |
| AP-UC-11 | ⚠️ | Config property missing default value | §6.6 |
| AP-UC-12 | ⚠️ | New Config Q_PROPERTY placed mid-list (merge conflict risk) | §3.5 |
| AP-UC-13 | ℹ️ | `visible: false` where `Loader` with `active:` would avoid instantiation | §1.4 |
| AP-UC-14 | ⚠️ | Single-pass generation over ~150 lines | §4.5 |
| AP-UC-15 | ⚠️ | Missing copyright header on new file | §6.1 |
| AP-UC-16 | ⚠️ | QML property declared as `var` when concrete type is known (`string`, `int`, `bool`, `color`, `real`) | §8.1 |
| AP-UC-17 | ⚠️ | QObject subclass missing `Q_OBJECT` macro | §6.5 |
| AP-UC-18 | ⚠️ | Reimplemented virtual method missing `override` keyword or with redundant `virtual` in header | §6.5 |
| AP-UC-19 | ⚠️ | Imperative JS setting QML properties where a declarative binding would work | §1.4 |

### Renderer / GPU

| ID | Sev | Trigger pattern | Fix ref |
|---|---|---|---|
| AP-UC-20 | ❌ | Atlas rebuilt per frame | §7.3 |
| AP-UC-21 | ❌ | `QSGTexture` or `QSGNode` created outside `updatePaintNode()` (wrong thread) | §7.4 |
| AP-UC-22 | ⚠️ | Vertex buffer fully reconstructed when only UVs changed | §7.4 |
| AP-UC-23 | ❌ | `QSGNode` leaked — missing destructor cleanup, or QSGNode reference retained in QQuickItem class | §7.4 |
| AP-UC-24 | ⚠️ | Per-frame trig that could be precomputed into lookup table | §7.6 |
| AP-UC-25 | ⚠️ | `GlyphAtlas` charset registered but font not loaded in `main.cpp` | §7.3 |
| AP-UC-26 | ❌ | QQuickItem constructor missing `setFlag(ItemHasContents, true)` — `updatePaintNode()` will never be called | §7.2 |
| AP-UC-27 | ❌ | Geometry or material changed in `updatePaintNode()` without calling `node->markDirty()` — changes won't render | §7.4 |

### Entity / HA Bridge

| ID | Sev | Trigger pattern | Fix ref |
|---|---|---|---|
| AP-UC-30 | ❌ | Entity value used as int without `parseInt()` | §8.4 |
| AP-UC-31 | ❌ | Entity accessed before `onEntityLoaded` success | §8.3 |
| AP-UC-32 | ⚠️ | Entity load not on `Component.onCompleted` | §8.3 |
| AP-UC-33 | ⚠️ | Push event with no timeout fallback | AVATAR_PLAN §1.6 |
| AP-UC-34 | ⚠️ | `state.set()` sensor without `state_bridge.py` seed | §8.5 |

### Upstream Compatibility

| ID | Sev | Trigger pattern | Fix ref |
|---|---|---|---|
| AP-UC-40 | ❌ | Upstream file reformatted with clang-format | §3.5 |
| AP-UC-41 | ⚠️ | Upstream variable/function renamed | §3.5 |
| AP-UC-42 | ⚠️ | Custom code inserted mid-upstream-function | §3.5 |
| AP-UC-43 | ℹ️ | Custom HEADERS/SOURCES not at END of lists in .pro | §3.5 |

**AP numbering:** IDs are stable. Gaps intentional. New APs get next number; retired IDs never reused.

---

## §6 C++ CONVENTIONS

### §6.1 Copyright headers
```cpp
// Custom file:
// Copyright (c) {year} madalone. Brief description.
// SPDX-License-Identifier: GPL-3.0-or-later

// Modified upstream — add below UC header:
// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) {year} madalone. Brief description of modifications.
// SPDX-License-Identifier: GPL-3.0-or-later
```

### §6.2 Formatting & linting
- **clang-format** (`.clang-format`: Google base, 4-space indent, 120 col)
- **cpplint** via `cpplint.sh`
- **C++17** — `CONFIG += c++17`
- Warnings: `-Wold-style-cast -Wfloat-equal -Woverloaded-virtual -Wshadow`

### §6.3 Include ordering
1. Own header (`#include "ui/avatargrid.h"`)
2. Qt headers (`#include <QObject>`, `#include <QQuickItem>`)
3. Project headers (`#include "../config/config.h"`)

### §6.4 Namespace
`uc` namespace (matches upstream).

### §6.5 Class conventions (Qt-specific)

**Q_OBJECT macro (MANDATORY):** Every QObject subclass MUST have the `Q_OBJECT` macro, even if it has no signals or slots. Without it, `qobject_cast` fails and meta-object features break. (Source: [Qt Wiki Coding Conventions](https://wiki.qt.io/Coding_Conventions))

**Virtual method reimplementation:** When reimplementing a virtual method:
- Do NOT put `virtual` in the header for the reimplementation.
- DO annotate with `override` after the declaration, before the `;`.
- Example: `QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;`

(Source: [Qt Wiki Coding Conventions](https://wiki.qt.io/Coding_Conventions))

**Q_PROPERTY ordering:** Group properties by domain: lifecycle → state → visual config → computed. New custom properties always at the END of the property list (minimizes merge conflicts with upstream).

**Signal naming:** `{propertyName}Changed` for property notifications. No other pattern.

### §6.6 Config macros — `SCRN_*` (canonical for custom singletons)

**Canonical pattern:** `SCRN_BOOL` / `SCRN_INT` / `SCRN_STRING` macros defined in `src/ui/screensaverconfig_macros.h`. Each invocation generates a complete property stanza — `Q_PROPERTY` declaration, inline getter (Qt5/QML-friendly `name()` style), setter, signal declaration — in one line.

```cpp
// src/ui/screensaverconfig.h — 1 line per property
SCRN_BOOL(idleEnabled,   "charging/idleEnabled",       false)
SCRN_INT (idleTimeout,   "charging/idleTimeout",       20)
SCRN_STRING(direction,   "charging/matrixDirection",   "down")
```

Each expansion is ~8 lines of equivalent hand-written Qt boilerplate. 108 screensaver properties currently use this pattern.

**When to use `SCRN_*`:** custom mod-specific config singletons you own outright (e.g. `ScreensaverConfig`). Use the macro for anything with more than ~5 properties — below that threshold, hand-writing is clearer.

**Upstream pattern — hand-written `Q_PROPERTY`:** upstream UC singletons (`Config`, `Power`, `Battery`, `Wifi`, `Haptic`) do NOT use macros — each property is hand-written one at a time. Follow this convention for any upstream-modified file to keep merge conflict surface small. Don't introduce macros into files in the conflict-surface table (see `docs/UPSTREAM_MERGE.md`).

**Key namespacing in QSettings:**
- `"charging/"` — the screensaver (current)
- `"avatar/"` — avatar mod (on `feature/avatar` branch)
- Any new mod picks its own prefix to avoid key collisions

Always provide a safe default. Getters read directly from QSettings (not cached), so the default fires every time the key is absent from the persistent store.

**Legacy `CFG_*` macros** — previously in `src/config/config_macros.h`, removed in Batch E as dead code. They were a stepping-stone pattern from when screensaver properties lived inside upstream's `Config` class. Once the properties migrated to `ScreensaverConfig`, zero call sites remained and the file became maintenance-only. `SCRN_*` supersedes them with a strictly richer generator (full `Q_PROPERTY` + signal, not just getter/setter).

### §6.7 Config bridge singletons
When QML needs **transformed** config values (speed/50.0, conditional logic, cross-property derivations), create a bridge singleton (`ScreensaverConfig` pattern). Raw values → `Config` directly.

---

## §7 GPU RENDERER PATTERNS

> **Source:** [Qt 5.15 Scene Graph](https://doc.qt.io/archives/qt-5.15/qtquick-visualcanvas-scenegraph.html) · [Custom Geometry Example](https://doc.qt.io/qt-6/qtquick-scenegraph-customgeometry-example.html)

### §7.1 Anatomy
```
src/ui/myrenderer.h      — QQuickItem subclass declaration
src/ui/myrenderer.cpp    — Grid model, simulation tick, QSG rendering
```

### §7.2 Lifecycle

1. **Constructor** — call `setFlag(ItemHasContents, true)`. This is MANDATORY — without this flag, Qt will never call `updatePaintNode()` and nothing renders. (AP-UC-26)
2. **`componentComplete()`** — deferred init. Do heavy setup here (load fonts, build atlas, allocate grid arrays), NOT in the constructor. Copy this pattern from `MatrixRainItem`.
3. **Timer** fires `tick()` at target FPS, gated by `m_displayOff`.
4. **`tick()`** updates all simulation state: cell values, animation timers, brightness arrays. Calls `update()` to schedule a render sync.
5. **`update()`** → Qt schedules `updatePaintNode()` at the next sync point.
6. **`updatePaintNode()`** — builds/updates `QSGGeometryNode`. This is the ONLY place to create or modify QSG objects.

```cpp
// Constructor — MANDATORY flag
MyRenderer::MyRenderer(QQuickItem *parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);  // Without this, updatePaintNode() is never called
}
```

### §7.3 GlyphAtlas integration
- Register charset in `glyphatlas.h` `charsetString()`.
- Add font loader alongside `loadCJKFont()` (same pattern, ~15 lines).
- Atlas provides UV lookup per `(glyphIndex, brightnessLevel)`.
- Atlas rebuild is ~50-150ms on ARM — acceptable for mood transitions, NOT per-frame. (AP-UC-20)
- Font bundled in `deploy/config/`, subsetted via `pyftsubset`.

### §7.4 Thread safety and the render sync point

Qt's scene graph runs on a **separate render thread**. The `updatePaintNode()` call happens at a **sync point** where the GUI (main) thread is blocked. This means:

**SAFE in `updatePaintNode()`:**
- Reading member variables (`m_gridChar[]`, `m_gridBright[]`, etc.) that were prepared by `tick()` on the main thread. The GUI thread is blocked, so there's no race.
- Creating `QSGNode` subclasses (`QSGGeometryNode`, `QSGSimpleTextureNode`, etc.).
- Creating `QSGTexture` via `window()->createTextureFromImage()`.
- Updating vertex buffers, index buffers, UV coordinates.
- Reading `width()`, `height()`, and other QQuickItem geometry.

**NEVER do in `updatePaintNode()`:**
- Call `tick()` or modify simulation state — the render thread shouldn't drive simulation.
- Create `QSGTexture` or `QSGNode` objects anywhere else (they belong to the render thread).
- Retain `QSGNode` pointers as class members — the scene graph owns the nodes.

**Rule of thumb from Qt docs:** Only use classes with the `QSG` prefix inside `updatePaintNode()`.

**Marking nodes dirty (MANDATORY):** After changing geometry or material properties on a node, you MUST call `node->markDirty(QSGNode::DirtyGeometry)` and/or `node->markDirty(QSGNode::DirtyMaterial)`. Without this, the scene graph doesn't know the node changed and won't re-render it. (AP-UC-27)

```cpp
// Example: updating geometry in updatePaintNode()
node->setGeometry(geometry);
node->markDirty(QSGNode::DirtyGeometry);

// Example: updating material/texture
node->setMaterial(material);
node->markDirty(QSGNode::DirtyMaterial);
```

**Node ownership:** The scene graph manages node lifetime. Never retain `QSGNode` references in QQuickItem member variables — they can be destroyed by the scene graph at any time on the render thread. The `oldNode` parameter in `updatePaintNode(QSGNode *oldNode, ...)` is the only safe way to access your previous node. If `oldNode` is null, create a new one; otherwise, update the existing one.

**Destructor cleanup:** Copy the `MatrixRainNode` destructor pattern — clean up GPU resources (textures) on the render thread, not in the QQuickItem destructor (which runs on the GUI thread).

**Alternative to timer-driven updates:** For nodes that need pre-render preparation without a timer, set `QSGNode::UsePreprocess` flag and override `QSGNode::preprocess()`. This is called just before rendering each frame. Useful for one-off adjustments, but our renderers use timers for their main simulation loop.

### §7.5 displayOff power gating (MANDATORY)
Every renderer MUST stop its timer when `displayOff` is true. Zero CPU/GPU when screen off. Non-negotiable for battery life. (AP-UC-08)

```cpp
void MyRenderer::setDisplayOff(bool off) {
    if (m_displayOff == off) return;
    m_displayOff = off;
    if (off) m_timer.stop();
    else if (m_running) m_timer.start();
    emit displayOffChanged();
}
```

### §7.6 Performance rules
- Precompute brightness maps: `m_brightnessMap[distance] → atlas_level`.
- Use lookup tables for trig functions in animation loops (AP-UC-24).
- Two concurrent renderers (rain + avatar) expected fine, but verify on device.
- Degradation path: reduce density/effects when dual-rendering if frame budget exceeded.
- Prefer stack allocation over heap for per-frame temporary data.
- The scene graph renderer batches draw calls and retains geometry on GPU — design nodes to be batch-friendly (same material/texture where possible).

---

## §8 QML CONVENTIONS

> **Source:** [Qt 5.15 QML Coding Conventions](https://doc.qt.io/archives/qt-5.15/qml-codingconventions.html) · [Qt 5.15 Best Practices](https://doc.qt.io/archives/qt-5.15/qtquick-bestpractices.html)

### §8.1 Property declaration order

This project extends the official Qt QML attribute ordering with additional granularity for our component patterns. The [Qt 5.15 official order](https://doc.qt.io/archives/qt-5.15/qml-codingconventions.html) is: id → property declarations → signal declarations → JavaScript functions → object properties → child objects → states → transitions. We split "object properties" into anchors/geometry vs visual, add explicit slots for Connections/Loaders, and place `Component.onCompleted` last for readability:

```qml
Item {
    id: myComponent                          // 1. id (always first)

    // 2. Property declarations (custom + alias)
    property bool isActive: false
    property string currentMood: "neutral"
    property var entityRef: null             // Use concrete types when possible (AP-UC-16)
    readonly property string _prefix: "hass.main"

    // 3. Signal declarations
    signal moodChanged(string newMood)

    // 4. JavaScript functions
    function resolveMood() { ... }

    // 5. Object properties (anchors, geometry, visual)
    anchors.fill: parent
    width: 480; height: 850
    opacity: 1.0
    visible: true

    // 6. Signal handlers
    onIsActiveChanged: { ... }

    // 7. Child components, Loaders, Repeaters
    Loader { id: featureLoader; ... }

    // 8. Connections blocks
    Connections { target: ...; ignoreUnknownSignals: true; ... }

    // 9. States and Transitions
    states: [ State { name: "active"; ... } ]
    transitions: [ Transition { from: ""; to: "active"; ... } ]

    // 10. Component.onCompleted (always last)
    Component.onCompleted: { ... }
}
```

**Typed properties (AP-UC-16):** Always use the actual type when known: `property string name`, `property int size`, `property color moodColor`. Avoid `property var` unless the value genuinely can be multiple types or null. Untyped properties defeat static analysis and produce confusing error messages pointing to the declaration rather than the assignment.

**Group notation:** When using multiple properties from the same group, prefer group notation:
```qml
// Prefer this:
font { pixelSize: 14; family: "monospace" }
// Over this:
font.pixelSize: 14
font.family: "monospace"
```

### §8.2 Component naming & files
PascalCase files: `AvatarDisplay.qml`, `MoodEngine.qml`. Every `Loader` gets an `id`.

### §8.3 Entity access pattern
```qml
Component.onCompleted: EntityController.load(entityId)
Connections {
    target: EntityController
    ignoreUnknownSignals: true  // MANDATORY (AP-UC-02) — entity may not exist
    function onEntityLoaded(success, entityId) {
        if (!success) return;
        myEntity = EntityController.get(entityId);
    }
}
```
Entity IDs: `{prefix}.{ha_entity_id}` → `hass.main.sensor.ai_avatar_mood`. Prefix is configurable via `Config.avatarHaPrefix`.

### §8.4 Entity value handling
`sensor.getValue()` returns `QString`. Use `parseInt()` for numeric comparisons. Always null-guard: `if (myEntity && myEntity.value !== "")`.

### §8.5 HA bridge rules
- All `state.set()` sensors are volatile — seed via `state_bridge.py` on HA startup.
- Entity access is optional — mods must work without HA (local fallback).
- `hasOwnProperty` guards when driving properties on dynamically loaded themes.

### §8.6 Settings page decomposition
Sub-pages when >12 items (ChargingScreen pattern with `chargingscreen/` subfolder).

---

## §9 MOD ANATOMY — Template for New Features

```
src/ui/{feature}.h/.cpp                   C++ renderer or logic
src/ui/{feature}config.h/.cpp             Config bridge singleton (if needed)
src/qml/components/{feature}/             QML components
  {Feature}Display.qml                      Main visual wrapper
  {Feature}Engine.qml                       State/logic resolver
  {Feature}Overlay.qml                      Popup overlay (if applicable)
src/qml/components/overlays/              Shared overlay components
src/qml/settings/settings/
  {Feature}.qml                             Settings page entry
  {feature}/                                Settings sub-pages
deploy/config/                             Bundled assets (fonts)
src/qml/components/{feature}/art/         Art assets (compiled to qrc)
```

**Registration checklist:**
- [ ] `.h` → HEADERS in `remote-ui.pro` (at END)
- [ ] `.cpp` → SOURCES in `remote-ui.pro` (at END)
- [ ] `qmlRegisterType<>()` in `main.cpp`
- [ ] `setFlag(ItemHasContents, true)` in constructor (if visual QQuickItem)
- [ ] `Q_OBJECT` macro in class declaration
- [ ] Config bridge instantiation in `main.cpp` (if applicable)
- [ ] All QML files in `resources/qrc/main.qrc`
- [ ] Q_PROPERTYs in `config.h`/`config.cpp` (at END)
- [ ] Settings entry in `Settings.qml`
- [ ] Update `docs/CUSTOM_FILES.md`

---

## §10 QA AUDIT CHECKLIST

### §10.1 Pre-build gate
Before writing code for any new component:

| Check | What to verify |
|---|---|
| **Design doc read** | Have you read the relevant design doc? All decisions resolved? |
| **Pattern match** | Does the task match an existing pattern (screensaver, overlay, settings)? |
| **Mod anatomy** | Does the planned file structure follow §9? |
| **Git pre-flight** | §3.3 completed? |
| **Complexity budget** | Does the planned scope fit §1.6 limits? |

### §10.2 Pre-output self-check
Before presenting generated code to the user:

| # | Check | What to look for | Severity |
|---|---|---|---|
| Q1 | **Anti-pattern scan** | Scan §5 table top to bottom | Per AP |
| Q2 | **displayOff** | Does every timer/animation stop when displayOff is true? | ❌ |
| Q3 | **Thread safety** | Does updatePaintNode only create/modify QSG objects at sync point? | ❌ |
| Q4 | **ItemHasContents** | Does every visual QQuickItem set the flag in constructor? | ❌ |
| Q5 | **markDirty** | Is markDirty called after every geometry/material change? | ❌ |
| Q6 | **Q_OBJECT** | Does every QObject subclass have the macro? | ⚠️ |
| Q7 | **override** | Do reimplemented virtuals use `override` and omit `virtual`? | ⚠️ |
| Q8 | **Null guards** | Are all entity accesses null-guarded? | ❌ |
| Q9 | **Config defaults** | Do all new Config properties have safe defaults? | ⚠️ |
| Q10 | **Copyright** | Do new files have the copyright header? | ⚠️ |
| Q11 | **Registration** | Are new files registered in .pro, .qrc, main.cpp? | ❌ |
| Q12 | **ignoreUnknownSignals** | On every Connections to dynamic targets? | ⚠️ |
| Q13 | **Upstream diff** | Are custom additions at END of upstream lists? | ⚠️ |
| Q14 | **Fallback** | Does the feature degrade gracefully when disabled/HA unavailable? | ⚠️ |
| Q15 | **Typed properties** | Are QML properties declared with concrete types? | ⚠️ |
| Q16 | **Node ownership** | Are QSGNode references NOT retained as class members? | ❌ |

### §10.3 Periodic audit
For full codebase audits (run quarterly or before major features):

| Domain | Check | Severity |
|---|---|---|
| **Performance** | Any renderer running without displayOff gating? | ❌ |
| **Performance** | Any per-frame computation that could be precomputed? | ⚠️ |
| **Memory** | Any QSGNode without proper cleanup in destructor? | ❌ |
| **Memory** | Any QSGNode reference retained in QQuickItem? | ❌ |
| **Memory** | Any atlas rebuilt unnecessarily? | ⚠️ |
| **Upstream** | Any upstream files reformatted? | ⚠️ |
| **Upstream** | Custom additions not at end of lists? | ℹ️ |
| **Config** | Any property without default? | ⚠️ |
| **Config** | Any property missing NOTIFY signal? | ❌ |
| **Docs** | `CUSTOM_FILES.md` up to date? | ⚠️ |
| **Docs** | Design docs reflect current implementation? | ℹ️ |
| **Git** | Any uncommitted work? | ⚠️ |

---

## §11 SESSION DISCIPLINE

### §11.1 Ship it or lose it
When a file is finalized, write it to disk immediately. Don't hold finished code in conversation.

### §11.2 Reference, don't repeat
Once a code block has been established, refer to it by name or location — don't paste it again. If the user needs to see something again, re-read the file.

### §11.3 Artifact-first
When the deliverable is code, write the file. Don't narrate 300 lines of C++ across conversational messages.

| Situation | Do this | Not this |
|---|---|---|
| Delivering a new class | Write the file, summarize in 2-3 sentences | Walk through every method conversationally |
| Applying 5 fixes | Make the edits, list what changed | Explain each fix in a paragraph, then edit |
| User asks "what changed?" | Reference the git diff | Paste before and after |

### §11.4 Session scoping
One major deliverable per session. Don't start a second renderer in the same conversation where you just finished a 400-line C++ class. Quick follow-ups are fine.

### §11.5 Turn threshold
~15 exchanges without shipping = pause and reassess scope.

---

## §12 UC3 HARDWARE CONSTRAINTS

| Spec | Value | Impact |
|---|---|---|
| CPU | ARM64 quad-core 1.8 GHz | Budget simulation complexity |
| GPU | Embedded (in SoC) | Single draw call preferred; two max |
| RAM | 4 GB | Atlas textures live in GPU memory |
| Display | 480 × 850px IPS | 14px font → ~68×67 cell grid. No burn-in risk. |
| Battery | ~8.88 Wh Li-ion | displayOff gating MANDATORY |
| Storage | 32 GB eMMC | Binary size matters |

---

## §13 COMMUNICATION STYLE

- Talk like Quark from DS9. Curse when it fits — for emphasis, frustration, or color.
- Be direct. Don't over-explain obvious things.
- When reviewing, suggest concrete improvements with code.
- Edit files directly when filesystem access is available.
- Present options with trade-offs and let the user choose.
- **Explain as you go** — narrate reasoning in real time, not just in footnotes after 400 lines of C++. If you hit a surprise mid-generation, say so.
