#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
SECURE = ROOT / "components/secure_bootstrap/secure_bootstrap_esp.c"
STORE = ROOT / "components/secure_bootstrap/phone_provisioning_store.c"
RUNTIME = ROOT / "components/secure_bootstrap/athom_oauth_runtime.c"
LOGIC = ROOT / "components/secure_bootstrap/secure_bootstrap_logic.c"
INCLUDE = ROOT / "components/secure_bootstrap/include"
TEMPLATE = ROOT / "components/secure_bootstrap/test_host/test_patch059_periodic_refresh.c"
V52D_BASELINE = Path.home() / "Downloads/PATCH076_V52D_OFFLINE_8gby5mi_/source"


def function_body(source: str, signature: str) -> str:
    search_from = 0
    start = -1
    brace = -1
    while True:
        candidate = source.find(signature, search_from)
        assert candidate >= 0, signature
        next_brace = source.find("{", candidate + len(signature))
        next_semicolon = source.find(";", candidate + len(signature))
        if next_brace >= 0 and (next_semicolon < 0 or next_brace < next_semicolon):
            start, brace = candidate, next_brace
            break
        search_from = candidate + len(signature)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated: {signature}")


secure = SECURE.read_text(encoding="utf-8")
store = STORE.read_text(encoding="utf-8")
runtime = RUNTIME.read_text(encoding="utf-8")
logic = LOGIC.read_text(encoding="utf-8")
baseline_secure = (V52D_BASELINE / "components/secure_bootstrap/secure_bootstrap_esp.c").read_text(encoding="utf-8")
baseline_opening = function_body(baseline_secure, "static esp_err_t open_provisioning(void)")
assert "phone_provisioning_on_wifi_offline();" in baseline_opening

# Production integration invariants: AP startup must preserve STA state, and
# STA events must update link readiness even while the UI state is PROVISIONING.
opening = function_body(secure, "static esp_err_t open_provisioning(void)")
assert "phone_provisioning_on_wifi_offline" not in opening
assert "WIFI_MODE_APSTA" in opening
wifi_event = function_body(secure, "static void wifi_event(")
provisioning_got_ip = wifi_event.index("observed_state == SECURE_BOOTSTRAP_WIFI_PROVISIONING")
ignored_got_ip = wifi_event.index('"WIFI_GOT_IP ignored=true state=%u"')
assert provisioning_got_ip < ignored_got_ip
assert "phone_provisioning_on_wifi_online();" in wifi_event[provisioning_got_ip:ignored_got_ip]
disconnected = wifi_event.index("if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)")
assert "phone_provisioning_on_wifi_offline();" in wifi_event[disconnected:]
assert "secure_bootstrap_sta_link_apply_event" in wifi_event

online = function_body(store, "void phone_provisioning_on_wifi_online(void)")
offline = function_body(store, "void phone_provisioning_on_wifi_offline(void)")
getter = function_body(store, "bool phone_provisioning_wifi_online(void)")
ready = function_body(store, "bool phone_provisioning_homey_runtime_ready(void)")
assert "s_wifi_online=true" in online and "s_wifi_online=false" in offline
assert "athom_oauth_runtime_on_wifi_offline" in offline
assert "wifi_link_is_online" in getter and "wifi_link_is_online" in ready
assert "portENTER_CRITICAL(&s_wifi_online_mux)" in store
assert "portEXIT_CRITICAL(&s_wifi_online_mux)" in store

runtime_offline = function_body(runtime, "void athom_oauth_runtime_on_wifi_offline(void)")
assert "s_wifi_online = false" in runtime_offline
assert "phone_provisioning_wifi_online()" in function_body(
    runtime, "static void patch031_diag_cloud_probe_worker(void *arg)"
)
oauth_worker = function_body(runtime, "static void oauth_worker(void *arg)")
assert oauth_worker.count("phone_provisioning_wifi_online()") >= 2
refresh_post = function_body(runtime, "static esp_err_t refresh_post(httpd_req_t *r)")
assert refresh_post.count("phone_provisioning_wifi_online()") >= 2
assert "phone_provisioning_wifi_online()" in function_body(
    runtime, "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready("
)
assert "phone_provisioning_wifi_online()" in function_body(
    runtime, "static esp_err_t connect_and_fetch_inventory(const char *homey_id)"
)
assert "phone_provisioning_wifi_online()" in function_body(
    runtime, "static esp_err_t fetch_inventory_from_cached_session(const char *homey_id)"
)
dashboard_show = function_body(secure, "static bool panel_show_dashboard(void)")
assert "s_homey_dashboard_state.generation_valid" in dashboard_show
assert "!s_homey_dashboard_state.stale" in dashboard_show
assert "athom_oauth_runtime_homey_data_state() == ATHOM_HOMEY_DATA_READY" in dashboard_show
assert ".state = connection_state" in dashboard_show
dashboard_poll = function_body(secure, "static void poll_homey_dashboard_if_due(")
assert "runtime_state == ATHOM_HOMEY_DATA_READY && snapshot_fresh" in dashboard_poll
assert "model_changed || connection_changed || readiness_changed" in dashboard_poll

