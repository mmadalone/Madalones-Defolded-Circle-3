# STYLE_GUIDE.md — UC Remote 3 Coding, Architecture & Operational Standards

> **Scope:** This guide governs all AI-assisted development on the UC Remote 3 custom firmware project. It covers coding conventions, architectural patterns, operational workflow, anti-patterns, quality gates, and session discipline. Adapted from the Project Fronkensteen HA Master Style Guide ("Rules of Acquisition") for the Qt 5.15 / QML / C++17 domain.

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
| AP-UC-07 | ❌ | `updatePaintNode()` accesses data not prepared in `tick()` | §7.4 |
| AP-UC-08 | ❌ | Timer continues when `displayOff` is true | §7.5 |
| AP-UC-09 | ⚠️ | Qt 6 API used (project is Qt 5.15) | §1.4 |
| AP-UC-10 | ⚠️ | Missing `id:` on QML Loader | §8.2 |
| AP-UC-11 | ⚠️ | Config property missing default value | §6.6 |
| AP-UC-12 | ⚠️ | New Config Q_PROPERTY placed mid-list (merge conflict risk) | §3.5 |
| AP-UC-13 | ℹ️ | `visible: false` where `Loader` with `active:` would avoid instantiation | §1.4 |
| AP-UC-14 | ⚠️ | Single-pass generation over ~150 lines | §4.5 |
| AP-UC-15 | ⚠️ | Missing copyright header on new file | §6.1 |
| AP-UC-16 | ℹ️ | QML property without type annotation when type is known | §8.1 |

### Renderer / GPU

| ID | Sev | Trigger pattern | Fix ref |
|---|---|---|---|
| AP-UC-20 | ❌ | Atlas rebuilt per frame | §7.3 |
| AP-UC-21 | ❌ | `QSGTexture` created outside `updatePaintNode()` | §7.4 |
| AP-UC-22 | ⚠️ | Vertex buffer fully reconstructed when only UVs changed | §7.4 |
| AP-UC-23 | ❌ | `QSGNode` leaked — missing destructor cleanup | §7.4 |
| AP-UC-24 | ⚠️ | Per-frame trig that could be precomputed into lookup table | §7.6 |
| AP-UC-25 | ⚠️ | `GlyphAtlas` charset registered but font not loaded in `main.cpp` | §7.3 |

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
1. Own header · 2. Qt headers · 3. Project headers

### §6.4 Namespace
`uc` namespace (matches upstream).

### §6.5 Q_PROPERTY ordering
Group: lifecycle → state → visual config → computed. New properties at END.

### §6.6 Config macros
`CFG_BOOL`, `CFG_INT`, `CFG_STRING` in `config_macros.h`. QSettings key namespacing: `"charging/"`, `"avatar/"`. Always provide safe defaults.

### §6.7 Config bridge singletons
When QML needs transformed values → bridge singleton (`ScreensaverConfig` pattern). Raw values → `Config` directly.

---

## §7 GPU RENDERER PATTERNS

### §7.1 Anatomy
`src/ui/myrenderer.h` (declaration) + `src/ui/myrenderer.cpp` (implementation).

### §7.2 Lifecycle
`componentComplete()` → timer → `tick()` → `update()` → `updatePaintNode()`.

### §7.3 GlyphAtlas
Register charset → add font loader → atlas provides UV per `(glyph, brightness)`. Rebuild ~50-150ms on ARM. Font in `deploy/config/`.

### §7.4 Thread safety
`updatePaintNode()` on render thread. Only read tick-prepared data. Create textures here. Copy MatrixRainNode destructor pattern.

### §7.5 displayOff (MANDATORY)
Stop timer when `displayOff` true. Zero CPU/GPU. Non-negotiable.

### §7.6 Performance
Precompute brightness maps. Lookup tables over per-frame math. Two renderers max. Degradation path: reduce density if frame budget exceeded.

---

## §8 QML CONVENTIONS

### §8.1 Property order
id → anchors → visual → custom properties → readonly → signals → handlers → children → Connections → functions → Component.onCompleted.

### §8.2 Naming
PascalCase files. Every Loader gets an `id`.

### §8.3 Entity access
`EntityController.load()` on `Component.onCompleted`. `ignoreUnknownSignals: true` on all entity Connections. Null-guard all access.

### §8.4 Entity values
`getValue()` returns `QString`. `parseInt()` for numbers. Configurable prefix via `Config.avatarHaPrefix`.

### §8.5 HA bridge
`state.set()` sensors seeded via `state_bridge.py`. HA optional — local fallback. `hasOwnProperty` guards on theme properties.

### §8.6 Settings decomposition
Sub-pages when >12 items (ChargingScreen pattern).

---

## §9 MOD ANATOMY

New mod template: C++ renderer → config bridge → QML components → settings pages → assets → qrc/pro registration → CUSTOM_FILES.md update. Full checklist in §9 of the extended guide.

---

## §10 QA AUDIT CHECKLIST

### §10.1 Pre-build gate
Design doc read? Pattern match? Mod anatomy? Git pre-flight? Complexity budget?

### §10.2 Pre-output self-check
Anti-pattern scan (§5) → displayOff check → thread safety → null guards → config defaults → copyright → registration → ignoreUnknownSignals → upstream diff position → fallback behavior.

### §10.3 Periodic audit
Performance (displayOff, precomputation) → Memory (QSGNode cleanup, atlas rebuilds) → Upstream (no reformatting, end-of-list additions) → Config (defaults, signals) → Docs (CUSTOM_FILES current) → Git (clean state).

---

## §11 SESSION DISCIPLINE

### §11.1 Ship it or lose it — write to disk immediately.
### §11.2 Reference, don't repeat — don't paste code twice.
### §11.3 Artifact-first — write files, not conversational narration.
### §11.4 One major deliverable per session.
### §11.5 ~15 exchanges without shipping = pause and reassess.

---

## §12 HARDWARE CONSTRAINTS

ARM64 1.8 GHz, 4 GB RAM, 480×850 IPS, ~8.88 Wh battery, 32 GB eMMC. displayOff gating mandatory. Single draw call preferred, two max.

---

## §13 COMMUNICATION STYLE

Talk like Quark. Curse when it fits. Be direct. Explain as you go. Edit files directly. Present trade-offs, let the user choose.
