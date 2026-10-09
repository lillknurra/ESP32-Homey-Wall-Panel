#include "athom_oauth_runtime.h"
#include "athom_cloud_client.h"
#include "runtime_diag_journal.h"

#include <stdbool.h>
#include <stddef.h>

static bool patch037_light_toggle_dispatch_gate(
    size_t widget_index,
    bool homey_runtime_ready,
    bool execution_ready)
{
    const bool supported_widget = widget_index == 4U || widget_index == 5U;
    return supported_widget && homey_runtime_ready && execution_ready;
}

static athom_light_toggle_dispatch_result_t patch037_map_light_write_result(
    athom_homey_light_write_result_t result)
{
    switch (result) {
    case ATHOM_HOMEY_LIGHT_WRITE_ACCEPTED:
        return ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED;
    case ATHOM_HOMEY_LIGHT_WRITE_NOT_READY:
        return ATHOM_LIGHT_TOGGLE_DISPATCH_NOT_READY;
    case ATHOM_HOMEY_LIGHT_WRITE_TARGET_NOT_FOUND:
        return ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_NOT_FOUND;
    case ATHOM_HOMEY_LIGHT_WRITE_TARGET_INVALID:
        return ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_INVALID;
    case ATHOM_HOMEY_LIGHT_WRITE_UNAUTHORIZED:
        return ATHOM_LIGHT_TOGGLE_DISPATCH_UNAUTHORIZED;
    case ATHOM_HOMEY_LIGHT_WRITE_REJECTED:
        return ATHOM_LIGHT_TOGGLE_DISPATCH_REJECTED;
    case ATHOM_HOMEY_LIGHT_WRITE_TRANSPORT_AMBIGUOUS:
        return ATHOM_LIGHT_TOGGLE_DISPATCH_TRANSPORT_AMBIGUOUS;
    case ATHOM_HOMEY_LIGHT_WRITE_INVALID_ARGUMENT:
    case ATHOM_HOMEY_LIGHT_WRITE_INTERNAL_ERROR:
    default:
        return ATHOM_LIGHT_TOGGLE_DISPATCH_INTERNAL_ERROR;
    }
}

static const char *patch037_light_toggle_dispatch_result_name(
    athom_light_toggle_dispatch_result_t result)
{
    switch (result) {
    case ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED: return "accepted";
    case ATHOM_LIGHT_TOGGLE_DISPATCH_INVALID_WIDGET: return "invalid_widget";
    case ATHOM_LIGHT_TOGGLE_DISPATCH_NOT_READY: return "not_ready";
    case ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_NOT_FOUND: return "target_not_found";
    case ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_INVALID: return "target_invalid";
    case ATHOM_LIGHT_TOGGLE_DISPATCH_UNAUTHORIZED: return "unauthorized";
    case ATHOM_LIGHT_TOGGLE_DISPATCH_REJECTED: return "rejected";
    case ATHOM_LIGHT_TOGGLE_DISPATCH_TRANSPORT_AMBIGUOUS: return "transport_ambiguous";
    case ATHOM_LIGHT_TOGGLE_DISPATCH_INTERNAL_ERROR: return "internal_error";
    default: return "unknown";
    }
}

static bool __attribute__((unused)) patch038_dispatch_result_requires_authoritative_refresh(
    athom_light_toggle_dispatch_result_t result)
{
    switch (result) {
    case ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED:
    case ATHOM_LIGHT_TOGGLE_DISPATCH_UNAUTHORIZED:
    case ATHOM_LIGHT_TOGGLE_DISPATCH_REJECTED:
    case ATHOM_LIGHT_TOGGLE_DISPATCH_TRANSPORT_AMBIGUOUS:
    case ATHOM_LIGHT_TOGGLE_DISPATCH_INTERNAL_ERROR:
        return true;
    case ATHOM_LIGHT_TOGGLE_DISPATCH_INVALID_WIDGET:
    case ATHOM_LIGHT_TOGGLE_DISPATCH_NOT_READY:
    case ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_NOT_FOUND:
    case ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_INVALID:
    default:
        return false;
    }
}

#ifdef ESP_PLATFORM
#include "athom_auth_store.h"
#include "athom_oauth_config.h"
#include "athom_oauth_flow.h"
#include "athom_restore_policy.h"
#include "panel_homey_favorites.h"
#include "phone_provisioning.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "mdns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "athom_oauth";
static bool s_mdns_started;
static athom_oauth_session_t s_session;
static athom_oauth_result_t s_last = ATHOM_OAUTH_ERR_STATE;
static athom_cloud_state_t s_cloud;
static char s_code[ATHOM_OAUTH_CODE_MAX];
static bool s_worker_running;
static bool s_select_worker_running;
static bool s_restore_worker_running;
static bool s_restore_started;
static bool s_schema_refresh_running;
static QueueHandle_t s_homey_command_queue;
static TaskHandle_t s_homey_command_worker_task;
static bool s_refresh_job_reserved;
static bool s_light_toggle_job_reserved;
static bool s_periodic_refresh_scheduler_running;
static size_t s_light_toggle_pending_widget;
static uint32_t s_light_toggle_completion_generation;
static bool s_periodic_snapshot_seen;
static uint64_t s_periodic_snapshot_captured_at_ms;
static uint64_t s_periodic_refresh_next_attempt_ms;
static portMUX_TYPE s_refresh_job_mux = portMUX_INITIALIZER_UNLOCKED;
static void maybe_start_preselection_restore_worker(void);

typedef enum {
    ATHOM_NETWORK_PHASE_NONE = 0,
    ATHOM_NETWORK_PHASE_AUTH_RESTORE,
    ATHOM_NETWORK_PHASE_OAUTH,
    ATHOM_NETWORK_PHASE_HOMEY_SELECT,
    ATHOM_NETWORK_PHASE_INVENTORY_REFRESH,
    ATHOM_NETWORK_PHASE_LIGHT_TOGGLE,
    ATHOM_NETWORK_PHASE_LIVE_TOKEN_REFRESH,
    ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE,
    ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC,
} athom_network_phase_owner_t;

static portMUX_TYPE s_network_phase_mux = portMUX_INITIALIZER_UNLOCKED;
static athom_network_phase_owner_t s_network_phase_owner = ATHOM_NETWORK_PHASE_NONE;

static const char *network_phase_owner_name(athom_network_phase_owner_t owner)
{
    switch (owner) {
    case ATHOM_NETWORK_PHASE_NONE: return "none";
    case ATHOM_NETWORK_PHASE_AUTH_RESTORE: return "auth_restore";
    case ATHOM_NETWORK_PHASE_OAUTH: return "oauth";
    case ATHOM_NETWORK_PHASE_HOMEY_SELECT: return "homey_select";
    case ATHOM_NETWORK_PHASE_INVENTORY_REFRESH: return "inventory_refresh";
    case ATHOM_NETWORK_PHASE_LIGHT_TOGGLE: return "light_toggle";
    case ATHOM_NETWORK_PHASE_LIVE_TOKEN_REFRESH: return "live_token_refresh";
    case ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE: return "preselection_restore";
    case ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC: return "patch031_diagnostic";
    default: return "unknown";
    }
}

static bool network_phase_try_reserve(athom_network_phase_owner_t owner)
{
    if (owner == ATHOM_NETWORK_PHASE_NONE) return false;
    bool reserved = false;
    portENTER_CRITICAL(&s_network_phase_mux);
    if (s_network_phase_owner == ATHOM_NETWORK_PHASE_NONE) {
        s_network_phase_owner = owner;
        reserved = true;
    }
    portEXIT_CRITICAL(&s_network_phase_mux);
    ESP_LOGI(
        TAG,
        "PATCH041_NETWORK_PHASE action=reserve owner=%s result=%s privacy=sanitized",
        network_phase_owner_name(owner),
        reserved ? "accepted" : "busy");
    return reserved;
}

static void network_phase_release(athom_network_phase_owner_t owner)
{
    bool released = false;
    portENTER_CRITICAL(&s_network_phase_mux);
    if (owner != ATHOM_NETWORK_PHASE_NONE &&
        s_network_phase_owner == owner) {
        s_network_phase_owner = ATHOM_NETWORK_PHASE_NONE;
        released = true;
    }
    portEXIT_CRITICAL(&s_network_phase_mux);
    ESP_LOGI(
        TAG,
        "PATCH041_NETWORK_PHASE action=release owner=%s result=%s privacy=sanitized",
        network_phase_owner_name(owner),
        released ? "released" : "owner_mismatch");
    if (released &&
        owner != ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE &&
        owner != ATHOM_NETWORK_PHASE_AUTH_RESTORE) {
        maybe_start_preselection_restore_worker();
    }
}

typedef enum {
    PATCH031_DIAG_PROBE_UNUSED = 0,
    PATCH031_DIAG_PROBE_RESERVED,
    PATCH031_DIAG_PROBE_RUNNING,
    PATCH031_DIAG_PROBE_COMPLETE,
    PATCH031_DIAG_PROBE_FAILED,
    PATCH031_DIAG_PROBE_TASK_CREATE_FAILED,
} patch031_diag_probe_state_t;

#define PATCH031_DIAG_PROBE_WORKER_STACK 12288U
#define PATCH031_DIAG_PROBE_WORKER_PRIORITY 5U

static portMUX_TYPE s_patch031_diag_probe_mux = portMUX_INITIALIZER_UNLOCKED;
static patch031_diag_probe_state_t s_patch031_diag_probe_state = PATCH031_DIAG_PROBE_UNUSED;
static athom_cloud_debug_probe_result_t s_patch031_diag_probe_result;
static bool s_patch031_live_refresh_running;

static bool patch031_diag_probe_active_locked(void)
{
    return s_patch031_diag_probe_state == PATCH031_DIAG_PROBE_RESERVED ||
           s_patch031_diag_probe_state == PATCH031_DIAG_PROBE_RUNNING;
}

static void patch031_diag_live_refresh_end(void)
{
    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    s_patch031_live_refresh_running = false;
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);
}

static const char *patch031_diag_probe_state_name(patch031_diag_probe_state_t state)
{
    switch (state) {
    case PATCH031_DIAG_PROBE_UNUSED: return "unused";
    case PATCH031_DIAG_PROBE_RESERVED: return "reserved";
    case PATCH031_DIAG_PROBE_RUNNING: return "running";
    case PATCH031_DIAG_PROBE_COMPLETE: return "complete";
    case PATCH031_DIAG_PROBE_FAILED: return "failed";
    case PATCH031_DIAG_PROBE_TASK_CREATE_FAILED: return "task_create_failed";
    default: return "unknown";
    }
}

static void patch031_diag_cloud_probe_worker(void *arg)
{
    (void)arg;

    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    s_patch031_diag_probe_state = PATCH031_DIAG_PROBE_RUNNING;
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    athom_cloud_debug_probe_result_t result = {0};
    esp_err_t err = athom_cloud_debug_probe_user_me(&s_cloud, &result);

    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    s_patch031_diag_probe_result = result;
    s_patch031_diag_probe_state =
        err == ESP_OK ? PATCH031_DIAG_PROBE_COMPLETE : PATCH031_DIAG_PROBE_FAILED;
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    network_phase_release(ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC);
    vTaskDelete(NULL);
}


static athom_homey_data_state_t s_homey_data_state = ATHOM_HOMEY_DATA_LOADING;

static void runtime_diag_emit(
    uint16_t event_type,
    uint16_t result,
    int32_t error_code,
    int http_status,
    unsigned attempt,
    uint32_t retry_delay_ms,
    uint32_t snapshot_generation,
    uint8_t origin,
    uint8_t stage)
{
    runtime_diag_event_t event = {
        .event_type = event_type,
        .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
        .source = 2U,
        .origin = origin,
        .attempt = attempt > UINT16_MAX ? UINT16_MAX : (uint16_t)attempt,
        .result = result,
        .error_code = error_code,
        .http_status = http_status < 0 ? 0U :
            (http_status > UINT16_MAX ? UINT16_MAX : (uint16_t)http_status),
        .retry_delay_ms = retry_delay_ms,
        .snapshot_generation = snapshot_generation,
        .stage = stage,
    };
    (void)runtime_diag_journal_record(&event);
}

static void set_homey_data_state(athom_homey_data_state_t state)
{
    if (s_homey_data_state != state) {
        s_homey_data_state = state;
        runtime_diag_emit(RUNTIME_DIAG_EVENT_HOMEY_DATA_STATE_CHANGE,
                          (uint16_t)state, 0, 0, 0U, 0U, 0U, 0U, 0U);
    }
}

static bool s_boot_auto_refresh_scheduler_running;
static bool s_preselection_restore_worker_running;
static bool s_preselection_restore_pending;
static bool s_wifi_online;
static portMUX_TYPE s_preselection_restore_mux = portMUX_INITIALIZER_UNLOCKED;

typedef enum {
    ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA = 1,
    ATHOM_HOMEY_COMMAND_LIGHT_TOGGLE,
} athom_homey_command_kind_t;

typedef enum {
    ATHOM_REFRESH_ORIGIN_BOOT_AUTO = 0,
    ATHOM_REFRESH_ORIGIN_MANUAL,
    ATHOM_REFRESH_ORIGIN_PERIODIC,
    ATHOM_REFRESH_ORIGIN_LIGHT_RECONCILIATION,
} athom_refresh_origin_t;

/* PATCH061_DIAGNOSTIC_TYPES_BEGIN */
typedef enum {
    ATHOM_INVENTORY_STAGE_UNKNOWN = 0,
    ATHOM_INVENTORY_STAGE_CLOUD_USER_DISCOVERY,
    ATHOM_INVENTORY_STAGE_CLOUD_TOKEN,
    ATHOM_INVENTORY_STAGE_DELEGATION,
    ATHOM_INVENTORY_STAGE_HOMEY_REMOTE_LOGIN,
    ATHOM_INVENTORY_STAGE_PARSE,
    ATHOM_INVENTORY_STAGE_FAVORITES,
    ATHOM_INVENTORY_STAGE_ZONES,
    ATHOM_INVENTORY_STAGE_DEVICES,
    ATHOM_INVENTORY_STAGE_INVENTORY_COMPLETE,
    ATHOM_INVENTORY_STAGE_HANDOFF,
    ATHOM_INVENTORY_STAGE_OTHER,
} athom_inventory_attempt_stage_t;

typedef struct {
    uint32_t cloud_request_count;
    uint32_t homey_request_count;
    uint32_t cloud_client_init_count;
    uint32_t cloud_client_reuse_count;
    uint32_t cloud_client_cleanup_count;
    uint32_t homey_client_init_count;
    uint32_t homey_client_reuse_count;
    uint32_t homey_client_cleanup_count;
    uint32_t homey_session_create_count;
    uint32_t remote_rebind_count;
} athom_inventory_attempt_counter_deltas_t;

