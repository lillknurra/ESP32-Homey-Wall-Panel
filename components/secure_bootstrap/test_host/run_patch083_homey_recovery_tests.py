#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile
import os
import re


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
runtime = Path(os.environ.get("PATCH083_RUNTIME_SOURCE", str(RUNTIME))).read_text(encoding="utf-8")
base = PATCH058_TEMPLATE.read_text(encoding="utf-8")
header = (COMPONENT / "include/athom_cloud_client.h").read_text()
start = header.index("/* PATCH069_FAVORITES_DIAGNOSTIC_BEGIN */")
end = header.index("/* PATCH069_FAVORITES_DIAGNOSTIC_END */")
base = base.replace("/* PATCH069_FAVORITES_TYPE */", header[start:end])
integration = PATCH059_TEMPLATE.read_text(encoding="utf-8")
integration = "#define PATCH083_HOMEY_RECOVERY_ONLY 1\n" + integration
integration = "void athom_oauth_runtime_awning_snapshot_observed(void);\n" + integration
integration = integration.replace(
    "static void reset_awning_worker_case(",
    "#if !defined(PATCH083_HOMEY_RECOVERY_ONLY)\nstatic void reset_awning_worker_case(",
    1,
)
integration = integration.replace(
    "int main(void)",
    "#endif\nint main(void)",
    1,
)
integration = integration.replace(
    "#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED\n"
    "    test_awning_queue_worker_and_authoritative_confirmation();\n"
    "    test_awning_request_debounce_matches_450ms_boundary();\n"
    "#endif\n    test_awning_compile_arms_gate_queue_and_worker_independently();\n",
    "",
)
for name in ("ATHOM_HOMEY_DATA_RETRY_1_MS", "ATHOM_HOMEY_DATA_RETRY_2_MS", "ATHOM_HOMEY_DATA_RETRY_3_MS", "ATHOM_HOMEY_DATA_RETRY_MAX_MS"):
    value = re.search(r"#define " + name + r" (\d+U)", runtime).group(1)
    integration = re.sub(r"(#define " + name + r" )\d+U", r"\g<1>" + value, integration)
integration = integration.replace("8000U", "ATHOM_HOMEY_DATA_RETRY_MAX_MS")
extra = (TEST_DIR / "test_patch083_homey_recovery.c").read_text()
integration = integration.replace("int main(void)", extra + "\nint main(void)")
integration = integration.replace("    if (s_worker_active) return;", "    if (s_worker_active) { if (s_v41_active) { assert(s_snapshot_publish_count == 0U); assert(!s_device_snapshot_store.active); assert(s_homey_data_state == ATHOM_HOMEY_DATA_RETRYING); s_v41_wait += ticks; if (s_v41_corrupt_identity) strcpy(s_cloud.selected_homey.id, \"different-synthetic-homey\"); if (s_v41_alias_fail) s_alias_activation_result = ESP_FAIL; if (s_v41_invalid_session) s_cloud.homey_session_token[0] = 0; } return; }")
integration = integration.replace("    test_favorites_transport_failure_keeps_worker_inventory_ready();", "    test_patch083_incomplete_devices_recovery();\n    test_favorites_transport_failure_keeps_worker_inventory_ready();")
base = base.replace("static unsigned s_request_count;", "static unsigned s_v41_device_failures, s_v41_device_calls, s_v41_wait;\nstatic int s_v41_http_status;\nstatic esp_err_t s_v41_perform_error;\nstatic bool s_v41_active, s_v41_corrupt_identity, s_v41_alias_fail, s_v41_timeout_between, s_v41_invalid_session;\nstatic unsigned s_request_count;")
base = base.replace("assert(s_response_count < 4U)", "assert(s_response_count < 16U)").replace("[4]", "[16]")
base = base.replace("    const size_t capacity = 96U;", """    if (s_collection_kind == 2) {
        s_v41_device_calls++;
        if (s_v41_device_failures > 0U) {
            s_v41_device_failures--;
            s_http_status = s_v41_http_status;
            *status_out = s_v41_http_status;
            if (s_v41_timeout_between && s_v41_device_calls == 2U) return ESP_ERR_TIMEOUT;
            return s_v41_perform_error;
        }
    }
    const size_t capacity = 96U;""")
base = "#define ESP_ERR_HTTP_INCOMPLETE_DATA 0x700C\n" + base


