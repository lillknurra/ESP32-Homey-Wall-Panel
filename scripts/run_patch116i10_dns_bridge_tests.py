#!/usr/bin/env python3
"""Deterministic host tests for the offline Patch116I10 DNS bridge."""
from __future__ import annotations

from pathlib import Path
import re
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

    # I11 deliberately object-compiles the adapter in the native component.
    # Require that one registration and its lwIP dependency, while preventing
    # activation: no firmware translation unit may call the I10 public API.
    component_cmake = (COMPONENT / "CMakeLists.txt").read_text(encoding="utf-8")
    registration_count = component_cmake.count(f'"{source.name}"')
    if registration_count != 1:
        raise RuntimeError(
            f"I10 source must be registered exactly once in panel_update_service; "
            f"found {registration_count}")
    requires = re.search(r"\bPRIV_REQUIRES\s+([^)]*)", component_cmake, re.DOTALL)
    if requires is None or "lwip" not in requires.group(1).split():
        raise RuntimeError("I10 native component is missing its lwip dependency")

    for path in (ROOT / "CMakeLists.txt", ROOT / "main/CMakeLists.txt",
                 ROOT / "components/secure_bootstrap/CMakeLists.txt"):
        content = path.read_text(encoding="utf-8") if path.exists() else ""
        if source.name in content or "PANEL_UPDATE_I10_DNS_BRIDGE_ENABLED" in content:
            raise RuntimeError(f"I10 unexpectedly enabled/registered by {path}")

    api_call = re.compile(r"\bpanel_update_i10_dns_(?:start|cancel|take_result)\s*\(")
    for tree in (ROOT / "main", ROOT / "components"):
        for path in tree.rglob("*.c"):
            if path == source:
                continue  # The only production occurrence is the function definition.
            if api_call.search(path.read_text(encoding="utf-8", errors="replace")):
                raise RuntimeError(f"I10 runtime API call found in firmware source: {path}")

    # These existing network gates must remain disabled by their production
    # defaults. This is a source check, not an ELF/build/runtime assertion.
    default_markers = (
        (COMPONENT / "panel_update_worker_freertos.c",
         "#define PANEL_UPDATE_READONLY_WORKER_ENABLED 0"),
        (COMPONENT / "panel_update_https_fetch.c",
         "#define PANEL_UPDATE_MANIFEST_HTTPS_ENABLED 0"),
        (COMPONENT / "include/panel_update_i3_probe.h",
         "#define PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED 0"),
    )
    for path, marker in default_markers:
        if marker not in path.read_text(encoding="utf-8"):
            raise RuntimeError(f"default-off network gate missing from {path}")

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
