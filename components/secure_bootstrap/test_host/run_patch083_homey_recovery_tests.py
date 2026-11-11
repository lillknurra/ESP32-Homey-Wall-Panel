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
GUARD = COMPONENT / "panel_awning_write_guard.c"
EVENT_RING = COMPONENT / "runtime_event_ring.c"
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
guard = GUARD.read_text(encoding="utf-8")
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
integration = integration.replace(
    "    test_favorites_transport_failure_keeps_worker_inventory_ready();",
    "    test_patch083_incomplete_devices_recovery();\n"
    "    test_repeated_eagain_hands_off_to_no_snapshot_scheduler();\n"
    "    test_favorites_auth_rejection_does_not_publish_inventory();\n"
    "    test_zones_auth_rejection_stops_before_devices();\n"
    "    test_cached_401_recovery_gate_covers_inventory_stages();\n"
    "    test_favorites_401_recovers_once_and_publishes_complete_inventory();\n"
    "    test_zones_401_recovers_once_and_publishes_complete_inventory();\n"
    "    test_repeated_cached_401_has_only_one_auth_recovery();\n"
    "    test_favorites_401_cannot_reuse_old_snapshot_for_awning_write();\n"
    "    test_favorites_401_refresh_failure_stays_closed();\n"
    "    test_favorites_401_not_supported_recovers_through_worker();\n"
    "    test_zones_401_not_supported_recovers_through_worker();\n"
    "    test_devices_401_not_supported_recovers_through_worker();\n"
    "    test_not_supported_without_fresh_401_does_not_recover();\n"
    "    test_transport_start_failure_does_not_reuse_stale_401();\n"
    "    test_401_not_supported_preserves_stage_and_raw_transport_error();\n"
    "    test_cached_devices_401_uses_one_existing_oauth_recovery();\n"
    "    test_devices_403_does_not_enter_auth_recovery();\n"
    "    test_cached_devices_401_without_valid_cloud_auth_fails_closed();\n"
    "    test_invalid_cache_discovery_429_uses_long_bounded_handoff();\n"
    "    puts(\"PATCH083_HOMEY_RECOVERY_TESTS PASS\");\n"
    "    test_favorites_transport_failure_keeps_worker_inventory_ready();")
# Preserve the template's existing inventory and scheduler test calls. Only
# calls whose definitions are intentionally excluded by the recovery-only
# compilation guard are excluded from this generated entry point; those
# command-admission tests remain covered by the normal Patch059 integration
# runner, which builds them without PATCH083_HOMEY_RECOVERY_ONLY.
main_start = integration.index("int main(void)")
post_guard_call = integration.rfind(
    "    test_patch119e_homey_ready_is_independent_of_awning_guard();")
if post_guard_call < main_start:
    raise SystemExit("missing post-guard test calls in generated integration main")
integration = (
    integration[:post_guard_call]
    + "#if !defined(PATCH083_HOMEY_RECOVERY_ONLY)\n"
    + integration[post_guard_call:]
)
awning_end = "    test_awning_compile_arms_gate_queue_and_worker_independently();"
awning_end_pos = integration.find(awning_end, post_guard_call)
if awning_end_pos < 0:
    raise SystemExit("missing end of post-guard test-call block")
awning_end_pos += len(awning_end)
integration = integration[:awning_end_pos] + "\n#endif" + integration[awning_end_pos:]
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
        (client, "static uint8_t runtime_stage_id("),
        (client, "static void favorites_read_capture("),
        (client, "static esp_err_t count_collection("),
        (client, "static esp_err_t inventory_error_for_http_response("),
        (client, "static bool cached_homey_session_matches("),
        (client, "static esp_err_t inventory_discard_homey_stage_client("),
        (client, "static esp_err_t athom_cloud_fetch_inventory_impl("),
        (client, "esp_err_t athom_cloud_fetch_inventory("),
        (client, "esp_err_t athom_cloud_fetch_inventory_from_cached_session("),
        (runtime, "static bool homey_inventory_result_verified("),
        (runtime, "static bool homey_data_failure_is_transient("),
    )
)
generated = base.replace(patch058_marker, patch058_functions)
generated = (
    '#define ESP_PLATFORM 1\n'
    '#include "panel_roller_model.h"\n'
    '#include "athom_favorites_transport_diag.h"\n'
    '#include "panel_first_boot_evidence.h"\n'
    '#include "runtime_event_ring.h"\n'
    '#define runtime_event_ring_set_context(...) ((void)0)\n'
    'static uint8_t runtime_event_stage_from_name(const char *stage) { (void)stage; return RUNTIME_EVENT_STAGE_OTHER; }\n'
    'static void homey_command_worker_signal_deferred_work(void) {}\n'
    '#define HOMEY_COMMAND_WORKER_WAKE_BIT (1UL << 0)\n'
    + generated
)
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
network_owner_start = runtime.find("typedef enum {\n    ATHOM_NETWORK_PHASE_NONE = 0,")
network_owner_end = runtime.find("} athom_network_phase_owner_t;", network_owner_start)
if network_owner_start < 0 or network_owner_end < 0:
    raise SystemExit("missing production network phase owner type")
