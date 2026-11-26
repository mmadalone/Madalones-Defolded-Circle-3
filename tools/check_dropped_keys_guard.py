#!/usr/bin/env python3
# Copyright (c) 2026 madalone. Drift check for the keys the screensaver still has down when it closes.
# SPDX-License-Identifier: GPL-3.0-or-later
"""Drift check for the keys the screensaver still has down when it closes (v1.6.5).

History
-------
InputController routes a key to the item that had the input at its press: the auto-repeat presses and the release of
that key go to the same item, or, once that item is destroyed, to the item in front. The screensaver is destroyed
right after it closes, and something else can close it under a key that is still down (a wake, a lift, a tap, another
key). Up to v1.6.4 the page behind then got the rest of that hold: a held BACK went back page by page, a cover page
opened its blind on the release.

Since v1.6.5 the screensaver's onClosed hands the keys it still has down to InputController::dropPendingKeys(). Those
keys become the controller's own: their auto-repeat presses only count as activity and are consumed, and their release
goes out with the controller as its owner, so it reaches no scope but still reaches the handlers that listen to every
owner (main.qml's VOICE handler has to see the release of a press it saw).

src/ui/inputController.{h,cpp} are upstream files. A merge that drops all of it, or only the block in eventFilter()
or the onClosed line, still builds. The behaviour is tested on the real controller by
test/screensaver/test_input_controller_dropped_keys.cpp and by test/qml/tst_screensaver_keys.qml (the real
ChargingScreen.qml and ButtonNavigation.qml). This check runs in seconds without Qt, so a merge that drops a part is
named at once. It checks:

  - src/ui/inputController.h declares `Q_INVOKABLE void dropPendingKeys(QObject* owner);` in the public part of the
    class (QML calls it, and QML does not see a private method);
  - src/ui/inputController.cpp: dropPendingKeys() re-points the entries of `owner` in m_keyOwner at the controller
    (`if (it.value().data() == owner) {` and `it.value() = this;`);
  - eventFilter(), KeyPress branch: the block `if (keyEvent->isAutoRepeat() && m_keyOwner.value(key).data() == this) {`
    sits after `cancelDeferredRelease(key);` and before `setKeypadActive(true);`, holds `emit keyPressed(mappedKey);`,
    `event->accept();` and `return true;`, and emits no keyPressedFor;
  - upstream's own lines the design relies on are still there: `m_keyOwner[key] = owner;` (a new press overwrites
    the mark), `QPointer<QObject> owner = m_keyOwner.take(key);` and `emit keyReleasedFor(owner.data(), mappedKey);`
    (the release takes the mark and goes out with it), and `timer->setInterval(150);` (how long a release flagged as
    auto-repeat is held back: the C++ test has a copy of that time);
  - src/qml/components/ChargingScreen.qml: the onClosed that gives the input back
    (`if (buttonNavigation) buttonNavigation.releaseControl();`) also calls
    `if (buttonNavigation) ui.inputController.dropPendingKeys(buttonNavigation.scope);`. The order of the two is not
    checked: the keys stay the popup's until it is destroyed, so the drop works before or after it.

What it does not look at, because the two tests do: the first lines of dropPendingKeys() (a null owner and the
controller itself are left alone), the rest of emitKeyRelease() (its fallback to the item in front for a key without
an owner), the info line and the debug line, and what the 3 s hold of POWER does. An edit there leaves this check
green and fails a test.

A statement counts only as live code at the start of a line, so a commented-out line is reported as missing; what
follows the statement on its line (a comment) is not looked at. It is a text check: a reformat that breaks one of
these lines up also trips it. Then compare with the two tests and update this script.

Wired into CI as the fast job "dropped-keys-guard" in .github/workflows/test.yml.

Exit codes:
  0 - every part is in place (with --self-test: and every doctored copy was reported).
  1 - a part is missing or changed (each one is listed), or, with --self-test, the self-test failed (each case is
      listed).
  2 - a file was not found (the repository layout changed).

Usage:
  python3 tools/check_dropped_keys_guard.py [--root DIR] [--self-test]
"""

import argparse
import re
import sys
from pathlib import Path

