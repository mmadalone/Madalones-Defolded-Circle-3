#!/usr/bin/env python3
# Copyright (c) 2026 madalone. Drift check for the wiring of the quick undock check.
# SPDX-License-Identifier: GPL-3.0-or-later
"""Drift check for the wiring of the quick undock check (v1.6.5, GitHub issue #3).

History
-------
The quick undock check closes the screensaver soon after the remote is lifted off its charger. Its logic is in our own
files (src/hardware/undockCheck.{h,cpp}, src/ui/screensaverClosePolicy.h) and has unit tests. What connects it to the
app is spread over three upstream files that no automated test loads:

  - src/qml/main.qml tells it when the screensaver is open, closes the screensaver when it asks, and opens no
    screensaver while it waits for the core to confirm a lift (UndockCheck.undockPending);
  - src/main.cpp hands it the "Close on wake" choice and the "Undock delay", at start-up and on every change;
  - src/hardware/hardwareController.cpp creates it, connects it to the power mode, the charger report and the
    connection, tells the phantom-wake check about a close, and registers it for QML.

A merge of a new upstream version rewrites those files, and a line dropped there breaks the feature without a failing
test: the v1.6.0 merge dropped a touch slider guard that way (tools/check_touchslider_guards.py). This check runs in
seconds without Qt and names each missing piece. It checks:

  src/qml/main.qml
  - `import UndockCheck 1.0`;
  - a `Binding` on UndockCheck's `screensaverOpen` whose value is the screensaver's `opened`;
  - a `Connections` on UndockCheck whose onCloseRequested calls `root._closeScreensaverNow();`;
  - dockedRearmTimer and dockChimeGraceTimer wait (`restart(); return;`) while `UndockCheck.undockPending`, before
    they open the screensaver;
  - the wake branch's condition is, white space aside, exactly
    `else if (!chargingScreenLoader.active && ScreensaverConfig.reopensOnWake(fromPowerMode === PowerModes.Idle,
    Battery.isCharging, Battery.powerSupply, UndockCheck.undockPending))`: what the rule is handed, in which place, and
    that its answer is not negated (upstream's own first term is the opposite one, `fromPowerMode !== PowerModes.Idle`);
    its body opens the screensaver and stops dockedRearmTimer;
  - `chargingScreenLoader.active` is assigned something other than `false` in exactly five places, wherever the
    assignment stands on its line (the idle timer, the two timers above, the dock handler, the wake branch): a sixth
    place that opens the screensaver would need the same guard;
  - the Loader's own binding is `active: HwInfo.modelNumber === "DEV"` (the desktop simulator only).

  src/main.cpp
  - the handlers of ScreensaverConfig's closeOnWakeChanged and undockCloseMsChanged push the new value;
  - the three pushes at start-up, in this order: setUndockCloseMs, onPowerSupplyChanged, setCloseOnWake (the last one
    is the one that can start the check, so it must find the delay and the charger already set).

  src/hardware/hardwareController.cpp
  - `m_undockCheck = new UndockCheck(m_core, this);`
  - its connections to Power::powerModeChanged, Battery::powerSupplyChanged and core::Api::disconnected;
  - UndockCheck::closeRequested connected to PhantomWakeSuppressor::onQuickUndock;
  - the order: Power, Battery and the phantom-wake check are created before it, and it before those four connects (a
    connect to a pointer that is not set yet connects nothing);
  - the QML singleton registration.

A statement counts only as live code: one that is commented out, with // or inside /* ... */, is reported as missing.
It is a text check: a harmless reformat of these lines also trips it. Then compare with this list and update the
script.

What it cannot see. It reads three files as text and does not run the app, so it does not notice:
  - a statement that is there but not reached or not in force: `enabled: false` on the Connections, another condition
    around the close in onCloseRequested, C++ lines inside `#if 0`;
  - a value it does not compare in full: the Binding's value is checked for its two pieces only (a `!` in front of
    one of them passes);
  - an opener written another way: a Binding or a State on the Loader's `active`, `chargingScreenLoader["active"]`,
    the Loader reached under another name or from another file;
  - anything in other files, and whether ScreensaverConfig.reopensOnWake() can be called from QML at all
    (test/screensaver/test_screensaver_config.cpp calls it from JavaScript).
After such a merge the proof is still a run of the app, and more than one lift: see docs/UPSTREAM_MERGE.md.

--self-test proves that each check can fail: it takes the three files, breaks one piece at a time in memory and
expects the check to name it. Every kind of message the check can give (KINDS) has at least one such case, and the
self-test fails if one has none. It also fails when a piece it wants to break is no longer there as written (this
script is out of step with the code), and when the files don't pass the check as they are; it then prints what the
check found, so its output names the missing piece too.

Wired into CI as the fast job "undock-wiring" in .github/workflows/test.yml: the check first, then the self-test.

Exit codes:
  0 - everything is in place (or the self-test passed).
  1 - a piece is missing or changed (each one is listed), or the self-test failed.
  2 - a file was not found (the repository layout changed).

Usage:
  python3 tools/check_undock_wiring.py [--root DIR]
  python3 tools/check_undock_wiring.py --self-test [--root DIR]
"""

