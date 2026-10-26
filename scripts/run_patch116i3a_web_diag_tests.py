#!/usr/bin/env python3
"""Host-only validation. No external connections or physical operations."""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
COMP = ROOT / "components" / "panel_update_service"
TEST = ROOT / "tests" / "patch116i3a"
I3 = ROOT / "tests" / "patch116i3"


def execute(args, env=None):
    p = subprocess.run(args, env=env, text=True, capture_output=True)
    if p.returncode:
        raise AssertionError(f"Failed: {' '.join(map(str,args))}\n{p.stdout}\n{p.stderr}")
    return p.stdout


def check_source():
    handler = (ROOT / "components" / "secure_bootstrap" / "athom_oauth_runtime.c").read_text()
    src = (COMP / "panel_update_i3_probe_esp.c").read_text()
    for needle in (
        "if (diagnostic_only)", "panel_update_i3a_snapshot_copy(&resource)",
        "panel_update_i3a_append_json(body, body_capacity", "PANEL_UPDATE_I3A_AUDIT_ONLY",
    ):
        assert needle in handler, needle
    assert "portENTER_CRITICAL(&s_i3a_snapshot_mux)" in src
    assert "portEXIT_CRITICAL(&s_i3a_snapshot_mux)" in src
    assert "esp_reset_reason()" in src
    for forbidden in ("esp_ota_write(", "esp_ota_begin(", "esp_restart(",
                      "nvs_set_", "esp_http_client_perform(", "athom_oauth_runtime_queue_"):
        assert forbidden not in (COMP / "panel_update_i3a_web_diag.c").read_text()
    print("STATIC_SCOPE_AND_GATES=PASS")


def test_compiler(cc, temp):
    common = [cc, "-std=c11", "-pedantic", "-Wall", "-Wextra", "-Werror",
              "-O1", "-g", "-fno-omit-frame-pointer", "-I" + str(COMP / "include")]
    for mode in ("native", "undefined", "address,undefined"):
        opts = [] if mode == "native" else ["-fsanitize=" + mode]
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
        env["UBSAN_OPTIONS"] = "halt_on_error=1"
        exe = temp / (cc + "_json_" + mode.replace(",", "_"))
        execute([*common, *opts, str(COMP / "panel_update_i3a_web_diag.c"),
                 str(TEST / "test_json.c"), "-o", str(exe)])
        out = execute([str(exe)], env)
        data = [json.loads(x) for x in out.splitlines() if x.startswith("{")]
        assert len(data) == 4, len(data)
        assert data[0]["resource_probe"]["valid"] is True
        assert data[0]["resource_probe"]["age_ms"] == 300
        assert data[1]["resource_probe"]["stale"] is True
        assert data[2]["resource_probe"]["valid"] is False
        assert data[3]["resource_probe"]["internal_free_bytes"] == 2**32 - 1
        print(cc, mode, out.splitlines()[-1])
    # Confirm default-off probe object does not import ANY external symbols.
    obj = temp / (cc + "_off.o")
    execute([*common, "-c", str(COMP / "panel_update_i3_probe_esp.c"), "-o", str(obj)])
    nm = execute(["nm", "-u", str(obj)]).strip()
    assert not nm, f"default-off object imports symbols: {nm}"
    print(cc, "DEFAULT_OFF_NO_EXTERNAL_SYMBOLS=PASS")
    for defines in (["-DPANEL_UPDATE_I3_OBSERVER_ENABLED=1"],):
        p = subprocess.run([*common, *defines, "-c",
                 str(COMP / "panel_update_i3_probe_esp.c"), "-o", str(temp / "fail.o")],
                 text=True, capture_output=True)
        assert p.returncode, "Audit lock unexpectedly bypassed"
    print(cc, "OBSERVER_PRODUCTION_LOCK=PASS")
    # The active adapter is tested with a thread-safe shim of the ESP-IDF
    # spinlock API, to detect torn snapshots and lock lifetime errors.
    for mode in ("native", "address,undefined"):
        opts = [] if mode == "native" else ["-fsanitize=" + mode]
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
        env["UBSAN_OPTIONS"] = "halt_on_error=1"
        exe = temp / (cc + "_concurrent_" + mode.replace(",", "_"))
        flags = ["-pthread", "-DESP_PLATFORM=1",
                 "-DPANEL_UPDATE_I3_OBSERVER_ENABLED=1", "-DPANEL_UPDATE_I3_AUDIT_ONLY=1",
                 "-DPANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED=0",
                 "-I" + str(TEST / "shim")]
        execute([*common, *opts, *flags,
                 str(COMP / "panel_update_i3_probe_esp.c"),
                 str(COMP / "panel_update_i3_probe_policy.c"),
                 str(TEST / "test_concurrent.c"), "-o", str(exe)])
        print(cc, mode, execute([str(exe)], env).strip())


check_source()
with tempfile.TemporaryDirectory(prefix="patch116i3a_") as folder:
    for compiler in ("gcc", "clang"):
        if shutil.which(compiler):
            test_compiler(compiler, Path(folder))
print("PATCH116I3A HOST TESTS PASS")
