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

These upstream files carry our changes and will conflict, or need a look, on every upstream merge. The exact list
for the current tree is one command away (`M` = upstream file we modify, `A` = our file, `D` = upstream file we removed):

```shell
git fetch upstream refs/tags/vX.Y.Z:refs/upstream-public/vX.Y.Z   # the upstream release we are on
git diff --name-status refs/upstream-public/vX.Y.Z HEAD
```

As of v1.6.0 (upstream v0.84.0): 62 upstream files modified, 3 removed (`.github/dependabot.yml`,
`.github/workflows/openspec-version.yml`, the `CLAUDE.md` symlink), the rest of the tree is upstream's or ours.

| Area | Upstream files | What we keep | Conflict risk |
|---|---|---|---|
| Build | `remote-ui.pro`, `resources/qrc/main.qrc`, `.gitignore` | Our sources and QML in HEADERS / SOURCES / qrc; `VERSION` in sync with `deploy/release.json`; extra warning flags; ignore entries (incl. `CLAUDE.md`) | High |
| CI | `.github/workflows/build.yml` | Version-sync + CHANGELOG gates, UCR3 install-bundle staging (every file of `deploy/config/`, no sub-dirs), GPG signing, digest-pinned 5.15.19 toolchain (upstream uses the same image unpinned) | Medium |
| Start-up | `src/main.cpp` | MatrixRain / Palettes registration; `ScreensaverConfig` built after `hwController` and before `uiController`; keeper + suppressor wiring; `DevDockToggle` in DEV | High |
| Settings store | `src/config/config.{h,cpp}` | Our Q_PROPERTYs: phantom wake, keep awake, dock chime, media buttons (v1.4.8), volume overlay, WiFi chip, ... | High |
| Core API | `src/core/core.{h,cpp}`, `src/core/structs.h` | `setPowerMode` (Mod 6, body field `mode`), REST helpers + standby-inhibitor calls (Mod 5), `Inhibitor` struct. Keep our declarations above upstream's `private:` | High |
| Hardware | `src/hardware/hardwareController.{h,cpp}`, `src/system/battery.h`, `src/system/wifi.{h,cpp}` | Keeper / suppressor construction and wiring; `Battery::instance()`; WiFi W1 (WoWLAN shown), W2 link fields, W4 reassociate, W6 30 s poll, W13 diagnostics | High: upstream reworks Wi-Fi often |
| Entities | `src/ui/entity/entityController.{h,cpp}`, `src/ui/entity/mediaPlayer.{h,cpp}` | Mod 5 hook in `connectLazySignals` (only players a screen asked for), `entityCommandIssued`, `LOW_POWER` in the wake flag; `hide_volume_overlay`; browse-thumbnail preview on the artwork pipeline | **High: conflicted on every merge so far.** Upstream keeps reworking wake/standby and artwork |
| Input, sound, UI | `src/ui/inputController.{h,cpp}`, `src/ui/soundEffects.{h,cpp}`, `src/ui/uiController.{h,cpp}`, `src/logging.{h,cpp}` | `touchDetected` (Mod 6); bundled sounds, dock-chime variants, `UC_SOUND_EFFECTS_PATH` fallback; EntityController accessor; `lcScreensaver` | Medium |
| Main window | `src/qml/main.qml`, `src/qml/components/ChargingScreen.qml` | Screensaver loader flags, boot grace, dock chime, docked rearm, `hostContainer` binding; the screensaver popup is a full replacement | Very high |
| Settings pages | `settings/Settings.qml`, `settings/settings/{Power,Sound,Ui,Wifi,WifiInfo}.qml`, `onboarding/Wifi.qml` | Screensaver menu entry; our setting rows and chip pickers; W10 scan gate on display off; Reconnect / Diagnostics buttons | High: upstream restyles settings |
| Components | `Slider.qml`, `Switch.qml`, `StatusBar.qml`, `Page.qml`, `LoadingFirst.qml`, `VolumeOverlay.qml`, `TouchSlider{Volume,Seek,Brightness,Position}.qml`, `entities/TitleStatus.qml`, `entities/activity/MediaComponent.qml`, `entities/media_player/MediaBrowser.qml`, `media_player/deviceclass/*.qml` | Tick gate (v1.4.33), DPAD centre toggles a switch, W3 signal bar, volume feature guards, DEV skips the splash, volume overlay toggles, screensaver touch-bar guards, title-row mirrors + WiFi chip, browse plumbing + MUTE/STOP/NEXT/PREV/CHANNEL keys, v1.4.8 media-button toggles | Medium |
| Other | `resources/icons/icon-fallback.json`, `tools/design-check-allow.txt`, `README.md`, `CONTRIBUTING.md`, `CHANGELOG.md`, `test/**`, `docs/**`, `resources/translations/en_US.ts` | `wave-pulse` fallback; screensaver artwork on the allow-list; fork docs; our test harness and suites | Low |

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

### When upstream has rewritten its history

Upstream rewrote its history at v0.74.5: none of its commits after that point descend from the commit our previous
merge used (v0.73.5, `3b46959`), so `git merge-base` falls back to v0.38.4 and a merge reports thousands of conflicts.
What worked for v0.84.0 (v1.6.0):