HEADER = "src/ui/inputController.h"
SOURCE = "src/ui/inputController.cpp"
POPUP = "src/qml/components/ChargingScreen.qml"
FILES = (HEADER, SOURCE, POPUP)

DECLARATION = "Q_INVOKABLE void dropPendingKeys(QObject* owner);"
METHOD = "void InputController::dropPendingKeys(QObject* owner) {"
OWNER_TEST = "if (it.value().data() == owner) {"
REPOINT = "it.value() = this;"
FILTER = "bool InputController::eventFilter(QObject* obj, QEvent* event) {"
RELEASE = "void InputController::emitKeyRelease(int key, const QString& mappedKey) {"
KEY_PRESS = "case QEvent::KeyPress: {"
CANCEL = "cancelDeferredRelease(key);"
BLOCK = "if (keyEvent->isAutoRepeat() && m_keyOwner.value(key).data() == this) {"
KEYPAD = "setKeypadActive(true);"
ACTIVITY = "emit keyPressed(mappedKey);"
ACCEPT = "event->accept();"
CONSUME = "return true;"
UPSTREAM_PRESS = "m_keyOwner[key] = owner;"
UPSTREAM_TAKE = "QPointer<QObject> owner = m_keyOwner.take(key);"
UPSTREAM_RELEASE = "emit keyReleasedFor(owner.data(), mappedKey);"
UPSTREAM_INTERVAL = "timer->setInterval(150);"
DROP_CALL = "if (buttonNavigation) ui.inputController.dropPendingKeys(buttonNavigation.scope);"
GIVE_BACK = "if (buttonNavigation) buttonNavigation.releaseControl();"

# an access label of a class: " public:", " private slots:", " signals:" ...
ACCESS_LABEL = re.compile(r"^[ \t]*(public|protected|private|signals|Q_SIGNALS)(?:[ \t]+(?:slots|Q_SLOTS))?[ \t]*:",
                          re.MULTILINE)


def line_pos(text: str, stmt: str, start: int = 0) -> int:
    """Position of `stmt` as live code at the start of a line (after indentation only) from `start` on, or -1."""
    m = re.compile(r"^[ \t]*" + re.escape(stmt), re.MULTILINE).search(text, start)
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


def block_of(text: str, opening: str) -> str:
    """The block that starts with the live line `opening` (empty if there is none)."""
    at = line_pos(text, opening)
    return block_after(text, at) if at >= 0 else ""


def check_header(text: str) -> list:
    at = line_pos(text, DECLARATION)
    if at < 0:
        if line_pos(text, DECLARATION.replace("Q_INVOKABLE ", "")) >= 0:
            return [f"{HEADER}: dropPendingKeys() is no longer Q_INVOKABLE: the screensaver's onClosed can't call it "
                    f"and throws a TypeError before it gives the input back; the page behind then gets the rest of "
                    f"the hold again."]
        return [f"{HEADER}: no `{DECLARATION}` (v1.6.5): the keys the screensaver still has down when it closes go "
                f"to the page behind again."]
    section = "no access label"
    for label in ACCESS_LABEL.finditer(text, 0, at):
        section = label.group(1)
    if section != "public":
        return [f"{HEADER}: `{DECLARATION}` stands under `{section}`, not in the public part of the class: QML does "
                f"not see it there, the screensaver's onClosed throws a TypeError before it gives the input back, "
                f"and the page behind gets the rest of the hold again."]
    return []


