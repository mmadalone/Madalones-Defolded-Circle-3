#!/usr/bin/env python3
"""Logdy WS live-stream probe — sends application-level {"message_type":"ping"} periodically.

The default Logdy WS appears to send 1 log_bulk on connect (recent backlog) and then
streams live logs ONLY if the client periodically pings. The JS frontend at /log/
sends `{"message_type":"ping"}` over the WS at a regular cadence; without that, our
server-side stream goes silent (only client_msg_status pings come back).

Usage: python -u test/probe_logdy_live.py <duration_sec> <comma_keywords> <outfile>
"""
import asyncio
import json
import sys
import time
from pathlib import Path

import websockets

UC3 = "192.168.2.204"
WS_URL = f"ws://{UC3}/log/ws"
PING_INTERVAL = 5.0  # seconds between application-level pings

async def ping_loop(ws, deadline):
    while time.time() < deadline:
        try:
            await ws.send('{"message_type":"ping"}')
        except Exception:
            return
        await asyncio.sleep(PING_INTERVAL)

async def recv_loop(ws, deadline, outfile, matches_file, keywords):
    captured = 0
    matched = 0
    while time.time() < deadline:
        remaining = deadline - time.time()
        if remaining <= 0:
            break
        try:
            raw = await asyncio.wait_for(ws.recv(), timeout=min(60.0, remaining))
        except asyncio.TimeoutError:
            continue
        except websockets.ConnectionClosed:
            break
        try:
            parsed = json.loads(raw)
            line = json.dumps(parsed, ensure_ascii=False)
        except Exception:
            line = raw if isinstance(raw, str) else raw.decode("utf-8", errors="replace")
        outfile.write(line + "\n")
        outfile.flush()
        captured += 1
        ll = line.lower()
        for kw in keywords:
            if kw.lower() in ll:
                matches_file.write(line + "\n")
                matches_file.flush()
                matched += 1
                break
    return captured, matched

async def main(duration: int, keywords: list, outpath: str):
    out = Path(outpath)
    out.parent.mkdir(parents=True, exist_ok=True)
    matches_path = out.with_suffix(out.suffix + ".matches.log")
    deadline = time.time() + duration
    print(f"[probe-live] connecting to {WS_URL}", file=sys.stderr, flush=True)
    async with websockets.connect(WS_URL, max_size=None, open_timeout=10, ping_interval=None) as ws:
        print(f"[probe-live] connected; capturing {duration}s with app-pings every {PING_INTERVAL}s", file=sys.stderr, flush=True)
        with open(out, "w", encoding="utf-8") as ofh, open(matches_path, "w", encoding="utf-8") as mfh:
            ofh.write(f"# probe-live start {time.strftime('%Y-%m-%d %H:%M:%S')} duration={duration}s keywords={keywords}\n")
            mfh.write(f"# probe-live start {time.strftime('%Y-%m-%d %H:%M:%S')} duration={duration}s keywords={keywords}\n")
            done, _ = await asyncio.wait(
                [asyncio.create_task(ping_loop(ws, deadline)),
                 asyncio.create_task(recv_loop(ws, deadline, ofh, mfh, keywords))],
                return_when=asyncio.FIRST_COMPLETED,
            )
            print(f"[probe-live] sessions ended", file=sys.stderr, flush=True)

if __name__ == "__main__":
    duration = int(sys.argv[1]) if len(sys.argv) > 1 else 60
    keywords = sys.argv[2].split(",") if len(sys.argv) > 2 else ["Probe"]
    outfile = sys.argv[3] if len(sys.argv) > 3 else "logs/uc3_logdy_live.txt"
    asyncio.run(main(duration, keywords, outfile))
