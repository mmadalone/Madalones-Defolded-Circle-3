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
> | GPU perf numbers (atlas rebuild) | Empirical testing on device | ⚠️ Empirical |
> | UC3 SoC constraints | UC marketing materials + empirical | ⚠️ Partial |
> | UCR3 firmware OpenAPI spec | `/doc/core-rest/openapi.yaml` (anonymous Swagger UI baked into firmware, OpenAPI 3.1.1) | ⚠️ Source-derived |
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
- **For integrations:** Consult UC's Core API docs, integration driver source, and entity model headers before assuming behavior. Verify from actual source code — don't guess at upstream behavior.
- Prefer established UC patterns over novel inventions:
  - `ButtonNavigation` for input handling (not raw `Keys.onPressed`).
  - `Popup` for overlays (not custom `Item` with manual z-ordering).
  - `Loader` for conditional components (not `visible: false` with full instantiation).
    - For `Repeater` with a non-trivial range: gate the **model**, not the wrapper. Use `model: cond ? N : 0` — `Repeater.model` evaluates regardless of any enclosing `visible:` / `enabled:`. Wrapper visibility gates rendering, not delegate instantiation. Severity scales with `to − from`. (v1.4.33: a 100–5000 Slider tick `Repeater` gated only by `Row { visible: showTicks }` materialized ~4,901 invisible Rectangles + O(N²) Row positioner reflows → 70-sec Settings → Power open on UCR3.)
  - `EntityController.load()` → `onEntityLoaded` for entity access (not raw WebSocket).
  - `core::Api` wrappers for firmware calls (not raw WebSocket; not raw HTTP without first checking `/doc/core-rest/openapi.yaml` per §1.4.1).
- **Prefer declarative property bindings over imperative JavaScript.** QML is a declarative language — use bindings for reactive UI updates. Use imperative JS only for complex logic that can't be expressed declaratively (multi-step calculations, network calls, state machine transitions). If a binding expression exceeds ~3 lines, extract it into a JS function — but the function should still *return* a value for binding, not imperatively set properties.

#### §1.4.1 Firmware API discovery hierarchy (MANDATORY before any firmware-interaction code)

Before adding ANY code path that talks to the UC core daemon — REST, WebSocket, or otherwise — walk this checklist top to bottom. Skipping it has cost us release cycles (Mod 5 hand-rolled `set_power_mode` ping loop while `/system/power/standby_inhibitors` was a documented native endpoint one Swagger-UI tab away).

| # | Check | What it gets you |
|---|---|---|
| 1 | **Open the firmware OpenAPI spec** at `http://${UC3_HOST}/doc/core-rest/` (Swagger UI, anonymous). Spec is at `/doc/core-rest/openapi.yaml`. 137 endpoints across 15 tag categories: `activities`, `api-keys`, `auth`, `cfg`, `dock`, `entities`, `external-token`, `info`, `infrared`, `integrations`, `macros`, `profiles`, `remotes`, `resources`, `system`. | If a documented REST endpoint exists for what you want, **prefer REST over WS** for one-shot calls. |
| 2 | **Search `src/core/enums.h::RequestTypes::Enum`** for related operation names. Then `git grep` to see whether the entry has actual call sites. | If wired (existing call sites): use the wrapper in `core::Api`. The codebase already trusts this RPC. |
| 3 | **Identify orphan WS RPCs** — `RequestTypes::Enum` entries with zero call sites. Often indicate firmware features the upstream UI never wired up. | Wiring an orphan = enabling a latent firmware capability for free. Track record: v1.4.10 (`entityAdded`), v1.4.12 (`REASSOCIATE`), v1.4.14 (`set_power_mode`), v1.4.20. See `project_orphan_request_types_pattern.md`. |
| 4 | **Undocumented territory** — neither REST nor `enums.h` has it. | Stop, flag to the user. Probing requires WS RPC name guessing, which is brittle. Don't reverse-engineer here without explicit go-ahead. |

**Source-of-truth memos:** `reference_uc_core_rest_api.md`, `project_orphan_request_types_pattern.md`.

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

0. **For firmware-interaction code: walk §1.4.1's discovery hierarchy first.** Open `/doc/core-rest/openapi.yaml`, search `RequestTypes::Enum`, identify whether a documented REST endpoint or wired/orphan WS RPC already exists. **This is step zero** — if you skip it, you may hand-roll something the firmware already does natively. Tracked anti-pattern: AP-UC-44.
1. **Read the design doc** — SCREENSAVER-IMPLEMENTATION.md.
2. **Read the actual source** — Don't assume API behavior. Check headers (`sensor.h`, `entityController.h`, `config.h`, `matrixrain.h`). Check QML files for existing patterns.
3. **Check for existing implementations** — Search the codebase for similar patterns before inventing new ones.
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
| 4 | **Upstream compatibility** — Don't break stock behavior | Users with features disabled must not be affected |
| 5 | **Reasoning-first** (§1.7) | Prevents hallucinated code |
| 6 | **Chunked generation** (§4.5) | Quality control for large files |
| 7 | **Anti-pattern scan** (§5) | Last gate before delivery |

