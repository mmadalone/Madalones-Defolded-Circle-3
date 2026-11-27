#!/usr/bin/env python3
# Copyright (c) 2026 madalone. The CHANGELOG section of one release, for the release notes and the tag gate.
# SPDX-License-Identifier: GPL-3.0-or-later
"""Print the CHANGELOG.md section of one release (v1.6.6).

Why
---
Up to v1.6.5 a tagged release was published with notes the release action made up from the commit list, and the
maintainer then replaced them by hand with the release's CHANGELOG section. The tag gate in build.yml only looked for
a heading, with a loose pattern: its dots matched any character (`## v1a6b5` passed for 1.6.5), it took a `b` after
the version, and it missed a heading with nothing after the version.

This script is both: the gate (exit 1 when the release has no section, or an empty one) and the notes (the section's
text on standard output, which `gh release create --notes-file` publishes).

What counts as the section
--------------------------
A heading line `## <version>` or `### <version>`, the version optionally written with a leading `v` and optionally in
square brackets, followed by the end of the line, a blank or `]`:

    ## v1.6.5 — 2026-11-26 — Close on wake closes soon after a lift
    ## [1.6.5] - 2026-11-26
    ### 1.6.5

The section runs to the next heading of the same or a higher level (for a `##` heading: the next `## ` or `# ` line),
or to the end of the file. Lines inside a fenced code block (``` or ~~~) are never headings. Blank lines at both ends
are dropped and the text ends with one newline. The first matching heading wins.

Exit codes:
  0 - the section was printed (with --self-test: every case gave the expected result).
  1 - no section for that version, or it is empty (with --self-test: a case failed; each one is listed).
  2 - the changelog file was not found, or the arguments are wrong.

Usage:
  python3 tools/release_notes.py <version> [--changelog FILE]
  python3 tools/release_notes.py --self-test
"""

import argparse
import re
import sys
from pathlib import Path

FENCE = re.compile(r"^[ ]{0,3}(```|~~~)")
HEADING = re.compile(r"^(#{1,6})[ \t]+\S")


def heading_pattern(version: str) -> "re.Pattern[str]":
    """The heading of `version` (given with or without a leading v)."""
    bare = version[1:] if version[:1] in ("v", "V") else version
    return re.compile(r"^(#{2,3})[ \t]+v?\[?" + re.escape(bare) + r"(?=[\] \t]|$)")


def section(text: str, version: str):
    """The text of the release's section, or None when there is no heading for it."""
    wanted = heading_pattern(version)
    lines = text.splitlines()
    in_fence = False
    start = level = None
    end = len(lines)
    for i, line in enumerate(lines):
        if FENCE.match(line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        if start is None:
            m = wanted.match(line)
            if m:
                start, level = i + 1, len(m.group(1))
            continue
        m = HEADING.match(line)
        if m and len(m.group(1)) <= level:
            end = i
            break
    if start is None:
        return None
    body = lines[start:end]
    while body and not body[0].strip():
        body.pop(0)
    while body and not body[-1].strip():
        body.pop()
    return "\n".join(body) + "\n" if body else ""


def self_test() -> int:
    sample = "\n".join([
        "# Changelog",
        "",
        "## Unreleased",
        "",
        "- not out yet",
        "",
        "## v1.6.50 — 2027-01-01 — a later one",
        "",
        "fifty",
        "",
        "## v1.6.5 — 2026-11-26 — the summary",
        "",
        "First paragraph.",
        "",
        "### Added",
        "- one",
        "```",
        "## not a heading, it is code",
        "```",
        "",
        "## [1.6.4] - 2026-11-15",
        "four",
        "## v1.6.3",
        "",
        "## v1.6.2-rc1 — a pre-release",
        "rc",
        "## Older releases",
        "### 1.2.3",
        "old style",
        "#### detail stays inside",
        "more",
        "### 1.2.2",
        "last",
        "",
    ])
    cases = [
        ("1.6.5", "First paragraph.\n\n### Added\n- one\n```\n## not a heading, it is code\n```\n"),
        ("v1.6.5", "First paragraph.\n\n### Added\n- one\n```\n## not a heading, it is code\n```\n"),
        ("1.6.50", "fifty\n"),
        ("1.6.4", "four\n"),
        ("1.6.3", ""),               # a heading with nothing under it
        ("1.6.2", None),             # 1.6.2-rc1 is another version
        ("1.6.2-rc1", "rc\n"),
        ("1.2.3", "old style\n#### detail stays inside\nmore\n"),
        ("1.2.2", "last\n"),         # runs to the end of the file
        ("1.6", None),               # a prefix is not a version
        ("9.9.9", None),
    ]
    failed = []
    for version, expected in cases:
        got = section(sample, version)
        if got != expected:
            failed.append(f"{version}: expected {expected!r}, got {got!r}")
    for line in failed:
        print("SELF-TEST FAIL " + line)
    if not failed:
        print(f"self-test ok: {len(cases)} cases")
    return 1 if failed else 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Print the CHANGELOG section of one release.")
    parser.add_argument("version", nargs="?", help="the release, with or without a leading v (1.6.5 or v1.6.5)")
    parser.add_argument("--changelog", default=None, help="the changelog file (default: CHANGELOG.md at the repo root)")
    parser.add_argument("--self-test", action="store_true", help="run the built-in cases and exit")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    if not args.version:
        parser.print_usage(sys.stderr)
        return 2
    path = Path(args.changelog) if args.changelog else Path(__file__).resolve().parent.parent / "CHANGELOG.md"
    if not path.is_file():
        print(f"release_notes: {path} not found", file=sys.stderr)
        return 2
    body = section(path.read_text(encoding="utf-8"), args.version)
    if body is None:
        print(f"release_notes: {path.name} has no section for {args.version}: add a heading like "
              f"'## v{args.version.lstrip('vV')} — <date> — <summary>' before tagging", file=sys.stderr)
        return 1
    if not body:
        print(f"release_notes: the section for {args.version} in {path.name} is empty", file=sys.stderr)
        return 1
    # bytes, so that Windows neither re-encodes the text nor turns the line ends into CR LF
    sys.stdout.buffer.write(body.encode("utf-8"))
    sys.stdout.buffer.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
