#!/usr/bin/env python3
"""Compile actual Q2 recovery-service/task functions with bounded API mocks."""
from pathlib import Path
import os
import subprocess
import tempfile


ROOT = Path(os.environ.get("Q2_SOURCE_ROOT", Path(__file__).resolve().parents[3]))
SOURCE = ROOT / "components/secure_bootstrap/secure_bootstrap_esp.c"
TXN_C = ROOT / "components/secure_bootstrap/panel_wifi_transaction.c"
TXN_H = ROOT / "components/secure_bootstrap/include/panel_wifi_transaction.h"


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


source = SOURCE.read_text(encoding="utf-8")
recovery = function_body(source, "static esp_err_t run_recovery_service(")
marker_callback = function_body(source, "static panel_wifi_txn_result_t txn_marker_write(")
task = function_body(source, "static void reconfigure_task(void *arg)")
apply = function_body(source, "static esp_err_t apply_actions(uint32_t actions)")
opener = function_body(source, "static esp_err_t open_provisioning(void)")
ensure_server = function_body(source, "static esp_err_t ensure_homey_server(void)")
logic_source = (ROOT / "components/secure_bootstrap/secure_bootstrap_logic.c").read_text(encoding="utf-8")
header_source = (ROOT / "components/secure_bootstrap/include/secure_bootstrap.h").read_text(encoding="utf-8")
wifi_decl_start = header_source.index("typedef enum {\n    SECURE_BOOTSTRAP_WIFI_BOOT")
wifi_decl_end = header_source.index("} secure_bootstrap_wifi_context_t;", wifi_decl_start) + len("} secure_bootstrap_wifi_context_t;")
wifi_types = header_source[wifi_decl_start:wifi_decl_end]
session_decl_start = header_source.index("typedef enum {\n    SECURE_BOOTSTRAP_SESSION_CLOSED")
session_decl_end = header_source.index("} secure_bootstrap_wifi_session_t;", session_decl_start) + len("} secure_bootstrap_wifi_session_t;")
session_types = header_source[session_decl_start:session_decl_end]
session_helpers = "\n".join(function_body(logic_source, signature) for signature in (
    "void secure_bootstrap_wifi_context_init(",
    "uint32_t secure_bootstrap_wifi_transition(",
    "void secure_bootstrap_wifi_session_init(",
    "static bool session_generation_matches(",
    "uint32_t secure_bootstrap_wifi_session_begin(",
    "bool secure_bootstrap_wifi_session_activate(",
    "bool secure_bootstrap_wifi_session_fail_open(",
    "bool secure_bootstrap_wifi_session_finish_commit(",
))

fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "panel_wifi_transaction.h"

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_STATE (-2)
#define ESP_ERR_INVALID_ARG (-5)
#define ESP_ERR_WIFI_NOT_STARTED (-3)
#define ESP_ERR_TIMEOUT (-4)
#define pdTRUE 1
#define pdMS_TO_TICKS(ms) (ms)
#define WIFI_MODE_APSTA 2
#define SECURE_BOOTSTRAP_WIFI_MAX_RETRIES 5U
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))

typedef enum { WIFI_RECOVERY_ACTION_NONE=0, WIFI_RECOVERY_ACTION_RESTORE_PREVIOUS,
               WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL } wifi_recovery_action_t;
typedef secure_bootstrap_wifi_session_t session_t;
typedef secure_bootstrap_wifi_context_t wifi_context_t;
typedef struct { unsigned char opaque[8]; } wifi_config_t;
typedef struct { char code[16]; bool consumed; } setup_code_t;
typedef struct { unsigned opaque; } secure_bootstrap_network_t;
typedef enum { PANEL_UI_WIFI_RECONFIGURE_OPENED=0,
               PANEL_UI_WIFI_RECONFIGURE_BLOCKED,
               PANEL_UI_WIFI_RECONFIGURE_FAILED,
               PANEL_UI_WIFI_CANCEL_FAILED,
               PANEL_UI_WIFI_RECONFIGURE_RESTORED } ui_result_t;

