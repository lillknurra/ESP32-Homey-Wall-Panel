#!/usr/bin/env python3
"""Patch116I9 host-only resolved endpoint policy tests; no network or IDF build."""
from __future__ import annotations

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPONENT = ROOT / "components/panel_update_service"
SOURCE = COMPONENT / "panel_update_i9_resolved_endpoint_policy.c"
HEADER = COMPONENT / "include/panel_update_i9_resolved_endpoint_policy.h"
TEST = ROOT / "tests/patch116i9/test_resolved_endpoint.c"


def main() -> int:
    for path in (SOURCE, HEADER, TEST):
        if not path.is_file():
            raise RuntimeError(f"required I9 host-test file missing: {path}")

    # I9 is an offline policy only. It must not enter the production component
    # source list or acquire an ESP-IDF/runtime call site in this patch.
    cmake_files = (
        ROOT / "CMakeLists.txt",
        COMPONENT / "CMakeLists.txt",
        ROOT / "components/secure_bootstrap/CMakeLists.txt",
        ROOT / "main/CMakeLists.txt",
    )
    forbidden = (SOURCE.name, "PANEL_UPDATE_I9_RESOLVED_ENDPOINT_ENABLED")
    for cmake in cmake_files:
        if cmake.exists():
            contents = cmake.read_text(encoding="utf-8")
            if any(marker in contents for marker in forbidden):
                raise RuntimeError(f"I9 was registered or enabled in {cmake}")

    with tempfile.TemporaryDirectory(prefix="patch116i9-host-") as tmp:
        binary = Path(tmp) / "resolved_endpoint_test"
        compile_result = subprocess.run(
            ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
             f"-I{COMPONENT / 'include'}", str(SOURCE), str(TEST), "-o", str(binary)],
            capture_output=True, text=True, timeout=30, check=False,
        )
        if compile_result.returncode != 0:
            raise RuntimeError(
                f"I9 host compile failed ({compile_result.returncode})\n"
                f"{compile_result.stdout}{compile_result.stderr}"
            )
        test_result = subprocess.run(
            [str(binary)], capture_output=True, text=True, timeout=30, check=False,
        )
        if test_result.returncode != 0:
            raise RuntimeError(
                f"I9 host tests failed ({test_result.returncode})\n"
                f"{test_result.stdout}{test_result.stderr}"
            )
        print(test_result.stdout.strip())
    print("PASS: I9 remains absent from native CMake; no DNS/TLS/HTTP operation")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