**User override:** The user can say "skip X" for workflow preferences — but NOT for safety, git checkpoints, or displayOff gating. Push back on those.

### §1.11 Auto-revert is a crash safety net, not a validation review

The UCR3 firmware automatically reverts the custom UI binary to stock if the binary fails to start (verified empirically — see `project_auto_revert_validated_on_uc3.md`). It is tempting to treat this as a catch-all for risky deploys.

**It is not.** Auto-revert triggers on:
- Process crash / segfault during startup.
- Hard hang detected by firmware watchdog.
- The UI failing to register with the core daemon within the boot timeout.

**It does NOT trigger on:**
- A binary that starts cleanly but sends a malformed `setPowerMode` body that the firmware accepts and propagates into a corrupted downstream state. (v1.4.22 silent failure precedent: every `setPowerMode` call returned HTTP 400 for 8 releases — Mod 5 / Mod 6 were silently broken, never auto-reverted, never logged any error.)
- A binary that starts cleanly but persists an out-of-bounds value to QSettings or a JSON state file, leaving the device in a degraded state on next boot.
- A binary that starts cleanly but pushes a malformed entity event through the core daemon, corrupting integration state.
- Any class of bug where the symptom is "wrong behavior" rather than "absent behavior."

**The trust boundary is code review, not auto-revert.** Treat auto-revert as the seatbelt — useful when something goes wrong, no excuse for skipping the brakes. Do not skip input validation review on `Api::set*` body construction, do not skip range clamping on persisted config, do not skip cross-checking that an entity push uses well-formed payloads, just because "auto-revert will catch it." It will not.