import argparse
import re
import sys
from pathlib import Path

MAIN_QML = "src/qml/main.qml"
MAIN_CPP = "src/main.cpp"
HW_CTRL = "src/hardware/hardwareController.cpp"
FILES = (MAIN_QML, MAIN_CPP, HW_CTRL)

# every kind of message the check can give; the self-test must have a case for each
KINDS = ("import", "binding", "binding-property", "binding-value", "close-handler", "close-call", "timer",
         "timer-guard", "timer-guard-step", "wake", "wake-active", "wake-pending", "wake-condition", "wake-body",
         "openers", "loader-active", "handler", "handler-push", "startup-push", "startup-order", "hardware",
         "hardware-order")

OPEN = "chargingScreenLoader.active = true;"
PENDING_GUARD = "if (UndockCheck.undockPending) {"
OPENERS = 5
# the whole condition of the wake branch, white space collapsed
WAKE_CONDITION = ("else if (!chargingScreenLoader.active && ScreensaverConfig.reopensOnWake(fromPowerMode === "
                  "PowerModes.Idle, Battery.isCharging, Battery.powerSupply, UndockCheck.undockPending))")
# the Loader's own binding: true in the desktop simulator only
LOADER_ACTIVE = 'HwInfo.modelNumber === "DEV"'
# an assignment to the Loader's `active` (not a comparison), and the one kind that opens nothing
ASSIGNMENT = re.compile(r"chargingScreenLoader\s*\.\s*active\s*[-+*/%&|^]?=(?!=)")
ASSIGNS_FALSE = re.compile(r"chargingScreenLoader\s*\.\s*active\s*=[ \t]*false[ \t]*(?:;|\}|$)", re.MULTILINE)
# (timer id, what it is for the user)
GUARDED_TIMERS = (("dockedRearmTimer", '"Run after dismissal while docked"'),
                  ("dockChimeGraceTimer", "the dock chime's grace timer"))

# src/main.cpp
ON_CLOSE_ON_WAKE = "QObject::connect(&screensaverConfig, &uc::ScreensaverConfig::closeOnWakeChanged, undockCheck,"
ON_UNDOCK_DELAY = "QObject::connect(&screensaverConfig, &uc::ScreensaverConfig::undockCloseMsChanged, undockCheck,"
PUSH_DELAY = "undockCheck->setUndockCloseMs(screensaverConfig.undockCloseMs());"
PUSH_CHARGER = "undockCheck->onPowerSupplyChanged(hwController.getBattery()->getPowerSupply());"
PUSH_CHOICE = "undockCheck->setCloseOnWake(screensaverConfig.closeOnWake());"

# src/hardware/hardwareController.cpp: statement -> what is lost without it
HW_STATEMENTS = (
    ("m_undockCheck = new UndockCheck(m_core, this);", "there is no quick undock check at all"),
    ("QObject::connect(m_power, &Power::powerModeChanged, m_undockCheck, &UndockCheck::onPowerModeChanged);",
     "it would go on asking while the display is off"),
    ("QObject::connect(m_battery, &Battery::powerSupplyChanged, m_undockCheck, &UndockCheck::onPowerSupplyChanged);",
     "it would never learn that the remote is on the charger, so it would never run"),
    ("QObject::connect(m_core, &core::Api::disconnected, m_undockCheck, &UndockCheck::onCoreDisconnected);",
     "it would go on asking a core that is gone"),
    ("QObject::connect(m_undockCheck, &UndockCheck::closeRequested, m_phantomWakeSuppressor, "
     "&PhantomWakeSuppressor::onQuickUndock);",
     "a quick close would no longer cancel a phantom-wake grace window that is still running, and the display would "
     "go off under a pick-up the quick undock check had already found"),
    ('qmlRegisterSingletonType<UndockCheck>("UndockCheck", 1, 0, "UndockCheck", &UndockCheck::qmlInstance);',
     "main.qml could not reach it"),
)
HW_CREATE = HW_STATEMENTS[0][0]
HW_CONNECTS = tuple(stmt for stmt, _ in HW_STATEMENTS[1:5])
# what the connects above are made to: each must be created before the quick undock check (the start of the statement)
HW_CREATED_BEFORE = ("m_power = new Power(", "m_battery = new Battery(",
                     "m_phantomWakeSuppressor = new PhantomWakeSuppressor(")


