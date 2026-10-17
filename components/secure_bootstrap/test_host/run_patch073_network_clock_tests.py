#!/usr/bin/env python3
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
test_dir = Path(__file__).resolve().parent
secure_source = (root / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
clock_source = (root / "panel_clock.c").read_text(encoding="utf-8")

with tempfile.TemporaryDirectory(prefix="patch073-clock-") as temp_dir:
    binary = Path(temp_dir) / "test_panel_clock"
    command = [
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-D_POSIX_C_SOURCE=200809L", "-I", str(root / "include"),
        str(root / "panel_clock.c"),
        str(root / "panel_ui_model.c"),
        str(root / "panel_homey_dashboard_binding.c"),
        str(root / "panel_roller_model.c"),
        str(root / "panel_ui_store.c"),
        str(test_dir / "test_panel_clock.c"), "-lm", "-o", str(binary),
    ]
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True)

# Source invariants for the ESP-IDF integration. No network calls are made here.
assert secure_source.count("esp_netif_sntp_init(&config)") == 1
assert "config.wait_for_sync = false;" in secure_source
assert "config.sync_cb = sntp_time_sync_callback;" in secure_source
assert "esp_netif_sntp_sync_wait" not in secure_source
assert "if (s_sntp_init_attempted) {\n        return;\n    }" in secure_source
assert secure_source.count("start_network_time_sync_once();") == 1
got_ip = secure_source.index("if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)")
got_ip_received = secure_source.index("SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP", got_ip)
start_request = secure_source.index("request_sntp_start();", got_ip_received)
assert got_ip < got_ip_received < start_request
transition = secure_source.index("secure_bootstrap_wifi_transition(", start_request)
apply = secure_source.index("apply_actions(actions)", transition)
assert start_request < transition < apply
sync_start = secure_source.index("static void start_network_time_sync_once(void)")
sync_end = secure_source.index("\nstatic ", sync_start + 1)
sync_function = secure_source[sync_start:sync_end]
assert "ESP_ERROR_CHECK" not in sync_function
assert "s_wifi.state" not in sync_function
assert "athom_" not in sync_function
callback_start = secure_source.index("static void sntp_time_sync_callback(")
callback_end = secure_source.index("\nstatic ", callback_start + 1)
callback = secure_source[callback_start:callback_end]
assert "lv_" not in callback and "panel_ui_" not in callback
assert "panel_clock_configure_timezone()" in secure_source
assert "panel_ui_set_time(s_panel_ui, NULL, false)" not in secure_source
assert "update_panel_clock_if_due();" in secure_source
rotation = secure_source[secure_source.index("static void rotation_task(void *arg)"):]
assert "wifi_state_snapshot() == SECURE_BOOTSTRAP_WIFI_ONLINE &&\n            take_sntp_start_request()" in rotation
assert "s_wifi.state" not in rotation.split("static esp_err_t wipe_homey_account_state", 1)[0]
assert rotation.index("take_sntp_start_request()") < rotation.index("start_network_time_sync_once();")
wifi_start = secure_source.index("static void wifi_event(")
wifi_end = secure_source.index("\nstatic bool dashboard_snapshot_poll_allowed", wifi_start)
assert "start_network_time_sync_once();" not in secure_source[wifi_start:wifi_end]
assert rotation.index("panel_display_lock_traced(50, false, &lock_acquired_us)") < rotation.index("update_panel_clock_if_due();")
clock_update_start = secure_source.index("static void update_panel_clock_if_due(void)")
clock_update_end = secure_source.index("\nstatic ", clock_update_start + 1)
clock_update = secure_source[clock_update_start:clock_update_end]
assert "panel_clock_render_is_due(" in clock_update
assert clock_update.index("panel_clock_render_is_due(") < clock_update.index("panel_ui_set_time(")
assert "ESP_ERROR_CHECK(result)" not in secure_source
assert "s_sntp_init_attempted = true;" in secure_source
assert "s_sntp_time_received = true;" in secure_source
assert "portENTER_CRITICAL(&s_sntp_time_mux);" in secure_source
assert "SNTP" in clock_source or "panel_clock" in clock_source
assert not re.search(r"\b(socket|connect|send|recv)\s*\(", clock_source)
print("PATCH073_NETWORK_CLOCK_SOURCE_INVARIANTS PASS")