network_owner_type = runtime[network_owner_start:network_owner_end + len("} athom_network_phase_owner_t;")]
generated = generated.replace(
    '#include "panel_roller_model.h"\n',
    '#include "panel_roller_model.h"\n' + network_owner_type + "\n",
    1,
)
production = "\n\n".join(
    [
        runtime_struct,
        function_body(runtime, "static const char *network_phase_owner_name("),
        function_body(runtime, "static bool network_phase_try_reserve("),
        function_body(runtime, "static void network_phase_release("),
        function_body(runtime, "static bool awning_guard_binding_matches("),
        function_body(runtime, "static void awning_guard_quarantine_locked("),
        function_body(runtime, "static void awning_guard_load_on_worker_start("),
        function_body(runtime, "static bool awning_guard_abandon_unsent("),
        function_body(runtime, "static bool homey_command_worker_notify("),
        function_body(client, "static athom_homey_awning_write_result_t patch085_classify_awning_write("),
        *([function_body(runtime, "static bool boot_devices_incomplete_retry_allowed(")] if "static bool boot_devices_incomplete_retry_allowed(" in runtime else []),
        function_body(runtime, "static bool inventory_refresh_worker_should_retry("),
        function_body(runtime, "static bool inventory_refresh_worker_should_retry_after_cloud_429("),
        function_body(runtime, "static bool boot_cached_session_401_recovery_allowed("),
        function_body(runtime, "static bool inventory_refresh_retry_reenters_discovery("),
        function_body(runtime, "static uint32_t preselection_restore_retry_delay_ms("),
        function_body(runtime, "static bool preselection_restore_retry_allowed("),
        function_body(runtime, "static uint32_t homey_data_retry_delay_ms("),
        function_body(runtime, "static uint32_t homey_data_retry_delay_ms_for_failure("),
        function_body(runtime, "static bool periodic_refresh_scheduler_should_attempt("),
        function_body(runtime, "static bool no_snapshot_recovery_allowed("),
        function_body(runtime, "static uint32_t no_snapshot_recovery_delay_ms("),
        function_body(runtime, "static uint64_t periodic_refresh_scheduler_defer_ms("),
        function_body(runtime, "static void periodic_refresh_scheduler_record_queue_result("),
        function_body(runtime, "static const char *inventory_refresh_origin_name("),
        function_body(runtime, "static esp_err_t connect_and_fetch_inventory("),
        function_body(runtime, "static esp_err_t fetch_inventory_from_cached_session("),
        function_body(runtime, "static esp_err_t connect_and_fetch_inventory_for_origin("),
        function_body(runtime, "athom_light_toggle_dispatch_result_t athom_oauth_runtime_dispatch_light_toggle("),
        function_body(runtime, "athom_awning_queue_result_t athom_oauth_runtime_queue_awning_step("),
        function_body(runtime, "static void patch085_complete_awning_job(void)"),
        function_body(runtime, "static void homey_command_worker("),
        function_body(runtime, "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready("),
        function_body(runtime, "static const char *refresh_queue_result_name("),
        function_body(runtime, "static void periodic_inventory_refresh_scheduler("),
    ]
)
# Exercise the actual durable-guard implementation against the deterministic
# NVS mocks already present in the Patch059 integration fixture.
guard_production = "\n\n".join(
    function_body(guard, signature)
    for signature in (
        "static uint32_t guard_crc32(",
        "static uint32_t record_crc(",
        "bool panel_awning_write_guard_record_valid(",
        "static void seal_record(",
        "static esp_err_t write_and_verify(",
        "esp_err_t panel_awning_write_guard_load(",
        "esp_err_t panel_awning_write_guard_initialize_absent(",
        "esp_err_t panel_awning_write_guard_begin(",
        "esp_err_t panel_awning_write_guard_resolve(",
    )
)
production = guard_production + "\n\n" + production
# The integration template already defines its sequence and first-boot test
# adapter. Keep those single host definitions and extract production policy
# and workers only.
commission_guard = r"""
static void awning_commission_complete_on_worker(
    const athom_homey_command_t *command)
{
    (void)command;
    assert(!"recovery-only Patch083 fixture must not execute commissioning");
}
"""
network_reserved_projection = (
    "#define s_network_reserved "
    "(s_network_phase_owner != ATHOM_NETWORK_PHASE_NONE)\n"
)
integration = integration.replace(
    patch059_marker,
    '#ifndef ESP_ERR_NOT_FOUND\n#define ESP_ERR_NOT_FOUND ESP_ERR_NVS_NOT_FOUND\n#endif\n'
    '#ifndef ESP_ERR_INVALID_CRC\n#define ESP_ERR_INVALID_CRC 0x109U\n#endif\n'
    '#define GUARD_MAGIC 0x41574731U\n#define GUARD_VERSION 1U\n'
    '#define GUARD_NAMESPACE "hpanel_awning"\n#define GUARD_KEY "guard_v1"\n'
    'static TaskHandle_t s_homey_command_worker_task = (TaskHandle_t)(uintptr_t)1U;\n'
    'static void homey_command_worker_run_deferred_work(void) { /* Recovery-only fixture; deferred/awning integration is covered by Patch059. */ }\n'
    + network_reserved_projection + commission_guard + "\n" + patch059_marker,
    1,
)
generated += "\n\n" + integration.replace(patch059_marker, production)
generated += r"""
bool athom_oauth_runtime_awning_interaction_snapshot(
    size_t widget_index, athom_awning_interaction_snapshot_t *out)
{
    (void)widget_index;
    if (out != NULL) memset(out, 0, sizeof(*out));
    return false;
}
"""
generated = generated.replace(
    "#define ESP_ERR_HTTP_EAGAIN (-20)",
    "#define ESP_ERR_HTTP_EAGAIN 0x7007",
)
generated += "\nvoid athom_oauth_runtime_awning_snapshot_observed(void) {}\n"
generated_main = function_body(generated, "int main(void)")
if "test_patch083_incomplete_devices_recovery();" not in generated_main:
    raise SystemExit("Patch083 recovery cases were not inserted into the generated main")
