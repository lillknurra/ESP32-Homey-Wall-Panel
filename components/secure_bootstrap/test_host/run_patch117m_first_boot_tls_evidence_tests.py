#!/usr/bin/env python3
"""Targeted host and source-wiring regressions for immutable first-boot transport evidence."""
from pathlib import Path
import subprocess
import sys

component = Path(__file__).resolve().parents[1]
root = component.parents[1]

subprocess.run(
    [sys.executable, str(component / "test_host" /
                         "run_patch117k_first_boot_evidence_tests.py")],
    check=True, cwd=root)

runtime = (component / "athom_oauth_runtime.c").read_text()
cloud = (component / "athom_cloud_client.c").read_text()
header = (component / "include" / "athom_cloud_client.h").read_text()
evidence = (component / "panel_first_boot_evidence.c").read_text()

capture = runtime[runtime.index("static void first_boot_capture_attempt(",
                              runtime.index("static uint8_t first_boot_stage_code")):
                 runtime.index("static bool athom_inventory_attempt_json_append(")]
assert "diagnostic.origin == origin" in capture
assert "diagnostic.attempt == attempt" in capture
assert "diagnostic.sequence == inventory_sequence" in capture
assert "first_boot_favorites_transport(&diagnostic.favorites_read" in capture
assert "first_boot_zones_transport(&diagnostic, stage_code, final_error" in capture
zones = runtime[runtime.index("static panel_first_boot_transport_t first_boot_zones_transport"):
                runtime.index("static void first_boot_capture_attempt(",
                              runtime.index("static panel_first_boot_transport_t first_boot_zones_transport"))]
for required in (
    "failed_stage != 2U",
    "diagnostic->stage != ATHOM_INVENTORY_STAGE_ZONES",
    "diagnostic->raw_transport_observed",
    "diagnostic->raw_role != ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE",
    "diagnostic->raw_request_sequence == 0U",
    "diagnostic->raw_perform_error != (int32_t)final_error",
):
    assert required in zones
assert "diagnostic.raw_request_sequence = after->perform_count" in runtime
assert ".request_sequence = perform_window_matches ? perform_before + 1U : 0U" in cloud
assert "observed_performs == expected_performs" in cloud
assert "last_perform_role == ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE" in cloud
assert "last_perform_err == error" in cloud
assert "last_perform_http_status == http_status" in cloud
assert "classification_available = perform_window_matches" in cloud
assert "PANEL_FIRST_BOOT_TRANSPORT_TLS" in evidence
assert "result->origin == 0U" in evidence
assert "PANEL_FIRST_BOOT_EVIDENCE_JSON_MAX 1920U" in (
    component / "include" / "panel_first_boot_evidence.h").read_text()

worker = runtime[runtime.index("static void homey_command_worker(void *arg)"):]
assert worker.index("athom_inventory_attempt_diagnostic_complete(") < worker.index(
    "first_boot_capture_attempt(")
print("PATCH117M_PRODUCTION_CORRELATION_WIRING PASS")
