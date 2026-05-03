# Core-API Audit — 2026-05-02

**Status:** research-only audit, no code changes. Output drives a multi-phase migration roadmap.

**Spec under review:** `logs/core-openapi.yaml` (gitignored, 602 KB) — OpenAPI 3.1.1, version 0.45.2, firmware 2.9.1. Re-pull anonymously from `http://${UC3_HOST}/doc/core-rest/openapi.yaml` if missing.

**Scope:** the three OpenAPI tag categories most likely to overlap with our custom mods — `system/*`, `cfg/*`, `auth/*` + `api-keys/*`. The other 12 categories (activities, profiles, entities, integrations, infrared, macros, remotes, resources, external-token, info, dock) are upstream-UI's territory and our mods don't touch them.

**Cross-referenced custom code:** `src/hardware/{activitySessionKeeper,phantomWakeSuppressor,wifi}.{h,cpp}`, `src/qml/settings/settings/{Power,Wifi,WifiInfo,WifiDiagnostics}.qml`, `src/qml/components/overlays/ReconnectingHUD.qml`, `src/ui/screensaverconfig.{h,cpp}` + macros, `src/ui/matrixrain.{h,cpp}` + helpers, `src/core/{core,enums}.{h,cpp}`, `src/config/config.{h,cpp}`.

---

## TL;DR

The audit covered ~58 in-scope endpoints across `system/*`, `cfg/*`, `auth/*`, `api-keys/*`. The single highest-value finding is the one we already knew about: **standby_inhibitors REST replaces Mod 5's 270 s ping race with an event-based contract**. Beyond that, almost everything that looked like a candidate is either already upstream-wired (most of `cfg/*` is plumbed by `Config::Q_PROPERTY` → WS RPC), or correctly WS-driven for the right reasons (Mod 3's WiFi diagnostics needs the live signal stream).

Three surprises shifted the plan:

1. **Phase 0 hypothesis ranking inverted.** The handoff guessed Bearer-via-`UC_TOKEN_PATH` was the most likely auth path. Spec verification today: Bearer is **advertised** in the intro (lines 38–43) but **not defined** in `securitySchemes` (lines 17296–17305). Only `basicAuth` and `cookieAuth` are formally declared, and global security is `basicAuth OR cookieAuth`. The empirically-verified `web-configurator:6984` Basic auth is now the leading hypothesis.
2. **The PIN is unobtainable without rotating it.** Source verification of `Config::getApiAccess`, `processApiAccess`, and `struct ApiAccess` confirms the firmware's `get_api_access` response only returns `{enabled, valid_to}` — no PIN field. The PIN is also not QSettings-backed, so it doesn't survive process restart. **The only way for the custom-ui to KNOW the PIN is to SET it.** This is a one-time destructive event that reorders the probe sequence (non-destructive H3/H2/H4 first; H1 with PIN regen last).
3. **`/cfg/power_saving.standby_sec=0` disables standby entirely.** Mod 5 doesn't read it before pinging, so if a user sets `standby_sec=0` (no standby ever), every ping is a wasted RPC. This is a CONFLICT, but it dissolves naturally if we ship M1 — inhibitors don't care what `standby_sec` is set to.

| Bucket | Count | Items |
|---|---|---|
| MIGRATE | 1 | M1 standby_inhibitors swap (Mod 5) |
| CONFLICT | 1 | C1 Mod 5 vs `cfg/power_saving.standby_sec=0` (auto-resolves with M1) |
| DEFER | 1 | D1 Mod 6 force-LOW_POWER REST swap (works on WS, no race to fix) |
| N/A | ~52 | Documented per category below |

**Mod 4 v2 is M1.** It is the first concrete code change after Phase 0 unblocks. Estimated 6–10 hours focused work. The rest is small follow-ons or hold-state.

**No CONFLICTS that block ship of v1.4.x.** Existing custom code coexists fine with the firmware's REST surface today; the firmware is just exposing more options than we've used.

**Four open questions resolved (2026-05-02):** Q1 (PIN access — no non-destructive path; probe handles it), Q2 (probe is local-only build, no tagged release), Q3 (M1 rollout strategy), Q5 (D1 permanently deferred). Full Q&A table at the bottom of this doc.

---

## Phase 0 — RESOLVED 2026-05-02 EOD: H2 wins (Bearer via UC_TOKEN content)

