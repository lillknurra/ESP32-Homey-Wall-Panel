#!/usr/bin/env python3
"""Offline-only host and negative compile gates for Patch116I7 admission."""
from __future__ import annotations

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "components/panel_update_service/panel_update_i7_admission_policy.c"
INC = ROOT / "components/panel_update_service/include"
TEST = ROOT / "tests/patch116i7/test_admission.c"
COMPONENT_CMAKE = ROOT / "components/panel_update_service/CMakeLists.txt"


def run(args: list[str], must_pass: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(args, text=True, input="", capture_output=True, timeout=30)
    if must_pass != (result.returncode == 0):
        raise RuntimeError(f"Unexpected result ({result.returncode}): {' '.join(args)}\n"
                           f"stdout: {result.stdout}\nstderr: {result.stderr}")
    return result


def main() -> int:
    if "panel_update_i7_admission_policy.c" in COMPONENT_CMAKE.read_text():
        raise RuntimeError("FAIL: I7 unexpectedly linked into production component")
    with tempfile.TemporaryDirectory(prefix="patch116i7-host-") as tmp:
        td = Path(tmp)
        common = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic", f"-I{INC}"]
        exe = td / "test_admission"
        run(common + [str(SRC), str(TEST), "-o", str(exe)])
        print(run([str(exe)]).stdout.strip())
        bad = run(common + ["-DESP_PLATFORM=1", "-c", str(SRC),
                            "-o", str(td / "unauthorized.o")], must_pass=False)
        if "offline/audit-only" not in bad.stderr:
            raise RuntimeError("FAIL: ESP_PLATFORM native compile gate missing")
        run(common + ["-DESP_PLATFORM=1", "-DPANEL_UPDATE_I7_AUDIT_ONLY=1",
                      "-c", str(SRC), "-o", str(td / "audit_only.o")])
    print("PASS: I7 not wired into native component; unauthorized native build rejected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
