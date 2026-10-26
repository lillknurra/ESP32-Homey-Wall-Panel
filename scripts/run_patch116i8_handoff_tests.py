#!/usr/bin/env python3
"""Patch116I8: offline-only test driver. No IDF build or network activity."""
from __future__ import annotations

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPONENT = ROOT / "components/panel_update_service"
INC = COMPONENT / "include"
I7 = COMPONENT / "panel_update_i7_admission_policy.c"
I8 = COMPONENT / "panel_update_i8_handoff_policy.c"
TEST = ROOT / "tests/patch116i8/test_handoff.c"


def run(args: list[str], pass_expected: bool = True) -> subprocess.CompletedProcess[str]:
    r = subprocess.run(args, input="", capture_output=True, text=True, timeout=30)
    if (r.returncode == 0) != pass_expected:
        raise RuntimeError(f"Unexpected exit={r.returncode} args={args}\n"
                           f"stdout={r.stdout}\nstderr={r.stderr}")
    return r


def main() -> int:
    if "panel_update_i8_handoff_policy.c" in (COMPONENT / "CMakeLists.txt").read_text():
        raise RuntimeError("FAIL: I8 accidentally registered as native ESP-IDF component")
    for p in (I7, I8, TEST):
        if not p.is_file():
            raise RuntimeError(f"FAIL: required file missing: {p}")
    with tempfile.TemporaryDirectory(prefix="patch116i8-host-") as d:
        temp = Path(d)
        common = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic", f"-I{INC}"]
        output = temp / "handoff_test"
        run(common + [str(I7), str(I8), str(TEST), "-o", str(output)])
        print(run([str(output)]).stdout.strip())
        fail = run(common + ["-DESP_PLATFORM=1", "-c", str(I8),
                             "-o", str(temp / "unauthorized_native.o")], False)
        if "unauthorized native transport integration" not in fail.stderr:
            raise RuntimeError("FAIL: missing native build fail-closed gate")
        run(common + ["-DESP_PLATFORM=1", "-DPANEL_UPDATE_I8_AUDIT_ONLY=1",
                      "-c", str(I8), "-o", str(temp / "audit_only.o")])
        # Test assertions must remain active; compile-time -DNDEBUG is forbidden.
    print("PASS: I8 remains unlinked and unactivated; unauthorized native build rejected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
