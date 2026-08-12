#include "athom_cloud_client.h"
#include "panel_homey_alias_store.h"
#ifdef ESP_PLATFORM

#include "freertos/FreeRTOS.h"
#include "esp_heap_caps.h"
#include "esp_system.h"

static const char *s_diagnostic_stage = "idle";
static esp_err_t s_diagnostic_error = ESP_OK;
static int s_diagnostic_http_status;
static panel_homey_snapshot_store_t s_device_snapshot_store;
static volatile bool s_device_snapshot_store_initialized;
static panel_homey_alias_runtime_t s_alias_runtime;
static portMUX_TYPE s_device_snapshot_init_mux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE s_device_snapshot_mux = portMUX_INITIALIZER_UNLOCKED;

static void device_snapshot_lock(void *context)
{
    portENTER_CRITICAL((portMUX_TYPE *)context);
}

static void device_snapshot_unlock(void *context)
{
    portEXIT_CRITICAL((portMUX_TYPE *)context);
}

static void ensure_device_snapshot_store(void)
{
    if (s_device_snapshot_store_initialized) {
        return;
    }

    portENTER_CRITICAL(&s_device_snapshot_init_mux);
    if (!s_device_snapshot_store_initialized) {
        panel_homey_snapshot_store_init(
            &s_device_snapshot_store,
            &s_device_snapshot_mux,
            device_snapshot_lock,
            device_snapshot_unlock);
        s_device_snapshot_store_initialized = true;
    }
    portEXIT_CRITICAL(&s_device_snapshot_init_mux);
}

panel_homey_alias_store_result_t athom_cloud_alias_activate(const char *selected_homey_id)
{
    panel_homey_alias_record_t record;
    bool present = false;
    panel_homey_alias_store_result_t result =
        panel_homey_alias_store_load(selected_homey_id, &record, &present);
    if (result != PANEL_HOMEY_ALIAS_STORE_OK || !present) {
        panel_homey_alias_runtime_invalidate(&s_alias_runtime);
        return result == PANEL_HOMEY_ALIAS_STORE_OK
            ? PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED
            : result;
    }
    return panel_homey_alias_runtime_activate(
        &s_alias_runtime, &record, selected_homey_id);
}

void athom_cloud_alias_invalidate(void)
{
    panel_homey_alias_runtime_invalidate(&s_alias_runtime);
}

panel_homey_read_result_t athom_cloud_copy_device_snapshot(
    uint64_t now_ms,
    panel_homey_read_snapshot_t *out)
{
    ensure_device_snapshot_store();
    return panel_homey_snapshot_copy(&s_device_snapshot_store, now_ms, out);
}

static void diagnostic_set(
    const char *stage,
    esp_err_t error)
{
    s_diagnostic_stage = stage != NULL ? stage : "unknown";
    s_diagnostic_error = error;
    s_diagnostic_http_status = 0;
}

static void diagnostic_set_http(
    const char *stage,
    esp_err_t error,
    int http_status)
{
    s_diagnostic_stage = stage != NULL ? stage : "unknown";
    s_diagnostic_error = error;
    s_diagnostic_http_status = http_status;
}

const char *athom_cloud_diagnostic_stage(void)
{
    return s_diagnostic_stage;
}

esp_err_t athom_cloud_diagnostic_error(void)
{
    return s_diagnostic_error;
}

int athom_cloud_diagnostic_http_status(void)
{
    return s_diagnostic_http_status;
}

#include "athom_oauth_config.h"
#include "athom_oauth_flow.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_tls.h"
#include <errno.h>
#include <time.h>
#include "mbedtls/base64.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "panel_homey_favorites.h"
#define ATHOM_TOKEN_URL "https://api.athom.com/oauth2/token"
#define ATHOM_USER_URL "https://api.athom.com/user/me"
#define HTTP_BODY_MAX 65536U
#define HTTP_INVENTORY_BODY_MAX 524288U
#define CLOUD_HTTP_TIMEOUT_MS 8000
#define HOMEY_REMOTE_HTTP_TIMEOUT_MS 8000

static const char *TAG = "athom_cloud";

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    size_t maximum;
    bool overflow;
} response_buffer_t;


typedef enum {
    HTTP_ROLE_CLOUD = 0,
    HTTP_ROLE_HOMEY_REMOTE,
} http_role_t;

typedef struct {
    esp_http_client_handle_t handle;
    char origin[ATHOM_HOMEY_URL_MAX];
    int timeout_ms;
    http_role_t role;
} persistent_http_client_t;

static persistent_http_client_t s_cloud_http = {
    .timeout_ms = CLOUD_HTTP_TIMEOUT_MS,
    .role = HTTP_ROLE_CLOUD,
};
static persistent_http_client_t s_homey_http = {
    .timeout_ms = HOMEY_REMOTE_HTTP_TIMEOUT_MS,
    .role = HTTP_ROLE_HOMEY_REMOTE,
};
static athom_transport_metrics_t s_transport_metrics;

static const char *transport_role_name(http_role_t role)
{
    return role == HTTP_ROLE_CLOUD ? "cloud" : "homey_remote";
}

static void patch019a16d_log_memory(
    const char *point,
    esp_err_t perform_err,
    int tls_error,
    int tls_flags,
    int socket_errno)
{
    const uint32_t internal_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    const uint32_t psram_caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;

    ESP_LOGI(
        TAG,
        "PATCH019A16D_MEMORY role=homey_remote point=%s internal_free=%u "
        "internal_largest=%u internal_minimum=%u psram_free=%u psram_largest=%u "
        "perform_err=%s tls_error=%d tls_flags=0x%x socket_errno=%d privacy=sanitized",
        point,
        (unsigned)heap_caps_get_free_size(internal_caps),
        (unsigned)heap_caps_get_largest_free_block(internal_caps),
        (unsigned)heap_caps_get_minimum_free_size(internal_caps),
        (unsigned)heap_caps_get_free_size(psram_caps),
        (unsigned)heap_caps_get_largest_free_block(psram_caps),
        esp_err_to_name(perform_err),
        tls_error,
        (unsigned)tls_flags,
        socket_errno);
}

typedef struct {
    uint32_t matching_failure_count;
    size_t first_requested_size;
    size_t last_requested_size;
    size_t max_requested_size;
    uint32_t caps;
    size_t internal_free;
    size_t internal_largest;
    size_t internal_minimum;
    bool all_heap_caps_calloc;
} patch019a16e_alloc_failure_t;

static bool s_patch019a16e_hook_attempted;
static bool s_patch019a16e_hook_registered;
static bool s_patch019a16e_homey_capture_active;
static patch019a16e_alloc_failure_t s_patch019a16e_failure;

static void patch019a16e_failed_alloc_hook(
    size_t requested_size,
    uint32_t caps,
    const char *function_name)
{
    const uint32_t expected_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    if (!s_patch019a16e_homey_capture_active || caps != expected_caps) {
        return;
    }

    const bool is_heap_caps_calloc =
        function_name != NULL && strcmp(function_name, "heap_caps_calloc") == 0;

    if (s_patch019a16e_failure.matching_failure_count == 0U) {
        s_patch019a16e_failure.first_requested_size = requested_size;
        s_patch019a16e_failure.all_heap_caps_calloc = is_heap_caps_calloc;
    } else {
        s_patch019a16e_failure.all_heap_caps_calloc =
            s_patch019a16e_failure.all_heap_caps_calloc && is_heap_caps_calloc;
    }

    s_patch019a16e_failure.matching_failure_count++;
    s_patch019a16e_failure.last_requested_size = requested_size;
    if (requested_size > s_patch019a16e_failure.max_requested_size) {
        s_patch019a16e_failure.max_requested_size = requested_size;
    }
    s_patch019a16e_failure.caps = caps;
    s_patch019a16e_failure.internal_free = heap_caps_get_free_size(expected_caps);
    s_patch019a16e_failure.internal_largest =
        heap_caps_get_largest_free_block(expected_caps);
    s_patch019a16e_failure.internal_minimum =
        heap_caps_get_minimum_free_size(expected_caps);
}

static void patch019a16e_arm_homey_alloc_capture(void)
{
    memset(&s_patch019a16e_failure, 0, sizeof(s_patch019a16e_failure));
    if (!s_patch019a16e_hook_attempted) {
        s_patch019a16e_hook_attempted = true;
        s_patch019a16e_hook_registered =
            heap_caps_register_failed_alloc_callback(patch019a16e_failed_alloc_hook) == ESP_OK;
    }
    s_patch019a16e_homey_capture_active = s_patch019a16e_hook_registered;
}

static void patch019a16e_disarm_homey_alloc_capture(void)
{
    s_patch019a16e_homey_capture_active = false;
}

