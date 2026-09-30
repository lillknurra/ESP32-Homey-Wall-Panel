#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components" / "secure_bootstrap"
TEST_DIR = COMPONENT / "test_host"
CLIENT = COMPONENT / "athom_cloud_client.c"
RUNTIME = COMPONENT / "athom_oauth_runtime.c"
PATCH058_TEMPLATE = TEST_DIR / "test_athom_cloud_inventory_snapshot_gate.c"
PATCH059_TEMPLATE = TEST_DIR / "test_patch059_scheduler_worker_integration.c"


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


client = CLIENT.read_text(encoding="utf-8")
runtime = RUNTIME.read_text(encoding="utf-8")
base = PATCH058_TEMPLATE.read_text(encoding="utf-8")
integration = PATCH059_TEMPLATE.read_text(encoding="utf-8")

patch058_marker = "/* PATCH058_PRODUCTION_FUNCTIONS */"
if base.count(patch058_marker) != 1:
    raise SystemExit("expected one Patch058 production marker")
patch058_functions = "\n\n".join(
    function_body(source, signature)
    for source, signature in (
        (client, "static esp_err_t count_collection("),
        (client, "esp_err_t athom_cloud_fetch_inventory("),
        (runtime, "static bool homey_inventory_result_verified("),
        (runtime, "static bool homey_data_failure_is_transient("),
    )
)
generated = base.replace(patch058_marker, patch058_functions)
homey_fields = 'char id[64];\n    char remote_url[ATHOM_HOMEY_URL_MAX];'
if generated.count(homey_fields) != 1:
    raise SystemExit("expected one host Homey model definition")
generated = generated.replace(
    homey_fields,
    'char id[64];\n    char name[32];\n    char remote_url[ATHOM_HOMEY_URL_MAX];',
    1,
)
if generated.count("int main(void)") != 1:
    raise SystemExit("expected one Patch058 host-test main")
generated = generated.replace("int main(void)", "int patch058_main(void)", 1)

patch059_marker = "#define PATCH059_RUNTIME_PRODUCTION_FUNCTIONS"
if integration.count(patch059_marker) != 1:
    raise SystemExit("expected one Patch059 integration marker")
runtime_struct_start = runtime.find("typedef struct {\n    bool snapshot_seen;")
runtime_struct_end = runtime.find("} periodic_refresh_scheduler_state_t;", runtime_struct_start)
if runtime_struct_start < 0 or runtime_struct_end < 0:
    raise SystemExit("missing production periodic scheduler state")
runtime_struct = runtime[runtime_struct_start:runtime_struct_end + len("} periodic_refresh_scheduler_state_t;")]
production = "\n\n".join(
    [
        runtime_struct,
        function_body(runtime, "static bool inventory_refresh_worker_should_retry("),
        function_body(runtime, "static bool periodic_refresh_scheduler_should_attempt("),
        function_body(runtime, "static uint64_t periodic_refresh_scheduler_defer_ms("),
        function_body(runtime, "static void periodic_refresh_scheduler_record_queue_result("),
        function_body(runtime, "static uint32_t homey_data_retry_delay_ms("),
        function_body(runtime, "static const char *inventory_refresh_origin_name("),
        function_body(runtime, "static esp_err_t connect_and_fetch_inventory("),
        function_body(runtime, "athom_light_toggle_dispatch_result_t athom_oauth_runtime_dispatch_light_toggle("),
        function_body(runtime, "static void homey_command_worker("),
        function_body(runtime, "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready("),
        function_body(runtime, "static const char *refresh_queue_result_name("),
        function_body(runtime, "static void periodic_inventory_refresh_scheduler("),
    ]
)
generated += "\n\n" + integration.replace(patch059_marker, production)

with tempfile.TemporaryDirectory() as temporary_directory:
    temporary = Path(temporary_directory)
    source = temporary / "test_patch059_scheduler_worker_integration.c"
    binary = temporary / "test_patch059_scheduler_worker_integration"
    source.write_text(generated, encoding="utf-8")
    command = [
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        str(source), "-o", str(binary),
    ]
    print("PATCH059_INTEGRATION_HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True, cwd=ROOT)
    subprocess.run([str(binary)], check=True, cwd=ROOT)
print("PATCH059_SCHEDULER_WORKER_INTEGRATION_TESTS PASS")
