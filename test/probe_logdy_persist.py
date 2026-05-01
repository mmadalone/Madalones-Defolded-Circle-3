#!/usr/bin/env python3
"""Persistent UC3 Logdy WS capture: survives suspend/wake by reconnecting.

Differences from probe_logdy.py:
  - Reconnects on ConnectionClosed / TimeoutError / OSError (device suspend kills WS).
  - Appends each frame to outfile immediately (flush=True) — no end-of-run write.
  - Also appends keyword-matched lines to a separate <outfile>.matches.log for fast scanning.
  - Runs for configured total wall-clock duration, regardless of disconnect cycles.

Usage:
    python -u test/probe_logdy_persist.py <duration_sec> <comma_keywords> <outfile>
"""
import asyncio
import json
import sys
import time
from pathlib import Path

import websockets

UC3 = "192.168.2.204"
WS_URL = f"ws://{UC3}/log/ws"


async def capture_session(deadline: float, outfile_path: Path, matches_path: Path, keywords: list[str]) -> str:
    """One WS connection lifecycle. Returns reason-for-exit string."""
    try:
        async with websockets.connect(WS_URL, max_size=None, open_timeout=5, ping_interval=20) as ws:
            print(f"[probe-persist] connected at {time.strftime('%H:%M:%S')}", file=sys.stderr, flush=True)
            with open(outfile_path, "a", encoding="utf-8") as fh, \
                 open(matches_path, "a", encoding="utf-8") as mfh:
                while time.time() < deadline:
                    remaining = deadline - time.time()
                    if remaining <= 0:
                        return "deadline"
                    try:
                        raw = await asyncio.wait_for(ws.recv(), timeout=min(60.0, remaining))
                    except asyncio.TimeoutError:
                        # idle window — connection likely fine, just nothing happening; loop again
                        continue
                    except websockets.ConnectionClosed:
                        return "ws_closed"

                    try:
                        parsed = json.loads(raw)
                        line = json.dumps(parsed, ensure_ascii=False)
                    except Exception:
                        line = raw if isinstance(raw, str) else raw.decode("utf-8", errors="replace")

                    fh.write(line + "\n")
                    fh.flush()

                    line_lower = line.lower()
                    for kw in keywords:
                        if kw.lower() in line_lower:
                            mfh.write(line + "\n")
                            mfh.flush()
                            break
        return "deadline_after_loop"
    except (OSError, asyncio.TimeoutError, websockets.WebSocketException) as exc:
        return f"connect_error:{type(exc).__name__}:{exc}"


async def main(total_sec: int, keywords: list[str], outfile: str):
    outfile_path = Path(outfile)
    matches_path = outfile_path.with_suffix(outfile_path.suffix + ".matches.log")
    outfile_path.parent.mkdir(parents=True, exist_ok=True)

    # Marker line so each run is identifiable even when concatenating
    with open(outfile_path, "a", encoding="utf-8") as fh:
        fh.write(f"# probe-persist start {time.strftime('%Y-%m-%d %H:%M:%S')} duration={total_sec}s keywords={keywords}\n")
    with open(matches_path, "a", encoding="utf-8") as mfh:
        mfh.write(f"# probe-persist start {time.strftime('%Y-%m-%d %H:%M:%S')} duration={total_sec}s keywords={keywords}\n")

    deadline = time.time() + total_sec
    sessions = 0
    while time.time() < deadline:
        sessions += 1
        reason = await capture_session(deadline, outfile_path, matches_path, keywords)
        print(f"[probe-persist] session {sessions} ended: {reason} at {time.strftime('%H:%M:%S')}",
              file=sys.stderr, flush=True)
        if reason == "deadline" or reason == "deadline_after_loop":
            break
        # Backoff: short for ws_closed (device just suspended; will be unreachable a while),
        # longer for connect_error (device offline). 15s sweet spot for both.
        await asyncio.sleep(15)

    with open(outfile_path, "a", encoding="utf-8") as fh:
        fh.write(f"# probe-persist end {time.strftime('%Y-%m-%d %H:%M:%S')} sessions={sessions}\n")
    print(f"[probe-persist] done. {sessions} session(s) over {total_sec}s. "
          f"out={outfile_path} matches={matches_path}", file=sys.stderr, flush=True)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("usage: probe_logdy_persist.py <duration_sec> <comma_keywords> <outfile>", file=sys.stderr)
        sys.exit(2)
    asyncio.run(main(int(sys.argv[1]), sys.argv[2].split(","), sys.argv[3]))
