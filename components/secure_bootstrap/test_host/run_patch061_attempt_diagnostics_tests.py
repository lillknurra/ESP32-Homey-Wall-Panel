#!/usr/bin/env python3
from pathlib import Path
import json
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
RUNTIME = ROOT / "components" / "secure_bootstrap" / "athom_oauth_runtime.c"
TEMPLATE = Path(__file__).with_name("test_patch061_attempt_diagnostics.c")


def function_body(source: str, signature: str) -> str:
    search_from = 0
    while True:
        start = source.find(signature, search_from)
        if start < 0:
            raise AssertionError(f"missing production function definition: {signature}")
        brace = source.find("{", start + len(signature))
        if brace < 0:
            raise AssertionError(f"missing function body: {signature}")
        if source.find(";", start + len(signature), brace) >= 0:
            search_from = start + len(signature)
            continue
        depth = 0
        for index in range(brace, len(source)):
            if source[index] == "{":
                depth += 1
            elif source[index] == "}":
                depth -= 1
                if depth == 0:
                    return source[start:index + 1]
        raise AssertionError(f"unterminated production function: {signature}")


prototype_probe = """static void probe(void);
static void wrapper(void) { probe(); }
static void probe(void) { portENTER_CRITICAL(&mux); portEXIT_CRITICAL(&mux); }
"""
probe_body = function_body(prototype_probe, "static void probe(void)")
assert "portENTER_CRITICAL" in probe_body
assert "wrapper" not in probe_body


runtime = RUNTIME.read_text(encoding="utf-8")
template = TEMPLATE.read_text(encoding="utf-8")
header = (ROOT / "components/secure_bootstrap/include/athom_cloud_client.h").read_text()
type_start = header.index("/* PATCH069_FAVORITES_DIAGNOSTIC_BEGIN */")
type_end = header.index("/* PATCH069_FAVORITES_DIAGNOSTIC_END */")
template = template.replace("/* PATCH069_FAVORITES_TYPE */", header[type_start:type_end])
marker = "/* PATCH061_PRODUCTION_DIAGNOSTICS */"
if template.count(marker) != 1:
    raise SystemExit("expected exactly one Patch061 production-function marker")

types_start = "/* PATCH061_DIAGNOSTIC_TYPES_BEGIN */"
types_end = "/* PATCH061_DIAGNOSTIC_TYPES_END */"
start = runtime.index(types_start)
end = runtime.index(types_end, start) + len(types_end)
production_types = runtime[start:end]
capacity_match = re.search(
    r"(?m)^#define ATHOM_INVENTORY_ATTEMPT_DIAGNOSTIC_JSON_MAX [^\n]+$",
    runtime,
)
if capacity_match is None:
    raise SystemExit("missing production diagnostic JSON capacity")
production_capacity = capacity_match.group(0)
production_functions = "\n\n".join(
    function_body(runtime, signature)
    for signature in (
        "static const char *inventory_refresh_origin_name(",
        "static const char *athom_inventory_attempt_stage_name(",
        "static athom_inventory_attempt_stage_t athom_inventory_attempt_stage_classify(",
        "static const char *athom_inventory_attempt_role_name(",
        "static uint32_t athom_inventory_attempt_counter_delta(",
        "static athom_inventory_attempt_diagnostic_t athom_inventory_attempt_build(",
        "static void athom_inventory_attempt_diagnostic_publish(",
        "static void athom_inventory_attempt_diagnostic_copy(",
        "static bool athom_inventory_attempt_json_append(",
        "static bool athom_favorites_fetch_diagnostic_json_append(",
        "static bool athom_favorites_fin_recovery_json_append(",
        "static bool athom_inventory_attempt_diagnostic_json(",
        "static void athom_inventory_attempt_diagnostic_begin(",
        "static void athom_inventory_attempt_diagnostic_complete(",
        "static esp_err_t preselection_transport_observed(",
    )
)
pre_tls_source = (ROOT / "components/secure_bootstrap/athom_pre_tls_diag.c").read_text()
pre_tls_functions = "\n".join(function_body(pre_tls_source, signature) for signature in (
    "static int bounded_error(", "static const char *handshake_state_name(", "static bool handshake_json(", "static const char *result_class(", "bool athom_pre_tls_diag_json("))