**Empirical outcome:** the probe ran on UCR3 (firmware 2.9.1) and short-circuited at H2. `Authorization: Bearer <UC_TOKEN_PATH content>` returns 200 OK on `GET http://127.0.0.1:8080/api/system/power`. **No PIN handling required for M1.**

The hypothesis ranking inversion below (Basic-with-PIN expected to win) was wrong about Bearer. The OpenAPI spec contradiction (Bearer mentioned in intro but absent from `securitySchemes`) reflects the spec — but empirically the firmware accepts Bearer regardless. File an upstream issue against `unfoldedcircle/core-api` to add Bearer to `securitySchemes`; low priority since our code uses the empirical contract.

| # | Hypothesis | Result |
|---|---|---|
| H3 | No-auth localhost | **401** — no localhost-trust pattern |
| H2 | Bearer via UC_TOKEN | **200** ✓ WINNER |
| H4/H1 | (token-as-PIN / regen PIN) | not reached (probe short-circuited at H2) |

**M1 implications:** build an `Authorization: Bearer <token>` helper on `core::Api` reusing the existing `UC_TOKEN_PATH` read at `core.cpp:79-90`. No QSettings PIN persistence, no PIN-rotation UI, no destructive first-enable side-effects. Cleanest possible outcome from Appendix A.

See `.claude-memory/project_uc3_rest_auth_mechanism.md` for the full empirical writeup including diagnostic trickery (Logdy WS does NOT capture remote-ui-custom logs in firmware 2.9.1; the probe used `Config::setDeviceName` as an externally-observable side-effect channel).

The detailed analysis below is preserved as historical context — Phase 0 implementation, hypothesis design, and the "PIN-access answer" subsection were correct in spirit but the destructive H1 path turned out to be unnecessary.

---

## Phase 0 — REST auth probe (original plan, preserved for context)

No (a) MIGRATE work proceeds without first answering: how does the custom-ui process running on the device authenticate to its own firmware's REST API? Today, `core::Api::authenticate()` (`core.cpp:69-101`) uses `UC_TOKEN_PATH` content as a WS RPC payload — it has never made a single authenticated REST call to the firmware. The four `QNetworkAccessManager` consumers (`voice.cpp`, `mediaPlayer.cpp` + headers) all hit external URLs (TTS audio, media artwork).

### Spec evidence (verified 2026-05-02)

- **Global security default** (`logs/core-openapi.yaml:71-73`): `basicAuth OR cookieAuth`. Bearer is **not** in this list.
- **Defined securitySchemes** (lines 17296–17305): only `basicAuth` (HTTP Basic) and `cookieAuth` (cookie name `id`, `apiKey` type). **Bearer is absent.**
- **Bearer mention in intro** (lines 38–43): `--header 'Authorization: Bearer $API_KEY'` example. This is documentation aspiration, not a declared security scheme. The spec contradicts itself.
- **Empirical (handoff session, 2026-05-02):** `web-configurator:6984` Basic auth → 200 OK on `/api/system/power/standby_inhibitors`. `madalone:hehehe` user account → 401 Unauthorized.

### Hypothesis ranking — non-destructive-first ordering

Compared to the original handoff memo, the ranking is fully reworked: the new ordering puts read-only probes first and the (one) destructive probe last. **The PIN is unobtainable without rotating it** — see "PIN-access answer" below.

| # | Hypothesis | Side effect | Likelihood | Why |
|---|---|---|---|---|
| **H3** | No-auth from `127.0.0.1` (localhost trust) | None | UNKNOWN | Spec says all non-`/api/pub` is secured, but localhost-trust patterns exist in some firmwares. Test first because it's free. |
| **H2** | Bearer via `UC_TOKEN_PATH` content | None | LOW | Bearer is not defined in `securitySchemes` (lines 17296–17305). Intro example at lines 38–43 contradicts the formal spec. Probably won't work but eliminates the option cheaply. |
| **H4** | Basic with `web-configurator:<UC_TOKEN content>` | None | VERY LOW | Free to test. PIN is a 4-digit value (per `WebConfig.qml:209-280` UI), token is much longer — almost certainly will fail, but eliminates a "what if they're the same secret?" question. |
| **H1** | Basic with regenerated PIN | **Destructive (one-time)** | **HIGH** likelihood | Empirically verified externally with `web-configurator:6984` → 200 OK on `/api/system/power/standby_inhibitors`. Run only if H3 + H2 + H4 all fail. |