#define WIFI_PERSIST_STAGE_IDLE 0
#define SECURE_BOOTSTRAP_MAX_NETWORKS 4U

static int s_wifi_session_mux;
static panel_wifi_recovery_slot_t s_wifi_recovery_slot;
static bool s_wifi_recovery_quarantine=true;
static bool s_wifi_recovery_can_restore=false;
static bool s_wifi_recovery_can_clear=true;
static bool s_wifi_recovery_backup_valid=false;
static uint32_t s_wifi_recovery_generation=91U;
static bool s_wifi_txn_marker_present=true;
static panel_wifi_txn_marker_t s_wifi_txn_marker={PANEL_WIFI_TXN_RECOVERY_REQUIRED,false,91U};
static session_t s_wifi_session={0};
static uint32_t s_persist_session_generation=0U;
static bool s_persist_active=false, s_persist_task_scheduled=false;
static int s_persist_stage=0;
static bool s_wifi_failure_ui_rendered=false;
static bool s_candidate_valid=false;
static wifi_config_t s_candidate_config={0};
static wifi_context_t s_wifi={0};
static int s_wifi_mode_calls, s_wifi_start_calls;
static int s_configure_ap_calls, s_server_start_calls, s_portal_successes;
static int s_marker_clear_calls, s_storage_clear_calls;
static panel_wifi_txn_result_t s_marker_clear_failure, s_storage_clear_failure;
static int s_marker_write_calls;
static int s_fail_marker_state=-1;
static panel_wifi_txn_marker_t s_durable_marker;
static panel_wifi_txn_result_t s_restore_failure;
static int s_mode_fail_call;
static esp_err_t s_mode_failure, s_start_failure, s_configure_ap_failure, s_server_start_failure;
static void *s_server;
static bool s_homey_handlers_registered;
static esp_err_t s_handler_registration_failure;
static ui_result_t s_ui_result=(ui_result_t)-1;
static bool s_retry_button=false, s_startup_active=false, s_recovery_controls=false;
static bool s_pending=false;
static int s_ap_config_mutex_storage;
static int *s_ap_config_mutex=&s_ap_config_mutex_storage;
static setup_code_t s_code={.code="12345678"};
static esp_err_t apply_actions(uint32_t actions);
static esp_err_t open_provisioning(void);
static panel_wifi_txn_result_t txn_marker_write(void *context,
    const panel_wifi_txn_marker_t *marker);
static int64_t esp_timer_get_time(void) { return 1000000; }
const char *esp_err_to_name(esp_err_t err) { (void)err; return "MOCK_ERROR"; }
static void vTaskDelete(void *task) { (void)task; }
static bool reconfigure_pending_set(bool value) { s_pending=value; return true; }
static void startup_status_active_set(bool value) { s_startup_active=value; }
static void set_recovery_controls_visible(bool restore, bool clear)
{ s_recovery_controls=restore||clear; }
static void set_reconfigure_button_visible(bool value) { s_retry_button=value; }
static void panel_wifi_result_update(ui_result_t value) { s_ui_result=value; }
static unsigned wifi_state_snapshot(void) { return s_wifi.state; }
static void update_homey_data_boot_view(uint64_t ms)
{ (void)ms; if (s_wifi.state==SECURE_BOOTSTRAP_WIFI_PROVISIONING_FAILED) s_retry_button=true; }

static panel_wifi_txn_result_t no_backup(void *c, bool p) { (void)c;(void)p;return 0; }
static esp_err_t wifi_txn_marker_write_verified(const panel_wifi_txn_marker_t *m)
{ s_marker_write_calls++;if((int)m->state==s_fail_marker_state)return ESP_FAIL;
  s_durable_marker=*m;return ESP_OK; }
