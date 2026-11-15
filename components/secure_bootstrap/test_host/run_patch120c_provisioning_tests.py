#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components/secure_bootstrap"
SOURCE = (COMPONENT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
QR = (COMPONENT / "panel_wifi_qr.c").read_text(encoding="utf-8")
QR_HEADER = (COMPONENT / "include/panel_wifi_qr.h").read_text(encoding="utf-8")

def body(source: str, signature: str) -> str:
    start = source.index(signature)
    while True:
        brace = source.index("{", start + len(signature))
        semicolon = source.find(";", start + len(signature), brace)
        if semicolon < 0:
            break
        start = source.index(signature, semicolon + 1)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated function: {signature}")

handler = body(SOURCE, "static esp_err_t wifi_handler(httpd_req_t *req)")
assert "panel_wifi_http_read_complete_body(" in handler
assert handler.index("panel_wifi_http_read_complete_body(") < handler.index("form_value(body")
assert handler.index("panel_wifi_http_read_complete_body(") < handler.index("secure_bootstrap_code_verify_and_consume")
assert handler.index("panel_wifi_http_read_complete_body(") < handler.index("secure_bootstrap_wifi_transition")
assert "httpd_req_recv(req, body, req->content_len)" not in handler
assert "cfg.recv_wait_timeout" not in SOURCE
assert "HTTPD_DEFAULT_CONFIG()" in SOURCE

open_portal = body(SOURCE, "static esp_err_t open_provisioning(void)")
assert "startup_logo_dwell_cancel();" in open_portal
assert "set_display_text(" in open_portal
assert open_portal.index("secure_bootstrap_wifi_session_activate") < open_portal.index("set_display_text(")
assert SOURCE.count("s_provisioning_screen, s_startup_logo, s_title, s_detail") == 2

startup_writer = body(SOURCE, "static bool set_startup_display_text_now(")
assert "panel_wifi_qr_startup_status_owned(" in startup_writer
assert startup_writer.index("if (provisioning_view_active)") < startup_writer.index("lv_label_set_text(s_detail, visible_status_text)")
assert "return layout_ok;" in startup_writer

normal_writer = body(SOURCE, "static void set_display_text(")
assert "if (active_wifi_session && (display_code == NULL || display_code[0] == '\\0'))" in normal_writer
assert "panel_wifi_qr_layout_startup(" in normal_writer
assert "lv_obj_t *startup_logo" in QR_HEADER
assert "lv_obj_set_hidden(startup_logo, true)" in QR

with tempfile.TemporaryDirectory(prefix="patch120c-http-body-") as temp:
    binary = Path(temp) / "test_http_body"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(COMPONENT / "include"),
        str(COMPONENT / "panel_wifi_http_body.c"),
        str(COMPONENT / "test_host/test_patch120c_http_body.c"),
        "-o", str(binary),
    ], check=True)
    result = subprocess.run([str(binary)], check=True, text=True, capture_output=True)
    print(result.stdout.strip())

print("PASS: Patch120C production provisioning integration guards")
