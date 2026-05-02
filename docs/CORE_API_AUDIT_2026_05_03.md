# Core-API Audit — 2026-05-03

**Status:** research-only audit, no code changes. Output drives a multi-phase migration roadmap.

**Spec under review:** `logs/core-openapi.yaml` (gitignored, 602 KB) — OpenAPI 3.1.1, version 0.45.2, firmware 2.9.1. Re-pull anonymously from `http://${UC3_HOST}/doc/core-rest/openapi.yaml` if missing.

**Scope:** the three OpenAPI tag categories most likely to overlap with our custom mods — `system/*`, `cfg/*`, `auth/*` + `api-keys/*`. The other 12 categories (activities, profiles, entities, integrations, infrared, macros, remotes, resources, external-token, info, dock) are upstream-UI's territory and our mods don't touch them.

**Cross-referenced custom code:** `src/hardware/{activitySessionKeeper,phantomWakeSuppressor,wifi}.{h,cpp}`, `src/qml/settings/settings/{Power,Wifi,WifiInfo,WifiDiagnostics}.qml`, `src/qml/components/overlays/ReconnectingHUD.qml`, `src/ui/screensaverconfig.{h,cpp}` + macros, `src/ui/matrixrain.{h,cpp}` + helpers, `src/core/{core,enums}.{h,cpp}`, `src/config/config.{h,cpp}`.

---

## TL;DR

The audit covered ~58 in-scope endpoints across `system/*`, `cfg/*`, `auth/*`, `api-keys/*`. The single highest-value finding is the one we already knew about: **standby_inhibitors REST replaces Mod 5's 270 s ping race with an event-based contract**. Beyond that, almost everything that looked like a candidate is either already upstream-wired (most of `cfg/*` is plumbed by `Config::Q_PROPERTY` → WS RPC), or correctly WS-driven for the right reasons (Mod 3's WiFi diagnostics needs the live signal stream).

Two surprises shifted the plan:

1. **Phase 0 hypothesis ranking inverted.** The handoff guessed Bearer-via-`UC_TOKEN_PATH` was the most likely auth path. Spec verification today: Bearer is **advertised** in the intro (lines 38–43) but **not defined** in `securitySchemes` (lines 17296–17305). Only `basicAuth` and `cookieAuth` are formally declared, and global security is `basicAuth OR cookieAuth`. The empirically-verified `web-configurator:6984` Basic auth is now the leading hypothesis. The probe stays — but the success criterion order flips.
2. **`/cfg/power_saving.standby_sec=0` disables standby entirely.** Mod 5 doesn't read it before pinging, so if a user sets `standby_sec=0` (no standby ever), every ping is a wasted RPC. This is a CONFLICT, but it dissolves naturally if we ship M1 — inhibitors don't care what `standby_sec` is set to.

| Bucket | Count | Items |
|---|---|---|
| MIGRATE | 1 | M1 standby_inhibitors swap (Mod 5) |
| CONFLICT | 1 | C1 Mod 5 vs `cfg/power_saving.standby_sec=0` (auto-resolves with M1) |
| NEW MOD | 3 | N1 ambient-light adaptation (med), N2 log shipper (low), N3 button LED color (cosmetic) |
| DEFER | 1 | D1 Mod 6 force-LOW_POWER REST swap (works on WS, no race to fix) |
| N/A | ~52 | Documented per category below |

**Mod 4 v2 is M1.** It is the first concrete code change after Phase 0 unblocks. Estimated 6–10 hours focused work. The rest is small follow-ons or hold-state.

**No CONFLICTS that block ship of v1.4.x.** Existing custom code coexists fine with the firmware's REST surface today; the firmware is just exposing more options than we've used.

---

## Phase 0 — REST auth probe (mandatory blocker)

No (a) MIGRATE work proceeds without first answering: how does the custom-ui process running on the device authenticate to its own firmware's REST API? Today, `core::Api::authenticate()` (`core.cpp:69-101`) uses `UC_TOKEN_PATH` content as a WS RPC payload — it has never made a single authenticated REST call to the firmware. The four `QNetworkAccessManager` consumers (`voice.cpp`, `mediaPlayer.cpp` + headers) all hit external URLs (TTS audio, media artwork).