```shell
# 1. Find upstream's rewritten counterpart of our previous base: the same release subject in the new history.
#    For v0.84.0: 4077183 "v0.73.5 changes" (parent bc72137, the rewritten v0.73.4); our previous base is the
#    old 3b46959 "v0.73.5 changes". The trees differ: the rewrite changed files as well.
git log --format="%h %p %s" refs/upstream-public/v0.84.0 | grep "v0.73.5 changes"
# 2. Graft our previous base onto it as an extra parent, so the merge base becomes our previous base
git replace --graft 4077183 bc72137 3b46959     # <counterpart> <its own parent> <our previous base>
git merge-base HEAD refs/upstream-public/v0.84.0  # must print 3b46959 (without the graft: f06f0f0, v0.38.4)
#    Upstream's changes between its v0.73.5 and the new release then arrive as one ordinary merge; differences the
#    rewrite made inside v0.73.5 itself show up as changes too, so review the conflicts with that in mind.
# 3. Merge, resolve, commit
git merge refs/upstream-public/v0.84.0
# 4. Delete the graft right after the merge commit; it must not stay in the repository
git replace -d 4077183
```

After the merge commit the merge base with upstream is upstream's own commit (`781882a` for v0.84.0), so the next
merge needs no graft, unless upstream rewrites its history again.

## Validation checklist

After the merge, before merging the rehearsal branch back into `main`:

- [ ] **ARM64 build** with the digest-pinned toolchain (`BUILD.md`): 0 errors; the QML compiler runs on every file
      (delete the `qrc` caches of an incremental build first)
- [ ] **Tests**, all in a Qt 5.15 container (`test.yml` has the commands): `test/hardware` (keeper, suppressor,
      entity controller, power), `test/matrixrain`, `test/integration`, `test/qml`, and upstream's CMake suite in `test/`
- [ ] **Code guidelines** as CI runs them: `python3 tools/design-check.py --self-test && python3 tools/design-check.py`,
      `python3 tools/icon-font.py check --require-free`, `python3 tools/icon-font.py check-mapping`, cpplint
      (filters in `code_guidelines.yml`); the docs link check (`docs-links.yml`)
- [ ] **Desktop simulator** (`UC_MODEL=DEV`, `docs/static-compile.md`): every settings page walked with the keys,
      top to bottom and back; QML log free of binding loops and TypeErrors from our files
- [ ] **On the remote** via `scripts/deploy-canary.sh` (auto-revert on failure):
  - screensaver: dock + chime, BACK/HOME/tap close it, also when the UI starts on the dock; all five themes render
  - settings: Screensaver, Power, Sound, User interface, WiFi pages walked with the d-pad
  - first button press after a wake works (no backlight blinking); keep awake holds during playback
  - media browser: play an item, its artwork shows at once
- [ ] **Translations**: `en_US.ts` updated with lupdate; the other `.ts` files keep upstream's text (ADR 0006)

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

Current fork base: **upstream `v0.84.0`** (`781882a`, the UI of firmware 2.12.0), merged in v1.6.0. Upstream tags
are fetched into `refs/upstream-public/` (the `upstream` remote fetches branches only). Next upstream release bump
requires walking through this doc end-to-end.

## Rehearsal history

Log each rehearsal here so "last known merge state" is always at a glance.

### 2026-10-31 — v0.84.0 merge (UI of firmware 2.12.0) → shipped as v1.6.0

- **Trigger:** the owner's remote runs firmware 2.12.0 with UI v0.84.0; the fork was on v0.73.5 (v1.5.2) plus five
  backports (v1.5.3).
- **History rewrite:** upstream rewrote its history at v0.74.5; merged with a temporary graft (see "When upstream has
  rewritten its history"). Merge commit `94c4006` (parents: our v1.5.4 `6649035`, upstream v0.84.0 `781882a`).
- **Conflicts:** resolved file by file from four conflict maps (C++ platform, entity layer, settings / WiFi QML,
  main / build / icons). Kept: every mod. Dropped as superseded or dead: Wake-replay HUD (owner: UC's retry
  spinners), our battery chip (UC's `TitleStatus`), WiFi W5/W9 (upstream fixed the causes), our WebSocket keep-alive
  (upstream's spec forbids it), the REST auth probe (it logged the PIN), `migrateLegacySettings()` (never worked).
  Adopted upstream's power-mode contract (an unchanged mode is not announced again).
- **After the merge:** our screens moved onto upstream's design system (screensaver settings, WiFi diagnostics,
  shared `ChipPicker`); browse-thumbnail preview re-implemented on the 0.74 artwork pipeline; keep awake follows only
  players a screen asked for (since 0.74 every configured entity is loaded at start-up); `MainContainer.qml` back to
  upstream (our start-up retry timer took the buttons from the screensaver).
- **Validation:** 0 errors on the ARM64 build; all suites pass (hardware 106, matrix rain 106, integration 41, QML 212,
  upstream 31 programs); design check 0 problems; desktop simulator walks of every theme; on-device check by the owner.

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
