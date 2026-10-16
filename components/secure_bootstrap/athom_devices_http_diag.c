#include "athom_devices_http_diag.h"
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#ifdef PATCH046_HOST_TEST
#include "test_patch046_platform.h"
#else
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_err.h"
#include "esp_tls_errors.h"
#endif

/* No critical section spans I/O, parsing, callbacks or formatting. Only the
 * owner updates s_work. GET never reads s_work or resets the one-shot latch. */
static portMUX_TYPE s_devices_mux = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t s_devices_owner;
static bool s_devices_claimed;
static athom_devices_http_diag_t s_devices_work, s_devices_published;

static uint32_t bounded32(size_t value)
{
    return value > UINT32_MAX ? UINT32_MAX : (uint32_t)value;
}
static uint64_t add64(uint64_t a, uint64_t b)
{
    return UINT64_MAX - a < b ? UINT64_MAX : a + b;
}
static uint64_t duration(uint64_t begin, uint64_t end)
{
    return end >= begin ? end - begin : 0U;
}
static void increment(uint32_t *value)
{
    if (*value != UINT32_MAX) ++*value;
}
static void heap_sample(uint32_t *free_bytes, uint32_t *largest, uint32_t *minimum)
{
    const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    *free_bytes = bounded32(heap_caps_get_free_size(caps));
    *largest = bounded32(heap_caps_get_largest_free_block(caps));
    *minimum = bounded32(heap_caps_get_minimum_free_size(caps));
}

bool athom_devices_http_diag_owned(void)
{
    portENTER_CRITICAL(&s_devices_mux);
    const bool owned = s_devices_owner != NULL &&
        s_devices_owner == xTaskGetCurrentTaskHandle();
    portEXIT_CRITICAL(&s_devices_mux);
    return owned;
}
bool athom_devices_http_diag_begin(uint32_t sequence, bool reused)
{
    const int saved_errno = errno;
    portENTER_CRITICAL(&s_devices_mux);
    const bool acquired = !s_devices_claimed;
    if (acquired) {
        s_devices_claimed = true;
        s_devices_owner = xTaskGetCurrentTaskHandle();
    }
    portEXIT_CRITICAL(&s_devices_mux);
    if (acquired) {
        memset(&s_devices_work, 0, sizeof(s_devices_work));
        s_devices_work.request_sequence = sequence;
        s_devices_work.client_reused = reused;
        s_devices_work.content_length = -1;
        heap_sample(&s_devices_work.internal_free_before,
                    &s_devices_work.internal_largest_before,
                    &s_devices_work.internal_min_before);
        s_devices_work.started_us = (uint64_t)esp_timer_get_time();
    }
    errno = saved_errno;
    return acquired;
}
void athom_devices_http_diag_connect(int result)
{
    increment(&s_devices_work.connect_calls);
    s_devices_work.connect_result = result;
}
void athom_devices_http_diag_write(void)
{
    if (!s_devices_work.connection_reuse_known) {
        s_devices_work.connection_reuse_known = true;
        s_devices_work.connection_reused = s_devices_work.connect_calls == 0U;
    }
}
void athom_devices_http_diag_read(int result, int saved_errno,
                                 uint64_t begin, uint64_t end)
{
    increment(&s_devices_work.read_calls);
    s_devices_work.last_read_end_us = end;
    const uint64_t elapsed = duration(begin, end);
    if (elapsed > s_devices_work.read_max_us) s_devices_work.read_max_us = elapsed;
    if (result > 0) {
        s_devices_work.wire_bytes = add64(s_devices_work.wire_bytes, (uint64_t)result);
    } else if (!s_devices_work.terminal_seen) {
        s_devices_work.terminal_seen = true;
        s_devices_work.terminal_in_body = s_devices_work.headers_complete;
        s_devices_work.terminal_result = result;
        s_devices_work.terminal_errno = saved_errno;
        s_devices_work.terminal_read_us = elapsed;
        s_devices_work.parser_before_terminal = s_devices_work.parser_final;
    }
}
void athom_devices_http_diag_parser(int before, int after, size_t bytes,
                                   uint64_t elapsed)
{
    (void)before;
    s_devices_work.parser_final = after;
    if (bytes != 0U && after != 0 && s_devices_work.parser_first_positive_error == 0) {
        s_devices_work.parser_first_positive_error = after;
    }
    if (elapsed > s_devices_work.parser_max_us) s_devices_work.parser_max_us = elapsed;
}
void athom_devices_http_diag_transport_errno(int value)
{
    if (value != 0 && s_devices_work.transport_errno_first == 0)
        s_devices_work.transport_errno_first = value;
    s_devices_work.transport_errno_last = value;
}
void athom_devices_http_diag_headers_sent(void)
{
    increment(&s_devices_work.response_blocks);
    /* Redirect/auth subresponses are not silently mixed with the final body. */
    s_devices_work.headers_complete = s_devices_work.chunked = false;
    s_devices_work.content_length = -1;
    s_devices_work.body_bytes = 0U;
    s_devices_work.http_status = 0;
    s_devices_work.terminal_seen = false;
    s_devices_work.parser_before_terminal = s_devices_work.parser_final = 0;
    s_devices_work.parser_first_positive_error = 0;
    s_devices_work.last_positive_body_read_us = 0;
}
void athom_devices_http_diag_status(int value) { s_devices_work.http_status = value; }
void athom_devices_http_diag_headers(int64_t length, bool chunked)
{
    s_devices_work.headers_complete = true;
    s_devices_work.content_length = length;
    s_devices_work.chunked = chunked;
}
void athom_devices_http_diag_data(size_t incoming, int result, uint64_t elapsed)
{
    s_devices_work.body_bytes = add64(s_devices_work.body_bytes, incoming);
    if (incoming != 0U) s_devices_work.last_positive_body_read_us = s_devices_work.last_read_end_us;
    s_devices_work.callback_total_us = add64(s_devices_work.callback_total_us, elapsed);
    if (elapsed > s_devices_work.callback_max_us) s_devices_work.callback_max_us = elapsed;
    if (result != 0) {
        s_devices_work.callback_failed = true;
        s_devices_work.last_callback_error = result;
    }
}
void athom_devices_http_diag_finish(bool acquired, int error, bool complete,
                                   size_t bytes, size_t capacity, bool overflow,
                                   int tls_query, int tls_error, int tls_flags)
{
    if (!acquired || !athom_devices_http_diag_owned()) return;
    const int saved_errno = errno;
    s_devices_work.finished_us = (uint64_t)esp_timer_get_time();
    s_devices_work.perform_error = error;
    s_devices_work.application_result = overflow ? ESP_ERR_NO_MEM : error;
    s_devices_work.tls_query = tls_query;
    s_devices_work.tls_error = tls_error;
    s_devices_work.tls_flags = tls_flags;
    s_devices_work.body_complete = complete;
    s_devices_work.app_bytes = bounded32(bytes);
    s_devices_work.app_capacity = bounded32(capacity);
    s_devices_work.application_overflow = overflow;
    s_devices_work.failure_latched = error != 0 || overflow || !complete;
    heap_sample(&s_devices_work.internal_free_after, &s_devices_work.internal_largest_after,
                &s_devices_work.internal_min_after);
    s_devices_work.valid = true;
    portENTER_CRITICAL(&s_devices_mux);
    s_devices_published = s_devices_work;
    s_devices_owner = NULL;
    /* A complete successful reception must not hide a later natural receive
     * failure. The first failed reception remains immutable until next boot. */
    s_devices_claimed = s_devices_work.failure_latched;
    portEXIT_CRITICAL(&s_devices_mux);
    errno = saved_errno;
}