typedef struct {
    bool valid;
    uint32_t sequence;
    athom_refresh_origin_t origin;
    uint32_t attempt;
    uint64_t completed_at_ms;
    int32_t final_error;
    int32_t final_http_status;
    athom_inventory_attempt_stage_t stage;
    bool raw_transport_observed;
    athom_transport_role_t raw_role;
    int32_t raw_perform_error;
    athom_transport_class_t raw_classification;
    int32_t raw_http_status;
    int32_t raw_tls_query;
    int32_t raw_tls_error;
    int32_t raw_tls_flags;
    int32_t raw_socket_errno;
    uint32_t raw_request_elapsed_ms;
    athom_inventory_attempt_counter_deltas_t deltas;
} athom_inventory_attempt_diagnostic_t;

static portMUX_TYPE s_inventory_attempt_diagnostic_mux =
    portMUX_INITIALIZER_UNLOCKED;
static athom_inventory_attempt_diagnostic_t s_last_inventory_attempt_diagnostic;
/* PATCH061_DIAGNOSTIC_TYPES_END */

#define ATHOM_INVENTORY_ATTEMPT_DIAGNOSTIC_JSON_MAX 2048U

typedef struct {
    athom_homey_command_kind_t kind;
    athom_refresh_origin_t origin;
    size_t widget_index;
    bool value;
} athom_homey_command_t;

#define PERIODIC_REFRESH_INTERVAL_MS 60000ULL
#define PERIODIC_REFRESH_BUSY_DEFER_MS 5000ULL
#define PERIODIC_REFRESH_COOLDOWN_MS 30000ULL
#define PERIODIC_REFRESH_SCHEDULER_POLL_MS 1000U
#define ATHOM_BOOT_AUTO_READY_WAIT_ATTEMPTS 120U
#define ATHOM_BOOT_AUTO_READY_WAIT_MS 1000U
#define ATHOM_PRESELECT_RESTORE_MAX_ATTEMPTS 12U
#define ATHOM_PRESELECT_RESTORE_RETRY_MS 2000U
#define ATHOM_PRESELECT_RESTORE_MAX_ELAPSED_MS 120000U
#define ATHOM_HOMEY_DATA_RETRY_1_MS 5000U
#define ATHOM_HOMEY_DATA_RETRY_2_MS 10000U
#define ATHOM_HOMEY_DATA_RETRY_3_MS 20000U
#define ATHOM_HOMEY_DATA_RETRY_MAX_MS 30000U

typedef enum {
    ATHOM_REFRESH_QUEUE_OK = 0,
    ATHOM_REFRESH_QUEUE_NOT_READY,
    ATHOM_REFRESH_QUEUE_BUSY,
    ATHOM_REFRESH_QUEUE_FAILED,
} athom_refresh_queue_result_t;
static const char *s_state_name = "idle";
static uint32_t s_runtime_id;
static uint32_t s_select_attempt;

static int64_t now_s(void){return esp_timer_get_time()/1000000LL;}
static uint32_t elapsed_ms_since(int64_t start_us)
{
    int64_t elapsed_us = esp_timer_get_time() - start_us;
    if (elapsed_us < 0) elapsed_us = 0;
    return (uint32_t)(elapsed_us / 1000LL);
}
static void zero_secure(void *p,size_t n){volatile unsigned char*q=p;while(n--)*q++=0U;}
static bool unreserved(unsigned char c){return(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='.'||c=='_'||c=='~';}

static void patch021_homey_phase_log(
    const char *phase,
    unsigned attempt,
    int64_t phase_start_us,
    esp_err_t err,
    int http_status)
{
    athom_transport_metrics_t metrics;
    athom_cloud_transport_metrics_copy(&metrics);
    ESP_LOGI(TAG,
             "PATCH021_HOMEY_PHASE phase=%s attempt=%u elapsed_ms=%u result=%s "
             "error=%s http_status=%d classification=%s cloud_requests=%u "
             "homey_requests=%u homey_init=%u homey_reuse=%u homey_cleanup=%u "
             "session_creates=%u remote_rebinds=%u privacy=sanitized",
             phase != NULL ? phase : "unknown",
             attempt,
             (unsigned)elapsed_ms_since(phase_start_us),
             err == ESP_OK ? "success" : "failure",
             esp_err_to_name(err),
             http_status,
             athom_cloud_transport_class_name(metrics.last_classification),
             (unsigned)metrics.cloud_request_count,
             (unsigned)metrics.homey_request_count,
             (unsigned)metrics.homey_client_init_count,
             (unsigned)metrics.homey_client_reuse_count,
             (unsigned)metrics.homey_client_cleanup_count,
             (unsigned)metrics.homey_session_create_count,
             (unsigned)metrics.remote_rebind_count);
}

static void patch021_homey_remote_log(
    const char *event,
    const char *origin,
    const char *result,
    unsigned attempt,
    uint32_t elapsed_ms,
    uint32_t next_delay_ms,
    esp_err_t err,
    int http_status,
    const char *stage,
    bool transient)
{
    athom_transport_metrics_t metrics;
    athom_cloud_transport_metrics_copy(&metrics);
    ESP_LOGI(TAG,
             "PATCH021_HOMEY_REMOTE event=%s origin=%s result=%s attempt=%u "
             "elapsed_ms=%u next_delay_ms=%u transient=%s error=%s http_status=%d stage=%s "
             "classification=%s cloud_requests=%u homey_requests=%u "
             "homey_init=%u homey_reuse=%u homey_cleanup=%u session_creates=%u "
             "remote_rebinds=%u privacy=sanitized",
             event != NULL ? event : "unknown",
             origin != NULL ? origin : "unknown",
             result != NULL ? result : "unknown",
             attempt,
             (unsigned)elapsed_ms,
             (unsigned)next_delay_ms,
             transient ? "yes" : "no",
             esp_err_to_name(err),
             http_status,
             stage != NULL ? stage : "unknown",
             athom_cloud_transport_class_name(metrics.last_classification),
             (unsigned)metrics.cloud_request_count,
             (unsigned)metrics.homey_request_count,
             (unsigned)metrics.homey_client_init_count,
             (unsigned)metrics.homey_client_reuse_count,
             (unsigned)metrics.homey_client_cleanup_count,
             (unsigned)metrics.homey_session_create_count,
             (unsigned)metrics.remote_rebind_count);
}

static bool pct(const char*in,char*out,size_t cap)
{
    static const char h[]="0123456789ABCDEF";
    size_t o=0;
    for(size_t i=0;in&&in[i];i++){
        unsigned char c=(unsigned char)in[i];
        if(unreserved(c)){if(o+1>=cap)return false;out[o++]=(char)c;}
        else{if(o+3>=cap)return false;out[o++]='%';out[o++]=h[c>>4];out[o++]=h[c&15];}
    }
    if(o>=cap)return false;
    out[o]=0;
    return true;
}

athom_homey_data_state_t athom_oauth_runtime_homey_data_state(void)
{
    return s_homey_data_state;
}

const char *athom_oauth_runtime_homey_data_state_name(void)
{
    switch (s_homey_data_state) {
    case ATHOM_HOMEY_DATA_LOADING: return "loading";
    case ATHOM_HOMEY_DATA_RETRYING: return "retrying";
    case ATHOM_HOMEY_DATA_READY: return "ready";
    case ATHOM_HOMEY_DATA_ERROR: return "error";
    default: return "unknown";
    }
}

athom_light_toggle_dispatch_result_t athom_oauth_runtime_dispatch_light_toggle(
    size_t widget_index,
    bool value)
{
    if (widget_index != 4U && widget_index != 5U) {
        return ATHOM_LIGHT_TOGGLE_DISPATCH_INVALID_WIDGET;
    }

    const bool homey_runtime_ready =
        s_homey_data_state == ATHOM_HOMEY_DATA_READY &&
        phone_provisioning_homey_runtime_ready() &&
        s_cloud.selected_homey.id[0] != '\0' &&
        s_cloud.homey_session_token[0] != '\0';

    /*
     * Patch036 readiness is sampled immediately before the only write
     * primitive call. A false result fails closed and no HTTP request occurs.
     */
    const bool execution_ready =
        panel_homey_favorites_light_toggle_execution_ready(
            widget_index,
            homey_runtime_ready);

    if (!patch037_light_toggle_dispatch_gate(
            widget_index,
            homey_runtime_ready,
            execution_ready)) {
        ESP_LOGI(
            TAG,
            "PATCH037_LIGHT_DISPATCH widget=%u requested=%s result=not_ready "
            "write_attempted=no privacy=sanitized",
            (unsigned)widget_index,
            value ? "true" : "false");
        return ATHOM_LIGHT_TOGGLE_DISPATCH_NOT_READY;
    }

    const athom_homey_light_write_result_t write_result =
        athom_cloud_set_favorite_light_onoff(
            &s_cloud,
            widget_index,
            value);
    const athom_light_toggle_dispatch_result_t result =
        patch037_map_light_write_result(write_result);

    ESP_LOGI(
        TAG,
        "PATCH037_LIGHT_DISPATCH widget=%u requested=%s result=%s "
        "optimistic_state=no state_authority=read_only_refresh privacy=sanitized",
        (unsigned)widget_index,
        value ? "true" : "false",
        patch037_light_toggle_dispatch_result_name(result));

    return result;
}

static const char *patch038_light_toggle_queue_result_name(
    athom_light_toggle_queue_result_t result)
{
    switch (result) {
    case ATHOM_LIGHT_TOGGLE_QUEUE_QUEUED: return "queued";
    case ATHOM_LIGHT_TOGGLE_QUEUE_INVALID_WIDGET: return "invalid_widget";
    case ATHOM_LIGHT_TOGGLE_QUEUE_NOT_READY: return "not_ready";
    case ATHOM_LIGHT_TOGGLE_QUEUE_BUSY: return "busy";
    case ATHOM_LIGHT_TOGGLE_QUEUE_FAILED: return "queue_failed";
    default: return "unknown";
    }
}

athom_light_toggle_queue_result_t athom_oauth_runtime_queue_light_toggle(
    size_t widget_index,
    bool value)
{
    if (widget_index != 4U && widget_index != 5U) {
        return ATHOM_LIGHT_TOGGLE_QUEUE_INVALID_WIDGET;
    }
    if (s_homey_command_queue == NULL) {
        return ATHOM_LIGHT_TOGGLE_QUEUE_NOT_READY;
    }

    const bool homey_runtime_ready =
        s_homey_data_state == ATHOM_HOMEY_DATA_READY &&
        phone_provisioning_homey_runtime_ready() &&
        s_cloud.selected_homey.id[0] != '\0' &&
        s_cloud.homey_session_token[0] != '\0';
    if (!homey_runtime_ready ||
        !panel_homey_favorites_light_toggle_execution_ready(
            widget_index, homey_runtime_ready)) {
        return ATHOM_LIGHT_TOGGLE_QUEUE_NOT_READY;
    }

    if (!network_phase_try_reserve(ATHOM_NETWORK_PHASE_LIGHT_TOGGLE)) {
        return ATHOM_LIGHT_TOGGLE_QUEUE_BUSY;
    }

    bool reserved = false;
    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    if (!patch031_diag_probe_active_locked() &&
        !s_worker_running &&
        !s_select_worker_running &&
        !s_restore_worker_running &&
        !s_preselection_restore_worker_running &&
        !s_schema_refresh_running) {
        portENTER_CRITICAL(&s_refresh_job_mux);
        if (!s_refresh_job_reserved && !s_light_toggle_job_reserved) {
            s_light_toggle_job_reserved = true;
            s_light_toggle_pending_widget = widget_index;
            reserved = true;
        }
        portEXIT_CRITICAL(&s_refresh_job_mux);
    }
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    if (!reserved) {
        network_phase_release(ATHOM_NETWORK_PHASE_LIGHT_TOGGLE);
        return ATHOM_LIGHT_TOGGLE_QUEUE_BUSY;
    }

    const athom_homey_command_t command = {
        .kind = ATHOM_HOMEY_COMMAND_LIGHT_TOGGLE,
        .origin = ATHOM_REFRESH_ORIGIN_MANUAL,
        .widget_index = widget_index,
        .value = value,
    };
    if (xQueueSend(s_homey_command_queue, &command, 0) == pdTRUE) {
        ESP_LOGI(TAG,
                 "PATCH038_LIGHT_QUEUE widget=%u result=%s privacy=sanitized",
                 (unsigned)widget_index,
                 patch038_light_toggle_queue_result_name(
                     ATHOM_LIGHT_TOGGLE_QUEUE_QUEUED));
        return ATHOM_LIGHT_TOGGLE_QUEUE_QUEUED;
    }

    portENTER_CRITICAL(&s_refresh_job_mux);
    s_light_toggle_job_reserved = false;
    s_light_toggle_pending_widget = 0U;
    portEXIT_CRITICAL(&s_refresh_job_mux);
    network_phase_release(ATHOM_NETWORK_PHASE_LIGHT_TOGGLE);
    return ATHOM_LIGHT_TOGGLE_QUEUE_FAILED;
}

bool athom_oauth_runtime_light_toggle_pending(size_t widget_index)
{
    if (widget_index != 4U && widget_index != 5U) return false;
    portENTER_CRITICAL(&s_refresh_job_mux);
    const bool pending = s_light_toggle_job_reserved &&
        s_light_toggle_pending_widget == widget_index;
    portEXIT_CRITICAL(&s_refresh_job_mux);
    return pending;
}

uint32_t athom_oauth_runtime_light_toggle_completion_generation(void)
{
    portENTER_CRITICAL(&s_refresh_job_mux);
    const uint32_t generation = s_light_toggle_completion_generation;
    portEXIT_CRITICAL(&s_refresh_job_mux);
    return generation;
}

esp_err_t athom_oauth_runtime_get_selected_homey_id(char *out, size_t capacity)
{
    if (out == NULL || capacity == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    out[0] = '\0';
    if (s_cloud.selected_homey.id[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }

    if (strlcpy(out, s_cloud.selected_homey.id, capacity) >= capacity) {
        out[0] = '\0';
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

esp_err_t athom_oauth_runtime_on_wifi_online(void)
{
    portENTER_CRITICAL(&s_preselection_restore_mux);
    s_wifi_online = true;
    portEXIT_CRITICAL(&s_preselection_restore_mux);
    maybe_start_preselection_restore_worker();

    if(s_mdns_started)return ESP_OK;
    esp_err_t e=mdns_init();
    if(e!=ESP_OK&&e!=ESP_ERR_INVALID_STATE)return e;
    e=mdns_hostname_set("homey-panel");
    if(e!=ESP_OK)return e;
    (void)mdns_instance_name_set("ESP32 Homey Wall Panel");
    e=mdns_service_add(NULL,"_http","_tcp",80,NULL,0);
    if(e!=ESP_OK&&e!=ESP_ERR_INVALID_STATE)return e;
    s_mdns_started=true;
    ESP_LOGI(TAG,"mDNS ready hostname=homey-panel.local");
    return ESP_OK;
}

static void publish_cloud_state(void)
{
    athom_auth_record_t *record = calloc(1U, sizeof(*record));
    if (record == NULL) {
        ESP_LOGE(TAG, "Homey auth persistence allocation failed");
        return;
    }

    record->tokens = s_cloud.tokens;
    record->selected_homey = s_cloud.selected_homey;
    memcpy(record->homey_session_token, s_cloud.homey_session_token,
           sizeof(record->homey_session_token));
    record->expires_at_s = s_cloud.expires_at_s;
    record->zone_count = s_cloud.zone_count;
    record->device_count = s_cloud.device_count;

    esp_err_t err = athom_auth_store_publish(record);
    zero_secure(record, sizeof(*record));
    free(record);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Homey auth persistence failed: %s",
                 esp_err_to_name(err));
    }
}

static void oauth_worker(void *arg)
{
    (void)arg;
    s_state_name = "token_exchange";
    esp_err_t err = athom_cloud_exchange_code(s_code, &s_cloud);
    zero_secure(s_code, sizeof(s_code));
    if (err == ESP_OK) {
        s_state_name = "fetching_homeys";
        err = athom_cloud_fetch_user_homeys(&s_cloud);
    }
    if (err == ESP_OK) {
        s_state_name = "homey_selection_required";
        publish_cloud_state();
        ESP_LOGI(TAG, "Athom OAuth complete homey_count=%u",
                 (unsigned)s_cloud.homeys.count);
    } else {
        s_state_name = "oauth_error";
        ESP_LOGE(TAG, "Athom OAuth failed: %s", esp_err_to_name(err));
    }
    s_worker_running = false;
    network_phase_release(ATHOM_NETWORK_PHASE_OAUTH);
    vTaskDelete(NULL);
}

static esp_err_t client_config_post(httpd_req_t*r){athom_oauth_client_config_t cur;bool present=false;esp_err_t e=athom_oauth_client_config_load(&cur,&present);zero_secure(&cur,sizeof(cur));if(e!=ESP_OK)return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"config read failed");if(present){httpd_resp_set_status(r,"409 Conflict");return httpd_resp_sendstr(r,"{\"stored\":false}");}if(r->content_len<=0||r->content_len>1024)return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"bad size");char*b=calloc(1,(size_t)r->content_len+1);if(!b)return ESP_ERR_NO_MEM;int n=httpd_req_recv(r,b,r->content_len);if(n!=r->content_len){zero_secure(b,(size_t)r->content_len+1);free(b);return ESP_FAIL;}cJSON*j=cJSON_ParseWithLength(b,n);cJSON*id=j?cJSON_GetObjectItemCaseSensitive(j,"client_id"):NULL;cJSON*sec=j?cJSON_GetObjectItemCaseSensitive(j,"client_secret"):NULL;cJSON*uri=j?cJSON_GetObjectItemCaseSensitive(j,"redirect_uri"):NULL;athom_oauth_client_config_t cfg;memset(&cfg,0,sizeof(cfg));bool ok=cJSON_IsString(id)&&cJSON_IsString(sec)&&cJSON_IsString(uri)&&athom_oauth_client_config_prepare(&cfg,id->valuestring,sec->valuestring,uri->valuestring);if(j)cJSON_Delete(j);zero_secure(b,(size_t)r->content_len+1);free(b);if(!ok){zero_secure(&cfg,sizeof(cfg));return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"invalid config");}e=athom_oauth_client_config_save(&cfg);zero_secure(&cfg,sizeof(cfg));if(e==ESP_ERR_INVALID_STATE){httpd_resp_set_status(r,"409 Conflict");return httpd_resp_sendstr(r,"{\"stored\":false}");}if(e!=ESP_OK)return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"save failed");ESP_LOGI(TAG,"Athom OAuth client configuration stored");s_state_name="login_required";httpd_resp_set_status(r,"201 Created");httpd_resp_set_type(r,"application/json");return httpd_resp_sendstr(r,"{\"stored\":true}");}

