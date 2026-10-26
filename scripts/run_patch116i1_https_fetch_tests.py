#!/usr/bin/env python3
"""Host-only mocks and compile-time fails; actual ESP-IDF integration is Codex's job."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
SERV = ROOT / "components" / "panel_update_service"
TEST = ROOT / "tests" / "patch116i1"


def run(cmd, cwd=None, expect_success=True):
    proc = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, check=False)
    if (proc.returncode == 0) != expect_success:
        raise RuntimeError(f"cmd={cmd}\nstatus={proc.returncode}\nstdout={proc.stdout}\nstderr={proc.stderr}")
    if expect_success and "PATCH116I1" in proc.stdout:
        print(proc.stdout.strip())


def main():
    with tempfile.TemporaryDirectory(prefix="patch116i1-") as tmp:
        for cc in ("gcc", "clang"):
            if not shutil.which(cc):
                raise RuntimeError(f"missing compiler: {cc}")
            base = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-O1", "-g",
                    "-D_DEFAULT_SOURCE", "-I", str(SERV / "include"), "-I", str(TEST / "shim")]
            source = str(SERV / "panel_update_https_fetch.c")
            binary = os.path.join(tmp, f"fetch_{cc}")
            flags = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
            run(base + flags + ["-DESP_PLATFORM=1", "-DPANEL_UPDATE_MANIFEST_HTTPS_ENABLED=1",
                                "-DCONFIG_MBEDTLS_CERTIFICATE_BUNDLE=1", "-DCONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS=1",
                                "-DPANEL_UPDATE_HTTPS_AUDIT_ONLY=1",
                                source, str(TEST / "test_fetch.c"), "-o", binary])
            run([binary], expect_success=True)
            off = os.path.join(tmp, f"disabled_{cc}")
            run(base + flags + [source, str(TEST / "test_disabled.c"), "-o", off])
            run([off])
            nm = subprocess.run(["nm", "-u", off], capture_output=True, text=True, check=True)
            if any(k in nm.stdout for k in ("esp_http_client", "esp_ota_", "esp_https_ota")):
                raise RuntimeError("default-off links network or OTA symbols")
            # Fail-closed config requirements.
            for bad in (
                ["-DESP_PLATFORM=1", "-DPANEL_UPDATE_MANIFEST_HTTPS_ENABLED=1", "-DCONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS=1", "-DPANEL_UPDATE_HTTPS_AUDIT_ONLY=1"],
                ["-DESP_PLATFORM=1", "-DPANEL_UPDATE_MANIFEST_HTTPS_ENABLED=1", "-DCONFIG_MBEDTLS_CERTIFICATE_BUNDLE=1", "-DPANEL_UPDATE_HTTPS_AUDIT_ONLY=1"],
                ["-DPANEL_UPDATE_MANIFEST_HTTPS_ENABLED=1"],
                ["-DPANEL_UPDATE_MANIFEST_HTTPS_ENABLED=2"],
                ["-DESP_PLATFORM=1", "-DPANEL_UPDATE_MANIFEST_HTTPS_ENABLED=1", "-DCONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS=1", "-DCONFIG_MBEDTLS_CERTIFICATE_BUNDLE=1"]):
                run(base + bad + ["-fsyntax-only", source], expect_success=False)
        print("PATCH116I1_TESTS_ALL=PASS")


if __name__ == "__main__":
    main()