This pairs with §1.5 (uncertainty signals — STOP and ask, don't guess) and §1.8 (research-first mandate). When in doubt about a firmware-side API contract, verify from headers, the upstream source, or a Logdy capture of a known-good message — never ship the guess and rely on auto-revert.

### §1.12 Probe-first delivery for firmware-write RPCs (MANDATORY)

When you're about to write code that *sends* a payload to the firmware — `core::Api::set*`, any new WS RPC body, any REST `POST` / `PUT` — the body shape MUST be confirmed against an empirical capture of a known-good message before the code lands. Reading the OpenAPI spec (§1.4.1) is necessary but not sufficient: REST URL field convention (`?power_mode=NORMAL`) and WS RPC body convention (`{"mode": "NORMAL"}`) are not interchangeable, and only a wire capture distinguishes them (AP-UC-47).

**The four-step gate:**

| # | Step | Artifact |
|---|---|---|
| 1 | Walk §1.4.1's discovery hierarchy. Identify the RPC name and transport. | Decision: REST endpoint, wired WS RPC, or orphan WS RPC |
| 2 | Capture a known-good message — Logdy on a successful upstream-UI action, a curl probe to a public reference client, or a `pyUnfoldedCircle` SDK call traced over Logdy. | Capture file under `logs/` (gitignored), referenced in the build log |
| 3 | Diff the captured body against the body your code will send, **field by field**. | Diff explicit in the build log or PR description |
| 4 | Only now write the body. The first runtime call against the device confirms the contract — log success/failure to `qCInfo` so a regression is visible without a debug build. | Code |

**Why mandatory:** v1.4.14 → v1.4.22 silent failure (`setPowerMode` body field `power_mode` instead of `mode`) ate 8 releases. The OpenAPI spec was internally consistent, the code compiled, the binary started cleanly, and auto-revert (§1.11) never tripped — every call returned HTTP 400 and Mods 5/6 were no-ops in production. A two-minute Logdy capture of an upstream-UI standby toggle would have shown the correct field name on day one.

**Industry analog:** contract-first / API design-first development (Stoplight, Bump.sh, Moesif). Write against a verified contract, not a guessed one. The same shape — capture or generate the contract, then implement — is established in OpenAPI mocking, gRPC reflection, and embedded protocol reverse engineering.

**Pairs with:** §1.4.1 (which surface), §1.5 (uncertainty), §1.11 (auto-revert is not a validation review), §13.4 (Logdy is the capture tool), AP-UC-47, AP-UC-49.

**Exceptions:**
- Read paths (`Api::get*`, anonymous endpoints `/api/pub/*`) — no body, no shape ambiguity.
- Pre-existing wrappers — §1.12 applies forward from this revision; retroactive audit of `core::Api::set*` callsites is a separate task, not implied here.

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
[renderer] matrixrain: implement per-cell breathing animation
[settings] screensaver: add clock overlay toggle and color picker
[fix] matrixrain: prevent atlas rebuild during displayOff
[audit] screensaver: 8 warnings fixed across 4 files
[docs] style guide v2: add operational framework
[mod] screensaver: initial implementation of Matrix theme system
```

### §3.3 Pre-flight checklist (MANDATORY — before first edit in any BUILD session)

**Stop. Before you edit any project file:**

1. ✅ Check `git status` — resolve any uncommitted changes from a previous session.
2. ✅ Commit or stash anything dirty with a descriptive message.
3. ✅ **If the task touches firmware interaction (REST or WS):** confirm you've walked §1.4.1's API discovery hierarchy. (AP-UC-44)
4. ✅ Only NOW may you edit files.

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

### §3.6 Pre-tag release gates (MANDATORY before `git tag`)

A tag commits to a binary the world can install. v1.4.18 (6 releases shipped with mismatched VERSION fields), v1.4.27 → v1.4.32 (link-error chain on hardware tests), and v1.4.37 → v1.4.38 (audit-fix introduced a fresh bug) are the cost of skipping gates.

The CI workflow `.github/workflows/build.yml:37-87` already enforces gates 1 + 2 on tag push. Running them locally before push saves a tag-rebuild roundtrip when they fail. Single-maintainer fork — `--no-verify`-equivalent skips defeat the gate.

| # | Gate | Status | Verify locally with |
|---|---|---|---|
| 1 | `remote-ui.pro` `VERSION` == `deploy/release.json` `version` == tag string | MUST | grep both files; CI mirrors at `build.yml:44-67` |
| 2 | `CHANGELOG.md` has a heading for the version | MUST | grep; CI mirrors at `build.yml:68-87` |
| 3 | `git status --porcelain` empty (no untracked, no modified, no `_build_logs/` staged accidentally) | MUST | one command |
| 4 | `lupdate` regen'd if any new `qsTr` strings landed | SHOULD | `git diff --stat resources/translations/` after running lupdate |
| 5 | Local clean test build (`qmake && make`, plus `test/hardware/*.pro` + `test/qml/*.pro` if those changed) | SHOULD | exit code |
| 6 | CI green on the commit you're about to tag (`build.yml`, `test.yml`, `code_guidelines.yml`, `tidy.yml`) | MUST | GH Actions UI |

**Skippable when:** doc/test-only release with byte-identical binary to the prior tag — note the fact in the release notes (precedent: v1.4.37, v1.4.38). Gates 4 and 5 are SHOULD, not MUST, specifically to keep this skip clean.

**No automation prescribed in this rev.** A `tools/release_gate.sh` mirroring gates 1-3 + 5 is a valid follow-up; out of scope here. The pre-commit hook stays focused on cpplint / clang-format (its current role at `.githooks/pre-commit`); release gates run at tag time, not commit time.

**Pairs with:** §3.3 (BUILD-mode pre-flight is per edit; this is per tag), §11.1, AP-UC-50, AP-UC-51.

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
| 1 | src/ui/matrixrain.h | Class declaration | ✅ |
| 2 | src/ui/matrixrain.cpp | Stream model + tick | 🔧 |

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
| AP-UC-44 | ⚠️ | Wrote firmware-interaction code without consulting `/doc/core-rest/openapi.yaml` first (skipped §1.4.1) | §1.4.1 / §1.8 |
| AP-UC-45 | ⚠️ | Polled state via Logdy log stream instead of a REST endpoint that exists for it | §13.4 |
| AP-UC-46 | ❌ | Hand-rolled firmware functionality (timer loop, retry, state machine) when a documented native endpoint exists. Cautionary tale: Mod 5's `set_power_mode` ping vs `/system/power/standby_inhibitors`. | §13.2 |
| AP-UC-47 | ⚠️ | REST URL query convention applied to WS RPC body (or vice versa). E.g., REST uses `?power_mode=NORMAL` query; WS RPC uses `{"mode": "NORMAL"}` body. They are NOT interchangeable. v1.4.22 cautionary tale (`project_setpowermode_field_bug.md`). | §13.2 |
| AP-UC-48 | ℹ️ | Repeatedly polling a state when a push-event channel exists, or subscribing to events when a one-shot REST GET would do. | §13.2 |
| AP-UC-49 | ❌ | Hand-rolled firmware-write RPC body without an empirical wire capture of a known-good message (skipped §1.12). v1.4.14 → v1.4.22 `setPowerMode` silent-failure precedent: 8 releases of HTTP 400, Mods 5/6 no-op in production, auto-revert never tripped. | §1.12 |
| AP-UC-50 | ⚠️ | Audit-derived remediation shipped without re-verifying the audit's claim (LOC, code path, or architectural assumption may be stale). v1.4.26 → v1.4.33: chased a GridLayout perf hypothesis the audit named; actual cause was upstream Slider's `Repeater.model`. Memo: `feedback_verify_audit_before_remediation.md`. | §1.7, §1.8 |
| AP-UC-51 | ⚠️ | Symptom fix landed before root cause was empirically confirmed (Logdy capture, probe log, or repro test). v1.4.37 → v1.4.38: a CI-test fix shipped on a wrong assumption about `InputHandler` dispatch path; symptom (red CI) was treated as cause. Confirm the fix actually addresses the failing premise before tagging. | §1.5 |
| AP-UC-52 | ⚠️ | New `Q_INVOKABLE`/signal/slot/`Q_PROPERTY` accessor added to a `Q_OBJECT` class that has a `test/hardware/mock_*.cpp` stub but no corresponding stub for the new symbol. moc generates a metacall reference; the test binary fails to link with `undefined reference to ...`. v1.4.39 case study: Phase 0's `Q_INVOKABLE void Api::probeRestAuth()` (commit `9af426a`) was a local-only build per audit Q2 — never went through CI's Hardware-tests workflow. The M1 push tagged for v1.4.39 surfaced the missing stub, blocked the tag until `fa3c11d` added a no-op `void Api::probeRestAuth() {}` to `mock_core_api.cpp`. Pattern check: any new MOC-instrumented member on a class with a stubbed test counterpart needs a parallel stub. | §6 |

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
| AP-UC-33 | ⚠️ | Push event with no timeout fallback | §8.6 |
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

Per [Qt Wiki Coding Conventions](https://wiki.qt.io/Coding_Conventions): specialized → generic Qt → STL → system, blank line between groups for readability.

1. Own header (`#include "ui/matrixrain.h"`)
2. Qt headers (`#include <QObject>`, `#include <QQuickItem>`)
3. Project headers (`#include "../config/config.h"`)
4. STL headers (`#include <memory>`, `#include <vector>`)
5. System headers (`#include <unistd.h>`, `#include <fcntl.h>`)

If `qplatformdefs.h` is ever included, it must be the **first** header — Qt requires this so platform-specific macro definitions take effect before any other parsing. The project doesn't use it today.

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
- Any new mod picks its own prefix to avoid key collisions

Always provide a safe default. Getters read directly from QSettings (not cached), so the default fires every time the key is absent from the persistent store.

**Legacy `CFG_*` macros** — previously in `src/config/config_macros.h`, removed in Batch E as dead code. They were a stepping-stone pattern from when screensaver properties lived inside upstream's `Config` class. Once the properties migrated to `ScreensaverConfig`, zero call sites remained and the file became maintenance-only. `SCRN_*` supersedes them with a strictly richer generator (full `Q_PROPERTY` + signal, not just getter/setter).

### §6.7 Config bridge singletons
When QML needs **transformed** config values (speed/50.0, conditional logic, cross-property derivations), create a bridge singleton (`ScreensaverConfig` pattern). Raw values → `Config` directly.

### §6.8 Shared firmware helpers (HTTP/REST + WS)

When a mod needs to call the firmware over HTTP/REST (e.g., to hit an endpoint listed in `/doc/core-rest/openapi.yaml`), the helper goes in `core::Api` — not in the mod itself. This is the WS-API pattern extended to HTTP.

**Why centralize:**
- Auth resolution (basic / Bearer / cookie / loopback exemption) is one decision, not per-mod.
- Connection state, host URL, and lifecycle are already managed by `core::Api`.
- Future REST consumers reuse the same helper instead of each one rolling its own `QNetworkAccessManager`.

**Pattern:** add typed wrappers (e.g., `core::Api::createStandbyInhibitor(...)`, `core::Api::deleteStandbyInhibitor(...)`) that mirror the WS-RPC wrapper style (`setPowerMode`, `setPowerSavingCfg`). Underlying transport is an implementation detail of `core::Api`. The mod calling code shouldn't care whether the wrapper hits REST or WS.

**Don't:** embed `QNetworkAccessManager` inside `activitySessionKeeper.cpp`, `phantomWakeSuppressor.cpp`, or any other mod's logic. That fragments auth handling and duplicates infra. Today the codebase has zero authenticated REST consumers — the first one to land sets the pattern for the rest.

See §13.3 for the auth ladder a shared helper should walk.

### §6.9 QObject ownership and smart pointers

Qt's parent-child memory model and the C++ Core Guidelines' smart-pointer rules (R.20-R.34) collide. Mixing `std::unique_ptr<T>` with a `T` that has a QObject parent causes double-deletion: the parent destructor frees the object first, then `unique_ptr` destruction frees it again. KDAB and cleanqt.io flag this as the most common Qt-plus-modern-C++ footgun.

| Pattern | Use |
|---|---|
| QObject created with a parent | Raw pointer. Parent owns lifetime. Don't wrap in `unique_ptr`. |
| QObject created without a parent (e.g., a singleton constructed in `main.cpp` and held for app lifetime) | `std::unique_ptr<T>` constructed via `std::make_unique<T>(...)` (per C++ Core Guidelines R.23) to enforce single-owner cleanup. |
| Non-owning observer of a QObject that may outlive the observer | `QPointer<T>` — auto-nulls when the target is destroyed. Never raw pointer when the target is parented elsewhere. |
| Shared ownership | Rare in this codebase. Prefer single-owner with `QPointer` observers. `std::shared_ptr<T>` only when refcount semantics are genuinely required. |

**Anti-pattern:** `auto child = std::make_unique<MyClass>(parent);` where `parent` is a QObject. Either drop the parent (`make_unique<MyClass>()`) or drop the smart pointer (`new MyClass(parent)`) — pick one ownership model.

**Forward-looking only.** This rule applies to new code from this revision; a retroactive scrub of existing `unique_ptr` / `QPointer` / parent-owned QObject usage is out of scope here.

**Sources:**
- [C++ Core Guidelines R.20-R.34](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#R-resource) — smart-pointer ownership rules. Verbatim canonical: R.20 *"Use unique_ptr or shared_ptr to represent ownership"*, R.21 *"Prefer unique_ptr over shared_ptr unless you need to share ownership"*, R.23 *"Use make_unique() to make unique_ptrs"*.
- [Qt Object Trees & Ownership](https://doc.qt.io/qt-5/objecttrees.html) — canonical doc for the QObject parent-child ownership model that conflicts with `std::unique_ptr`.
- Qt Wiki *[Shared Pointers and QML Ownership](https://wiki.qt.io/Shared_Pointers_and_QML_Ownership)* — focuses on `QSharedPointer`, but the parent-ownership-collision principle generalizes to `std::unique_ptr`. Article notes: *"It is too dangerous to mix QSharedPointer ownership with the QObject parent/child ownership"*.
- [cleanqt.io *Crash course in Qt for C++ developers, Part 4*](https://www.cleanqt.io/blog/crash-course-in-qt-for-c++-developers,-part-4) — practical patterns and code examples.

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

**Marking nodes dirty (MANDATORY when bypassing helpers):** Most `QSGNode` setter methods (`QSGGeometryNode::setGeometry()`, `setMaterial()`, `appendChildNode()`, etc.) implicitly call `markDirty()` for you — Qt docs: *"Most of the functions on the node classes will implicitly call markDirty()."* You MUST call it explicitly only when you bypass those helpers — constructing geometry/material structs by hand and assigning them through low-level pointers, or modifying child-node trees outside the `appendChildNode()` family. The flags: `DirtyGeometry` (0x1000), `DirtyMaterial` (0x2000), `DirtyMatrix` (0x0100), `DirtyOpacity` (0x4000), `DirtyNodeAdded` (0x0400), `DirtyNodeRemoved` (0x0800), `DirtySubtreeBlocked` (0x0080). Without `markDirty()` on a manual modification, the scene graph won't re-render the node. (AP-UC-27; see [QSGNode docs](https://doc.qt.io/archives/qt-5.15/qsgnode.html))

```cpp
// Example: updating geometry in updatePaintNode()
node->setGeometry(geometry);
node->markDirty(QSGNode::DirtyGeometry);

// Example: updating material/texture
node->setMaterial(material);
node->markDirty(QSGNode::DirtyMaterial);
```

**Node ownership:** The scene graph manages node lifetime. Never retain `QSGNode` references in QQuickItem member variables — they can be destroyed by the scene graph at any time on the render thread. The `oldNode` parameter in `updatePaintNode(QSGNode *oldNode, ...)` is the only safe way to access your previous node. If `oldNode` is null, create a new one; otherwise, update the existing one. Child nodes appended via `appendChildNode()` get the `QSGNode::OwnedByParent` flag automatically — Qt docs: *"Assigning ownership to the scene graph is often preferable as it simplifies cleanup when the scene graph lives outside the GUI thread."* (AP-UC-23)

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
- Multiple concurrent renderers expected fine, but verify on device.
- Degradation path: reduce density/effects when rendering if frame budget exceeded.
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

### §8.1.1 Qt 5.15 official syntax conventions (ℹ️ INFO)

Three minor syntax conventions Qt's [official QML Coding Conventions](https://doc.qt.io/archives/qt-5.15/qml-codingconventions.html) call out that aren't worth their own AP-UC entries but worth aligning with for tooling and visual consistency with Qt's own examples:

1. **Avoid square brackets for single-element lists.** Prefer `states: State { ... }` over `states: [ State { ... } ]`. Brackets are only required for multi-element lists.
2. **Don't use semicolons in single-line expressions.** `text: "hello"` not `text: "hello";`.
3. **Use semicolons consistently in multi-statement JS blocks.** All statements get a semicolon, or none do — pick one within a block and stick with it.

These don't break anything; they're stylistic alignment with Qt's own examples and Qt Quick Designer / qmllint expectations.

### §8.2 Component naming & files
PascalCase files: `MatrixTheme.qml`, `ScreenOffOverlay.qml`. Every `Loader` gets an `id`.

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
Entity IDs: `{prefix}.{ha_entity_id}` → `hass.main.sensor.living_room_temperature`. Prefix is configurable via `Config`.

### §8.4 Entity value handling
`sensor.getValue()` returns `QString`. Use `parseInt()` for numeric comparisons. Always null-guard: `if (myEntity && myEntity.value !== "")`.

### §8.5 HA bridge rules
- All `state.set()` sensors are volatile — seed via `state_bridge.py` on HA startup.
- Entity access is optional — mods must work without HA (local fallback).
- `hasOwnProperty` guards when driving properties on dynamically loaded themes.

### §8.6 Settings page decomposition
Sub-pages when >12 items (ChargingScreen pattern with `chargingscreen/` subfolder).

### §8.7 QML property value sources are unreachable from outside the component (TEST-ONLY)

Animations attached via `<Type> on <Property>` syntax — `SequentialAnimation on opacity { ... }`, `RotationAnimation on rotation { ... }`, `Behavior on y { ... }` — register as Qt property value sources. **They do not appear in the parent's `children`, `resources`, or `data` lists.** A test file in another QML can't reach them via `findByObjectName` recursion; setting `objectName: "..."` on the animation is a no-op for traversal.

Production is unaffected — within the source QML file, the animation's `id` is in scope so `running: hudRoot.active`-style bindings work as written. The unreachability is **TEST-ONLY**.

**Two solutions:**
- **(A) Trust the binding chain (preferred).** Verify the source-of-truth property (`readonly property bool active: SomeSingleton.someProperty`); trust Qt to propagate it to `running: hudRoot.active`. The test reads the source-of-truth property, not the animation.
- **(B) Expose `property alias` on the component root** for direct test access. Cost: pollutes production surface with test-only properties.

**When:** Option A when the binding chain is short and the source-of-truth property is exposed. Option B when the animation's triggering logic isn't derivable from a single root property.

**Source incident:** v1.4.34 → v1.4.35 — `tst_reconnecting_hud.qml` shipped with `findByObjectName(banner, "hudPulse")` lookups; CI failed; replaced with a binding-chain test on `hud.active`. Memory: `feedback_qml_animation_value_sources.md`.

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

**If the mod talks to the firmware** (REST, WS, or both): walk §1.4.1's API discovery hierarchy before writing the first line of transport code. Add any new transport wrappers to `core::Api` (§6.8), not to the mod itself. Authentication choices follow the §13.3 ladder. Diagnostics go through Logdy per §13.4 — for *debugging*, not for telemetry.

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
| Q17 | **Probe-first body shape** | For every firmware-write call, was the body diffed against a captured known-good message? (§1.12 / AP-UC-49) | ❌ |
| Q18 | **Audit claim freshness** | For any audit-derived edit costing >30 min: have you re-read the current code and confirmed the claim, the LOC, and the simplest fix? (§1.8 / AP-UC-50) | ⚠️ |
| Q19 | **Root cause empirical** | For any bug fix: do you have an empirical artifact (Logdy capture, probe log, repro test) confirming the root cause? Or are you patching a symptom? (AP-UC-51) | ⚠️ |

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

### §11.6 Propose style-guide updates from session learnings

This guide is a living document. At end of session — or when you notice the trigger mid-session — propose a style-guide update if you've hit:

- The same gotcha in two different sessions (codify so a third doesn't happen)
- A new memory that captures a behavioral rule the guide doesn't have a section for
- A firmware-API or upstream-merge surprise that future sessions will repeat without the rule
- A pattern from a shipped mod worth generalizing (new template, new anti-pattern, new sub-§)

**Process:** propose in one sentence — what the rule is and where it'd live (existing §, new sub-§, or new AP-UC). Wait for yes / no / defer. If yes, make the edit in the same session.

**Don't propose:**
- Speculative or tentative rules ("maybe we should…") — only add what's proven by repeated incidence
- One-off observations — patterns only
- Backlog items or TODOs — those belong in build logs or memory, not the guide
- Anything `git log` or an existing memo already captures

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

### §12.1 Disk I/O is slow on UC3 eMMC

Empirical timing on UCR3 hardware: a 3.7 MB `QSaveFile` write took ~7 s; plain `QFile` ~12 s. Even `QFile::open` on a non-existent path blocks significantly. Two cache-revert incidents on this fork are direct consequences.

**Never** put synchronous disk I/O in:
- Render hot paths (renderer ticks, `updatePaintNode()`)
- Startup-critical code (binary load, `Component.onCompleted`, `componentComplete()`)
- `updatePolish()` callbacks
- Any code on the GUI thread that fires more than once per second

Use in-memory caching (static class members; see `AtlasBuilder::s_singleCacheKey` precedent in `src/ui/matrixrain/atlasbuilder.cpp`). If persistence is needed, defer to a background thread (`QtConcurrent::run`, `QThreadPool::globalInstance()->start(...)`) or trigger on `Power::Idle`.

Memory: `feedback_uc3_disk_io.md`.

---

## §13 FIRMWARE API & DIAGNOSTICS

This section gathers what we know about talking to the UC core daemon and observing the firmware. It's the operational counterpart to §1.4.1 (the discovery checklist) — once you've decided *what* surface to call, this section covers *how* to call it correctly and how to debug when things misbehave.

### §13.1 Sources of truth (where to look first)

Before any firmware-interaction code, consult these in order:

| Source | URL / path | Auth | Use for |
|---|---|---|---|
| Swagger UI | `http://${UC3_HOST}/doc/core-rest/` | Anonymous (docs only) | Browsing endpoints by tag, reading schemas |
| OpenAPI spec | `http://${UC3_HOST}/doc/core-rest/openapi.yaml` | Anonymous | Programmatic search; save a local copy to `logs/core-openapi.yaml` (gitignored) |
| WS RPC enum | `src/core/enums.h::RequestTypes::Enum` | N/A (declarations) | Searching for wired or orphan WS operations |
| `core::Api` class | `src/core/core.{h,cpp}` | Existing WS auth | Existing wrappers — use these before adding new ones |

**Status snapshot (2026-05-02):** 137 endpoints across 15 tag categories — `activities`, `api-keys`, `auth`, `cfg`, `dock`, `entities`, `external-token`, `info`, `infrared`, `integrations`, `macros`, `profiles`, `remotes`, `resources`, `system`. OpenAPI 3.1.1. Title: "Remote Two/3 REST Core-API". Memo: `reference_uc_core_rest_api.md`.

### §13.2 REST vs WebSocket decision matrix

Once an operation exists in *both* REST and WS form, pick by the call's shape — not by which one the codebase uses today.

| Call shape | Prefer | Why |
|---|---|---|
| One-shot command (POST something, expect 200/201) | **REST** | Documented schema, version-stable, no field-shape footguns (AP-UC-47) |
| One-shot read (GET current state) | **REST** | Same as above. Don't subscribe to events for a value you only need once. |
| Long-lived push subscription (state changes you genuinely need pushed) | **WS** | Native bidirectional channel; REST has no equivalent. |
| Operation only present in `enums.h`, not in OpenAPI | **WS** | REST may not implement it. Verify with the discovery hierarchy. |
| Operation only present in OpenAPI, not in `enums.h` | **REST** | WS may not route the RPC name. The inhibitor API is the canonical example. |
| Operation present in both but you're unsure | Read the spec, then `git grep` `enums.h` for call sites. If WS has zero callers it's an orphan — see §13.5. | |

**Field-shape warning (AP-UC-47):** REST URL query parameters and WS RPC body fields are NOT interchangeable. The `power_mode` argument is `?power_mode=NORMAL` in REST URL convention but `{"mode": "NORMAL"}` as a WS RPC body field. Copying one shape to the other is the v1.4.22 silent failure that broke Mod 5 and Mod 6 across 8 releases. Memo: `project_setpowermode_field_bug.md`.

**Polling vs push (AP-UC-48):** if the firmware emits push events for what you want, use them. If it doesn't, polling REST is fine — that's how state endpoints (`/system/power`, `/system/power/battery`) are designed to work. Don't fabricate event subscriptions where push events don't exist.

### §13.3 Authentication

The firmware accepts three documented auth schemes. Today's empirical probe (2026-05-02) confirmed all three work for the standby-inhibitor endpoints:

| Scheme | Header / mechanism | When usable | Notes |
|---|---|---|---|
| **Basic auth** | `Authorization: Basic base64(web-configurator:PIN)` | Off-device tooling (curl, deploy scripts) | Documented in OpenAPI. Requires PIN — we have it for off-device probing but not for the on-device UI process. |
| **Cookie session** | POST `/api/pub/login` with PIN → `Set-Cookie: id=...` → reuse cookie | Web-configurator-style frontends | Documented. Same PIN dependency as basic auth. |
| **Bearer api_key** | `Authorization: Bearer <key>` | Long-lived programmatic clients | Mentioned in OpenAPI prose, NOT in formal `securitySchemes`. Mintable via `POST /auth/api_keys`. Existing keys on test device: `pyUnfoldedCircle`, `intg-manager`, `System Monitor Client 776`. |
| **Loopback (hypothesis)** | None? | On-device UI talking to `127.0.0.1:8080` | The OpenAPI server list includes `http://localhost:8080/api`. Whether loopback bypasses auth is **unknown without on-device testing**. |

**Auth ladder for new REST consumers:** any new REST helper added to `core::Api` (§6.8) should walk this ladder at first call and cache the result:

1. Try no auth (loopback exemption hypothesis). One-shot probe at startup.
2. Try the WS token from `UC_TOKEN_PATH` as a Bearer.
3. If both fail: log it, fail-open (behave as if the feature is disabled), surface a `qCWarning` so Logdy captures the auth gap.

**Anonymous endpoints** (no auth needed, useful as reachability probes): `/api/pub/version`, `/api/pub/status`, `/api/pub/health_check`, `/api/pub/login`. The OpenAPI spec marks these with `security: []` (empty array = override global security). Use `/api/pub/version` as a "is the device reachable?" check that doesn't burn an auth attempt.

### §13.4 Logdy is diagnostic, not telemetry (AP-UC-45)

Logdy is the WebSocket log stream baked into the firmware:

- **URL:** `ws://${UC3_HOST}/log/ws`
- **Auth:** anonymous
- **Probe scripts:** `test/probe_logdy.py`, `test/probe_logdy_persist.py`

**What it streams:** whatever the core service emits — typically connection state changes (dock reconnects, integration setup), errors, and a slice of system events.

**What it does NOT stream:**
- Battery capacity changes (use `GET /api/system/power/battery`)
- Routine power-mode transitions for every transition (use `GET /api/system/power`)
- Most internal state-machine ticks
- **The custom `remote-ui` process's logs.** Logdy `/log/ws` is filtered to the `core` service. Our `qCDebug` / `qCInfo` / `qCWarning` output does **not** stream regardless of level; log-level promotion has no effect here. For custom-UI debugging, capture wire frames manually (the §13.6 probe table) or instrument via REST endpoints. Memory: `feedback_uc3_systemlogs_core_only.md`.

**Empirical evidence:** a 60 s capture during idle on 2026-05-02 yielded 62 frames, almost all dock-reconnect retries. Zero battery events.

**Use Logdy for:** "what's the firmware doing right now?" investigations — when you don't yet know what events exist or what triggered an unexpected behavior. Tail the stream while reproducing the bug.

**Don't use Logdy for:** state polling, telemetry pipelines, drain measurement, anything where you need a known event at a known cadence. For state, REST is the answer. Memo: `feedback_uc3_systemlogs_core_only.md`.

### §13.5 Orphan-RequestTypes pattern

`enums.h::RequestTypes::Enum` declares every WS RPC the upstream UI knows about — but not all of them are wired. Entries with **zero call sites** in the codebase are "orphans" — RPC names declared but never sent.

**Why orphans matter:** they often indicate firmware capabilities the upstream UI never bothered to surface. Wiring an orphan is essentially free — the firmware-side handler already exists; we just call it.

**Track record:**
- v1.4.10 — `entityAdded` (signal orphan, related): pre-existing latent upstream bug fixed
- v1.4.12 — `REASSOCIATE` wired for the WiFi reconnect button (W4)
- v1.4.14 — `set_power_mode` wired for ActivitySessionKeeper (Mod 5) — *though this turned out to be the wrong API; see §13.2 — REST inhibitor was the right answer*
- v1.4.20 — wake-trigger gating (Mod 6 phantom-wake suppressor)

**How to find an orphan:**
1. Open `src/core/enums.h`, find an entry whose name describes what you want.
2. `git grep -n "<name>"` across `src/`. If it appears only in `enums.h` itself and the enum-string conversion table, it's an orphan.
3. Confirm the firmware-side handler exists (Swagger UI search, or send a test RPC and see if it returns OK or 4xx).

**Caution:** the orphan-RPC route is more brittle than REST because the WS RPC name is undocumented. Prefer §1.4.1 step 1 (REST) before falling back to step 3 (orphan WS). Memo: `project_orphan_request_types_pattern.md`.

### §13.6 Probing the device (no SSH by design)

UCR3 has no shell access. The only diagnostic surfaces are REST, WS (including Logdy), and physical interaction with the device. Memos: `feedback_never_say_ssh_uc3.md`, `project_uc3_no_ssh.md`.

**Probing workflow:**

| Goal | Method |
|---|---|
| "Is the device reachable?" | `curl http://${UC3_HOST}/api/pub/version` (anonymous, fast, doesn't trigger auth) |
| "Wake the device before probing other endpoints" | Hit `/api/system/power/battery` first — WoWLAN may not wake the chip in time for arbitrary endpoints (memo: `feedback_wowlan_phantom_wake.md`) |
| "What endpoint do I need?" | Browse Swagger UI at `/doc/core-rest/`. Search by tag. |
| "Does this endpoint actually do what the spec says?" | curl probe with basic auth (off-device) |
| "What is the firmware doing right now?" | `python test/probe_logdy.py 60 'keyword1,keyword2'` |
| "What WS RPCs is the UI sending?" | Logdy capture during the action you're investigating |

**Where probe captures live:** `logs/` directory in the project root. Never `/tmp/` (resolves to `AppData/Local/Temp` on Windows Python and breaks on session restart). Memo: `feedback_logs_directory.md`.

### §13.7 API version awareness (post-upstream-merge ritual)

After every `git fetch upstream && git merge upstream/main`, run this as part of the post-merge sanity check:

```bash
curl -s http://${UC3_HOST}/api/pub/version
```

The `api` field tracks the firmware REST API version. Today (2026-05-02): `api: "0.17.6"`, `core: "0.71.1-bt"`, `ui: "1.4.38"`, `os: "2.9.1"`. If the `api` value bumps after a firmware update:

- Re-download the OpenAPI spec to `logs/core-openapi.yaml` (overwrite previous).
- Diff against the prior version to spot new endpoints (potentially new orphans to wire) or schema changes (potential breaking changes for our REST consumers).
- Note the bump in CHANGELOG with a one-line summary of relevant changes.

**Cautionary note:** `/api/pub/version` `ui` field is firmware-cached and may stay at the OLD git-describe until device reboot — trust `/api/system/install/ui` plus visual verification for our own UI version. Memo: `project_pub_version_ui_field_staleness.md`.

---

## §14 COMMUNICATION STYLE

- Talk like Quark from DS9. Curse when it fits — for emphasis, frustration, or color.
- Be direct. Don't over-explain obvious things.
- **Default to plain language.** Lead with what changes and why it matters in real-world terms. API schemas, file paths, anti-pattern IDs, and code-level detail are for when the user is reviewing those artifacts — not the default shape of every reply. Tables and structured headings are for genuinely complex comparisons, not the default response.
- **Confirm scope before deep multi-step planning.** For PR scoping, refactors, multi-file architecture changes, style-guide overhauls, and similar multi-step work: ask one or two clarifying questions about scope/preference *before* producing the full structured plan. Thoroughness about content coverage is fine when planning is invited; jumping straight to a 400-word matrix is not.
- When reviewing, suggest concrete improvements with code.
- Edit files directly when filesystem access is available.
- Present options with trade-offs and let the user choose.
- **Explain as you go** — narrate reasoning in real time, not just in footnotes after 400 lines of C++. If you hit a surprise mid-generation, say so.