static esp_err_t login_get(httpd_req_t *r)
{
    athom_oauth_client_config_t *config =
        calloc(1U, sizeof(*config));
    char *encoded_client_id =
        calloc(1U, ATHOM_CLIENT_ID_MAX * 3U);
    char *encoded_redirect =
        calloc(1U, ATHOM_REDIRECT_URI_MAX * 3U);
    char *url = calloc(1U, 768U);

    if (config == NULL ||
        encoded_client_id == NULL ||
        encoded_redirect == NULL ||
        url == NULL) {
        if (config != NULL) {
            zero_secure(config, sizeof(*config));
            free(config);
        }

        if (encoded_client_id != NULL) {
            zero_secure(
                encoded_client_id,
                ATHOM_CLIENT_ID_MAX * 3U);
            free(encoded_client_id);
        }

        if (encoded_redirect != NULL) {
            zero_secure(
                encoded_redirect,
                ATHOM_REDIRECT_URI_MAX * 3U);
            free(encoded_redirect);
        }

        if (url != NULL) {
            zero_secure(url, 768U);
            free(url);
        }

        return httpd_resp_send_err(
            r,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Minnesallokering misslyckades");
    }

    bool present = false;
    esp_err_t err =
        athom_oauth_client_config_load(config, &present);

    if (err != ESP_OK || !present) {
        httpd_resp_set_status(r, "503 Service Unavailable");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");

        err = httpd_resp_sendstr(
            r,
            "Homey OAuth-klienten är inte provisionerad");

        zero_secure(config, sizeof(*config));
        zero_secure(
            encoded_client_id,
            ATHOM_CLIENT_ID_MAX * 3U);
        zero_secure(
            encoded_redirect,
            ATHOM_REDIRECT_URI_MAX * 3U);
        zero_secure(url, 768U);

        free(config);
        free(encoded_client_id);
        free(encoded_redirect);
        free(url);

        return err;
    }

    uint8_t raw[ATHOM_OAUTH_STATE_BYTES];
    char state[ATHOM_OAUTH_STATE_TEXT_MAX];

    esp_fill_random(raw, sizeof(raw));

    if (athom_oauth_session_begin(
            &s_session,
            now_s(),
            raw) != ATHOM_OAUTH_OK) {
        zero_secure(raw, sizeof(raw));
        zero_secure(state, sizeof(state));
        zero_secure(config, sizeof(*config));
        zero_secure(
            encoded_client_id,
            ATHOM_CLIENT_ID_MAX * 3U);
        zero_secure(
            encoded_redirect,
            ATHOM_REDIRECT_URI_MAX * 3U);
        zero_secure(url, 768U);

        free(config);
        free(encoded_client_id);
        free(encoded_redirect);
        free(url);

        return ESP_FAIL;
    }

    bool ok =
        athom_oauth_state_encode(raw, state) &&
        pct(
            config->client_id,
            encoded_client_id,
            ATHOM_CLIENT_ID_MAX * 3U) &&
        pct(
            config->redirect_uri,
            encoded_redirect,
            ATHOM_REDIRECT_URI_MAX * 3U);

    zero_secure(raw, sizeof(raw));
    zero_secure(config, sizeof(*config));
    free(config);

    if (!ok) {
        zero_secure(state, sizeof(state));
        zero_secure(
            encoded_client_id,
            ATHOM_CLIENT_ID_MAX * 3U);
        zero_secure(
            encoded_redirect,
            ATHOM_REDIRECT_URI_MAX * 3U);
        zero_secure(url, 768U);

        free(encoded_client_id);
        free(encoded_redirect);
        free(url);

        return ESP_ERR_INVALID_SIZE;
    }

    int written = snprintf(
        url,
        768U,
        "https://api.athom.com/oauth2/authorise?"
        "response_type=code&client_id=%s&redirect_uri=%s&state=%s",
        encoded_client_id,
        encoded_redirect,
        state);

    zero_secure(state, sizeof(state));
    zero_secure(
        encoded_client_id,
        ATHOM_CLIENT_ID_MAX * 3U);
    zero_secure(
        encoded_redirect,
        ATHOM_REDIRECT_URI_MAX * 3U);

    free(encoded_client_id);
    free(encoded_redirect);

    if (written <= 0 || written >= 768) {
        zero_secure(url, 768U);
        free(url);
        return ESP_ERR_INVALID_SIZE;
    }

    s_state_name = "awaiting_callback";

    httpd_resp_set_status(r, "302 Found");
    httpd_resp_set_hdr(r, "Location", url);

    err = httpd_resp_send(r, NULL, 0);

    zero_secure(url, 768U);
    free(url);

    return err;
}

static esp_err_t callback_get(httpd_req_t*r)
{
    char q[1024]={0};
    char state_text[ATHOM_OAUTH_STATE_TEXT_MAX]={0};
    char code[ATHOM_OAUTH_CODE_MAX]={0};
    size_t n=httpd_req_get_url_query_len(r);
    if(n==0||n>=sizeof(q)){
        s_last=ATHOM_OAUTH_ERR_ARGUMENT;
        return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Callback saknar parametrar");
    }
    if(httpd_req_get_url_query_str(r,q,sizeof(q))!=ESP_OK)return ESP_FAIL;
    bool has_state=httpd_query_key_value(q,"state",state_text,sizeof(state_text))==ESP_OK;
    bool has_code=httpd_query_key_value(q,"code",code,sizeof(code))==ESP_OK;
    if (!network_phase_try_reserve(ATHOM_NETWORK_PHASE_OAUTH)) {
        zero_secure(q,sizeof(q));
        zero_secure(state_text,sizeof(state_text));
        zero_secure(code,sizeof(code));
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(r, "Ett Homey-jobb pågår redan");
    }
    s_last=athom_oauth_callback_consume(&s_session,now_s(),has_state?state_text:NULL,has_code);
    zero_secure(q,sizeof(q));zero_secure(state_text,sizeof(state_text));
    if(s_last!=ATHOM_OAUTH_OK){
        zero_secure(code,sizeof(code));
        network_phase_release(ATHOM_NETWORK_PHASE_OAUTH);
        return httpd_resp_send_err(r,HTTPD_400_BAD_REQUEST,"Homey-inloggningen kunde inte verifieras");
    }
    bool oauth_reserved = false;
    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    if (!patch031_diag_probe_active_locked() &&
        !s_worker_running &&
        !s_restore_worker_running) {
        s_worker_running = true;
        oauth_reserved = true;
    }
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    if (!oauth_reserved) {
        zero_secure(code, sizeof(code));
        network_phase_release(ATHOM_NETWORK_PHASE_OAUTH);
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(r, "Homey-inloggning pågår redan");
    }
    zero_secure(s_code,sizeof(s_code));
    memcpy(s_code,code,strlen(code)+1U);
    zero_secure(code,sizeof(code));
    if (xTaskCreate(oauth_worker,"athom_oauth",12288,NULL,5,NULL)!=pdPASS) {
        s_worker_running=false;zero_secure(s_code,sizeof(s_code));
        network_phase_release(ATHOM_NETWORK_PHASE_OAUTH);
        return httpd_resp_send_err(r,HTTPD_500_INTERNAL_SERVER_ERROR,"Kunde inte starta Homey-inloggningen");
    }
    httpd_resp_set_type(r,"text/html; charset=utf-8");
    return httpd_resp_sendstr(r,
        "<!doctype html><html lang=sv><meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>Homey</title><body><h1>Inloggningen behandlas</h1>"
        "<p>Återgå till Homey Panel. Dina Homeys hämtas nu.</p></body></html>");
}

typedef struct {
    panel_homey_snapshot_inspection_t snapshot;
    panel_homey_snapshot_publish_inspection_t publish;
    panel_homey_alias_store_diagnostic_t alias_store;
    char awning_json[ATHOM_HOMEY_AWNING_SNAPSHOT_JSON_MAX];
    athom_inventory_attempt_diagnostic_t inventory_attempt;
    char inventory_attempt_json[ATHOM_INVENTORY_ATTEMPT_DIAGNOSTIC_JSON_MAX];
} athom_live_status_diagnostic_workspace_t;

static void athom_inventory_attempt_diagnostic_copy(
    athom_inventory_attempt_diagnostic_t *out);
static bool athom_inventory_attempt_diagnostic_json(
    const athom_inventory_attempt_diagnostic_t *diagnostic,
    char *output,
    size_t output_capacity);

static void athom_live_status_diagnostic_workspace_free(
    athom_live_status_diagnostic_workspace_t *workspace)
{
    if (workspace == NULL) return;
    zero_secure(workspace, sizeof(*workspace));
    free(workspace);
}

static esp_err_t status_get(httpd_req_t *r)
{
    const size_t body_capacity = ATHOM_HOMEY_LIVE_STATUS_JSON_MAX;
    athom_live_status_diagnostic_workspace_t *diagnostics =
        calloc(1U, sizeof(*diagnostics));
    if (diagnostics == NULL) return ESP_ERR_NO_MEM;

    char *body = calloc(1U, body_capacity);
    if (body == NULL) {
        athom_live_status_diagnostic_workspace_free(diagnostics);
        return ESP_ERR_NO_MEM;
    }
    const athom_homey_t *selected =
        s_cloud.selected_homey.id[0] != '\0' ? &s_cloud.selected_homey : NULL;
    bool ok = athom_homey_status_json(
        body, body_capacity, s_state_name, &s_cloud.homeys, selected,
        s_cloud.zone_count, s_cloud.device_count);
    if (!ok) {
        zero_secure(body, body_capacity);
        free(body);
        athom_live_status_diagnostic_workspace_free(diagnostics);
        return ESP_ERR_INVALID_SIZE;
    }

    size_t body_length = strlen(body);

    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000LL);
    (void)athom_cloud_inspect_device_snapshot_with_publish(
        now_ms, &diagnostics->snapshot, &diagnostics->publish);
    const athom_cloud_alias_activation_status_t activation =
        athom_cloud_alias_activation_status();
    athom_inventory_attempt_diagnostic_copy(&diagnostics->inventory_attempt);
    if (!athom_inventory_attempt_diagnostic_json(
            &diagnostics->inventory_attempt,
            diagnostics->inventory_attempt_json,
            sizeof(diagnostics->inventory_attempt_json))) {
        zero_secure(body, body_capacity);
        free(body);
        athom_live_status_diagnostic_workspace_free(diagnostics);
        return ESP_ERR_INVALID_RESPONSE;
    }
    (void)panel_homey_alias_store_inspect(
        selected != NULL ? selected->id : NULL,
        &diagnostics->alias_store);
    if (!athom_homey_awning_snapshot_json(
        diagnostics->awning_json, sizeof(diagnostics->awning_json),
        &diagnostics->snapshot, &diagnostics->publish,
        activation.attempted, activation.result,
        &diagnostics->alias_store)) {
        zero_secure(body, body_capacity);
        free(body);
        athom_live_status_diagnostic_workspace_free(diagnostics);
        return ESP_ERR_INVALID_RESPONSE;
    }
    int diag_written = snprintf(
        body + body_length - 1U,
        body_capacity - body_length + 1U,
        ",\"awning_snapshot\":%s}", diagnostics->awning_json);
    if (diag_written <= 0 ||
        (size_t)diag_written >= body_capacity - body_length + 1U) {
        zero_secure(body, body_capacity);
        free(body);
        athom_live_status_diagnostic_workspace_free(diagnostics);
        return ESP_ERR_INVALID_SIZE;
    }
    body_length = body_length - 1U + (size_t)diag_written;
    diag_written = snprintf(
        body + body_length - 1U,
        body_capacity - body_length + 1U,
        ",\"last_inventory_attempt_transport\":%s}",
        diagnostics->inventory_attempt_json);
    if (diag_written <= 0 ||
        (size_t)diag_written >= body_capacity - body_length + 1U) {
        zero_secure(body, body_capacity);
        free(body);
        athom_live_status_diagnostic_workspace_free(diagnostics);
        return ESP_ERR_INVALID_SIZE;
    }
    body_length = body_length - 1U + (size_t)diag_written;
    athom_live_status_diagnostic_workspace_free(diagnostics);

    if (body_length == 0U || body[body_length - 1U] != 125) {
        zero_secure(body, body_capacity);
        free(body);
        return ESP_ERR_INVALID_RESPONSE;
    }

    int appended = snprintf(
        body + body_length - 1U,
        body_capacity - body_length + 1U,
        ",\"detail\":\"%s\","
        "\"last_error\":%d,"
        "\"http_status\":%d,"
        "\"runtime_id\":%u,"
        "\"select_attempt\":%u,"
        "\"homey_data_state\":\"%s\"}",
        athom_cloud_diagnostic_stage(),
        (int)athom_cloud_diagnostic_error(),
        athom_cloud_diagnostic_http_status(),
        (unsigned)s_runtime_id,
        (unsigned)s_select_attempt,
        athom_oauth_runtime_homey_data_state_name());

    if (appended <= 0 ||
        (size_t)appended >= body_capacity - body_length + 1U) {
        zero_secure(body, body_capacity);
        free(body);
        return ESP_ERR_INVALID_SIZE;
    }

    httpd_resp_set_type(r,"application/json; charset=utf-8");
    esp_err_t err=httpd_resp_sendstr(r,body);
    zero_secure(body,body_capacity);free(body);return err;
}