static panel_wifi_txn_result_t txn_marker_write_recovery_takeover(
    const panel_wifi_txn_marker_t *m)
{ (void)m;return ESP_OK; }
static panel_wifi_txn_result_t marker_clear(void *c,uint32_t g)
{ (void)c;assert(g==91U);s_marker_clear_calls++;if(s_marker_clear_failure==0)s_wifi_txn_marker_present=false;return s_marker_clear_failure; }
static panel_wifi_txn_result_t backup_clear(void *c) { (void)c;return 0; }
static bool executor(void *c,uint32_t g) { (void)c;(void)g;return true; }
static panel_wifi_txn_result_t candidate(void *c,uint32_t g) { (void)c;(void)g;return 0; }
static panel_wifi_txn_result_t restore(void *c,uint32_t g)
{ (void)c;(void)g;return s_restore_failure; }
static panel_wifi_txn_result_t clear_first(void *c,uint32_t g)
{ (void)c;assert(g==91U);s_storage_clear_calls++;return s_storage_clear_failure; }
static panel_wifi_txn_ops_t wifi_txn_ops(void)
{
    return (panel_wifi_txn_ops_t){.context=&s_wifi,
      .backup_prepare_verified=no_backup,.marker_write_verified=txn_marker_write,
      .marker_clear_verified=marker_clear,.backup_clear=backup_clear,
      .executor_available=executor,.candidate_persist_verified=candidate,
      .restore_previous_verified=restore,.clear_first_install_verified=clear_first};
}
static esp_err_t esp_wifi_set_mode(int mode)
{ assert(mode==WIFI_MODE_APSTA);s_wifi_mode_calls++;return s_wifi_mode_calls==s_mode_fail_call?s_mode_failure:ESP_OK; }
static esp_err_t esp_wifi_start(void)
{ s_wifi_start_calls++;return s_start_failure; }
static int xSemaphoreTake(void *sem, unsigned ticks) { (void)sem;(void)ticks;return pdTRUE; }
static int xSemaphoreGive(void *sem) { (void)sem;return pdTRUE; }
static void startup_logo_dwell_cancel(void) {}
static esp_err_t generate_code(void) { return ESP_OK; }
static esp_err_t configure_access_point(const char *code)
{ assert(code!=NULL);s_configure_ap_calls++;return s_configure_ap_failure; }
static esp_err_t server_start(void)
{ s_server_start_calls++;if(s_server_start_failure==ESP_OK)s_server=(void *)1;return s_server_start_failure; }
static void athom_oauth_runtime_report_bootstrap_failure(esp_err_t err) { (void)err; }
static unsigned athom_oauth_runtime_bootstrap_error_code(void) { return 0U; }
static void athom_oauth_runtime_clear_bootstrap_failure(void) {}
static void panel_provisioning_display_update(const char *title,const char *detail,const char *code)
{ (void)title;(void)detail;(void)code; }
static void phone_provisioning_set_display_callback(
    void (*cb)(const char *,const char *,const char *)) { (void)cb; }
static void phone_provisioning_set_portal_css(const char *css) { (void)css; }
static const char *portal_shared_css(void) { return ""; }
static esp_err_t phone_provisioning_register_handlers(void *server)
{ (void)server;return s_handler_registration_failure; }
static void set_display_text(const char *a,const char *b,const char *c)
{ (void)a;(void)b;(void)c; }
static void panel_wifi_present_session_locked(void) { s_portal_successes++; }
static esp_err_t start_network_scan(void) { return ESP_OK; }
static bool networks_snapshot_copy(secure_bootstrap_network_t *p,size_t cap,size_t *n)
{ (void)p;(void)cap;*n=0U;return true; }
static void secure_zero(void *p,size_t n) { volatile unsigned char *v=p;while(n--)*v++=0U; }
static esp_err_t restore_saved_ram(void) { return ESP_OK; }
static esp_err_t connect_current(void) { return ESP_OK; }
static void close_provisioning(void) {}
static esp_err_t schedule_persistent_commit(void) { return ESP_OK; }
static void phone_provisioning_on_wifi_online(void) {}
static uint32_t wifi_transition_serialized(int event)
{ return secure_bootstrap_wifi_transition(&s_wifi,(secure_bootstrap_wifi_event_t)event); }

