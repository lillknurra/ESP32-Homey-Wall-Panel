#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components" / "secure_bootstrap"
TEST_DIR = COMPONENT / "test_host"
CLIENT = COMPONENT / "athom_cloud_client.c"
RUNTIME = COMPONENT / "athom_oauth_runtime.c"
TEMPLATE = TEST_DIR / "test_athom_cloud_inventory_snapshot_gate.c"


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise AssertionError(f"missing production function: {signature}")
    brace = source.find("{", start)
    if brace < 0:
        raise AssertionError(f"missing function body: {signature}")
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated function: {signature}")


client_text = CLIENT.read_text(encoding="utf-8")
runtime_text = RUNTIME.read_text(encoding="utf-8")
template = TEMPLATE.read_text(encoding="utf-8")
header = (COMPONENT / "include/athom_cloud_client.h").read_text()
type_start = header.index("/* PATCH069_FAVORITES_DIAGNOSTIC_BEGIN */")
type_end = header.index("/* PATCH069_FAVORITES_DIAGNOSTIC_END */")
template = template.replace("/* PATCH069_FAVORITES_TYPE */", header[type_start:type_end])
marker = "/* PATCH058_PRODUCTION_FUNCTIONS */"
if template.count(marker) != 1:
    raise SystemExit("expected exactly one Patch058 function marker")

production_functions = "\n\n".join(
    (
        function_body(client_text, "static void favorites_read_capture("),
        function_body(client_text, "static esp_err_t count_collection("),
        function_body(client_text, "static bool cached_homey_session_matches("),
        function_body(client_text, "static esp_err_t athom_cloud_fetch_inventory_impl("),
        function_body(client_text, "esp_err_t athom_cloud_fetch_inventory("),
        function_body(client_text, "esp_err_t athom_cloud_fetch_inventory_from_cached_session("),
        function_body(runtime_text, "static bool homey_inventory_result_verified("),
        function_body(runtime_text, "static bool homey_data_failure_is_transient("),
    )
)
generated = template.replace(marker, production_functions)

with tempfile.TemporaryDirectory() as temporary_directory:
    temporary = Path(temporary_directory)
    source = temporary / "test_athom_cloud_inventory_snapshot_gate.c"
    binary = temporary / "test_athom_cloud_inventory_snapshot_gate"
    source.write_text(generated, encoding="utf-8")
    command = [
        "cc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic",
        str(source),
        "-o",
        str(binary),
    ]
    print("PATCH058_HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True, cwd=ROOT)
    subprocess.run([str(binary)], check=True, cwd=ROOT)
print("ATHOM_CLOUD_INVENTORY_SNAPSHOT_GATE_TESTS PASS")