class Findings(list):
    """(kind, message) pairs."""

    def add(self, kind: str, message: str) -> None:
        assert kind in KINDS, kind
        self.append((kind, message))


def line_pos(text: str, stmt: str) -> int:
    """Position of `stmt` as live code at the start of a line (after indentation only), or -1."""
    m = re.search(r"^[ \t]*" + re.escape(stmt), text, re.MULTILINE)
    return m.start() if m else -1


def has_line(text: str, pattern: str) -> bool:
    return re.search(r"^[ \t]*" + pattern, text, re.MULTILINE) is not None


def enclosed(text: str, start: int, opener: str, closer: str) -> str:
    """The text from `start` to the `closer` that matches the first `opener` at or after it (empty if unbalanced)."""
    open_at = text.find(opener, start)
    if start < 0 or open_at < 0:
        return ""
    depth = 0
    for i in range(open_at, len(text)):
        if text[i] == opener:
            depth += 1
        elif text[i] == closer:
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    return ""


def block_after(text: str, start: int) -> str:
    return enclosed(text, start, "{", "}")


def blocks(text: str, head: str) -> list:
    """(position, text) of every `head { ... }` whose head starts a line."""
    heads = re.finditer(r"^[ \t]*" + re.escape(head) + r"\s*\{", text, re.MULTILINE)
    return [(m.start(), block_after(text, m.start())) for m in heads]


def block_with(text: str, head: str, line_pattern: str) -> tuple:
    """(position, text) of the first `head { ... }` that holds a line matching `line_pattern`, or (-1, "")."""
    for at, block in blocks(text, head):
        if has_line(block, line_pattern):
            return at, block
    return -1, ""


def timer_block(text: str, timer_id: str) -> tuple:
    return block_with(text, "Timer", r"id:\s*" + re.escape(timer_id) + r"\s*$")


def wake_branch(text: str) -> tuple:
    """(position, condition) of the `else if (...)` that asks ScreensaverConfig.reopensOnWake(), or (-1, "")."""
    for m in re.finditer(r"^[ \t]*else if\s*\(", text, re.MULTILINE):
        condition = enclosed(text, m.start(), "(", ")")
        if "ScreensaverConfig.reopensOnWake(" in condition:
            return m.start(), condition
    return -1, ""