static void patch019a16e_log_failed_alloc(
    esp_err_t perform_err,
    int tls_error,
    int tls_flags,
    int socket_errno)
{
    const bool single_failure = s_patch019a16e_failure.matching_failure_count == 1U;
    const bool request_gt_largest = single_failure &&
        s_patch019a16e_failure.last_requested_size > s_patch019a16e_failure.internal_largest;

    ESP_LOGI(
        TAG,
        "PATCH019A16E_ALLOC_FAIL role=homey_remote hook_registered=%s matching_failures=%u "
        "first_requested_size=%u last_requested_size=%u max_requested_size=%u caps=0x%x "
        "all_heap_caps_calloc=%s internal_free_at_failure=%u internal_largest_at_failure=%u "
        "internal_minimum_at_failure=%u request_gt_largest=%s perform_err=%s tls_error=%d "
        "tls_flags=0x%x socket_errno=%d privacy=sanitized",
        s_patch019a16e_hook_registered ? "true" : "false",
        (unsigned)s_patch019a16e_failure.matching_failure_count,
        (unsigned)s_patch019a16e_failure.first_requested_size,
        (unsigned)s_patch019a16e_failure.last_requested_size,
        (unsigned)s_patch019a16e_failure.max_requested_size,
        (unsigned)s_patch019a16e_failure.caps,
        s_patch019a16e_failure.all_heap_caps_calloc ? "true" : "false",
        (unsigned)s_patch019a16e_failure.internal_free,
        (unsigned)s_patch019a16e_failure.internal_largest,
        (unsigned)s_patch019a16e_failure.internal_minimum,
        request_gt_largest ? "true" : "false",
        esp_err_to_name(perform_err),
        tls_error,
        (unsigned)tls_flags,
        socket_errno);
}

#define PATCH019A16F_REQUIRED_CONTIGUOUS 16717U

typedef struct {
    size_t internal_free;
    size_t internal_largest;
    size_t internal_minimum;
} patch019a16f_internal_snapshot_t;

static uint32_t s_patch019a16f_cloud_perform_count;
static uint32_t s_patch019a16f_homey_perform_count;
static bool s_patch019a16f_cloud_first_success_logged;
static bool s_patch019a16f_cloud_checkpoint_valid;
static size_t s_patch019a16f_cloud_last_largest;

static patch019a16f_internal_snapshot_t patch019a16f_internal_snapshot(void)
{
    const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    patch019a16f_internal_snapshot_t snapshot = {
        .internal_free = heap_caps_get_free_size(caps),
        .internal_largest = heap_caps_get_largest_free_block(caps),
        .internal_minimum = heap_caps_get_minimum_free_size(caps),
    };
    return snapshot;
}

static void patch019a16f_log_checkpoint(
    const char *role,
    const char *point,
    uint32_t ordinal,
    const patch019a16f_internal_snapshot_t *snapshot,
    size_t previous_largest,
    esp_err_t perform_err,
    int http_status,
    bool first_fresh)
{
    ESP_LOGI(
        TAG,
        "PATCH019A16F_MEMORY role=%s point=%s ordinal=%u internal_free=%u "
        "internal_largest=%u internal_minimum=%u previous_largest=%u required_contiguous=%u "
        "largest_ge_required=%s perform_err=%s http_status=%d first_fresh=%s privacy=sanitized",
        role,
        point,
        (unsigned)ordinal,
        (unsigned)snapshot->internal_free,
        (unsigned)snapshot->internal_largest,
        (unsigned)snapshot->internal_minimum,
        (unsigned)previous_largest,
        (unsigned)PATCH019A16F_REQUIRED_CONTIGUOUS,
        snapshot->internal_largest >= PATCH019A16F_REQUIRED_CONTIGUOUS ? "true" : "false",
        esp_err_to_name(perform_err),
        http_status,
        first_fresh ? "true" : "false");
}

static void patch019a16f_cloud_before_perform(void)
{
    const uint32_t ordinal = s_patch019a16f_cloud_perform_count + 1U;
    const patch019a16f_internal_snapshot_t snapshot = patch019a16f_internal_snapshot();
    const size_t previous_largest =
        s_patch019a16f_cloud_checkpoint_valid ? s_patch019a16f_cloud_last_largest : 0U;
    const bool first = ordinal == 1U;
    const bool changed = !s_patch019a16f_cloud_checkpoint_valid ||
        snapshot.internal_largest != s_patch019a16f_cloud_last_largest;

    if (first || changed) {
        patch019a16f_log_checkpoint(
            "cloud",
            first ? "cloud_before_first_perform" : "cloud_before_perform_change",
            ordinal,
            &snapshot,
            previous_largest,
            ESP_OK,
            0,
            first);
    }

    s_patch019a16f_cloud_checkpoint_valid = true;
    s_patch019a16f_cloud_last_largest = snapshot.internal_largest;
}

static void patch019a16f_cloud_after_perform(esp_err_t err, int http_status)
{
    const uint32_t ordinal = s_patch019a16f_cloud_perform_count;
    const patch019a16f_internal_snapshot_t snapshot = patch019a16f_internal_snapshot();
    const size_t previous_largest =
        s_patch019a16f_cloud_checkpoint_valid ? s_patch019a16f_cloud_last_largest : 0U;
    const bool first_success = err == ESP_OK && !s_patch019a16f_cloud_first_success_logged;
    const bool changed = !s_patch019a16f_cloud_checkpoint_valid ||
        snapshot.internal_largest != s_patch019a16f_cloud_last_largest;

    if (first_success || changed) {
        patch019a16f_log_checkpoint(
            "cloud",
            first_success ? "cloud_after_first_successful_perform" : "cloud_after_perform_change",
            ordinal,
            &snapshot,
            previous_largest,
            err,
            http_status,
            first_success && ordinal == 1U);
    }

    if (first_success) {
        s_patch019a16f_cloud_first_success_logged = true;
    }
    s_patch019a16f_cloud_checkpoint_valid = true;
    s_patch019a16f_cloud_last_largest = snapshot.internal_largest;
}

static void patch019a16f_homey_before_first_perform(void)
{
    if (s_patch019a16f_homey_perform_count != 0U) {
        return;
    }
    const patch019a16f_internal_snapshot_t snapshot = patch019a16f_internal_snapshot();
    patch019a16f_log_checkpoint(
        "homey_remote",
        "homey_before_first_perform",
        1U,
        &snapshot,
        s_patch019a16f_cloud_checkpoint_valid ? s_patch019a16f_cloud_last_largest : 0U,
        ESP_OK,
        0,
        true);
}

static bool s_patch019a17_cloud_transport_live;
static uint32_t s_patch019a17_handoff_close_count;

static void patch019a17_note_cloud_perform_result(esp_err_t perform_err)
{
    s_patch019a17_cloud_transport_live = perform_err == ESP_OK;
}

static esp_err_t patch019a17_cloud_to_homey_handoff(void)
{
    if (!s_patch019a17_cloud_transport_live) {
        return ESP_OK;
    }
    if (s_cloud_http.handle == NULL) {
        s_patch019a17_cloud_transport_live = false;
        return ESP_OK;
    }

    const esp_err_t close_err = esp_http_client_close(s_cloud_http.handle);
    if (close_err == ESP_OK) {
        s_patch019a17_cloud_transport_live = false;
        s_patch019a17_handoff_close_count++;
    }
    ESP_LOGI(
        TAG,
        "PATCH019A17_HANDOFF action=cloud_to_homey_close close_called=true close_err=%s "
        "handle_preserved=true close_count=%u privacy=sanitized",
        esp_err_to_name(close_err),
        (unsigned)s_patch019a17_handoff_close_count);
    return close_err;
}


/* Patch019A1.3 diagnostic-only helpers. No transport decisions are made here. */
static const char *patch019a13_role_name(http_role_t role)
{
    return role == HTTP_ROLE_CLOUD ? "CLOUD" : "HOMEY_REMOTE";
}

static void patch019a13_preflight_log(
    http_role_t role,
    const char *step,
    esp_err_t err)
{
    ESP_LOGI(
        TAG,
        "PATCH019A13_PREFLIGHT role=%s step=%s result=%s err=%s privacy=sanitized",
        patch019a13_role_name(role),
        step,
        err == ESP_OK ? "OK" : "FAIL",
        esp_err_to_name(err));
}

static void patch019a13_preflight_log_raw(
    http_role_t role,
    const char *step,
    esp_err_t effective_err,
    esp_err_t raw_err)
{
    ESP_LOGI(
        TAG,
        "PATCH019A13_PREFLIGHT role=%s step=%s result=%s err=%s raw_err=%s privacy=sanitized",
        patch019a13_role_name(role),
        step,
        effective_err == ESP_OK ? "OK" : "FAIL",
        esp_err_to_name(effective_err),
        esp_err_to_name(raw_err));
}

