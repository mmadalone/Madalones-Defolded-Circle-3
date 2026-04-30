#!/usr/bin/env python3
"""Connect to UC3 Logdy WS, capture log stream for N seconds, filter for keywords."""
import asyncio
import json
import sys
import time

import websockets

UC3 = "192.168.2.204"

async def capture(duration_sec: int, keywords: list[str], outfile: str | None = None):
    url = f"ws://{UC3}/log/ws"
    print(f"[probe] connecting to {url}", file=sys.stderr)
    captured = []
    deadline = time.time() + duration_sec

    try:
        async with websockets.connect(url, max_size=None) as ws:
            print(f"[probe] connected; capturing {duration_sec}s", file=sys.stderr)
            while time.time() < deadline:
                try:
                    raw = await asyncio.wait_for(ws.recv(), timeout=max(0.1, deadline - time.time()))
                except asyncio.TimeoutError:
                    break
                except websockets.ConnectionClosed:
                    break

                # Logdy frames are JSON. Try to parse, fall back to raw.
                try:
                    msg = json.loads(raw)
                except Exception:
                    captured.append(raw)
                    continue

                # The interesting payload may be in msg.content or msg.json_content.msg etc.
                line = json.dumps(msg, ensure_ascii=False)
                captured.append(line)

    except Exception as exc:
        print(f"[probe] ws error: {exc}", file=sys.stderr)

    print(f"[probe] captured {len(captured)} frames", file=sys.stderr)

    matches = []
    for line in captured:
        for kw in keywords:
            if kw.lower() in line.lower():
                matches.append(line)
                break

    if outfile:
        with open(outfile, "w", encoding="utf-8") as fh:
            fh.write("\n".join(captured))
        print(f"[probe] wrote {len(captured)} lines to {outfile}", file=sys.stderr)

    print(f"[probe] {len(matches)} keyword matches:", file=sys.stderr)
    for m in matches[:200]:
        print(m)


if __name__ == "__main__":
    from pathlib import Path
    DEFAULT_LOGS_DIR = Path(__file__).resolve().parent.parent / "logs"
    DEFAULT_LOGS_DIR.mkdir(exist_ok=True)
    duration = int(sys.argv[1]) if len(sys.argv) > 1 else 30
    keywords = sys.argv[2].split(",") if len(sys.argv) > 2 else [
        "setPreviewImage", "MediaPlayer", "MediaBrowser",
        "image://", "thumbnail", "pseudotv", "PseudoTV",
        "ProtocolUnknownError", "applyMediaImageUrl",
    ]
    outfile = sys.argv[3] if len(sys.argv) > 3 else str(DEFAULT_LOGS_DIR / "uc3_logdy_capture.txt")
    asyncio.run(capture(duration, keywords, outfile))
