#!/usr/bin/env python3
"""Deterministic host tests for the offline Patch116I10 DNS bridge."""
from __future__ import annotations

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPONENT = ROOT / "components/panel_update_service"
TEST_DIR = ROOT / "tests/patch116i10"
SCENARIOS = ("schedule", "immediate", "async", "cancel", "reuse", "capacity",
             "scheduler-capacity")


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, capture_output=True, text=True, timeout=30,
                          check=False)


def main() -> int:
    source = COMPONENT / "panel_update_i10_dns_bridge.c"
    header = COMPONENT / "include/panel_update_i10_dns_bridge.h"
    test = TEST_DIR / "test_dns_bridge.c"
    shim = TEST_DIR / "include/panel_update_i10_dns_test_api.h"
    for path in (source, header, test, shim):
        if not path.is_file():
            raise RuntimeError(f"required I10 host-test file missing: {path}")

    # The bridge is intentionally offline and outside every native firmware
    # source list. Keep that invariant explicit in the test runner.
    for path in (ROOT / "CMakeLists.txt", COMPONENT / "CMakeLists.txt",
                 ROOT / "main/CMakeLists.txt",
                 ROOT / "components/secure_bootstrap/CMakeLists.txt"):
        content = path.read_text(encoding="utf-8") if path.exists() else ""
        if source.name in content or "PANEL_UPDATE_I10_DNS_BRIDGE_ENABLED" in content:
            raise RuntimeError(f"I10 DNS bridge was linked/enabled by {path}")

    with tempfile.TemporaryDirectory(prefix="patch116i10-host-") as tmp:
        binary = Path(tmp) / "i10_dns_bridge_test"
        base = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                f"-I{COMPONENT / 'include'}", f"-I{TEST_DIR / 'include'}",
                str(source), str(test), "-o", str(binary)]
        compiled = run(base[:-2] + ["-fsanitize=address,undefined",
                                     "-fno-omit-frame-pointer"] + base[-2:])
        sanitizer = compiled.returncode == 0
        if not sanitizer:
            compiled = run(base)
        if compiled.returncode != 0:
            raise RuntimeError(f"I10 host compile failed\n{compiled.stdout}{compiled.stderr}")
        for scenario in SCENARIOS:
            result = run([str(binary), scenario])
            if result.returncode != 0:
                raise RuntimeError(
                    f"I10 scenario {scenario!r} failed ({result.returncode})\n"
                    f"{result.stdout}{result.stderr}")
        print(f"PASS: {len(SCENARIOS)} deterministic I10 scenarios")
        print(f"SANITIZERS={'ASAN+UBSAN' if sanitizer else 'UNAVAILABLE; plain C11 used'}")
        if sanitizer:
            # Verify the sanitizer runtime is functional in this environment.
            # The real scenarios above were already run with the instrumented binary.
            print("PASS: address and undefined-behavior sanitizers enabled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