### Spec evidence (verified 2026-05-03)

- **Global security default** (`logs/core-openapi.yaml:71-73`): `basicAuth OR cookieAuth`. Bearer is **not** in this list.
- **Defined securitySchemes** (lines 17296–17305): only `basicAuth` (HTTP Basic) and `cookieAuth` (cookie name `id`, `apiKey` type). **Bearer is absent.**
- **Bearer mention in intro** (lines 38–43): `--header 'Authorization: Bearer $API_KEY'` example. This is documentation aspiration, not a declared security scheme. The spec contradicts itself.
- **Empirical (handoff session, 2026-05-02):** `web-configurator:6984` Basic auth → 200 OK on `/api/system/power/standby_inhibitors`. `madalone:hehehe` user account → 401 Unauthorized.

### Inverted hypothesis ranking

Compared to the handoff memo:

| # | Hypothesis | Was | Now | Why the change |
|---|---|---|---|---|
| **H1** | Basic auth with PIN (`web-configurator:<PIN>`) | "burdensome" / unlikely | **HIGH** likelihood | Empirically verified externally; spec confirms `basicAuth` is a defined scheme. |
| **H2** | Bearer via `UC_TOKEN_PATH` content | "high likelihood" | LOW likelihood | Bearer is not defined in `securitySchemes`. Intro mention is a doc bug. |
| **H3** | Unauth from `127.0.0.1` (localhost trust) | possible | UNKNOWN | Spec says all non-`/api/pub` endpoints are secured, but special-case localhost paths exist in some firmwares; cheap to test. |

### Probe design

