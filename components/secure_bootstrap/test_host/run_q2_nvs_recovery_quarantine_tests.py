#!/usr/bin/env python3
"""Exercise the production rollback/NVS marker paths with persistent mocks."""
from pathlib import Path
import os
import subprocess
import tempfile


ROOT = Path(os.environ.get("Q2_SOURCE_ROOT", Path(__file__).resolve().parents[1]))
SOURCE = ROOT / "secure_bootstrap_esp.c"
LOGIC = ROOT / "secure_bootstrap_logic.c"
TXN_C = ROOT / "panel_wifi_transaction.c"
TXN_H = ROOT / "include/panel_wifi_transaction.h"


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
current_failure = '''    } else {
        /* Keep runtime quarantine authoritative even when persisting the
         * stronger marker state fails. The original unresolved marker and
         * backup remain the durable recovery evidence in that case. */
        portENTER_CRITICAL(&s_wifi_session_mux);
        s_wifi_recovery_quarantine = true;
        s_wifi_recovery_generation = generation;
        s_wifi_recovery_can_restore =
            s_wifi_txn_marker.previous_config_present && s_wifi_recovery_backup_valid;
        s_wifi_recovery_can_clear =
            !s_wifi_txn_marker.previous_config_present && !s_wifi_recovery_backup_valid;
        portEXIT_CRITICAL(&s_wifi_session_mux);
        panel_wifi_txn_marker_t failed = s_wifi_txn_marker;
        failed.state = PANEL_WIFI_TXN_RECOVERY_REQUIRED;
        const esp_err_t marker_update_result = txn_marker_write(NULL, &failed);
        (void)wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED);
        portENTER_CRITICAL(&s_wifi_session_mux);
        s_deferred_rollback_queued = false;
        (void)panel_wifi_recovery_finish(&s_wifi_recovery_slot, generation,
            PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING);
        /* Keep persistence/session ownership until a confirmed manual service. */
        portEXIT_CRITICAL(&s_wifi_session_mux);
        ESP_LOGE(TAG, "WIFI_ROLLBACK recovery_required=true result=%s marker_update=%s runtime_quarantine=true durable_marker=%s",
                 esp_err_to_name(result), esp_err_to_name(marker_update_result),
                 marker_update_result == ESP_OK ? "recovery_required" : "unresolved_unverified");
    }'''
baseline_failure = '''    } else {
        panel_wifi_txn_marker_t failed = s_wifi_txn_marker;
        failed.state = PANEL_WIFI_TXN_RECOVERY_REQUIRED;
        (void)txn_marker_write(NULL, &failed);
        (void)wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED);
        portENTER_CRITICAL(&s_wifi_session_mux);
        s_deferred_rollback_queued = false;
        (void)panel_wifi_recovery_finish(&s_wifi_recovery_slot, generation,
            PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING);
        /* Keep persistence/session ownership until a confirmed manual service. */
        portEXIT_CRITICAL(&s_wifi_session_mux);
        ESP_LOGE(TAG, "WIFI_ROLLBACK recovery_required=true result=%s", esp_err_to_name(result));
    }'''
if current_failure not in source:
    raise SystemExit("cannot construct exact pre-fix production function for baseline regression")
baseline_source = source.replace(current_failure, baseline_failure, 1)
marker_write_lock = '''    /* Serialize marker writers with generation-checked clears. NVS calls are
     * individually thread-safe, but get-then-erase is not an atomic compare
     * and clear. The existing task-context mutex prevents another firmware
     * transaction from replacing the marker between those operations. */
    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
        secure_zero(expected, sizeof(expected));
        return ESP_ERR_TIMEOUT;
    }
'''
marker_clear_lock = '''    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(3000)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
'''
marker_unlock = '    (void)xSemaphoreGive(s_ap_config_mutex);\n'
signatures = (
    "static esp_err_t wifi_txn_marker_read(",
    "static esp_err_t wifi_txn_marker_write_verified_internal(",
    "static esp_err_t wifi_txn_marker_write_verified(",
    "static esp_err_t wifi_txn_marker_clear_verified(",
    "static esp_err_t backup_clear(",
    "static esp_err_t backup_write_saved(",
    "static esp_err_t backup_read(",
    "static panel_wifi_txn_result_t txn_marker_write(",
    "static panel_wifi_txn_result_t txn_marker_clear(",
    "static void execute_rollback_owned(",
    "static esp_err_t wifi_recovery_boot_load(",
)
functions = "\n".join(function_body(source, sig) for sig in signatures)
if (functions.count(marker_write_lock) != 1 or functions.count(marker_clear_lock) != 1 or
        functions.count(marker_unlock) != 2):
    raise SystemExit("cannot construct pre-serialization marker adapter for race regression")