const char *athom_cloud_transport_class_name(athom_transport_class_t value)
{
    switch (value) {
    case ATHOM_TRANSPORT_OK: return "OK";
    case ATHOM_TRANSPORT_DNS_FAIL: return "DNS_FAIL";
    case ATHOM_TRANSPORT_TCP_CONNECT_FAIL: return "TCP_CONNECT_FAIL";
    case ATHOM_TRANSPORT_TLS_FAIL: return "TLS_FAIL";
    case ATHOM_TRANSPORT_HTTP_TIMEOUT: return "HTTP_TIMEOUT";
    case ATHOM_TRANSPORT_HTTP_401: return "HTTP_401";
    case ATHOM_TRANSPORT_HTTP_403: return "HTTP_403";
    case ATHOM_TRANSPORT_HTTP_408: return "HTTP_408";
    case ATHOM_TRANSPORT_HTTP_429: return "HTTP_429";
    case ATHOM_TRANSPORT_HTTP_5XX: return "HTTP_5XX";
    case ATHOM_TRANSPORT_HOMEY_SESSION_FAIL: return "HOMEY_SESSION_FAIL";
    case ATHOM_TRANSPORT_FAVORITES_FAIL: return "FAVORITES_FAIL";
    case ATHOM_TRANSPORT_ZONES_FAIL: return "ZONES_FAIL";
    case ATHOM_TRANSPORT_DEVICES_FAIL: return "DEVICES_FAIL";
    case ATHOM_TRANSPORT_PARSE_FAIL: return "PARSE_FAIL";
    case ATHOM_TRANSPORT_NO_VALID_ENDPOINT: return "NO_VALID_ENDPOINT";
    default: return "UNKNOWN";
    }
}

void athom_cloud_transport_metrics_copy(athom_transport_metrics_t *out)
{
    if (out != NULL) *out = s_transport_metrics;
}

static void transport_cleanup_one(persistent_http_client_t *ctx)
{
    if (ctx == NULL || ctx->handle == NULL) return;
    esp_http_client_cleanup(ctx->handle);
    ctx->handle = NULL;
    ctx->origin[0] = 0;
    if (ctx->role == HTTP_ROLE_CLOUD) {
        s_transport_metrics.cloud_client_cleanup_count++;
    } else {
        s_transport_metrics.homey_client_cleanup_count++;
    }
}

void athom_cloud_transport_reset(void)
{
    transport_cleanup_one(&s_cloud_http);
    transport_cleanup_one(&s_homey_http);
    ESP_LOGI(TAG, "PATCH019A1_TRANSPORT action=explicit_reset privacy=sanitized");
}

static bool transport_extract_origin(const char *url, char *out, size_t capacity)
{
    if (url == NULL || out == NULL || capacity < 2U) return false;
    const char *scheme_end = strstr(url, "://");
    if (scheme_end == NULL) return false;
    const char *path = strchr(scheme_end + 3, '/');
    size_t length = path != NULL ? (size_t)(path - url) : strlen(url);
    if (length == 0U || length >= capacity) return false;
    memcpy(out, url, length);
    out[length] = 0;
    return true;
}

static bool transport_url_is_cloud(const char *url)
{
    return url != NULL && strncmp(url, "https://api.athom.com/", 22U) == 0;
}

static void transport_memory_log(const char *point, size_t response_bytes)
{
    ESP_LOGI(
        TAG,
        "PATCH019A1_MEMORY point=%s heap_free=%u heap_min=%u heap_largest=%u "
        "psram_free=%u psram_largest=%u response_bytes=%u privacy=sanitized",
        point != NULL ? point : "UNKNOWN",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
        (unsigned)esp_get_minimum_free_heap_size(),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
        (unsigned)response_bytes);
}

static athom_transport_class_t transport_classify(
    esp_err_t err,
    int http_status,
    esp_err_t tls_query,
    int tls_error,
    int tls_flags,
    int socket_errno)
{
    if (http_status == 401) return ATHOM_TRANSPORT_HTTP_401;
    if (http_status == 403) return ATHOM_TRANSPORT_HTTP_403;
    if (http_status == 408) return ATHOM_TRANSPORT_HTTP_408;
    if (http_status == 429) return ATHOM_TRANSPORT_HTTP_429;
    if (http_status >= 500 && http_status <= 599) return ATHOM_TRANSPORT_HTTP_5XX;
    if (err == ESP_OK) return ATHOM_TRANSPORT_OK;

    if (err == ESP_ERR_TIMEOUT || socket_errno == ETIMEDOUT) {
        return ATHOM_TRANSPORT_HTTP_TIMEOUT;
    }
    if (tls_query == ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME ||
        tls_error == ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME) {
        return ATHOM_TRANSPORT_DNS_FAIL;
    }
    if (tls_query == ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT ||
        tls_error == ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT) {
        return ATHOM_TRANSPORT_HTTP_TIMEOUT;
    }
    if (tls_query == ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET ||
        tls_query == ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST ||
        tls_error == ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET ||
        tls_error == ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST ||
        err == ESP_ERR_HTTP_CONNECT) {
        return ATHOM_TRANSPORT_TCP_CONNECT_FAIL;
    }
    if (tls_query != ESP_OK || tls_error != 0 || tls_flags != 0) {
        return ATHOM_TRANSPORT_TLS_FAIL;
    }
    return ATHOM_TRANSPORT_TCP_CONNECT_FAIL;
}

static void transport_stage_failure(
    athom_transport_class_t classification,
    const char *stage,
    esp_err_t err,
    int http_status)
{
    s_transport_metrics.last_classification = classification;
    s_transport_metrics.last_http_status = http_status;
    ESP_LOGW(
        TAG,
        "PATCH019A1_STAGE classification=%s stage=%s err=%s http_status=%d privacy=sanitized",
        athom_cloud_transport_class_name(classification),
        stage != NULL ? stage : "unknown",
        esp_err_to_name(err),
        http_status);
}

static void zero_secure(void *buffer, size_t size)
{
    volatile unsigned char *p = buffer;
    while (size--) *p++ = 0U;
}

static bool copy_optional(char *dst, size_t cap, const cJSON *object, const char *key)
{
    cJSON *value = cJSON_GetObjectItemCaseSensitive((cJSON *)object, key);
    if (!cJSON_IsString(value) || value->valuestring == NULL) {
        dst[0] = '\0';
        return true;
    }
    size_t n = strlen(value->valuestring);
    if (n >= cap) return false;
    memcpy(dst, value->valuestring, n + 1U);
    return true;
}

static esp_err_t event_handler(esp_http_client_event_t *event)
{
    response_buffer_t *buffer = event != NULL ? event->user_data : NULL;
    if (event == NULL || buffer == NULL) return ESP_OK;

    switch (event->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "ATHOM_NET http_event=error");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "ATHOM_NET http_event=connected");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "ATHOM_NET http_event=disconnected");
        break;
    default:
        break;
    }

    if (event->event_id == HTTP_EVENT_ON_DATA &&
        event->data_len > 0) {
        size_t incoming = (size_t)event->data_len;

        if (incoming >= buffer->maximum ||
            buffer->length >
                buffer->maximum - incoming - 1U) {
            buffer->overflow = true;
            return ESP_ERR_NO_MEM;
        }

        size_t required =
            buffer->length + incoming + 1U;

        if (required > buffer->capacity) {
            size_t new_capacity = buffer->capacity;

            while (new_capacity < required) {
                if (new_capacity >= buffer->maximum) {
                    buffer->overflow = true;
                    return ESP_ERR_NO_MEM;
                }

                if (new_capacity >
                    buffer->maximum / 2U) {
                    new_capacity = buffer->maximum;
                } else {
                    new_capacity *= 2U;
                }
            }

            char *resized = realloc(
                buffer->data,
                new_capacity);

            if (resized == NULL) {
                buffer->overflow = true;
                return ESP_ERR_NO_MEM;
            }

            memset(
                resized + buffer->capacity,
                0,
                new_capacity - buffer->capacity);

            buffer->data = resized;
            buffer->capacity = new_capacity;
        }

        memcpy(
            buffer->data + buffer->length,
            event->data,
            incoming);

        buffer->length += incoming;
        buffer->data[buffer->length] = 0;
    }

    return ESP_OK;
}

