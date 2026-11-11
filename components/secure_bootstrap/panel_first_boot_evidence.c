#include "panel_first_boot_evidence.h"

#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
static portMUX_TYPE s_first_boot_mux = portMUX_INITIALIZER_UNLOCKED;
#define FIRST_BOOT_LOCK() portENTER_CRITICAL(&s_first_boot_mux)
#define FIRST_BOOT_UNLOCK() portEXIT_CRITICAL(&s_first_boot_mux)
#else
#include <pthread.h>
static pthread_mutex_t s_first_boot_mux = PTHREAD_MUTEX_INITIALIZER;
#define FIRST_BOOT_LOCK() ((void)pthread_mutex_lock(&s_first_boot_mux))
#define FIRST_BOOT_UNLOCK() ((void)pthread_mutex_unlock(&s_first_boot_mux))
#endif

_Static_assert(sizeof(panel_first_boot_evidence_t) <= 256U,
               "first-boot evidence RAM budget");

static panel_first_boot_evidence_t s_first_boot;
static bool s_initialized;

void panel_first_boot_evidence_init(uint32_t runtime_id)
{
    FIRST_BOOT_LOCK();
    if (s_initialized && s_first_boot.runtime_id == runtime_id) {
        FIRST_BOOT_UNLOCK();
        return;
    }
    memset(&s_first_boot, 0, sizeof(s_first_boot));
    s_first_boot.runtime_id = runtime_id;
    s_first_boot.queue_first_result = -1;
    s_first_boot.queue_last_result = -1;
    s_first_boot.inventory.final_http_status = -1;
    s_first_boot.inventory.favorites_http_status = -1;
    s_first_boot.inventory.zones_http_status = -1;
    s_first_boot.inventory.devices_expected_bytes = -1;
    s_first_boot.inventory.devices_http_status = -1;
    s_initialized = true;
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_auth_started(uint32_t now_ms)
{
    FIRST_BOOT_LOCK();
    if (s_first_boot.auth_state == 0U) {
        s_first_boot.auth_started_ms = now_ms;
        s_first_boot.auth_state = 1U;
    }
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_auth_result(uint32_t now_ms, int32_t error,
                                           bool record_present,
                                           bool selected_homey_present)
{
    (void)now_ms;
    FIRST_BOOT_LOCK();
    if (s_first_boot.auth_state == 0U || s_first_boot.auth_state == 1U) {
        s_first_boot.auth_error = error;
        s_first_boot.selected_homey_present = selected_homey_present;
        s_first_boot.auth_state = error != 0 ? 4U : (record_present ? 2U : 3U);
    }
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_scheduler_started(uint32_t now_ms)
{
    FIRST_BOOT_LOCK();
    if (!s_first_boot.scheduler_started) {
        s_first_boot.scheduler_started_ms = now_ms;
        s_first_boot.scheduler_started = true;
    }
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_scheduler_failed(uint8_t reason)
{
    FIRST_BOOT_LOCK();
    if (!s_first_boot.scheduler_started) s_first_boot.scheduler_skip_reason = reason;
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_scheduler_skipped(uint8_t reason)
{
    FIRST_BOOT_LOCK();
    if (s_first_boot.scheduler_skip_reason == 0U)
        s_first_boot.scheduler_skip_reason = reason;
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_queue_result(int8_t result)
{
    FIRST_BOOT_LOCK();
    if (s_first_boot.inventory_state == PANEL_FIRST_BOOT_INVENTORY_NOT_STARTED) {
        if (s_first_boot.queue_attempts == 0U) s_first_boot.queue_first_result = result;
        if (s_first_boot.queue_attempts != UINT8_MAX) s_first_boot.queue_attempts++;
        s_first_boot.queue_last_result = result;
    }
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_inventory_started(uint8_t origin,
                                                  uint32_t attempt,
                                                  uint32_t sequence,
                                                  uint32_t now_ms)
{
    /* BOOT_AUTO is the existing origin value 0. Only its first actual attempt
     * is captured; later retry/periodic work cannot replace it. */
    if (origin != 0U || attempt != 1U) return;
    FIRST_BOOT_LOCK();
    if (s_first_boot.inventory_state == PANEL_FIRST_BOOT_INVENTORY_NOT_STARTED) {
        s_first_boot.inventory.sequence = sequence;
        s_first_boot.inventory.attempt = attempt;
        s_first_boot.inventory.started_ms = now_ms;
        s_first_boot.inventory.final_http_status = -1;
        s_first_boot.inventory.favorites_http_status = -1;
        s_first_boot.inventory.zones_http_status = -1;
        s_first_boot.inventory.devices_http_status = -1;
        s_first_boot.inventory.devices_expected_bytes = -1;
        s_first_boot.inventory_state = PANEL_FIRST_BOOT_INVENTORY_IN_PROGRESS;
    }
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_inventory_completed(
    const panel_first_boot_attempt_result_t *result)
{
    if (result == NULL) return;
    FIRST_BOOT_LOCK();
    if (s_first_boot.inventory_state == PANEL_FIRST_BOOT_INVENTORY_IN_PROGRESS &&
        result->origin == 0U &&
        s_first_boot.inventory.sequence == result->sequence &&
        s_first_boot.inventory.attempt == result->attempt) {
        panel_first_boot_inventory_t *inventory = &s_first_boot.inventory;
        inventory->completed_ms = result->completed_ms;
        inventory->final_error = result->final_error;
        inventory->final_http_status = result->final_http_status;
        inventory->stage = result->stage;
        inventory->snapshot_published = result->snapshot_published;
        inventory->favorites_attempted = result->favorites_attempted;
        inventory->favorites_error = result->favorites_error;
        inventory->favorites_http_status = result->favorites_http_status;
        inventory->favorites_elapsed_ms = result->favorites_elapsed_ms;
        inventory->zones_result = result->stage == 2U ? 2U :
            (result->stage >= 3U || result->verified ? 1U : 0U);
        inventory->zones_http_status = result->stage == 2U
            ? result->final_http_status : -1;
        inventory->devices_available = result->devices_available;
        inventory->devices_error = result->devices_error;
        inventory->devices_http_status = result->devices_http_status;
        inventory->devices_expected_bytes = result->devices_expected_bytes;
        inventory->devices_received_bytes = result->devices_received_bytes;
        inventory->devices_elapsed_ms = result->devices_elapsed_ms;
        inventory->devices_body_complete = result->devices_body_complete;
        if (result->favorites_transport.available &&
            result->favorites_transport.request_sequence != 0U &&
            result->favorites_transport.request_count != 0U &&
            result->favorites_transport.captured_request != 0U &&
            result->favorites_transport.captured_request <=
                result->favorites_transport.request_count) {
            inventory->favorites_transport = result->favorites_transport;
        }
        if (result->zones_transport.available &&
            result->zones_transport.request_sequence != 0U &&
            result->zones_transport.request_count != 0U &&
            result->zones_transport.captured_request != 0U &&
            result->zones_transport.captured_request <=
                result->zones_transport.request_count) {
            inventory->zones_transport = result->zones_transport;
        }
        s_first_boot.inventory_state = result->verified
            ? PANEL_FIRST_BOOT_INVENTORY_SUCCESS
            : PANEL_FIRST_BOOT_INVENTORY_FAILED;
    }
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_recovery_used(uint8_t origin, uint32_t attempt)
{
    if (origin == 0U && attempt <= 1U) return;
    FIRST_BOOT_LOCK();
    if (!s_first_boot.first_ready_reached) s_first_boot.recovery_used_before_first_ready = true;
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_ready(uint8_t origin, uint32_t attempt,
                                     uint32_t now_ms)
{
    FIRST_BOOT_LOCK();
    if (!s_first_boot.first_ready_reached) {
        s_first_boot.first_ready_reached = true;
        s_first_boot.first_ready_origin = origin;
        s_first_boot.first_ready_attempt = attempt;
        s_first_boot.first_ready_elapsed_ms = now_ms;
    }
    FIRST_BOOT_UNLOCK();
}

void panel_first_boot_evidence_copy(panel_first_boot_evidence_t *out)
{
    if (out == NULL) return;
    FIRST_BOOT_LOCK();
    *out = s_first_boot;
    FIRST_BOOT_UNLOCK();
}

static const char *inventory_state_name(panel_first_boot_inventory_state_t state)
{
    switch (state) {
    case PANEL_FIRST_BOOT_INVENTORY_NOT_STARTED: return "not_started";
    case PANEL_FIRST_BOOT_INVENTORY_IN_PROGRESS: return "in_progress";
    case PANEL_FIRST_BOOT_INVENTORY_FAILED: return "failed";
    case PANEL_FIRST_BOOT_INVENTORY_SUCCESS: return "completed_successfully";
    default: return "unknown";
    }
}

static bool append_transport_json(char *body, size_t capacity, size_t *offset,
                                  const panel_first_boot_transport_t *transport)
{
    const uint8_t fields = transport->available_fields;
    int n = snprintf(body + *offset, capacity - *offset,
        "{\"available\":%s,\"request_sequence\":",
        transport->available ? "true" : "false");
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    if (transport->available) {
        n = snprintf(body + *offset, capacity - *offset, "%u",
                     (unsigned)transport->request_sequence);
    } else {
        n = snprintf(body + *offset, capacity - *offset, "null");
    }
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    n = snprintf(body + *offset, capacity - *offset,
        ",\"request_count\":%u,\"captured_request\":%u,"
        "\"failed_request\":%u,\"perform_error\":",
        (unsigned)transport->request_count,
        (unsigned)transport->captured_request,
        (unsigned)transport->failed_request);
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;

    if (transport->available) {
        n = snprintf(body + *offset, capacity - *offset, "%ld",
                     (long)transport->perform_error);
    } else {
        n = snprintf(body + *offset, capacity - *offset, "null");
    }
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    n = snprintf(body + *offset, capacity - *offset, ",\"http_status\":");
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;

#define APPEND_FIELD(bit, fmt, value) do { \
    if ((fields & (bit)) != 0U) { \
        n = snprintf(body + *offset, capacity - *offset, fmt, value); \
    } else { \
        n = snprintf(body + *offset, capacity - *offset, "null"); \
    } \
    if (n <= 0 || (size_t)n >= capacity - *offset) return false; \
    *offset += (size_t)n; \
} while (0)

    APPEND_FIELD(PANEL_FIRST_BOOT_TRANSPORT_HTTP_STATUS,
                 "%ld", (long)transport->http_status);
    n = snprintf(body + *offset, capacity - *offset, ",\"tls_query\":");
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    APPEND_FIELD(PANEL_FIRST_BOOT_TRANSPORT_TLS, "%ld", (long)transport->tls_query);
    n = snprintf(body + *offset, capacity - *offset, ",\"tls_error\":");
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    APPEND_FIELD(PANEL_FIRST_BOOT_TRANSPORT_TLS, "%ld", (long)transport->tls_error);
    n = snprintf(body + *offset, capacity - *offset, ",\"tls_flags\":");
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    APPEND_FIELD(PANEL_FIRST_BOOT_TRANSPORT_TLS, "%ld", (long)transport->tls_flags);
    n = snprintf(body + *offset, capacity - *offset, ",\"socket_errno\":");
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    APPEND_FIELD(PANEL_FIRST_BOOT_TRANSPORT_SOCKET_ERRNO,
                 "%ld", (long)transport->socket_errno);
    n = snprintf(body + *offset, capacity - *offset, ",\"connect_result\":");
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    APPEND_FIELD(PANEL_FIRST_BOOT_TRANSPORT_CONNECT_RESULT,
                 "%ld", (long)transport->connect_result);
    n = snprintf(body + *offset, capacity - *offset, ",\"elapsed_ms\":");
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    APPEND_FIELD(PANEL_FIRST_BOOT_TRANSPORT_ELAPSED,
                 "%u", (unsigned)transport->elapsed_ms);
    if ((fields & PANEL_FIRST_BOOT_TRANSPORT_CLASSIFICATION) != 0U) {
        n = snprintf(body + *offset, capacity - *offset,
                     ",\"classification\":%u}",
                     (unsigned)transport->classification);
    } else {
        n = snprintf(body + *offset, capacity - *offset,
                     ",\"classification\":null}");
    }
#undef APPEND_FIELD
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    return true;
}

bool panel_first_boot_evidence_append_json(char *body, size_t capacity,
                                            size_t *length)
{
    if (body == NULL || length == NULL || *length < 1U || *length >= capacity ||
        body[*length - 1U] != '}') return false;
    panel_first_boot_evidence_t evidence;
    panel_first_boot_evidence_copy(&evidence);
    size_t offset = *length - 1U;
    const char *separator = (*length == 2U && body[0] == '{') ? "" : ",";
    int written = snprintf(body + offset, capacity - offset,
        "%s\"first_boot_evidence\":{\"version\":1,\"runtime_id\":%u,"
        "\"auth_state\":%u,\"auth_error\":%ld,\"selected_homey_present\":%s,"
        "\"auth_started_ms\":%u,\"scheduler_started\":%s,"
        "\"scheduler_started_ms\":%u,\"scheduler_skip_reason\":%u,"
        "\"queue_attempts\":%u,\"queue_first_result\":%d,\"queue_last_result\":%d,"
        "\"inventory_state\":\"%s\",\"sequence\":%u,\"origin\":0,\"attempt\":%u,"
        "\"started_ms\":%u,\"completed_ms\":%u,\"final_error\":%ld,"
        "\"final_http_status\":%ld,\"failed_stage\":%u,\"snapshot_published\":%s,"
        "\"favorites_attempted\":%s,\"favorites_error\":%ld,\"favorites_http_status\":%ld,"
        "\"favorites_elapsed_ms\":%u,\"zones_result\":%u,\"zones_http_status\":%ld,"
        "\"devices_available\":%s,\"devices_error\":%ld,\"devices_http_status\":%ld,"
        "\"devices_expected_bytes\":%ld,\"devices_received_bytes\":%u,"
        "\"devices_elapsed_ms\":%u,\"devices_body_complete\":%s,"
        "\"heap_minimum_available\":false,\"largest_free_block_minimum_available\":false,"
        "\"recovery_used_before_first_ready\":%s,\"first_ready_reached\":%s,"
        "\"first_ready_origin\":%u,\"first_ready_attempt\":%u,\"first_ready_elapsed_ms\":%u,"
        "\"favorites_transport\":",
        separator,
        (unsigned)evidence.runtime_id, (unsigned)evidence.auth_state,
        (long)evidence.auth_error,
        evidence.selected_homey_present ? "true" : "false",
        (unsigned)evidence.auth_started_ms,
        evidence.scheduler_started ? "true" : "false",
        (unsigned)evidence.scheduler_started_ms,
        (unsigned)evidence.scheduler_skip_reason,
        (unsigned)evidence.queue_attempts, (int)evidence.queue_first_result,
        (int)evidence.queue_last_result,
        inventory_state_name(evidence.inventory_state),
        (unsigned)evidence.inventory.sequence, (unsigned)evidence.inventory.attempt,
        (unsigned)evidence.inventory.started_ms,
        (unsigned)evidence.inventory.completed_ms,
        (long)evidence.inventory.final_error,
        (long)evidence.inventory.final_http_status,
        (unsigned)evidence.inventory.stage,
        evidence.inventory.snapshot_published ? "true" : "false",
        evidence.inventory.favorites_attempted ? "true" : "false",
        (long)evidence.inventory.favorites_error,
        (long)evidence.inventory.favorites_http_status,
        (unsigned)evidence.inventory.favorites_elapsed_ms,
        (unsigned)evidence.inventory.zones_result,
        (long)evidence.inventory.zones_http_status,
        evidence.inventory.devices_available ? "true" : "false",
        (long)evidence.inventory.devices_error,
        (long)evidence.inventory.devices_http_status,
        (long)evidence.inventory.devices_expected_bytes,
        (unsigned)evidence.inventory.devices_received_bytes,
        (unsigned)evidence.inventory.devices_elapsed_ms,
        evidence.inventory.devices_body_complete ? "true" : "false",
        evidence.recovery_used_before_first_ready ? "true" : "false",
        evidence.first_ready_reached ? "true" : "false",
        (unsigned)evidence.first_ready_origin,
        (unsigned)evidence.first_ready_attempt,
        (unsigned)evidence.first_ready_elapsed_ms);
    if (written <= 0 || (size_t)written >= capacity - offset) return false;
    offset += (size_t)written;
    if (!append_transport_json(body, capacity, &offset,
                               &evidence.inventory.favorites_transport)) return false;
    written = snprintf(body + offset, capacity - offset, ",\"zones_transport\":");
    if (written <= 0 || (size_t)written >= capacity - offset) return false;
    offset += (size_t)written;
    if (!append_transport_json(body, capacity, &offset,
                               &evidence.inventory.zones_transport)) return false;
    written = snprintf(body + offset, capacity - offset, "}}");
    if (written <= 0 || (size_t)written >= capacity - offset) return false;
    offset += (size_t)written;
    *length = offset;
    return true;
}
