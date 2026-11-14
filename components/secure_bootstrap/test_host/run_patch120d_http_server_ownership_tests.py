#!/usr/bin/env python3
"""Exercise the production HTTP server ownership helper with bounded mocks."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
SOURCE = (ROOT / "components/secure_bootstrap/secure_bootstrap_esp.c").read_text()


def function_body(source: str, signature: str) -> str:
    search = 0
    while True:
        start = source.index(signature, search)
        brace = source.find("{", start + len(signature))
        semicolon = source.find(";", start + len(signature))
        if brace >= 0 and (semicolon < 0 or brace < semicolon):
            break
        search = start + len(signature)
    depth = 0
    for end in range(brace, len(source)):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
            if depth == 0:
                return source[start:end + 1]
    raise AssertionError(f"unterminated function: {signature}")


helper = function_body(SOURCE, "static esp_err_t stop_homey_server_owned(void)")
ensure = function_body(SOURCE, "static esp_err_t ensure_homey_server(void)")
worker = function_body(SOURCE, "static void persistent_commit_task(void *arg)")
close_portal = function_body(SOURCE, "static void close_provisioning(void)")
assert worker.index("stop_homey_server_owned()") < worker.index("panel_wifi_txn_begin_candidate(")
stop_failure = worker[worker.index("if (server_stop != ESP_OK)"):worker.index("panel_wifi_txn_ops_t ops")]
assert "return;" in stop_failure and "panel_wifi_txn_begin_candidate" not in stop_failure
assert "apply_actions(" not in stop_failure
assert 'portal_state=unknown' in stop_failure
assert "set_reconfigure_button_visible(false)" in stop_failure
assert ensure.index("s_server_health_unknown") < ensure.index("server_start()")
assert "if (stop_err != ESP_OK)" in ensure
assert close_portal.index("stop_homey_server_owned()") < close_portal.index("esp_wifi_set_mode(WIFI_MODE_STA)")
assert "secure_bootstrap_code_wipe(&s_code)" in close_portal
assert "set_reconfigure_button_visible(true)" not in stop_failure
assert "s_server_health_unknown = true" in helper

fixture = r'''#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int esp_err_t;
typedef void *httpd_handle_t;
typedef struct { int flags; char text[80]; } lv_obj_t;
typedef struct { char code[16]; bool consumed; } secure_bootstrap_code_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define TAG "host"
#define WIFI_MODE_STA 1
#define LV_OBJ_FLAG_HIDDEN 1
#define ESP_LOGE(...) ((void)esp_err_to_name(ESP_FAIL))
#define ESP_LOGW(...) ((void)esp_err_to_name(ESP_FAIL))
static httpd_handle_t s_server;
static bool s_homey_handlers_registered;
static bool s_server_health_unknown;
static int s_stop_result;
static int s_stop_calls;
static int s_start_result;
static int s_start_calls;
static int s_report_calls;
static int s_register_result;
static int s_register_calls;
static int s_wifi_mode;
static int64_t s_last_code_countdown_s;
static void *s_panel_ui = (void *)0x1111;
static lv_obj_t qr, frame, caption, code_label;
static lv_obj_t *s_qr = &qr, *s_qr_frame = &frame;
static lv_obj_t *s_code_caption = &caption, *s_code_label = &code_label;
static secure_bootstrap_code_t s_code;
static const char *esp_err_to_name(int e) { return e == ESP_OK ? "ESP_OK" : "ESP_FAIL"; }
static int httpd_stop(httpd_handle_t h) { assert(h == s_server); s_stop_calls++; return s_stop_result; }
static int server_start(void) { s_start_calls++; if (s_start_result == ESP_OK) s_server = (void *)0x5678; return s_start_result; }
static void athom_oauth_runtime_report_bootstrap_failure(int e) { (void)e; s_report_calls++; }
static void athom_oauth_runtime_clear_bootstrap_failure(void) {}
static int athom_oauth_runtime_bootstrap_error_code(void) { return 0; }
static void panel_provisioning_display_update(const char *title, const char *detail, const char *code) { (void)title; (void)detail; (void)code; }
static void phone_provisioning_set_display_callback(void (*callback)(const char *, const char *, const char *)) { (void)callback; }
static void phone_provisioning_set_portal_css(const char *css) { (void)css; }
static const char *portal_shared_css(void) { return ""; }
static int phone_provisioning_register_handlers(httpd_handle_t h) { assert(h == s_server); s_register_calls++; return s_register_result; }
static bool bsp_display_lock(int timeout) { (void)timeout; return true; }
static void bsp_display_unlock(void) {}
static int panel_ui_clear_wifi_provisioning(void *ui, lv_obj_t *obj) { (void)ui; (void)obj; return 0; }
static int lv_qrcode_update(lv_obj_t *obj, const char *data, size_t size) { (void)obj; (void)data; (void)size; return 0; }
static void lv_obj_add_flag(lv_obj_t *obj, int flag) { obj->flags |= flag; }
static void lv_label_set_text(lv_obj_t *obj, const char *text) { snprintf(obj->text, sizeof(obj->text), "%s", text); }
static int esp_wifi_set_mode(int mode) { s_wifi_mode = mode; return ESP_OK; }
static void secure_bootstrap_code_wipe(secure_bootstrap_code_t *code) { memset(code, 0, sizeof(*code)); }
HELPER
ENSURE
PORTAL
int main(void) {
    s_server = (void *)0x1234; s_homey_handlers_registered = true;
    s_stop_result = ESP_FAIL;
    assert(stop_homey_server_owned() == ESP_FAIL);
    assert(s_server == (void *)0x1234 && s_homey_handlers_registered && s_stop_calls == 1);
    assert(s_server_health_unknown);
    /* Whether a mocked retained server would answer requests is deliberately
     * irrelevant: health is unverified, so ensure must not reuse the handle. */
    s_start_result = ESP_OK;
    assert(ensure_homey_server() == ESP_FAIL);
    assert(s_server == (void *)0x1234 && s_start_calls == 0 && s_report_calls == 1);
    s_stop_result = ESP_OK;
    assert(ensure_homey_server() == ESP_OK);
    assert(s_server == (void *)0x5678 && s_homey_handlers_registered);
    assert(!s_server_health_unknown && s_homey_handlers_registered);
    assert(s_start_calls == 1 && s_stop_calls == 3 && s_register_calls == 1);
    assert(stop_homey_server_owned() == ESP_OK);
    assert(s_server == NULL && !s_homey_handlers_registered && s_stop_calls == 4);
    /* Stop success followed by server-start failure must be returned, not
     * mistaken for a reopened portal. */
    s_server = (void *)0x9999; s_homey_handlers_registered = true;
    s_server_health_unknown = true; s_stop_result = ESP_OK; s_start_result = ESP_FAIL;
    assert(ensure_homey_server() == ESP_FAIL);
    assert(s_server == NULL && !s_homey_handlers_registered && s_start_calls == 2 &&
           s_stop_calls == 5);
    /* close_provisioning preserves ownership, takes AP mode down and clears
     * visible one-time setup credentials for both stop outcomes. */
    s_server = (void *)0x2222; s_homey_handlers_registered = true;
    s_server_health_unknown = false; s_stop_result = ESP_FAIL;
    strcpy(s_code.code, "SYNTHETIC");
    close_provisioning();
    assert(s_server == (void *)0x2222 && s_server_health_unknown);
    assert(s_wifi_mode == WIFI_MODE_STA && (frame.flags & LV_OBJ_FLAG_HIDDEN));
    assert((qr.flags & LV_OBJ_FLAG_HIDDEN) && caption.text[0] == '\0' &&
           code_label.text[0] == '\0' && s_code.code[0] == '\0');
    s_stop_result = ESP_OK;
    close_provisioning();
    assert(s_server == NULL && !s_homey_handlers_registered && !s_server_health_unknown);
    return 0;
}'''.replace("HELPER", helper).replace("ENSURE", ensure).replace("PORTAL", close_portal)

with tempfile.TemporaryDirectory(prefix="patch120d-http-ownership-") as temp:
    c_file = Path(temp) / "test.c"
    binary = Path(temp) / "test"
    c_file.write_text(fixture)
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", str(c_file), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

print("PATCH120D_HTTP_SERVER_OWNERSHIP_PASS (production helper with bounded API mock)")