'''

c_source = "#include <stdbool.h>\n#include <stddef.h>\n#include <stdint.h>\n#include <string.h>\n#define SECURE_BOOTSTRAP_WIFI_MAX_RETRIES 5U\n" + wifi_types + "\n" + session_types + "\n" + session_helpers + "\n" + fixture + "\n" + marker_callback + "\n" + ensure_server + "\n" + apply + "\n" + opener + "\n" + recovery + "\n" + task + r'''
static void reset_fixture(void)
{
    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    s_wifi_recovery_quarantine=true;s_wifi_recovery_can_restore=false;
    s_wifi_recovery_can_clear=true;s_wifi_recovery_backup_valid=false;
    s_wifi_recovery_generation=91U;s_wifi_txn_marker_present=true;
    s_wifi_txn_marker=(panel_wifi_txn_marker_t){PANEL_WIFI_TXN_RECOVERY_REQUIRED,false,91U};
    secure_bootstrap_wifi_context_init(&s_wifi);s_wifi_session=(session_t){0};
    secure_bootstrap_wifi_transition(&s_wifi,SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_RECOVERY_REQUIRED);
    secure_bootstrap_wifi_session_init(&s_wifi_session);
    s_persist_session_generation=0U;s_persist_active=false;
    s_persist_task_scheduled=false;s_persist_stage=0;
    s_wifi_failure_ui_rendered=false;
    s_wifi_mode_calls=s_wifi_start_calls=0;
    s_configure_ap_calls=s_server_start_calls=s_portal_successes=0;
    s_server=NULL;s_homey_handlers_registered=false;
    s_handler_registration_failure=ESP_OK;
    s_marker_clear_calls=s_storage_clear_calls=0;
    s_marker_write_calls=0;s_fail_marker_state=-1;s_restore_failure=0;
    memset(&s_durable_marker,0,sizeof(s_durable_marker));
    s_mode_fail_call=0;
    s_mode_failure=s_start_failure=s_configure_ap_failure=s_server_start_failure=ESP_OK;
    s_marker_clear_failure=s_storage_clear_failure=0;
    s_ui_result=(ui_result_t)-1;s_retry_button=false;s_startup_active=false;
    startup_status_active_set(false);
    s_recovery_controls=false;s_pending=true;
}

static void set_committing_session(void)
{
    s_wifi_session.generation=91U;
    s_wifi_session.phase=SECURE_BOOTSTRAP_SESSION_COMMITTING;
    s_persist_session_generation=91U;s_persist_active=true;
    s_persist_task_scheduled=true;s_persist_stage=1;
}

static void run_case(const char *name, esp_err_t mode, esp_err_t start,
                     int fail_mode_call, esp_err_t ap, esp_err_t server,
                     bool expect_portal)
{
    reset_fixture();s_mode_failure=mode;s_start_failure=start;
    s_mode_fail_call=fail_mode_call;s_configure_ap_failure=ap;
    s_server_start_failure=server;
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL);
    assert(s_storage_clear_calls==1);
    assert(s_marker_clear_calls==1);
    assert(!s_wifi_txn_marker_present && !s_wifi_recovery_quarantine);
    if(expect_portal) {
        assert(s_ui_result==PANEL_UI_WIFI_RECONFIGURE_OPENED);
        assert(s_wifi.state==SECURE_BOOTSTRAP_WIFI_PROVISIONING && s_portal_successes==1);
        assert(s_wifi_session.phase==SECURE_BOOTSTRAP_SESSION_ACTIVE);
        assert(s_wifi_session.generation==1U);
        assert(!s_retry_button);
    } else {
        assert(s_ui_result==PANEL_UI_WIFI_RECONFIGURE_FAILED);
        assert(s_wifi.state==SECURE_BOOTSTRAP_WIFI_PROVISIONING_FAILED);
        assert(s_retry_button);
        assert(!s_recovery_controls);
    }
    printf("%s=PASS storage=complete portal=%s retry=%s\n",name,
           expect_portal?"open":"failed",s_retry_button?"available":"not_needed");
}

