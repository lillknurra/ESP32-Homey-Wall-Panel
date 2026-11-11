#include "panel_first_failure_diag.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "panel_render_diag.h"
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
#define LOCK() portENTER_CRITICAL(&s_mux)
#define UNLOCK() portEXIT_CRITICAL(&s_mux)
#else
#include <pthread.h>
static pthread_mutex_t s_mux = PTHREAD_MUTEX_INITIALIZER;
#define LOCK() ((void)pthread_mutex_lock(&s_mux))
#define UNLOCK() ((void)pthread_mutex_unlock(&s_mux))
#endif

_Static_assert(sizeof(panel_first_failure_diag_t) <= 160U,
               "first-failure diagnostic RAM budget");

static panel_first_failure_diag_t s_first_failure;
static uint32_t s_runtime_id;

bool panel_first_failure_diag_rgb_snapshot(panel_first_failure_rgb_t *out)
{
    if (out == NULL) return false;
#ifdef ESP_PLATFORM
    panel_diag_metrics_t metrics = {0};
    uint64_t observed_at_us = 0U;
    if (!panel_render_diag_snapshot(&metrics, &observed_at_us)) return false;
    (void)observed_at_us;
    *out = (panel_first_failure_rgb_t){metrics.shortfall,
        metrics.restart_decisions, metrics.restart_paths};
    return true;
#else
    return false;
#endif
}

void panel_first_failure_diag_init(uint32_t runtime_id)
{
    LOCK();
    if (s_runtime_id != runtime_id) {
        memset(&s_first_failure, 0, sizeof(s_first_failure));
        s_runtime_id = runtime_id;
    }
    UNLOCK();
}

void panel_first_failure_diag_capture(const panel_first_failure_diag_t *failure)
{
    if (failure == NULL || !failure->valid || failure->final_error == 0) return;
    LOCK();
    if (s_runtime_id == failure->runtime_id && !s_first_failure.valid) {
        s_first_failure = *failure;
    }
    UNLOCK();
}

void panel_first_failure_diag_copy(panel_first_failure_diag_t *out)
{
    if (out == NULL) return;
    LOCK();
    *out = s_first_failure;
    UNLOCK();
}

static const char *origin_name(uint8_t origin)
{
    switch (origin) {
    case 0U: return "boot_auto";
    case 1U: return "manual";
    case 2U: return "periodic";
    case 3U: return "light_reconciliation";
    case 4U: return "preselection_restore";
    default: return "unknown";
    }
}

static const char *stage_name(uint8_t stage)
{
    switch (stage) {
    case 1U: return "favorites";
    case 2U: return "zones";
    case 3U: return "devices";
    default: return "other_or_unknown";
    }
}

bool panel_first_failure_diag_append_json(char *body, size_t capacity,
                                           size_t *length)
{
    if (body == NULL || length == NULL || *length < 2U || *length >= capacity ||
        body[*length - 1U] != '}') return false;
    panel_first_failure_diag_t d;
    panel_first_failure_diag_copy(&d);
    char *at = body + *length - 1U;
    const size_t remaining = capacity - (*length - 1U);
    const char *separator = (*length == 2U && body[0] == '{') ? "" : ",";
    int n;
    if (!d.valid) {
        n = snprintf(at, remaining, "%s\"first_failed_inventory\":null}",
                     separator);
    } else {
        n = snprintf(at, remaining,
            "%s\"first_failed_inventory\":{\"valid\":true,\"runtime_id\":%" PRIu32
            ",\"sequence\":%" PRIu32 ",\"origin\":\"%s\",\"attempt\":%" PRIu32
            ",\"stage\":\"%s\",\"started_us\":%" PRIu64 ",\"completed_us\":%" PRIu64
            ",\"error\":%" PRId32 ",\"http_status_available\":%s,\"http_status\":%" PRId32
            ",\"request\":{\"available\":%s,\"sequence\":%" PRIu32
            ",\"error\":%" PRId32 ",\"http_status_available\":%s,\"http_status\":%" PRId32
            ",\"elapsed_ms\":%" PRIu32 ",\"tls_query\":%" PRId32
            ",\"tls_error\":%" PRId32 ",\"socket_errno\":%" PRId32
            ",\"connect_result\":",
            separator, d.runtime_id, d.inventory_sequence, origin_name(d.origin), d.attempt,
            stage_name(d.stage), d.started_us, d.completed_us, d.final_error,
            d.http_status > 0 ? "true" : "false",
            d.http_status, d.request_available ? "true" : "false",
            d.request_sequence, d.request_error,
            d.request_http_status > 0 ? "true" : "false",
            d.request_http_status,
            d.request_elapsed_ms, d.tls_query, d.tls_error, d.socket_errno);
        if (n > 0 && (size_t)n < remaining) {
            size_t used = (size_t)n;
            const char *connect = d.connect_result_available ? "" : "null";
            if (d.connect_result_available) {
                n = snprintf(at + used, remaining - used,
                    "%" PRId32, d.connect_result);
            } else {
                n = snprintf(at + used, remaining - used, "%s", connect);
            }
            if (n > 0 && (size_t)n < remaining - used) {
                used += (size_t)n;
                n = snprintf(at + used, remaining - used,
                    "},\"devices\":{\"available\":%s,\"headers_complete\":%s,"
                    "\"content_length\":%" PRId64 ",\"received_bytes\":%" PRIu32
                    ",\"body_complete\":%s,\"last_positive_read_us\":%" PRIu64
                    ",\"terminal\":{\"available\":%s,\"result\":%" PRId32
                    ",\"errno\":%" PRId32 ",\"duration_us\":%" PRIu64 "}},"
                    "\"memory\":{\"available\":%s,\"internal_free\":%" PRIu32
                    ",\"internal_largest\":%" PRIu32 ",\"internal_minimum\":%" PRIu32
                    ",\"psram_free\":%" PRIu32 ",\"psram_largest\":%" PRIu32 "},"
                    "\"rgb\":{\"available\":%s,\"shortfall_delta\":%" PRIu32
                    ",\"restart_decision_delta\":%" PRIu32 ",\"restart_path_delta\":%" PRIu32 "},"
                    "\"socket_close\":{\"correlation_complete\":false,\"uncertain\":true,\"release\":\"unknown\"}}}",
                    d.device_body_available ? "true" : "false",
                    d.headers_complete ? "true" : "false", d.content_length,
                    d.received_bytes, d.body_complete ? "true" : "false",
                    d.last_positive_read_us, d.terminal_available ? "true" : "false",
                    d.terminal_result, d.terminal_errno, d.terminal_read_us,
                    d.memory_available ? "true" : "false", d.internal_free,
                    d.internal_largest, d.internal_minimum, d.psram_free,
                    d.psram_largest, d.rgb_available ? "true" : "false",
                    d.rgb_shortfall_delta, d.rgb_restart_decision_delta,
                    d.rgb_restart_path_delta);
                if (n > 0 && (size_t)n < remaining - used) {
                    *length = *length - 1U + used + (size_t)n;
                    return true;
                }
            }
        }
        return false;
    }
    if (n <= 0 || (size_t)n >= remaining) return false;
    *length = *length - 1U + (size_t)n;
    return true;
}
