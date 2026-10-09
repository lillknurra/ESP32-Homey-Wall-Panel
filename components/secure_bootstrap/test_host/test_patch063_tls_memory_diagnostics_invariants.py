#!/usr/bin/env python3
"""Source guards for bounded, same-attempt Cloud TLS allocation diagnostics."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
CLIENT = (ROOT / "components/secure_bootstrap/athom_cloud_client.c").read_text()
HEADER = (ROOT / "components/secure_bootstrap/include/athom_cloud_client.h").read_text()
RUNTIME = (ROOT / "components/secure_bootstrap/athom_oauth_runtime.c").read_text()


def function(source: str, signature: str) -> str:
    scrubbed = re.sub(
        r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*',
        lambda match: " " * len(match[0]), source, flags=re.S)
    search_from = 0
    while True:
        start = source.index(signature, search_from)
        brace = scrubbed.index("{", start + len(signature))
        if ";" in scrubbed[start:brace]:
            search_from = start + len(signature)
            continue
        depth = 0
        for index in range(brace, len(source)):
            depth += (scrubbed[index] == "{") - (scrubbed[index] == "}")
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated function: {signature}")


request = function(CLIENT, "static esp_err_t http_request_limited(")
assert request.index("patch019a16e_begin_transport_alloc_capture();") < request.index(
    "esp_http_client_perform(ctx->handle)")
assert request.index("esp_http_client_perform(ctx->handle)") < request.index(
    "patch019a16e_finish_transport_alloc_capture();")
assert request.index("patch019a16e_finish_transport_alloc_capture();") < request.index(
    "last_tls_memory_diagnostic = s_patch019a16e_memory_diagnostic;")
assert "if (ctx->role == HTTP_ROLE_HOMEY_REMOTE) {\n        patch019a16e_begin" not in request
assert "tls_error == 141" in request

for field in (
    "capture_attempted", "hook_registered", "matching_failure_count",
    "internal_8bit_free_before", "internal_8bit_largest_before",
    "internal_8bit_minimum_before", "internal_8bit_free_at_failure",
    "internal_8bit_largest_at_failure", "internal_8bit_minimum_at_failure",
    "internal_8bit_free_after", "internal_8bit_largest_after",
    "internal_8bit_minimum_after",
):
    assert field in HEADER and field in RUNTIME

attempt_build = function(RUNTIME, "static athom_inventory_attempt_diagnostic_t athom_inventory_attempt_build(")
assert "diagnostic.raw_tls_memory_diagnostic = after->last_tls_memory_diagnostic;" in attempt_build
serializer = function(RUNTIME, "static bool athom_inventory_attempt_diagnostic_json(")
assert '\\"scope\\":\\"perform_window\\"' in serializer
assert '\\"matching_failure\\":' in serializer
assert '\\"internal_8bit\\"' in serializer
assert "device_id" not in serializer and "capability_id" not in serializer

status_get = function(RUNTIME, "static esp_err_t status_get(httpd_req_t *r)")
for forbidden in (
    "esp_http_client_perform", "queue_inventory", "connect_and_fetch",
    "runtime_diag_journal_record", "nvs_set_", "http_request(",
):
    assert forbidden not in status_get, forbidden

print("PATCH063_TLS_MEMORY_DIAGNOSTICS_INVARIANTS PASS")
