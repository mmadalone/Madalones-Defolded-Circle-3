#!/usr/bin/env python3
"""Probe DockCommands::REMOTE_CHARGED / REMOTE_LOW_BATTERY / REMOTE_NORMAL on UCR3.

Usage:
    python test/probe_dock_charge.py CHARGED      # send REMOTE_CHARGED
    python test/probe_dock_charge.py LOW_BATTERY  # send REMOTE_LOW_BATTERY
    python test/probe_dock_charge.py NORMAL       # send REMOTE_NORMAL
    python test/probe_dock_charge.py STATUS       # just show current power_supply + dock state
    python test/probe_dock_charge.py WATCH        # poll battery every 10 s, print delta -- Ctrl-C to stop

Recommended test workflow (terminal A vs B, remote on dock):
    A: python test/probe_dock_charge.py WATCH
    A: ... wait ~30 s, confirm capacity climbing while status=CHARGING ...
    B: python test/probe_dock_charge.py CHARGED
    A: ... watch for ~120 s -- does capacity flatten? does status flip? ...
    B: python test/probe_dock_charge.py LOW_BATTERY
    A: ... confirm charging resumes ...
    A: Ctrl-C

Reads UC3_HOST + UC3_PIN from env vars; falls back to project defaults
(192.168.2.204 / 6984) -- matches CLAUDE.md's documented home-network pin.
"""
import asyncio
import base64
import json
import os
import sys
import time

import websockets

UC3_HOST = os.environ.get("UC3_HOST", "192.168.2.204")
UC3_PIN  = os.environ.get("UC3_PIN",  "6984")
USER     = "web-configurator"

DOCK_ID  = "UCD3-0BCF00"

VALID_CMDS = {"CHARGED", "LOW_BATTERY", "NORMAL", "STATUS", "WATCH", "TEST", "WAIT_HIGH"}

WATCH_INTERVAL_SEC = 10

# TEST mode timing (all in seconds)
TEST_BASELINE_SEC = 30   # baseline observation before firing CHARGED
TEST_OBSERVE_SEC  = 90   # observation after CHARGED, before restore
TEST_RECOVER_SEC  = 30   # observation after LOW_BATTERY restore

# WAIT_HIGH mode: wait for high SOC, then run experiment
WAIT_HIGH_THRESHOLD_PCT = int(os.environ.get("WAIT_HIGH_THRESHOLD", "90"))
WAIT_HIGH_POLL_SEC      = 30           # while waiting, poll less aggressively
WAIT_HIGH_TIMEOUT_MIN   = 60           # bail out if we never reach the threshold

CMD_MAP = {
    "CHARGED":      "REMOTE_CHARGED",
    "LOW_BATTERY":  "REMOTE_LOW_BATTERY",
    "NORMAL":       "REMOTE_NORMAL",
}


