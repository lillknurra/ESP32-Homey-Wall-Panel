#!/usr/bin/env python3
import json
from pathlib import Path
import subprocess
import tempfile

COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]

with tempfile.TemporaryDirectory() as td:
    binary = Path(td) / "first_boot_evidence_test"
    subprocess.run(
        ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
         "-pthread", "-I", str(COMPONENT / "include"),
         str(COMPONENT / "panel_first_boot_evidence.c"),
         str(COMPONENT / "test_host" / "test_panel_first_boot_evidence.c"),
         "-o", str(binary)], check=True, cwd=ROOT)
    result = subprocess.run([str(binary)], check=True, capture_output=True,
                            text=True, cwd=ROOT)
    lines = result.stdout.splitlines()
    empty_line = next(line for line in lines if line.startswith("PATCH117L_EMPTY_JSON="))
    empty_json = empty_line.split("=", 1)[1]
    empty_parsed = json.loads(empty_json)
    assert empty_parsed["first_boot_evidence"]["version"] == 1

    existing_line = next(line for line in lines if line.startswith("PATCH117L_EXISTING_JSON="))
    existing_json = existing_line.split("=", 1)[1]
    existing_parsed = json.loads(existing_json)
    assert existing_parsed["state"] == "error"
    assert existing_parsed["first_boot_evidence"]["version"] == 1

    transport_line = next(line for line in lines
                          if line.startswith("PATCH117M_TRANSPORT_JSON="))
    transport_parsed = json.loads(transport_line.split("=", 1)[1])
    transport = transport_parsed["first_boot_evidence"]
    assert transport["inventory_state"] == "failed"
    assert transport["favorites_transport"]["request_sequence"] == 401
    assert transport["favorites_transport"]["request_count"] == 2
    assert transport["favorites_transport"]["failed_request"] == 1
    assert transport["favorites_transport"]["tls_query"] == 32769
    assert transport["zones_transport"]["socket_errno"] == 104
    assert transport["zones_transport"]["tls_query"] == 0
    assert "token" not in transport_line and "remote_url" not in transport_line
    unavailable_line = next(line for line in lines
                            if line.startswith("PATCH117M_UNAVAILABLE_JSON="))
    unavailable = json.loads(unavailable_line.split("=", 1)[1])
    assert unavailable["first_boot_evidence"]["favorites_transport"]["tls_query"] is None
    assert unavailable["first_boot_evidence"]["favorites_transport"]["socket_errno"] == 11
    assert unavailable["first_boot_evidence"]["zones_transport"]["available"] is False

    json_line = next(line for line in lines if line.startswith('{'))
    worst_case_json = next(line for line in lines if line.startswith('{') and '4294967295' in line)
    parsed = json.loads(json_line)
    assert parsed["state"] == "error"
    evidence = parsed["first_boot_evidence"]
    assert evidence["version"] == 1
    assert evidence["inventory_state"] == "not_started"
    assert evidence["devices_expected_bytes"] == -1
    assert evidence["heap_minimum_available"] is False
    assert evidence["largest_free_block_minimum_available"] is False
    assert "token" not in json_line and "remote_url" not in json_line
    assert "PANEL_FIRST_BOOT_EVIDENCE_HOST_TESTS PASS" in result.stdout
    print(f"PATCH117M_FIRST_BOOT_JSON_BYTES={len(json_line.encode('utf-8'))}")
    assert "completed_successfully" not in worst_case_json
    assert len(worst_case_json.encode('utf-8')) < 1920
    print(f"PATCH117M_FIRST_BOOT_WORST_CASE_JSON_BYTES={len(worst_case_json.encode('utf-8'))}")
    ram_line = next(line for line in lines
                    if line.startswith("PATCH117M_STATIC_EVIDENCE_BYTES="))
    ram_bytes = int(ram_line.split("=", 1)[1])
    assert ram_bytes <= 256
    print(f"PATCH117M_STATIC_EVIDENCE_BYTES={ram_bytes}")

RUNTIME = (COMPONENT / "athom_oauth_runtime.c").read_text()
ESP = (COMPONENT / "secure_bootstrap_esp.c").read_text()
JOURNAL = (COMPONENT / "runtime_diag_journal.c").read_text()
CM = (COMPONENT / "CMakeLists.txt").read_text()
assert '"panel_first_boot_evidence.c"' in CM
assert "panel_first_boot_evidence_init(s_runtime_id)" in RUNTIME
assert "panel_first_boot_evidence_inventory_started(" in RUNTIME
assert "static void first_boot_capture_attempt(" in RUNTIME
assert "panel_first_boot_evidence_ready(" in RUNTIME
assert "panel_first_boot_evidence_recovery_used(" in RUNTIME
assert 'memcmp(query, "first_boot=1", sizeof("first_boot=1"))' in RUNTIME
assert "PANEL_FIRST_BOOT_EVIDENCE_JSON_MAX" in RUNTIME
first_boot_fast_path = RUNTIME[RUNTIME.index("if (first_boot_only) {"):RUNTIME.index("const size_t body_capacity = ATHOM_HOMEY_LIVE_STATUS_JSON_MAX")]
assert "panel_first_boot_evidence_append_json" in first_boot_fast_path
assert "athom_" not in first_boot_fast_path
assert '"/homey/live-status",HTTP_GET,status_get,NULL' in RUNTIME
assert '"/homey/debug/runtime-events",HTTP_GET,runtime_events_get,NULL' in RUNTIME
assert "runtime_diag_journal_pause_flash_writes()" in ESP
assert "s_flash_writes_paused = true;" in JOURNAL
assert "record_state_accepts(s_available, s_full, s_append_fault," in JOURNAL
print("PATCH117M_PRODUCTION_HOOK_AND_JOURNAL_INVARIANTS PASS")