static void run_storage_failure(const char *name, bool fail_marker_clear)
{
    reset_fixture();
    if(fail_marker_clear)s_marker_clear_failure=-1;
    else s_storage_clear_failure=-1;
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL);
    assert(s_ui_result==PANEL_UI_WIFI_RECONFIGURE_FAILED);
    assert(s_wifi_txn_marker_present && s_wifi_recovery_quarantine);
    assert(!s_retry_button && s_recovery_controls);
    assert(!s_marker_clear_calls || fail_marker_clear);
    printf("%s=PASS marker_retained=true portal_not_started=true\n",name);
}

static void run_handler_registration_failure(void)
{
    reset_fixture();s_handler_registration_failure=ESP_FAIL;
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL);
    assert(s_ui_result==PANEL_UI_WIFI_RECONFIGURE_FAILED);
    assert(!s_wifi_txn_marker_present && !s_wifi_recovery_quarantine);
    assert(s_retry_button && s_server_start_calls==1);
    printf("portal_handler_registration_failure=PASS storage=complete portal=failed retry=available\n");
}

static void run_committing_session_success(void)
{
    reset_fixture();
    set_committing_session();
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL);
    assert(s_storage_clear_calls==1 && !s_wifi_txn_marker_present);
    assert(s_wifi_session.phase==SECURE_BOOTSTRAP_SESSION_ACTIVE);
    assert(s_wifi_session.generation==93U);
    assert(!s_persist_active && !s_persist_task_scheduled && s_persist_stage==WIFI_PERSIST_STAGE_IDLE);
    assert(s_portal_successes==1 && s_ui_result==PANEL_UI_WIFI_RECONFIGURE_OPENED);
    printf("committing_finalize_then_new_active_session=PASS old=91 new=93\n");
}

static void run_committing_storage_failure(bool marker_failure)
{
    reset_fixture();set_committing_session();
    if(marker_failure)s_marker_clear_failure=ESP_FAIL;
    else s_storage_clear_failure=ESP_FAIL;
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL);
    assert(s_ui_result==PANEL_UI_WIFI_RECONFIGURE_FAILED);
    assert(s_wifi_session.phase==SECURE_BOOTSTRAP_SESSION_COMMITTING);
    assert(s_wifi_session.generation==91U && s_persist_active);
    assert(s_wifi_txn_marker_present && s_wifi_recovery_quarantine);
    assert(s_server_start_calls==0 && s_portal_successes==0);
    printf("committing_%s_failure=PASS owner=retained phase=COMMITTING generation=91\n",
           marker_failure?"marker_finalization":"storage");
}

static void run_stale_service_generation(void)
{
    reset_fixture();set_committing_session();s_wifi_recovery_generation=92U;
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL);
    assert(s_ui_result==PANEL_UI_WIFI_RECONFIGURE_FAILED);
    assert(s_storage_clear_calls==0 && s_marker_clear_calls==0);
    assert(s_wifi_session.phase==SECURE_BOOTSTRAP_SESSION_COMMITTING);
    assert(s_wifi_session.generation==91U && s_persist_active);
    assert(s_wifi_txn_marker_present && s_wifi_recovery_quarantine);
    assert(s_server_start_calls==0 && s_portal_successes==0);
    printf("stale_service_generation=PASS storage_untouched=true session_untouched=true\n");
}