static esp_err_t http_request_limited(
    const char *url,
    esp_http_client_method_t method,
    const char *authorization,
    const char *content_type,
    const char *body,
    char **response_out,
    int *status_out,
    size_t response_maximum,
    size_t *response_capacity_out)
{
    if (url == NULL || response_out == NULL || status_out == NULL || response_maximum < 2U) {
        patch019a13_preflight_log(HTTP_ROLE_CLOUD, "extract_origin", ESP_ERR_INVALID_ARG);
        return ESP_ERR_INVALID_ARG;
    }

    *response_out = NULL;
    *status_out = 0;
    if (response_capacity_out != NULL) *response_capacity_out = 0U;

    const bool cloud = transport_url_is_cloud(url);
    persistent_http_client_t *ctx = cloud ? &s_cloud_http : &s_homey_http;
    char origin[ATHOM_HOMEY_URL_MAX] = {0};
    const bool patch019a13_origin_ok =
        transport_extract_origin(url, origin, sizeof(origin));
    patch019a13_preflight_log(
        ctx->role,
        "extract_origin",
        patch019a13_origin_ok ? ESP_OK : ESP_ERR_INVALID_ARG);
    if (!patch019a13_origin_ok) {
        transport_stage_failure(ATHOM_TRANSPORT_NO_VALID_ENDPOINT,
                                s_diagnostic_stage, ESP_ERR_INVALID_ARG, 0);
        return ESP_ERR_INVALID_ARG;
    }

    size_t initial_capacity = HTTP_BODY_MAX;
    if (initial_capacity > response_maximum) initial_capacity = response_maximum;
    char *response = calloc(1U, initial_capacity);
    patch019a13_preflight_log(
        ctx->role, "response_alloc", response != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    if (response == NULL) return ESP_ERR_NO_MEM;

    response_buffer_t buffer = {
        .data = response,
        .length = 0U,
        .capacity = initial_capacity,
        .maximum = response_maximum,
        .overflow = false,
    };

    if (ctx->handle != NULL && strcmp(ctx->origin, origin) != 0) {
        transport_cleanup_one(ctx);
        if (!cloud) s_transport_metrics.remote_rebind_count++;
    }
    patch019a13_preflight_log(ctx->role, "origin_rebind_cleanup", ESP_OK);

    if (ctx->handle == NULL) {
        esp_http_client_config_t config = {
            .url = url,
            .event_handler = event_handler,
            .user_data = &buffer,
            .timeout_ms = ctx->timeout_ms,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .buffer_size = 2048,
            .buffer_size_tx = 2048,
        };
        ctx->handle = esp_http_client_init(&config);
        patch019a13_preflight_log(
            ctx->role, "client_init", ctx->handle != NULL ? ESP_OK : ESP_FAIL);
        if (ctx->handle == NULL) {
            zero_secure(buffer.data, buffer.capacity);
            free(buffer.data);
            return ESP_FAIL;
        }
        patch019a13_preflight_log(ctx->role, "set_url", ESP_OK);
        (void)snprintf(ctx->origin, sizeof(ctx->origin), "%s", origin);
        if (cloud) s_transport_metrics.cloud_client_init_count++;
        else s_transport_metrics.homey_client_init_count++;
    } else {
        patch019a13_preflight_log(ctx->role, "client_init", ESP_OK);
        if (cloud) s_transport_metrics.cloud_client_reuse_count++;
        else s_transport_metrics.homey_client_reuse_count++;
        esp_err_t set_url_err = esp_http_client_set_url(ctx->handle, url);
        patch019a13_preflight_log(ctx->role, "set_url", set_url_err);
        if (set_url_err != ESP_OK) {
            zero_secure(buffer.data, buffer.capacity);
            free(buffer.data);
            return set_url_err;
        }
    }

    if (cloud) s_transport_metrics.cloud_request_count++;
    else s_transport_metrics.homey_request_count++;

    esp_err_t configure_err = esp_http_client_set_user_data(ctx->handle, &buffer);
    patch019a13_preflight_log(ctx->role, "set_user_data", configure_err);
    if (configure_err == ESP_OK) {
        configure_err = esp_http_client_set_method(ctx->handle, method);
        patch019a13_preflight_log(ctx->role, "set_method", configure_err);
    }
    if (configure_err == ESP_OK) {
        const esp_err_t raw_err =
            esp_http_client_delete_header(ctx->handle, "Authorization");
        patch019a13_preflight_log_raw(
            ctx->role, "delete_authorization_header", ESP_OK, raw_err);
    }
    if (configure_err == ESP_OK) {
        const esp_err_t raw_err =
            esp_http_client_delete_header(ctx->handle, "Content-Type");
        patch019a13_preflight_log_raw(
            ctx->role, "delete_content_type_header", ESP_OK, raw_err);
    }
    if (configure_err == ESP_OK && authorization != NULL) {
        configure_err = esp_http_client_set_header(ctx->handle, "Authorization", authorization);
        patch019a13_preflight_log(ctx->role, "set_authorization_header", configure_err);
    } else if (configure_err == ESP_OK) {
        patch019a13_preflight_log(ctx->role, "set_authorization_header", ESP_OK);
    }
    if (configure_err == ESP_OK && content_type != NULL) {
        configure_err = esp_http_client_set_header(ctx->handle, "Content-Type", content_type);
        patch019a13_preflight_log(ctx->role, "set_content_type_header", configure_err);
    } else if (configure_err == ESP_OK) {
        patch019a13_preflight_log(ctx->role, "set_content_type_header", ESP_OK);
    }
    if (configure_err == ESP_OK) {
        esp_err_t post_err;
        if (body != NULL) {
            post_err = esp_http_client_set_post_field(
                ctx->handle, body, (int)strlen(body));
        } else {
            post_err = esp_http_client_set_post_field(ctx->handle, NULL, 0);
            if (post_err == ESP_ERR_NOT_FOUND) {
                post_err = ESP_OK;
            }
        }
        configure_err = post_err;
        patch019a13_preflight_log(ctx->role, "set_post_field", configure_err);
    }
    if (configure_err != ESP_OK) {
        zero_secure(buffer.data, buffer.capacity);
        free(buffer.data);
        return configure_err;
    }

    const int64_t request_begin_us = esp_timer_get_time();
    if (ctx->role == HTTP_ROLE_CLOUD) {
        patch019a16f_cloud_before_perform();
    } else if (ctx->role == HTTP_ROLE_HOMEY_REMOTE) {
        patch019a16f_homey_before_first_perform();
    }
    if (ctx->role == HTTP_ROLE_HOMEY_REMOTE) {
        patch019a16d_log_memory("before_perform", ESP_OK, 0, 0, 0);
        patch019a16e_arm_homey_alloc_capture();
    }
    if (ctx->role == HTTP_ROLE_CLOUD) {
        s_patch019a16f_cloud_perform_count++;
    } else if (ctx->role == HTTP_ROLE_HOMEY_REMOTE) {
        s_patch019a16f_homey_perform_count++;
    }
    patch019a13_preflight_log(ctx->role, "perform_enter", ESP_OK);
    esp_err_t err = esp_http_client_perform(ctx->handle);
    if (ctx->role == HTTP_ROLE_HOMEY_REMOTE) {
        patch019a16e_disarm_homey_alloc_capture();
    }
    const uint32_t elapsed_ms = (uint32_t)((esp_timer_get_time() - request_begin_us) / 1000LL);
    const int http_status = esp_http_client_get_status_code(ctx->handle);
    const int socket_errno = esp_http_client_get_errno(ctx->handle);
    if (http_status > 0) *status_out = http_status;

    int tls_error = 0;
    int tls_flags = 0;
    esp_err_t tls_query = esp_http_client_get_and_clear_last_tls_error(
        ctx->handle, &tls_error, &tls_flags);

    if (ctx->role == HTTP_ROLE_CLOUD) {
        patch019a16f_cloud_after_perform(err, http_status);
        patch019a17_note_cloud_perform_result(err);
    }

    if (ctx->role == HTTP_ROLE_HOMEY_REMOTE && err != ESP_OK) {
        patch019a16d_log_memory(
            "after_failed_perform", err, tls_error, tls_flags, socket_errno);
        if (tls_error == 141) {
            patch019a16e_log_failed_alloc(err, tls_error, tls_flags, socket_errno);
        }
    }

    const athom_transport_class_t classification = transport_classify(
        err, http_status, tls_query, tls_error, tls_flags, socket_errno);
    s_transport_metrics.last_request_elapsed_ms = elapsed_ms;
    s_transport_metrics.last_classification = classification;
    s_transport_metrics.last_http_status = http_status;
    s_transport_metrics.last_tls_error = tls_error;
    s_transport_metrics.last_tls_flags = tls_flags;

    ESP_LOGI(
        TAG,
        "PATCH019A1_TRANSPORT role=%s endpoint=%s stage=%s elapsed_ms=%u classification=%s "
        "http_status=%d tls_error=%d tls_flags=0x%x socket_errno=%d "
        "cloud_init=%u cloud_reuse=%u cloud_cleanup=%u homey_init=%u homey_reuse=%u "
        "homey_cleanup=%u cloud_requests=%u homey_requests=%u session_creates=%u "
        "remote_rebinds=%u privacy=sanitized",
        transport_role_name(ctx->role),
        ctx->role == HTTP_ROLE_CLOUD ? "CLOUD" : "REMOTE",
        s_diagnostic_stage != NULL ? s_diagnostic_stage : "unknown",
        (unsigned)elapsed_ms,
        athom_cloud_transport_class_name(classification),
        http_status,
        tls_error,
        (unsigned)tls_flags,
        socket_errno,
        (unsigned)s_transport_metrics.cloud_client_init_count,
        (unsigned)s_transport_metrics.cloud_client_reuse_count,
        (unsigned)s_transport_metrics.cloud_client_cleanup_count,
        (unsigned)s_transport_metrics.homey_client_init_count,
        (unsigned)s_transport_metrics.homey_client_reuse_count,
        (unsigned)s_transport_metrics.homey_client_cleanup_count,
        (unsigned)s_transport_metrics.cloud_request_count,
        (unsigned)s_transport_metrics.homey_request_count,
        (unsigned)s_transport_metrics.homey_session_create_count,
        (unsigned)s_transport_metrics.remote_rebind_count);

    (void)esp_http_client_set_post_field(ctx->handle, NULL, 0);
    (void)esp_http_client_delete_header(ctx->handle, "Authorization");
    (void)esp_http_client_delete_header(ctx->handle, "Content-Type");
    (void)esp_http_client_set_user_data(ctx->handle, NULL);

    if (err != ESP_OK) {
        const esp_err_t close_err = esp_http_client_close(ctx->handle);
        ESP_LOGI(
            TAG,
            "PATCH019A16_RECOVERY role=%s perform_err=%s close_called=true close_err=%s "
            "handle_preserved=true privacy=sanitized",
            transport_role_name(ctx->role),
            esp_err_to_name(err),
            esp_err_to_name(close_err));
    }

    if (buffer.overflow) {
        ESP_LOGE(TAG,
                 "ATHOM_NET response_overflow=true response_bytes=%u capacity=%u maximum=%u",
                 (unsigned)buffer.length, (unsigned)buffer.capacity, (unsigned)buffer.maximum);
        zero_secure(buffer.data, buffer.capacity);
        free(buffer.data);
        return ESP_ERR_NO_MEM;
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG,
                 "ATHOM_NET response_overflow=false response_bytes=%u capacity=%u maximum=%u",
                 (unsigned)buffer.length, (unsigned)buffer.capacity, (unsigned)buffer.maximum);
        *status_out = http_status;
        *response_out = buffer.data;
        if (response_capacity_out != NULL) *response_capacity_out = buffer.capacity;
        return ESP_OK;
    }

    zero_secure(buffer.data, buffer.capacity);
    free(buffer.data);
    return err;
}