def check_source(text: str) -> list:
    errors = []

    method = block_of(text, METHOD)
    if not method:
        errors.append(f"{SOURCE}: no `{METHOD}` (v1.6.5).")
    else:
        if line_pos(method, OWNER_TEST) < 0:
            errors.append(f"{SOURCE}: dropPendingKeys() lost `{OWNER_TEST}`: it must take the keys of that owner "
                          f"only, a key held on the page behind stays the page's.")
        if line_pos(method, REPOINT) < 0:
            errors.append(f"{SOURCE}: dropPendingKeys() lost `{REPOINT}`: the keys must become the controller's "
                          f"own; a cleared entry sends the rest of the hold to the item in front.")

    event_filter = block_of(text, FILTER)
    key_press = block_of(event_filter, KEY_PRESS)
    if not key_press:
        errors.append(f"{SOURCE}: no `{KEY_PRESS}` branch in eventFilter() any more: the routing changed, look at "
                      f"dropPendingKeys() again.")
    else:
        cancel_at = line_pos(key_press, CANCEL)
        block_at = line_pos(key_press, BLOCK)
        keypad_at = line_pos(key_press, KEYPAD)
        if block_at < 0:
            errors.append(f"{SOURCE}: eventFilter() lost the block `{BLOCK}` (v1.6.5): the auto-repeat presses of "
                          f"a dropped key are routed again, and a handler that listens to every owner takes each "
                          f"one for a press.")
        else:
            if cancel_at < 0 or cancel_at > block_at:
                errors.append(f"{SOURCE}: the dropped-repeat block must come after `{CANCEL}`: on a desktop "
                              f"keyboard the held-back release of a release/press pair otherwise goes out in the "
                              f"middle of the hold.")
            if keypad_at < 0 or keypad_at < block_at:
                errors.append(f"{SOURCE}: the dropped-repeat block must come before `{KEYPAD}`: a dropped repeat "
                              f"must not bring the keypad selection back.")
            block = block_after(key_press, block_at)
            for stmt, why in ((ACTIVITY, "a held button no longer counts as activity"),
                              (ACCEPT, "the dropped repeat is not marked as handled"),
                              (CONSUME, "the dropped repeat goes on to the QML focus chain of the screen in front")):
                if line_pos(block, stmt) < 0:
                    errors.append(f"{SOURCE}: the dropped-repeat block lost `{stmt}`: {why}.")
            if "keyPressedFor" in block:
                errors.append(f"{SOURCE}: the dropped-repeat block emits keyPressedFor: a dropped repeat must reach "
                              f"no handler.")
        if line_pos(key_press, UPSTREAM_PRESS) < 0:
            errors.append(f"{SOURCE}: upstream's `{UPSTREAM_PRESS}` is gone from the KeyPress branch: a new press "
                          f"of a dropped key must overwrite its mark. Look at dropPendingKeys() again.")

    if event_filter and line_pos(event_filter, UPSTREAM_INTERVAL) < 0:
        errors.append(f"{SOURCE}: upstream's `{UPSTREAM_INTERVAL}` is gone from eventFilter(), or the time changed: "
                      f"kDeferredReleaseMs in test/screensaver/test_input_controller_dropped_keys.cpp is a copy of "
                      f"it. Set it to the new time, or the test no longer shows that the dropped-repeat block runs "
                      f"after `{CANCEL}`.")

    release = block_of(text, RELEASE)
    for stmt in (UPSTREAM_TAKE, UPSTREAM_RELEASE):
        if line_pos(release, stmt) < 0:
            errors.append(f"{SOURCE}: upstream's `{stmt}` is gone from emitKeyRelease(): the release of a dropped "
                          f"key must take its mark and go out with the controller as its owner. Look at "
                          f"dropPendingKeys() again.")
    return errors


def check_popup(text: str) -> list:
    closed = ""
    for m in re.finditer(r"^[ \t]*onClosed:\s*\{", text, re.MULTILINE):
        block = block_after(text, m.start())
        if line_pos(block, GIVE_BACK) >= 0:
            closed = block
            break
    if not closed:
        return [f"{POPUP}: no onClosed with `{GIVE_BACK}` any more: check by hand that the keys still down are "
                f"dropped where the screensaver gives the input back."]
    if line_pos(closed, DROP_CALL) < 0:
        return [f"{POPUP}: onClosed lost `{DROP_CALL}` (v1.6.5): the page behind gets the repeats and the release "
                f"of a key that was down when the screensaver closed."]
    return []


def check(texts: dict) -> list:
    """Every drift in the three files, given as {relative path: text}."""
    return check_header(texts[HEADER]) + check_source(texts[SOURCE]) + check_popup(texts[POPUP])


def line_index(lines: list, stmt: str, start: int = 0) -> int:
    """Index of the first line from `start` on that starts with `stmt`, indentation aside, as line_pos() matches."""
    for i in range(start, len(lines)):
        if lines[i].lstrip(" \t").startswith(stmt):
            return i
    raise LookupError(f"no line starts with `{stmt}`")


