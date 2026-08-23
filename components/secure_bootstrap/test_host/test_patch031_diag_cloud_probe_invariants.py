#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
CLOUD = (ROOT / "components/secure_bootstrap/athom_cloud_client.c").read_text(encoding="utf-8")
HEADER = (ROOT / "components/secure_bootstrap/include/athom_cloud_client.h").read_text(encoding="utf-8")
RUNTIME = (ROOT / "components/secure_bootstrap/athom_oauth_runtime.c").read_text(encoding="utf-8")

def body(src: str, signature: str) -> str:
    start = src.index(signature)
    brace = src.index("{", start)
    depth = 0
    for i in range(brace, len(src)):
        if src[i] == "{": depth += 1
        elif src[i] == "}":
            depth -= 1
            if depth == 0:
                return src[brace:i+1]
    raise AssertionError(f"unterminated function: {signature}")

probe = body(CLOUD, "esp_err_t athom_cloud_debug_probe_user_me(")
handler = body(RUNTIME, "static esp_err_t patch031_diag_cloud_user_me_probe_post(")
worker = body(RUNTIME, "static void patch031_diag_cloud_probe_worker(")
result_get = body(RUNTIME, "static esp_err_t patch031_diag_cloud_user_me_probe_result_get(")

# Raw probe: exactly one production transport request, no policy/mutation calls.
assert probe.count("http_request(") == 1
assert "ATHOM_USER_URL" in probe and "HTTP_METHOD_GET" in probe
for forbidden in (
    "athom_cloud_refresh(", "athom_cloud_fetch_user_homeys(",
    "publish_cloud_state(", "athom_auth_store_publish(",
    "athom_cloud_select_and_connect(", "athom_cloud_fetch_inventory(",
    "parse_homeys(",
):
    assert forbidden not in probe, forbidden

# Fresh-status rule: the generic request path owns request-local freshness.
assert "const int fresh_http_status = status;" in probe
assert "out->transport_response_received = status > 0;" in probe
assert "const athom_transport_class_t fresh_classification = metrics.last_classification;" in probe
assert "transport_classify(" not in probe

# Secret sanitation: no token/response body output or logging from probe.
assert "zero_secure(authorization" in probe
assert "zero_secure(response, HTTP_BODY_MAX)" in probe
assert "ESP_LOG" not in probe
assert "access_token" in probe  # presence/use only, not output

# Endpoint: POST-only, empty body, one-shot, worker guards, no product mutations.
assert '{"/homey/debug/patch031-cloud-user-me-probe",HTTP_POST,' in RUNTIME
assert '{"/homey/debug/patch031-cloud-user-me-probe-result",HTTP_GET,' in RUNTIME
assert "r->content_len != 0" in handler
assert "s_patch031_diag_probe_state" in handler
assert "xTaskCreate(" in handler
assert "PATCH031_DIAG_PROBE_WORKER_STACK" in handler
assert "athom_cloud_debug_probe_user_me(" not in handler
assert worker.count("athom_cloud_debug_probe_user_me(") == 1
assert "#define PATCH031_DIAG_PROBE_WORKER_STACK 12288U" in RUNTIME
assert "xTaskCreateStatic" not in worker
assert "xTaskCreatePinnedToCoreWithCaps" not in worker
assert "MALLOC_CAP_SPIRAM" not in worker
assert "athom_cloud_debug_probe_user_me(" not in result_get
assert "s_patch031_diag_cloud_probe_mux" not in RUNTIME
assert "static bool patch031_diag_probe_active_locked(void)" in RUNTIME
assert "s_patch031_live_refresh_running" in RUNTIME
for guard in (
    "s_worker_running", "s_select_worker_running", "s_restore_worker_running",
    "s_preselection_restore_worker_running", "s_schema_refresh_running",
    "s_refresh_job_reserved",
):
    assert guard in handler, guard
for forbidden in (
    "athom_cloud_refresh(", "publish_cloud_state(", "athom_auth_store_publish(",
    "athom_cloud_select_and_connect(", "athom_cloud_fetch_inventory(",
    "s_state_name =", "s_homey_data_state =", "s_select_attempt++",
):
    assert forbidden not in handler, forbidden

for forbidden in (
    "athom_cloud_refresh(", "publish_cloud_state(", "athom_auth_store_publish(",
    "athom_cloud_select_and_connect(", "athom_cloud_fetch_inventory(",
    "s_state_name =", "s_homey_data_state =", "s_select_attempt++",
):
    assert forbidden not in worker, forbidden
    assert forbidden not in result_get, forbidden

# Sanitized output contract only.
# After async redesign, the POST handler only accepts/reserves/starts the
# worker. Sanitized diagnostic fields are exposed only by the read-only
# result GET handler.
for field in (
    "perform_err", "fresh_http_status", "transport_response_received",
    "classification", "tls_error", "socket_errno", "elapsed_ms",
    "cloud_init", "cloud_reuse", "cloud_cleanup",
):
    assert field in result_get, field
