#!/usr/bin/env python3
"""Capture artwork-related Logdy frames from UC3 for the configured duration.

Extracts:
  - 'Kodi extracted properties' lines (the raw art dict from Kodi)
  - 'artwork attempt' / 'artwork emit' / 'artwork fetch exhausted' lines (integration retry path)
  - 'Kodi update' lines containing MEDIA_IMAGE_URL (what gets pushed to the firmware)
  - 'ENTITY_CHANGE' events on media_player.* (what the firmware actually receives)
"""
import asyncio
import json
import re
import sys
import time

import websockets

UC3 = "192.168.2.204"
WS_URL = f"ws://{UC3}/log/ws"

PATTERNS = [
    ("EXTRACTED", re.compile(r"Kodi extracted properties")),
    ("ATTEMPT",   re.compile(r"artwork (attempt|fetch exhausted|emit|cache hit|cache miss)")),
    ("KODI_UPD",  re.compile(r"Kodi update.*media_image_url", re.IGNORECASE)),
    ("ENT_CHG",   re.compile(r"ENTITY_CHANGE.*media_player\.")),
    ("ART_DICT",  re.compile(r"'art':\s*\{")),
    ("PSEUDOTV",  re.compile(r"pseudotv|PseudoTV", re.IGNORECASE)),
    ("DEFAULT",   re.compile(r"Default(Video|Audio|TVShow|Movie)")),
    ("MEDIAPLAY", re.compile(r"setPreviewImage|Media_image_url|applyMediaImageUrl|isKodiDefaultPlaceholder", re.IGNORECASE)),
    ("MEDIABROW", re.compile(r"MediaBrowser|browseMedia|browse_response", re.IGNORECASE)),
    ("STATE",     re.compile(r"<Attributes\.STATE.*'(PLAYING|PAUSED|ON|BUFFERING)'")),
    ("ART_KEY",   re.compile(r"clearlogo|tvshow\.poster|tvshow\.banner|tvshow\.fanart|season\.poster")),
]


def categorise(msg: str) -> list[str]:
    return [tag for tag, rx in PATTERNS if rx.search(msg)]


async def capture(duration: int, outfile: str):
    print(f"[probe] connecting {WS_URL}", file=sys.stderr)
    raw_lines = []
    interesting = []

    deadline = time.time() + duration
    try:
        async with websockets.connect(WS_URL, max_size=None) as ws:
            print(f"[probe] connected; capturing {duration}s — interact with UC3 NOW", file=sys.stderr)
            while time.time() < deadline:
                try:
                    raw = await asyncio.wait_for(ws.recv(), timeout=max(0.1, deadline - time.time()))
                except asyncio.TimeoutError:
                    break
                except websockets.ConnectionClosed:
                    break

                try:
                    frame = json.loads(raw)
                except Exception:
                    raw_lines.append(raw)
                    continue

                # Logdy frames are typed: log_bulk wraps a "messages" list, log is single.
                msgs = []
                if frame.get("message_type") == "log_bulk":
                    msgs = frame.get("messages", [])
                elif frame.get("message_type") == "log":
                    msgs = [frame]

                for m in msgs:
                    jc = m.get("json_content") or {}
                    txt = jc.get("msg") or m.get("content") or ""
                    svc = jc.get("service") or ""
                    ts  = jc.get("ts") or ""
                    raw_lines.append(f"{ts}\t{svc}\t{txt}")
                    tags = categorise(txt)
                    if tags:
                        interesting.append((ts, svc, tags, txt))

    except Exception as exc:
        print(f"[probe] ws error: {exc}", file=sys.stderr)

    print(f"[probe] captured {len(raw_lines)} log lines, {len(interesting)} relevant", file=sys.stderr)

    with open(outfile, "w", encoding="utf-8") as fh:
        fh.write("\n".join(raw_lines))
    print(f"[probe] full dump -> {outfile}", file=sys.stderr)

    def safe_print(s: str) -> None:
        print(s.encode("ascii", "replace").decode("ascii"))

    print()
    print("=" * 80)
    print("RELEVANT FRAMES (chronological, non-ASCII replaced with '?' for console)")
    print("=" * 80)
    for ts, svc, tags, txt in interesting:
        tagstr = ",".join(tags)
        safe_print(f"\n[{ts}] [{svc}] [{tagstr}]")
        if "EXTRACTED" in tags or "ART_DICT" in tags:
            safe_print(txt[:1200] + ("..." if len(txt) > 1200 else ""))
        else:
            safe_print(txt[:600] + ("..." if len(txt) > 600 else ""))

    # Final-state media_image_url decision per entity (what firmware sees).
    print()
    print("=" * 80)
    print("FINAL MEDIA_IMAGE_URL VALUES PUSHED TO FIRMWARE (chronological)")
    print("=" * 80)
    image_url_rx = re.compile(r"<Attributes\.MEDIA_IMAGE_URL: 'media_image_url'>:\s*'([^']*)'")
    seen = []
    for ts, svc, tags, txt in interesting:
        if "KODI_UPD" in tags or "ENT_CHG" in tags:
            for url in image_url_rx.findall(txt):
                seen.append((ts, url))
    prev = None
    for ts, url in seen:
        if url == prev:
            continue
        prev = url
        if url.startswith("data:"):
            safe_print(f"  [{ts}] data URI ({len(url)} chars, ~{int(len(url)*3/4)} bytes binary)")
        elif url.startswith("http"):
            safe_print(f"  [{ts}] HTTP -> {url}")
        elif url == "":
            safe_print(f"  [{ts}] (empty)")
        else:
            safe_print(f"  [{ts}] {url[:200]}")


if __name__ == "__main__":
    import os
    from pathlib import Path
    DEFAULT_LOGS_DIR = Path(__file__).resolve().parent.parent / "logs"
    DEFAULT_LOGS_DIR.mkdir(exist_ok=True)
    duration = int(sys.argv[1]) if len(sys.argv) > 1 else 30
    outfile = sys.argv[2] if len(sys.argv) > 2 else str(DEFAULT_LOGS_DIR / "uc3_artwork_capture.txt")
    asyncio.run(capture(duration, outfile))