pre_lock_functions = (functions.replace(marker_write_lock, "", 1)
                      .replace(marker_clear_lock, "", 1)
                      .replace(marker_unlock, "", 2))
missing_marker_guard = "    if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_ERR_INVALID_STATE;\n"
generation_guard = """            if (current_valid && current_marker.generation != marker->generation)
                err = ESP_ERR_INVALID_STATE;
            else if (!current_valid && !allow_recovery_takeover)
                err = ESP_ERR_INVALID_CRC;
"""
if functions.count(missing_marker_guard) != 1 or functions.count(generation_guard) != 1:
    raise SystemExit("cannot construct exact pre-F1/F2 marker adapter baseline")
pre_f1_f2_functions = (functions.replace(missing_marker_guard, "", 1)
                       .replace(generation_guard, "", 1))
logic_source = LOGIC.read_text(encoding="utf-8")
logic_functions = "\n".join(function_body(logic_source, sig) for sig in (
    "uint32_t secure_bootstrap_crc32(",
    "bool secure_bootstrap_wifi_backup_encode(",
    "bool secure_bootstrap_wifi_backup_decode(",
))

fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "panel_wifi_transaction.h"
typedef int esp_err_t;
typedef int nvs_handle_t;
typedef int portMUX_TYPE;
typedef void *SemaphoreHandle_t;
typedef struct { uint32_t generation; int phase; } session_t;
typedef struct { struct { uint8_t ssid[32]; uint8_t password[64]; } sta; } wifi_config_t;
typedef struct { int state; bool saved_config_present; } wifi_context_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_ARG (-2)
#define ESP_ERR_INVALID_CRC (-3)
#define ESP_ERR_INVALID_STATE (-4)
#define ESP_ERR_NVS_NOT_FOUND (-5)
#define ESP_ERR_NO_MEM (-6)
#define ESP_ERR_INVALID_SIZE (-7)
#define ESP_ERR_TIMEOUT (-8)
#define pdTRUE 1
#define pdMS_TO_TICKS(ms) (ms)
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define WIFI_BACKUP_NAMESPACE "hpanel_wifi"
#define WIFI_BACKUP_KEY "backup_v1"
#define WIFI_TXN_MARKER_KEY "txn_v1"
#define SECURE_BOOTSTRAP_WIFI_BACKUP_MAGIC 0x48505742U
#define SECURE_BOOTSTRAP_WIFI_BACKUP_VERSION 1U
#define SECURE_BOOTSTRAP_WIFI_BACKUP_PAYLOAD_MAX 256U
#define SECURE_BOOTSTRAP_WIFI_BACKUP_BLOB_SIZE (12U + SECURE_BOOTSTRAP_WIFI_BACKUP_PAYLOAD_MAX)
typedef struct { uint32_t magic; uint16_t version,payload_size; uint32_t payload_crc32; uint8_t payload[SECURE_BOOTSTRAP_WIFI_BACKUP_PAYLOAD_MAX]; } secure_bootstrap_wifi_backup_blob_t;
uint32_t secure_bootstrap_crc32(const void *data,size_t size);
bool secure_bootstrap_wifi_backup_encode(secure_bootstrap_wifi_backup_blob_t *blob,const void *payload,size_t payload_size);
bool secure_bootstrap_wifi_backup_decode(const secure_bootstrap_wifi_backup_blob_t *blob,void *payload,size_t payload_capacity);
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
#define TAG "host"
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGD(...) ((void)0)
enum { SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_OK = 10,
       SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED = 11,
       SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_RECOVERY_REQUIRED = 12 };
enum { SECURE_BOOTSTRAP_SESSION_CLOSED = 0, SECURE_BOOTSTRAP_SESSION_COMMITTING = 2 };
enum { SECURE_BOOTSTRAP_WIFI_BOOT = 0 };
enum { WIFI_PERSIST_STAGE_IDLE = 0, WIFI_PERSIST_STAGE_ROLLBACK = 4 };
static int s_wifi_session_mux;
static int mock_mutex_storage;
static SemaphoreHandle_t s_ap_config_mutex = &mock_mutex_storage;
static bool marker_mutex_held;
static bool inject_marker_replacement_on_erase;
static esp_err_t marker_replacement_result = ESP_OK;
static esp_err_t wifi_txn_marker_write_verified(const panel_wifi_txn_marker_t *marker);
static int xSemaphoreTake(SemaphoreHandle_t semaphore, unsigned timeout)
{ (void)semaphore;(void)timeout;if(marker_mutex_held)return 0;marker_mutex_held=true;return pdTRUE; }
static int xSemaphoreGive(SemaphoreHandle_t semaphore)
{ (void)semaphore;assert(marker_mutex_held);marker_mutex_held=false;return pdTRUE; }
static bool s_persist_active = true;
static bool s_persist_task_scheduled = true;
static int s_persist_stage = 2;
static uint32_t s_persist_session_generation = 41U;
static bool s_wifi_txn_marker_present = true;
static panel_wifi_txn_marker_t s_wifi_txn_marker = {
    PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING, true, 41U};