# Compile actual scheduler/queue functions from production with actual Wi-Fi
# link and bootstrap state transitions. The queue uses a deterministic fake.
template = TEMPLATE.read_text(encoding="utf-8")
marker = "/* PATCH059_PRODUCTION_FUNCTIONS */"
assert template.count(marker) == 1
production = "\n\n".join(
    function_body(runtime, signature)
    for signature in (
        "static bool inventory_refresh_worker_should_retry(",
        "static bool inventory_refresh_worker_should_retry_after_cloud_429(",
        "static uint32_t homey_data_retry_delay_ms(",
        "static uint32_t homey_data_retry_delay_ms_for_failure(",
        "static bool periodic_refresh_scheduler_should_attempt(",
        "static uint64_t periodic_refresh_scheduler_defer_ms(",
        "static void periodic_refresh_scheduler_record_queue_result(",
        "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready(",
    )
)
template = template.replace(marker, production)
template = template.replace(
    "#include <assert.h>",
    '#include "secure_bootstrap.h"\n#include <assert.h>',
    1,
)
scenario = r'''
static void test_apsta_preserves_periodic_refresh_and_real_disconnect_gates_it(void)
{
    /* Reproduce the exact old V52D state effect and prove the production queue gate rejects it. */
    reset_fixture();
    s_wifi_online = false; /* old open_provisioning() called on_wifi_offline() */
    assert(queue_inventory_refresh_if_ready(ATHOM_REFRESH_ORIGIN_PERIODIC) ==
           ATHOM_REFRESH_QUEUE_NOT_READY);
    assert(s_fake_queue.sent == 0U);

    reset_fixture();
    secure_bootstrap_wifi_context_t wifi;
    secure_bootstrap_wifi_context_init(&wifi);
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP) ==
        (SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING |
         SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE));
    bool sta_online = secure_bootstrap_sta_link_apply_event(
        false, SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP);
    assert(sta_online);
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING);
    assert(wifi.state == SECURE_BOOTSTRAP_WIFI_PROVISIONING);

    /* APSTA mode itself emits no link-loss event, so the known STA link stays up. */
    s_wifi_online = sta_online;
    periodic_refresh_scheduler_state_t scheduler = {0};
    assert(periodic_refresh_scheduler_should_attempt(
        &scheduler, true, 1000U, PERIODIC_REFRESH_INTERVAL_MS, 61000U));
    assert(queue_inventory_refresh_if_ready(ATHOM_REFRESH_ORIGIN_PERIODIC) ==
           ATHOM_REFRESH_QUEUE_OK);
    assert(s_fake_queue.sent == 1U);
    assert(s_fake_queue.last.origin == ATHOM_REFRESH_ORIGIN_PERIODIC);

    reset_fixture();
    s_wifi_online = secure_bootstrap_sta_link_apply_event(
        true, SECURE_BOOTSTRAP_STA_LINK_EVENT_DISCONNECTED);
    assert(!s_wifi_online);
    assert(queue_inventory_refresh_if_ready(ATHOM_REFRESH_ORIGIN_PERIODIC) ==
           ATHOM_REFRESH_QUEUE_NOT_READY);
    assert(s_fake_queue.sent == 0U);

    /* A GOT_IP received while the portal remains active restores the gate. */
    s_wifi_online = secure_bootstrap_sta_link_apply_event(
        s_wifi_online, SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP);
    assert(s_wifi_online);
    assert(queue_inventory_refresh_if_ready(ATHOM_REFRESH_ORIGIN_PERIODIC) ==
           ATHOM_REFRESH_QUEUE_OK);
    assert(s_fake_queue.sent == 1U);
}
'''
template = template.replace("int main(void)\n{", scenario + "\nint main(void)\n{")
template = template.replace(
    "    test_queue_serialization_and_periodic_origin();\n    return 0;",
    "    test_queue_serialization_and_periodic_origin();\n"
    "    test_apsta_preserves_periodic_refresh_and_real_disconnect_gates_it();\n"
    "    return 0;",
)
with tempfile.TemporaryDirectory(prefix="patch076-wifi-refresh-") as td:
    td = Path(td)
    generated = td / "test_wifi_refresh.c"
    binary = td / "test_wifi_refresh"
    generated.write_text(template, encoding="utf-8")
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(INCLUDE), str(LOGIC), str(generated), "-o", str(binary)
    ], check=True, cwd=ROOT)
    subprocess.run([str(binary)], check=True, cwd=ROOT)

print("PATCH076_WIFI_STA_REFRESH_TESTS=PASS")
