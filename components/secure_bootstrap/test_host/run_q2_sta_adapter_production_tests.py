#!/usr/bin/env python3
"""Compile the real Q2 STA persistence adapters against deterministic ESP-IDF mocks."""
from pathlib import Path
import os
import subprocess
import tempfile


ROOT = Path(os.environ.get("Q2_SOURCE_ROOT", Path(__file__).resolve().parents[1]))
SOURCE = (ROOT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
LOGIC = (ROOT / "secure_bootstrap_logic.c").read_text(encoding="utf-8")


def body(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for end in range(brace, len(source)):
        depth += source[end] == "{"
        depth -= source[end] == "}"
        if depth == 0:
            return source[start:end + 1]
    raise AssertionError(f"unterminated {signature}")


prod = "\n".join(body(SOURCE, sig) for sig in (
    "static bool has_saved_ssid(",
    "static bool wifi_sta_config_equal(",
    "static bool wifi_sta_config_empty(",
    "static esp_err_t backup_read(",
    "static esp_err_t driver_init_flash(",
    "static esp_err_t persist_write_and_reopen(",
    "static bool txn_executor_available(",
    "static panel_wifi_txn_result_t txn_candidate_persist(",
    "static panel_wifi_txn_result_t txn_restore_previous(",
    "static panel_wifi_txn_result_t txn_clear_first_install(",
))
logic = "\n".join(body(LOGIC, sig) for sig in (
    "uint32_t secure_bootstrap_crc32(",
    "bool secure_bootstrap_wifi_backup_encode(",
    "bool secure_bootstrap_wifi_backup_decode(",
))

fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "secure_bootstrap.h"
#include "panel_wifi_transaction.h"
typedef int esp_err_t; typedef int nvs_handle_t; typedef int portMUX_TYPE;
typedef void *SemaphoreHandle_t; typedef void *TaskHandle_t; typedef int eTaskState;
typedef struct { int rssi, authmode; } threshold_t;
typedef struct { bool capable, required; } pmf_t;
typedef struct {
 uint8_t ssid[32], password[64]; int scan_method; bool bssid_set; uint8_t bssid[6];
 uint8_t channel; uint16_t listen_interval; int sort_method; threshold_t threshold;
 pmf_t pmf_cfg; bool rm_enabled, btm_enabled, mbo_enabled, ft_enabled, owe_enabled;
 bool transition_disable, disable_wpa3_compatible_mode; int sae_pwe_h2e, sae_pk_mode;
 uint8_t failure_retry_cnt; bool he_dcm_set; int he_dcm_max_constellation_tx;
 int he_dcm_max_constellation_rx; bool he_mcs9_enabled, he_su_beamformee_disabled;
 bool he_trig_su_bmforming_feedback_disabled, he_trig_mu_bmforming_partial_feedback_disabled;
 bool he_trig_cqi_feedback_disabled, vht_su_beamformee_disabled, vht_mu_beamformee_disabled;
 bool vht_mcs8_enabled; uint8_t reserved1, reserved2; uint8_t sae_h2e_identifier[32];
} wifi_sta_config_t;
typedef struct { wifi_sta_config_t sta; } wifi_config_t;
typedef struct { int dummy; } wifi_init_config_t;
typedef struct { uint32_t generation; int phase; } session_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_ARG (-2)
#define ESP_ERR_INVALID_STATE (-3)
#define ESP_ERR_INVALID_CRC (-4)
#define ESP_ERR_INVALID_SIZE (-5)
#define ESP_ERR_NOT_FOUND (-6)
#define ESP_ERR_WIFI_NOT_STARTED (-7)
#define ESP_ERR_NVS_NOT_FOUND (-8)
#define WIFI_INIT_CONFIG_DEFAULT() ((wifi_init_config_t){0})
#define WIFI_MODE_STA 1
#define WIFI_STORAGE_FLASH 1
#define WIFI_IF_STA 0
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define WIFI_BACKUP_NAMESPACE "hpanel_wifi"
#define WIFI_BACKUP_KEY "backup_v1"
#define SECURE_BOOTSTRAP_SESSION_COMMITTING 2
#define pdTRUE 1
#define pdMS_TO_TICKS(x) (x)
#define eDeleted 4
#define eSuspended 3
#define ESP_RETURN_ON_ERROR(err, tag, msg) do { (void)(tag); (void)(msg); esp_err_t _e=(err); if (_e!=ESP_OK) return _e; } while (0)
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
#define ESP_LOGI(...) ((void)0)
#define TAG "host"
static int s_wifi_session_mux;
static bool s_saved_valid; static wifi_config_t s_saved_config, s_candidate_config;
static bool s_persist_active=true, s_deferred_provisioning_worker_ready=true;
static uint32_t s_persist_session_generation=7; static session_t s_wifi_session={7,2};
static int s_deferred_provisioning_queue_storage, s_worker_storage;
static void *s_deferred_provisioning_queue=&s_deferred_provisioning_queue_storage;
static TaskHandle_t s_deferred_provisioning_task_handle=&s_worker_storage;
static wifi_config_t sta_flash, sta_ram, backup_config;
static bool backup_present;
static int selected_storage, driver_running;
static bool force_readback_mismatch;
static eTaskState mock_worker_state=1;
static int calls[16], fail_on_call[16], fail_with[16];
enum { OP_STOP=1, OP_DEINIT, OP_INIT, OP_MODE, OP_STORAGE, OP_SET_CONFIG,
       OP_GET_CONFIG, OP_CONNECT, OP_NVS_OPEN, OP_NVS_GET, OP_NVS_SET,
       OP_NVS_COMMIT, OP_NVS_ERASE };
static int op(int n) { ++calls[n]; return calls[n]==fail_on_call[n] ? fail_with[n] : ESP_OK; }
static esp_err_t esp_wifi_stop(void) { int e=op(OP_STOP); if(e==ESP_OK)driver_running=0;return e; }
static esp_err_t esp_wifi_deinit(void) { int e=op(OP_DEINIT); if(e==ESP_OK)driver_running=0;return e; }
static esp_err_t esp_wifi_init(const wifi_init_config_t *c) { (void)c;int e=op(OP_INIT);if(e==ESP_OK)driver_running=1;return e; }
static esp_err_t esp_wifi_set_mode(int m) { (void)m;return op(OP_MODE); }
static esp_err_t esp_wifi_set_storage(int s) { int e=op(OP_STORAGE);if(e==ESP_OK)selected_storage=s;return e; }
static esp_err_t esp_wifi_set_config(int iface,const wifi_config_t *c) { (void)iface;int e=op(OP_SET_CONFIG);if(e==ESP_OK){if(selected_storage==WIFI_STORAGE_FLASH)sta_flash=*c;else sta_ram=*c;}return e; }
static esp_err_t esp_wifi_get_config(int iface,wifi_config_t *c) { (void)iface;int e=op(OP_GET_CONFIG);if(e==ESP_OK){*c=(selected_storage==WIFI_STORAGE_FLASH)?sta_flash:sta_ram;if(force_readback_mismatch)c->sta.ssid[0]^=1U;}return e; }
static esp_err_t esp_wifi_connect(void) { return op(OP_CONNECT); }
static eTaskState eTaskGetState(TaskHandle_t t) { (void)t;return mock_worker_state; }
static int xSemaphoreTake(SemaphoreHandle_t s,unsigned t) { (void)s;(void)t;return pdTRUE; }
static int xSemaphoreGive(SemaphoreHandle_t s) { (void)s;return pdTRUE; }
static const char *esp_err_to_name(esp_err_t e) { (void)e;return "MOCK"; }
static void secure_zero(void *p,size_t n) { volatile uint8_t *b=p;while(n--)*b++=0; }
static esp_err_t nvs_open(const char *ns,int mode,nvs_handle_t *h) { (void)ns;(void)mode;int e=op(OP_NVS_OPEN);if(e==ESP_OK)*h=1;return e; }
static esp_err_t nvs_get_blob(nvs_handle_t h,const char *k,void *d,size_t *n) { (void)h;(void)k;int e=op(OP_NVS_GET);if(e!=ESP_OK)return e;if(!backup_present)return ESP_ERR_NVS_NOT_FOUND;if(d==NULL){*n=sizeof(secure_bootstrap_wifi_backup_blob_t);return ESP_OK;}secure_bootstrap_wifi_backup_blob_t b;if(!secure_bootstrap_wifi_backup_encode(&b,&backup_config,sizeof(backup_config)))return ESP_FAIL;if(*n<sizeof(b))return ESP_ERR_INVALID_SIZE;memcpy(d,&b,sizeof(b));*n=sizeof(b);return ESP_OK; }
static esp_err_t nvs_set_blob(nvs_handle_t h,const char *k,const void *d,size_t n) { (void)h;(void)k;(void)d;(void)n;return op(OP_NVS_SET); }
static esp_err_t nvs_commit(nvs_handle_t h) { (void)h;return op(OP_NVS_COMMIT); }
static esp_err_t nvs_erase_key(nvs_handle_t h,const char *k) { (void)h;(void)k;return op(OP_NVS_ERASE); }
static void nvs_close(nvs_handle_t h) { (void)h; }
static bool has_saved_ssid(const wifi_config_t *c);
'''

main = r'''
static wifi_config_t config(uint8_t ssid,uint8_t pass) { wifi_config_t c={0};c.sta.ssid[0]=ssid;c.sta.password[0]=pass;c.sta.channel=3;c.sta.pmf_cfg.capable=true;return c; }
static void reset(void) { memset(calls,0,sizeof(calls));memset(fail_on_call,0,sizeof(fail_on_call));memset(fail_with,0,sizeof(fail_with));selected_storage=WIFI_STORAGE_FLASH;driver_running=1;s_saved_config=config('O','P');backup_config=s_saved_config;backup_present=true;s_saved_valid=true;s_candidate_config=config('N','Q');sta_flash=config('O','P');sta_ram=config('R','M');s_persist_active=true;s_deferred_provisioning_worker_ready=true;s_wifi_session=(session_t){7,2};s_persist_session_generation=7;force_readback_mismatch=false;mock_worker_state=1; }
static void inject(int operation,int nth,int error) { fail_on_call[operation]=nth;fail_with[operation]=error; }
int main(void) {
 reset();assert(txn_candidate_persist(NULL,7)==ESP_OK);assert(wifi_sta_config_equal(&sta_flash,&s_candidate_config));assert(wifi_sta_config_equal(&s_saved_config,&s_candidate_config));assert(!wifi_sta_config_equal(&sta_ram,&sta_flash));
 reset();inject(OP_SET_CONFIG,1,ESP_FAIL);assert(txn_candidate_persist(NULL,7)!=ESP_OK);assert(wifi_sta_config_equal(&sta_flash,&backup_config));
 reset();inject(OP_GET_CONFIG,1,ESP_FAIL);assert(txn_candidate_persist(NULL,7)!=ESP_OK);
 reset();inject(OP_STOP,1,ESP_FAIL);assert(txn_candidate_persist(NULL,7)==ESP_FAIL);
 reset();inject(OP_DEINIT,1,ESP_FAIL);assert(txn_candidate_persist(NULL,7)==ESP_FAIL);
 reset();inject(OP_INIT,1,ESP_FAIL);assert(txn_candidate_persist(NULL,7)==ESP_FAIL);
 reset();inject(OP_STORAGE,1,ESP_FAIL);assert(txn_candidate_persist(NULL,7)==ESP_FAIL);
 reset();inject(OP_MODE,1,ESP_FAIL);assert(txn_candidate_persist(NULL,7)==ESP_FAIL);
 reset();inject(OP_DEINIT,2,ESP_FAIL);assert(txn_candidate_persist(NULL,7)==ESP_FAIL);
 reset();inject(OP_INIT,2,ESP_FAIL);assert(txn_candidate_persist(NULL,7)==ESP_FAIL);
 reset();inject(OP_MODE,2,ESP_FAIL);assert(txn_candidate_persist(NULL,7)==ESP_FAIL);
 reset();inject(OP_STORAGE,2,ESP_FAIL);assert(txn_candidate_persist(NULL,7)==ESP_FAIL);
 reset();s_deferred_provisioning_worker_ready=false;assert(txn_candidate_persist(NULL,7)==ESP_ERR_INVALID_STATE);assert(calls[OP_SET_CONFIG]==0);
 reset();mock_worker_state=eSuspended;assert(txn_candidate_persist(NULL,7)==ESP_ERR_INVALID_STATE);assert(calls[OP_SET_CONFIG]==0);
 reset();assert(txn_restore_previous(NULL,7)==ESP_OK);assert(wifi_sta_config_equal(&sta_flash,&backup_config));
 reset();inject(OP_SET_CONFIG,1,ESP_FAIL);assert(txn_restore_previous(NULL,7)!=ESP_OK);
 reset();inject(OP_MODE,1,ESP_FAIL);assert(txn_restore_previous(NULL,7)!=ESP_OK);
 reset();inject(OP_STORAGE,1,ESP_FAIL);assert(txn_restore_previous(NULL,7)!=ESP_OK);
 reset();force_readback_mismatch=true;assert(txn_candidate_persist(NULL,7)==ESP_ERR_INVALID_STATE);
 /* Failed driver calls and post-write readback mismatch remain fail-closed. */
 reset();inject(OP_GET_CONFIG,1,ESP_FAIL);assert(txn_restore_previous(NULL,7)!=ESP_OK);
 reset();force_readback_mismatch=true;assert(txn_restore_previous(NULL,7)==ESP_ERR_INVALID_CRC);
 reset();assert(txn_clear_first_install(NULL,7)==ESP_OK);assert(wifi_sta_config_empty(&sta_flash));assert(!wifi_sta_config_empty(&sta_ram));
 reset();inject(OP_STOP,1,ESP_FAIL);assert(txn_clear_first_install(NULL,7)==ESP_FAIL);
 reset();inject(OP_SET_CONFIG,1,ESP_FAIL);assert(txn_clear_first_install(NULL,7)==ESP_FAIL);
 reset();force_readback_mismatch=true;assert(txn_clear_first_install(NULL,7)==ESP_ERR_INVALID_STATE);
 puts("PASS production STA adapter core paths");return 0;
}
'''

unit = fixture + "\n" + prod + "\n" + logic + "\n" + main
with tempfile.TemporaryDirectory(prefix="q2-sta-adapter-") as td:
    c = Path(td) / "sta.c"
    exe = Path(td) / "sta"
    c.write_text(unit, encoding="utf-8")
    cmd = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
           "-Wno-unused-function", "-I", str(ROOT / "include"), str(c), "-o", str(exe)]
    if os.environ.get("Q2_HOST_SANITIZERS") == "1":
        cmd[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    subprocess.run(cmd, check=True)
    subprocess.run([str(exe)], check=True)
