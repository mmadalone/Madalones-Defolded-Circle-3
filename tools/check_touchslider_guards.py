#!/usr/bin/env python3
# Copyright (c) 2026 madalone. Drift check for the touch slider's screensaver guards.
# SPDX-License-Identifier: GPL-3.0-or-later
"""Drift check for the screensaver guards in the four touch slider overlays.

History
-------
While the screensaver shows, the touch slider belongs to it (Matrix speed, Starfield density). The overlays that send
a slide to a device, src/qml/components/TouchSlider{Volume,Brightness,Position,Seek}.qml, are upstream files that
carry our guards for that:

  - v1.2.0: a press while the screensaver shows starts no slide (the press guard).
  - v1.4.7: moves and the lift under the screensaver are ignored too (a move guard and the release guard).
  - v1.6.0: the merge of upstream v0.84.0 dropped the move guard; nobody noticed, because nothing checked for it.
  - v1.6.4: a slide that is under way when the screensaver opens ends right there (the handler of
    applicationWindow.screensaverActive), and a lift with no slide under way sends nothing but still closes the
    overlay 1 s later (the no-slide guard).

The behaviour is tested in test/qml/tst_touch_slider_screensaver.qml. This check runs in seconds without Qt, so a
merge that drops a guard is named at once, file by file, even before the QML job runs. It checks:

  - src/qml/main.qml still declares `property bool screensaverActive` (what the guards read);
  - each overlay's onTouchPressed holds `if (applicationWindow.screensaverActive) return;`;
  - each overlay's onTouchReleased holds the same line, then `if (!sliderContainer.touchSliderActive) {` with
    `releaseTimer.start();` and `return;` and no command, before the release's own `updateDataTimer.stop();`;
  - each overlay has a `Connections` with `target: applicationWindow` and no `ignoreUnknownSignals` (a renamed
    screensaverActive must warn), whose onScreensaverActiveChanged returns early unless the screensaver opened while a
    slide is active, then stops updateDataTimer, sends the target (only if the entity is enabled and the value
    differs), sets touchSliderActive to false and calls close();
  - the brightness overlay no longer calls upstream's `Math.rodun` (a TypeError).

A statement counts only as live code at the start of a line, so a commented-out guard is reported as missing. It is a
text check: a harmless reformat of these lines also trips it. Then compare with the QML test and update this script.

Wired into CI as the fast job "touchslider-guards" in .github/workflows/test.yml.

Exit codes:
  0 - every guard is in place.
  1 - a guard is missing or changed (each one is listed).
  2 - a file was not found (the repository layout changed).

Usage:
  python3 tools/check_touchslider_guards.py [--root DIR]
"""

import argparse
import re
import sys
from pathlib import Path

# overlay file -> (the entity property it sends, the entity setter, the overlay's target property)
OVERLAYS = {
    "TouchSliderVolume.qml": ("volume", "setVolume", "targetVolume"),
    "TouchSliderBrightness.qml": ("brightness", "setBrightness", "targetBrightness"),
    "TouchSliderPosition.qml": ("position", "setPosition", "targetPosition"),
    "TouchSliderSeek.qml": ("mediaPosition", "seek", "targetPosition"),
}
SCREENSAVER_GUARD = "if (applicationWindow.screensaverActive) return;"
NO_SLIDE_GUARD = "if (!sliderContainer.touchSliderActive) {"
HANDLER_RETURN = "if (!applicationWindow.screensaverActive || !sliderContainer.touchSliderActive) {"
TYPO = "Math.rodun("


def line_pos(text: str, stmt: str) -> int:
    """Position of `stmt` as live code at the start of a line (after indentation only), or -1."""
    m = re.search(r"^[ \t]*" + re.escape(stmt), text, re.MULTILINE)
    return m.start() if m else -1


