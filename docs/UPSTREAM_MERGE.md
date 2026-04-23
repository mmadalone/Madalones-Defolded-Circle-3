# Upstream merge playbook

This fork of [unfoldedcircle/remote-ui](https://github.com/unfoldedcircle/remote-ui) adds custom screensaver mods on top of the upstream UC codebase. The `upstream` remote points at the UC repo; merges from upstream are manual and require conflict resolution on the files listed below. This doc is the reference for how to do those merges without breaking the custom work.

## Remote setup

Run once on a fresh clone:

```shell
git remote add upstream https://github.com/unfoldedcircle/remote-ui.git
git fetch upstream
```

Verify with `git remote -v` — you should see both `origin` (this fork) and `upstream` (UC's repo).

## Upstream files we've modified (conflict surface)

These files are touched by our custom code and will likely conflict on any upstream merge. Each one has custom changes that **must be preserved** during conflict resolution.

| File | What we added | Conflict risk |
|---|---|---|
| `src/main.cpp` | `qmlRegisterType<MatrixRainItem>`, `qmlRegisterUncreatableType<GravityDirection>`, `ScreensaverConfig` singleton instantiation, any custom registrations | High |
| `src/config/config.h` / `.cpp` | Screensaver properties removed (migrated to `ScreensaverConfig`). Redirect-comment stub remains in `config.h`. | High |
| `remote-ui.pro` | Added custom `HEADERS`/`SOURCES` (matrixrain, screensaverconfig, rainsimulation, gravitydirection, glitchengine, messageengine, glyphatlas); custom `VERSION` line kept in sync with `deploy/release.json`; `QMAKE_CXXFLAGS_WARN_ON` additions (`-Wall -Wextra -Werror=format`, plus the `-Wold-style-cast` family) | High |
| `resources/qrc/main.qrc` | Registered all custom QML files and settings sub-pages | High |
| `src/qml/main.qml` | `screensaverActive` property; idle timer DEV mode bypass; `ScreensaverConfig` import; `_nextOpenViaIdleTimer` one-shot flag; custom `Battery`/`Power` handlers for `motionToClose`, `_shouldOpenOnIdle`, Loader `onStatusChanged` flag propagation | Very High |
| `src/qml/components/ChargingScreen.qml` | Full replacement — theme Loader system, `ButtonNavigation`, screen-off countdown poller, `cancelScreenOffEffect(isWakeFromOff)`, Battery `Connections` for undock baseline reset, belt-and-suspenders scene-graph `update()` on wake, `_openedViaIdleTimer` flag, `postAnimationSafetyTimer` | Very High |
| `src/qml/settings/settings/ChargingScreen.qml` | Full replacement — screensaver theme selector + sub-pages (GeneralBehavior, MatrixAppearance, MatrixEffects, Chaos, Tap, DirectionGlitch, Message, Starfield, Minimal, TvStatic, Analog, Common toggles) | Very High |
| `src/qml/settings/Settings.qml` | Added "Screensaver" menu entry | Medium |
| `src/qml/components/Switch.qml` | Added `Keys.onReturnPressed`/`onEnterPressed` for DPAD center toggle | Low |
| `src/qml/components/TouchSliderVolume.qml` / `Seek.qml` / `Brightness.qml` / `Position.qml` | Added `applicationWindow.screensaverActive` guard (suppress touchbar during screensaver) | Low |
| `src/qml/components/overlays/BatteryOverlay.qml` | Color-coded battery levels, fully translatable text via `qsTr`/`%1` placeholders | Medium (if upstream renames) |
| `.github/workflows/build.yml` | Added `version-sync-check` job with VERSION + CHANGELOG gating; added `needs: version-sync-check` to `build-embedded` and `build-desktop` | Medium |
| `.gitignore` | Added custom entries (build artifacts, `.env.local`, `CLAUDE.md`) | Low |
| `deploy/release.json` | Custom `name`, `description`, `developer`, `version` | Low |

For the authoritative custom-file manifest (purely custom, not modified upstream), see [CUSTOM_FILES.md](CUSTOM_FILES.md).

## Merge rehearsal procedure

Never merge upstream directly into `main`. Always rehearse first:

```shell
git fetch upstream
git checkout -b rehearsal/upstream-$(date +%Y-%m-%d)
git merge upstream/main
```

### When conflicts hit

For **every** file in the conflict-surface table above:

1. **Upstream side (theirs):** new logic, bug fixes, API changes — preserve.
2. **Our side (ours):** all custom screensaver code — preserve.
3. **Mutual-exclusion cases** (e.g. upstream renamed a property we're using): inspect carefully, migrate our usage to the new API, update CUSTOM_FILES.md entry if the file path changed.
4. **Never reformat upstream code** in the resolve. Match upstream's indentation/style inside their blocks even if it disagrees with our `.clang-format`. Minimize the diff against upstream.

### When a file in the table doesn't conflict

Still inspect it — upstream may have touched nearby code that subtly breaks our custom logic without triggering a textual conflict. Example: upstream changes a signal signature, our custom handler keeps compiling but silently ignores the new argument.

### New upstream files we haven't touched

Pass through cleanly — no action needed.

## Validation checklist

After the merge, before merging the rehearsal branch back into `main`:

- [ ] **Build (macOS dev):** `qmake && make` clean, no new warnings
- [ ] **Build (ARM64 cross-compile):** `docker run --rm --user=$(id -u):$(id -g) -v "$(pwd)":/sources unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest` clean, binary produced
- [ ] **Docker VNC preview** renders all 5 themes (Matrix, Starfield, Minimal, Analog, TvStatic) without crashes
- [ ] **Deploy to test UC3** via `curl` using `.env.local` credentials
- [ ] **Bug 1 regression:** "Close on wake" toggle OFF → undock → screensaver stays open
- [ ] **Bug 1 regression:** "Close on wake" toggle ON → undock → screensaver closes
- [ ] **Bug 2 regression:** dock → wait full `displayTimeout` + screen-off anim + blank → wake → Matrix rain re-renders instantly (no stuck black)
- [ ] **Bug 2b regression:** undock with toggle OFF → animation fires at ≈ `displayTimeout` after undock, not earlier; blackout-to-display-off gap < 2s
- [ ] **Bug 3 regression:** "Idle screensaver" OFF → wait past `idleTimeout` undocked → screensaver stays closed
- [ ] **DPAD/touch direction regression:** rain bends smoothly via gravity lerp, no full respawn
- [ ] **Thermal regression:** 10+ min sustained dock with Matrix theme, device does not warm noticeably
- [ ] **Tests:** `cd test/matrixrain && qmake && make && ./test_matrixrain.app/Contents/MacOS/test_matrixrain` passes
- [ ] **Settings round-trip:** every screensaver settings slider/toggle persists across popup recreation (dock/undock cycle)

## Rollback procedure

### Rollback the merge locally (before pushing)

```shell
git checkout main
git branch -D rehearsal/upstream-<date>
# merge never happened — main is untouched
```

### Rollback after pushing to `origin/main`

```shell
# Find the last known-good commit (before the merge)
git log --oneline

# Hard reset main
git checkout main
git reset --hard <last-good-sha>

# Force-push — ONLY if no one else has pulled the bad merge
git push --force-with-lease origin main
```

**Do not** force-push main without first confirming nothing downstream has pulled the bad merge. `--force-with-lease` protects against overwriting unknown concurrent pushes but doesn't protect against collaborators who already pulled.

### Rollback on a deployed UC3

If the bad build is already on a device:

```shell
source .env.local
curl -X PUT "http://${UC3_HOST}/api/system/install/ui?enable=false" \
    -u "${UC3_USER}:${UC3_PIN}"
```

This disables our custom UI and the device reverts to the stock UC firmware — full backstop.

## Maintenance rules

Update this doc whenever:
- A new upstream file is modified for a new mod (add row to the table)
- A custom file is deleted or replaced (remove/update row)
- The validation checklist changes (new regression tests, new features)
- The rollback procedure changes (new API endpoint, new branch structure)

Keep the conflict-surface table in sync with [CUSTOM_FILES.md](CUSTOM_FILES.md) — that file lists all custom files; this doc lists only the subset that overlap with upstream and will conflict on merge.

## Known upstream base

Current fork base: **`v0.71.1`** (tracked via the `upstream` remote). Next upstream release bump requires walking through this doc end-to-end.

## Rehearsal history

Log each rehearsal here so "last known merge state" is always at a glance.

### 2026-04-23 — v0.72.0 merge (first real upstream-advance under this playbook) → shipped as v1.4.0

- **Trigger:** upstream `unfoldedcircle/remote-ui` advanced `main` with commit `c76ff05` (subject: `v0.72.0 changes` — UC no longer git-tags releases; version lives in commit subject only).
- **Fetch result:** `git fetch upstream --tags` — 1 new commit on `upstream/main` since the 2026-04-13 rehearsal.
- **Divergence (pre-merge):**
  - `git rev-list --count HEAD..upstream/main` → **1 commit behind upstream**
  - `git rev-list --count upstream/main..HEAD` → **135 commits ahead of upstream**
  - Merge base: `0586d45` (our old upstream pin = `v0.71.1`)
- **Upstream scope:** 12 files, +433/-93 lines. Three functional changes: (a) media browse press-and-hold + new shuffle/repeat/browse/sources controls row in `MediaComponent.qml`, (b) smarter media-browse error handling in `MediaBrowser.qml`, (c) **UC independently shipped the same feature as our Mod 3** ("Show battery indicator everywhere") with different property name + layout approach.
- **CI red-herring:** upstream's own aarch64 CI for `c76ff05` failed — investigation revealed it was a GitHub Actions deprecation (`actions/upload-artifact@v3` sunset), not a code defect. Our workflows already use `@v4`. Source compiles cleanly on our Docker toolchain.
- **Conflict surface:** 5 files with real conflicts (`config.h`, `config.cpp`, `BaseTitle.qml`, `Activity.qml deviceclass`, `Ui.qml`, `en_US.ts`). `config.h` auto-merged with both our `showBatteryOnDetailPages` AND upstream's `showBatteryEveryWhere` coexisting — required manual removal of our leftover decls.
- **Resolution strategy (Option B rebase):** adopt upstream's public API (`showBatteryEveryWhere`, QSettings key `ui/batteryEveryWhere`, upstream's Settings toggle wording) while keeping our superior Option A chain-anchoring `RowLayout` in `BaseDetail.qml`. Reject upstream's inline battery `Row` in `BaseTitle.qml` / `Activity.qml` (would duplicate the chip we already render via `BatteryStatusChip.qml` Loader). Accept-theirs on MediaComponent, MediaBrowser, SelectWidget, SensorWidget, icons, translations.
- **QSettings migration:** one-shot `migrateLegacySettings()` helper added to `src/main.cpp` — carries legacy `ui/batteryOnDetailPages` value (v1.3.0 default `true`) forward into `ui/batteryEveryWhere` (upstream default `false`). Ensures v1.3.0 users who accepted the default chip-visible state don't silently lose it on upgrade.
- **Icons:** initial Explore-agent read flagged that upstream removed `uc:heat` / `uc:brightness` / `uc:list` / `uc:bluetooth` / `uc:battery-low`. Independent verification showed this was a dedup — upstream's JSON had duplicate keys across primary + aliases sections; only the aliases-section copies were removed. All 5 icon names still resolve post-merge. Two icons (`uc:list` / `uc:heat`) get different glyphs (visual change only).
- **Branch workflow:** merged on `rehearsal/upstream-2026-04-23`; rollback tag `pre-v0.72.0-merge-2026-04-23` set on main pre-merge. Promoted via `git checkout main && git merge --no-ff rehearsal/upstream-2026-04-23` + `git tag v1.4.0` + `git push origin main v1.4.0`.
- **Validation checklist outcome:** canary deploy via `scripts/deploy-canary.sh` healthy at elapsed 0s. On-device smoke test: media browser works, battery chip works (confirming Mod 3 Option B rebase + migration), screensaver works, no errors in Logdy or `/api/system/logs`. About-screen UI version display shows stale `0.38.4-32-g1266974` instead of `1.4.0` — pre-existing upstream quirk unrelated to this merge (binary correctly compiled with `DAPP_VERSION=\"1.4.0\"`, but `softwareUpdate.h`'s inline `getUiVersion()` apparently isn't the source the screen reads from — out-of-scope investigation).
- **Divergence (post-merge):** 135 commits ahead (our custom work preserved) + the merge commit itself. Next upstream-advance will restart the counter.
- **Follow-up:** `v1.4.1` shipped same day — fixed 7 unguarded `volume.start()` call sites upstream (unblocks per-device `suppress_volume_overlay` toggles in integration drivers). See `_build_logs/2026-04-24_v1.4.1_osd_guards.md` for that log.
- **Conclusion:** first successful non-trivial upstream merge under this playbook. Option B rebase pattern established for future merges where upstream and our fork collide on feature semantics.

### 2026-04-13 — safety re-check before broader distribution

- **Trigger:** Maintainer asked "should we contrast our build with upstream before anyone else's UC3 installs it?" — quick re-check to confirm upstream hasn't moved since the Batch G rehearsal earlier in the day.
- **Fetch result:** `git fetch upstream` — no new commits on `upstream/main` since the earlier rehearsal.
- **Divergence:**
  - `git rev-list --count HEAD..upstream/main` → **0 commits behind upstream** (unchanged)
  - `git rev-list --count upstream/main..HEAD` → **91 commits ahead of upstream** (+9 from this session's batch 0 fixes, avatar cleanup, button-lockout fix, doc refreshes, CI tidy fix, test determinism fix)
  - `git merge-base HEAD upstream/main` → `0586d45b3ee7a04d2f1a15d9e4b2606c24d7ae08` (unchanged)
  - Upstream `HEAD` commit: `0586d45 v0.71.1 changes` (unchanged since the earlier rehearsal)
- **Conclusion:** Fork is still a strict superset of `upstream/main`. No new signal renames, no new upstream bug fixes missing from our tree. Safe to distribute from the "upstream deltas" angle. Real blast-radius concerns for non-maintainer devices (firmware/hardware revision mismatch, untested code paths) are orthogonal to this check and are documented in the new Install warnings in `README.md` and `SCREENSAVER-README.md`.

### 2026-04-13 — Batch G #17 initial rehearsal

- **Trigger:** Batch G #17 of the path-to-A plan (first rehearsal under this playbook).
- **Fetch result:** `git fetch upstream` — no new commits on `upstream/main` since our fork point.
- **Divergence:**
  - `git rev-list --count HEAD..upstream/main` → **0 commits behind upstream**
  - `git rev-list --count upstream/main..HEAD` → **82 commits ahead of upstream**
  - `git merge-base HEAD upstream/main` → `0586d45b3ee7a04d2f1a15d9e4b2606c24d7ae08`
  - Upstream `HEAD` commit: `0586d45 v0.71.1 changes` (same commit as the merge-base)
- **Upstream tag state:** Latest upstream tag is `v0.38.4` (`f06f0f0`), which lags the upstream `main` commit. Upstream's `main` commit message claims "v0.71.1 changes" but they haven't pushed a `v0.71.1` tag yet — a mild inconsistency in the upstream repo, not our problem.
- **Dry merge:** `git merge --no-commit --no-ff upstream/main` on a throwaway `rehearsal/upstream-2026-04-13` branch → **"Already up to date."** — zero conflicts, zero files touched.
- **Branch cleanup:** rehearsal branch deleted cleanly (`git branch -D rehearsal/upstream-2026-04-13`).
- **Validation checklist outcome:** not run — there's nothing to validate when the merge is a no-op. When the next rehearsal has real upstream commits, work through the Validation checklist section above.
- **Conclusion:** Our fork is a strict superset of `upstream/main`. Next rehearsal should happen after upstream advances `main`; watch the upstream repo or their release cadence for triggering events.

### Template for future entries

```
### YYYY-MM-DD — <trigger description>

- **Trigger:** <why we ran the rehearsal>
- **Fetch result:** `git fetch upstream` — <N new commits / no change>
- **Divergence:** HEAD is <N commits behind> / <M commits ahead>
- **Conflicts:** <N files / none>
  - <file 1>: <conflict type + resolution>
  - <file 2>: ...
- **Dry merge / resolution:** <what happened>
- **Validation checklist outcome:** <pass / fail + notes>
- **Conclusion:** <merged to main / held / rolled back>
```
