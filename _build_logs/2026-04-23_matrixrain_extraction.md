# Build Log: matrixrain.cpp subsystem extraction (Mod 1 refactor)

**Date:** 2026-04-23 | **Mode:** BUILD (planning phase) | **Design doc:** TBD (this log + Plan agent output until a dedicated `MATRIXRAIN_EXTRACTION_PLAN.md` is produced)

---

## Origin

Triggered by codebase deep-scan audit (this session, 2026-04-23). Three parallel Explore agents (C++, QML, project hygiene) plus synthesis landed at **honest grade B** — held back primarily by:

- `src/ui/matrixrain.cpp` at **2055 lines** (4× §1.6 budget of ~500)
- `MatrixRainItem` carrying **150+ Q_PROPERTYs** (6× §1.6 budget of ~25)
- `updatePaintNode()` at **214 lines** (2.7× §1.6 budget of ~80)
- Helper functions oversized: `bindToScreensaverConfig()` 159 lines, `buildCombinedAtlas()` 110 lines

The class is correct, memory-safe, render-thread-clean — but it's a god-object that violates the project's own stated complexity budget. Audit consensus: *"A-grade engineering, B-grade architecture"*.

## Decision (2026-04-23)

User picked the top-5 punch list:

1. **Extract `matrixrain.cpp` into 3-4 units** ← THIS LOG
2. CI test runner (separate, not blocked by this)
3. Decompose `ChargingScreen.qml` (later session)
4. Purge legacy tarballs from git history (coordination decision, later)
5. `Palettes.qml` singleton (afternoon task, separate)

User confirmed: **plan #1, #3, #4 individually; do #2 and #5 as quick wins separately.** This log covers #1 only.

## Target architecture (sketch — Plan agent will refine)

Working hypothesis from the audit:

- `TapEffectController` — owns `tapBurst`, `tapRipple`, `tapWipe`, `tapSquareBurst`, `tapScramble`, `tapSpawn`, `tapMessage` and their timers/state.
- `MessageOverlayController` — owns whatever isn't already in `MessageEngine` (effect orchestration, wave timers, spawn gating).
- `AtlasBuilder` — owns `buildCombinedAtlas()` and the atlas-rebuild dirty-tracking. `MatrixRainItem` holds the `GlyphAtlas*` produced.
- `MatrixRainItem` keeps: GPU pipeline (`updatePaintNode()`, geometry, material), the simulation tick driver, displayOff gating, the render-thread boundary.

**Hard constraint:** zero observable behavior change. External API surface (Q_PROPERTYs read by QML, signals consumed by `ScreensaverConfig`/settings pages, QML callers like `MatrixTheme.qml`) **must not change** in this refactor. Pure internal restructuring.

## Plan agent brief sent

See parallel agent output. Awaiting architect plan with: extraction boundaries, ownership model, signal/property forwarding strategy, sequenced phases with git checkpoints, per-phase validation steps, .pro/.qrc/main.cpp registration deltas, risks.

## Pre-flight (per §3.3 — to be run BEFORE first edit)

- [ ] `git status` clean — resolve any uncommitted work from previous session
- [ ] Commit/stash anything dirty
- [ ] Tag pre-refactor commit: `git tag matrixrain-pre-extraction-2026-04-23`

Status: `.gitignore` modified (pre-existing, not from this session). Build log untracked. Awaiting user decision before commit/tag.

## Pre-refactor baseline (captured 2026-04-23 ~12:35, commit c9d8036)

Build: ARM64 cross-compile of current `main`, deployed to UC3 (192.168.2.204), HTTP 201, version 1.3.0-pre active. Layers enabled (multi-layer path = the more expensive cold case = best baseline to have).

| Run | Wall-clock (user stopwatch) | `ctorToPaintMs` | `cache` | `buildMs[far,mid,near]` | `composeMs` | `polishMs` |
|---|---|---|---|---|---|---|
| Cold (post-reboot) | ~1.52s | **566** | miss | [81, 76, 93] | 130 | 437 |
| Wake 1 (warm) | ~2.38s | **179** | hit | [0, 0, 0] | 0 | 0 |
| Wake 2 (warm) | <1s | **92** | hit | [0, 0, 0] | 0 | 0 |