### PIN-access answer (Q1 resolved 2026-05-02)

There is **no non-destructive path** for the custom-ui to obtain the firmware-side PIN. Source-verified:

| Step | File:Line | What |
|---|---|---|
| 1 | `config.h:451` | Default value `m_webConfiguratorPin = "••••"` — placeholder, not real |
| 2 | `core.cpp:1233-1234` `Api::getApiAccess()` | Sends `RequestTypes::get_api_access` |
| 3 | `core.cpp:2750-2760` `Api::processApiAccess()` | Response only contains `{enabled, valid_to}` — **no PIN field** |
| 4 | `structs.h:254-257` `struct ApiAccess` | Confirms — only `bool enabled` + `QDateTime validTo` |
| 5 | `config.cpp:866-883` `generateNewWebConfigPin()` | Generates fresh PIN locally, calls `setApiAccess(true, newPin)`, caches `m_webConfiguratorPin = webConfiguratorPin` on success |
| 6 | `config.cpp:948-983` `setWebConfiguratorEnabled(true)` | Same pattern — generate + setApiAccess + cache |
| 7 | (grep `m_settings->value` in config.cpp) | **PIN is NOT QSettings-backed.** Every other custom Q_PROPERTY uses `m_settings->value()`/`setValue()`; webConfiguratorPin does not. **Does not survive process restart.** |
| 8 | `WebConfig.qml:288` + `Finish.qml:221` | Only triggers for PIN regeneration — both user-initiated |

**Implication:** to use H1, the custom-ui must call `setApiAccess(true, newPin)` itself — which rotates the PIN, invalidating the user's known value (e.g., the empirical `6984`). This is destructive but bounded to one event per probe run.

### Probe design

