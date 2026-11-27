#!/usr/bin/env python3
# Copyright (c) 2026 madalone. Check that the GitHub workflows keep their pinned actions and read-only token.
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check the GitHub workflows: pinned actions, a read-only default token, no stored credentials (v1.6.6).

History
-------
Upstream pinned every action to a commit, gave the job token read access by default and replaced an abandoned release
action. The fork's merge with upstream v0.84.0 kept its own older workflows, so none of that arrived: up to v1.6.5 the
release job ran `marvinpinto/action-automatic-releases@latest`, an archived action, with write access and right after
the signing key was loaded. Nothing failed, and nobody noticed for a month.

The workflows were rebuilt for v1.6.6. This check runs in seconds without any tool beyond Python, so a merge or an edit
that brings one of those things back is named at once. For every file in .github/workflows it checks:

  - every `uses:` names a local file (`./...`) or an action at a full 40-character commit: no tag (`@v4`), no branch
    (`@main`), no `@latest`. A tag can be moved to other code by whoever controls the action; a commit cannot.
    One exception is listed below: upstream's own docs-links.yml, which the fork keeps byte-identical.
  - an action is pinned to the same commit in every workflow (after an upstream merge, re-pin test.yml and tidy.yml
    to the commits upstream's files use: docs/UPSTREAM_MERGE.md);
  - the workflow has a top-level `permissions:` that grants nothing but read access. A job that has to write
    (the release jobs) raises it for itself;
  - every `actions/checkout` step sets `persist-credentials: false`, so the job token is not left in the checkout's
    git config for later steps.

It is a text check, not a YAML parser: it reads `uses:`, `permissions:` and list items line by line. A workflow
written in an unusual layout (flow style, a `uses:` split over lines) can trip it or slip by; keep the plain layout.

What it does not look at: a job's own `permissions:` (the two release jobs of build.yml raise theirs to
`contents: write`, as they must; a third job that did the same would pass this check).

Wired into CI as the fast job "Workflow and release-tool checks" in .github/workflows/test.yml.

Exit codes:
  0 - every workflow passes (with --self-test: and every doctored copy was reported).
  1 - a workflow breaks a rule (each case is listed), or the self-test failed.
  2 - .github/workflows was not found.

Usage:
  python3 tools/check_workflows.py [--root DIR] [--self-test]