**Phase 5 parity gate:** post-refactor `ctorToPaintMs` must land within ±10% of these values:
- Cold: target **510 – 622 ms**
- Wake 1: target **161 – 197 ms**
- Wake 2: target **83 – 101 ms**

**Notes on interpretation:**
- The wall-clock stopwatch numbers are NOT the parity metric — they include dock-detect, ChargingScreen Popup creation, PowerMode idle transition (none of which our refactor touches). The wake1 stopwatch (2.38s) being higher than cold (1.52s) is a tell that wall-clock is dominated by external timing, not renderer init.
- `ctorToPaintMs` is the ONLY metric the refactor can move. It measures `MatrixRainItem` ctor → first paint. Cache-miss cold path includes 3-layer atlas build + compose + remap + sync + render. Cache-hit warm path skips build + compose + remap.
- Cold `buildMs=[81,76,93]` ≈ 250ms total atlas build across 3 layers — this is the workload that lives in `LayerPipeline::build()` (and the `cacheKey` hashing + cache lookup that lives in `AtlasBuilder`) after refactor.
- Wake-2 (92ms) being faster than wake-1 (179ms) suggests warming of GPU texture upload paths and JIT effects in the runtime — both renderer-external. Two warm samples bracket the steady-state.

## Edit Log

| # | File | Change | Status |
|---|------|--------|--------|
| — | — | (no edits yet — planning phase) | — |

## Current State

- Audit complete (in conversation transcript).
- Top-5 punch list agreed with user.
- This build log opened.
- Plan agent dispatched (parallel with this write).
- Zero source files modified.

## Decisions Made

- **Refactor target = 3-4 units max**, not aggressive over-decomposition. Match the precedent set by existing extractions (`RainSimulation`, `GlitchEngine`, `MessageEngine`, `GravityDirection`) which proved you can split this code without losing render-thread correctness.
- **External API frozen for the duration.** Settings pages, `MatrixTheme.qml`, `ScreensaverConfig` bindings keep consuming `MatrixRainItem` exactly as today. Internal-only refactor.
- **Phased with build-green checkpoints.** No "extract everything then fix the build" — each phase compiles and visually previews in the Docker harness before moving on.
- **Visual preview is the regression gate.** Per `feedback_no_ssh_uc3.md` (no SSH on UC3) and `feedback_menu_changes_dev_preview.md` (preview before deploy), Docker preview at port 5909 plus a final on-device deploy is the validation chain. No QSGNode can change behavior without showing up in the preview.

## Outstanding

- Plan agent output (incoming).
- Decision on whether to land this on `main` directly with intermediate commits or use a `refactor/matrixrain-extraction` branch. Lean toward branch given the size, but waiting on Plan agent's phase count to decide.
- Decision on whether the new units live in `src/ui/` alongside `matrixrain.cpp` or in a `src/ui/matrixrain/` subdirectory. (Subdirectory cleaner; sets precedent for future mods.)

## Risks flagged so far

1. **Render-thread sync invariants.** `tick()` writes simulation state on main thread, `updatePaintNode()` reads it at sync point. New controllers must not introduce a second writer outside that boundary.
2. **Signal chain complexity.** Some Q_PROPERTYs use the dual-emit MOC workaround documented in `screensaverconfig.cpp:31-38`. Extracted properties must preserve that pattern or the bindings silently break.
3. **`bindToScreensaverConfig()` wires 159 lines of `connect()` calls.** Extraction must rewire these correctly — easy place for a property to silently stop updating.
4. **Atlas dirty-flag ownership.** Currently `m_atlasDirty` is a `MatrixRainItem` member. If `AtlasBuilder` owns the atlas, who owns the dirty flag? Race risk if both touch it.
5. **Tap effect state interleaving.** Some tap effects mutate the simulation grid directly (e.g., `tapBurst` writes characters, `tapWipe` clears columns). Extracted controller needs a clean handle to mutate `RainSimulation` state without becoming a friend class.