def edited(text: str, stmt: str, new, after: str = "") -> str:
    """`text` with the line that starts with `stmt` replaced by `new`, at the same indentation (None takes the line
    out). With `after`, the first such line below the line `after`; else the first one."""
    lines = text.split("\n")
    at = line_index(lines, stmt, line_index(lines, after) if after else 0)
    indent = lines[at][:len(lines[at]) - len(lines[at].lstrip())]
    return "\n".join(lines[:at] + ([] if new is None else [indent + new]) + lines[at + 1:])


def moved(text: str, block: str, before: str = "", after: str = "") -> str:
    """`text` with the lines of `block` moved to just before the line `before`, or to just after the line `after`."""
    lines = text.split("\n")
    first = block.strip().split("\n")[0].strip()
    n = len(block.strip().split("\n"))
    at = line_index(lines, first)
    rest = lines[:at] + lines[at + n:]
    to = line_index(rest, before or after) + (1 if after else 0)
    return "\n".join(rest[:to] + lines[at:at + n] + rest[to:])


def self_test(texts: dict) -> int:
    """Doctors one part at a time in a copy of the three texts: each must be reported, and by the right message.
    Does it once on the tree as it is and once on a copy with comments behind guarded lines and the two onClosed
    lines swapped, which the check must accept."""

    def line(stmt: str, new, after: str = ""):
        return lambda t: edited(t, stmt, new, after)

    def block_of_repeats(t: str) -> str:
        return block_of(block_of(block_of(t, FILTER), KEY_PRESS), BLOCK)

    doctored = [
        ("the declaration is not Q_INVOKABLE", HEADER, line(DECLARATION, DECLARATION.replace("Q_INVOKABLE ", "")),
         "no longer Q_INVOKABLE"),
        ("the declaration is gone", HEADER, line(DECLARATION, None), "no `Q_INVOKABLE void dropPendingKeys"),
        ("the declaration is commented out", HEADER, line(DECLARATION, "// " + DECLARATION),
         "no `Q_INVOKABLE void dropPendingKeys"),
        ("the declaration stands under private:", HEADER, lambda t: moved(t, DECLARATION, after="private:"),
         "not in the public part"),
        ("the method is gone", SOURCE, line(METHOD, "void InputController::dropNothing(QObject* owner) {"),
         "no `void InputController::dropPendingKeys"),
        ("every key is dropped", SOURCE, line(OWNER_TEST, "if (true) {"), "the keys of that owner"),
        ("the keys are forgotten", SOURCE, line(REPOINT, "it.value() = nullptr;"), "become the controller's"),
        ("the block is gone", SOURCE, line(BLOCK, "if (false) {"), "lost the block"),
        ("the block is commented out", SOURCE, line(BLOCK, "// " + BLOCK), "lost the block"),
        ("the block runs before the deferred release is cancelled", SOURCE,
         lambda t: moved(t, block_of_repeats(t), before=CANCEL), "must come after"),
        ("the block runs after the keypad state", SOURCE, lambda t: moved(t, block_of_repeats(t), after=KEYPAD),
         "must come before"),
        ("a dropped repeat is no activity", SOURCE, line(ACTIVITY, None, after=BLOCK),
         "no longer counts as activity"),
        ("a dropped repeat is not accepted", SOURCE, line(ACCEPT, "// " + ACCEPT, after=BLOCK),
         "not marked as handled"),
        ("a dropped repeat is not consumed", SOURCE, line(CONSUME, "break;", after=BLOCK),
         "goes on to the QML focus chain"),
        ("a dropped repeat is routed", SOURCE,
         line(ACTIVITY, ACTIVITY + " emit keyPressedFor(this, mappedKey);", after=BLOCK), "emits keyPressedFor"),
        ("the KeyPress branch is written differently", SOURCE, line(KEY_PRESS, "case QEvent::KeyPress:  {"),
         "no `case QEvent::KeyPress: {` branch"),
        ("upstream no longer records the owner of a press", SOURCE, line(UPSTREAM_PRESS, "m_owner[key] = owner;"),
         "must overwrite its mark"),
        ("upstream no longer takes the owner at the release", SOURCE,
         line(UPSTREAM_TAKE, "QPointer<QObject> owner = m_keyOwner.value(key);"), "must take its mark"),
        ("upstream no longer sends the release with its owner", SOURCE,
         line(UPSTREAM_RELEASE, "emit keyReleasedFor(m_activeItem.data(), mappedKey);"), "must take its mark"),
        ("upstream holds a flagged release back for another time", SOURCE,
         line(UPSTREAM_INTERVAL, "timer->setInterval(300);"), "kDeferredReleaseMs"),
        ("onClosed drops nothing", POPUP, line(DROP_CALL, None), "onClosed lost"),
        ("the drop is commented out", POPUP, line(DROP_CALL, "// " + DROP_CALL), "onClosed lost"),
        ("onClosed no longer gives the input back", POPUP, line(GIVE_BACK, None), "check by hand"),
    ]

    # edits the check must accept: a comment behind a guarded line, and the two onClosed lines the other way round
    harmless = [
        (HEADER, line(DECLARATION, DECLARATION + "  // called by ChargingScreen.qml")),
        (SOURCE, line(REPOINT, REPOINT + "  // the controller owns it now")),
        (SOURCE, line(UPSTREAM_PRESS, UPSTREAM_PRESS + "  // a new press takes the key back")),
        (SOURCE, line(KEYPAD, KEYPAD + "  // from the first key press on")),
        (POPUP, line(DROP_CALL, DROP_CALL + "  // before the popup is destroyed")),
        (POPUP, lambda t: moved(t, DROP_CALL, after=GIVE_BACK)),
    ]

    failures = []
    try:
        commented = dict(texts)
        for rel, edit in harmless:
            commented[rel] = edit(commented[rel])
    except LookupError as error:
        failures.append(f"the copy with comments behind guarded lines could not be made: {error}")
        commented = None
    if commented is not None and check(commented):
        failures.append(f"a comment behind a guarded line, or the two onClosed lines swapped, is reported: "
                        f"{check(commented)}")
        commented = None

    trees = [("the tree as it is", texts)] + ([("the tree with comments behind guarded lines", commented)]
                                              if commented is not None else [])
    for tree, base in trees:
        for name, rel, doctor, expected in doctored:
            copy = dict(base)
            try:
                copy[rel] = doctor(base[rel])
            except LookupError as error:
                failures.append(f"{name} ({tree}): nothing to doctor in {rel}, {error} (was the line reformatted?)")
                continue
            if copy[rel] == base[rel]:
                failures.append(f"{name} ({tree}): the doctoring changed nothing in {rel}")
                continue
            errors = check(copy)
            if not errors:
                failures.append(f"{name} ({tree}): not reported")
            elif not any(expected in e for e in errors):
                failures.append(f"{name} ({tree}): reported, but not as `{expected}`: {errors}")

    if failures:
        print("Dropped keys guard self-test: FAILED")
        for f in failures:
            print(f"  - {f}")
        return 1
    print(f"Dropped keys guard self-test: OK ({len(doctored)} doctored copies, each one reported, on the tree as it "
          f"is and on a copy with {len(harmless)} harmless edits, which is accepted)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1],
                        help="repository root to check (default: this repository)")
    parser.add_argument("--self-test", action="store_true",
                        help="after the check: prove that it reports doctored copies of the files and accepts "
                             "harmless edits")
    args = parser.parse_args()
    root = args.root.resolve()

    missing = [root / rel for rel in FILES if not (root / rel).exists()]
    if missing:
        for p in missing:
            print(f"FILE NOT FOUND: {p}")
        return 2
    texts = {rel: (root / rel).read_text(encoding="utf-8") for rel in FILES}

    errors = check(texts)
    if errors:
        print("Dropped keys guard: DRIFT")
        for e in errors:
            print(f"  - {e}")
        print("See test/screensaver/test_input_controller_dropped_keys.cpp, test/qml/tst_screensaver_keys.qml and "
              "docs/UPSTREAM_MERGE.md (Input).")
        return 1
    if args.self_test:
        return self_test(texts)
    print(f"Dropped keys guard: OK ({len(FILES)} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
