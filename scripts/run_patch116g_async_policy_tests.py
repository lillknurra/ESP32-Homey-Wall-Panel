#!/usr/bin/env python3
"""Run standalone deterministic async transport policy tests (no ESP-IDF/device)."""
import os
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCES = [ROOT / "main/panel_ota_async_policy.c", ROOT / "tests/patch116g/test_async_policy.c"]


def run(args, **kwargs):
    subprocess.run(args, check=True, **kwargs)


def main():
    for cc in ("clang", "gcc"):
        with tempfile.TemporaryDirectory(prefix="patch116g-") as d:
            exe = os.path.join(d, "hosttest")
            cmd = [cc, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
                   "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
                   "-I", str(ROOT / "main"), *map(str, SOURCES), "-o", exe]
            run(cmd)
            run([exe])
        print(f"PATCH116G_{cc.upper()}=PASS")

    # This code must NEVER be enabled by flipping the old synchronous stage flag.
    # Use preprocess-only to verify the safety lock does not require IDF headers.
    stage = ROOT / "main/panel_ota_https_stage.c"
    with tempfile.TemporaryDirectory(prefix="patch116g-preprocess-") as d:
        # Only a safe compile-time guard is evaluated; do not need native headers.
        pathlib.Path(d, "panel_ota_https_stage.h").write_text("/* isolated test stub */\n")
        pathlib.Path(d, "sdkconfig.h").write_text("/* isolated test stub */\n")
        result = subprocess.run(["cc", "-std=c11", "-E", "-I", d,
                                 "-DPANEL_OTA_HTTPS_STAGE_ENABLED=1", "-DESP_PLATFORM=1", str(stage)],
                                capture_output=True, text=True)
    if result.returncode == 0 or "PATCH116G: synchronous HTTPS staging forbidden in firmware" not in result.stderr:
        raise RuntimeError("unsafe legacy HTTPS stage was not compile-time rejected")
    print("PATCH116G_SYNC_STAGE_ACTIVATION=REJECTED_AS_EXPECTED")

if __name__ == "__main__":
    main()
