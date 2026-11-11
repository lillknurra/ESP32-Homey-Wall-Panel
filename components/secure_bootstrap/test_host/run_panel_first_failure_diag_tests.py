#!/usr/bin/env python3
import json
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[3]
component = root / "components/secure_bootstrap"
with tempfile.TemporaryDirectory(prefix="first-failure-diag-") as tmp:
    exe = pathlib.Path(tmp) / "first_failure_test"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-pthread",
        "-I", str(component / "include"),
        str(component / "panel_first_failure_diag.c"),
        str(component / "test_host/test_panel_first_failure_diag.c"),
        "-o", str(exe),
    ], check=True)
    output = subprocess.check_output([str(exe)], text=True).strip()
    parsed = json.loads(output)
    record = parsed["first_failed_inventory"]
    assert record["runtime_id"] == 42
    assert record["sequence"] == 2
    assert record["origin"] == "periodic"
    assert record["stage"] == "devices"
    assert record["devices"]["content_length"] == 314680
    assert record["devices"]["received_bytes"] == 108767
    assert record["devices"]["terminal"]["errno"] == 11
    assert record["socket_close"] == {
        "correlation_complete": False, "uncertain": True, "release": "unknown"
    }
    print("first-failure snapshot, immutability, runtime reset and bounded JSON: PASS")

# Integration guard: capture is made only after attempt completion and only on
# a failed inventory; it does not participate in transport or write paths.
runtime = (component / "athom_oauth_runtime.c").read_text()
assert "if (!verified) {\n                first_failure_capture_attempt(" in runtime
assert "panel_first_failure_diag_init(s_runtime_id);" in runtime
assert "panel_first_failure_diag_append_json(" in runtime
cloud = (component / "athom_cloud_client.c").read_text()
assert "first_failure" not in cloud
guard = (component / "panel_awning_write_guard.c").read_text()
assert "first_failure" not in guard
print("worker-only failure hook; transport and guard sources untouched: PASS")