def qml_live(text: str) -> str:
    """QML with every comment blanked, // ... and /* ... */, so that a statement parked inside one is not live code,
    wherever it stands on its line. The text keeps its length and its line breaks, so a position in the result is the
    same position in `text`. String literals are left as they are (a // or /* inside one starts no comment)."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            j = min(j + 1, n)
            out.append(text[i:j])
            i = j
        elif text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("".join("\n" if ch == "\n" else " " for ch in text[i:j]))
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def code_of(text: str) -> str:
    """C++ without comments, all whitespace runs as one space (string literals kept as they are)."""
    out = []
    i, n = 0, len(text)
    while i < n:
        if text[i] == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
        else:
            out.append(text[i])
            i += 1
    return " ".join("".join(out).split())


def check_main_qml(text: str) -> Findings:
    found = Findings()
    label = MAIN_QML
    text = qml_live(text)

    if line_pos(text, "import UndockCheck 1.0") < 0:
        found.add("import", f"{label}: no `import UndockCheck 1.0`: every UndockCheck line below would fail at run "
                            f"time.")

    _, binding = block_with(text, "Binding", r"target:\s*UndockCheck\s*$")
    if not binding:
        found.add("binding", f"{label}: no `Binding {{ target: UndockCheck }}`: the quick undock check would never "
                             f"learn that the screensaver is open, so it would never run.")
    else:
        if not has_line(binding, r'property:\s*"screensaverOpen"\s*$'):
            found.add("binding-property", f"{label}: the Binding on UndockCheck no longer sets the property "
                                          f"`screensaverOpen`.")
        value = re.search(r"^[ \t]*value:(.*)$", binding, re.MULTILINE)
        if not value or "chargingScreenLoader.item !== null" not in value.group(1) \
                or "chargingScreenLoader.item.opened" not in value.group(1):
            found.add("binding-value", f"{label}: the Binding on UndockCheck no longer has the value "
                                       f"`chargingScreenLoader.item !== null && chargingScreenLoader.item.opened` "
                                       f"(the screensaver loaded and neither opening nor closing).")

    closer = ""
    for _, block in blocks(text, "Connections"):
        if has_line(block, r"target:\s*UndockCheck\s*$") and line_pos(block, "function onCloseRequested") >= 0:
            closer = block
            break
    if not closer:
        found.add("close-handler", f"{label}: no `Connections {{ target: UndockCheck }}` with onCloseRequested: the "
                                   f"quick undock check would ask for the close and nothing would close the "
                                   f"screensaver.")
    else:
        handler = block_after(closer, line_pos(closer, "function onCloseRequested"))
        if line_pos(handler, "root._closeScreensaverNow();") < 0:
            found.add("close-call", f"{label}: UndockCheck's onCloseRequested no longer calls "
                                    f"`root._closeScreensaverNow();`.")

    for timer_id, what in GUARDED_TIMERS:
        _, timer = timer_block(text, timer_id)
        guard_at = line_pos(timer, PENDING_GUARD)
        open_at = line_pos(timer, OPEN)
        if not timer or open_at < 0:
            found.add("timer", f"{label}: no Timer `{timer_id}` that opens the screensaver; this check knows it as "
                               f"one of the {OPENERS} places that do.")
        elif guard_at < 0 or guard_at > open_at:
            found.add("timer-guard", f"{label}: {timer_id} lost its `{PENDING_GUARD}` before `{OPEN}`: {what} would "
                                     f"open the screensaver in the hand of someone who just lifted the remote.")
        else:
            guard = block_after(timer, guard_at)
            for step in ("restart();", "return;"):
                if line_pos(guard, step) < 0:
                    found.add("timer-guard-step", f"{label}: the `{PENDING_GUARD}` of {timer_id} lost `{step}`: it "
                                                  f"must look again later and open nothing now.")

    wake_at, wake = wake_branch(text)
    if not wake:
        found.add("wake", f"{label}: no `else if (...)` that asks `ScreensaverConfig.reopensOnWake(`: the wake branch "
                          f"decides by itself again and opens the screensaver while a lift is unconfirmed.")
    else:
        if "!chargingScreenLoader.active" not in wake:
            found.add("wake-active", f"{label}: the wake branch's condition lost `!chargingScreenLoader.active`: the "
                                     f"rule would be asked, and logged, at every wake of a screensaver that is "
                                     f"already showing.")
        if "UndockCheck.undockPending" not in wake:
            found.add("wake-pending", f"{label}: the wake branch no longer hands `UndockCheck.undockPending` to "
                                      f"ScreensaverConfig.reopensOnWake(): a wake would open the screensaver while a "
                                      f"lift is unconfirmed.")
        condition = " ".join(wake.split())
        if "!chargingScreenLoader.active" in wake and "UndockCheck.undockPending" in wake \
                and condition != WAKE_CONDITION:
            found.add("wake-condition", f"{label}: the wake branch's condition is `{condition}`, not "
                                        f"`{WAKE_CONDITION}`. What the rule is handed, in which place, and whether "
                                        f"its answer is negated decides when a wake opens the screensaver (the first "
                                        f"argument is fromIdle: upstream's own term is the opposite, `fromPowerMode "
                                        f"!== PowerModes.Idle`).")
        body = block_after(text, wake_at + len(wake))
        for step in (OPEN, "dockedRearmTimer.stop();"):
            if line_pos(body, step) < 0:
                found.add("wake-body", f"{label}: the wake branch lost `{step}`.")

    openers = sum(1 for m in ASSIGNMENT.finditer(text) if not ASSIGNS_FALSE.match(text, m.start()))
    if openers != OPENERS:
        found.add("openers", f"{label}: `chargingScreenLoader.active` is assigned something other than `false` in "
                             f"{openers} places, not {OPENERS}. Every place that opens the screensaver on the "
                             f"charger must wait while UndockCheck.undockPending; if a place was added or removed on "
                             f"purpose, give it the guard and change OPENERS in this script.")

    _, loader = block_with(text, "Loader", r"id:\s*chargingScreenLoader\s*$")
    active = re.findall(r"^[ \t]*active\s*:(.*)$", loader, re.MULTILINE)
    if len(active) != 1 or " ".join(active[0].split()) != LOADER_ACTIVE:
        found.add("loader-active", f"{label}: the Loader `chargingScreenLoader` no longer has the one binding "
                                   f"`active: {LOADER_ACTIVE}` (the desktop simulator only). A binding that can be "
                                   f"true on a remote, or none (a Loader is active by default), opens the screensaver "
                                   f"without asking UndockCheck.undockPending.")
    return found


def check_main_cpp(text: str) -> Findings:
    found = Findings()
    label = MAIN_CPP
    code = code_of(text)

    for head, push, signal in ((ON_CLOSE_ON_WAKE, PUSH_CHOICE, "closeOnWakeChanged"),
                               (ON_UNDOCK_DELAY, PUSH_DELAY, "undockCloseMsChanged")):
        at = code.find(head)
        end = code.find("});", at) if at >= 0 else -1
        if at < 0 or end < 0:
            found.add("handler", f"{label}: no handler of ScreensaverConfig's {signal} for the quick undock check "
                                 f"(`{head}`): a change of the setting would reach it only after a restart of the UI.")
            continue
        if push not in code[at:end]:
            found.add("handler-push", f"{label}: the handler of {signal} no longer calls `{push}`.")
        code = code[:at] + code[end + 3:]  # what is left holds the start-up pushes only

    pushes = ((PUSH_DELAY, "the Undock delay"), (PUSH_CHARGER, "the charger as Battery last heard it"),
              (PUSH_CHOICE, "the Close on wake choice"))
    at = [code.find(push) for push, _ in pushes]
    for pos, (push, name) in zip(at, pushes):
        if pos < 0:
            found.add("startup-push", f"{label}: no start-up push `{push}`: the quick undock check would start "
                                      f"without {name} (the NOTIFY signals don't fire at start-up).")
    if min(at) >= 0 and not at[0] < at[1] < at[2]:
        found.add("startup-order", f"{label}: the start-up pushes are out of order: `{PUSH_CHOICE}` must come last, "
                                   f"after the delay and the charger; it is the one that can start the check.")
    return found


def check_hardware_controller(text: str) -> Findings:
    found = Findings()
    code = code_of(text)
    for stmt, why in HW_STATEMENTS:
        if " ".join(stmt.split()) not in code:
            found.add("hardware", f"{HW_CTRL}: no `{stmt}`: {why}.")

    # the order: a connect made to a pointer that is not set yet connects nothing (Qt warns at run time, no test sees)
    created = code.find(HW_CREATE)
    if created < 0:
        return found
    for stmt in HW_CREATED_BEFORE:
        at = code.find(stmt)
        if at < 0:
            found.add("hardware-order", f"{HW_CTRL}: no `{stmt}...`: this check knows it as created before the quick "
                                        f"undock check, which is connected to it.")
        elif at > created:
            found.add("hardware-order", f"{HW_CTRL}: `{stmt}...` now comes after `{HW_CREATE}`: the quick undock "
                                        f"check's connect to it would be made to a pointer that is not set yet.")
    for stmt in HW_CONNECTS:
        at = code.find(" ".join(stmt.split()))
        if 0 <= at < created:
            found.add("hardware-order", f"{HW_CTRL}: `{stmt}` now comes before `{HW_CREATE}`: it would be made to a "
                                        f"pointer that is not set yet.")
    return found


def check(texts: dict) -> Findings:
    found = Findings()
    found.extend(check_main_qml(texts[MAIN_QML]))
    found.extend(check_main_cpp(texts[MAIN_CPP]))
    found.extend(check_hardware_controller(texts[HW_CTRL]))
    return found


def report(found: Findings) -> None:
    for _, message in found:
        print(f"  - {message}")


def self_test(texts: dict) -> int:
    """Breaks one piece at a time and expects the check to name it."""
    found = check(texts)
    if found:
        print("Quick undock check wiring, self-test: the files don't pass the check as they are, so nothing was "
              "broken on purpose. The check found:")
        report(found)
        return 1
    qml, cpp, hwc = texts[MAIN_QML], texts[MAIN_CPP], texts[HW_CTRL]

    def once(text, old, new):
        """`old` replaced by `new` if it is there exactly once (otherwise the text as it was: out of step)."""
        return text.replace(old, new) if text.count(old) == 1 else text

    def inside(text, at, part, old, new):
        """The same, within the `part` of `text` that starts at `at`."""
        if at < 0 or not part or part.count(old) != 1:
            return text
        return text[:at] + part.replace(old, new) + text[at + len(part):]

    def drop_line(text, stmt, which):
        """Without the first (which = 0) or the last (which = -1) line that starts with `stmt`."""
        lines = list(re.finditer(r"^[ \t]*" + re.escape(stmt) + r"[^\n]*\n", text, re.MULTILINE))
        return text if not lines else text[:lines[which].start()] + text[lines[which].end():]

    def commented_out(text, stmt):
        m = re.search(r"^[ \t]*" + r"\s*".join(re.escape(part) for part in stmt.split()), text, re.MULTILINE)
        return text if m is None else text[:m.start()] + "/* " + text[m.start():m.end()] + " */" + text[m.end():]

    def guard_of(timer_id):
        """(position in main.qml, text) of the timer's undockPending guard."""
        timer_at, timer = timer_block(qml, timer_id)
        guard_at = line_pos(timer, PENDING_GUARD)
        return (timer_at + guard_at, block_after(timer, guard_at)) if guard_at >= 0 else (-1, "")

    binding_at, binding = block_with(qml, "Binding", r"target:\s*UndockCheck\s*$")
    closer_at, closer = block_with(qml, "Connections", r"function onCloseRequested")
    rearm_at, rearm = timer_block(qml, "dockedRearmTimer")
    chime_at, chime = timer_block(qml, "dockChimeGraceTimer")
    rearm_guard_at, rearm_guard = guard_of("dockedRearmTimer")
    chime_guard_at, chime_guard = guard_of("dockChimeGraceTimer")
    wake_at, wake = wake_branch(qml)
    body_at = wake_at + len(wake) if wake else -1
    body = block_after(qml, body_at) if wake else ""
    bare_branch = "(fromPowerMode !== PowerModes.Idle && Battery.isCharging && Battery.powerSupply)"
    # a new function in front of _closeScreensaverNow(), holding `statement`
    close_fn = "function _closeScreensaverNow() {"

    def with_function(statement):
        return once(qml, close_fn, "function _sixth() {\n" + " " * 12 + statement + "\n" + " " * 8 + "}\n\n"
                    + " " * 8 + close_fn)

    loader_at, loader = block_with(qml, "Loader", r"id:\s*chargingScreenLoader\s*$")
    loader_line = re.search(r"^[ \t]*active\s*:[^\n]*\n", loader, re.MULTILINE)
    loader_line = loader_line.group(0) if loader_line else ""
    hw_register = "    qmlRegisterSingletonType<UndockCheck>("

    def created_late(stmt):
        """hardwareController.cpp with the line that starts with `stmt` moved down, in front of the QML registration."""
        m = re.search(r"^[ \t]*" + re.escape(stmt) + r"[^\n]*\n", hwc, re.MULTILINE)
        if m is None or hwc.count(hw_register) != 1:
            return hwc
        moved = hwc[:m.start()] + hwc[m.end():]
        return moved.replace(hw_register, m.group(0) + hw_register)

    cases = [
        # (what is broken, file, the broken text, the kind of message, a part of the message the check must give)
        ("the import", MAIN_QML, once(qml, "import UndockCheck 1.0\n", ""), "import", "no `import UndockCheck 1.0`"),
        ("the import, commented out", MAIN_QML,
         once(qml, "import UndockCheck 1.0\n", "// import UndockCheck 1.0\n"), "import", "no `import UndockCheck 1.0`"),
        ("the Binding", MAIN_QML, inside(qml, binding_at, binding, "target: UndockCheck", "target: Battery"),
         "binding", "no `Binding { target: UndockCheck }`"),
        ("the Binding's property", MAIN_QML, once(qml, 'property: "screensaverOpen"', 'property: "screensaverShown"'),
         "binding-property", "no longer sets the property `screensaverOpen`"),
        ("the Binding's value", MAIN_QML,
         once(qml, "chargingScreenLoader.item.opened", "chargingScreenLoader.item.visible"),
         "binding-value", "the Binding on UndockCheck no longer has the value"),
        ("the close handler", MAIN_QML, once(qml, "function onCloseRequested() {", "function onCloseAsked() {"),
         "close-handler", "with onCloseRequested"),
        ("the close handler's Connections, inside /* */", MAIN_QML,
         inside(qml, closer_at, closer, closer, "/*\n" + closer + "\n*/"),
         "close-handler", "with onCloseRequested"),
        ("the close itself", MAIN_QML,
         inside(qml, closer_at, closer, "root._closeScreensaverNow();", "// root._closeScreensaverNow();"),
         "close-call", "no longer calls `root._closeScreensaverNow();`"),
        ("the re-arm timer, renamed", MAIN_QML,
         inside(qml, rearm_at, rearm, "id: dockedRearmTimer", "id: dockedRearmTimerOld"),
         "timer", "no Timer `dockedRearmTimer`"),
        ("the chime grace timer, no longer opening the screensaver", MAIN_QML,
         inside(qml, chime_at, chime, OPEN, ""), "timer", "no Timer `dockChimeGraceTimer`"),
        ("the re-arm's guard", MAIN_QML, inside(qml, rearm_guard_at, rearm_guard, PENDING_GUARD, "if (false) {"),
         "timer-guard", "dockedRearmTimer lost its"),
        ("the chime grace timer's guard", MAIN_QML,
         inside(qml, chime_guard_at, chime_guard, PENDING_GUARD, "if (false) {"),
         "timer-guard", "dockChimeGraceTimer lost its"),
        ("the re-arm guard's return", MAIN_QML, inside(qml, rearm_guard_at, rearm_guard, "return;", ""),
         "timer-guard-step", "of dockedRearmTimer lost `return;`"),
        ("the chime grace guard's restart", MAIN_QML, inside(qml, chime_guard_at, chime_guard, "restart();", ""),
         "timer-guard-step", "of dockChimeGraceTimer lost `restart();`"),
        ("the wake branch, back to upstream's", MAIN_QML,
         inside(qml, wake_at, wake, wake[wake.find("("):], bare_branch),
         "wake", "that asks `ScreensaverConfig.reopensOnWake(`"),
        ("the wake branch without the active check", MAIN_QML,
         inside(qml, wake_at, wake, "!chargingScreenLoader.active", "true"),
         "wake-active", "lost `!chargingScreenLoader.active`"),
        ("the wake branch without undockPending", MAIN_QML,
         inside(qml, wake_at, wake, "UndockCheck.undockPending", "false"),
         "wake-pending", "hands `UndockCheck.undockPending`"),
        ("the wake branch with the rule's answer negated", MAIN_QML,
         inside(qml, wake_at, wake, "&& ScreensaverConfig.reopensOnWake(", "&& !ScreensaverConfig.reopensOnWake("),
         "wake-condition", "&& !ScreensaverConfig.reopensOnWake("),
        ("the wake branch with upstream's `!==` as the first argument", MAIN_QML,
         inside(qml, wake_at, wake, "fromPowerMode === PowerModes.Idle", "fromPowerMode !== PowerModes.Idle"),
         "wake-condition", "reopensOnWake(fromPowerMode !== PowerModes.Idle, Battery.isCharging"),
        ("the wake branch with undockPending negated", MAIN_QML,
         inside(qml, wake_at, wake, "UndockCheck.undockPending", "!UndockCheck.undockPending"),
         "wake-condition", "Battery.powerSupply, !UndockCheck.undockPending))`, not"),
        ("the wake branch with two arguments swapped", MAIN_QML,
         inside(qml, wake_at, wake, "Battery.isCharging, Battery.powerSupply,",
                "Battery.powerSupply, Battery.isCharging,"),
         "wake-condition", "Battery.powerSupply, Battery.isCharging, UndockCheck.undockPending))`, not"),
        ("the wake branch without its open", MAIN_QML, inside(qml, body_at, body, OPEN, ""),
         "wake-body", f"the wake branch lost `{OPEN}`"),
        ("the wake branch without the re-arm's stop", MAIN_QML,
         inside(qml, body_at, body, "dockedRearmTimer.stop();", ""),
         "wake-body", "the wake branch lost `dockedRearmTimer.stop();`"),
        ("a sixth place that opens the screensaver", MAIN_QML, with_function(OPEN),
         "openers", f"in {OPENERS + 1} places"),
        ("a sixth opener after an `if (...)` on the same line", MAIN_QML,
         with_function("if (Battery.powerSupply && !ui.editMode) " + OPEN), "openers", f"in {OPENERS + 1} places"),
        ("a sixth opener without its semicolon", MAIN_QML, with_function(OPEN[:-1]),
         "openers", f"in {OPENERS + 1} places"),
        ("a sixth opener that assigns an expression", MAIN_QML,
         with_function("chargingScreenLoader.active = Battery.powerSupply;"), "openers", f"in {OPENERS + 1} places"),
        ("a sixth opener as a one-line handler", MAIN_QML,
         once(qml, "id: dockChimeGraceTimer\n", "id: dockChimeGraceTimer\n" + " " * 12
              + "onRunningChanged: chargingScreenLoader.active = true\n"),
         "openers", f"in {OPENERS + 1} places"),
        ("upstream's bare wake branch on one line, in front of ours", MAIN_QML,
         inside(qml, wake_at, wake, wake, "else if " + bare_branch + " " + OPEN + "\n" + wake),
         "openers", f"in {OPENERS + 1} places"),
        ("an opener fewer", MAIN_QML, inside(qml, body_at, body, OPEN, "chargingScreenLoader.active = false;"),
         "openers", f"in {OPENERS - 1} places"),
        ("the Loader's own binding, true on the charger", MAIN_QML,
         inside(qml, loader_at, loader, loader_line,
                loader_line.replace(LOADER_ACTIVE, LOADER_ACTIVE + " || Battery.powerSupply")),
         "loader-active", "no longer has the one binding"),
        ("the Loader's own binding, removed", MAIN_QML, inside(qml, loader_at, loader, loader_line, ""),
         "loader-active", "no longer has the one binding"),
        ("the handler of closeOnWakeChanged", MAIN_CPP,
         once(cpp, "&uc::ScreensaverConfig::closeOnWakeChanged, undockCheck,",
              "&uc::ScreensaverConfig::closeOnWakeChanged, nullptr,"),
         "handler", "no handler of ScreensaverConfig's closeOnWakeChanged"),
        ("the handler of undockCloseMsChanged", MAIN_CPP,
         once(cpp, "&uc::ScreensaverConfig::undockCloseMsChanged, undockCheck,",
              "&uc::ScreensaverConfig::undockCloseMsChanged, nullptr,"),
         "handler", "no handler of ScreensaverConfig's undockCloseMsChanged"),
        ("the handler of closeOnWakeChanged, emptied", MAIN_CPP, once(cpp, "{ " + PUSH_CHOICE + " }", "{}"),
         "handler-push", "the handler of closeOnWakeChanged no longer calls"),
        ("the handler of undockCloseMsChanged, emptied", MAIN_CPP, drop_line(cpp, PUSH_DELAY, 0),
         "handler-push", "the handler of undockCloseMsChanged no longer calls"),
        ("the start-up push of the delay", MAIN_CPP, drop_line(cpp, PUSH_DELAY, -1),
         "startup-push", "without the Undock delay"),
        ("the start-up push of the charger", MAIN_CPP, drop_line(cpp, PUSH_CHARGER, -1),
         "startup-push", "without the charger"),
        ("the start-up push of the choice", MAIN_CPP, drop_line(cpp, PUSH_CHOICE, -1),
         "startup-push", "without the Close on wake choice"),
        ("the choice pushed before the charger", MAIN_CPP,
         once(drop_line(cpp, PUSH_CHOICE, -1), "    " + PUSH_CHARGER, "    " + PUSH_CHOICE + "\n    " + PUSH_CHARGER),
         "startup-order", "out of order"),
    ]
    for stmt, why in HW_STATEMENTS:
        cases.append((f"{stmt[:70]} (commented out)", HW_CTRL, commented_out(hwc, stmt), "hardware", why))
    cases.append(("the quick undock check created after its connects", HW_CTRL, created_late(HW_CREATE),
                  "hardware-order", "now comes before `" + HW_CREATE))
    for stmt in HW_CREATED_BEFORE:
        cases.append((f"{stmt}...) created after the quick undock check", HW_CTRL, created_late(stmt),
                      "hardware-order", f"`{stmt}...` now comes after"))
    cases.append(("Battery created under another name", HW_CTRL,
                  once(hwc, HW_CREATED_BEFORE[1], "m_battery = new BatteryV2("),
                  "hardware-order", f"no `{HW_CREATED_BEFORE[1]}...`"))

    failed = 0
    for what, rel, broken, kind, expected in cases:
        if broken == texts[rel]:
            print(f"  SELF-TEST OUT OF STEP: could not break: {what} ({rel} no longer holds it as this script expects)")
            failed += 1
            continue
        found = check({**texts, rel: broken})
        if not any(k == kind and expected in message for k, message in found):
            print(f"  SELF-TEST FAILED: broke: {what}; the check gave no `{kind}` message with `{expected}`, it said: "
                  f"{[message for _, message in found]}")
            failed += 1
    uncovered = [kind for kind in KINDS if kind not in {case[3] for case in cases}]
    for kind in uncovered:
        print(f"  SELF-TEST INCOMPLETE: no case breaks anything that gives a `{kind}` message")
    if failed or uncovered:
        print(f"Quick undock check wiring, self-test: FAILED ({failed} of {len(cases)} cases, {len(uncovered)} of "
              f"{len(KINDS)} kinds of message without a case)")
        return 1
    print(f"Quick undock check wiring, self-test: OK ({len(cases)} broken pieces, each one named; all {len(KINDS)} "
          f"kinds of message have a case)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1],
                        help="repository root to check (default: this repository)")
    parser.add_argument("--self-test", action="store_true",
                        help="prove that each check can fail, on copies of the files in memory")
    args = parser.parse_args()
    root = args.root.resolve()

    missing = [root / rel for rel in FILES if not (root / rel).exists()]
    if missing:
        for p in missing:
            print(f"FILE NOT FOUND: {p}")
        return 2
    texts = {rel: (root / rel).read_text(encoding="utf-8") for rel in FILES}

    if args.self_test:
        return self_test(texts)

    found = check(texts)
    if found:
        print("Quick undock check wiring: DRIFT")
        report(found)
        print("See the list at the top of tools/check_undock_wiring.py and docs/UPSTREAM_MERGE.md.")
        return 1
    print(f"Quick undock check wiring: OK ({', '.join(FILES)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
