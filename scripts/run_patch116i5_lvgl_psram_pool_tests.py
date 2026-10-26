#!/usr/bin/env python3
"""Host regressions and compile-gate checks for the Patch116I5 pool hook."""
from __future__ import annotations

import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMP = ROOT / "components/panel_update_service"
INC = COMP / "include"
SRC = COMP / "panel_update_lvgl_psram_pool.c"
TEST = ROOT / "tests/patch116i5/pool_host_test.c"


def run(cmd: list[str], *, ok: bool = True) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(cmd, text=True, capture_output=True, stdin=subprocess.DEVNULL, timeout=20)
    if ok and proc.returncode:
        raise RuntimeError(f"command failed ({proc.returncode}): {' '.join(cmd)}\n{proc.stderr}")
    if not ok and proc.returncode == 0:
        raise RuntimeError(f"command unexpectedly passed: {' '.join(cmd)}")
    return proc


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="patch116i5-host-") as td:
        exe = str(pathlib.Path(td) / "pool_host_test")
        run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
             "-DPANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED=1",
             "-DPANEL_UPDATE_I5_AUDIT_ONLY=1", f"-I{INC}", f"-I{COMP}",
             str(SRC), str(TEST), "-o", exe])
        for scenario in ("success", "wrong-size", "no-psram", "alloc-fail", "misaligned", "internal"):
            run([exe, scenario])

        unauthorized = run(["cc", "-std=c11", "-DPANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED=1",
                            "-DPANEL_UPDATE_I5_AUDIT_ONLY=0", f"-I{INC}",
                            "-include", "panel_update_lvgl_psram_pool.h",
                            "-x", "c", "-c", "-o", str(pathlib.Path(td) / "bad.o"), "-"], ok=False)
        if "audit-only build" not in unauthorized.stderr:
            raise RuntimeError("compile-time authorization failure did not report the expected gate")

        default_obj = pathlib.Path(td) / "default.o"
        run(["cc", "-std=c11", f"-I{INC}", "-c", str(SRC), "-o", str(default_obj)])
        symbols = run(["nm", "-g", str(default_obj)]).stdout
        if "panel_update_lvgl_psram_pool_alloc" in symbols:
            raise RuntimeError("default-disabled object unexpectedly exports the pool allocator")

    print("PASS: exact 65536-byte aligned explicit-PSRAM allocation, fail-closed gates, stable reuse, no fallback, default-off symbol absence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