def block_after(text: str, start: int) -> str:
    """The text from `start` to the brace that closes the first `{` at or after it (empty if unbalanced)."""
    open_at = text.find("{", start)
    if open_at < 0:
        return ""
    depth = 0
    for i in range(open_at, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    return ""


def function_body(text: str, name: str) -> str:
    m = re.search(r"^[ \t]*function\s+" + re.escape(name) + r"\s*\(", text, re.MULTILINE)
    return block_after(text, m.start()) if m else ""


def check_overlay(path: Path, label: str, prop: str, setter: str, target: str) -> list:
    text = path.read_text(encoding="utf-8")
    errors = []

    if line_pos(function_body(text, "onTouchPressed"), SCREENSAVER_GUARD) < 0:
        errors.append(f"{label}: onTouchPressed lost the press guard `{SCREENSAVER_GUARD}` (v1.2.0): a press under "
                      f"the screensaver would start a slide on the device.")

    release = function_body(text, "onTouchReleased")
    if line_pos(release, SCREENSAVER_GUARD) < 0:
        errors.append(f"{label}: onTouchReleased lost the release guard `{SCREENSAVER_GUARD}` (v1.4.7): a lift "
                      f"under the screensaver would send to the device.")
    guard_at = line_pos(release, NO_SLIDE_GUARD)
    send_at = line_pos(release, "updateDataTimer.stop();")
    if guard_at < 0 or (send_at >= 0 and guard_at > send_at):
        errors.append(f"{label}: onTouchReleased lost the no-slide guard `{NO_SLIDE_GUARD}` before its "
                      f"`updateDataTimer.stop();` (v1.6.4): a lift with no slide under way would send the slider's "
                      f"last target, 0 on a fresh overlay.")
    else:
        guard = block_after(release, guard_at)
        for step in ("releaseTimer.start();", "return;"):
            if line_pos(guard, step) < 0:
                errors.append(f"{label}: the no-slide guard in onTouchReleased lost `{step}` (v1.6.4).")
        if "entityObj." in guard:
            errors.append(f"{label}: the no-slide guard in onTouchReleased talks to the entity; it must send nothing.")

    handler_conn = ""
    for m in re.finditer(r"^[ \t]*Connections\s*\{", text, re.MULTILINE):
        block = block_after(text, m.start())
        if re.search(r"^[ \t]*target:\s*applicationWindow\s*$", block, re.MULTILINE) and \
                line_pos(block, "function onScreensaverActiveChanged") >= 0:
            handler_conn = block
            break
    if not handler_conn:
        errors.append(f"{label}: no `Connections {{ target: applicationWindow }}` with onScreensaverActiveChanged "
                      f"(v1.6.4): a slide under way when the screensaver opens would keep driving the device.")
    else:
        if re.search(r"^[ \t]*ignoreUnknownSignals\s*:", handler_conn, re.MULTILINE):
            errors.append(f"{label}: the applicationWindow Connections sets ignoreUnknownSignals: a renamed "
                          f"screensaverActive would then disable the handler without a warning.")
        body = function_body(handler_conn, "onScreensaverActiveChanged")
        steps = [
            (HANDLER_RETURN, "every opening of the screensaver would end, or send for, a slide that isn't there"),
            ("updateDataTimer.stop();", "the 200 ms updates would go on under the screensaver"),
            (f"if (entityObj.enabled && entityObj.{prop} != sliderContainer.{target}) {{",
             "the value would be sent to an unavailable entity, or again when the device already has it"),
            (f"entityObj.{setter}(sliderContainer.{target});", "the value the slider showed would not be sent"),
            ("sliderContainer.touchSliderActive = false;", "the slide would stay active under the screensaver"),
            ("sliderContainer.close();", "the overlay would stay open"),
        ]
        for step, why in steps:
            if line_pos(body, step) < 0:
                errors.append(f"{label}: onScreensaverActiveChanged lost `{step}` (v1.6.4): {why}.")

    if TYPO in text:
        errors.append(f"{label}: calls upstream's `{TYPO}` again (a TypeError; it is Math.round).")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1],
                        help="repository root to check (default: this repository)")
    root = parser.parse_args().root.resolve()

    main_qml = root / "src" / "qml" / "main.qml"
    overlays = {name: root / "src" / "qml" / "components" / name for name in OVERLAYS}
    missing = [p for p in [main_qml] + list(overlays.values()) if not p.exists()]
    if missing:
        for p in missing:
            print(f"FILE NOT FOUND: {p}")
        return 2

    errors = []
    if not re.search(r"^[ \t]*(?:readonly\s+)?property\s+bool\s+screensaverActive\s*:",
                     main_qml.read_text(encoding="utf-8"), re.MULTILINE):
        errors.append("src/qml/main.qml: no `property bool screensaverActive` any more; the touch slider guards read "
                      "applicationWindow.screensaverActive.")
    for name, path in overlays.items():
        errors.extend(check_overlay(path, f"src/qml/components/{name}", *OVERLAYS[name]))

    if errors:
        print("Touch slider screensaver guards: DRIFT")
        for e in errors:
            print(f"  - {e}")
        print("See test/qml/tst_touch_slider_screensaver.qml and docs/UPSTREAM_MERGE.md (TouchSlider*.qml).")
        return 1
    print(f"Touch slider screensaver guards: OK (main.qml + {len(overlays)} overlays)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