static bool s_deferred_rollback_queued = true;
static bool s_wifi_recovery_quarantine = false;
static bool s_wifi_recovery_backup_valid = true;
static bool s_wifi_recovery_can_restore = true;
static bool s_wifi_recovery_can_clear = false;
static uint32_t s_wifi_recovery_generation = 41U;
static panel_wifi_recovery_slot_t s_wifi_recovery_slot;
static session_t s_wifi_session={41U,SECURE_BOOTSTRAP_SESSION_COMMITTING};
static int rollback_result = ESP_FAIL;
static int transition_calls;
static int nvs_failure;
static int nvs_get_calls;
static bool persistent_marker_present;
static uint8_t persistent_marker[PANEL_WIFI_TXN_MARKER_SIZE];
static uint8_t staged_marker[PANEL_WIFI_TXN_MARKER_SIZE];
static bool staged_marker_valid;
static bool persistent_backup_present = true;
static uint8_t persistent_backup[SECURE_BOOTSTRAP_WIFI_BACKUP_BLOB_SIZE];
static uint8_t original_backup[SECURE_BOOTSTRAP_WIFI_BACKUP_BLOB_SIZE];
static uint8_t staged_backup[SECURE_BOOTSTRAP_WIFI_BACKUP_BLOB_SIZE];
static bool staged_backup_valid;
static bool staged_backup_erase;
static unsigned homey_sentinel = 0x55aa33ccU;
static unsigned saved_homey_sentinel;
static bool s_wifi_recovery_ui_rendered;
static wifi_config_t s_saved_config;
static bool s_saved_valid;
static wifi_context_t s_wifi;

enum { FAIL_NONE, FAIL_SET, FAIL_COMMIT, FAIL_READBACK, FAIL_OPEN,
       FAIL_CLEAR_READBACK, FAIL_ERASE, FAIL_BACKUP_SET, FAIL_BACKUP_COMMIT,
       FAIL_BACKUP_READBACK, FAIL_BACKUP_OPEN, FAIL_BACKUP_ERASE,
       FAIL_BACKUP_CLEAR_COMMIT, FAIL_BACKUP_CORRUPT, FAIL_MARKER_READ };
static esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *handle)
{ (void)mode; if (nvs_failure==FAIL_OPEN || (!strcmp(ns,"hpanel_wifi") && nvs_failure==FAIL_BACKUP_OPEN)) return ESP_ERR_NO_MEM; *handle=1; return ESP_OK; }
static esp_err_t nvs_set_blob(nvs_handle_t handle,const char *key,const void *data,size_t size)
{ (void)handle;
  if (!strcmp(key,"backup_v1")) {
    if (size!=sizeof(staged_backup))return ESP_ERR_INVALID_ARG;
    if(nvs_failure==FAIL_BACKUP_SET)return ESP_ERR_NO_MEM;
    memcpy(staged_backup,data,size);staged_backup_valid=true;return ESP_OK;
  }
  if (strcmp(key,"txn_v1") || size!=sizeof(staged_marker)) return ESP_ERR_INVALID_ARG;
  if (nvs_failure==FAIL_SET) return ESP_ERR_NO_MEM; memcpy(staged_marker,data,size);staged_marker_valid=true;return ESP_OK; }
static esp_err_t nvs_get_blob(nvs_handle_t handle,const char *key,void *data,size_t *size)
{ (void)handle; ++nvs_get_calls; if(nvs_failure==FAIL_OPEN)return ESP_ERR_NO_MEM;
  if(!strcmp(key,"backup_v1")) {
    if(nvs_failure==FAIL_BACKUP_OPEN)return ESP_ERR_NO_MEM;
    if(!persistent_backup_present)return ESP_ERR_NVS_NOT_FOUND;
    if(nvs_failure==FAIL_BACKUP_READBACK)return ESP_ERR_NO_MEM;
    if(data==NULL){*size=sizeof(persistent_backup);return ESP_OK;}
    if(*size<sizeof(persistent_backup))return ESP_ERR_INVALID_ARG;
    memcpy(data,persistent_backup,sizeof(persistent_backup));
    if(nvs_failure==FAIL_BACKUP_CORRUPT)((uint8_t*)data)[24]^=1U;
    *size=sizeof(persistent_backup);return ESP_OK;
  }
  if(strcmp(key,"txn_v1"))return ESP_ERR_NVS_NOT_FOUND;
  if(nvs_failure==FAIL_MARKER_READ)return ESP_ERR_NO_MEM;
  if(nvs_failure==FAIL_READBACK && nvs_get_calls>=2)return ESP_ERR_NO_MEM;
  if(nvs_failure==FAIL_CLEAR_READBACK && nvs_get_calls>=2)return ESP_ERR_NO_MEM;
  if(!persistent_marker_present)return ESP_ERR_NVS_NOT_FOUND;
  if(data==NULL){*size=sizeof(persistent_marker);return ESP_OK;}
  if(*size<sizeof(persistent_marker))return ESP_ERR_INVALID_ARG;
  memcpy(data,persistent_marker,sizeof(persistent_marker));*size=sizeof(persistent_marker);return ESP_OK; }
