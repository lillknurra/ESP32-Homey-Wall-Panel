#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
CLOUD = ROOT / "components/secure_bootstrap/athom_cloud_client.c"
RUNTIME = ROOT / "components/secure_bootstrap/athom_oauth_runtime.c"
cloud = CLOUD.read_text()
runtime = RUNTIME.read_text()


def body(text, signature):
    start = text.find(signature)
    assert start >= 0, signature
    brace = text.find("{", start)
    assert brace >= 0, signature
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    raise AssertionError("unterminated:" + signature)

request = body(cloud, "static esp_err_t http_request_limited(")
classifier = body(cloud, "static athom_transport_class_t transport_classify(")
probe = body(cloud, "esp_err_t athom_cloud_debug_probe_user_me(")

checks = {
    "cloud_timeout_12000": "#define CLOUD_HTTP_TIMEOUT_MS 12000" in cloud,
    "homey_timeout_8000": "#define HOMEY_REMOTE_HTTP_TIMEOUT_MS 8000" in cloud,
    "request_local_flag": "bool fresh_status_received;" in cloud,
    "request_local_status": "int fresh_http_status;" in cloud,
    "status_event": "case HTTP_EVENT_ON_STATUS_CODE:" in cloud,
    "event_sets_flag": "buffer->fresh_status_received = true;" in cloud,
    "event_sets_status": "buffer->fresh_http_status = status;" in cloud,
    "fresh_rule": "const int fresh_http_status = response_received ? buffer.fresh_http_status : 0;" in request,
    "no_persistent_status_getter": "esp_http_client_get_status_code(ctx->handle)" not in request,
    "status_out_fresh_only": "if (response_received) *status_out = fresh_http_status;" in request,
    "success_status_fresh_only": "*status_out = fresh_http_status;" in request,
    "memory_uses_fresh": "patch019a16f_cloud_after_perform(err, fresh_http_status);" in request,
    "classifier_uses_fresh": re.search(r"transport_classify\(\s*err,\s*fresh_http_status,", request) is not None,
    "metrics_uses_fresh": "s_transport_metrics.last_http_status = fresh_http_status;" in request,
    "response_received_logged_twice": request.count("response_received=%s http_status=%d") == 2,
    "token_error_preserves_fresh_status": "diagnostic_set_http(\"oauth_token_request\", err, status);" in cloud,
    "probe_status_contract": "const int fresh_http_status = status;" in probe,
    "probe_response_contract": "out->transport_response_received = status > 0;" in probe,
    "probe_uses_generic_classification": "const athom_transport_class_t fresh_classification = metrics.last_classification;" in probe,
    "probe_no_reclassification": "transport_classify(" not in probe,
    "classifier_http_first": classifier.find("if (http_status == 401)") < classifier.find("if (err == ESP_OK)"),
    "preselect_attempts_unchanged": "#define ATHOM_PRESELECT_RESTORE_MAX_ATTEMPTS 12U" in runtime,
    "preselect_retry_unchanged": "#define ATHOM_PRESELECT_RESTORE_RETRY_MS 2000U" in runtime,
    "preselect_elapsed_unchanged": "#define ATHOM_PRESELECT_RESTORE_MAX_ELAPSED_MS 120000U" in runtime,
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f"PATCH031_FRESHNESS_TIMEOUT_INVARIANT {name}={'PASS' if ok else 'FAIL'}")
if failed:
    raise SystemExit("PATCH031_FRESHNESS_TIMEOUT_INVARIANTS=FAIL:" + ",".join(failed))
print("PATCH031_FRESHNESS_TIMEOUT_INVARIANTS=PASS")