static bool runtime_diag_parse_query(
    httpd_req_t *request,
    uint32_t *limit_out,
    uint64_t *before_sequence_out)
{
    const size_t query_length = httpd_req_get_url_query_len(request);
    if (query_length == 0U) {
        return runtime_diag_journal_parse_query(NULL, 0U, limit_out,
                                               before_sequence_out);
    }
    if (query_length >= 96U) return false;
    char query[96];
    if (httpd_req_get_url_query_str(request, query, sizeof(query)) != ESP_OK) return false;
    query[query_length] = '\0';
    return runtime_diag_journal_parse_query(query, query_length,
                                            limit_out, before_sequence_out);
}

static esp_err_t runtime_diag_journal_get(httpd_req_t *request)
{
    uint32_t limit = 0U;
    uint64_t before_sequence = 0U;
    if (!runtime_diag_parse_query(request, &limit, &before_sequence)) {
        httpd_resp_set_status(request, "400 Bad Request");
        httpd_resp_set_type(request, "application/json");
        return httpd_resp_sendstr(request, "{\"error\":\"invalid_query\"}");
    }

    runtime_diag_record_t *records = calloc(limit, sizeof(*records));
    if (records == NULL) {
        httpd_resp_set_status(request, "503 Service Unavailable");
        return httpd_resp_sendstr(request, "{\"available\":false}");
    }
    size_t record_count = 0U;
    runtime_diag_journal_info_t info = {0};
    const esp_err_t read_result = runtime_diag_journal_get_page(
        limit, before_sequence, records, limit, &record_count, &info);
    if (read_result != ESP_OK) {
        free(records);
        httpd_resp_set_status(request, "503 Service Unavailable");
        httpd_resp_set_type(request, "application/json");
        return httpd_resp_sendstr(request, "{\"available\":false}");
    }

    httpd_resp_set_type(request, "application/json; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    char chunk[384];
    int written = snprintf(chunk, sizeof(chunk),
        "{\"format_version\":1,\"capacity\":%u,\"journal_full\":%s,"
        "\"free_slots\":%u,\"valid_record_count\":%u,"
        "\"oldest_sequence\":%llu,\"newest_sequence\":%llu,"
        "\"volatile_queue_drop_count\":%u,\"returned_count\":%u,\"records\":[",
        (unsigned)info.capacity, info.journal_full ? "true" : "false",
        (unsigned)info.free_slots, (unsigned)info.valid_record_count,
        (unsigned long long)info.oldest_sequence,
        (unsigned long long)info.newest_sequence,
        (unsigned)info.volatile_queue_drop_count, (unsigned)record_count);
    esp_err_t err = written > 0 && (size_t)written < sizeof(chunk)
        ? httpd_resp_send_chunk(request, chunk, written) : ESP_ERR_INVALID_SIZE;

    for (size_t i = 0U; err == ESP_OK && i < record_count; ++i) {
        const runtime_diag_record_t *record = &records[i];
        written = snprintf(chunk, sizeof(chunk),
            "%s{\"sequence\":%llu,\"boot_sequence\":%u,\"monotonic_ms\":%llu,"
            "\"event\":\"%s\",\"source\":%u,\"origin\":%u,\"attempt\":%u,"
            "\"result\":%u,\"error_code\":%ld,\"http_status\":%u,"
            "\"transport\":%u,\"stage\":%u,\"retry_delay_ms\":%u,"
            "\"snapshot_generation\":%u,\"reset_reason\":\"%s\",\"drop_count\":%u}",
            i == 0U ? "" : ",",
            (unsigned long long)record->sequence,
            (unsigned)record->boot_sequence,
            (unsigned long long)record->monotonic_ms,
            runtime_diag_event_name(record->event_type),
            (unsigned)record->source, (unsigned)record->origin,
            (unsigned)record->attempt, (unsigned)record->result,
            (long)record->error_code, (unsigned)record->http_status,
            (unsigned)record->transport, (unsigned)record->stage,
            (unsigned)record->retry_delay_ms,
            (unsigned)record->snapshot_generation,
            runtime_diag_reset_reason_name(record->reset_reason),
            (unsigned)record->drop_count);
        err = written > 0 && (size_t)written < sizeof(chunk)
            ? httpd_resp_send_chunk(request, chunk, written) : ESP_ERR_INVALID_SIZE;
    }
    if (err == ESP_OK) err = httpd_resp_send_chunk(request, "]}", 2U);
    if (err == ESP_OK) err = httpd_resp_send_chunk(request, NULL, 0U);
    free(records);
    return err;
}

static esp_err_t patch031_diag_cloud_user_me_probe_post(httpd_req_t *r)
{
    if (r == NULL) return ESP_ERR_INVALID_ARG;
    if (r->content_len != 0) {
        httpd_resp_set_status(r, "400 Bad Request");
        httpd_resp_set_type(r, "application/json");
        return httpd_resp_sendstr(r, "{\"accepted\":false,\"reason\":\"body_not_allowed\"}");
    }

    if (!network_phase_try_reserve(ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC)) {
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "application/json");
        return httpd_resp_sendstr(r, "{\"accepted\":false,\"reason\":\"busy\"}");
    }

    bool reserved = false;
    bool busy = false;
    bool no_access_token = false;
    bool consumed = false;

    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    consumed = s_patch031_diag_probe_state != PATCH031_DIAG_PROBE_UNUSED;
    busy =
        !s_restore_started ||
        s_worker_running ||
        s_select_worker_running ||
        s_restore_worker_running ||
        s_preselection_restore_worker_running ||
        s_schema_refresh_running ||
        s_refresh_job_reserved ||
        s_light_toggle_job_reserved ||
        s_patch031_live_refresh_running;
    no_access_token = s_cloud.tokens.access_token[0] == '\0';

    if (!consumed && !busy && !no_access_token) {
        memset(&s_patch031_diag_probe_result, 0, sizeof(s_patch031_diag_probe_result));
        s_patch031_diag_probe_state = PATCH031_DIAG_PROBE_RESERVED;
        reserved = true;
    }
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    if (consumed) {
        network_phase_release(ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC);
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "application/json");
        return httpd_resp_sendstr(r, "{\"accepted\":false,\"reason\":\"one_shot_consumed\"}");
    }
    if (busy) {
        network_phase_release(ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC);
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "application/json");
        return httpd_resp_sendstr(r, "{\"accepted\":false,\"reason\":\"busy\"}");
    }
    if (no_access_token) {
        network_phase_release(ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC);
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "application/json");
        return httpd_resp_sendstr(r, "{\"accepted\":false,\"reason\":\"no_access_token\"}");
    }
    if (!reserved) {
        network_phase_release(ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC);
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "application/json");
        return httpd_resp_sendstr(r, "{\"accepted\":false,\"reason\":\"reservation_failed\"}");
    }

    BaseType_t created = xTaskCreate(
        patch031_diag_cloud_probe_worker,
        "patch031_diag_probe",
        PATCH031_DIAG_PROBE_WORKER_STACK,
        NULL,
        PATCH031_DIAG_PROBE_WORKER_PRIORITY,
        NULL);

    if (created != pdPASS) {
        portENTER_CRITICAL(&s_patch031_diag_probe_mux);
        s_patch031_diag_probe_state = PATCH031_DIAG_PROBE_TASK_CREATE_FAILED;
        portEXIT_CRITICAL(&s_patch031_diag_probe_mux);
        network_phase_release(ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC);

        httpd_resp_set_status(r, "503 Service Unavailable");
        httpd_resp_set_type(r, "application/json");
        return httpd_resp_sendstr(r, "{\"accepted\":false,\"reason\":\"worker_create_failed\"}");
    }

    httpd_resp_set_status(r, "202 Accepted");
    httpd_resp_set_type(r, "application/json");
    return httpd_resp_sendstr(r, "{\"accepted\":true,\"state\":\"running\"}");
}

static esp_err_t patch031_diag_cloud_user_me_probe_result_get(httpd_req_t *r)
{
    if (r == NULL) return ESP_ERR_INVALID_ARG;

    patch031_diag_probe_state_t state;
    athom_cloud_debug_probe_result_t result;

    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    state = s_patch031_diag_probe_state;
    result = s_patch031_diag_probe_result;
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    char json[512];
    int n = snprintf(
        json,
        sizeof(json),
        "{\"state\":\"%s\",\"executed\":%s,"
        "\"perform_err\":\"%s\",\"fresh_http_status\":%d,"
        "\"transport_response_received\":%s,\"classification\":\"%s\","
        "\"tls_error\":%d,\"socket_errno\":%d,\"elapsed_ms\":%u,"
        "\"cloud_init\":%u,\"cloud_reuse\":%u,\"cloud_cleanup\":%u}",
        patch031_diag_probe_state_name(state),
        result.executed ? "true" : "false",
        esp_err_to_name(result.perform_err),
        result.fresh_http_status,
        result.transport_response_received ? "true" : "false",
        athom_cloud_transport_class_name(result.classification),
        result.tls_error,
        result.socket_errno,
        (unsigned)result.elapsed_ms,
        (unsigned)result.cloud_client_init_count,
        (unsigned)result.cloud_client_reuse_count,
        (unsigned)result.cloud_client_cleanup_count);

    if (n <= 0 || (size_t)n >= sizeof(json)) {
        return ESP_ERR_INVALID_SIZE;
    }
    httpd_resp_set_type(r, "application/json");
    return httpd_resp_send(r, json, n);
}

typedef struct {
    char homey_id[ATHOM_HOMEY_ID_MAX];
} athom_select_work_t;

static esp_err_t connect_and_fetch_inventory(const char *homey_id)
{
    char selected_homey_id[ATHOM_HOMEY_ID_MAX] = {0};
    size_t selected_homey_id_length = strnlen(homey_id, sizeof(selected_homey_id));
    if (selected_homey_id_length == 0U || selected_homey_id_length >= sizeof(selected_homey_id)) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(selected_homey_id, homey_id, selected_homey_id_length + 1U);

    int64_t phase_start_us = esp_timer_get_time();
    ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=homeys_fetch_begin attempt=1");
    esp_err_t err = athom_cloud_fetch_user_homeys(&s_cloud);
    const int first_homeys_http_status = athom_cloud_diagnostic_http_status();
    ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=homeys_fetch_end attempt=1 http_status=%d result=%s error=%s",
             first_homeys_http_status,
             err == ESP_OK ? "success" : "failure",
             esp_err_to_name(err));
    patch021_homey_phase_log(
        "homeys_fetch", 1U, phase_start_us, err, first_homeys_http_status);

    if (first_homeys_http_status == 401) {
        phase_start_us = esp_timer_get_time();
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=token_refresh_begin");
        err = athom_cloud_refresh(&s_cloud);
        const int refresh_http_status = athom_cloud_diagnostic_http_status();
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=token_refresh_end result=%s error=%s",
                 err == ESP_OK ? "success" : "failure", esp_err_to_name(err));
        patch021_homey_phase_log(
            "token_refresh", 1U, phase_start_us, err, refresh_http_status);
        if (err == ESP_OK) {
            phase_start_us = esp_timer_get_time();
            ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=homeys_fetch_begin attempt=2");
            err = athom_cloud_fetch_user_homeys(&s_cloud);
            const int second_homeys_http_status = athom_cloud_diagnostic_http_status();
            ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=homeys_fetch_end attempt=2 http_status=%d result=%s error=%s",
                     second_homeys_http_status,
                     err == ESP_OK ? "success" : "failure",
                     esp_err_to_name(err));
            patch021_homey_phase_log(
                "homeys_fetch", 2U, phase_start_us, err, second_homeys_http_status);
        }
    }

    if (err == ESP_OK) {
        phase_start_us = esp_timer_get_time();
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=connect_begin");
        err = athom_cloud_select_and_connect(&s_cloud, selected_homey_id);
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=connect_end result=%s error=%s",
                 err == ESP_OK ? "success" : "failure", esp_err_to_name(err));
        patch021_homey_phase_log(
            "homey_connect", 1U, phase_start_us, err,
            athom_cloud_diagnostic_http_status());
    }
    if (err == ESP_OK) {
        phase_start_us = esp_timer_get_time();
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=inventory_begin");
        err = athom_cloud_fetch_inventory(&s_cloud);
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=inventory_end result=%s error=%s",
                 err == ESP_OK ? "success" : "failure", esp_err_to_name(err));
        patch021_homey_phase_log(
            "inventory_fetch", 1U, phase_start_us, err,
            athom_cloud_diagnostic_http_status());
    }
    zero_secure(selected_homey_id, sizeof(selected_homey_id));
    return err;
}

