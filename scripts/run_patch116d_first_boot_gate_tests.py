#!/usr/bin/env python3
"""Test the real Patch116D native adapter against simulated ESP-IDF APIs."""
from pathlib import Path
import subprocess
import tempfile
import os
import sys

root = Path(__file__).resolve().parents[1]
testdir = root / "tests" / "patch116d"
src = root / "main" / "panel_ota_first_boot_gate.c"
test = testdir / "test_first_boot_gate.c"
flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-O1", "-g",
         "-DPANEL_OTA_FIRST_BOOT_ENABLED=1", "-DCONFIG_IDF_TARGET_ESP32S3=1",
         "-DCONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=1",
         "-DCONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT=1",
         "-DCONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT=1",
         "-DPANEL_OTA_MIN_INTERNAL_BLOCK_BYTES=8192",
         "-DPANEL_OTA_MIN_PSRAM_BLOCK_BYTES=8192",
         "-I", str(testdir / "stubs"), "-I", str(root / "main")]
for compiler in ("gcc", "clang"):
    with tempfile.TemporaryDirectory(prefix="patch116d_") as d:
        exe = str(Path(d) / "test")
        cmd = [compiler, *flags, "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
               str(src), str(test), "-o", exe]
        subprocess.run(cmd, check=True)
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = "detect_leaks=0"
        result = subprocess.run([exe], env=env, text=True, capture_output=True)
        if result.returncode:
            print(result.stdout, result.stderr, file=sys.stderr)
            raise RuntimeError(f"{compiler}: test executable failed {result.returncode}")
        print(f"{compiler}: {result.stdout.strip()} ASAN_UBSAN_PASS")

# Default is truly disabled: compile with only a stub sdkconfig and no IDF APIs.
with tempfile.TemporaryDirectory(prefix="patch116d_disabled_") as d:
    mini = Path(d) / "mini.c"
    mini.write_text('#include "panel_ota_first_boot_gate.h"\n'
                    'int main(void){return panel_ota_first_boot_gate_after_nvs()?0:1;}\n')
    subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
                    "-I", str(testdir / "stubs"), "-I", str(root / "main"), str(src), str(mini), "-o", str(Path(d) / "test")], check=True)
    subprocess.run([str(Path(d) / "test")], check=True)
print("PATCH116D_DEFAULT_DISABLED=PASS")

# The active adapter must refuse to compile without signing/rollback evidence.
for missing in ("CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE", "CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT",
                "CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT", "PANEL_OTA_MIN_INTERNAL_BLOCK_BYTES",
                "PANEL_OTA_MIN_PSRAM_BLOCK_BYTES"):
    with tempfile.TemporaryDirectory(prefix="patch116d_failclosed_") as d:
        args = [x for x in flags if not x.startswith("-D" + missing + "=")]
        result = subprocess.run(["gcc", *args, "-c", str(src), "-o", str(Path(d) / "bad.o")],
                                capture_output=True, text=True)
        if result.returncode == 0:
            raise AssertionError(f"Compile-time guard unexpectedly accepted missing {missing}")
        print(f"CONFIG_GUARD_{missing}=PASS")
print("PATCH116D_HOST_TESTS=PASS")