static void run_portal_failure_then_explicit_retry(void)
{
    reset_fixture();set_committing_session();s_server_start_failure=ESP_FAIL;
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL);
    assert(s_ui_result==PANEL_UI_WIFI_RECONFIGURE_FAILED);
    assert(s_wifi_session.phase==SECURE_BOOTSTRAP_SESSION_CLOSED);
    assert(s_wifi_session.generation==93U);
    assert(!s_pending && s_retry_button && !s_wifi_txn_marker_present);
    assert(s_storage_clear_calls==1 && s_marker_clear_calls==1);
    const uint32_t closed_generation=s_wifi_session.generation;
    s_server_start_failure=ESP_OK;
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_NONE);
    assert(s_ui_result==PANEL_UI_WIFI_RECONFIGURE_OPENED);
    assert(s_wifi_session.phase==SECURE_BOOTSTRAP_SESSION_ACTIVE);
    assert(s_wifi_session.generation==closed_generation+1U);
    assert(s_storage_clear_calls==1 && s_marker_clear_calls==1);
    assert(s_portal_successes==1);
    printf("portal_failure_operator_retry=PASS clear_count=1 marker_clear_count=1 new_generation=%u\n",
           (unsigned)s_wifi_session.generation);
}

static void run_duplicate_service_notification(void)
{
    reset_fixture();set_committing_session();
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL);
    assert(s_wifi_session.phase==SECURE_BOOTSTRAP_SESSION_ACTIVE);
    const uint32_t generation=s_wifi_session.generation;
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL);
    assert(s_wifi_session.generation==generation);
    assert(s_storage_clear_calls==1 && s_marker_clear_calls==1);
    assert(s_portal_successes==1);
    printf("duplicate_service=PASS finalization_not_repeated=true\n");
}

static void run_restore_then_recovery_marker_failure(void)
{
    reset_fixture();
    s_wifi_txn_marker=(panel_wifi_txn_marker_t){PANEL_WIFI_TXN_RECOVERY_REQUIRED,true,91U};
    s_wifi_txn_marker_present=true;
    s_wifi_recovery_quarantine=true;s_wifi_recovery_can_restore=true;
    s_wifi_recovery_can_clear=false;s_wifi_recovery_backup_valid=true;
    s_wifi_recovery_generation=91U;s_restore_failure=ESP_FAIL;
    s_fail_marker_state=PANEL_WIFI_TXN_RECOVERY_REQUIRED;
    reconfigure_task((void *)(uintptr_t)WIFI_RECOVERY_ACTION_RESTORE_PREVIOUS);
    assert(s_marker_write_calls==2);
    assert(s_durable_marker.state==PANEL_WIFI_TXN_ROLLBACK_PENDING);
    assert(s_wifi_txn_marker_present && s_wifi_txn_marker.state==PANEL_WIFI_TXN_ROLLBACK_PENDING);
    assert(s_wifi_recovery_backup_valid && s_wifi_recovery_can_restore);
    assert(s_wifi_recovery_quarantine);
    assert(s_ui_result==PANEL_UI_WIFI_RECONFIGURE_FAILED);
    assert(s_recovery_controls && !s_pending);
    assert(s_marker_clear_calls==0 && s_storage_clear_calls==0);
    printf("restore_failure_recovery_marker_failure=PASS quarantine=true backup_retained=true retry_control=visible writes=%d\n",
           s_marker_write_calls);
}