static void select_worker(void *arg)
{
    athom_select_work_t *work = (athom_select_work_t *)arg;

    /*
     * Give the HTTP task time to transmit its 202 response before the
     * network-heavy Homey connection work starts.
     */
    vTaskDelay(pdMS_TO_TICKS(50));

    const int64_t select_start_us = esp_timer_get_time();
    esp_err_t err = connect_and_fetch_inventory(work->homey_id);
    patch021_homey_remote_log(
        "select_end",
        "manual_select",
        err == ESP_OK ? "success" : "failure",
        s_select_attempt,
        elapsed_ms_since(select_start_us),
        0U,
        err,
        athom_cloud_diagnostic_http_status(),
        athom_cloud_diagnostic_stage(),
        false);

    zero_secure(work, sizeof(*work));
    free(work);

    if (err == ESP_OK) {
        s_state_name = "ready";
        publish_cloud_state();

        phone_provisioning_show_live_ready(
            s_cloud.selected_homey.name);

        ESP_LOGI(
            TAG,
            "Homey connection complete zones=%u devices=%u",
            (unsigned)s_cloud.zone_count,
            (unsigned)s_cloud.device_count);
    } else {
        s_state_name = "homey_connection_error";

        ESP_LOGE(
            TAG,
            "Homey connection failed: %s",
            esp_err_to_name(err));
    }

    s_select_worker_running = false;
    network_phase_release(ATHOM_NETWORK_PHASE_HOMEY_SELECT);
    vTaskDelete(NULL);
}