Add `Q_INVOKABLE void Core::probeRestAuth()` debug method behind a `Config.probeRestAuth = true` flag. Fires three test `GET /api/system/power` calls in sequence at startup, logs each result via `qCInfo(lcCore)` for Logdy capture (`feedback_uc3_systemlogs_core_only.md` reminds us: `/api/system/logs` won't surface custom-ui entries; Logdy WS is the only way).

| # | Hypothesis | Probe HTTP request | Success signal | Failure signal |
|---|---|---|---|---|
| 1 | Basic with PIN | `GET /api/system/power` with `Authorization: Basic base64(web-configurator:<PIN>)` | 200 + `PowerModeResponse` JSON | 401 (wrong PIN format) / 400 (encoding) |
| 2 | Bearer via UC_TOKEN_PATH | `GET /api/system/power` with `Authorization: Bearer <file content>` | 200 + JSON | 401 (Bearer not actually accepted) |
| 3 | No auth from 127.0.0.1 | `GET http://127.0.0.1/api/system/power` no Authorization header | 200 + JSON | 401 (spec wins, no localhost-trust) |

### Probe implementation outline

1. **Spec the probe** — done above.
2. **Implementation** — `Q_INVOKABLE void Core::probeRestAuth()` reads PIN from wherever it lives (TBD — open question Q1 below), reads `UC_TOKEN_PATH` content via existing `authenticate()` plumbing, fires 3 sequential `QNetworkRequest`s with detailed logging.
3. **Gate** — `Config.probeRestAuth` toggle, default `false`. Set to `true` for one probe-only release; auto-runs at startup; revert in next release.
4. **Capture** — Logdy WS catches `qCInfo(lcCore) << "REST Auth Probe #N..."` lines.
5. **Document** — new memory `project_uc3_rest_auth_mechanism.md` records the answer.
6. **Revert** — feature-gate the probe code (kept compilable, off by default) so we can re-run if a future firmware breaks this.

### Critical open question — where does the custom-ui get the PIN?

`Config::getWebConfiguratorPin()` returns `"••••"` per the handoff — that's a UI placeholder, not the real PIN. The real PIN is generated by the firmware and shown on screen during initial setup; the UI displays it but doesn't necessarily store it in a process-readable location. Possible answers: (a) a config file we haven't found yet, (b) the same `UC_TOKEN_PATH`-like mechanism but for the PIN, (c) the PIN is the WS auth token itself (if so, H2 effectively *is* H1 with different framing). The probe must answer this before H1 can be implemented.

**Phase 0 is one release of focused work, ~4–6 hours.** Everything below depends on it.

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
4. **Config toggle for safe rollout.** New `Q_PROPERTY(bool sessionKeeperUseInhibitorApi)` defaulting `false`. v1.4.39 ships dormant; user opts in via Settings → Power → "Use REST inhibitor API (experimental)". Flip default after a 2-week soak window.
5. **Tests.** `test/hardware/keeper_test/*` already has the WS-side test scaffolding (49 methods per `project_path_to_a_post_v1_4_35.md`). Add 5 new tests for the REST path with mocked `QNetworkAccessManager` (or a thin `IRestClient` adapter for testability). At minimum: create-on-active, delete-on-inactive, orphan-cleanup-on-reconnect, 401-fallback-to-WS, 409-on-duplicate.

#### Risk callouts

- **Phase 0 dependency.** Cannot start until auth path is empirically known. If H1 (Basic with PIN) wins, we need a way for the keeper code to access the PIN at runtime — likely a new helper on `core::Api` that `probeRestAuth` also exposes.
- **Inhibitor orphaning.** Crash → blocking inhibitor sticks until reboot. Mitigation #3 above (cleanup on reconnect) handles the common case. The pathological case (UI crashes mid-handoff between disconnect and reconnect, then a *different* process re-creates an inhibitor) is bounded — worst case the user notices the device staying awake and reboots once.
- **Fallback strategy.** If REST auth flakes mid-session (unlikely but possible — token rotation, firmware glitch), do we fall back to the WS ping path? **Recommend: no.** Hard-fail the inhibitor call, log loudly, and let the firmware's normal standby fire. Silent fallback to the WS path masks the auth failure that the probe was supposed to surface.
- **The `standby_sec=0` edge case dissolves.** Today, if a user has `standby_sec=0` (no standby), Mod 5's pings are wasted no-ops. With M1, the inhibitor is a no-op too (nothing to inhibit), but it doesn't fire any RPCs — so zero waste. Free win.
- **Mod 6 unaffected.** Mod 6 uses `setPowerMode(LOW_POWER)` to *force* standby on phantom wakes. That direction has no inhibitor analog; spec verification confirmed `PUT /system/power?power_mode=LOW_POWER` exists (lines 10575–10595, accepting all 4 modes including LOW_POWER) but there's no race to fix on Mod 6 — see D1 below.

#### Dependencies

- **Blocked by:** Phase 0 auth probe.
- **Blocks:** N1 (ambient-light adaptation), N2 (log shipper), and any other REST-using mod — they all need the same auth foundation that M1 establishes.
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

## (c) NEW MOD — 3 findings

### N1 — Ambient-light adaptation for screensaver brightness (medium priority)

**Affected files:** new mod or extension of Mod 1 (matrixrain / ChargingScreen). `src/qml/components/ChargingScreen.qml`, `src/ui/screensaverconfig_macros.h`, possibly `src/ui/matrixrain.cpp`.

**Effort:** M (8–12 h for a polished feature with config UI).

#### Endpoint

`GET /system/sensors/ambient_light` (`logs/core-openapi.yaml:10761-10778`). Returns `AmbientLight` schema (`logs/core-openapi.yaml:16768`+ — intensity field, integer 0-65535 per the handoff's earlier note).

#### Sketch

Poll the sensor at low cadence (~30 s) when screensaver is active. Map intensity → screensaver brightness multiplier. Dim Matrix rain glyphs in dark rooms; brighten them in bright rooms. Battery + eye-strain win for any UCR3 used in a bedroom/living-room context where ambient light varies.

Settings UI: new "Adaptive brightness" toggle in Settings → Screensaver → General Behavior, alongside the existing v1.4.15 "Run after dismissal while docked" slider.

#### Risk callouts

- **Phase 0 dependency.** REST poll needs auth.
- **Sensor availability.** UCR3 has the sensor (handoff confirms via spec); UCR2 may or may not. Schema/feature-detection needed before exposing the toggle.
- **Don't over-tune.** Map intensity → multiplier with hysteresis to avoid flicker as someone walks past the sensor. Probably a 5-second EWMA on the intensity value before applying.
- **Compete with `cfg/display.auto_brightness`?** Display brightness is upstream-managed. Our adaptation should target only the screensaver layer; let the firmware own the LCD backlight.

#### Dependencies

- **Blocked by:** Phase 0.
- **Blocks:** none.

---

### N2 — Log shipper / on-device diagnostic capture (low priority)

**Affected files:** new mod entirely; settings page in `src/qml/settings/settings/`; new `src/hardware/logShipper.{h,cpp}` candidate.

**Effort:** M (8–12 h for a privacy-aware MVP).

#### Endpoint

`GET /system/logs` (`logs/core-openapi.yaml:10333-10430`). Query-based log retrieval with filters: priority (`p`, 0-8, default 5), services (`s`, CSV), `from`/`to` (ISO 8601), text search (`q`), `boot_ids`. Returns JSON array of `SystemLogEntry` or text (tab-separated) up to 10K entries.

#### Sketch

User-triggered (Settings → Diagnostics → "Capture logs and email me a link") rather than always-on telemetry. On trigger: pull last N minutes of `core` + `web` + `ui` logs, compress, POST to a user-configurable destination (their own gist/pastebin/Sentry). Include a context header (firmware version, mod versions, recent settings).

#### Why this is lower priority than it looks

Per `feedback_uc3_systemlogs_core_only.md`, `/api/system/logs` filters by service and **never shows our custom-ui entries** at any priority level. So the practical value is capturing *firmware* logs around an issue, not capturing our own qCDebug output (Logdy WS is the only path for ours). That makes N2 a *firmware diagnostic* aid, not a *custom-mod diagnostic* aid. Useful when reporting a firmware bug to UC; less useful for our own debugging where Logdy already covers the gap.

#### Risk callouts

- **Privacy.** System logs may contain Wi-Fi SSIDs, integration tokens, paired devices. Require explicit opt-in per destination, redact sensitive substrings before shipping.
- **Phase 0 dependency.** REST auth needed.
- **Rate limiting.** Cap at 1 capture per 10 minutes; exponential back-off on 429.
- **Boot-ID semantics.** A capture spanning a reboot needs explicit `boot_ids=` to include both. UX-wise, default to current-boot-only.

#### Dependencies

- **Blocked by:** Phase 0.
- **Blocks:** none.

---

### N3 — Button LED static_color picker (cosmetic, lowest priority)

**Affected files:** `src/config/config.{h,cpp}` (new Q_PROPERTY); new QML color picker, likely as a sub-page of Settings → UI.

**Effort:** S (~6 h if firmware feature-detection works cleanly, M if device-class gating is finicky).

#### Endpoint

`/cfg/button` (`logs/core-openapi.yaml:7210-7269`). Schema includes `static_color` field — RGB or preset, plus `features` array including `RGB_COLOR` and `ZONES`. We expose `buttonBrightness` and `buttonAutoBrightness` (Config.h:56-58) but never `static_color`.

#### Why low priority

Pure cosmetic. No user has asked. Backwards-compatible with all device classes (gate the picker on `features.includes("RGB_COLOR")`). Defer until a user request lands; document it here so we don't re-discover it.

#### Risk callouts

- **Schema definition for `StaticButtonColor`** is referenced but not pasted into this audit — needs verification (hex string vs preset enum) before wiring.
- **No conflict with brightness.** Spec lists them as independent; our existing brightness slider keeps working.

---

## (d) DEFER — 1 finding

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

## (e) N/A — Documented per category

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
│  v1.4.39 (or whatever the next release is)    ~4–6 h focused        │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─ Phase 1 ──────────────────────────────────────────────────────────┐
│  M1 — standby_inhibitors swap (Mod 5)         M    ~6–10 h          │
│  v1.4.40 (feature-flagged off)                                       │
│  v1.4.4N (default-on after 2-week soak)                              │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─ Phase 2 (parallelizable) ─────────────────────────────────────────┐
│  N1 — ambient-light adaptation                M    ~8–12 h          │
│  N2 — log shipper (low priority)              M    ~8–12 h          │
│  N3 — button color picker (cosmetic)          S    ~6 h             │
│  C1 — auto-resolved by M1 (no code)           0 h                   │
└─────────────────────────────────────────────────────────────────────┘

Permanently DEFER: D1 (Mod 6 stays on WS).
```

### Sequencing rationale

- **Phase 0 → Phase 1 → Phase 2** is sequential because Phase 0 establishes the REST auth helper that all subsequent phases reuse. Trying to ship M1 in parallel with the probe would mean either two passes through `core::Api`'s REST layer or speculative implementation.
- **Phase 2 items are independent** of each other and can ship in any order based on user demand. N3 is so cosmetic it should only ship if requested.
- **No phase blocks v1.4.x maintenance work.** A11Y manual pass (open per `project_path_to_a_post_v1_4_35.md`), upstream merges, bug fixes proceed in parallel.

---

## Open questions for the user

(Concrete enough to answer in one sentence each.)

1. **PIN access at runtime.** Where does the custom-ui process actually have access to the web-configurator PIN? Is it stored in a config file, derivable from `UC_TOKEN_PATH`, or do we need to add a path for the user to enter it once at first run? This blocks the Phase 0 probe implementation.
2. **Probe release cadence.** Should the Phase 0 probe binary ship as a tagged release (v1.4.39-probe) that you deploy + capture + revert, or as an unreleased local build you flash, capture from, and discard? The latter avoids GH Releases noise but is harder to reproduce later if needed.
3. **M1 rollout strategy.** Default-off feature flag with manual user opt-in, or default-off-then-flip in a follow-up release? My recommendation is the latter (less friction for testing) but the former is more conservative.
4. **N1 priority.** Worth it now (after M1) or defer until a user asks? My read is "now" — adaptive brightness is the kind of feature people don't think to request but appreciate when it lands.
5. **D1 — agree to defer Mod 6 indefinitely?** Or do you want a follow-up audit checkpoint in 6 months to re-evaluate (e.g., "if firmware deprecates WS RPCs, revisit")?

---

## Appendix A — Auth probe parking lot

If H1 (Basic with PIN) fails the probe, the backup plan tree:

- **H2 fails too** → fall back to H3 (no-auth from 127.0.0.1). If that works, REST is "trusted localhost only" — accept the limitation, document it, proceed with M1 using no-auth REST.
- **H2 fails AND H3 fails** → audit hits a hard wall. Options: (a) ask UC team directly via GitHub Issues / Discord with the probe Logdy capture as evidence, (b) reverse-engineer the upstream web-configurator's REST auth flow (it must work somehow — it's a web app talking to the same API), (c) abandon the audit's MIGRATE bucket entirely; M1 stays WS, defer the whole thing as "wait for UC to publish proper auth docs."
- **All three pass** → bonus problem of choosing the right one. Bearer (if it works) is cleanest; Basic is most documented; localhost no-auth is fastest. Likely Bearer wins by default.

The probe code itself should ALWAYS test all three even if one succeeds — gives us future-proofing data.

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

*Audit conducted 2026-05-03. Spec version 0.45.2 (firmware 2.9.1). Custom code state: post-v1.4.38 + post-`b08a4ba` UCR3 artifact rename. Three parallel Explore agents + one synthesis pass + one verification pass against the spec to catch agent overreach (`feedback_verify_audit_before_remediation.md` triggered twice — system agent inflated `CreateStandbyInhibitor` schema and the WiFi MIGRATE bucket).*