int main(void)
{
    if(getenv("Q2_SESSION_BASELINE_ONLY")!=NULL) {
        run_committing_session_success();
        return 0;
    }
    run_case("wifi_mode_setup_failure",ESP_FAIL,ESP_OK,1,ESP_OK,ESP_OK,false);
    run_case("wifi_start_failure",ESP_OK,ESP_FAIL,0,ESP_OK,ESP_OK,false);
    run_case("portal_mode_setup_failure",ESP_FAIL,ESP_OK,2,ESP_OK,ESP_OK,false);
    run_case("ap_configuration_failure",ESP_OK,ESP_OK,0,ESP_FAIL,ESP_OK,false);
    run_case("http_portal_start_failure",ESP_OK,ESP_OK,0,ESP_OK,ESP_FAIL,false);
    run_case("portal_success",ESP_OK,ESP_OK,0,ESP_OK,ESP_OK,true);
    run_committing_session_success();
    run_committing_storage_failure(false);
    run_committing_storage_failure(true);
    run_stale_service_generation();
    run_portal_failure_then_explicit_retry();
    run_duplicate_service_notification();
    run_restore_then_recovery_marker_failure();
    run_handler_registration_failure();
    run_storage_failure("sta_clear_failure",false);
    run_storage_failure("marker_clear_failure",true);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="q2-first-install-portal-") as td:
    td = Path(td)
    cfile = td / "test.c"
    binary = td / "test"
    cfile.write_text(c_source, encoding="utf-8")
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(TXN_H.parent), str(cfile), str(TXN_C), "-o", str(binary)
    ], check=True)
    subprocess.run([str(binary)], check=True)

    baseline_marker_callback = r'''static panel_wifi_txn_result_t txn_marker_write(void *context, const panel_wifi_txn_marker_t *marker)
{
    (void)context;
    if (marker == NULL) return ESP_ERR_INVALID_ARG;
    const esp_err_t err = wifi_txn_marker_write_verified(marker);
    if (err == ESP_OK) {
        s_wifi_txn_marker = *marker;
        s_wifi_txn_marker_present = true;
        s_wifi_recovery_quarantine =
            marker->state == PANEL_WIFI_TXN_RECOVERY_REQUIRED;
        if (s_wifi_recovery_quarantine) s_wifi_recovery_generation = marker->generation;
    }
    return err;
}'''
    baseline_source = c_source.replace(marker_callback, baseline_marker_callback, 1)
    if baseline_source == c_source:
        raise SystemExit("cannot construct exact pre-F3 callback baseline")
    baseline_file = td / "baseline.c"
    baseline_binary = td / "baseline"
    baseline_file.write_text(baseline_source, encoding="utf-8")
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(TXN_H.parent), str(baseline_file), str(TXN_C), "-o", str(baseline_binary)
    ], check=True)
    baseline_result = subprocess.run([str(baseline_binary)], capture_output=True)
    if baseline_result.returncode == 0:
        raise SystemExit("pre-F3 callback unexpectedly passed recovery quarantine assertion")
    print("PASS: pre-F3 actual run_recovery_service path fails quarantine assertion:",
          baseline_result.returncode)

# Compile the actual touchscreen action handler separately to verify that a
# failed short-lived service-task creation restores the explicit retry button.
recovery_event = function_body(source, "static void recovery_action_event(lv_event_t *event)")
event_fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { int code; void *data; } lv_event_t;
typedef struct { bool hidden; } lv_obj_t;
typedef enum { WIFI_RECOVERY_ACTION_NONE=0, WIFI_RECOVERY_ACTION_RESTORE_PREVIOUS,
               WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL } wifi_recovery_action_t;