if os.environ.get("PATCH083_BASELINE_CAPTURE") == "1":
    generated = "#define PATCH083_BASELINE_CAPTURE 1\n" + generated

with tempfile.TemporaryDirectory() as temporary_directory:
    temporary = Path(temporary_directory)
    source = temporary / "test_patch059_scheduler_worker_integration.c"
    binary = temporary / "test_patch059_scheduler_worker_integration"
    source.write_text(generated, encoding="utf-8")
    command = [
        "cc", "-I", str(COMPONENT / "include"), "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-pthread",
        "-Wno-unused-variable", "-Wno-unused-function", "-Wno-unused-but-set-variable",
        *( ["-DPATCH085_AWNING_2_WRITE_ARMED=1", "-DPATCH085_AWNING_3_WRITE_ARMED=1"]
           if os.environ.get("PATCH083_TEST_AWNING_ARMS") == "1/1" else [] ),
        str(source), str(COMPONENT / "panel_first_boot_evidence.c"),
        str(COMPONENT / "panel_roller_model.c"), str(EVENT_RING), "-o", str(binary),
    ]
    print("PATCH059_INTEGRATION_HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True, cwd=ROOT)
    subprocess.run([str(binary)], check=True, cwd=ROOT)
print("PATCH059_SCHEDULER_WORKER_INTEGRATION_TESTS PASS")
