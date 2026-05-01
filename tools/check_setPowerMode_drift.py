#!/usr/bin/env python3
"""Build-time drift check for the `setPowerMode` WS RPC body field name.

History
-------
v1.4.14 (Mod 5 ActivitySessionKeeper) and v1.4.20 (Mod 6 PhantomWakeSuppressor) shipped a
silent bug: `Api::setPowerMode` constructed the WS RPC body with field name `"power_mode"`
instead of `"mode"`. Firmware's serde deserializer rejected every call with HTTP 400
`Error("missing field 'mode'", ...)`. Both Mods were no-ops in production for ~8 releases.

The bug was fixed in v1.4.22 at `src/core/core.cpp:821` by changing the field-name string
to `"mode"`. v1.4.27 added regression-prevention unit tests (`test_ping_body_shape` and
`test_forceLowPower_body_shape`) that mock `core::Api` with a verbatim mirror of the v1.4.22
fix line and assert the body shape. The mock lives at `test/hardware/mock_core_api.cpp`.

The catch: the mock's verbatim-mirror line and the production `core.cpp:821` line MUST stay
in sync. If a future change to `core.cpp` regresses the field name back to `"power_mode"`,
and a corresponding update to the mock is also made by the same change author, the unit
tests would still pass (because they assert against the mock's mirror, not the production
implementation directly). That's a real failure mode of contract-test-via-mirroring.

This script closes that gap with a build-time literal-grep check:

  - Production: `src/core/core.cpp` MUST contain `msgData.insert("mode"` (the v1.4.22 fix line).
  - Mock:       `test/hardware/mock_core_api.cpp` MUST contain `msgData.insert("mode"` (its mirror).
  - Negative:   Neither file may contain `msgData.insert("power_mode"` (the bug's original form).

Wired into CI as a fast standalone job in `.github/workflows/test.yml` ("setpowermode-drift").
The job runs in seconds (no Qt, no compile) so even casual PR commits get the signal cheaply.

Exit codes:
  0 — both files have the expected `"mode"` literal; neither has `"power_mode"`. Drift safe.
  1 — production or mock is missing the expected literal, OR has the regressed `"power_mode"`.
  2 — file paths not found (repository layout broke).

Usage:
  python tools/check_setPowerMode_drift.py
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRODUCTION = ROOT / "src" / "core" / "core.cpp"
MOCK = ROOT / "test" / "hardware" / "mock_core_api.cpp"

EXPECTED_LITERAL = 'msgData.insert("mode"'
REGRESSED_LITERAL = 'msgData.insert("power_mode"'


def check_file(path: Path, label: str) -> list[str]:
    """Return a list of error messages (empty on success)."""
    errors: list[str] = []
    if not path.exists():
        errors.append(f"FILE NOT FOUND: {label} expected at {path}")
        return errors
    text = path.read_text(encoding="utf-8")
    if EXPECTED_LITERAL not in text:
        errors.append(
            f"DRIFT: {label} ({path.relative_to(ROOT)}) does NOT contain `{EXPECTED_LITERAL}`. "
            f"This is the v1.4.22 setPowerMode fix line — firmware's serde deserializer requires "
            f"body field name `mode` (not `power_mode`). If you intentionally changed the API contract, "
            f"update both the production line in src/core/core.cpp AND the mock mirror in "
            f"test/hardware/mock_core_api.cpp, then update this script."
        )
    if REGRESSED_LITERAL in text:
        errors.append(
            f"REGRESSION: {label} ({path.relative_to(ROOT)}) contains `{REGRESSED_LITERAL}`. "
            f"This is the original v1.4.14 → v1.4.21 bug — firmware rejects this with HTTP 400. "
            f"The fix is to use `mode` (without the `power_` prefix). See "
            f"https://github.com/mmadalone/Madalones-Defolded-Circle-3/blob/main/CHANGELOG.md "
            f"and search for `v1.4.22` for the original incident analysis."
        )
    return errors


def main() -> int:
    all_errors: list[str] = []
    all_errors.extend(check_file(PRODUCTION, "production core.cpp"))
    all_errors.extend(check_file(MOCK, "mock_core_api.cpp"))

    if any("FILE NOT FOUND" in e for e in all_errors):
        for e in all_errors:
            print(f"ERROR: {e}", file=sys.stderr)
        return 2

    if all_errors:
        print("setPowerMode body-field drift check: FAILED", file=sys.stderr)
        for e in all_errors:
            print(f"  - {e}", file=sys.stderr)
        return 1

    print("setPowerMode body-field drift check: OK")
    print(f"  [OK]{PRODUCTION.relative_to(ROOT)} contains `{EXPECTED_LITERAL}`")
    print(f"  [OK]{MOCK.relative_to(ROOT)} contains `{EXPECTED_LITERAL}`")
    print(f"  [OK]Neither file contains the regressed `{REGRESSED_LITERAL}`")
    return 0


if __name__ == "__main__":
    sys.exit(main())