static bool staged_erase;
static esp_err_t nvs_commit(nvs_handle_t handle)
{ (void)handle; if(nvs_failure==FAIL_COMMIT || nvs_failure==FAIL_BACKUP_COMMIT || nvs_failure==FAIL_BACKUP_CLEAR_COMMIT)return ESP_FAIL;
  if(staged_erase){persistent_marker_present=false;staged_erase=false;}
  if(staged_backup_erase){persistent_backup_present=false;staged_backup_erase=false;}
  if(staged_backup_valid){memcpy(persistent_backup,staged_backup,sizeof(persistent_backup));persistent_backup_present=true;staged_backup_valid=false;}
  if(staged_marker_valid){memcpy(persistent_marker,staged_marker,sizeof(persistent_marker));persistent_marker_present=true;staged_marker_valid=false;} return ESP_OK; }
static esp_err_t nvs_erase_key(nvs_handle_t handle,const char *key)
{ (void)handle;
  if(!strcmp(key,"backup_v1")) {
    if(nvs_failure==FAIL_BACKUP_ERASE)return ESP_FAIL;
    staged_backup_erase=true;return ESP_OK;
  }
  if(strcmp(key,"txn_v1"))return ESP_ERR_INVALID_ARG;
  if(inject_marker_replacement_on_erase){
    inject_marker_replacement_on_erase=false;
    panel_wifi_txn_marker_t newer={PANEL_WIFI_TXN_RECOVERY_REQUIRED,true,42U};
    marker_replacement_result=wifi_txn_marker_write_verified(&newer);
  }
  if(nvs_failure==FAIL_ERASE)return ESP_FAIL; staged_erase=true;return ESP_OK; }
