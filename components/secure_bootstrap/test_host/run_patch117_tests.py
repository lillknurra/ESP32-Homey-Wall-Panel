#!/usr/bin/env python3
"""Patch117 offline policy and real-LVGL provisioning ownership tests."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
component = root / "components/secure_bootstrap"
build = Path(tempfile.mkdtemp(prefix="patch117-host-"))
binary = build / "boot_recovery_policy"
subprocess.run([
    "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
    "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
    "-I", str(component / "include"),
    str(component / "panel_homey_boot_recovery.c"),
    str(component / "test_host/test_patch117_boot_recovery_policy.c"),
    "-o", str(binary),
], check=True)
subprocess.run([str(binary)], check=True)

wifi_binary = build / "wifi_state_machine"
subprocess.run([
    "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
    "-I", str(component / "include"),
    str(component / "secure_bootstrap_logic.c"),
    str(component / "test_host/test_wifi_state_machine.c"),
    "-o", str(wifi_binary),
], check=True)
subprocess.run([str(wifi_binary)], check=True)

startup_binary = build / "startup_status"
subprocess.run([
    "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
    "-I", str(component / "include"),
    str(component / "panel_startup_status.c"),
    str(component / "test_host/test_patch105_startup_status.c"),
    "-o", str(startup_binary),
], check=True)
subprocess.run([str(startup_binary)], check=True)

deferred_binary = build / "wifi_deferred_owner"
subprocess.run([
    "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
    "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
    "-I", str(component / "include"),
    str(component / "panel_wifi_deferred.c"),
    str(component / "test_host/test_panel_wifi_deferred.c"),
    "-o", str(deferred_binary),
], check=True)
subprocess.run([str(deferred_binary)], check=True)

subprocess.run([
    "python3", str(component / "test_host/run_patch076_v37_stability_tests.py"),
    "--output-dir", str(build / "lvgl"),
], check=True)

# Integration assertions supplement the compiled policy and real LVGL tests:
# these verify the production event/task wiring and fail-closed READY gate.
runtime = (component / "athom_oauth_runtime.c").read_text()
secure = (component / "secure_bootstrap_esp.c").read_text()
assert 'maybe_start_boot_auto_refresh_scheduler("wifi_online")' in runtime
assert 'maybe_start_boot_auto_refresh_scheduler("auth_restore_complete")' in runtime
assert "PANEL_HOMEY_BOOT_RECOVERY_WAIT_ATTEMPTS" in runtime
assert "PANEL_HOMEY_BOOT_RECOVERY_WAIT_MS" in runtime
boot_scheduler = runtime[runtime.index("static void boot_auto_refresh_scheduler(void *arg)"):runtime.index("static esp_err_t schema_refresh_get(")]
assert "attempt <= ATHOM_BOOT_AUTO_READY_WAIT_ATTEMPTS" in boot_scheduler
assert "vTaskDelay(pdMS_TO_TICKS(ATHOM_BOOT_AUTO_READY_WAIT_MS))" in boot_scheduler
assert "set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);" in runtime[
    runtime.index('"HOMEY_BOOT_AUTO_REFRESH phase=end result=timeout"') - 900:
    runtime.index('"HOMEY_BOOT_AUTO_REFRESH phase=end result=timeout"')]
assert "panel_homey_boot_recovery_try_begin" in runtime
assert "s_refresh_job_reserved" in runtime
for marker in (
    "static void periodic_inventory_refresh_scheduler(void *arg)",
    "static void boot_auto_refresh_scheduler(void *arg)",
    "static void maybe_start_boot_auto_refresh_scheduler(const char *cause)\n{",
):
    start = runtime.index(marker)
    end = runtime.find("\nstatic ", start + len(marker))
    body = runtime[start:end if end >= 0 else None]
    assert "panel_homey_snapshot_inspection_t" not in body
    assert "athom_cloud_inspect_device_snapshot_freshness" in body
assert "state == ATHOM_HOMEY_DATA_READY && snapshot_fresh" in secure
assert "wifi_phase != SECURE_BOOTSTRAP_SESSION_CLOSED" in secure
event_handler = secure[secure.index("static void wifi_event(void *arg"):secure.index("static bool dashboard_snapshot_poll_allowed")]
assert "consume_network_scan_results();" not in event_handler
assert "open_provisioning();" not in event_handler
assert "apply_wifi_event_actions(actions)" in event_handler
assert "queue_deferred_provisioning();" in secure[
    secure.index("static esp_err_t apply_wifi_event_actions(uint32_t actions)"):
    secure.index("static void reconfigure_task", secure.index(
        "static esp_err_t apply_wifi_event_actions(uint32_t actions)"))]
assert "queue_scan_results_from_event();" in event_handler
assert 'xQueueCreate(16, sizeof(deferred_provisioning_request_t))' in secure
assert '"wifi_provision_deferred", 6144' in secure
assert "SECURE_BOOTSTRAP_WIFI_PROVISIONING_PENDING" in secure
assert "request.generation == s_wifi_event_epoch" in secure
assert "panel_wifi_deferred_begin" in secure
assert "panel_wifi_deferred_still_current" in secure
assert "panel_wifi_deferred_invalidate(&s_deferred_provisioning_owner)" in event_handler
assert "apply_actions(" not in event_handler
print("PATCH117_OFFLINE_INTEGRATION_CHECKS PASS")
