#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
RUNTIME = ROOT / "components/secure_bootstrap/athom_oauth_runtime.c"
PHONE = ROOT / "components/secure_bootstrap/phone_provisioning_store.c"
HEADER = ROOT / "components/secure_bootstrap/include/phone_provisioning.h"

runtime = RUNTIME.read_text(encoding="utf-8")
phone = PHONE.read_text(encoding="utf-8")
header = HEADER.read_text(encoding="utf-8")


def function_body(text: str, signature: str) -> str:
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

queue = function_body(runtime, "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready(bool boot_auto)")
worker = function_body(runtime, "static void homey_command_worker(void *arg)")
dispatch = function_body(runtime, "athom_light_toggle_dispatch_result_t athom_oauth_runtime_dispatch_light_toggle(")
light_queue = function_body(runtime, "athom_light_toggle_queue_result_t athom_oauth_runtime_queue_light_toggle(")
awning_post = function_body(phone, "static esp_err_t awning_bindings_post(httpd_req_t *request)")
light_get = function_body(phone, "static esp_err_t light_bindings_get(httpd_req_t *r)")
phone_online = function_body(phone, "void phone_provisioning_on_wifi_online(void)")
phone_offline = function_body(phone, "void phone_provisioning_on_wifi_offline(void)")
phone_wifi_getter = function_body(phone, "bool phone_provisioning_wifi_online(void)")

# Patch052 core: the read-only recovery queue must use transport prerequisites,
# not strict command/provisioning readiness, otherwise strict readiness cannot
# be recovered by a fresh authoritative inventory.
assert "phone_provisioning_wifi_online()" in queue
assert "phone_provisioning_homey_runtime_ready()" not in queue
assert "s_cloud.selected_homey.id[0] == 0" in queue
assert "s_cloud.homey_session_token[0] == 0" in queue
assert "s_homey_command_queue == NULL" in queue
assert "network_phase_try_reserve(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH)" in queue
assert "xQueueSend(s_homey_command_queue" in queue

# The Wi-Fi prerequisite is the same phone-provisioning state exposed by
# /homey/status; it must preserve explicit online/offline updates.
assert "return s_wifi_online;" in phone_wifi_getter
assert "s_wifi_online=true;" in phone_online
assert "s_wifi_online=false;" in phone_offline
assert "bool phone_provisioning_wifi_online(void);" in header

# Recovery remains authoritative: only a verified inventory republishes strict
# live readiness; failure publishes an error state instead.
assert "homey_inventory_result_verified(" in worker
assert "s_homey_data_state = ATHOM_HOMEY_DATA_READY;" in worker
assert "phone_provisioning_show_live_ready(s_cloud.selected_homey.name);" in worker
assert "s_homey_data_state = ATHOM_HOMEY_DATA_ERROR;" in worker

# Patch052 must not weaken any write/provisioning gate.
assert "phone_provisioning_homey_runtime_ready()" in dispatch
assert "phone_provisioning_homey_runtime_ready()" in light_queue
assert "phone_provisioning_homey_runtime_ready()" in awning_post
assert "phone_provisioning_homey_runtime_ready()" in light_get

print("PATCH052_READ_ONLY_REFRESH_RECOVERY_INVARIANTS=PASS")
