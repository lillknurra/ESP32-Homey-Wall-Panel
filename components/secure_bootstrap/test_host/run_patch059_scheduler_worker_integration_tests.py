#!/usr/bin/env python3
from pathlib import Path
import os
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
    while start >= 0:
        brace = source.find("{", start)
        semicolon = source.find(";", start)
        if brace >= 0 and (semicolon < 0 or brace < semicolon):
            break
        start = source.find(signature, start + len(signature))
    if start < 0 or brace < 0:
        raise AssertionError(f"missing production function body: {signature}")
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
header = (COMPONENT / "include/athom_cloud_client.h").read_text()
start = header.index("/* PATCH069_FAVORITES_DIAGNOSTIC_BEGIN */")
end = header.index("/* PATCH069_FAVORITES_DIAGNOSTIC_END */")
base = base.replace("/* PATCH069_FAVORITES_TYPE */", header[start:end])
integration = PATCH059_TEMPLATE.read_text(encoding="utf-8")

patch058_marker = "/* PATCH058_PRODUCTION_FUNCTIONS */"
if base.count(patch058_marker) != 1:
    raise SystemExit("expected one Patch058 production marker")
patch058_functions = "\n\n".join(
    function_body(source, signature)
    for source, signature in (
        (client, "static uint8_t runtime_stage_id("),
        (client, "static esp_err_t inventory_error_for_http_response("),
        (client, "static void favorites_read_capture("),
        (client, "static esp_err_t count_collection("),
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
generated = '#include "athom_favorites_transport_diag.h"\n#include "runtime_event_ring.h"\n#include "panel_first_boot_evidence.h"\n#include "panel_roller_model.h"\n#include "athom_restore_policy.h"\n' + generated
network_enum_start = runtime.index("typedef enum {\n    ATHOM_NETWORK_PHASE_NONE")
network_enum_end = runtime.index("} athom_network_phase_owner_t;", network_enum_start)
network_enum = runtime[network_enum_start:network_enum_end + len("} athom_network_phase_owner_t;")]
generated = generated.replace(
    '#include "panel_roller_model.h"\n',
    '#include "panel_roller_model.h"\n' + network_enum + '\n'
    'void runtime_event_ring_record_now(uint16_t, uint8_t, int32_t, uint32_t, uint32_t);\n'
    'void runtime_event_ring_record_resources(uint16_t, uint8_t);\n',
    1,
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
network_phase_release = function_body(runtime, "static void network_phase_release(")
production = "\n\n".join(
    [
        "static void patch085_dispatch_next_awning_job(void);\n"
        "static void maybe_start_preselection_restore_worker(void);\n"
        "static void homey_command_worker_signal_deferred_work(void);\n"
        "static void homey_command_worker_run_deferred_work(void);\n"
        "static bool homey_preselection_restore_start_is_pending(void);\n"
        "static bool homey_awning_continuation_is_pending(void);\n"
        "static bool homey_deferred_work_is_pending(void);\n"
        "static bool homey_command_worker_notify(void);\n"
        "static bool awning_guard_resolution_pending(void);\n"
        "static void awning_guard_resolve_on_worker(void);\n"
        "static bool awning_guard_abandon_unsent(\n"
        "    const panel_awning_guard_record_t *pending);\n"
        "static void awning_commission_complete_on_worker(\n"
        "    const athom_homey_command_t *command);\n"
        "void athom_oauth_runtime_awning_commission_cancel_review(void);\n"
        "#define HOMEY_COMMAND_WORKER_WAKE_BIT (1UL << 0)",
        function_body(client, "static athom_homey_awning_write_result_t patch085_classify_awning_write("),
        runtime_struct,
        function_body(runtime, "static bool awning_guard_binding_matches("),
        function_body(runtime, "static void awning_guard_quarantine_locked(void)"),
        function_body(runtime, "static void awning_guard_load_on_worker_start(void)"),
        function_body(runtime, "static bool awning_guard_resolution_pending(void)"),
        function_body(runtime, "static void awning_guard_resolve_on_worker(void)"),
        function_body(runtime, "static bool awning_guard_abandon_unsent("),
        function_body(runtime, "static const char *network_phase_owner_name("),
        function_body(runtime, "static bool network_phase_try_reserve("),
        network_phase_release,
        function_body(runtime, "static bool network_phase_is_idle(void)"),
        function_body(runtime, "static bool network_phase_is_owned_by("),
        function_body(runtime, "static bool homey_preselection_restore_start_is_pending(void)"),
        function_body(runtime, "static bool homey_awning_continuation_is_pending(void)"),
        function_body(runtime, "static bool homey_deferred_work_is_pending(void)"),
        function_body(runtime, "static bool homey_command_worker_notify(void)"),
        function_body(runtime, "static void homey_command_worker_signal_deferred_work(void)"),
        function_body(runtime, "static void homey_command_worker_run_deferred_work(void)"),
        function_body(runtime, "static bool awning_commission_context_equal("),
        function_body(runtime, "static bool awning_commission_contexts_current("),
        function_body(runtime, "static bool awning_commission_runtime_idle_locked(void)"),
        function_body(runtime, "bool athom_oauth_runtime_awning_commission_review("),
        function_body(runtime, "bool athom_oauth_runtime_awning_commission_confirm(void)"),
        function_body(runtime, "void athom_oauth_runtime_awning_commission_cancel_review(void)"),
        function_body(runtime, "static bool awning_commission_worker_owned_locked("),
        function_body(runtime, "static void awning_commission_complete_on_worker("),
        function_body(runtime, "static void maybe_start_preselection_restore_worker(void)"),
        function_body(runtime, "static bool inventory_refresh_worker_should_retry("),
        function_body(runtime, "static bool inventory_refresh_worker_should_retry_after_cloud_429("),
        function_body(runtime, "static bool boot_cached_session_401_recovery_allowed("),
        function_body(runtime, "static bool inventory_refresh_retry_reenters_discovery("),
        function_body(runtime, "static bool boot_devices_incomplete_retry_allowed("),
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
        function_body(runtime, "bool athom_oauth_runtime_awning_interaction_snapshot("),
        function_body(runtime, "void athom_oauth_runtime_awning_tick("),
        function_body(runtime, "static void patch085_dispatch_next_awning_job(void)"),
        function_body(runtime, "static uint8_t runtime_event_stage_from_name(const char *stage)"),
        function_body(runtime, "void athom_oauth_runtime_awning_snapshot_observed(void)"),
        function_body(runtime, "static void patch085_complete_awning_job(void)"),
        function_body(runtime, "static void homey_command_worker("),
        function_body(runtime, "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready("),
        function_body(runtime, "static const char *refresh_queue_result_name("),
        function_body(runtime, "static void periodic_inventory_refresh_scheduler("),
    ]
)
generated += "\n\n" + integration.replace(patch059_marker, production)

with tempfile.TemporaryDirectory() as temporary_directory:
    temporary = Path(temporary_directory)
    fake_include = temporary / "fake_include"
    fake_include.mkdir()
    (fake_include / "esp_err.h").write_text(r"""
#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_INVALID_SIZE 0x104
#define ESP_ERR_NOT_FOUND 0x105
#define ESP_ERR_NOT_SUPPORTED 0x106
#define ESP_ERR_INVALID_CRC 0x107
""", encoding="utf-8")
    (fake_include / "nvs.h").write_text(r"""
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
typedef uint32_t nvs_handle_t;
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define ESP_ERR_NVS_NOT_FOUND 0x1102
esp_err_t nvs_open(const char *, int, nvs_handle_t *);
void nvs_close(nvs_handle_t);
esp_err_t nvs_get_blob(nvs_handle_t, const char *, void *, size_t *);
esp_err_t nvs_set_blob(nvs_handle_t, const char *, const void *, size_t);
esp_err_t nvs_commit(nvs_handle_t);
""", encoding="utf-8")
    source = temporary / "test_patch059_scheduler_worker_integration.c"
    source.write_text(generated, encoding="utf-8")
    variants = [
        ("default-enabled-both", []),
        ("explicit-disarmed-both", ["-DPATCH085_AWNING_2_WRITE_ARMED=0",
                                     "-DPATCH085_AWNING_3_WRITE_ARMED=0"]),
        ("awning-2-only", ["-DPATCH085_AWNING_2_WRITE_ARMED=1",
                            "-DPATCH085_AWNING_3_WRITE_ARMED=0"]),
        ("awning-3-only", ["-DPATCH085_AWNING_2_WRITE_ARMED=0",
                            "-DPATCH085_AWNING_3_WRITE_ARMED=1"]),
        ("both-explicit", ["-DPATCH085_AWNING_2_WRITE_ARMED=1",
                            "-DPATCH085_AWNING_3_WRITE_ARMED=1"]),
    ]
    focused = os.environ.get("PATCH059_TEST_CASE")
    if focused:
        focused_flags = {
            "f1": "PATCH119A_ONLY_F1",
            "f2": "PATCH119A_ONLY_F2",
            "f3": "PATCH119A_ONLY_F3",
            "f4": "PATCH119A_ONLY_F4",
            "f5": "PATCH119B_ONLY_F5",
            "f6": "PATCH119B_ONLY_F6",
            "f7": "PATCH119C_ONLY_F7",
            "f8": "PATCH119C_ONLY_F8",
            "d": "PATCH119D_ONLY",
            "e1a": "PATCH119E1A_ONLY",
            "e2b": "PATCH119E2B_ONLY",
        }
        if focused not in focused_flags:
            raise SystemExit(f"unknown PATCH059_TEST_CASE: {focused}")
        focused_generation = "patch119e2b" if focused == "e2b" else (
            "patch119e1a" if focused == "e1a" else (
                "patch119d" if focused == "d" else (
                    "patch119c" if focused in ("f7", "f8") else (
                        "patch119b" if focused in ("f5", "f6") else "patch119a"
                    )
                )
            )
        )
        variants = [(f"{focused_generation}-{focused}", [
            "-DPATCH085_AWNING_2_WRITE_ARMED=1",
            "-DPATCH085_AWNING_3_WRITE_ARMED=1",
            f"-D{focused_flags[focused]}=1",
        ])]
    for name, flags in variants:
        binary = temporary / f"test_patch059_scheduler_worker_integration_{name}"
        guard_object = temporary / f"panel_awning_write_guard_{name}.o"
        adapter_command = [
            "cc", "-DESP_PLATFORM", "-I", str(fake_include),
            "-I", str(COMPONENT / "include"), "-std=c11", "-Wall",
            "-Wextra", "-Werror", "-pedantic", "-c",
            str(COMPONENT / "panel_awning_write_guard.c"),
            "-o", str(guard_object),
        ]
        subprocess.run(adapter_command, check=True, cwd=ROOT)
        command = [
            "cc", "-I", str(COMPONENT / "include"), *flags,
            "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
            str(source), str(guard_object),
            str(COMPONENT / "panel_roller_model.c"),
            str(COMPONENT / "runtime_event_ring.c"),
            str(COMPONENT / "panel_first_boot_evidence.c"),
            str(COMPONENT / "athom_restore_policy.c"), "-pthread", "-lm",
            "-o", str(binary),
        ]
        print("PATCH059_INTEGRATION_HOST_COMPILE:", name)
        subprocess.run(command, check=True, cwd=ROOT)
        subprocess.run([str(binary)], check=True, cwd=ROOT)
print("PATCH059_SCHEDULER_WORKER_INTEGRATION_TESTS PASS")
