from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components" / "secure_bootstrap"
MODEL = (COMPONENT / "athom_cloud_model.c").read_text(encoding="utf-8")
RUNTIME = (COMPONENT / "athom_oauth_runtime.c").read_text(encoding="utf-8")
CLIENT = (COMPONENT / "athom_cloud_client.c").read_text(encoding="utf-8")
PORTAL = (COMPONENT / "phone_provisioning_store.c").read_text(encoding="utf-8")


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start + len(signature))
    depth = 0
    state = "code"
    index = brace
    while index < len(source):
        char = source[index]
        next_char = source[index + 1] if index + 1 < len(source) else ""

        if state == "string" or state == "char":
            if char == "\\":
                index += 2
                continue
            if (state == "string" and char == '"') or (state == "char" and char == "'"):
                state = "code"
        elif state == "line_comment":
            if char == "\n":
                state = "code"
        elif state == "block_comment":
            if char == "*" and next_char == "/":
                state = "code"
                index += 2
                continue
        elif char == '"':
            state = "string"
        elif char == "'":
            state = "char"
        elif char == "/" and next_char == "/":
            state = "line_comment"
            index += 2
            continue
        elif char == "/" and next_char == "*":
            state = "block_comment"
            index += 2
            continue
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
        index += 1
    raise AssertionError(f"unterminated function: {signature}")


serializer = function_body(MODEL, "bool athom_homey_status_json(")
assert "selected->id" not in serializer
assert "homeys->items[i].id" not in serializer
assert serializer.count('\\"name\\":') >= 2
assert '\\"id\\":' not in serializer

status_get = function_body(RUNTIME, "static esp_err_t status_get(httpd_req_t *r)")
assert "athom_homey_status_json(" in status_get
copy_call = "athom_inventory_attempt_diagnostic_copy(&diagnostics->inventory_attempt);"
serialize_call = "athom_inventory_attempt_diagnostic_json("
serialized_value = "diagnostics->inventory_attempt_json"
json_key = '\\"last_inventory_attempt_transport\\":%s}'
assert copy_call in status_get
assert serialize_call in status_get
assert serialized_value in status_get
assert "last_inventory_attempt_transport" in status_get
assert status_get.index(copy_call) < status_get.index(serialize_call)
assert re.search(
    r"athom_inventory_attempt_diagnostic_json\(\s*&diagnostics->inventory_attempt,\s*"
    r"diagnostics->inventory_attempt_json,\s*sizeof\(diagnostics->inventory_attempt_json\)\s*\)",
    status_get,
)
key_append_start = status_get.rfind("diag_written = snprintf(", 0, status_get.index(json_key))
key_append_end = status_get.index("if (diag_written", key_append_start)
key_append = status_get[key_append_start:key_append_end]
assert "body + body_length - 1U" in key_append
assert json_key in key_append
assert "diagnostics->inventory_attempt_json" in key_append
assert re.search(r"httpd_resp_sendstr\(\s*r\s*,\s*body\s*\)", status_get)
assert status_get.index(json_key) < status_get.index("httpd_resp_sendstr(r,body)")
assert re.search(r"if\s*\(!athom_inventory_attempt_diagnostic_json\([\s\S]*?\)\)\s*\{[\s\S]*?return ESP_ERR_INVALID_RESPONSE;", status_get)
print("LIVE_STATUS_RESPONSE_CHAIN_INVARIANT_PRESERVED=YES")
for forbidden in (
    "queue_inventory_refresh_if_ready",
    "connect_and_fetch_inventory",
    "network_phase_try_reserve",
    "esp_http_client_perform",
    "runtime_diag_journal_record",
    "nvs_set_",
    "athom_inventory_attempt_diagnostic_publish(",
):
    assert forbidden not in status_get, f"live-status must stay passive: {forbidden}"

assert RUNTIME.count('"/homey/live-status"') == 1
assert RUNTIME.count("athom_homey_status_json(") == 1

select_post = function_body(RUNTIME, "static esp_err_t select_post(httpd_req_t *r)")
select_worker = function_body(RUNTIME, "static void select_worker(void *arg)")
assert '"homey_id"' in select_post
assert "work->homey_id" in select_worker
assert "athom_cloud_select_and_connect(&s_cloud, selected_homey_id)" in RUNTIME
select = function_body(CLIENT, "esp_err_t athom_cloud_select_and_connect(")
assert "athom_homey_find_exact(&state->homeys, homey_id)" in select
assert "state->selected_homey = *selected" in select
assert '{"/homey/live-select",HTTP_POST,select_post,NULL}' in RUNTIME
assert '{"/homey/select",HTTP_GET,select_get,NULL}' in PORTAL
assert '{"/homey/select",HTTP_POST,select_post,NULL}' in PORTAL
assert 's_ctx.result.homeys[i].id' in PORTAL

# Diagnostic stage text is safe only while every producer remains a fixed literal.
stage_args = re.findall(
    r"\bdiagnostic_set(?:_http)?\s*\(\s*([^,]+),", CLIENT
)
producer_args = [arg.strip() for arg in stage_args if "*stage" not in arg]
assert producer_args, "expected diagnostic stage producers"
assert all(re.fullmatch(r'"[A-Za-z0-9_]+"', arg) for arg in producer_args), producer_args

print("PATCH061_LIVE_STATUS_PRIVACY_INVARIANTS PASS")