for forbidden_text in ("refresh_token", "client_secret", "authorization_code"):
    assert forbidden_text not in handler

# Header must expose raw diagnostic fields used for fresh recomputation.
for field in ("last_socket_errno", "last_perform_err", "last_tls_query"):
    assert field in HEADER




# Patch031 async diagnostic/product Cloud concurrency interlock.
callback = body(RUNTIME, "static esp_err_t callback_get(")
select_post_body = body(RUNTIME, "static esp_err_t select_post(")
refresh_post_body = body(RUNTIME, "static esp_err_t refresh_post(")
queue_refresh = body(RUNTIME, "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready(")
preselect_gate = body(RUNTIME, "static void maybe_start_preselection_restore_worker(void)\n{")

# Diagnostic busy evaluation and UNUSED->RESERVED transition must share the
# diagnostic mux, so product reservations cannot pass between check/reserve.
assert "portENTER_CRITICAL(&s_patch031_diag_probe_mux);" in handler
assert "s_patch031_diag_probe_state = PATCH031_DIAG_PROBE_RESERVED;" in handler
assert "s_patch031_live_refresh_running" in handler
assert "!s_restore_started" in handler

# Product Cloud start reservations must use the same diagnostic mux and block
# only while diagnostic state is RESERVED/RUNNING.
for product_start in (callback, select_post_body, refresh_post_body, queue_refresh, preselect_gate):
    assert "s_patch031_diag_probe_mux" in product_start
    assert "patch031_diag_probe_active_locked()" in product_start

assert "s_worker_running = true;" in callback
assert "s_select_worker_running = true;" in select_post_body
assert "s_patch031_live_refresh_running = true;" in refresh_post_body
assert "s_refresh_job_reserved = true;" in queue_refresh
assert "s_preselection_restore_worker_running = true;" in preselect_gate

# Live refresh releases diagnostic arbitration on both success and failure.
assert refresh_post_body.count("patch031_diag_live_refresh_end();") == 2

# Terminal diagnostic states do not block normal product operation.
active_helper = body(RUNTIME, "static bool patch031_diag_probe_active_locked(")
assert "PATCH031_DIAG_PROBE_RESERVED" in active_helper
assert "PATCH031_DIAG_PROBE_RUNNING" in active_helper
assert "PATCH031_DIAG_PROBE_COMPLETE" not in active_helper
assert "PATCH031_DIAG_PROBE_FAILED" not in active_helper
assert "PATCH031_DIAG_PROBE_TASK_CREATE_FAILED" not in active_helper

# Patch031 diagnostic URI handler capacity invariant.
SECURE = (ROOT / "components/secure_bootstrap/secure_bootstrap_esp.c").read_text(encoding="utf-8")
PHONE = (ROOT / "components/secure_bootstrap/phone_provisioning_store.c").read_text(encoding="utf-8")

secure_handlers = set(re.findall(
    r'const httpd_uri_t\s+\w+\s*=\{\.uri="([^"]+)",\.method=HTTP_(GET|POST)', SECURE,
))
phone_handlers = set(re.findall(r'\{"([^"]+)",HTTP_(GET|POST),', PHONE))
runtime_handlers = set(re.findall(r'\{"([^"]+)",HTTP_(GET|POST),', RUNTIME))

assert secure_handlers == {
    ("/", "GET"), ("/networks", "GET"), ("/wifi", "POST"), ("/status", "GET"),
}
assert phone_handlers == {
    ("/homey", "GET"), ("/homey/status", "GET"), ("/homey/start", "POST"),
    ("/homey/mock/complete", "POST"), ("/homey/select", "GET"), ("/homey/select", "POST"),
    ("/homey/change", "POST"), ("/homey/lights", "GET"), ("/homey/wipe", "POST"),
}
assert runtime_handlers == {
    ("/homey/client-config", "POST"), ("/homey/login", "GET"), ("/oauth/callback", "GET"),
    ("/homey/live-status", "GET"), ("/homey/debug/patch031-cloud-user-me-probe", "POST"),
    ("/homey/live-select", "POST"), ("/homey/live-refresh", "POST"),
    ("/homey/debug/refresh-inventory-schema", "GET"),
    ("/homey/debug/patch031-cloud-user-me-probe-result", "GET"),
}
required_handler_count = len(secure_handlers) + len(phone_handlers) + len(runtime_handlers)
assert len(secure_handlers) == 4
assert len(phone_handlers) == 9
assert len(runtime_handlers) == 9
assert required_handler_count == 22
capacity_matches = re.findall(r'cfg\.max_uri_handlers\s*=\s*(\d+)\s*;', SECURE)
assert capacity_matches == ["22"]
assert int(capacity_matches[0]) >= required_handler_count

print("PATCH031_DIAG_SOURCE_INVARIANTS=PASS")
