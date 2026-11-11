#!/usr/bin/env python3
import json
import pathlib
import subprocess
import sys
import tempfile

component = pathlib.Path(__file__).resolve().parents[1]
source = component / "runtime_events_json.c"
header = component / "include" / "runtime_events_json.h"
test = component / "test_host" / "test_runtime_events_json.c"
runtime = component / "athom_oauth_runtime.c"
baseline = "--expect-baseline-invalid" in sys.argv
if "--source" in sys.argv:
    source = pathlib.Path(sys.argv[sys.argv.index("--source") + 1]).resolve()

with tempfile.TemporaryDirectory(prefix="runtime-events-json-") as temp:
    binary = pathlib.Path(temp) / "test_runtime_events_json"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(component / "include"),
        "-I", str(component.parent / "lwip" / "include"),
        str(source), str(test), "-o", str(binary),
    ], check=True)
    undefined = subprocess.check_output(["nm", "-u", str(binary)], text=True)
    forbidden = (" malloc", " calloc", " realloc", " free")
    assert not any(symbol in undefined for symbol in forbidden), undefined
    output = subprocess.check_output([str(binary)], text=True)

cases = {}
controls = set()
for line in output.splitlines():
    parts = line.split("\t", 2)
    if parts[0] == "CASE":
        assert len(parts) == 3 and parts[1] not in cases
        cases[parts[1]] = parts[2]
    elif parts[0] == "CONTROL":
        assert len(parts) == 3 and parts[2] == "PASS"
        controls.add(parts[1])
assert set(cases) == {"empty", "single", "multiple", "page_max", "page_boundary"}
assert controls == {"truncated_writer", "invalid_arguments"}

if baseline:
    invalid = []
    for name, text in cases.items():
        try:
            json.loads(text)
        except json.JSONDecodeError as error:
            invalid.append((name, error.msg))
        else:
            raise AssertionError(f"baseline {name} unexpectedly parsed as JSON")
    assert invalid, "baseline serialization defect was not reproduced"
    print("BASELINE_INVALID_JSON=PASS")
    print("BASELINE_CASES=" + repr(invalid))
else:
    parsed = {name: json.loads(text) for name, text in cases.items()}
    for name, obj in parsed.items():
        assert isinstance(obj, dict) and isinstance(obj.get("records"), list), name
        assert isinstance(obj.get("boot_progress"), dict), name
        close_diag = obj.get("socket_close_diag")
        assert isinstance(close_diag, dict) and isinstance(close_diag.get("events"), list), name
        assert len(close_diag["events"]) == close_diag["event_count"], name
        assert close_diag["capacity"] == 16, name
        assert isinstance(close_diag["scope_overflow_count"], int), name
        assert "access_token" not in obj and "device_id" not in obj, name
        assert obj["capacity"] == 48 and obj["record_size_bytes"] == 32, name
    assert parsed["empty"]["records"] == []
    assert parsed["empty"]["boot_progress"]["phase"] == 0
    assert parsed["single"]["boot_progress"]["auth_restore_started"] is True
    assert len(parsed["single"]["records"]) == 1
    assert [r["sequence"] for r in parsed["multiple"]["records"]] == [50, 51, 52]
    assert len(parsed["page_max"]["records"]) == 12
    assert parsed["page_boundary"]["page_first_sequence"] == 50
    assert parsed["page_boundary"]["page_last_sequence"] == 50
    assert parsed["page_max"]["records"][-1]["sequence"] == 61
    assert parsed["empty"]["socket_close_diag"]["events"] == []
    assert parsed["single"]["socket_close_diag"]["events"][0]["outcome"] == "release_confirmed"
    assert parsed["single"]["socket_close_diag"]["events"][0]["correlation_id"] == 0
    assert parsed["multiple"]["socket_close_diag"]["events"][1]["outcome"] == "no_close_attempt"
    assert parsed["page_max"]["socket_close_diag"]["event_count"] == 16
    assert parsed["page_max"]["socket_close_diag"]["scope_overflow_count"] == 2
    assert parsed["page_max"]["socket_close_diag"]["events"][2]["kind"] == "tls_connect_attempt_begin"
    assert parsed["page_max"]["socket_close_diag"]["events"][3]["outcome"] == "operation_failed"
    assert parsed["page_max"]["socket_close_diag"]["events"][2]["correlation_id"] == 77
    assert not any("fd" in key.lower() or "device" in key.lower()
        for event in parsed["page_max"]["socket_close_diag"]["events"] for key in event)
    assert all("correlation_id" in event for event in parsed["page_max"]["socket_close_diag"]["events"])
    assert not any("task" in key.lower() or "handle" in key.lower()
        for event in parsed["page_max"]["socket_close_diag"]["events"] for key in event)
    expected_record_keys = {
        "sequence", "monotonic_ms", "runtime_id", "inventory_sequence", "event_id",
        "event", "origin", "stage", "result", "value_a", "value_b",
    }
    assert set(parsed["single"]["records"][0]) == expected_record_keys

    # Keep the existing route and its registered handler; this patch only fixes
    # serialization behind the existing endpoint.
    runtime_source = runtime.read_text(encoding="utf-8")
    assert '{"/homey/debug/runtime-events",HTTP_GET,runtime_events_get,NULL}' in runtime_source
    assert "runtime_diag_parse_query(request, &limit, &before_sequence)" in runtime_source
    assert "httpd_resp_set_type(request, \"application/json; charset=utf-8\")" in runtime_source
    assert "httpd_resp_set_hdr(request, \"Cache-Control\", \"no-store\")" in runtime_source
    assert "httpd_resp_send_chunk(request, NULL, 0U)" in runtime_source
    assert "Authorization" not in source.read_text(encoding="utf-8")
    print("JSON_CASES=5 PASS (empty, single, multiple, maximum page, pagination boundary)")
    print("SCHEMA_ORDER_PRIVACY=PASS")
    print("SOCKET_CLOSE_DIAGNOSTICS_BOUNDED=PASS")
    print("BOUNDED_CHUNKS_NO_HEAP=PASS")
print(output, end="")
