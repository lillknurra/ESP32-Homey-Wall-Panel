from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components" / "secure_bootstrap"
CLIENT = (COMPONENT / "athom_cloud_client.c").read_text(encoding="utf-8")
HEADER = (COMPONENT / "include" / "athom_cloud_client.h").read_text(encoding="utf-8")
RUNTIME = (COMPONENT / "athom_oauth_runtime.c").read_text(encoding="utf-8")


def function_body(source: str, signature: str) -> str:
    search_from = 0
    while True:
        start = source.find(signature, search_from)
        if start < 0:
            raise AssertionError(f"missing function definition {signature}")
        brace = source.find("{", start + len(signature))
        if brace < 0:
            raise AssertionError(f"missing function body {signature}")
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
        raise AssertionError(f"unterminated function {signature}")


perform = function_body(CLIENT, "static esp_err_t http_request_limited(")
assert perform.index("esp_http_client_perform(ctx->handle)") < perform.index(
    "s_transport_metrics.perform_count++"
)
for raw_field in (
    "last_perform_role",
    "last_perform_classification",
    "last_perform_http_status",
    "last_perform_err",
    "last_tls_query",
    "last_tls_error",
    "last_tls_flags",
    "last_socket_errno",
    "last_request_elapsed_ms",
):
    assert raw_field in perform or raw_field in HEADER, raw_field
assert "last_perform_classification = classification" in perform
stage_failure = function_body(CLIENT, "static void transport_stage_failure(")
assert "last_perform_classification" not in stage_failure
assert "last_perform_role" not in stage_failure
assert "last_perform_http_status" not in stage_failure
assert re.search(r"ATHOM_TRANSPORT_ROLE_NONE.*ATHOM_TRANSPORT_ROLE_CLOUD.*ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE", HEADER, re.S)

worker = function_body(RUNTIME, "static void homey_command_worker(void *arg)")
assert worker.index("athom_inventory_attempt_diagnostic_begin(") < worker.index(
    "connect_and_fetch_inventory(selected_homey_id)"
)
assert worker.index("homey_inventory_result_verified(") < worker.index(
    "athom_inventory_attempt_diagnostic_complete(")
assert "runtime_diag_emit" in worker  # Existing journal path remains intact.
assert "homey_data_retry_delay_ms(attempt)" in worker
begin = function_body(RUNTIME, "static void athom_inventory_attempt_diagnostic_begin(")
complete = function_body(RUNTIME, "static void athom_inventory_attempt_diagnostic_complete(")
assert "athom_cloud_diagnostic_revision()" in begin
assert "athom_cloud_diagnostic_revision() != diagnostic_revision_baseline" in complete
assert '"unknown"' in complete and "? final_http_status : 0" in complete
for setter_signature in ("static void diagnostic_set(", "static void diagnostic_set_http("):
    assert "s_diagnostic_revision++" in function_body(CLIENT, setter_signature)

publish = function_body(RUNTIME, "static void athom_inventory_attempt_diagnostic_publish(")
copy = function_body(RUNTIME, "static void athom_inventory_attempt_diagnostic_copy(")
assert publish.index("portENTER_CRITICAL") < publish.index("s_last_inventory_attempt_diagnostic = *diagnostic") < publish.index("portEXIT_CRITICAL")
assert copy.index("portENTER_CRITICAL") < copy.index("*out = s_last_inventory_attempt_diagnostic") < copy.index("portEXIT_CRITICAL")

status_get = function_body(RUNTIME, "static esp_err_t status_get(httpd_req_t *r)")
assert "athom_inventory_attempt_diagnostic_copy(" in status_get
assert "athom_inventory_attempt_diagnostic_json(" in status_get
for forbidden in (
    "queue_inventory_refresh_if_ready",
    "connect_and_fetch_inventory",
    "network_phase_try_reserve",
    "esp_http_client_perform",
    "runtime_diag_journal_record",
    "nvs_set_",
    "athom_inventory_attempt_diagnostic_publish(",
):
    assert forbidden not in status_get, f"GET must remain passive: {forbidden}"
assert RUNTIME.count('"/homey/live-status"') == 1
assert RUNTIME.count("last_inventory_attempt_transport") == 1

json_fn = function_body(RUNTIME, "static bool athom_inventory_attempt_diagnostic_json(")
for forbidden in (
    "Authorization", "Bearer", "access_token", "refresh_token", "homey_id",
    "device_id", "capability_id", "remote_url", "response_body", "request_body",
    "ssid", "password",
):
    assert forbidden not in json_fn
assert "athom_inventory_attempt_stage_name(diagnostic->stage)" in json_fn
assert "raw_transport_observed ?" in json_fn
assert "raw_transport =" not in json_fn
for sensitive_stage in (
    "oauth_user_me_request", "delegation_request", "homey_login_request",
    "favorites_user_me", "inventory_zones", "inventory_devices",
):
    assert sensitive_stage in function_body(RUNTIME, "static athom_inventory_attempt_stage_t athom_inventory_attempt_stage_classify(")
assert "return ATHOM_INVENTORY_STAGE_UNKNOWN;" in function_body(
    RUNTIME, "static athom_inventory_attempt_stage_t athom_inventory_attempt_stage_classify("
)

print("PATCH061_ATTEMPT_DIAGNOSTIC_INVARIANTS PASS")