static bool append_fields(char *body, size_t capacity, size_t *offset,
                          const char *format, ...)
{
    if (*offset >= capacity) return false;
    va_list args;
    va_start(args, format);
    int n = vsnprintf(body + *offset, capacity - *offset, format, args);
    va_end(args);
    if (n <= 0 || (size_t)n >= capacity - *offset) return false;
    *offset += (size_t)n;
    return true;
}

bool athom_devices_http_diag_append_json(char *body, size_t capacity, size_t *length)
{
    if (!body || !length || *length == 0U || *length >= capacity || body[*length - 1U] != '}') return false;
    athom_devices_http_diag_t d;
    portENTER_CRITICAL(&s_devices_mux);
    d = s_devices_published;
    const bool in_progress = s_devices_owner != NULL;
    portEXIT_CRITICAL(&s_devices_mux);
    const char *separator = *length == 2U ? "" : ",";
    size_t offset = *length - 1U;
    if (!d.valid) {
        if (!append_fields(body, capacity, &offset,
                     "%s\"devices_http_capture\":{\"valid\":false,\"in_progress\":%s}}",
                     separator, in_progress ? "true" : "false")) return false;
    } else {
        const char *kind = d.callback_failed || d.application_overflow ? "callback_failure" :
            d.parser_first_positive_error != 0 ? "parser_failure" :
            d.terminal_seen && d.terminal_result == 0 ? "transport_timeout" :
            d.terminal_seen && d.terminal_result == -1 && d.tls_query == ESP_ERR_ESP_TLS_TCP_CLOSED_FIN ? "peer_fin" :
            d.terminal_seen && d.terminal_result == -1 ? "fin_return_unconfirmed" :
            d.perform_error == 0 && d.body_complete ? "complete" : "other_failure";
        if (!append_fields(body, capacity, &offset,
          "%s\"devices_http_capture\":{\"valid\":true,\"phase\":\"devices\",\"kind\":\"%s\","
          "\"sequence\":%" PRIu32 ",\"started_us\":%llu,\"finished_us\":%llu,\"total_us\":%llu,",
           separator, kind, d.request_sequence, (unsigned long long)d.started_us,
          (unsigned long long)d.finished_us, (unsigned long long)duration(d.started_us, d.finished_us)) ||
        !append_fields(body, capacity, &offset,
          "\"http_status\":%" PRId32 ",\"perform_error\":%" PRId32 ",\"application_result\":%" PRId32 ",\"failure_latched\":%s,"
          "\"headers_complete\":%s,\"content_length\":%lld,\"chunked\":%s,"
          "\"chunk_completion_known\":%s,\"chunk_complete\":%s,\"body_complete\":%s,",
           d.http_status, d.perform_error, d.application_result, d.failure_latched ? "true":"false",
          d.headers_complete ? "true":"false", (long long)d.content_length, d.chunked ? "true":"false",
          d.chunked && d.headers_complete ? "true":"false", d.chunked && d.body_complete ? "true":"false",
          d.body_complete ? "true":"false") ||
        !append_fields(body, capacity, &offset,
          "\"body_bytes\":%llu,\"wire_bytes\":%llu,\"app_bytes\":%" PRIu32 ",\"app_capacity\":%" PRIu32 ","
          "\"last_positive_body_read_us\":%llu,\"terminal_seen\":%s,\"terminal_in_body\":%s,"
          "\"terminal_result\":%" PRId32 ",\"terminal_errno\":%" PRId32 ",\"terminal_read_us\":%llu,"
          "\"transport_errno_first\":%" PRId32 ",\"transport_errno_last\":%" PRId32 ",",
           (unsigned long long)d.body_bytes, (unsigned long long)d.wire_bytes, d.app_bytes, d.app_capacity,
          (unsigned long long)d.last_positive_body_read_us, d.terminal_seen ? "true":"false",
          d.terminal_in_body ? "true":"false", d.terminal_result, d.terminal_errno,
          (unsigned long long)d.terminal_read_us, d.transport_errno_first, d.transport_errno_last) ||
        !append_fields(body, capacity, &offset,
          "\"parser_before_terminal\":%" PRId32 ",\"parser_final\":%" PRId32 ",\"parser_first_positive_error\":%" PRId32 ","
          "\"read_calls\":%" PRIu32 ",\"read_max_us\":%llu,\"parser_max_us\":%llu,"
          "\"callback_max_us\":%llu,\"callback_total_us\":%llu,\"callback_failed\":%s,"
          "\"last_callback_error\":%" PRId32 ",\"application_overflow\":%s,\"response_blocks\":%" PRIu32 ",",
           d.parser_before_terminal, d.parser_final, d.parser_first_positive_error, d.read_calls,
          (unsigned long long)d.read_max_us, (unsigned long long)d.parser_max_us,
          (unsigned long long)d.callback_max_us, (unsigned long long)d.callback_total_us,
          d.callback_failed ? "true":"false", d.last_callback_error,
          d.application_overflow ? "true":"false", d.response_blocks) ||
        !append_fields(body, capacity, &offset,
          "\"client_reused\":%s,\"connection_reuse_known\":%s,\"connection_reused\":%s,"
          "\"connect_calls\":%" PRIu32 ",\"connect_result\":%" PRId32 ",\"tls_query\":%" PRId32 ",\"tls_error\":%" PRId32 ",\"tls_flags\":%" PRId32 ","
          "\"internal_before\":{\"free\":%" PRIu32 ",\"largest\":%" PRIu32 ",\"minimum\":%" PRIu32 "},"
          "\"internal_after\":{\"free\":%" PRIu32 ",\"largest\":%" PRIu32 ",\"minimum\":%" PRIu32 "}}}",
          d.client_reused ? "true":"false", d.connection_reuse_known ? "true":"false",
          d.connection_reused ? "true":"false", d.connect_calls, d.connect_result, d.tls_query, d.tls_error, d.tls_flags,
          d.internal_free_before, d.internal_largest_before, d.internal_min_before,
          d.internal_free_after, d.internal_largest_after, d.internal_min_after)) return false;
    }
    *length = offset;
    return true;
}