Add `Q_INVOKABLE void Core::probeRestAuth()` debug method behind a `Config.probeRestAuth = true` flag. Fires hypotheses H3 → H2 → H4 → H1 sequentially against `GET /api/system/power`, logs each result via `qCInfo(lcCore)` for Logdy capture (`feedback_uc3_systemlogs_core_only.md` reminds us: `/api/system/logs` won't surface custom-ui entries; Logdy WS is the only way).

| Order | Hypothesis | Probe HTTP request | Success signal | Failure signal |
|---|---|---|---|---|
| 1 | H3 — localhost no-auth | `GET http://127.0.0.1/api/system/power` no Authorization header | 200 + `PowerModeResponse` JSON | 401 (no localhost-trust) |
| 2 | H2 — Bearer via UC_TOKEN | `GET /api/system/power` with `Authorization: Bearer <file content>` | 200 + JSON | 401 (Bearer not actually accepted) |
| 3 | H4 — Basic with token-as-PIN | `GET /api/system/power` with `Authorization: Basic base64("web-configurator:<UC_TOKEN content>")` | 200 + JSON | 401 (token isn't the PIN) |
| 4 | H1 — Basic with regen PIN | Call `Config::generateNewWebConfigPin()`, then `GET /api/system/power` with `Authorization: Basic base64("web-configurator:" + m_webConfiguratorPin)` | 200 + JSON; **PIN is now rotated, log the new value to chat for user record** | 401 (auth still wrong somehow) |

**Short-circuit rule:** if H3, H2, or H4 succeeds, **skip H1** — no need to rotate the PIN if a non-destructive path already works.

### Probe implementation outline

1. **Spec the probe** — done above.
2. **Implementation** — `Q_INVOKABLE void Core::probeRestAuth()` reads `UC_TOKEN_PATH` content via existing `authenticate()` plumbing (`core.cpp:80-85`), fires `QNetworkRequest`s in H3 → H2 → H4 order, short-circuits if any succeeds. Only if all three fail: call `Config::generateNewWebConfigPin()`, await success, then run H1. Each step logs at `qCInfo(lcCore)` with `[REST Auth Probe Hn]` prefix for Logdy filterability.
3. **Gate** — `Config.probeRestAuth` toggle, default `false`. Auto-runs at startup if true.
4. **Cadence** — local-only build per Q2 (no tagged release). Build → flash → capture → discard binary. Probe code stays in the working tree feature-gated.
5. **Capture** — Logdy WS catches the `[REST Auth Probe Hn]` lines.
6. **Document** — new memory `project_uc3_rest_auth_mechanism.md` records the answer + (if H1 fired) the new PIN value the user will need to know.
7. **Revert** — feature-gate the probe code (kept compilable, off by default) so we can re-run if a future firmware breaks this.

**Phase 0 is one local-only build of focused work, ~4–6 hours.** Everything below depends on it.

---

## (a) MIGRATE — 1 finding

### M1 — `/system/power/standby_inhibitors` swap for Mod 5 (Active Session Keeper)

**Affected files:** `src/hardware/activitySessionKeeper.{h,cpp}`, `src/core/core.{h,cpp}`, `src/config/config.{h,cpp}`, `src/qml/settings/settings/Power.qml`, `test/hardware/keeper_test/*`.

**Effort:** M (6–10 h focused, after Phase 0).

**Status:** highest-value finding in this audit. Already partially planned in handoff (`project_session_handoff_2026_05_02_eod.md` §"Mod 4 v2"). This audit confirms the plan and corrects one schema detail.

#### Endpoint contract

`POST /system/power/standby_inhibitors` — `logs/core-openapi.yaml:10692-10720`. Request body schema `CreateStandbyInhibitor` (`logs/core-openapi.yaml:16745-16767`):

```yaml
CreateStandbyInhibitor:
  type: object
  properties:
    id: string (1-64 chars, optional — auto-generated if omitted)
    who: string (1-64 chars, REQUIRED — descriptive client identifier)
    why: string (0-64 chars, optional — descriptive reason)
    delay: integer (≥1 seconds, optional — temporary inhibitor; OMIT for blocking)
  required: [who]
```

**Schema correction (audit-trust caveat triggered):** the parallel system agent claimed a `type: temporary | blocking` enum field. **That field does not exist.** The temporary/blocking distinction is implicit: **presence of `delay` → temporary** (auto-removes after delay + idle); **absence of `delay` → blocking** (persists until client `DELETE`). This matters for our migration plan — we want blocking semantics (explicit lifecycle, matches WS ping mental model), so we omit `delay`.

Response: `201 Created` with `{ id: string }`. `409 Conflict` if an inhibitor with the same client-supplied `id` already exists.

`GET /system/power/standby_inhibitors` — list active inhibitors (line 10666). Returns `Inhibitors` schema. Useful for: (a) state sync on reconnect to detect orphaned inhibitors from a previous crashed session; (b) the `PowerModeResponse.standby_inhibitors: bool` flag (`logs/core-openapi.yaml:16653-16657`) tells us whether *any* inhibitors are active without enumerating them.

`DELETE /system/power/standby_inhibitors/{id}` (line 10737) — single removal. `DELETE /system/power/standby_inhibitors` (line 10721) — remove all (we won't use this; too destructive — could nuke other clients' inhibitors).

#### Current custom code

`activitySessionKeeper.cpp:136-146`:

```cpp
void ActivitySessionKeeper::ping() {
    qCDebug(lcHw()) << "ActivitySessionKeeper ping → set_power_mode(NORMAL)";
    int id = m_core->setPowerMode(core::PowerEnums::PowerMode::NORMAL);
    m_core->onResult(id, [](){}, [](int code, QString message) {
        qCWarning(lcHw()) << "ActivitySessionKeeper ping failed:" << code << message;
    });
}
```

Called from `evaluateSession()` (line 116) via repeating 270 s `m_pingTimer` (configured `activitySessionKeeper.cpp:28`). The ping resets the firmware's standby countdown to its configured `standby_sec` (typically 300) on each LOW_POWER/IDLE → NORMAL transition. **The race window:** if activity stops 260 s into the cycle, only 40 s remain until standby fires — and the next ping isn't due for 10 more seconds. With WoWLAN-induced phantom wakes and multi-second WS RTTs, this margin has nuked sessions in the wild (the empirical reason Mod 5 exists).

#### Migration sketch

1. **Add REST capability** to `core::Api`. New method `int Api::createStandbyInhibitor(const QString& who, const QString& why, int delaySec = -1)` returning a request ID. Internally fires `POST /api/system/power/standby_inhibitors` with body `{who, why, [delay]}`. Companion: `int Api::deleteStandbyInhibitor(const QString& id)` → `DELETE /api/system/power/standby_inhibitors/{id}`.
2. **State machine in keeper unchanged.** `evaluateSession()` (line 116) keeps its current "active = enabled && (onAc || !requireAc) && (mediaPlaying || idleTimer.isActive())" logic. Only the *effector* changes:
   - Was-inactive → now-active edge: call `createStandbyInhibitor("madalone.session-keeper", "media-playing or recent-input", /* no delay = blocking */)`. Store the returned `id` in `m_inhibitorId`.
   - Was-active → now-inactive edge: call `deleteStandbyInhibitor(m_inhibitorId)`. Clear the field.
   - Drop `m_pingTimer` entirely.
3. **Crash safety.** On `core::Api::disconnected` (already wired at line 33), we lose our `m_inhibitorId` reference but the firmware-side inhibitor lives on indefinitely (blocking type). On reconnect, call `GET /system/power/standby_inhibitors`, find any with `who == "madalone.session-keeper"`, delete them. Fresh start. This handles UI crashes, restart-on-deploy, OTA updates.
4. **Config toggle for safe rollout.** New `Q_PROPERTY(bool sessionKeeperUseInhibitorApi)` defaulting `false` (per Q3 — user-controlled opt-in, no auto-flip in a later release). User opts in via Settings → Power → "Use REST inhibitor API (experimental)". The toggle stays user-driven indefinitely; the WS ping path remains the default. **PIN handling:** if Phase 0 lands on H1 (PIN regen needed), enabling the toggle for the first time triggers a one-time `Config::generateNewWebConfigPin()` call. The new PIN is shown in the existing Settings → WebConfig page (already bound to `m_webConfiguratorPin`). Persist the PIN in QSettings (`power/m1RestPin`) so it survives reboots without re-rotating.
5. **Tests.** `test/hardware/keeper_test/*` already has the WS-side test scaffolding (49 methods per `project_path_to_a_post_v1_4_35.md`). Add 5 new tests for the REST path with mocked `QNetworkAccessManager` (or a thin `IRestClient` adapter for testability). At minimum: create-on-active, delete-on-inactive, orphan-cleanup-on-reconnect, 401-fallback-to-WS, 409-on-duplicate.

#### Risk callouts

- **Phase 0 dependency.** Cannot start until auth path is empirically known. If H1 (Basic with regenerated PIN) wins, the keeper triggers PIN regen + QSettings persistence on first toggle-enable (per Q1 resolution + step #4 above). If H3 (localhost no-auth) wins, no PIN handling needed at all.
- **Inhibitor orphaning.** Crash → blocking inhibitor sticks until reboot. Mitigation #3 above (cleanup on reconnect) handles the common case. The pathological case (UI crashes mid-handoff between disconnect and reconnect, then a *different* process re-creates an inhibitor) is bounded — worst case the user notices the device staying awake and reboots once.
- **Fallback strategy.** If REST auth flakes mid-session (unlikely but possible — token rotation, firmware glitch), do we fall back to the WS ping path? **Recommend: no.** Hard-fail the inhibitor call, log loudly, and let the firmware's normal standby fire. Silent fallback to the WS path masks the auth failure that the probe was supposed to surface.
- **The `standby_sec=0` edge case dissolves.** Today, if a user has `standby_sec=0` (no standby), Mod 5's pings are wasted no-ops. With M1, the inhibitor is a no-op too (nothing to inhibit), but it doesn't fire any RPCs — so zero waste. Free win.
- **Mod 6 unaffected.** Mod 6 uses `setPowerMode(LOW_POWER)` to *force* standby on phantom wakes. That direction has no inhibitor analog; spec verification confirmed `PUT /system/power?power_mode=LOW_POWER` exists (lines 10575–10595, accepting all 4 modes including LOW_POWER) but there's no race to fix on Mod 6 — see D1 below.

#### Dependencies

- **Blocked by:** Phase 0 auth probe.
- **Blocks:** any future REST-using mod — all such mods will reuse the auth foundation M1 establishes.
- **Resolves:** C1 (the standby_sec=0 race).

---

## (b) CONFLICT — 1 finding

### C1 — Mod 5 race with `cfg/power_saving.standby_sec=0` semantics

**Affected files:** `src/hardware/activitySessionKeeper.cpp`.

**Effort:** S (≤2 h if we choose to fix standalone) or **0 h** (auto-resolves with M1).

#### The conflict

`/cfg/power_saving` schema (`logs/core-openapi.yaml:15166-15187`):

```yaml
CfgPowerSaving:
  properties:
    wakeup_sensitivity: integer (0-3)
    display_off_sec: integer (0-60)
    standby_sec: integer (0-10800, "0 disables standby mode")
```

The user-facing wording for `standby_sec` says "0 disables standby". Today, `activitySessionKeeper.cpp:136` fires `setPowerMode(NORMAL)` every 270 s while a session is active, regardless of what `standby_sec` is set to. If standby is disabled, the ping is a no-op — wasted bandwidth and wakeup of the firmware-side handler for nothing.

This is a soft conflict (no user-visible bug, no broken behavior), but it's an ergonomic miss: power users who set `standby_sec=0` to force always-on still pay for Mod 5's heartbeat.

#### Two paths

**(a) Defensive guard** — add `int Api::getPowerSavingCfg()` read on Mod 5 startup; if `standby_sec == 0`, suppress the ping loop. Listen for `cfgPowerSavingChanged` to re-evaluate. Cost: ~20 LOC, 1 unit test, ~2 h. **Pure no-op except in the corner case.**

**(b) Ship M1 instead** — inhibitor-based keeper is silent when there's nothing to inhibit. C1 dissolves with no specific code addition. Cost: 0 incremental over M1.

#### Recommendation

**Skip (a). Wait for M1.** The standalone fix is cheap but adds branching logic to a code path we're about to rewrite. If M1 slips for any reason, revisit (a) as a one-release polish.

---

## (c) DEFER — 1 finding

### D1 — Mod 6 force-LOW_POWER could move to REST, but no win

**Affected files:** `src/hardware/phantomWakeSuppressor.cpp:167`, `src/core/core.{h,cpp}`.

**Effort:** S (3–4 h to swap the effector + tests), but **no functional benefit**.

#### What changed in this audit

The handoff stated "no inverse 'force standby' REST API exists." Spec verification today: **`PUT /system/power?power_mode=LOW_POWER` does exist** (lines 10575–10595), accepting all four modes (NORMAL, IDLE, LOW_POWER, SUSPEND). The handoff was wrong on this specific claim.

So D1 is technically a MIGRATE candidate. But:

#### Why it's still DEFER

- Mod 5 had a structural problem (poll-vs-event race). Mod 6 doesn't — `setPowerMode(LOW_POWER)` is a fire-and-forget single call after a 500 ms grace timer, fundamentally event-driven already.
- WS plumbing for `setPowerMode` is already in `core::Api::setPowerMode()` (`core.h:197`) and known-working since v1.4.22 fixed the field-shape bug (`project_setpowermode_field_bug.md`). REST migration is pure transport churn.
- The same auth-path dependency (Phase 0) gates D1, but the engineering payoff is zero. Switching for the sake of "consistency with M1" is the wrong reason.

**Decision: leave Mod 6 on WS. Re-evaluate only if a future firmware deprecates the WS RPC.**

---

## (d) N/A — Documented per category

### `system/*` (24 endpoints)

| Endpoint | N/A reason |
|---|---|
| `/system` GET | System info read; no custom mod consumer. Upstream `getSystemInfo()` covers any future need. |
| `/system` POST | STANDBY/REBOOT/POWER_OFF/RESTART commands; admin-driven, custom mods have no need. |
| `/system/backup/*` (4 endpoints) | Backup export/restore/snapshots; admin workflow. |
| `/system/bt` PUT/DELETE | Bluetooth radio control; no custom-mod consumer. |
| `/system/factory_reset` GET/POST | Factory reset token + execution; admin-only. |
| `/system/install`, `/install/{component}` | Custom-component install/enable/remove (this is HOW our binary gets onto the device); admin-driven, not runtime custom-code. |
| `/system/logs/boots`, `/logs/services`, `/logs/hci`, `/logs/web` | Log metadata + HCI trace + Logdy config; admin-only or covered by Logdy WS. |
| `/system/power/battery`, `/power/charger` | Battery/charger status + wireless charging; no custom-mod consumer (Mod 5 reads `Battery::powerSupplyChanged` from the existing C++ singleton, not a REST endpoint). |
| `/system/update`, `/update/{id}` | OTA update check/force; admin-driven. |
| `/system/wifi`, `/wifi/scan`, `/wifi/networks`, `/wifi/networks/{id}` (~10 endpoints) | Mod 3 covers all of these via WS. **Switching to REST is pure transport churn — no race to fix, polling cadence (30 s) is correct as-is, RSSI history needs the live WS stream anyway.** The system agent classified these as MIGRATE; that was overreach. Downgraded to N/A. If a future requirement lands (e.g., async WiFi config from a non-UI path), revisit. |

### `cfg/*` (15 endpoints)

| Endpoint | N/A reason |
|---|---|
| `/cfg`, `/cfg/device`, `/cfg/device/*` (4 endpoints) | Already upstream-wired via `Config::deviceName` etc. |
| `/cfg/display` | `Config::displayBrightness`, `displayAutoBrightness` (Config.h:51-54) already wired. |
| `/cfg/sound` | `Config::soundEnabled`, `soundVolume` (Config.h:48-49) already wired. |
| `/cfg/voice_control`, `/voice_control/voice_assistants` | `Config::voiceAssistantId`, `voiceAssistantProfileId`, `voiceAssistantSpeechResponse`, `micEnabled` (Config.h:42-46) already wired. |
| `/cfg/network`, `/cfg/network/wifi` | `Config::wifiEnabled`, `wowlanEnabled`, `wifiBands`, `wifiBand`, `scanIntervalSec` (Config.h:74-78) already wired. |
| `/cfg/localization`, `/localization/*` (4 endpoints) | `Config::language`, `country`, `timezone`, `clock24h`, `unitSystem` (Config.h:29-37) already wired. Country/tz lookups are metadata reads. |
| `/cfg/profile` | Admin PIN check; no custom-mod consumer. |
| `/cfg/software_update` | OTA window/channel/auto-update; upstream-managed. |
| `/cfg/features` | Read-only feature flags; informational, no custom consumer today. |
| `/cfg/bt`, `/cfg/bt/profiles*` | Bluetooth settings + paired-device profiles; firmware-managed, no custom mod. |
| `/cfg/entity/commands` | Entity command metadata; informational. |
| `/cfg/haptic` | The cfg agent flagged this as "NEW MOD candidate (haptic intensity)" but the schema only exposes `enabled: bool`. No intensity, waveform, or per-zone fields. **Confirmed N/A.** Reopen only if a future firmware version extends the schema. |

### `auth/*` + `api-keys/*` (~19 endpoints)

| Endpoint | N/A reason |
|---|---|
| `/auth/api_keys` (HEAD/GET/POST/DELETE) + `/api_keys/{id}` (GET/PATCH/DELETE) | API-key lifecycle; admin-only feature, custom-ui doesn't surface API-key management to end users. |
| `/auth/scopes` GET | Lists scopes for API keys; informational. Worth reading once if Phase 0 lands on Bearer auth, otherwise irrelevant. |
| `/auth/callback` GET | OAuth2 redirect handler for external integrations; not relevant to custom-ui ↔ firmware auth. |
| `/auth/external` family (8 endpoints) | External-system credential storage (HA tokens, Spotify OAuth, etc.); for integration drivers, not for the custom-ui. |
| `/pub/login`, `/pub/logout` | Cookie-session creation; H3 fallback only if H1+H2 both fail. Document but don't plan around. |

---

## Roadmap

```
┌─ Phase 0 ──────────────────────────────────────────────────────────┐
│  REST auth probe                              S    blocking         │
│  Local-only build (Q2 — no tagged release)    ~4–6 h focused        │
│  H3 → H2 → H4 → H1 sequence; H1 destructive   one-time PIN rotation │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─ Phase 1 ──────────────────────────────────────────────────────────┐
│  M1 — standby_inhibitors swap (Mod 5)         M    ~6–10 h          │
│  Ships in v1.4.39 (toggle default ON after empirical verification)  │
│  C1 auto-resolves with M1 (no separate code).                       │
└─────────────────────────────────────────────────────────────────────┘

Permanently DEFER: D1 (Mod 6 stays on WS).
```

### Sequencing rationale

- **Phase 0 → Phase 1** is sequential because Phase 0 establishes the REST auth helper that M1 reuses. Trying to ship M1 in parallel with the probe would mean either two passes through `core::Api`'s REST layer or speculative implementation.
- **No phase blocks v1.4.x maintenance work.** A11Y manual pass (open per `project_path_to_a_post_v1_4_35.md`), upstream merges, bug fixes proceed in parallel.

---

## Open questions — RESOLVED 2026-05-02

| # | Question | Answer |
|---|---|---|
| Q1 | PIN access at runtime | **No non-destructive path exists.** Verified from source — `getApiAccess` returns only `{enabled, valid_to}`, PIN is not QSettings-backed, no env-var path. The probe handles this by trying H3/H2/H4 (non-destructive) first; H1 (PIN regen) runs only as a last resort. M1 ships behind a default-off opt-in toggle so any PIN rotation is user-controlled. See "PIN-access answer" subsection above. |
| Q2 | Probe release cadence | **Local-only build.** No tagged release. Build → flash → capture → discard binary. Probe code stays in the working tree feature-gated for re-runs. |
| Q3 | M1 rollout strategy | **Shipped behind `Config.sessionKeeperUseInhibitorApi` toggle.** Initial default OFF for M1 implementation; flipped ON in a follow-up commit (`25a2050`) after Phase 0 + 4 lifecycle smokes confirmed end-to-end behavior. Toggle remains user-controlled for fallback. |
| Q5 | D1 — defer Mod 6 indefinitely | **Agreed.** Permanent DEFER. No follow-up audit checkpoint planned — re-evaluate only if a future firmware deprecates the WS `setPowerMode` RPC. |

---

## Appendix A — Auth probe outcomes parking lot

Outcome tree for the H3 → H2 → H4 → H1 probe sequence:

- **H3 passes** → REST is "trusted localhost only." Cleanest possible answer — no PIN handling, no token reuse. M1 ships using direct localhost REST calls. Document the limitation: any future REST consumer must run on the device itself.
- **H2 passes** → Bearer-via-UC_TOKEN works. Reuse `core.cpp:80-85` token reading. M1 builds an `Authorization: Bearer ...` helper on `core::Api` and uses it for inhibitor calls. **Spec is wrong** about Bearer not being supported — file an upstream issue against `unfoldedcircle/core-api`.
- **H4 passes** → Surprise discovery — token IS the PIN. H1 strategy auto-resolves (no rotation needed); use the same token for both WS auth and REST Basic. This would be the cleanest possible outcome but is the least likely.
- **H3 + H2 + H4 all fail, H1 passes** → Basic-with-regenerated-PIN works. Document the new PIN. M1 strategy: rotate PIN once on first toggle-enable, persist in QSettings (`power/m1RestPin`), use for all subsequent inhibitor calls. The existing `WebConfig.qml` page surfaces the PIN to the user automatically since it's bound to `m_webConfiguratorPin`.
- **All four fail** → Audit hits a hard wall. Options: (a) ask UC team directly via GitHub Issues / Discord with the probe Logdy capture as evidence, (b) reverse-engineer the upstream web-configurator's REST auth flow (it must work somehow — it's a web app talking to the same API), (c) abandon the audit's MIGRATE bucket; M1 stays on WS, defer indefinitely as "wait for UC to publish proper auth docs."

The probe code SHOULD short-circuit (skip later hypotheses if an earlier one succeeds, especially the destructive H1) BUT log each attempted-or-skipped hypothesis explicitly so Logdy gives a complete picture for future re-runs.

## Appendix B — What this audit did NOT cover

Out of scope per the handoff's category-skip rules:

- `activities/*` — upstream's domain (activity sequences, IR macros, button mappings)
- `profiles/*` — user profiles, admin PIN gating
- `entities/*` — HA-style entity model; we have custom mods (Mod 3 WiFi diagnostics is "an entity" arguably) but they don't *write* the entity API, they consume it
- `integrations/*` + `infrared/*` + `macros/*` + `remotes/*` + `resources/*` — UC's integration / IR / macro / remote / resource subsystems
- `external-token/*` — covered briefly in `auth/*` audit; same conclusion (integration-driver-only)
- `info/*` — `/api/pub/version` etc.; we use `/api/pub/version` already (see `project_pub_version_ui_field_staleness.md` — that field has a known cache bug)
- `dock/*` — pairing-station management

If a future audit wants to revisit one of these, the same four-bucket framework applies. Recommend doing one category per audit; this audit covered three because of the unusual Phase-0-blocker dependency.

---

*Audit conducted 2026-05-02. Spec version 0.45.2 (firmware 2.9.1). Custom code state: post-v1.4.38 + post-`b08a4ba` UCR3 artifact rename. Three parallel Explore agents + one synthesis pass + one verification pass against the spec to catch agent overreach (`feedback_verify_audit_before_remediation.md` triggered twice — system agent inflated `CreateStandbyInhibitor` schema and the WiFi MIGRATE bucket).*