#define LV_EVENT_CLICKED 1
#define LV_OBJ_FLAG_HIDDEN 2U
#define pdPASS 1
#define ESP_LOGE(...) ((void)0)
static bool s_wifi_recovery_quarantine=true;
static bool s_wifi_recovery_can_restore=false,s_wifi_recovery_can_clear=true;
static bool s_pending=false,s_startup_active=true;
static lv_obj_t s_restore_button={true},s_clear_button={true};
static lv_obj_t *s_recovery_restore_button=&s_restore_button;
static lv_obj_t *s_recovery_clear_button=&s_clear_button;
static void reconfigure_task(void *arg) { (void)arg; }
static int lv_event_get_code(lv_event_t *e) { return e->code; }
static void *lv_event_get_user_data(lv_event_t *e) { return e->data; }
static bool reconfigure_pending_get(void) { return s_pending; }
static bool reconfigure_pending_set(bool value) { s_pending=value;return true; }
static bool reconfigure_pending_set_if_clear(void)
{
    if (s_pending) return false;
    s_pending = true;
    return true;
}
static void startup_status_active_set(bool b) { s_startup_active=b; }
static void lv_obj_add_flag(lv_obj_t *o,unsigned f) { assert(f==LV_OBJ_FLAG_HIDDEN);o->hidden=true; }
static void lv_obj_remove_flag(lv_obj_t *o,unsigned f) { assert(f==LV_OBJ_FLAG_HIDDEN);o->hidden=false; }
static int xTaskCreate(void (*f)(void *),const char *n,unsigned s,void *a,unsigned p,void *h)
{ (void)f;(void)n;(void)s;(void)a;(void)p;(void)h;return 0; }
'''
event_main = r'''
int main(void)
{
    lv_event_t event={LV_EVENT_CLICKED,(void *)(uintptr_t)WIFI_RECOVERY_ACTION_CLEAR_FIRST_INSTALL};
    recovery_action_event(&event);
    assert(!s_pending && s_startup_active);
    assert(!s_clear_button.hidden && s_restore_button.hidden);
    assert(s_wifi_recovery_quarantine && s_wifi_recovery_can_clear);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="q2-service-create-failure-") as td:
    td=Path(td); cfile=td/"test.c"; binary=td/"test"
    cfile.write_text(event_fixture+"\n"+recovery_event+"\n"+event_main,encoding="utf-8")
    subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror","-pedantic",
                    str(cfile),"-o",str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print("service_task_creation_failure=PASS marker_retained=true retry_control=visible")

# Exercise the actual Wi-Fi/NVS adapter decision for STA readback. The lower
# ESP-IDF calls are mocked; the production adapter must reject nonempty data.
clear_adapter = function_body(source, "static panel_wifi_txn_result_t txn_clear_first_install(")
adapter_fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef int esp_err_t;
typedef int panel_wifi_txn_result_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_WIFI_NOT_STARTED (-2)
#define ESP_ERR_INVALID_STATE (-3)
#define WIFI_IF_STA 0
typedef struct { unsigned char sta[8]; } wifi_config_t;
static wifi_config_t s_flash_config;
static wifi_config_t s_saved_config;
static bool s_saved_valid=true;
static bool s_force_nonempty_readback;
static int esp_wifi_stop(void) { return ESP_OK; }
static int esp_wifi_deinit(void) { return ESP_OK; }
static int driver_init_flash(void) { return ESP_OK; }
static int esp_wifi_set_config(int iface,const wifi_config_t *config)
{ assert(iface==WIFI_IF_STA);s_flash_config=*config;return ESP_OK; }
static int esp_wifi_get_config(int iface,wifi_config_t *config)
{ assert(iface==WIFI_IF_STA);*config=s_flash_config;
  if (s_force_nonempty_readback) {
      config->sta[0] = 0x5a;
  }
  return ESP_OK;
}
static bool wifi_sta_config_empty(const wifi_config_t *config)
{ static const wifi_config_t empty={0};return memcmp(config,&empty,sizeof(*config))==0; }
static void secure_zero(void *p,size_t n)
{ volatile unsigned char *v=p;while(n--)*v++=0; }
'''
adapter_main = r'''
int main(void)
{
    s_flash_config.sta[0]=0x41;
    assert(txn_clear_first_install(NULL,9U)==ESP_OK);
    assert(wifi_sta_config_empty(&s_flash_config) && !s_saved_valid);
    s_saved_valid=true;s_force_nonempty_readback=true;
    assert(txn_clear_first_install(NULL,10U)==ESP_ERR_INVALID_STATE);
    assert(s_saved_valid);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="q2-sta-readback-adapter-") as td:
    td=Path(td); cfile=td/"test.c"; binary=td/"test"
    cfile.write_text(adapter_fixture+"\n"+clear_adapter+"\n"+adapter_main,encoding="utf-8")
    subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror","-pedantic",
                    str(cfile),"-o",str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print("actual_sta_clear_adapter_readback_mismatch=PASS")
