#!/usr/bin/env python3
from pathlib import Path
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
        "static bool athom_inventory_attempt_diagnostic_json(",
        "static void athom_inventory_attempt_diagnostic_begin(",
        "static void athom_inventory_attempt_diagnostic_complete(",
        "static esp_err_t preselection_transport_observed(",
    )
)
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
        str(source), "-o", str(binary),
    ]
    subprocess.run(command, check=True, cwd=ROOT)
    subprocess.run([str(binary)], check=True, cwd=ROOT)

print("PATCH061_ATTEMPT_DIAGNOSTICS_RUNNER PASS")