static esp_err_t select_post(httpd_req_t *r)
{
    char body[256] = {0};

    if (r->content_len <= 0 ||
        r->content_len >= (int)sizeof(body)) {
        return httpd_resp_send_err(
            r,
            HTTPD_400_BAD_REQUEST,
            "homey_id saknas");
    }

    int received = httpd_req_recv(r, body, r->content_len);

    if (received != r->content_len) {
        zero_secure(body, sizeof(body));
        return ESP_FAIL;
    }

    body[received] = 0;

    athom_select_work_t *work = calloc(1U, sizeof(*work));

    if (work == NULL) {
        zero_secure(body, sizeof(body));
        return httpd_resp_send_err(
            r,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Minnesallokering misslyckades");
    }

    esp_err_t parse_err = httpd_query_key_value(
        body,
        "homey_id",
        work->homey_id,
        sizeof(work->homey_id));

    zero_secure(body, sizeof(body));

    if (parse_err != ESP_OK || work->homey_id[0] == 0) {
        zero_secure(work, sizeof(*work));
        free(work);

        return httpd_resp_send_err(
            r,
            HTTPD_400_BAD_REQUEST,
            "homey_id saknas");
    }

    if (!network_phase_try_reserve(ATHOM_NETWORK_PHASE_HOMEY_SELECT)) {
        zero_secure(work, sizeof(*work));
        free(work);
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(r, "Ett Homey-jobb pågår redan");
    }

    bool select_reserved = false;
    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    if (!patch031_diag_probe_active_locked() &&
        !s_worker_running &&
        !s_select_worker_running &&
        !s_restore_worker_running &&
        !s_light_toggle_job_reserved) {
        s_select_worker_running = true;
        select_reserved = true;
    }
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    if (!select_reserved) {
        zero_secure(work, sizeof(*work));
        free(work);
        network_phase_release(ATHOM_NETWORK_PHASE_HOMEY_SELECT);

        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");

        return httpd_resp_sendstr(
            r,
            "Ett Homey-jobb pågår redan");
    }

    s_select_attempt++;
    s_state_name = "connecting_homey";

    if (xTaskCreate(
            select_worker,
            "athom_select",
            24576,
            work,
            5,
            NULL) != pdPASS) {
        s_select_worker_running = false;
        s_state_name = "homey_selection_required";

        zero_secure(work, sizeof(*work));
        free(work);
        network_phase_release(ATHOM_NETWORK_PHASE_HOMEY_SELECT);

        return httpd_resp_send_err(
            r,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Kunde inte starta Homey-anslutningen");
    }

    httpd_resp_set_status(r, "202 Accepted");
    httpd_resp_set_type(r, "application/json; charset=utf-8");
    httpd_resp_set_hdr(r, "Cache-Control", "no-store");

    return httpd_resp_sendstr(
        r,
        "{\"accepted\":true,\"state\":\"connecting_homey\"}");
}

static bool homey_inventory_result_verified(
    esp_err_t transport_result,
    esp_err_t *effective_error_out,
    int *http_status_out,
    const char **stage_out)
{
    const char *stage = athom_cloud_diagnostic_stage();
    const int http_status = athom_cloud_diagnostic_http_status();
    esp_err_t effective_error = transport_result;

    if (transport_result == ESP_OK &&
        stage != NULL && strcmp(stage, "inventory_complete") == 0) {
        effective_error = ESP_OK;
    } else if (transport_result == ESP_OK) {
        effective_error = athom_cloud_diagnostic_error();
        if (effective_error == ESP_OK) effective_error = ESP_FAIL;
    }

    if (effective_error_out != NULL) *effective_error_out = effective_error;
    if (http_status_out != NULL) *http_status_out = http_status;
    if (stage_out != NULL) *stage_out = stage != NULL ? stage : "unknown";
    return effective_error == ESP_OK;
}

static bool homey_data_failure_is_transient(esp_err_t error, int http_status)
{
    if (error == ESP_ERR_HTTP_CONNECT || error == ESP_ERR_TIMEOUT) return true;
    if (http_status == 408 || http_status == 429) return true;
    return http_status >= 500 && http_status <= 599;
}

static uint32_t homey_data_retry_delay_ms(unsigned failed_attempt)
{
    if (failed_attempt == 1U) return ATHOM_HOMEY_DATA_RETRY_1_MS;
    if (failed_attempt == 2U) return ATHOM_HOMEY_DATA_RETRY_2_MS;
    if (failed_attempt == 3U) return ATHOM_HOMEY_DATA_RETRY_3_MS;
    return ATHOM_HOMEY_DATA_RETRY_MAX_MS;
}

static bool inventory_refresh_worker_should_retry(
    athom_refresh_origin_t origin,
    bool transient)
{
    return origin == ATHOM_REFRESH_ORIGIN_BOOT_AUTO && transient;
}

typedef struct {
    bool snapshot_seen;
    uint64_t snapshot_captured_at_ms;
    uint64_t next_attempt_ms;
} periodic_refresh_scheduler_state_t;

static bool periodic_refresh_scheduler_should_attempt(
    periodic_refresh_scheduler_state_t *state,
    bool snapshot_present,
    uint64_t snapshot_captured_at_ms,
    uint64_t snapshot_age_ms,
    uint64_t now_ms)
{
    if (state == NULL || !snapshot_present) return false;

    if (!state->snapshot_seen ||
        state->snapshot_captured_at_ms != snapshot_captured_at_ms) {
        state->snapshot_seen = true;
        state->snapshot_captured_at_ms = snapshot_captured_at_ms;
        state->next_attempt_ms = 0U;
    }

    return snapshot_age_ms >= PERIODIC_REFRESH_INTERVAL_MS &&
        now_ms >= state->next_attempt_ms;
}

static uint64_t periodic_refresh_scheduler_defer_ms(
    athom_refresh_queue_result_t result)
{
    if (result == ATHOM_REFRESH_QUEUE_BUSY ||
        result == ATHOM_REFRESH_QUEUE_NOT_READY) {
        return PERIODIC_REFRESH_BUSY_DEFER_MS;
    }
    return PERIODIC_REFRESH_COOLDOWN_MS;
}

static void periodic_refresh_scheduler_record_queue_result(
    periodic_refresh_scheduler_state_t *state,
    athom_refresh_queue_result_t result,
    uint64_t now_ms)
{
    if (state == NULL) return;
    const uint64_t defer_ms = periodic_refresh_scheduler_defer_ms(result);
    state->next_attempt_ms = UINT64_MAX - now_ms < defer_ms
        ? UINT64_MAX : now_ms + defer_ms;
}

static const char *inventory_refresh_origin_name(
    athom_refresh_origin_t origin)
{
    switch (origin) {
    case ATHOM_REFRESH_ORIGIN_BOOT_AUTO: return "boot_auto";
    case ATHOM_REFRESH_ORIGIN_MANUAL: return "manual";
    case ATHOM_REFRESH_ORIGIN_PERIODIC: return "periodic";
    case ATHOM_REFRESH_ORIGIN_LIGHT_RECONCILIATION: return "light_reconciliation";
    default: return "unknown";
    }
}

static const char *athom_inventory_attempt_stage_name(
    athom_inventory_attempt_stage_t stage)
{
    switch (stage) {
    case ATHOM_INVENTORY_STAGE_CLOUD_USER_DISCOVERY: return "cloud_user_discovery";
    case ATHOM_INVENTORY_STAGE_CLOUD_TOKEN: return "cloud_token";
    case ATHOM_INVENTORY_STAGE_DELEGATION: return "delegation";
    case ATHOM_INVENTORY_STAGE_HOMEY_REMOTE_LOGIN: return "homey_remote_login";
    case ATHOM_INVENTORY_STAGE_PARSE: return "parse";
    case ATHOM_INVENTORY_STAGE_FAVORITES: return "favorites";
    case ATHOM_INVENTORY_STAGE_ZONES: return "zones";
    case ATHOM_INVENTORY_STAGE_DEVICES: return "devices";
    case ATHOM_INVENTORY_STAGE_INVENTORY_COMPLETE: return "inventory_complete";
    case ATHOM_INVENTORY_STAGE_HANDOFF: return "handoff";
    case ATHOM_INVENTORY_STAGE_OTHER: return "other";
    case ATHOM_INVENTORY_STAGE_UNKNOWN:
    default: return "unknown";
    }
}

static athom_inventory_attempt_stage_t athom_inventory_attempt_stage_classify(
    const char *stage)
{
    if (stage == NULL) return ATHOM_INVENTORY_STAGE_UNKNOWN;
    if (strcmp(stage, "oauth_user_me_request") == 0 ||
        strcmp(stage, "oauth_user_me_http") == 0 ||
        strcmp(stage, "oauth_complete") == 0) {
        return ATHOM_INVENTORY_STAGE_CLOUD_USER_DISCOVERY;
    }
    if (strcmp(stage, "oauth_token_request") == 0 ||
        strcmp(stage, "oauth_token_http") == 0) {
        return ATHOM_INVENTORY_STAGE_CLOUD_TOKEN;
    }
    if (strcmp(stage, "delegation_request") == 0 ||
        strcmp(stage, "delegation_http") == 0) {
        return ATHOM_INVENTORY_STAGE_DELEGATION;
    }
    if (strcmp(stage, "homey_login_remote") == 0 ||
        strcmp(stage, "homey_login_request") == 0 ||
        strcmp(stage, "homey_login_http") == 0 ||
        strcmp(stage, "session_ready") == 0) {
        return ATHOM_INVENTORY_STAGE_HOMEY_REMOTE_LOGIN;
    }
    if (strcmp(stage, "oauth_token_parse") == 0 ||
        strcmp(stage, "oauth_user_me_parse") == 0 ||
        strcmp(stage, "oauth_homey_parse") == 0 ||
        strcmp(stage, "delegation_parse") == 0 ||
        strcmp(stage, "homey_login_parse") == 0) {
        return ATHOM_INVENTORY_STAGE_PARSE;
    }
    if (strcmp(stage, "favorites_user_me") == 0 ||
        strcmp(stage, "favorites_user_me_blocked_by_scope") == 0 ||
        strcmp(stage, "favorites_user_me_unavailable") == 0) {
        return ATHOM_INVENTORY_STAGE_FAVORITES;
    }
    if (strcmp(stage, "inventory_zones") == 0) return ATHOM_INVENTORY_STAGE_ZONES;
    if (strcmp(stage, "inventory_devices") == 0) return ATHOM_INVENTORY_STAGE_DEVICES;
    if (strcmp(stage, "inventory_complete") == 0) {
        return ATHOM_INVENTORY_STAGE_INVENTORY_COMPLETE;
    }
    if (strcmp(stage, "homey_to_cloud_handoff") == 0 ||
        strcmp(stage, "cloud_to_homey_handoff") == 0) {
        return ATHOM_INVENTORY_STAGE_HANDOFF;
    }
    if (strcmp(stage, "argument_validation") == 0 ||
        strcmp(stage, "homey_lookup") == 0 ||
        strcmp(stage, "url_selection") == 0 ||
        strcmp(stage, "inventory_remote") == 0) {
        return ATHOM_INVENTORY_STAGE_OTHER;
    }
    return ATHOM_INVENTORY_STAGE_UNKNOWN;
}

static const char *athom_inventory_attempt_role_name(
    athom_transport_role_t role)
{
    switch (role) {
    case ATHOM_TRANSPORT_ROLE_CLOUD: return "cloud";
    case ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE: return "homey_remote";
    case ATHOM_TRANSPORT_ROLE_NONE:
    default: return "none";
    }
}

static uint32_t athom_inventory_attempt_counter_delta(
    uint32_t before,
    uint32_t after)
{
    if (after >= before) return after - before;
    if (before >= UINT32_MAX - 65535U && after <= 65535U) {
        return after - before;
    }
    return 0U;
}

static athom_inventory_attempt_diagnostic_t athom_inventory_attempt_build(
    athom_refresh_origin_t origin,
    uint32_t attempt,
    uint64_t completed_at_ms,
    esp_err_t final_error,
    int final_http_status,
    const char *stage,
    const athom_transport_metrics_t *before,
    const athom_transport_metrics_t *after)
{
    athom_inventory_attempt_diagnostic_t diagnostic = {
        .valid = before != NULL && after != NULL,
        .origin = origin,
        .attempt = attempt,
        .completed_at_ms = completed_at_ms,
        .final_error = (int32_t)final_error,
        .final_http_status = final_http_status,
        .stage = athom_inventory_attempt_stage_classify(stage),
    };
    if (!diagnostic.valid) return diagnostic;

    diagnostic.deltas.cloud_request_count = athom_inventory_attempt_counter_delta(
        before->cloud_request_count, after->cloud_request_count);
    diagnostic.deltas.homey_request_count = athom_inventory_attempt_counter_delta(
        before->homey_request_count, after->homey_request_count);
    diagnostic.deltas.cloud_client_init_count = athom_inventory_attempt_counter_delta(
        before->cloud_client_init_count, after->cloud_client_init_count);
    diagnostic.deltas.cloud_client_reuse_count = athom_inventory_attempt_counter_delta(
        before->cloud_client_reuse_count, after->cloud_client_reuse_count);
    diagnostic.deltas.cloud_client_cleanup_count = athom_inventory_attempt_counter_delta(
        before->cloud_client_cleanup_count, after->cloud_client_cleanup_count);
    diagnostic.deltas.homey_client_init_count = athom_inventory_attempt_counter_delta(
        before->homey_client_init_count, after->homey_client_init_count);
    diagnostic.deltas.homey_client_reuse_count = athom_inventory_attempt_counter_delta(
        before->homey_client_reuse_count, after->homey_client_reuse_count);
    diagnostic.deltas.homey_client_cleanup_count = athom_inventory_attempt_counter_delta(
        before->homey_client_cleanup_count, after->homey_client_cleanup_count);
    diagnostic.deltas.homey_session_create_count = athom_inventory_attempt_counter_delta(
        before->homey_session_create_count, after->homey_session_create_count);
    diagnostic.deltas.remote_rebind_count = athom_inventory_attempt_counter_delta(
        before->remote_rebind_count, after->remote_rebind_count);

    diagnostic.raw_transport_observed = athom_inventory_attempt_counter_delta(
        before->perform_count, after->perform_count) != 0U;
    if (diagnostic.raw_transport_observed) {
        diagnostic.raw_role = after->last_perform_role;
        diagnostic.raw_perform_error = (int32_t)after->last_perform_err;
        diagnostic.raw_classification = after->last_perform_classification;
        diagnostic.raw_http_status = after->last_perform_http_status;
        diagnostic.raw_tls_query = (int32_t)after->last_tls_query;
        diagnostic.raw_tls_error = after->last_tls_error;
        diagnostic.raw_tls_flags = after->last_tls_flags;
        diagnostic.raw_socket_errno = after->last_socket_errno;
        diagnostic.raw_request_elapsed_ms = after->last_request_elapsed_ms;
    }
    return diagnostic;
}

static void athom_inventory_attempt_diagnostic_publish(
    const athom_inventory_attempt_diagnostic_t *diagnostic)
{
    if (diagnostic == NULL || !diagnostic->valid) return;
    portENTER_CRITICAL(&s_inventory_attempt_diagnostic_mux);
    uint32_t sequence = s_last_inventory_attempt_diagnostic.sequence + 1U;
    if (sequence == 0U) sequence = 1U;
    s_last_inventory_attempt_diagnostic = *diagnostic;
    s_last_inventory_attempt_diagnostic.sequence = sequence;
    portEXIT_CRITICAL(&s_inventory_attempt_diagnostic_mux);
}

static void athom_inventory_attempt_diagnostic_copy(
    athom_inventory_attempt_diagnostic_t *out)
{
    if (out == NULL) return;
    portENTER_CRITICAL(&s_inventory_attempt_diagnostic_mux);
    *out = s_last_inventory_attempt_diagnostic;
    portEXIT_CRITICAL(&s_inventory_attempt_diagnostic_mux);
}

static bool athom_inventory_attempt_diagnostic_json(
    const athom_inventory_attempt_diagnostic_t *diagnostic,
    char *output,
    size_t output_capacity)
{
    if (diagnostic == NULL || output == NULL || output_capacity == 0U) return false;
    if (!diagnostic->valid) {
        const int n = snprintf(output, output_capacity, "{\"valid\":false}");
        return n > 0 && (size_t)n < output_capacity;
    }

    char raw_transport[512] = "null";
    if (diagnostic->raw_transport_observed) {
        const int raw_n = snprintf(
            raw_transport, sizeof(raw_transport),
            "{\"role\":\"%s\",\"perform_err\":%d,"
            "\"classification\":\"%s\",\"http_status\":%d,"
            "\"tls_query\":%d,\"tls_error\":%d,\"tls_flags\":%d,"
            "\"socket_errno\":%d,\"request_elapsed_ms\":%u}",
            athom_inventory_attempt_role_name(diagnostic->raw_role),
            (int)diagnostic->raw_perform_error,
            athom_cloud_transport_class_name(diagnostic->raw_classification),
            (int)diagnostic->raw_http_status,
            (int)diagnostic->raw_tls_query,
            (int)diagnostic->raw_tls_error,
            (int)diagnostic->raw_tls_flags,
            (int)diagnostic->raw_socket_errno,
            (unsigned)diagnostic->raw_request_elapsed_ms);
        if (raw_n <= 0 || (size_t)raw_n >= sizeof(raw_transport)) return false;
    }

    const int n = snprintf(
        output, output_capacity,
        "{\"valid\":true,\"sequence\":%u,\"origin\":\"%s\","
        "\"attempt\":%u,\"completed\":true,\"completed_at_ms\":%llu,"
        "\"final_error\":%d,\"final_http_status\":%d,\"stage\":\"%s\","
        "\"raw_transport_observed\":%s,\"raw_transport\":%s,"
        "\"counter_deltas\":{\"cloud_request_count\":%u,"
        "\"homey_request_count\":%u,\"cloud_client_init_count\":%u,"
        "\"cloud_client_reuse_count\":%u,\"cloud_client_cleanup_count\":%u,"
        "\"homey_client_init_count\":%u,\"homey_client_reuse_count\":%u,"
        "\"homey_client_cleanup_count\":%u,\"homey_session_create_count\":%u,"
        "\"remote_rebind_count\":%u}}",
        (unsigned)diagnostic->sequence,
        inventory_refresh_origin_name(diagnostic->origin),
        (unsigned)diagnostic->attempt,
        (unsigned long long)diagnostic->completed_at_ms,
        (int)diagnostic->final_error,
        (int)diagnostic->final_http_status,
        athom_inventory_attempt_stage_name(diagnostic->stage),
        diagnostic->raw_transport_observed ? "true" : "false",
        raw_transport,
        (unsigned)diagnostic->deltas.cloud_request_count,
        (unsigned)diagnostic->deltas.homey_request_count,
        (unsigned)diagnostic->deltas.cloud_client_init_count,
        (unsigned)diagnostic->deltas.cloud_client_reuse_count,
        (unsigned)diagnostic->deltas.cloud_client_cleanup_count,
        (unsigned)diagnostic->deltas.homey_client_init_count,
        (unsigned)diagnostic->deltas.homey_client_reuse_count,
        (unsigned)diagnostic->deltas.homey_client_cleanup_count,
        (unsigned)diagnostic->deltas.homey_session_create_count,
        (unsigned)diagnostic->deltas.remote_rebind_count);
    return n > 0 && (size_t)n < output_capacity;
}

static void athom_inventory_attempt_diagnostic_begin(
    athom_transport_metrics_t *baseline,
    uint32_t *diagnostic_revision_baseline)
{
    if (baseline != NULL) athom_cloud_transport_metrics_copy(baseline);
    if (diagnostic_revision_baseline != NULL) {
        *diagnostic_revision_baseline = athom_cloud_diagnostic_revision();
    }
}

static void athom_inventory_attempt_diagnostic_complete(
    athom_refresh_origin_t origin,
    uint32_t attempt,
    esp_err_t final_error,
    int final_http_status,
    const char *stage,
    const athom_transport_metrics_t *baseline,
    uint32_t diagnostic_revision_baseline)
{
    athom_transport_metrics_t completed = {0};
    athom_cloud_transport_metrics_copy(&completed);
    const bool stage_updated =
        athom_cloud_diagnostic_revision() != diagnostic_revision_baseline;
    const athom_inventory_attempt_diagnostic_t diagnostic =
        athom_inventory_attempt_build(
            origin, attempt,
            (uint64_t)(esp_timer_get_time() / 1000LL),
            final_error,
            stage_updated ? final_http_status : 0,
            stage_updated && stage != NULL ? stage : "unknown",
            baseline, &completed);
    athom_inventory_attempt_diagnostic_publish(&diagnostic);
}

static bool patch038_refresh_authoritative_state_after_write(void)
{
    char selected_homey_id[ATHOM_HOMEY_ID_MAX] = {0};
    memcpy(selected_homey_id, s_cloud.selected_homey.id, sizeof(selected_homey_id));
    if (selected_homey_id[0] == '\0') {
        set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
        s_state_name = "homey_connection_error";
        return false;
    }

    s_schema_refresh_running = true;
    const int64_t refresh_start_us = esp_timer_get_time();
    ESP_LOGI(TAG,
             "PATCH038_LIGHT_REFRESH phase=begin retry_policy=none privacy=sanitized");

    athom_transport_metrics_t attempt_metrics_baseline = {0};
    uint32_t attempt_diagnostic_revision_baseline = 0U;
    athom_inventory_attempt_diagnostic_begin(
        &attempt_metrics_baseline, &attempt_diagnostic_revision_baseline);
    const esp_err_t transport_result =
        connect_and_fetch_inventory(selected_homey_id);
    esp_err_t effective_error = ESP_OK;
    int http_status = 0;
    const char *stage = "unknown";
    const bool verified = homey_inventory_result_verified(
        transport_result, &effective_error, &http_status, &stage);
    athom_inventory_attempt_diagnostic_complete(
        ATHOM_REFRESH_ORIGIN_LIGHT_RECONCILIATION, 1U,
        effective_error, http_status, stage, &attempt_metrics_baseline,
        attempt_diagnostic_revision_baseline);

    if (verified) {
        set_homey_data_state(ATHOM_HOMEY_DATA_READY);
        s_state_name = "ready";
        publish_cloud_state();
        phone_provisioning_show_live_ready(s_cloud.selected_homey.name);
    } else {
        set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
        s_state_name = "homey_connection_error";
    }

    ESP_LOGI(TAG,
             "PATCH038_LIGHT_REFRESH phase=end verified=%s elapsed_ms=%u "
             "error=%s http_status=%d stage=%s privacy=sanitized",
             verified ? "true" : "false",
             (unsigned)elapsed_ms_since(refresh_start_us),
             esp_err_to_name(effective_error),
             http_status,
             stage != NULL ? stage : "unknown");

    zero_secure(selected_homey_id, sizeof(selected_homey_id));
    s_schema_refresh_running = false;
    return verified;
}

static void patch038_complete_light_toggle_job(void)
{
    portENTER_CRITICAL(&s_refresh_job_mux);
    s_light_toggle_job_reserved = false;
    s_light_toggle_pending_widget = 0U;
    s_light_toggle_completion_generation++;
    if (s_light_toggle_completion_generation == 0U) {
        s_light_toggle_completion_generation = 1U;
    }
    portEXIT_CRITICAL(&s_refresh_job_mux);
}

static void homey_command_worker(void *arg)
{
    (void)arg;
    athom_homey_command_t command;

    for (;;) {
        if (xQueueReceive(s_homey_command_queue, &command, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (command.kind == ATHOM_HOMEY_COMMAND_LIGHT_TOGGLE) {
            const athom_light_toggle_dispatch_result_t dispatch_result =
                athom_oauth_runtime_dispatch_light_toggle(
                    command.widget_index, command.value);
            const bool refresh_required =
                patch038_dispatch_result_requires_authoritative_refresh(
                    dispatch_result);
            bool refresh_verified = false;
            if (refresh_required) {
                refresh_verified =
                    patch038_refresh_authoritative_state_after_write();
            }
            ESP_LOGI(TAG,
                     "PATCH038_LIGHT_ASYNC widget=%u dispatch=%s "
                     "refresh_required=%s refresh_verified=%s "
                     "automatic_write_retry=no optimistic_state=no "
                     "state_authority=read_only_refresh privacy=sanitized",
                     (unsigned)command.widget_index,
                     patch037_light_toggle_dispatch_result_name(dispatch_result),
                     refresh_required ? "true" : "false",
                     refresh_verified ? "true" : "false");
            patch038_complete_light_toggle_job();
            network_phase_release(ATHOM_NETWORK_PHASE_LIGHT_TOGGLE);
            continue;
        }

        if (command.kind != ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA) {
            continue;
        }

        const bool boot_auto =
            command.origin == ATHOM_REFRESH_ORIGIN_BOOT_AUTO;
        s_schema_refresh_running = true;
        ESP_LOGI(TAG, "HOMEY_SCHEMA path=queued_refresh phase=begin origin=%s",
                 inventory_refresh_origin_name(command.origin));
        const char *origin_name = inventory_refresh_origin_name(command.origin);
        runtime_diag_emit(RUNTIME_DIAG_EVENT_HOMEY_REFRESH_BEGIN,
                          (uint16_t)command.origin, 0, 0, 0U, 0U, 0U,
                          (uint8_t)command.origin, 0U);
        const int64_t refresh_start_us = esp_timer_get_time();
        patch021_homey_remote_log(
            "refresh_begin",
            origin_name,
            "started",
            0U,
            0U,
            0U,
            ESP_OK,
            0,
            "queued_refresh",
            false);

        char selected_homey_id[ATHOM_HOMEY_ID_MAX] = {0};
        memcpy(selected_homey_id, s_cloud.selected_homey.id, sizeof(selected_homey_id));

        unsigned attempt = 0U;
        for (;;) {
            attempt++;
            const int64_t attempt_start_us = esp_timer_get_time();
            athom_transport_metrics_t attempt_metrics_baseline = {0};
            uint32_t attempt_diagnostic_revision_baseline = 0U;
            athom_inventory_attempt_diagnostic_begin(
                &attempt_metrics_baseline,
                &attempt_diagnostic_revision_baseline);
            runtime_diag_emit(RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_BEGIN,
                              0U, 0, 0, attempt, 0U, 0U,
                              (uint8_t)command.origin, 0U);
            ESP_LOGI(TAG, "HOMEY_DATA phase=attempt_begin attempt=%u origin=%s",
                     attempt, origin_name);
            patch021_homey_remote_log(
                "attempt_begin",
                origin_name,
                "started",
                attempt,
                0U,
                0U,
                ESP_OK,
                0,
                "attempt_begin",
                false);

            esp_err_t transport_result = connect_and_fetch_inventory(selected_homey_id);
            esp_err_t effective_error = ESP_OK;
            int http_status = 0;
            const char *stage = "unknown";
            const bool verified = homey_inventory_result_verified(
                transport_result, &effective_error, &http_status, &stage);
            athom_inventory_attempt_diagnostic_complete(
                command.origin, attempt, effective_error, http_status,
                stage, &attempt_metrics_baseline,
                attempt_diagnostic_revision_baseline);

            if (verified) {
                set_homey_data_state(ATHOM_HOMEY_DATA_READY);
                runtime_diag_emit(RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_SUCCESS,
                                  1U, 0, http_status, attempt, 0U, 0U,
                                  (uint8_t)command.origin, 0U);
                if (command.origin == ATHOM_REFRESH_ORIGIN_PERIODIC) {
                    runtime_diag_emit(RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_RESULT,
                                      1U, 0, http_status, attempt, 0U, 0U,
                                      (uint8_t)command.origin, 0U);
                }
                s_state_name = "ready";
                publish_cloud_state();
                phone_provisioning_show_live_ready(s_cloud.selected_homey.name);
                ESP_LOGI(TAG,
                         "HOMEY_DATA state=ready attempt=%u verified_inventory=true favorites_state=%s",
                         attempt,
                         panel_homey_favorites_state_name(panel_homey_favorites_get_state()));
                patch021_homey_remote_log(
                    "attempt_end",
                    origin_name,
                    "success",
                    attempt,
                    elapsed_ms_since(attempt_start_us),
                    0U,
                    effective_error,
                    http_status,
                    stage,
                    false);
                ESP_LOGI(TAG,
                         "HOMEY_SCHEMA path=queued_refresh phase=end result=success attempts=%u",
                         attempt);
                patch021_homey_remote_log(
                    "refresh_end",
                    origin_name,
                    "success",
                    attempt,
                    elapsed_ms_since(refresh_start_us),
                    0U,
                    effective_error,
                    http_status,
                    stage,
                    false);
                break;
            }

            const bool transient =
                homey_data_failure_is_transient(effective_error, http_status) ||
                (boot_auto &&
                 effective_error == ESP_ERR_HTTP_EAGAIN &&
                 http_status == 0);
            runtime_diag_emit(RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_FAILURE,
                              transient ? 1U : 2U, effective_error, http_status,
                              attempt, 0U, 0U, (uint8_t)command.origin, 0U);
            ESP_LOGW(TAG,
                     "HOMEY_DATA phase=attempt_end attempt=%u result=failure transient=%s error=%s http_status=%d stage=%s",
                     attempt, transient ? "yes" : "no",
                     esp_err_to_name(effective_error), http_status, stage);
            patch021_homey_remote_log(
                "attempt_end",
                origin_name,
                "failure",
                attempt,
                elapsed_ms_since(attempt_start_us),
                0U,
                effective_error,
                http_status,
                stage,
                transient);

            if (!inventory_refresh_worker_should_retry(
                    command.origin, transient)) {
                set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
                if (command.origin == ATHOM_REFRESH_ORIGIN_PERIODIC) {
                    runtime_diag_emit(RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_RESULT,
                                      2U, effective_error, http_status, attempt,
                                      0U, 0U, (uint8_t)command.origin, 0U);
                }
                s_state_name = "homey_connection_error";
                ESP_LOGE(TAG,
                         "HOMEY_DATA state=error attempt=%u transient=%s error=%s http_status=%d stage=%s",
                         attempt, transient ? "yes" : "no",
                         esp_err_to_name(effective_error), http_status, stage);
                ESP_LOGI(TAG,
                         "HOMEY_SCHEMA path=queued_refresh phase=end result=failure attempts=%u",
                         attempt);
                patch021_homey_remote_log(
                    "refresh_end",
                    origin_name,
                    "failure",
                    attempt,
                    elapsed_ms_since(refresh_start_us),
                    0U,
                    effective_error,
                    http_status,
                    stage,
                    transient);
                break;
            }

            const uint32_t delay_ms = homey_data_retry_delay_ms(attempt);
            set_homey_data_state(ATHOM_HOMEY_DATA_RETRYING);
            runtime_diag_emit(RUNTIME_DIAG_EVENT_HOMEY_RETRY_SCHEDULED,
                              1U, effective_error, http_status, attempt,
                              delay_ms, 0U, (uint8_t)command.origin, 0U);
            s_state_name = "connecting_homey";
            ESP_LOGW(TAG,
                     "HOMEY_DATA state=retrying attempt=%u next_delay_ms=%u error=%s http_status=%d stage=%s",
                     attempt, (unsigned)delay_ms,
                     esp_err_to_name(effective_error), http_status, stage);
            patch021_homey_remote_log(
                "retry_scheduled",
                origin_name,
                "waiting",
                attempt,
                elapsed_ms_since(refresh_start_us),
                delay_ms,
                effective_error,
                http_status,
                stage,
                transient);
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }

        zero_secure(selected_homey_id, sizeof(selected_homey_id));
        s_schema_refresh_running = false;
        portENTER_CRITICAL(&s_refresh_job_mux);
        s_refresh_job_reserved = false;
        portEXIT_CRITICAL(&s_refresh_job_mux);
        network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    }
}



static athom_refresh_queue_result_t queue_inventory_refresh_if_ready(
    athom_refresh_origin_t origin)
{
    /*
     * A read-only inventory refresh is the recovery path that can re-establish
     * strict live readiness. Requiring strict runtime readiness here creates a
     * fail-closed deadlock after that readiness has been cleared. Gate this
     * read-only operation on transport prerequisites only; a verified inventory
     * republishes strict readiness in homey_command_worker().
     */
    if (!phone_provisioning_wifi_online() ||
        s_cloud.selected_homey.id[0] == 0 ||
        s_cloud.homey_session_token[0] == 0 ||
        s_homey_command_queue == NULL) {
        return ATHOM_REFRESH_QUEUE_NOT_READY;
    }

    if (!network_phase_try_reserve(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH)) {
        return ATHOM_REFRESH_QUEUE_BUSY;
    }

    bool refresh_reserved = false;
    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    if (!patch031_diag_probe_active_locked() &&
        !s_worker_running &&
        !s_select_worker_running &&
        !s_restore_worker_running) {
        portENTER_CRITICAL(&s_refresh_job_mux);
        if (!s_refresh_job_reserved && !s_light_toggle_job_reserved) {
            s_refresh_job_reserved = true;
            refresh_reserved = true;
        }
        portEXIT_CRITICAL(&s_refresh_job_mux);
    }
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    if (!refresh_reserved) {
        network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
        return ATHOM_REFRESH_QUEUE_BUSY;
    }

    const athom_homey_command_t command = {
        .kind = ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA,
        .origin = origin,
        .widget_index = 0U,
        .value = false,
    };
    if (xQueueSend(s_homey_command_queue, &command, 0) == pdTRUE) {
        return ATHOM_REFRESH_QUEUE_OK;
    }

    portENTER_CRITICAL(&s_refresh_job_mux);
    s_refresh_job_reserved = false;
    portEXIT_CRITICAL(&s_refresh_job_mux);
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    return ATHOM_REFRESH_QUEUE_FAILED;
}

static const char *refresh_queue_result_name(athom_refresh_queue_result_t result)
{
    switch (result) {
    case ATHOM_REFRESH_QUEUE_OK: return "queued";
    case ATHOM_REFRESH_QUEUE_NOT_READY: return "not_ready";
    case ATHOM_REFRESH_QUEUE_BUSY: return "busy";
    case ATHOM_REFRESH_QUEUE_FAILED: return "queue_failed";
    default: return "unknown";
    }
}

static void periodic_inventory_refresh_scheduler(void *arg)
{
    (void)arg;
    periodic_refresh_scheduler_state_t scheduler = {0};

    for (;;) {
        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000LL);
        panel_homey_snapshot_inspection_t inspection = {0};
        (void)athom_cloud_inspect_device_snapshot(now_ms, &inspection);

        if (periodic_refresh_scheduler_should_attempt(
                &scheduler,
                inspection.present,
                inspection.snapshot.captured_at_ms,
                inspection.age_ms,
                now_ms)) {
            const athom_refresh_queue_result_t result =
                queue_inventory_refresh_if_ready(
                    ATHOM_REFRESH_ORIGIN_PERIODIC);
            periodic_refresh_scheduler_record_queue_result(
                &scheduler, result, now_ms);
            runtime_diag_emit(RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_QUEUE_RESULT,
                              (uint16_t)result, 0, 0, 0U,
                              (uint32_t)periodic_refresh_scheduler_defer_ms(result),
                              (uint32_t)inspection.snapshot.generation,
                              ATHOM_REFRESH_ORIGIN_PERIODIC, 0U);
            ESP_LOGI(TAG,
                     "HOMEY_PERIODIC_REFRESH queue_result=%s snapshot_age_ms=%llu defer_ms=%llu",
                     refresh_queue_result_name(result),
                     (unsigned long long)inspection.age_ms,
                     (unsigned long long)periodic_refresh_scheduler_defer_ms(result));
        }

        vTaskDelay(pdMS_TO_TICKS(PERIODIC_REFRESH_SCHEDULER_POLL_MS));
    }
}

static void boot_auto_refresh_scheduler(void *arg)
{
    (void)arg;
    runtime_diag_emit(RUNTIME_DIAG_EVENT_BOOT_AUTO_SCHEDULER_BEGIN,
                      1U, 0, 0, 0U, 0U, 0U,
                      ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 0U);
    bool queued = false;

    for (unsigned attempt = 1U;
         attempt <= ATHOM_BOOT_AUTO_READY_WAIT_ATTEMPTS;
         ++attempt) {
        /* If Homey auth restore happened just before Wi-Fi became online,
         * re-assert the already-restored live-ready publication. This uses the
         * existing phone-provisioning readiness mechanism; no new Wi-Fi state
         * or network path is introduced. */
        if (!phone_provisioning_homey_runtime_ready() &&
            s_cloud.selected_homey.name[0] != 0) {
            phone_provisioning_show_live_ready(s_cloud.selected_homey.name);
        }

        if (strcmp(s_state_name, "ready") == 0 &&
            phone_provisioning_homey_runtime_ready()) {
            ESP_LOGI(TAG,
                     "HOMEY_BOOT_AUTO_REFRESH phase=end result=already_ready attempt=%u",
                     attempt);
            queued = true;
            break;
        }

        athom_refresh_queue_result_t result =
            queue_inventory_refresh_if_ready(ATHOM_REFRESH_ORIGIN_BOOT_AUTO);
        runtime_diag_emit(RUNTIME_DIAG_EVENT_BOOT_AUTO_QUEUE_RESULT,
                          (uint16_t)result, 0, 0, attempt, 0U, 0U,
                          ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 0U);

        ESP_LOGI(TAG,
                 "HOMEY_BOOT_AUTO_REFRESH phase=wait attempt=%u result=%s",
                 attempt, refresh_queue_result_name(result));

        if (result == ATHOM_REFRESH_QUEUE_OK) {
            ESP_LOGI(TAG,
                     "HOMEY_BOOT_AUTO_REFRESH phase=queue result=success attempt=%u executor=manual_refresh_worker",
                     attempt);
            queued = true;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(ATHOM_BOOT_AUTO_READY_WAIT_MS));
    }

    if (!queued) {
        s_state_name = "homey_connection_error";
        ESP_LOGE(TAG,
                 "HOMEY_BOOT_AUTO_REFRESH phase=end result=timeout");
    }

    s_boot_auto_refresh_scheduler_running = false;
    vTaskDelete(NULL);
}
static esp_err_t schema_refresh_get(httpd_req_t *r)
{
    httpd_resp_set_type(r, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(r, "Cache-Control", "no-store");

    const athom_refresh_queue_result_t result =
        queue_inventory_refresh_if_ready(ATHOM_REFRESH_ORIGIN_MANUAL);

    if (result == ATHOM_REFRESH_QUEUE_NOT_READY) {
        httpd_resp_set_status(r, "409 Conflict");
        return httpd_resp_sendstr(r, "not ready");
    }

    if (result == ATHOM_REFRESH_QUEUE_BUSY) {
        httpd_resp_set_status(r, "409 Conflict");
        return httpd_resp_sendstr(r, "busy");
    }

    if (result == ATHOM_REFRESH_QUEUE_FAILED) {
        httpd_resp_set_status(r, "503 Service Unavailable");
        return httpd_resp_sendstr(r, "queue failed");
    }

    httpd_resp_set_status(r, "202 Accepted");
    return httpd_resp_sendstr(r, "queued");
}

static esp_err_t refresh_post(httpd_req_t *r)
{
    if (!network_phase_try_reserve(ATHOM_NETWORK_PHASE_LIVE_TOKEN_REFRESH)) {
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(r, "busy");
    }

    bool refresh_allowed = false;
    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    if (!patch031_diag_probe_active_locked() &&
        !s_light_toggle_job_reserved) {
        s_patch031_live_refresh_running = true;
        refresh_allowed = true;
    }
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    if (!refresh_allowed) {
        network_phase_release(ATHOM_NETWORK_PHASE_LIVE_TOKEN_REFRESH);
        httpd_resp_set_status(r, "409 Conflict");
        httpd_resp_set_type(r, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(r, "busy");
    }

    s_state_name="refreshing";
    esp_err_t err=athom_cloud_refresh(&s_cloud);
    if(err==ESP_OK)err=athom_cloud_fetch_user_homeys(&s_cloud);
    if(err!=ESP_OK){
        s_state_name="login_required";
        patch031_diag_live_refresh_end();
        network_phase_release(ATHOM_NETWORK_PHASE_LIVE_TOKEN_REFRESH);
        return httpd_resp_send_err(r,HTTPD_401_UNAUTHORIZED,"Homey-inloggning krävs igen");
    }
    s_state_name=s_cloud.selected_homey.id[0]?"ready":"homey_selection_required";
    publish_cloud_state();
    patch031_diag_live_refresh_end();
    network_phase_release(ATHOM_NETWORK_PHASE_LIVE_TOKEN_REFRESH);
    return httpd_resp_sendstr(r,"ok");
}

static bool preselection_restore_failure_is_transient(esp_err_t err, int http_status)
{
    if (homey_data_failure_is_transient(err, http_status)) return true;
    athom_transport_metrics_t metrics;
    athom_cloud_transport_metrics_copy(&metrics);
    return metrics.last_classification == ATHOM_TRANSPORT_DNS_FAIL ||
           metrics.last_classification == ATHOM_TRANSPORT_TCP_CONNECT_FAIL ||
           metrics.last_classification == ATHOM_TRANSPORT_HTTP_TIMEOUT;
}

static void preselection_stack_hwm_log(const char *phase)
{
    UBaseType_t hwm_bytes = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG,
             "HOMEY_PRESELECT_STACK phase=%s hwm_bytes=%u privacy=sanitized",
             phase != NULL ? phase : "unknown",
             (unsigned)hwm_bytes);
}

static void preselection_restore_worker(void *arg)
{
    (void)arg;
    preselection_stack_hwm_log("entry");
    const int64_t start_us = esp_timer_get_time();
    bool refreshed = false;

    ESP_LOGI(TAG, "HOMEY_PRESELECT_RESTORE phase=worker result=started privacy=sanitized");

    for (unsigned attempt = 1U; attempt <= ATHOM_PRESELECT_RESTORE_MAX_ATTEMPTS; ++attempt) {
        set_homey_data_state(ATHOM_HOMEY_DATA_LOADING);
        s_state_name = "fetching_homeys";

        esp_err_t err = athom_cloud_fetch_user_homeys(&s_cloud);
 preselection_stack_hwm_log("after_discovery");
        const int http_status = athom_cloud_diagnostic_http_status();
        const bool transient = preselection_restore_failure_is_transient(err, http_status);
        const uint32_t elapsed_ms = elapsed_ms_since(start_us);
        const bool within_time = elapsed_ms < ATHOM_PRESELECT_RESTORE_MAX_ELAPSED_MS;
        const bool can_retry = attempt < ATHOM_PRESELECT_RESTORE_MAX_ATTEMPTS && within_time;
        athom_restore_policy_action_t action = athom_restore_policy_after_discovery(
            (int)err, http_status, s_cloud.homeys.count, refreshed, transient, can_retry);
        runtime_diag_emit(RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_ATTEMPT_RESULT,
                          (uint16_t)action, err, http_status, attempt, 0U, 0U,
                          0U, 1U);

        if (action == ATHOM_RESTORE_POLICY_SELECTION_REQUIRED) {
            set_homey_data_state(ATHOM_HOMEY_DATA_LOADING);
            s_state_name = "homey_selection_required";
            publish_cloud_state();
            ESP_LOGI(TAG,
                     "HOMEY_PRESELECT_RESTORE result=selection_required refreshed=%s attempt=%u homey_count=%u privacy=sanitized",
                     refreshed ? "true" : "false", attempt, (unsigned)s_cloud.homeys.count);
            break;
        }

        if (action == ATHOM_RESTORE_POLICY_REFRESH_AUTH) {
            set_homey_data_state(ATHOM_HOMEY_DATA_LOADING);
            s_state_name = "refreshing";
            preselection_stack_hwm_log("before_refresh");
            esp_err_t refresh_err = athom_cloud_refresh(&s_cloud);
            preselection_stack_hwm_log("after_refresh");
            if (athom_restore_policy_after_refresh((int)refresh_err) ==
                ATHOM_RESTORE_POLICY_LOGIN_REQUIRED) {
                set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
                s_state_name = "login_required";
                ESP_LOGW(TAG,
                         "HOMEY_PRESELECT_RESTORE result=login_required phase=refresh error=%s privacy=sanitized",
                         esp_err_to_name(refresh_err));
                break;
            }
            refreshed = true;
            publish_cloud_state();
            ESP_LOGI(TAG,
                     "HOMEY_PRESELECT_RESTORE phase=refresh result=success attempt=%u privacy=sanitized",
                     attempt);
            continue;
        }

        if (action == ATHOM_RESTORE_POLICY_LOGIN_REQUIRED) {
            set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
            s_state_name = "login_required";
            ESP_LOGW(TAG,
                     "HOMEY_PRESELECT_RESTORE result=login_required phase=discovery attempt=%u http_status=%d privacy=sanitized",
                     attempt, http_status);
            break;
        }

        if (action == ATHOM_RESTORE_POLICY_RETRY_TRANSIENT) {
            set_homey_data_state(ATHOM_HOMEY_DATA_LOADING);
            s_state_name = "restoring_preselection";
            ESP_LOGW(TAG,
                     "HOMEY_PRESELECT_RESTORE phase=attempt result=retry attempt=%u elapsed_ms=%u next_delay_ms=%u error=%s http_status=%d privacy=sanitized",
                     attempt, (unsigned)elapsed_ms, (unsigned)ATHOM_PRESELECT_RESTORE_RETRY_MS,
                     esp_err_to_name(err), http_status);
            runtime_diag_emit(RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_RETRY,
                              1U, err, http_status, attempt,
                              ATHOM_PRESELECT_RESTORE_RETRY_MS, 0U, 0U, 0U);
            vTaskDelay(pdMS_TO_TICKS(ATHOM_PRESELECT_RESTORE_RETRY_MS));
            continue;
        }

        set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
        s_state_name = "homey_connection_error";
        ESP_LOGW(TAG,
                 "HOMEY_PRESELECT_RESTORE result=connection_error attempt=%u elapsed_ms=%u transient=%s error=%s http_status=%d privacy=sanitized",
                 attempt, (unsigned)elapsed_ms, transient ? "yes" : "no",
                 esp_err_to_name(err), http_status);
        break;
    }

    portENTER_CRITICAL(&s_preselection_restore_mux);
    s_preselection_restore_worker_running = false;
    portEXIT_CRITICAL(&s_preselection_restore_mux);
    runtime_diag_emit(RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_RESULT,
                      (uint16_t)s_homey_data_state, 0, 0, 0U, 0U, 0U, 0U, 0U);
    preselection_stack_hwm_log("before_delete");
    network_phase_release(ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE);
    vTaskDelete(NULL);
}

static void maybe_start_preselection_restore_worker(void)
{
    if (!network_phase_try_reserve(ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE)) {
        return;
    }

    bool start = false;

    portENTER_CRITICAL(&s_patch031_diag_probe_mux);
    if (!patch031_diag_probe_active_locked()) {
        portENTER_CRITICAL(&s_preselection_restore_mux);
        if (athom_restore_policy_should_start_preselection(
                s_wifi_online,
                s_restore_worker_running,
                s_preselection_restore_pending,
                s_preselection_restore_worker_running)) {
            s_preselection_restore_pending = false;
            s_preselection_restore_worker_running = true;
            start = true;
        }
        portEXIT_CRITICAL(&s_preselection_restore_mux);
    }
    portEXIT_CRITICAL(&s_patch031_diag_probe_mux);

    if (!start) {
        network_phase_release(ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE);
        return;
    }

    runtime_diag_emit(RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_BEGIN,
                      1U, 0, 0, 0U, 0U, 0U, 0U, 0U);

    if (xTaskCreate(preselection_restore_worker, "athom_preselect",
                    12288, NULL, 5, NULL) != pdPASS) {
        portENTER_CRITICAL(&s_preselection_restore_mux);
        s_preselection_restore_worker_running = false;
        portEXIT_CRITICAL(&s_preselection_restore_mux);
        set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
        runtime_diag_emit(RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_RESULT,
                          ATHOM_HOMEY_DATA_ERROR, ESP_ERR_NO_MEM, 0,
                          0U, 0U, 0U, 0U, 0U);
        s_state_name = "homey_connection_error";
        ESP_LOGE(TAG,
                 "HOMEY_PRESELECT_RESTORE phase=worker result=create_failed privacy=sanitized");
        network_phase_release(ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE);
    }
}

static void auth_restore_worker(void *arg)
{
    (void)arg;
    s_state_name = "restoring_session";
    set_homey_data_state(ATHOM_HOMEY_DATA_LOADING);
    runtime_diag_emit(RUNTIME_DIAG_EVENT_AUTH_RESTORE_BEGIN,
                      1U, 0, 0, 0U, 0U, 0U, 0U, 0U);

    athom_auth_record_t *restored = calloc(1U, sizeof(*restored));
    if (restored == NULL) {
        ESP_LOGE(TAG, "Homey auth restore allocation failed");
        set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
        runtime_diag_emit(RUNTIME_DIAG_EVENT_AUTH_RESTORE_RESULT,
                          3U, ESP_ERR_NO_MEM, 0, 0U, 0U, 0U, 0U, 0U);
        s_state_name = "login_required";
        s_restore_worker_running = false;
        network_phase_release(ATHOM_NETWORK_PHASE_AUTH_RESTORE);
        vTaskDelete(NULL);
        return;
    }

    bool present = false;
    esp_err_t restore_err = athom_auth_store_load(restored, &present);
    runtime_diag_emit(RUNTIME_DIAG_EVENT_AUTH_RESTORE_RESULT,
                      restore_err != ESP_OK ? 3U : (present ? 1U : 2U),
                      restore_err, 0, 0U, 0U, 0U, 0U, 0U);

    if (restore_err == ESP_OK && present) {
        memcpy(&s_cloud.tokens, &restored->tokens, sizeof(s_cloud.tokens));
        memcpy(&s_cloud.selected_homey, &restored->selected_homey, sizeof(s_cloud.selected_homey));
        memcpy(s_cloud.homey_session_token, restored->homey_session_token, sizeof(s_cloud.homey_session_token));
        s_cloud.expires_at_s = restored->expires_at_s;
        s_cloud.zone_count = restored->zone_count;
        s_cloud.device_count = restored->device_count;

        const bool selected_homey_present = s_cloud.selected_homey.id[0] != '\0';
        ESP_LOGI(TAG,
                 "HOMEY_BOOT_AUTO_REFRESH restored selected=%s persisted_zones=%u persisted_devices=%u",
                 selected_homey_present ? "true" : "false",
                 (unsigned)s_cloud.zone_count, (unsigned)s_cloud.device_count);

        if (selected_homey_present) {
            set_homey_data_state(ATHOM_HOMEY_DATA_LOADING);
            s_state_name = "connecting_homey";
            ESP_LOGI(TAG, "HOMEY_DATA state=loading source=boot_restore persisted_counts_not_ready=true");

            /* v4.4 compatibility: this flag is a queue prerequisite only.
             * v4.6 separately gates dashboard visibility on HOMEY_DATA_READY. */
            if (s_cloud.selected_homey.name[0] != 0) {
                phone_provisioning_show_live_ready(s_cloud.selected_homey.name);
            }

            if (!s_boot_auto_refresh_scheduler_running) {
                s_boot_auto_refresh_scheduler_running = true;
                if (xTaskCreate(boot_auto_refresh_scheduler, "athom_boot_gate",
                                4096, NULL, 5, NULL) != pdPASS) {
                    s_boot_auto_refresh_scheduler_running = false;
                    set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
                    s_state_name = "homey_connection_error";
                    ESP_LOGE(TAG, "HOMEY_BOOT_AUTO_REFRESH phase=scheduler result=create_failed");
                } else {
                    ESP_LOGI(TAG, "HOMEY_BOOT_AUTO_REFRESH phase=scheduler result=started");
                }
            }
        } else {
            set_homey_data_state(ATHOM_HOMEY_DATA_LOADING);
            s_state_name = "restoring_preselection";
            portENTER_CRITICAL(&s_preselection_restore_mux);
            s_preselection_restore_pending = true;
            portEXIT_CRITICAL(&s_preselection_restore_mux);
            ESP_LOGI(TAG,
                     "HOMEY_PRESELECT_RESTORE phase=restore result=pending privacy=sanitized");
        }

        ESP_LOGI(TAG,
                 "Homey auth restore complete selected=%s zones=%u devices=%u",
                 selected_homey_present ? "true" : "false",
                 (unsigned)s_cloud.zone_count, (unsigned)s_cloud.device_count);
    } else if (restore_err == ESP_OK) {
        set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
        s_state_name = "login_required";
        ESP_LOGI(TAG, "No stored Homey auth session");
    } else {
        set_homey_data_state(ATHOM_HOMEY_DATA_ERROR);
        s_state_name = "login_required";
        ESP_LOGW(TAG, "Homey auth restore failed: %s", esp_err_to_name(restore_err));
    }

    zero_secure(restored, sizeof(*restored));
    free(restored);
    portENTER_CRITICAL(&s_preselection_restore_mux);
    s_restore_worker_running = false;
    portEXIT_CRITICAL(&s_preselection_restore_mux);
    network_phase_release(ATHOM_NETWORK_PHASE_AUTH_RESTORE);
    maybe_start_preselection_restore_worker();
    vTaskDelete(NULL);
}

esp_err_t athom_oauth_runtime_register_handlers(httpd_handle_t s)
{
    if(!s)return ESP_ERR_INVALID_ARG;

    if (s_runtime_id == 0U) {
        s_runtime_id = esp_random();

        if (s_runtime_id == 0U) {
            s_runtime_id = 1U;
        }
    }

    const httpd_uri_t handlers[]={
        {"/homey/client-config",HTTP_POST,client_config_post,NULL},
        {"/homey/login",HTTP_GET,login_get,NULL},
        {"/oauth/callback",HTTP_GET,callback_get,NULL},
        {"/homey/live-status",HTTP_GET,status_get,NULL},
        {"/homey/debug/runtime-journal",HTTP_GET,runtime_diag_journal_get,NULL},
        {"/homey/debug/patch031-cloud-user-me-probe",HTTP_POST,patch031_diag_cloud_user_me_probe_post,NULL},{"/homey/debug/patch031-cloud-user-me-probe-result",HTTP_GET,patch031_diag_cloud_user_me_probe_result_get,NULL},
        {"/homey/live-select",HTTP_POST,select_post,NULL},
        {"/homey/live-refresh",HTTP_POST,refresh_post,NULL},
        {"/homey/debug/refresh-inventory-schema",HTTP_GET,schema_refresh_get,NULL}
    };
    for(size_t i=0;i<sizeof(handlers)/sizeof(handlers[0]);i++){
        esp_err_t e=httpd_register_uri_handler(s,&handlers[i]);
        if(e!=ESP_OK&&e!=ESP_ERR_HTTPD_HANDLER_EXISTS)return e;
    }

    if (s_homey_command_queue == NULL) {
        s_homey_command_queue = xQueueCreate(1U, sizeof(athom_homey_command_t));
        if (s_homey_command_queue == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    if (s_homey_command_worker_task == NULL) {
        if (xTaskCreate(
                homey_command_worker,
                "athom_command",
                24576,
                NULL,
                5,
                &s_homey_command_worker_task) != pdPASS) {
            vQueueDelete(s_homey_command_queue);
            s_homey_command_queue = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    if (!s_periodic_refresh_scheduler_running) {
        s_periodic_refresh_scheduler_running = true;
        if (xTaskCreate(
                periodic_inventory_refresh_scheduler,
                "athom_periodic",
                4096,
                NULL,
                4,
                NULL) != pdPASS) {
            s_periodic_refresh_scheduler_running = false;
            return ESP_ERR_NO_MEM;
        }
    }

    if (s_restore_started == false) {
        if (!network_phase_try_reserve(ATHOM_NETWORK_PHASE_AUTH_RESTORE)) {
            return ESP_ERR_INVALID_STATE;
        }
        s_restore_started = true;
        s_restore_worker_running = true;
        s_state_name = "restoring_session";

        if (xTaskCreate(
                auth_restore_worker,
                "athom_restore",
                16384,
                NULL,
                5,
                NULL) != pdPASS) {
            s_restore_worker_running = false;
            s_restore_started = false;
            s_state_name = "login_required";
            network_phase_release(ATHOM_NETWORK_PHASE_AUTH_RESTORE);

            ESP_LOGE(
                TAG,
                "Could not start Homey auth restore task");

            return ESP_ERR_NO_MEM;
        }
    }

    return ESP_OK;
}
#endif