static esp_err_t http_request(
    const char *url,
    esp_http_client_method_t method,
    const char *authorization,
    const char *content_type,
    const char *body,
    char **response_out,
    int *status_out)
{
    return http_request_limited(
        url,
        method,
        authorization,
        content_type,
        body,
        response_out,
        status_out,
        HTTP_BODY_MAX,
        NULL);
}

static bool url_encode(const char *input, char *output, size_t capacity)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t used = 0U;
    if (input == NULL || output == NULL || capacity == 0U) return false;
    for (size_t i = 0U; input[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)input[i];
        bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                    c == '.' || c == '~';
        if (safe) {
            if (used + 1U >= capacity) return false;
            output[used++] = (char)c;
        } else {
            if (used + 3U >= capacity) return false;
            output[used++] = '%';
            output[used++] = hex[c >> 4];
            output[used++] = hex[c & 0x0FU];
        }
    }
    output[used] = '\0';
    return true;
}

static esp_err_t basic_authorization(
    const athom_oauth_client_config_t *config,
    char *output,
    size_t capacity)
{
    char joined[ATHOM_CLIENT_ID_MAX + ATHOM_CLIENT_SECRET_MAX + 2U];
    int written = snprintf(joined, sizeof(joined), "%s:%s",
                           config->client_id, config->client_secret);
    if (written <= 0 || (size_t)written >= sizeof(joined)) {
        zero_secure(joined, sizeof(joined));
        return ESP_ERR_INVALID_SIZE;
    }

    unsigned char encoded[640];
    size_t encoded_len = 0U;
    int rc = mbedtls_base64_encode(
        encoded, sizeof(encoded) - 1U, &encoded_len,
        (const unsigned char *)joined, (size_t)written);
    zero_secure(joined, sizeof(joined));
    if (rc != 0 || encoded_len + 7U >= capacity) {
        zero_secure(encoded, sizeof(encoded));
        return ESP_ERR_INVALID_SIZE;
    }
    encoded[encoded_len] = '\0';
    written = snprintf(output, capacity, "Basic %s", encoded);
    zero_secure(encoded, sizeof(encoded));
    return written > 0 && (size_t)written < capacity ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

static esp_err_t bearer_authorization(
    const char *token, char *output, size_t capacity)
{
    if (token == NULL || token[0] == '\0') return ESP_ERR_INVALID_ARG;
    int written = snprintf(output, capacity, "Bearer %s", token);
    return written > 0 && (size_t)written < capacity ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

static esp_err_t parse_token_response(
    const char *json,
    athom_token_set_t *tokens,
    bool refresh_flow)
{
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        transport_stage_failure(ATHOM_TRANSPORT_PARSE_FAIL, "oauth_token_parse", ESP_ERR_INVALID_RESPONSE, 0);
        return ESP_ERR_INVALID_RESPONSE;
    }

    cJSON *access = cJSON_GetObjectItemCaseSensitive(root, "access_token");
    cJSON *refresh = cJSON_GetObjectItemCaseSensitive(root, "refresh_token");
    cJSON *expires = cJSON_GetObjectItemCaseSensitive(root, "expires_in");

    const char *access_value = cJSON_IsString(access) ? access->valuestring : NULL;
    const char *refresh_value = cJSON_IsString(refresh) ? refresh->valuestring : NULL;
    uint32_t expires_value = cJSON_IsNumber(expires) && expires->valuedouble > 0
        ? (uint32_t)expires->valuedouble : 3600U;

    bool ok = athom_token_set_apply_refresh(
        tokens, access_value,
        refresh_flow ? refresh_value : refresh_value,
        expires_value);
    cJSON_Delete(root);
    return ok ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t token_request(
    const char *grant_body,
    athom_token_set_t *tokens,
    bool refresh_flow)
{
    athom_oauth_client_config_t config;
    bool present = false;
    esp_err_t err = athom_oauth_client_config_load(&config, &present);
    if (err != ESP_OK || !present) {
        zero_secure(&config, sizeof(config));
        return err == ESP_OK ? ESP_ERR_NOT_FOUND : err;
    }

    char authorization[768];
    err = basic_authorization(&config, authorization, sizeof(authorization));
    zero_secure(&config, sizeof(config));
    if (err != ESP_OK) return err;

    char *response = NULL;
    int status = 0;

    diagnostic_set("oauth_token_request", ESP_OK);

    err = http_request(
        ATHOM_TOKEN_URL, HTTP_METHOD_POST, authorization,
        "application/x-www-form-urlencoded", grant_body,
        &response, &status);

    zero_secure(authorization, sizeof(authorization));

    if (err != ESP_OK) {
        diagnostic_set("oauth_token_request", err);
        return err;
    }

    diagnostic_set_http("oauth_token_http", ESP_OK, status);

    if (status < 200 || status >= 300) {
        esp_err_t status_error =
            status == 400 || status == 401
                ? ESP_ERR_INVALID_STATE
                : ESP_FAIL;

        diagnostic_set_http(
            "oauth_token_http",
            status_error,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return status_error;
    }

    diagnostic_set("oauth_token_parse", ESP_OK);
    err = parse_token_response(response, tokens, refresh_flow);

    if (err != ESP_OK) {
        diagnostic_set("oauth_token_parse", err);
    }

    zero_secure(response, HTTP_BODY_MAX);
    free(response);
    return err;
}

esp_err_t athom_cloud_exchange_code(
    const char *authorization_code,
    athom_cloud_state_t *state)
{
    if (authorization_code == NULL || state == NULL) return ESP_ERR_INVALID_ARG;

    char encoded_code[ATHOM_OAUTH_CODE_MAX * 3U];
    char encoded_redirect[ATHOM_REDIRECT_URI_MAX * 3U];
    if (!url_encode(authorization_code, encoded_code, sizeof(encoded_code)) ||
        !url_encode(ATHOM_REDIRECT_URI, encoded_redirect, sizeof(encoded_redirect))) {
        zero_secure(encoded_code, sizeof(encoded_code));
        zero_secure(encoded_redirect, sizeof(encoded_redirect));
        return ESP_ERR_INVALID_SIZE;
    }

    char body[2048];
    int written = snprintf(
        body, sizeof(body),
        "grant_type=authorization_code&code=%s&redirect_uri=%s",
        encoded_code, encoded_redirect);
    zero_secure(encoded_code, sizeof(encoded_code));
    zero_secure(encoded_redirect, sizeof(encoded_redirect));
    if (written <= 0 || (size_t)written >= sizeof(body)) {
        zero_secure(body, sizeof(body));
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t err = token_request(body, &state->tokens, false);
    zero_secure(body, sizeof(body));
    if (err == ESP_OK) {
        state->expires_at_s =
            (uint64_t)(esp_timer_get_time() / 1000000LL) +
            state->tokens.expires_in_s;
    }
    return err;
}

esp_err_t athom_cloud_refresh(athom_cloud_state_t *state)
{
    if (state == NULL || state->tokens.refresh_token[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    char encoded_refresh[ATHOM_TOKEN_MAX * 3U];
    if (!url_encode(state->tokens.refresh_token,
                    encoded_refresh, sizeof(encoded_refresh))) {
        zero_secure(encoded_refresh, sizeof(encoded_refresh));
        return ESP_ERR_INVALID_SIZE;
    }

    char *body = malloc(strlen(encoded_refresh) + 64U);
    if (body == NULL) {
        zero_secure(encoded_refresh, sizeof(encoded_refresh));
        return ESP_ERR_NO_MEM;
    }
    snprintf(body, strlen(encoded_refresh) + 64U,
             "grant_type=refresh_token&refresh_token=%s", encoded_refresh);
    zero_secure(encoded_refresh, sizeof(encoded_refresh));

    esp_err_t err = token_request(body, &state->tokens, true);
    zero_secure(body, strlen(body));
    free(body);
    if (err == ESP_OK) {
        state->expires_at_s =
            (uint64_t)(esp_timer_get_time() / 1000000LL) +
            state->tokens.expires_in_s;
    }
    return err;
}

static esp_err_t parse_homeys(const char *json, athom_homey_list_t *list)
{
    diagnostic_set("oauth_user_me_parse", ESP_OK);

    cJSON *root = cJSON_Parse(json);

    if (root == NULL) {
        diagnostic_set(
            "oauth_user_me_parse",
            ESP_ERR_INVALID_RESPONSE);
        transport_stage_failure(ATHOM_TRANSPORT_PARSE_FAIL, "oauth_user_me_parse", ESP_ERR_INVALID_RESPONSE, 0);
        return ESP_ERR_INVALID_RESPONSE;
    }

    diagnostic_set("oauth_homey_parse", ESP_OK);
    cJSON *homeys = cJSON_GetObjectItemCaseSensitive(root, "homeys");
    if (homeys == NULL) {
        cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "result");
        if (cJSON_IsObject(result)) {
            homeys = cJSON_GetObjectItemCaseSensitive(result, "homeys");
        }
    }

    memset(list, 0, sizeof(*list));
    if (cJSON_IsArray(homeys)) {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, homeys) {
            if (list->count >= ATHOM_HOMEY_MAX || !cJSON_IsObject(item)) break;
            athom_homey_t *homey = &list->items[list->count];
            if (!copy_optional(homey->id, sizeof(homey->id), item, "_id")) {
                cJSON_Delete(root);
                return ESP_ERR_INVALID_SIZE;
            }
            if (homey->id[0] == '\0' &&
                !copy_optional(homey->id, sizeof(homey->id), item, "id")) {
                cJSON_Delete(root);
                return ESP_ERR_INVALID_SIZE;
            }
            if (!copy_optional(homey->name, sizeof(homey->name), item, "name") ||
                !copy_optional(homey->platform, sizeof(homey->platform), item, "platform") ||
                !copy_optional(homey->local_url_secure, sizeof(homey->local_url_secure), item, "localUrlSecure") ||
                !copy_optional(homey->local_url, sizeof(homey->local_url), item, "localUrl") ||
                !copy_optional(homey->remote_url, sizeof(homey->remote_url), item, "remoteUrl")) {
                cJSON_Delete(root);
                return ESP_ERR_INVALID_SIZE;
            }
            if (homey->id[0] != '\0') list->count++;
        }
    } else if (cJSON_IsObject(homeys)) {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, homeys) {
            if (list->count >= ATHOM_HOMEY_MAX || !cJSON_IsObject(item)) break;
            athom_homey_t *homey = &list->items[list->count];
            if (!copy_optional(homey->id, sizeof(homey->id), item, "_id") ||
                !copy_optional(homey->name, sizeof(homey->name), item, "name") ||
                !copy_optional(homey->platform, sizeof(homey->platform), item, "platform") ||
                !copy_optional(homey->local_url_secure, sizeof(homey->local_url_secure), item, "localUrlSecure") ||
                !copy_optional(homey->local_url, sizeof(homey->local_url), item, "localUrl") ||
                !copy_optional(homey->remote_url, sizeof(homey->remote_url), item, "remoteUrl")) {
                cJSON_Delete(root);
                return ESP_ERR_INVALID_SIZE;
            }
            if (homey->id[0] != '\0') list->count++;
        }
    }
    cJSON_Delete(root);
    return list->count > 0U ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t athom_cloud_fetch_user_homeys(athom_cloud_state_t *state)
{
    transport_memory_log("BOOTSTRAP_BEGIN", 0U);
    const size_t patch019a13_access_len = state != NULL
        ? strnlen(state->tokens.access_token, ATHOM_TOKEN_MAX)
        : 0U;
    const bool patch019a13_access_present = patch019a13_access_len > 0U;
    const bool patch019a13_access_length_valid =
        patch019a13_access_len > 0U && patch019a13_access_len < ATHOM_TOKEN_MAX;
    if (state == NULL || state->tokens.access_token[0] == '\0') {
        ESP_LOGI(
            TAG,
            "PATCH019A13_AUTH role=CLOUD access_token_present=%s "
            "access_token_length_valid=%s authorization_constructed=false privacy=sanitized",
            patch019a13_access_present ? "true" : "false",
            patch019a13_access_length_valid ? "true" : "false");
        return ESP_ERR_INVALID_ARG;
    }
    char authorization[ATHOM_TOKEN_MAX + 16U];
    esp_err_t err = bearer_authorization(
        state->tokens.access_token, authorization, sizeof(authorization));
    ESP_LOGI(
        TAG,
        "PATCH019A13_AUTH role=CLOUD access_token_present=%s "
        "access_token_length_valid=%s authorization_constructed=%s privacy=sanitized",
        patch019a13_access_present ? "true" : "false",
        patch019a13_access_length_valid ? "true" : "false",
        err == ESP_OK ? "true" : "false");
    if (err != ESP_OK) return err;

    char *response = NULL;
    int status = 0;

    diagnostic_set("oauth_user_me_request", ESP_OK);

    err = http_request(
        ATHOM_USER_URL, HTTP_METHOD_GET, authorization,
        NULL, NULL, &response, &status);

    zero_secure(authorization, sizeof(authorization));

    if (err != ESP_OK) {
        if (status > 0) {
            diagnostic_set_http(
                "oauth_user_me_request",
                err,
                status);
        } else {
            diagnostic_set("oauth_user_me_request", err);
        }
        return err;
    }

    diagnostic_set_http("oauth_user_me_http", ESP_OK, status);

    if (status == 401) {
        diagnostic_set_http(
            "oauth_user_me_http",
            ESP_ERR_INVALID_STATE,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return ESP_ERR_INVALID_STATE;
    }

    if (status < 200 || status >= 300) {
        diagnostic_set_http(
            "oauth_user_me_http",
            ESP_FAIL,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return ESP_FAIL;
    }

    err = parse_homeys(response, &state->homeys);

    if (err != ESP_OK) {
        if (
            athom_cloud_diagnostic_stage() == NULL ||
            strcmp(
                athom_cloud_diagnostic_stage(),
                "oauth_user_me_parse") != 0
        ) {
            diagnostic_set("oauth_homey_parse", err);
        }
    } else {
        diagnostic_set("oauth_complete", ESP_OK);
    }

    zero_secure(response, HTTP_BODY_MAX);
    free(response);
    transport_memory_log("AFTER_CLOUD", 0U);
    return err;
}

static esp_err_t parse_json_string_token(
    const char *json,
    char *out,
    size_t capacity)
{
    if (json == NULL || out == NULL || capacity == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (!cJSON_IsString(root) ||
        root->valuestring == NULL ||
        root->valuestring[0] == 0) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    size_t token_length = strlen(root->valuestring);

    if (token_length >= capacity) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(out, root->valuestring, token_length + 1U);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t delegation_token(
    const char *access_token, char *out, size_t capacity)
{
    char authorization[ATHOM_TOKEN_MAX + 16U];
    esp_err_t err = bearer_authorization(access_token, authorization, sizeof(authorization));
    if (err != ESP_OK) return err;
    char *response = NULL;
    int status = 0;
    diagnostic_set("delegation_request", ESP_OK);

    err = http_request(
        "https://api.athom.com/delegation/token?audience=homey",
        HTTP_METHOD_POST,
        authorization,
        "application/json",
        "{}",
        &response,
        &status);

    zero_secure(authorization, sizeof(authorization));

    if (err != ESP_OK) {
        diagnostic_set("delegation_request", err);
        return err;
    }

    diagnostic_set_http("delegation_http", ESP_OK, status);

    if (status < 200 || status >= 300) {
        diagnostic_set_http(
            "delegation_http",
            ESP_FAIL,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return ESP_FAIL;
    }

    diagnostic_set("delegation_parse", ESP_OK);
    err = parse_json_string_token(response, out, capacity);

    if (err != ESP_OK) {
        diagnostic_set("delegation_parse", err);
    }

    zero_secure(response, HTTP_BODY_MAX);
    free(response);
    return err;
}

static esp_err_t homey_login(
    const char *base_url,
    const char *delegation,
    char *session,
    size_t session_capacity)
{
    char url[ATHOM_HOMEY_URL_MAX + 64U];
    int written = snprintf(url, sizeof(url), "%s/api/manager/users/login", base_url);
    if (written <= 0 || (size_t)written >= sizeof(url)) return ESP_ERR_INVALID_SIZE;

    char *escaped = cJSON_PrintUnformatted(cJSON_CreateString(delegation));
    if (escaped == NULL) return ESP_ERR_NO_MEM;
    size_t body_len = strlen(escaped) + 16U;
    char *body = malloc(body_len);
    if (body == NULL) {
        zero_secure(escaped, strlen(escaped));
        free(escaped);
        return ESP_ERR_NO_MEM;
    }
    snprintf(body, body_len, "{\"token\":%s}", escaped);
    zero_secure(escaped, strlen(escaped));
    free(escaped);

    char *response = NULL;
    int status = 0;

    diagnostic_set("homey_login_request", ESP_OK);

    esp_err_t err = http_request(
        url,
        HTTP_METHOD_POST,
        NULL,
        "application/json",
        body,
        &response,
        &status);

    zero_secure(body, body_len);
    free(body);

    if (err != ESP_OK) {
        diagnostic_set("homey_login_request", err);
        return err;
    }

    diagnostic_set_http("homey_login_http", ESP_OK, status);

    if (status < 200 || status >= 300) {
        diagnostic_set_http(
            "homey_login_http",
            ESP_FAIL,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return ESP_FAIL;
    }

    diagnostic_set("homey_login_parse", ESP_OK);
    err = parse_json_string_token(
        response,
        session,
        session_capacity);

    if (err != ESP_OK) {
        diagnostic_set("homey_login_parse", err);
    }

    zero_secure(response, HTTP_BODY_MAX);
    free(response);
    return err;
}

esp_err_t athom_cloud_select_and_connect(
    athom_cloud_state_t *state,
    const char *homey_id)
{
    if (state == NULL || homey_id == NULL) {
        diagnostic_set("argument_validation", ESP_ERR_INVALID_ARG);
        return ESP_ERR_INVALID_ARG;
    }

    athom_cloud_alias_invalidate();
    diagnostic_set("homey_lookup", ESP_OK);
    const athom_homey_t *selected = athom_homey_find_exact(&state->homeys, homey_id);
    if (selected == NULL) {
        diagnostic_set("homey_lookup", ESP_ERR_NOT_FOUND);
        return ESP_ERR_NOT_FOUND;
    }

    diagnostic_set("url_selection", ESP_OK);
    if (selected->remote_url[0] == 0) {
        diagnostic_set("url_selection", ESP_ERR_NOT_FOUND);
        transport_stage_failure(ATHOM_TRANSPORT_NO_VALID_ENDPOINT,
                                "homey_login_remote", ESP_ERR_NOT_FOUND, 0);
        return ESP_ERR_NOT_FOUND;
    }

    char delegation[ATHOM_TOKEN_MAX];
    esp_err_t err = delegation_token(state->tokens.access_token, delegation, sizeof(delegation));
    if (err != ESP_OK) {
        zero_secure(delegation, sizeof(delegation));
        return err;
    }

    err = patch019a17_cloud_to_homey_handoff();
    if (err != ESP_OK) {
        zero_secure(delegation, sizeof(delegation));
        diagnostic_set("cloud_to_homey_handoff", err);
        return err;
    }

    transport_memory_log("BEFORE_HOMEY_LOGIN", 0U);
    char session[ATHOM_TOKEN_MAX] = {0};
    diagnostic_set("homey_login_remote", ESP_OK);
    s_transport_metrics.homey_session_create_count++;
    err = homey_login(selected->remote_url, delegation, session, sizeof(session));
    zero_secure(delegation, sizeof(delegation));
    transport_memory_log("AFTER_HOMEY_LOGIN", 0U);

    if (err != ESP_OK) {
        zero_secure(session, sizeof(session));
        transport_stage_failure(ATHOM_TRANSPORT_HOMEY_SESSION_FAIL,
                                "homey_login_remote", err,
                                athom_cloud_diagnostic_http_status());
        return err;
    }

    state->selected_homey = *selected;
    /* Patch019A is intentionally REMOTE-only. Keep parsed local URLs in the
     * discovery model, but remove them from the selected runtime endpoint. */
    state->selected_homey.local_url_secure[0] = 0;
    state->selected_homey.local_url[0] = 0;
    memcpy(state->homey_session_token, session, strlen(session) + 1U);
    zero_secure(session, sizeof(session));
    diagnostic_set("session_ready", ESP_OK);
    (void)athom_cloud_alias_activate(state->selected_homey.id);
    return ESP_OK;
}

static const char *homey_schema_json_type(const cJSON *item)
{
    if (cJSON_IsBool(item)) return "bool";
    if (cJSON_IsString(item)) return "string";
    if (cJSON_IsNumber(item)) return "number";
    if (cJSON_IsArray(item)) return "array";
    if (cJSON_IsObject(item)) return "object";
    if (cJSON_IsNull(item)) return "null";
    return "unknown";
}

static bool homey_schema_is_favorite_key(const char *key)
{
    if (key == NULL) return false;
    return strcmp(key, "favorite") == 0 ||
           strcmp(key, "favourite") == 0 ||
           strcmp(key, "isFavorite") == 0 ||
           strcmp(key, "isFavourite") == 0;
}

static void homey_schema_log_object_keys(
    size_t device_index,
    const cJSON *object)
{
    const cJSON *field = NULL;
    cJSON_ArrayForEach(field, object) {
        if (field->string == NULL) continue;
        ESP_LOGI(
            TAG,
            "HOMEY_SCHEMA device_index=%u top_key=%s type=%s",
            (unsigned)device_index,
            field->string,
            homey_schema_json_type(field));
        if (homey_schema_is_favorite_key(field->string)) {
            ESP_LOGI(
                TAG,
                "HOMEY_SCHEMA device_index=%u favorite_marker=%s type=%s",
                (unsigned)device_index,
                field->string,
                homey_schema_json_type(field));
        }
    }
}

static void homey_schema_log_settings_markers(
    size_t device_index,
    const cJSON *device)
{
    const cJSON *settings =
        cJSON_GetObjectItemCaseSensitive(device, "settings");
    if (!cJSON_IsObject(settings)) return;
    const cJSON *field = NULL;
    cJSON_ArrayForEach(field, settings) {
        if (field->string != NULL && homey_schema_is_favorite_key(field->string)) {
            ESP_LOGI(
                TAG,
                "HOMEY_SCHEMA device_index=%u favorite_marker=settings.%s type=%s",
                (unsigned)device_index,
                field->string,
                homey_schema_json_type(field));
        }
    }
}

static void homey_schema_log_capabilities(
    size_t device_index,
    const cJSON *device)
{
    const cJSON *capabilities =
        cJSON_GetObjectItemCaseSensitive(device, "capabilities");
    if (cJSON_IsArray(capabilities)) {
        const cJSON *capability = NULL;
        cJSON_ArrayForEach(capability, capabilities) {
            if (cJSON_IsString(capability) && capability->valuestring != NULL) {
                ESP_LOGI(
                    TAG,
                    "HOMEY_SCHEMA device_index=%u capability_name=%s source=capabilities",
                    (unsigned)device_index,
                    capability->valuestring);
            }
        }
    }

    const cJSON *capabilities_object =
        cJSON_GetObjectItemCaseSensitive(device, "capabilitiesObj");
    if (cJSON_IsObject(capabilities_object)) {
        const cJSON *capability = NULL;
        cJSON_ArrayForEach(capability, capabilities_object) {
            if (capability->string != NULL) {
                ESP_LOGI(
                    TAG,
                    "HOMEY_SCHEMA device_index=%u capability_name=%s source=capabilitiesObj type=%s",
                    (unsigned)device_index,
                    capability->string,
                    homey_schema_json_type(capability));
            }
        }
    }
}

static void homey_schema_log_inventory(const char *json)
{
    if (json == NULL) return;
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        ESP_LOGW(TAG, "HOMEY_SCHEMA parse=failed");
        return;
    }

    cJSON *collection = cJSON_GetObjectItemCaseSensitive(root, "result");
    if (collection == NULL) collection = root;

    size_t index = 0U;
    const cJSON *device = NULL;
    if (cJSON_IsArray(collection)) {
        cJSON_ArrayForEach(device, collection) {
            if (!cJSON_IsObject(device)) continue;
            ESP_LOGI(
                TAG,
                "HOMEY_SCHEMA device_index=%u device_label=device_%03u",
                (unsigned)index,
                (unsigned)index);
            homey_schema_log_object_keys(index, device);
            homey_schema_log_settings_markers(index, device);
            homey_schema_log_capabilities(index, device);
            index++;
        }
    } else if (cJSON_IsObject(collection)) {
        cJSON_ArrayForEach(device, collection) {
            if (!cJSON_IsObject(device)) continue;
            ESP_LOGI(
                TAG,
                "HOMEY_SCHEMA device_index=%u device_label=device_%03u",
                (unsigned)index,
                (unsigned)index);
            homey_schema_log_object_keys(index, device);
            homey_schema_log_settings_markers(index, device);
            homey_schema_log_capabilities(index, device);
            index++;
        }
    }

    ESP_LOGI(TAG, "HOMEY_SCHEMA device_count=%u complete=true", (unsigned)index);
    cJSON_Delete(root);
}

static esp_err_t favorites_fetch_user_me(
    const char *base_url,
    const char *session_token,
    char **response_out,
    size_t *response_capacity_out,
    int *status_out)
{
    if (base_url == NULL || session_token == NULL || response_out == NULL ||
        response_capacity_out == NULL || status_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char url[ATHOM_HOMEY_URL_MAX + 128U];
    int written = snprintf(
        url,
        sizeof(url),
        "%s/api/manager/users/user/me",
        base_url);
    if (written <= 0 || (size_t)written >= sizeof(url)) return ESP_ERR_INVALID_SIZE;

    char authorization[ATHOM_TOKEN_MAX + 16U];
    esp_err_t err = bearer_authorization(
        session_token,
        authorization,
        sizeof(authorization));
    if (err != ESP_OK) return err;

    *response_out = NULL;
    *response_capacity_out = 0U;
    *status_out = 0;
    err = http_request_limited(
        url,
        HTTP_METHOD_GET,
        authorization,
        NULL,
        NULL,
        response_out,
        status_out,
        HTTP_BODY_MAX,
        response_capacity_out);
    zero_secure(authorization, sizeof(authorization));
    return err;
}

static esp_err_t count_collection(
    const char *base_url,
    const char *path,
    const char *session_token,
    size_t *count_out,
    size_t response_maximum,
    bool publish_device_snapshot,
    const char *favorite_user_json)
{
    char url[ATHOM_HOMEY_URL_MAX + 128U];
    int written =
        snprintf(url, sizeof(url), "%s%s", base_url, path);

    if (written <= 0 ||
        (size_t)written >= sizeof(url)) {
        return ESP_ERR_INVALID_SIZE;
    }

    char authorization[ATHOM_TOKEN_MAX + 16U];
    esp_err_t err = bearer_authorization(
        session_token,
        authorization,
        sizeof(authorization));

    if (err != ESP_OK) return err;

    char *response = NULL;
    size_t response_capacity = 0U;
    int status = 0;

    err = http_request_limited(
        url,
        HTTP_METHOD_GET,
        authorization,
        NULL,
        NULL,
        &response,
        &status,
        response_maximum,
        &response_capacity);

    zero_secure(authorization, sizeof(authorization));

    if (err != ESP_OK) return err;

    if (status < 200 || status >= 300) {
        zero_secure(response, response_capacity);
        free(response);
        return ESP_FAIL;
    }

    if (publish_device_snapshot) {
        transport_memory_log("DEVICES_RESPONSE_RECEIVED", strlen(response));
        ensure_device_snapshot_store();
        const panel_homey_alias_provider_t provider = {
            .context = &s_alias_runtime,
            .resolve = panel_homey_alias_runtime_resolve,
        };
        panel_homey_read_result_t snapshot_result =
            panel_homey_snapshot_publish_json(
                &s_device_snapshot_store,
                response,
                &provider,
                (uint64_t)(esp_timer_get_time() / 1000LL));
        if (favorite_user_json == NULL) {
            panel_homey_favorites_clear();
        } else if (panel_homey_favorites_parse_and_publish(
                       favorite_user_json,
                       response) != PANEL_HOMEY_FAVORITES_OK) {
            panel_homey_favorites_clear();
            ESP_LOGW(TAG, "HOMEY_FAVORITES authoritative_binding=unavailable");
        }
        homey_schema_log_inventory(response);
        ESP_LOGI(
            TAG,
            "HOMEY_SNAPSHOT provider=not_configured result=%d",
            (int)snapshot_result);
    }

    cJSON *root = cJSON_Parse(response);

    zero_secure(response, response_capacity);
    free(response);

    if (root == NULL) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    cJSON *value =
        cJSON_GetObjectItemCaseSensitive(root, "result");

    if (value == NULL) value = root;

    size_t count = 0U;

    if (cJSON_IsArray(value)) {
        count = (size_t)cJSON_GetArraySize(value);
    } else if (cJSON_IsObject(value)) {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, value) {
            count++;
        }
    }

    cJSON_Delete(root);
    *count_out = count;
    return ESP_OK;
}



esp_err_t athom_cloud_fetch_inventory(athom_cloud_state_t *state)
{
    if (state == NULL || state->homey_session_token[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    (void)athom_cloud_alias_activate(state->selected_homey.id);
    const char *base_url = state->selected_homey.remote_url;
    if (base_url == NULL || base_url[0] == 0) {
        transport_stage_failure(ATHOM_TRANSPORT_NO_VALID_ENDPOINT, "inventory_remote", ESP_ERR_NOT_FOUND, 0);
        return ESP_ERR_NOT_FOUND;
    }

    char *favorite_user_json = NULL;
    size_t favorite_user_capacity = 0U;
    int favorite_user_status = 0;
    transport_memory_log("BEFORE_FAVORITES", 0U);
    esp_err_t favorite_user_err = favorites_fetch_user_me(
        base_url,
        state->homey_session_token,
        &favorite_user_json,
        &favorite_user_capacity,
        &favorite_user_status);
    transport_memory_log("AFTER_FAVORITES", favorite_user_capacity);
    bool favorite_user_blocked_by_scope = favorite_user_status == 403;
    if (favorite_user_err != ESP_OK || favorite_user_status < 200 || favorite_user_status >= 300) {
        panel_homey_favorites_clear();
        transport_stage_failure(ATHOM_TRANSPORT_FAVORITES_FAIL, "favorites_user_me", favorite_user_err, favorite_user_status);
        if (favorite_user_blocked_by_scope) {
            ESP_LOGW(TAG, "HOMEY_FAVORITES user_me_access=BLOCKED_BY_SCOPE http_status=403");
        } else {
            ESP_LOGW(
                TAG,
                "HOMEY_FAVORITES user_me_access=UNAVAILABLE err=%s http_status=%d",
                esp_err_to_name(favorite_user_err),
                favorite_user_status);
        }
        if (favorite_user_json != NULL) {
            zero_secure(favorite_user_json, favorite_user_capacity);
            free(favorite_user_json);
            favorite_user_json = NULL;
            favorite_user_capacity = 0U;
        }
    } else {
        ESP_LOGI(TAG, "HOMEY_FAVORITES user_me_access=SUCCESS source=user_properties.favoriteDevices");
    }

    diagnostic_set("inventory_zones", ESP_OK);

    esp_err_t err = count_collection(
        base_url,
        "/api/manager/zones/zone",
        state->homey_session_token,
        &state->zone_count,
        HTTP_BODY_MAX,
        false,
        NULL);

    if (err != ESP_OK) {
        if (favorite_user_json != NULL) {
            zero_secure(favorite_user_json, favorite_user_capacity);
            free(favorite_user_json);
            favorite_user_json = NULL;
            favorite_user_capacity = 0U;
        }
        diagnostic_set("inventory_zones", err);
        transport_stage_failure(ATHOM_TRANSPORT_ZONES_FAIL, "inventory_zones", err, athom_cloud_diagnostic_http_status());
        return err;
    }

    diagnostic_set("inventory_devices", ESP_OK);
    transport_memory_log("BEFORE_DEVICES", 0U);

    err = count_collection(
        base_url,
        "/api/manager/devices/device",
        state->homey_session_token,
        &state->device_count,
        HTTP_INVENTORY_BODY_MAX,
        true,
        favorite_user_json);

    if (favorite_user_json != NULL) {
        zero_secure(favorite_user_json, favorite_user_capacity);
        free(favorite_user_json);
        favorite_user_json = NULL;
        favorite_user_capacity = 0U;
    }

    transport_memory_log("AFTER_DEVICES_PARSE", 0U);
    if (err != ESP_OK) {
        diagnostic_set("inventory_devices", err);
        transport_stage_failure(ATHOM_TRANSPORT_DEVICES_FAIL, "inventory_devices", err, athom_cloud_diagnostic_http_status());
        return err;
    }

    if (favorite_user_blocked_by_scope) {
        diagnostic_set_http("favorites_user_me_blocked_by_scope", ESP_FAIL, 403);
    } else if (favorite_user_err != ESP_OK || favorite_user_status < 200 || favorite_user_status >= 300) {
        diagnostic_set_http("favorites_user_me_unavailable", favorite_user_err, favorite_user_status);
    } else {
        diagnostic_set("inventory_complete", ESP_OK);
    }
    transport_memory_log("BOOTSTRAP_END", 0U);
    return ESP_OK;
}
#endif