"""

import argparse
import re
import sys
import tempfile
from pathlib import Path

WORKFLOWS = ".github/workflows"

# (workflow file, the `uses:` value) pairs that may stay on a tag. Only for files the fork keeps byte-identical to
# upstream's; drop the entry when upstream pins the action.
EXCEPTIONS = {("docs-links.yml", "lycheeverse/lychee-action@v2")}

USES = re.compile(r"""^[ \t]*(?:-[ \t]+)?uses:[ \t]*["']?([^"'\s#]+)["']?""")
PINNED = re.compile(r"^[\w.-]+/[\w./-]+@[0-9a-f]{40}$")
ITEM = re.compile(r"^([ \t]*)-[ \t]")
TOP_KEY = re.compile(r"^[A-Za-z_'\"][^#\n]*:")
PERMISSIONS = re.compile(r"^permissions:[ \t]*([^#\n]*)")
GRANT = re.compile(r"""^[ \t]+([\w-]+):[ \t]*["']?([\w-]+)["']?""")
PERSIST = re.compile(r"^[ \t]*persist-credentials:[ \t]*false\b")


def is_comment(line: str) -> bool:
    return line.lstrip().startswith("#")


def check_file(name: str, text: str, pins: dict) -> list:
    """The findings for one workflow. `pins` collects action -> {commit: [files]} over all files."""
    found = []
    lines = text.splitlines()

    # uses: pins
    for no, line in enumerate(lines, 1):
        if is_comment(line):
            continue
        m = USES.match(line)
        if not m:
            continue
        value = m.group(1)
        if value.startswith("./"):
            continue
        if (name, value) in EXCEPTIONS:
            continue
        if not PINNED.match(value):
            found.append(f"{name}:{no}: `uses: {value}` is not pinned to a full commit (40 hex characters): a tag, a "
                         f"branch or `@latest` runs whatever code it points to on the day of the run.")
            continue
        action, commit = value.rsplit("@", 1)
        pins.setdefault(action.lower(), {}).setdefault(commit, []).append(f"{name}:{no}")

    # top-level permissions
    at = next((i for i, line in enumerate(lines) if PERMISSIONS.match(line)), None)
    if at is None:
        found.append(f"{name}: no top-level `permissions:`: every job then gets the repository's default token "
                     f"rights. Add `permissions:` with `contents: read`; a job that must write raises it itself.")
    else:
        inline = PERMISSIONS.match(lines[at]).group(1).strip()
        if inline:
            if inline not in ("read-all", "{}"):
                found.append(f"{name}:{at + 1}: top-level `permissions: {inline}` grants more than read access.")
        else:
            for i in range(at + 1, len(lines)):
                line = lines[i]
                if not line.strip() or is_comment(line):
                    continue
                if not line.startswith((" ", "\t")):
                    break
                g = GRANT.match(line)
                if g and g.group(2) not in ("read", "none"):
                    found.append(f"{name}:{i + 1}: top-level `permissions` grants `{g.group(1)}: {g.group(2)}`: only "
                                 f"read access belongs there; a job that must write raises it for itself.")

    # actions/checkout steps keep no credentials
    i = 0
    while i < len(lines):
        m = ITEM.match(lines[i]) if not is_comment(lines[i]) else None
        if not m:
            i += 1
            continue
        indent = len(m.group(1).expandtabs())
        end = i + 1
        while end < len(lines):
            nxt = lines[end]
            if nxt.strip() and not is_comment(nxt):
                if len(nxt.expandtabs()) - len(nxt.expandtabs().lstrip()) <= indent:
                    break
            end += 1
        block = [line for line in lines[i:end] if not is_comment(line)]
        uses = next((USES.match(line) for line in block if USES.match(line)), None)
        if uses and uses.group(1).lower().startswith("actions/checkout@"):
            if not any(PERSIST.match(line) for line in block):
                found.append(f"{name}:{i + 1}: this `actions/checkout` step lacks `persist-credentials: false`: the "
                             f"job token stays in the checkout's git config for every later step.")
        i = end
    return found


def check_tree(root: Path):
    """(findings, None), or (None, error) when the folder is missing."""
    folder = root / WORKFLOWS
    if not folder.is_dir():
        return None, f"{folder} not found"
    found = []
    pins = {}
    for path in sorted(list(folder.glob("*.yml")) + list(folder.glob("*.yaml"))):
        found += check_file(path.name, path.read_text(encoding="utf-8"), pins)
    for action, commits in sorted(pins.items()):
        if len(commits) > 1:
            where = "; ".join(f"{commit[:12]}… in {', '.join(places)}" for commit, places in sorted(commits.items()))
            found.append(f"`{action}` is pinned to {len(commits)} different commits ({where}): use one commit in "
                         f"every workflow (after an upstream merge: the commit upstream's files use).")
    return found, None


# ---------------------------------------------------------------------------------------------------------------
# self-test
# ---------------------------------------------------------------------------------------------------------------

SHA_A = "3d3c42e5aac5ba805825da76410c181273ba90b1"
SHA_B = "043fb46d1a93c77aae656e7c1c64a875d1fc6a0a"

GOOD_BUILD = f"""---
name: "Build"

on:
  push:
    paths:
      - 'src/**'

permissions:
  contents: read

jobs:
  tests:
    uses: ./.github/workflows/test.yml

  build:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@{SHA_A} # v7.0.1
        with:
          persist-credentials: false
          fetch-depth: 0
      # - uses: some/old-action@latest
      - uses: actions/upload-artifact@{SHA_B} # v7.0.1
        with:
          name: x

  release:
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - run: gh release create
"""

GOOD_TEST = f"""---
name: "Tests"
on:
  workflow_call:
permissions:
  contents: read
jobs:
  unit:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@{SHA_A} # v7.0.1
        with:
          persist-credentials: false
      - run: make test
"""

GOOD_DOCS = """---
name: "Docs links"
on:
  push:
permissions:
  contents: read
jobs:
  links:
    runs-on: ubuntu-latest
    steps:
      - uses: lycheeverse/lychee-action@v2
"""


def self_test() -> int:
    good = {"build.yml": GOOD_BUILD, "test.yml": GOOD_TEST, "docs-links.yml": GOOD_DOCS}

    def run(files: dict) -> list:
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp) / WORKFLOWS
            folder.mkdir(parents=True)
            for name, text in files.items():
                (folder / name).write_text(text, encoding="utf-8")
            found, error = check_tree(Path(tmp))
            return found if error is None else [error]

    def with_change(name: str, old: str, new: str) -> dict:
        assert old in good[name], f"self-test case: {old!r} is not in {name}"
        files = dict(good)
        files[name] = good[name].replace(old, new, 1)
        return files

    # (label, files, the text the report must contain; None = no finding at all)
    cases = [
        ("the good set", good, None),
        ("a tag", with_change("build.yml", f"actions/upload-artifact@{SHA_B}", "actions/upload-artifact@v4"),
         "is not pinned to a full commit"),
        ("@latest in quotes", with_change("build.yml", f"uses: actions/upload-artifact@{SHA_B}",
                                          'uses: "marvinpinto/action-automatic-releases@latest"'),
         "is not pinned to a full commit"),
        ("a branch", with_change("test.yml", f"actions/checkout@{SHA_A}", "actions/checkout@main"),
         "is not pinned to a full commit"),
        ("a short commit", with_change("test.yml", f"actions/checkout@{SHA_A}", "actions/checkout@3d3c42e"),
         "is not pinned to a full commit"),
        ("the listed exception in another file",
         with_change("test.yml", "      - run: make test", "      - uses: lycheeverse/lychee-action@v2"),
         "is not pinned to a full commit"),
        ("two commits for one action", with_change("test.yml", f"actions/checkout@{SHA_A}",
                                                   "actions/checkout@" + "a" * 40),
         "is pinned to 2 different commits"),
        ("no permissions", with_change("test.yml", "permissions:\n  contents: read\n", ""),
         "no top-level `permissions:`"),
        ("permissions: write-all", with_change("test.yml", "permissions:\n  contents: read", "permissions: write-all"),
         "grants more than read access"),
        ("top-level contents: write", with_change("build.yml", "permissions:\n  contents: read\n\njobs:",
                                                  "permissions:\n  contents: write\n\njobs:"),
         "grants `contents: write`"),
        ("a second top-level grant", with_change("build.yml", "permissions:\n  contents: read\n\njobs:",
                                                 "permissions:\n  contents: read\n  packages: write\n\njobs:"),
         "grants `packages: write`"),
        ("a top-level grant in quotes", with_change("build.yml", "permissions:\n  contents: read\n\njobs:",
                                                    "permissions:\n  contents: \"write\"\n\njobs:"),
         "grants `contents: write`"),
        ("checkout that keeps the token",
         with_change("build.yml", "          persist-credentials: false\n          fetch-depth: 0", "          fetch-depth: 0"),
         "lacks `persist-credentials: false`"),
        ("checkout with persist-credentials: true",
         with_change("test.yml", "persist-credentials: false", "persist-credentials: true"),
         "lacks `persist-credentials: false`"),
        ("the setting only in a comment",
         with_change("test.yml", "          persist-credentials: false", "          # persist-credentials: false"),
         "lacks `persist-credentials: false`"),
    ]
    failed = []
    for label, files, wanted in cases:
        found = run(files)
        if wanted is None:
            if found:
                failed.append(f"{label}: expected no finding, got: {' | '.join(found)}")
        elif not any(wanted in line for line in found):
            failed.append(f"{label}: expected a finding with {wanted!r}, got: {' | '.join(found) or 'none'}")
        elif len(found) != 1:
            failed.append(f"{label}: expected exactly 1 finding, got {len(found)}: {' | '.join(found)}")
    for line in failed:
        print("SELF-TEST FAIL " + line)
    if not failed:
        print(f"self-test ok: {len(cases)} cases")
    return 1 if failed else 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Check the GitHub workflows' pins, permissions and checkouts.")
    parser.add_argument("--root", default=None, help="the repository root (default: the parent of tools/)")
    parser.add_argument("--self-test", action="store_true", help="run the built-in cases and exit")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    root = Path(args.root) if args.root else Path(__file__).resolve().parent.parent
    found, error = check_tree(root)
    if error:
        print("check_workflows: " + error, file=sys.stderr)
        return 2
    for line in found:
        print(line)
    if found:
        print(f"\n{len(found)} finding(s)")
        return 1
    print("workflows ok: every action pinned to a commit, read-only default token, no stored credentials")
    return 0


if __name__ == "__main__":
    sys.exit(main())