static void nvs_close(nvs_handle_t handle) {(void)handle;}
static void secure_zero(void *p,size_t n){volatile uint8_t *v=p;while(n--)*v++=0;}
static const char *esp_err_to_name(esp_err_t e){(void)e;return "MOCK";}
static uint32_t esp_random(void){return 77U;}
static bool has_saved_ssid(const wifi_config_t *c){return c!=NULL&&c->sta.ssid[0]!=0U;}
static uint32_t secure_bootstrap_wifi_transition(wifi_context_t *ctx,int event)
{if(event==SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_RECOVERY_REQUIRED)ctx->state=7;return 0;}
static esp_err_t rollback_once(uint32_t generation){assert(generation==41U);return rollback_result;}
static uint32_t wifi_transition_serialized(int event){assert(event==SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED);++transition_calls;return 0;}
static esp_err_t esp_wifi_start(void){return ESP_OK;}
static esp_err_t apply_actions(uint32_t actions){(void)actions;return ESP_OK;}
static bool secure_bootstrap_wifi_session_finish_commit(void *session,uint32_t generation)
{(void)session;(void)generation;assert(!"failure path must not finish session");return false;}
static void reset_runtime(void)
{
    s_persist_active=true;s_persist_task_scheduled=true;s_persist_stage=2;
    s_persist_session_generation=41U;s_wifi_txn_marker_present=true;
    s_wifi_session=(session_t){41U,SECURE_BOOTSTRAP_SESSION_COMMITTING};
    s_wifi_txn_marker=(panel_wifi_txn_marker_t){PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING,true,41U};
    s_deferred_rollback_queued=true;s_wifi_recovery_quarantine=false;
    s_wifi_recovery_backup_valid=true;s_wifi_recovery_can_restore=true;
    s_wifi_recovery_can_clear=false;s_wifi_recovery_generation=41U;
    s_wifi_recovery_ui_rendered=false;s_saved_valid=false;
    memset(&s_saved_config,0,sizeof(s_saved_config));
    s_saved_config.sta.ssid[0]='S';s_saved_config.sta.password[0]='P';s_saved_valid=true;
    secure_bootstrap_wifi_backup_blob_t backup={0};
    assert(secure_bootstrap_wifi_backup_encode(&backup,&s_saved_config,sizeof(s_saved_config)));
    memcpy(persistent_backup,&backup,sizeof(persistent_backup));
    memcpy(original_backup,persistent_backup,sizeof(original_backup));
    secure_zero(&backup,sizeof(backup));
    memset(&s_wifi,0,sizeof(s_wifi));
    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    s_wifi_recovery_slot.generation=41U;
    s_wifi_recovery_slot.owner=PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING;
    transition_calls=0;nvs_get_calls=0;nvs_failure=FAIL_NONE;staged_marker_valid=false;
    panel_wifi_txn_marker_encode(&s_wifi_txn_marker,persistent_marker);
    persistent_marker_present=true;persistent_backup_present=true;
    saved_homey_sentinel=homey_sentinel;
}
static void simulated_reboot_runtime(void)
{
    s_persist_active=false;s_persist_task_scheduled=false;s_persist_stage=0;
    s_persist_session_generation=0U;s_wifi_txn_marker_present=false;
    s_wifi_session=(session_t){0U,SECURE_BOOTSTRAP_SESSION_CLOSED};
    memset(&s_wifi_txn_marker,0,sizeof(s_wifi_txn_marker));
    s_deferred_rollback_queued=false;s_wifi_recovery_quarantine=false;
    s_wifi_recovery_backup_valid=false;s_wifi_recovery_can_restore=false;
    s_wifi_recovery_can_clear=false;s_wifi_recovery_generation=0U;
    s_wifi_recovery_ui_rendered=false;s_saved_valid=false;
    memset(&s_saved_config,0,sizeof(s_saved_config));memset(&s_wifi,0,sizeof(s_wifi));
    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    nvs_failure=FAIL_NONE;nvs_get_calls=0;staged_marker_valid=false;staged_erase=false;
}
'''

main = r'''
int main(int argc,char **argv)
{
    assert(argc==2); reset_runtime();
    const char *mode=argv[1];
    if(!strcmp(mode,"set"))nvs_failure=FAIL_SET;
    else if(!strcmp(mode,"commit"))nvs_failure=FAIL_COMMIT;
    else if(!strcmp(mode,"readback"))nvs_failure=FAIL_READBACK;
    else if(!strcmp(mode,"open"))nvs_failure=FAIL_OPEN;
    else if(!strcmp(mode,"clear-commit"))nvs_failure=FAIL_COMMIT;
    else if(!strcmp(mode,"clear-readback"))nvs_failure=FAIL_CLEAR_READBACK;
    else if(!strcmp(mode,"clear-erase"))nvs_failure=FAIL_ERASE;
    else if(!strcmp(mode,"backup-set"))nvs_failure=FAIL_BACKUP_SET;
    else if(!strcmp(mode,"backup-commit"))nvs_failure=FAIL_BACKUP_COMMIT;
    else if(!strcmp(mode,"backup-readback"))nvs_failure=FAIL_BACKUP_READBACK;
    else if(!strcmp(mode,"backup-open"))nvs_failure=FAIL_BACKUP_OPEN;
    else if(!strcmp(mode,"backup-corrupt-readback"))nvs_failure=FAIL_BACKUP_CORRUPT;
    else if(!strcmp(mode,"backup-clear-erase"))nvs_failure=FAIL_BACKUP_ERASE;
    else if(!strcmp(mode,"backup-clear-commit"))nvs_failure=FAIL_BACKUP_CLEAR_COMMIT;
    else if(!strcmp(mode,"read-error"))nvs_failure=FAIL_OPEN;
    else if(!strcmp(mode,"read-corrupt"))persistent_marker[12]^=0x80U;
    else if(!strcmp(mode,"read-version"))persistent_marker[4]=0xffU;
    else if(!strcmp(mode,"boot-normal")){
        persistent_marker_present=false;persistent_backup_present=false;
        assert(wifi_recovery_boot_load()==ESP_OK);
        assert(!s_wifi_recovery_quarantine && !s_wifi_txn_marker_present);
        assert(s_wifi.state==0);return 0;
    }
    else if(!strcmp(mode,"boot-first-install")){
        s_wifi_txn_marker=(panel_wifi_txn_marker_t){PANEL_WIFI_TXN_CANDIDATE_WRITE_PENDING,false,41U};
        panel_wifi_txn_marker_encode(&s_wifi_txn_marker,persistent_marker);
        persistent_backup_present=false;
        assert(wifi_recovery_boot_load()==ESP_OK);
        assert(s_wifi_recovery_quarantine && s_wifi_recovery_can_clear && !s_wifi_recovery_can_restore);
        return 0;
    }
    else if(!strcmp(mode,"boot-corrupt")){
        persistent_marker[12]^=0x80U;
        assert(wifi_recovery_boot_load()==ESP_OK);
        assert(s_wifi_recovery_quarantine && !s_wifi_recovery_can_clear);
        return 0;
    }
    else if(!strcmp(mode,"boot-missing-backup")){
        persistent_marker_present=false;persistent_backup_present=true;
        assert(wifi_recovery_boot_load()==ESP_OK);
        assert(s_wifi_recovery_quarantine && s_wifi_recovery_can_restore);
        return 0;
    }
    else if(!strcmp(mode,"boot-valid-marker-no-backup")){
        persistent_backup_present=false;
        assert(wifi_recovery_boot_load()==ESP_OK);
        assert(s_wifi_recovery_quarantine && !s_wifi_recovery_can_restore &&
               !s_wifi_recovery_can_clear);
        return 0;
    }
    else if(!strcmp(mode,"boot-corrupt-backup")){
        nvs_failure=FAIL_BACKUP_CORRUPT;
        assert(wifi_recovery_boot_load()==ESP_OK);
        assert(s_wifi_recovery_quarantine && !s_wifi_recovery_can_restore &&
               !s_wifi_recovery_can_clear);
        return 0;
    }
    else if(!strcmp(mode,"boot-marker-read-error")){
        nvs_failure=FAIL_MARKER_READ;
        assert(wifi_recovery_boot_load()==ESP_OK);
        assert(s_wifi_recovery_quarantine && s_wifi_recovery_can_restore &&
               !s_wifi_recovery_can_clear);
        return 0;
    }
    else if(!strcmp(mode,"boot-backup-read-error")){
        nvs_failure=FAIL_BACKUP_READBACK;
        assert(wifi_recovery_boot_load()==ESP_OK);
        assert(s_wifi_recovery_quarantine && !s_wifi_recovery_can_restore &&
               !s_wifi_recovery_can_clear);
        return 0;
    }
    else if(!strcmp(mode,"read-valid")) {
        panel_wifi_txn_marker_t out={0}; bool present=false;
        assert(wifi_txn_marker_read(&out,&present)==ESP_OK && present);
        assert(out.generation==41U);return 0;
    }
    else if(!strcmp(mode,"generation")){
        panel_wifi_txn_marker_t newer={PANEL_WIFI_TXN_RECOVERY_REQUIRED,true,42U};
        panel_wifi_txn_marker_encode(&newer,persistent_marker);
        s_wifi_txn_marker.generation=41U;
        assert(txn_marker_clear(NULL,41U)!=ESP_OK);
        assert(persistent_marker_present);
        return 0;
    } else if(!strcmp(mode,"clear-missing-marker")){
        persistent_marker_present=false;
        s_wifi_txn_marker_present=true;
        s_wifi_txn_marker.generation=41U;
        s_wifi_recovery_quarantine=true;
        assert(txn_marker_clear(NULL,41U)!=ESP_OK);
        assert(!persistent_marker_present && s_wifi_recovery_quarantine &&
               persistent_backup_present && s_wifi_txn_marker_present);
        return 0;
    } else if(!strcmp(mode,"stale-generation-write")){
        panel_wifi_txn_marker_t newer={PANEL_WIFI_TXN_RECOVERY_REQUIRED,true,42U};
        panel_wifi_txn_marker_encode(&newer,persistent_marker);
        panel_wifi_txn_marker_t stale={PANEL_WIFI_TXN_RECOVERY_REQUIRED,true,41U};
        assert(wifi_txn_marker_write_verified(&stale)!=ESP_OK);
        panel_wifi_txn_marker_t disk={0};
        assert(panel_wifi_txn_marker_decode(persistent_marker,sizeof(persistent_marker),&disk));
        assert(persistent_marker_present && disk.generation==42U);
        return 0;
    } else if(!strcmp(mode,"replacement-race")){
        inject_marker_replacement_on_erase=true;
        const esp_err_t err=txn_marker_clear(NULL,41U);
        assert(err==ESP_OK);
        assert(marker_replacement_result==ESP_ERR_TIMEOUT);
        assert(!persistent_marker_present);
        assert(!marker_mutex_held);
        return 0;
    } else if(!strcmp(mode,"quarantine")) { }
    else if(!strncmp(mode,"backup-",7)) { }
    else return 2;
    if(!strncmp(mode,"read-",5)){
        panel_wifi_txn_marker_t out={0}; bool present=false;
        const esp_err_t err=wifi_txn_marker_read(&out,&present);
        if(!strcmp(mode,"read-error"))assert(err==ESP_ERR_NO_MEM && !present);
        else assert(err==ESP_ERR_INVALID_CRC && !present);
        return 0;
    }
    if(!strncmp(mode,"backup-",7)){
        if(!strcmp(mode,"backup-clear-erase") || !strcmp(mode,"backup-clear-commit")){
            const esp_err_t err=backup_clear();
            assert(err!=ESP_OK && persistent_backup_present);
            return 0;
        }
        if(!strcmp(mode,"backup-clear-success")){
            assert(backup_clear()==ESP_OK && !persistent_backup_present);return 0;
        }
        if(!strcmp(mode,"backup-read-success")){
            wifi_config_t out={0};bool present=false;
            assert(backup_read(&out,&present)==ESP_OK && present);
            assert(memcmp(&out,&s_saved_config,sizeof(out))==0);return 0;
        }
        if(!strcmp(mode,"backup-write-success")){
            assert(backup_write_saved()==ESP_OK);
            wifi_config_t out={0};bool present=false;
            assert(backup_read(&out,&present)==ESP_OK && present);
            assert(memcmp(&out,&s_saved_config,sizeof(out))==0);return 0;
        }
        if(!strcmp(mode,"backup-read-corrupt"))nvs_failure=FAIL_BACKUP_CORRUPT;
        if(!strcmp(mode,"backup-read-error"))nvs_failure=FAIL_BACKUP_READBACK;
        if(!strcmp(mode,"backup-read-open-error"))nvs_failure=FAIL_BACKUP_OPEN;
        if(!strcmp(mode,"backup-read-corrupt") || !strcmp(mode,"backup-read-error") ||
           !strcmp(mode,"backup-read-open-error")){
            wifi_config_t out={0};bool present=false;
            const esp_err_t err=backup_read(&out,&present);
            assert(err!=ESP_OK && !present);return 0;
        }
        if(!strcmp(mode,"backup-set") || !strcmp(mode,"backup-commit") ||
           !strcmp(mode,"backup-readback") || !strcmp(mode,"backup-open") ||
           !strcmp(mode,"backup-corrupt-readback")){
            const esp_err_t err=backup_write_saved();
            assert(err!=ESP_OK);
            assert(persistent_backup_present);
            if(!strcmp(mode,"backup-set") || !strcmp(mode,"backup-commit") || !strcmp(mode,"backup-open"))
                assert(memcmp(persistent_backup,original_backup,sizeof(persistent_backup))==0);
            return 0;
        }
        return 2;
    }
    if(!strncmp(mode,"clear-",6)){
        s_wifi_recovery_quarantine=true;
        const esp_err_t err=txn_marker_clear(NULL,41U);
        assert(err!=ESP_OK && s_wifi_txn_marker_present);
        assert(s_wifi_recovery_quarantine && persistent_backup_present);
        if(!strcmp(mode,"clear-readback")){
            assert(!persistent_marker_present);
            simulated_reboot_runtime();
            assert(wifi_recovery_boot_load()==ESP_OK);
            assert(s_wifi_recovery_quarantine && s_wifi_recovery_can_restore);
        }
        else assert(persistent_marker_present);
        return 0;
    }
    execute_rollback_owned(41U);
    assert(transition_calls==1);
    assert(s_persist_active && s_persist_task_scheduled);
    assert(s_wifi_recovery_slot.owner==PANEL_WIFI_RECOVERY_OWNER_NONE);
    assert(persistent_backup_present && homey_sentinel==saved_homey_sentinel);
    assert(persistent_marker_present);
    panel_wifi_txn_marker_t disk={0};
    assert(panel_wifi_txn_marker_decode(persistent_marker,sizeof(persistent_marker),&disk));
    assert(disk.generation==41U);
    if(!strcmp(mode,"readback") || !strcmp(mode,"quarantine"))assert(disk.state==PANEL_WIFI_TXN_RECOVERY_REQUIRED);
    else assert(disk.state==PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING);
    assert(s_wifi_recovery_quarantine);
    assert(s_wifi_recovery_can_restore && !s_wifi_recovery_can_clear);
    if(!strcmp(mode,"set") || !strcmp(mode,"commit") || !strcmp(mode,"readback") || !strcmp(mode,"open"))
        assert(s_wifi_txn_marker.state==PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING);
    else assert(s_wifi_txn_marker.state==PANEL_WIFI_TXN_RECOVERY_REQUIRED);
    simulated_reboot_runtime();
    assert(wifi_recovery_boot_load()==ESP_OK);
    assert(s_wifi_recovery_quarantine && s_wifi_recovery_can_restore && !s_wifi_recovery_can_clear);
    assert(s_wifi_txn_marker_present && s_wifi.saved_config_present);
    return 0;
}
'''

unit = ("#include <stdbool.h>\n#include <stddef.h>\n#include <stdint.h>\n#include <string.h>\n"
        + fixture + "\n" + functions + "\n" + logic_functions + "\n" + main)

with tempfile.TemporaryDirectory(prefix="q2-nvs-recovery-") as td:
    td = Path(td)
    c_file = td / "nvs_recovery.c"
    binary = td / "nvs_recovery"
    c_file.write_text(unit, encoding="utf-8")
    cmd = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
           "-Wno-unused-function", "-Wno-unused-variable", "-I", str(ROOT / "include"),
           str(c_file), str(TXN_C), "-o", str(binary)]
    if os.environ.get("Q2_HOST_SANITIZERS") == "1":
        cmd[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    subprocess.run(cmd, check=True)
    modes = ("set", "commit", "readback", "open", "quarantine",
             "read-valid", "read-corrupt", "read-version", "read-error",
             "clear-commit", "clear-readback", "clear-erase",
             "boot-normal", "boot-first-install", "boot-corrupt", "boot-missing-backup",
             "boot-valid-marker-no-backup", "boot-corrupt-backup",
             "boot-marker-read-error", "boot-backup-read-error",
             "backup-set", "backup-commit", "backup-readback", "backup-open",
             "backup-corrupt-readback", "backup-clear-erase", "backup-clear-commit",
             "backup-read-success", "backup-read-corrupt", "backup-read-error",
             "backup-read-open-error", "backup-clear-success", "backup-write-success")
    modes += ("clear-missing-marker", "stale-generation-write")
    results = {}
    details = {}
    for mode in modes:
        result = subprocess.run([str(binary), mode], capture_output=True)
        results[mode] = result.returncode
        details[mode] = result.stderr.decode(errors="replace")
    generation = subprocess.run([str(binary), "generation"], capture_output=True)
    results["generation"] = generation.returncode
    replacement = subprocess.run([str(binary), "replacement-race"], capture_output=True)
    results["replacement-race"] = replacement.returncode
    if any(results[name] != 0 for name in modes):
        raise SystemExit(f"rollback quarantine failure injection failed: {results}; {details}")
    # Compile the exact pre-correction production branch and prove it violates
    # the same runtime quarantine assertions for every marker persistence error.
    baseline_functions = "\n".join(function_body(baseline_source, sig) for sig in signatures)
    baseline_file = td / "baseline.c"
    baseline_binary = td / "baseline"
    baseline_file.write_text(unit.replace(functions, baseline_functions), encoding="utf-8")
    baseline_cmd = cmd.copy()
    baseline_cmd[baseline_cmd.index(str(c_file))] = str(baseline_file)
    baseline_cmd[baseline_cmd.index("-o") + 1] = str(baseline_binary)
    subprocess.run(baseline_cmd, check=True)
    baseline_results = {}
    for mode in ("set", "commit", "readback", "open"):
        baseline_results[mode] = subprocess.run(
            [str(baseline_binary), mode], capture_output=True).returncode
    if any(code == 0 for code in baseline_results.values()):
        raise SystemExit(f"pre-fix production path unexpectedly passed quarantine checks: {baseline_results}")
    print("PASS: corrected production rollback/NVS marker matrix:", results)
    print("PASS: baseline actual production path fails quarantine assertions:", baseline_results)
    if generation.returncode != 0 or replacement.returncode != 0:
        raise SystemExit("marker generation ownership/interleaving test failed")
    print("PASS: stale-generation and serialized marker replacement interleaving")
    pre_lock_file = td / "pre_lock.c"
    pre_lock_binary = td / "pre_lock"
    pre_lock_file.write_text(unit.replace(functions, pre_lock_functions), encoding="utf-8")
    pre_lock_cmd = cmd.copy()
    pre_lock_cmd[pre_lock_cmd.index(str(c_file))] = str(pre_lock_file)
    pre_lock_cmd[pre_lock_cmd.index("-o") + 1] = str(pre_lock_binary)
    subprocess.run(pre_lock_cmd, check=True)
    pre_lock_race = subprocess.run([str(pre_lock_binary), "replacement-race"], capture_output=True)
    if pre_lock_race.returncode == 0:
        raise SystemExit("pre-lock marker adapter unexpectedly passed replacement interleaving")
    print("PASS: pre-lock production adapter fails marker replacement interleaving:",
          pre_lock_race.returncode)
    pre_f1_f2_file = td / "pre_f1_f2.c"
    pre_f1_f2_binary = td / "pre_f1_f2"
    pre_f1_f2_file.write_text(unit.replace(functions, pre_f1_f2_functions), encoding="utf-8")
    pre_f1_f2_cmd = cmd.copy()
    pre_f1_f2_cmd[pre_f1_f2_cmd.index(str(c_file))] = str(pre_f1_f2_file)
    pre_f1_f2_cmd[pre_f1_f2_cmd.index("-o") + 1] = str(pre_f1_f2_binary)
    subprocess.run(pre_f1_f2_cmd, check=True)
    pre_f1_clear = subprocess.run(
        [str(pre_f1_f2_binary), "clear-missing-marker"], capture_output=True)
    pre_f2_write = subprocess.run(
        [str(pre_f1_f2_binary), "stale-generation-write"], capture_output=True)
    if pre_f1_clear.returncode == 0 or pre_f2_write.returncode == 0:
        raise SystemExit("pre-fix marker adapter unexpectedly passed F1/F2 regressions")
    print("PASS: exact pre-F1 adapter fails missing-marker clear regression:",
          pre_f1_clear.returncode)
    print("PASS: exact pre-F2 adapter fails stale-generation write regression:",
          pre_f2_write.returncode)