patch058_marker = "/* PATCH058_PRODUCTION_FUNCTIONS */"
if base.count(patch058_marker) != 1:
    raise SystemExit("expected one Patch058 production marker")
patch058_functions = "\n\n".join(
    function_body(source, signature)
    for source, signature in (
        (client, "static void favorites_read_capture("),
        (client, "static esp_err_t count_collection("),
        (client, "static bool cached_homey_session_matches("),
        (client, "static esp_err_t athom_cloud_fetch_inventory_impl("),
        (client, "esp_err_t athom_cloud_fetch_inventory("),
        (client, "esp_err_t athom_cloud_fetch_inventory_from_cached_session("),
        (runtime, "static bool homey_inventory_result_verified("),
        (runtime, "static bool homey_data_failure_is_transient("),
    )
)
generated = base.replace(patch058_marker, patch058_functions)
generated = '#include "athom_favorites_transport_diag.h"\n' + generated
homey_fields = 'char id[ATHOM_HOMEY_ID_MAX];\n    char remote_url[ATHOM_HOMEY_URL_MAX];'
if generated.count(homey_fields) != 1:
    raise SystemExit("expected one host Homey model definition")
generated = generated.replace(
    homey_fields,
    'char id[ATHOM_HOMEY_ID_MAX];\n    char name[32];\n    char remote_url[ATHOM_HOMEY_URL_MAX];',
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
        *([function_body(runtime, "static bool boot_devices_incomplete_retry_allowed(")] if "static bool boot_devices_incomplete_retry_allowed(" in runtime else []),
        function_body(runtime, "static bool inventory_refresh_worker_should_retry("),
        function_body(runtime, "static bool inventory_refresh_worker_should_retry_after_cloud_429("),
        function_body(runtime, "static bool inventory_refresh_retry_reenters_discovery("),
        function_body(runtime, "static uint32_t preselection_restore_retry_delay_ms("),
        function_body(runtime, "static bool preselection_restore_retry_allowed("),
        function_body(runtime, "static bool periodic_refresh_scheduler_should_attempt("),
        function_body(runtime, "static bool no_snapshot_recovery_allowed("),
        function_body(runtime, "static uint32_t no_snapshot_recovery_delay_ms("),
        function_body(runtime, "static uint64_t periodic_refresh_scheduler_defer_ms("),
        function_body(runtime, "static void periodic_refresh_scheduler_record_queue_result("),
        function_body(runtime, "static uint32_t homey_data_retry_delay_ms("),
        function_body(runtime, "static uint32_t homey_data_retry_delay_ms_for_failure("),
        function_body(runtime, "static const char *inventory_refresh_origin_name("),
        function_body(runtime, "static esp_err_t connect_and_fetch_inventory("),
        function_body(runtime, "static esp_err_t fetch_inventory_from_cached_session("),
        function_body(runtime, "static esp_err_t connect_and_fetch_inventory_for_origin("),
        function_body(runtime, "athom_light_toggle_dispatch_result_t athom_oauth_runtime_dispatch_light_toggle("),
        function_body(runtime, "static void patch085_complete_awning_job(void)"),
        function_body(runtime, "static void homey_command_worker("),
        function_body(runtime, "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready("),
        function_body(runtime, "static const char *refresh_queue_result_name("),
        function_body(runtime, "static void periodic_inventory_refresh_scheduler("),
    ]
)
generated += "\n\n" + integration.replace(patch059_marker, production)
generated += "\nvoid athom_oauth_runtime_awning_snapshot_observed(void) {}\n"
if os.environ.get("PATCH083_BASELINE_CAPTURE") == "1":
    generated = "#define PATCH083_BASELINE_CAPTURE 1\n" + generated

with tempfile.TemporaryDirectory() as temporary_directory:
    temporary = Path(temporary_directory)
    source = temporary / "test_patch059_scheduler_worker_integration.c"
    binary = temporary / "test_patch059_scheduler_worker_integration"
    source.write_text(generated, encoding="utf-8")
    command = [
        "cc", "-I", str(COMPONENT / "include"), "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-Wno-unused-variable", "-Wno-unused-function",
        str(source), "-o", str(binary),
    ]
    print("PATCH059_INTEGRATION_HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True, cwd=ROOT)
    subprocess.run([str(binary)], check=True, cwd=ROOT)
print("PATCH059_SCHEDULER_WORKER_INTEGRATION_TESTS PASS")