favorites_source = (ROOT / "components/secure_bootstrap/athom_favorites_transport_diag.c").read_text()
favorites_class = function_body(favorites_source, "const char *athom_favorites_transport_diag_class(")
template = '#include <stdio.h>\n#include <string.h>\n#include "athom_pre_tls_diag.h"\n#include "athom_favorites_transport_diag.h"\n' + favorites_class + '\n' + pre_tls_functions + "\n" + template
generated = template.replace(
    marker,
    production_types + "\n" + production_capacity + "\n\n" + production_functions,
)

with tempfile.TemporaryDirectory() as temporary_directory:
    temporary = Path(temporary_directory)
    source = temporary / "test_patch061_attempt_diagnostics.c"
    binary = temporary / "test_patch061_attempt_diagnostics"
    source.write_text(generated, encoding="utf-8")
    command = [
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-pthread",
        "-I", str(ROOT / "components/secure_bootstrap/include"),
        str(source), "-o", str(binary),
    ]
    subprocess.run(command, check=True, cwd=ROOT)
    result = subprocess.run(
        [str(binary)], check=False, cwd=ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stderr, end="")
        raise SystemExit(result.returncode)
    maximum_lines = [line for line in result.stdout.splitlines()
                     if line.startswith("PATCH070_MAX_ATTEMPT_JSON_BYTES=")]
    assert len(maximum_lines) == 1
    maximum = int(maximum_lines[0].split("=", 1)[1])
    model_header = (ROOT / "components/secure_bootstrap/include/athom_cloud_model.h").read_text()
    live_capacity = int(re.search(r"#define ATHOM_HOMEY_LIVE_STATUS_JSON_MAX (\d+)U", model_header).group(1))
    awning_capacity = int(re.search(r"#define ATHOM_HOMEY_AWNING_SNAPSHOT_JSON_MAX (\d+)U", model_header).group(1))
    # Covers bounded diagnostic status fields and both inserted property names.
    assert maximum + awning_capacity + 512 < live_capacity
    print(maximum_lines[0], "LIVE_STATUS_CAPACITY=PASS")
    sample_lines = [line for line in result.stdout.splitlines()
                    if line.startswith("PATCH063_JSON_SAMPLE=")]
    if len(sample_lines) != 1:
        raise SystemExit("expected one serialized Patch063 attempt JSON sample")
    sample = json.loads(sample_lines[0].split("=", 1)[1])
    favorites_lines = [line for line in result.stdout.splitlines()
                       if line.startswith("PATCH069_JSON_SAMPLE=")]
    assert len(favorites_lines) == 1
    isolated = json.loads(favorites_lines[0].split("=", 1)[1])
    outcome = isolated["read_outcome"]
    assert outcome["snapshot_published"] is True
    assert outcome["favorites"]["error"] == 0x7004
    assert outcome["favorites"]["data_verified"] is False
    assert isolated["raw_transport"]["http_status"] == 200
    assert all(type(value) in (int, bool) for key, value in outcome["favorites"].items() if key not in ("fetch", "fin_recovery"))
    fetch = outcome["favorites"]["fetch"]
    assert fetch["valid"] is True and fetch["last_header_read_result"] == -0x7100
    assert fetch["class"] == "header_zero_bytes_error"
    assert fetch["first_connection_reused"] is True
    assert all(type(value) in (int, bool, str) or value is None for value in fetch.values())
    recovery = outcome["favorites"]["fin_recovery"]
    assert recovery["eligible"] is True and recovery["attempted"] is True
    assert recovery["fresh_connection"] is True and recovery["result"] == "failed"
    assert recovery["logical_request_attempt_count"] == 2
    assert recovery["original"]["tls_query"] == 0x8008
    assert recovery["original"]["fetch"]["request_result"] == 0x7004
    assert recovery["original"]["fetch"]["last_header_read_result"] == -1
    assert all(type(value) in (int, bool, str) or value is None
               for key,value in recovery.items() if key != "original")
    memory = sample["raw_transport"]["tls_memory"]
    assert sample["raw_transport"]["role"] == "cloud"
    assert memory["scope"] == "perform_window"
    assert memory["matching_failure"]["internal_8bit_largest"] == 1024
    assert memory["internal_8bit"]["free_before"] == 50000

print("PATCH061_ATTEMPT_DIAGNOSTICS_RUNNER PASS")
