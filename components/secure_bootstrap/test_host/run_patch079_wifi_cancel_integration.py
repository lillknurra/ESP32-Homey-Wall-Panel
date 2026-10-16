from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
secure = (ROOT / "components/secure_bootstrap/secure_bootstrap_esp.c").read_text()
ui = (ROOT / "components/secure_bootstrap/panel_ui.c").read_text()
header = (ROOT / "components/secure_bootstrap/include/secure_bootstrap.h").read_text()
ui_header = (ROOT / "components/secure_bootstrap/include/panel_ui.h").read_text()


def body(source: str, signature: str) -> str:
    start = source.index(signature)
    while True:
        brace = source.index("{", start + len(signature))
        semicolon = source.find(";", start + len(signature), brace)
        if semicolon < 0:
            break
        start = source.index(signature, semicolon + 1)
    depth = 0
    for i in range(brace, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise AssertionError(f"unterminated function: {signature}")


open_portal = body(secure, "static esp_err_t open_provisioning(void)")
assert open_portal.index("configure_access_point(s_code.code)") < open_portal.index("ensure_homey_server()")
assert open_portal.index("ensure_homey_server()") < open_portal.index("panel_wifi_present_session_locked()")
assert "panel_wifi_present_session_locked();\n    (void)xSemaphoreGive(s_ap_config_mutex);" in open_portal
assert '"WIFI:T:WPA;S:%s;P:%s;;"' in secure
assert "AP_SSID,\n                display_code" in secure

present_locked = body(secure, "static void panel_wifi_present_session_locked(void)")
assert "s_ap_config_mutex" not in present_locked
assert "s_code.code" in present_locked
present_wrapper = body(secure, "static void panel_wifi_present_session(void)")
assert "xSemaphoreTake(s_ap_config_mutex" in present_wrapper
assert "panel_wifi_present_session_locked()" in present_wrapper

handler = body(secure, "static esp_err_t wifi_handler(httpd_req_t *req)")
assert "request_phase != SECURE_BOOTSTRAP_SESSION_ACTIVE" in handler
assert "xSemaphoreTake(s_ap_config_mutex" in handler
assert "secure_bootstrap_wifi_session_is_live(&s_wifi_session, request_generation)" in handler
assert "secure_bootstrap_wifi_session_begin_candidate(&s_wifi_session, request_generation)" in handler
assert handler.index("secure_bootstrap_wifi_session_begin_candidate") < handler.index("candidate_disconnect_and_drain_events(request_generation)")
assert handler.index("candidate_disconnect_and_drain_events(request_generation)") < handler.index("esp_wifi_set_config(WIFI_IF_STA, &s_candidate_config)")
assert handler.index("esp_wifi_set_config(WIFI_IF_STA, &s_candidate_config)") < handler.index("secure_bootstrap_wifi_candidate_arm_connect(")

got_ip = body(secure, "static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)")
assert "secure_bootstrap_wifi_candidate_got_ip_verified" in got_ip
assert got_ip.index("wifi_got_ip_matches_candidate(got_ip)") < got_ip.index("secure_bootstrap_wifi_candidate_got_ip_verified")
candidate_gate = got_ip.index("secure_bootstrap_wifi_candidate_got_ip_verified")
assert candidate_gate < got_ip.index("if (!accepted)", candidate_gate)
assert got_ip.index("if (!accepted)", candidate_gate) < got_ip.index(
    "s_ip_obtained = secure_bootstrap_sta_link_apply_event", candidate_gate)
assert "SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED" in got_ip

commit_worker = body(secure, "static void persistent_commit_task(void *arg)")
assert commit_worker.index("secure_bootstrap_wifi_session_commit_started") < commit_worker.index("backup_write_saved()")
assert "s_persist_session_generation" in commit_worker
schedule_commit = body(secure, "static esp_err_t schedule_persistent_commit(void)")
assert "s_persist_task_scheduled || s_persist_active" in schedule_commit
assert "secure_bootstrap_wifi_session_commit_start_failed" in schedule_commit
assert "SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_TASK_CREATE_FAILED" in schedule_commit
assert schedule_commit.index("commit_start_failed") < schedule_commit.index("PERSIST_TASK_CREATE_FAILED")

cancel_worker = body(secure, "static void wifi_cancel_task(void *arg)")
assert cancel_worker.index("httpd_stop(s_server)") < cancel_worker.index("restore_saved_ram()")
assert "nvs_" not in cancel_worker
assert "secure_bootstrap_wifi_session_finish_cancel" in cancel_worker
assert "generate_code()" in cancel_worker
assert "portal_stop_failed = true" in cancel_worker
assert "bool recovered = !portal_stop_failed" in cancel_worker
failed_stop = cancel_worker[cancel_worker.index("if (portal_stop_failed)"):cancel_worker.index("/* Restore a usable portal")]
assert "s_wifi_session.phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED" in failed_stop
assert "s_wifi_session.generation == generation" in failed_stop
assert "secure_bootstrap_wifi_session_finish_cancel" in failed_stop
assert "s_wifi.state = SECURE_BOOTSTRAP_WIFI_PROVISIONING" in failed_stop
assert "s_wifi.candidate_active = false" in failed_stop
assert "s_cancel_task_scheduled = false" in failed_stop
assert "httpd_stop" not in failed_stop[failed_stop.index("if (resumed)"):]
assert "panel_wifi_result_update(PANEL_UI_WIFI_CANCEL_FAILED)" in failed_stop

display_text = body(secure, "static void set_display_text(")
assert "preserve_wifi_session" in display_text
assert "display_code != NULL || !preserve_wifi_session" in display_text
assert "phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED" in display_text
assert "phase != SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED" in display_text

wifi_publish = body(ui, "bool panel_ui_update_wifi_provisioning(")
assert '"WIFI:T:WPA;S:%s;P:%s;;"' in wifi_publish
assert "PANEL_UI_QR_UPDATE(qr, qr_payload" in wifi_publish
assert "#define PANEL_UI_QR_UPDATE(qr, payload, length) lv_qrcode_update" in ui
assert "clear_ui_secret(qr_payload" in wifi_publish

assert "SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED" in header
assert "SECURE_BOOTSTRAP_SESSION_COMMIT_RESERVED" in header
assert "request_wifi_cancel" in ui_header
assert "lv_indev_reset(input, NULL)" in ui
assert "lv_indev_wait_release(input)" in ui
assert "ui->wifi_cancel_allowed" in ui
assert "lv_obj_add_flag(ui->status_overlay, LV_OBJ_FLAG_CLICKABLE)" in ui
assert "panel_ui_clear_wifi_provisioning" in ui

print("PASS: Patch079 production integration invariants")