async def send_cmd(arg: str) -> int:
    auth_b64 = base64.b64encode(f"{USER}:{UC3_PIN}".encode()).decode()
    url = f"ws://{UC3_HOST}/ws"

    print(f"[probe] connecting to {url} (Basic auth as {USER})")

    async with websockets.connect(
        url,
        additional_headers={"Authorization": f"Basic {auth_b64}"},
        max_size=None,
    ) as ws:
        # Server greets us with an authentication resp on connect; read it.
        first = await asyncio.wait_for(ws.recv(), timeout=5)
        try:
            j = json.loads(first)
            print(f"[probe] auth ack: code={j.get('code')} core={j.get('msg_data', {}).get('core')}")
        except Exception:
            print(f"[probe] auth ack (raw): {first!r}")

        async def poll_one(req_id_box, header_emitted_box, last_cap_box, marker=""):
            """Send get_power_mode, parse, print one row. Updates last_cap_box[0] in place."""
            await ws.send(json.dumps({
                "kind": "req", "id": req_id_box[0], "msg": "get_power_mode", "msg_data": {},
            }))
            req_id_box[0] += 1
            raw = await asyncio.wait_for(ws.recv(), timeout=5)
            try:
                j = json.loads(raw)
                d = j.get("msg_data", {})
                b = d.get("battery", {}) or {}
                cap = b.get("capacity")
                status = b.get("status", "?")
                ps = b.get("power_supply")
                mode = d.get("mode", "?")
            except Exception:
                cap = status = ps = mode = "?"

            if not header_emitted_box[0]:
                print(f"{'time':<10}  {'cap':>5}  {'status':<14}  "
                      f"{'on_dock':<7}  {'delta':>6}  {'mode':<10}  marker", flush=True)
                header_emitted_box[0] = True

            last_cap = last_cap_box[0]
            if last_cap is None or cap == "?" or last_cap == "?":
                delta = "  -  "
            else:
                d_pp = cap - last_cap
                delta = f"{d_pp:+3d}%" if d_pp != 0 else "  0%"
            last_cap_box[0] = cap

            ts = time.strftime("%H:%M:%S")
            cap_s = f"{cap}%" if isinstance(cap, int) else str(cap)
            ps_s = "yes" if ps else ("no" if ps is False else "?")
            print(f"{ts:<10}  {cap_s:>5}  {status:<14}  "
                  f"{ps_s:<7}  {delta:>6}  {mode:<10}  {marker}",
                  flush=True)

        async def fire_cmd(req_id_box, name):
            payload = {
                "kind": "req", "id": req_id_box[0], "msg": "dock_command",
                "msg_data": {"dock_id": DOCK_ID, "command": name},
            }
            req_id_box[0] += 1
            ts = time.strftime("%H:%M:%S")
            print(f">>> [{ts}] FIRING {name}", flush=True)
            await ws.send(json.dumps(payload))
            raw = await asyncio.wait_for(ws.recv(), timeout=5)
            print(f"    response: {raw}", flush=True)

        if arg == "WAIT_HIGH":
            print(f"[probe] WAIT_HIGH mode -- polling every {WAIT_HIGH_POLL_SEC}s "
                  f"until capacity >= {WAIT_HIGH_THRESHOLD_PCT}%, "
                  f"then run the CHARGED/LOW_BATTERY experiment.")
            print(f"[probe] timeout: {WAIT_HIGH_TIMEOUT_MIN} min if threshold never reached.")
            req_id_box = [10]
            header_emitted_box = [False]
            last_cap_box = [None]

            deadline = time.monotonic() + WAIT_HIGH_TIMEOUT_MIN * 60
            while time.monotonic() < deadline:
                await poll_one(req_id_box, header_emitted_box, last_cap_box,
                               marker="(waiting)")
                cap = last_cap_box[0]
                if isinstance(cap, int) and cap >= WAIT_HIGH_THRESHOLD_PCT:
                    print(f"[probe] threshold {WAIT_HIGH_THRESHOLD_PCT}% reached.", flush=True)
                    break
                await asyncio.sleep(WAIT_HIGH_POLL_SEC)
            else:
                print(f"[probe] WAIT_HIGH timed out after {WAIT_HIGH_TIMEOUT_MIN} min "
                      f"-- never reached {WAIT_HIGH_THRESHOLD_PCT}%. Bailing.",
                      flush=True)
                return 1

            # Settle for 20s of high-SOC baseline so we can see the genuine
            # near-full charging cadence.
            for _ in range(2):
                await asyncio.sleep(WATCH_INTERVAL_SEC)
                await poll_one(req_id_box, header_emitted_box, last_cap_box,
                               marker="(high-baseline)")

            # Fire CHARGED, watch 90s
            await fire_cmd(req_id_box, "REMOTE_CHARGED")
            n = TEST_OBSERVE_SEC // WATCH_INTERVAL_SEC
            for _ in range(n):
                await asyncio.sleep(WATCH_INTERVAL_SEC)
                await poll_one(req_id_box, header_emitted_box, last_cap_box,
                               marker="(post-CHARGED@high)")

            # Restore, watch 30s
            await fire_cmd(req_id_box, "REMOTE_LOW_BATTERY")
            n = TEST_RECOVER_SEC // WATCH_INTERVAL_SEC
            for _ in range(n):
                await asyncio.sleep(WATCH_INTERVAL_SEC)
                await poll_one(req_id_box, header_emitted_box, last_cap_box,
                               marker="(recovery@high)")

            print("[probe] WAIT_HIGH done.", flush=True)
            return 0

        if arg == "TEST":
            print(f"[probe] TEST mode -- baseline {TEST_BASELINE_SEC}s, observe {TEST_OBSERVE_SEC}s, "
                  f"recover {TEST_RECOVER_SEC}s. Total ~{TEST_BASELINE_SEC + TEST_OBSERVE_SEC + TEST_RECOVER_SEC}s.")
            req_id_box = [10]
            header_emitted_box = [False]
            last_cap_box = [None]

            # Phase 1: baseline
            n = TEST_BASELINE_SEC // WATCH_INTERVAL_SEC
            for i in range(n):
                await poll_one(req_id_box, header_emitted_box, last_cap_box,
                               marker="(baseline)")
                if i < n - 1:
                    await asyncio.sleep(WATCH_INTERVAL_SEC)

            # Phase 2: fire CHARGED, then observe
            await fire_cmd(req_id_box, "REMOTE_CHARGED")
            n = TEST_OBSERVE_SEC // WATCH_INTERVAL_SEC
            for i in range(n):
                await asyncio.sleep(WATCH_INTERVAL_SEC)
                await poll_one(req_id_box, header_emitted_box, last_cap_box,
                               marker="(post-CHARGED)")

            # Phase 3: fire LOW_BATTERY, then observe recovery
            await fire_cmd(req_id_box, "REMOTE_LOW_BATTERY")
            n = TEST_RECOVER_SEC // WATCH_INTERVAL_SEC
            for i in range(n):
                await asyncio.sleep(WATCH_INTERVAL_SEC)
                await poll_one(req_id_box, header_emitted_box, last_cap_box,
                               marker="(recovery)")

            print("[probe] TEST done.", flush=True)
            return 0

        if arg == "WATCH":
            print("[probe] WATCH mode -- polling get_power_mode every "
                  f"{WATCH_INTERVAL_SEC}s. Ctrl-C to stop.")
            print(f"{'time':<10}  {'cap':>5}  {'status':<14}  "
                  f"{'on_dock':<7}  {'delta':>6}  {'mode':<10}")
            last_cap = None
            req_id = 100
            try:
                while True:
                    await ws.send(json.dumps({
                        "kind": "req", "id": req_id, "msg": "get_power_mode", "msg_data": {},
                    }))
                    req_id += 1
                    raw = await asyncio.wait_for(ws.recv(), timeout=5)
                    try:
                        j = json.loads(raw)
                        d = j.get("msg_data", {})
                        b = d.get("battery", {}) or {}
                        cap = b.get("capacity")
                        status = b.get("status", "?")
                        ps = b.get("power_supply")
                        mode = d.get("mode", "?")
                    except Exception:
                        cap = status = ps = mode = "?"

                    if last_cap is None or cap == "?" or last_cap == "?":
                        delta = "  -  "
                    else:
                        d_pp = cap - last_cap
                        delta = f"{d_pp:+3d}%" if d_pp != 0 else "  0%"
                    last_cap = cap

                    ts = time.strftime("%H:%M:%S")
                    cap_s = f"{cap}%" if isinstance(cap, int) else str(cap)
                    ps_s = "yes" if ps else ("no" if ps is False else "?")
                    print(f"{ts:<10}  {cap_s:>5}  {status:<14}  "
                          f"{ps_s:<7}  {delta:>6}  {mode:<10}",
                          flush=True)

                    await asyncio.sleep(WATCH_INTERVAL_SEC)
            except KeyboardInterrupt:
                print("\n[probe] WATCH stopped.")
                return 0

        if arg == "STATUS":
            # Send a pair of probes: get_power_mode and get_dock list.
            await ws.send(json.dumps({"kind": "req", "id": 1, "msg": "get_power_mode", "msg_data": {}}))
            r1 = await asyncio.wait_for(ws.recv(), timeout=5)
            print(f"[probe] get_power_mode -> {r1}")

            await ws.send(json.dumps({"kind": "req", "id": 2, "msg": "get_docks", "msg_data": {}}))
            r2 = await asyncio.wait_for(ws.recv(), timeout=5)
            print(f"[probe] get_docks -> {r2[:300]}{'...' if len(r2) > 300 else ''}")
            return 0

        cmd_name = CMD_MAP[arg]
        payload = {
            "kind": "req",
            "id": 99,
            "msg": "dock_command",
            "msg_data": {
                "dock_id": DOCK_ID,
                "command": cmd_name,
            },
        }
        print(f"[probe] sending: {json.dumps(payload)}")
        t0 = time.monotonic()
        await ws.send(json.dumps(payload))

        # Listen for up to 5 s for any response (success or error)
        deadline = time.monotonic() + 5.0
        while time.monotonic() < deadline:
            try:
                resp = await asyncio.wait_for(
                    ws.recv(),
                    timeout=max(0.05, deadline - time.monotonic()),
                )
            except asyncio.TimeoutError:
                break
            elapsed_ms = (time.monotonic() - t0) * 1000
            print(f"[probe +{elapsed_ms:6.0f} ms] {resp}")

        return 0


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in VALID_CMDS:
        print(__doc__)
        return 2
    return asyncio.run(send_cmd(sys.argv[1]))


if __name__ == "__main__":
    sys.exit(main())
