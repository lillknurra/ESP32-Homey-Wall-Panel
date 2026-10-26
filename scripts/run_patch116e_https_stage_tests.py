#!/usr/bin/env python3
"""Offline sanitizer tests for Patch116F bounded HTTPS+native esp_ota adapter."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
stubs = root / "tests/patch116e/stubs"
src = root / "main/panel_ota_https_stage.c"
test = root / "tests/patch116e/test_https_stage.c"
flags = [
    "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-O1", "-g",
    "-DPANEL_OTA_HTTPS_STAGE_ENABLED=1", "-DPANEL_OTA_FIRST_BOOT_ENABLED=1",
    "-DCONFIG_IDF_TARGET_ESP32S3=1", "-DCONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=1",
    "-DCONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT=1",
    "-DCONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT=1",
    "-DCONFIG_MBEDTLS_CERTIFICATE_BUNDLE=1",
    '-DPANEL_OTA_RELEASE_URL="https://updates.example.org/firmware.bin"',
    "-DPANEL_OTA_MAX_IMAGE_BYTES=4194304", "-DPANEL_OTA_MAX_TRANSFER_MS=240000",
    "-I", str(stubs), "-I", str(root / "main")
]
for cc in ("gcc", "clang"):
    with tempfile.TemporaryDirectory(prefix="patch116f_") as d:
        exe = str(Path(d) / "run")
        subprocess.run([cc, *flags, "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                        str(src), str(test), "-o", exe], check=True)
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
        result = subprocess.run([exe], capture_output=True, text=True, env=env)
        if result.returncode:
            raise RuntimeError(f"{cc} failed {result.returncode}: {result.stdout} {result.stderr}")
        print(cc, result.stdout.strip(), "ASAN_UBSAN_PASS")

# Default-off must compile without a single IDF symbol/side-effect.
with tempfile.TemporaryDirectory(prefix="patch116f_disabled_") as d:
    mini = Path(d) / "disabled.c"
    mini.write_text('#include "panel_ota_https_stage.h"\n'
                    'int main(void){return panel_ota_https_stage(0)==PANEL_OTA_STAGE_DISABLED?0:1;}\n')
    binary = Path(d) / "disabled"
    subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
                    "-I", str(stubs), "-I", str(root / "main"),
                    str(src), str(mini), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("PATCH116F_DEFAULT_DISABLED=PASS")

# Invalid URL fixtures must be rejected before network access.
invalid_urls = [
    "http://updates.example.org/firmware.bin",
    "https://user@updates.example.org/firmware.bin",
    "https://updates.example.org/firmware.bin?token=secret",
    "https://updates.example.org/",
    "https://127.0.0.1/firmware.bin",
]
with tempfile.TemporaryDirectory(prefix="patch116f_badurl_") as d:
    base = [a for a in flags if not a.startswith("-DPANEL_OTA_RELEASE_URL=")]
    for i, url in enumerate(invalid_urls):
        exe = str(Path(d) / f"badurl_{i}")
        subprocess.run(["gcc", *base, f'-DPANEL_OTA_RELEASE_URL="{url}"',
                        "-DPANEL_OTA_EXPECT_BAD_URL=1", str(src), str(test), "-o", exe],
                        check=True)
        subprocess.run([exe], check=True, capture_output=True)
print(f"PATCH116F_BAD_URL_VARIANTS={len(invalid_urls)} PASS")

# Compile-time fail-closed gates, do not use synthetic macros in production.
required = ["PANEL_OTA_FIRST_BOOT_ENABLED", "CONFIG_IDF_TARGET_ESP32S3",
            "CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE", "CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT",
            "CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT", "CONFIG_MBEDTLS_CERTIFICATE_BUNDLE",
            "PANEL_OTA_RELEASE_URL", "PANEL_OTA_MAX_IMAGE_BYTES", "PANEL_OTA_MAX_TRANSFER_MS"]
with tempfile.TemporaryDirectory(prefix="patch116f_guard_") as d:
    for macro in required:
        bad = [x for x in flags if not x.startswith(f"-D{macro}=")]
        result = subprocess.run(["gcc", *bad, "-c", str(src), "-o", str(Path(d) / "bad.o")],
                                capture_output=True, text=True)
        if result.returncode == 0:
            raise AssertionError(f"missing {macro} unexpectedly compiled")
    result = subprocess.run(["gcc", *flags, "-DCONFIG_ESP_HTTPS_OTA_ALLOW_HTTP=1",
                             "-c", str(src), "-o", str(Path(d) / "http.o")],
                             capture_output=True, text=True)
    if result.returncode == 0:
        raise AssertionError("insecure HTTP option accepted")
    for marker in ("CONFIG_SECURE_BOOT", "CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK",
                   "CONFIG_FLASH_ENCRYPTION_ENABLED"):
        result = subprocess.run(["gcc", *flags, f"-D{marker}=1", "-c", str(src),
                                 "-o", str(Path(d) / "forbidden.o")],
                                 capture_output=True, text=True)
        if result.returncode == 0:
            raise AssertionError(f"forbidden configuration accepted: {marker}")
    for macro in ("CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT", "CONFIG_MBEDTLS_CERTIFICATE_BUNDLE"):
        bad = [x for x in flags if not x.startswith(f"-D{macro}=")]
        bad.append(f"-D{macro}=0")
        result = subprocess.run(["gcc", *bad, "-c", str(src), "-o", str(Path(d) / "zero.o")],
                                capture_output=True, text=True)
        if result.returncode == 0:
            raise AssertionError(f"zero-valued required setting accepted: {macro}")
print("PATCH116F_CONFIG_GUARDS=PASS")
