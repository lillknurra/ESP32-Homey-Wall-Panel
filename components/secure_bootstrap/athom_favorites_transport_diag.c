#include "athom_favorites_transport_diag.h"
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#ifdef PATCH070_HOST_TEST
#include "test_patch070_platform.h"
#else
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_transport.h"
#include "http_parser.h"
#endif

/* GET never reads this workspace. Only the owning task observes wrappers;
 * the finished scalar copy joins Patch069's immutable attempt publication. */
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t s_owner;
static athom_favorites_fetch_diagnostic_t s_diag;
static int64_t s_header_start_us;
static bool s_header_wait_started;
static bool s_header_wait_finished;

static bool owned(void)
{
    portENTER_CRITICAL(&s_mux);
    bool result = s_owner != NULL && s_owner == xTaskGetCurrentTaskHandle();
    portEXIT_CRITICAL(&s_mux);
    return result;
}

static uint32_t add_bounded(uint32_t value, uint32_t amount)
{
    return UINT32_MAX - value < amount ? UINT32_MAX : value + amount;
}

static void header_wait_finish(void)
{
    if (s_header_wait_started && !s_header_wait_finished) {
        const int64_t elapsed = (esp_timer_get_time() - s_header_start_us) / 1000;
        s_diag.header_wait_elapsed_ms = elapsed <= 0 ? 0U :
            elapsed > UINT32_MAX ? UINT32_MAX : (uint32_t)elapsed;
        s_header_wait_finished = true;
    }
}

bool athom_favorites_transport_diag_begin(bool client_reused)
{
    int saved_errno = errno;
    portENTER_CRITICAL(&s_mux);
    bool acquired = s_owner == NULL;
    if (acquired) s_owner = xTaskGetCurrentTaskHandle();
    portEXIT_CRITICAL(&s_mux);
    if (acquired) {
        memset(&s_diag, 0, sizeof(s_diag));
        s_diag.valid = true;
        s_diag.client_reused = client_reused;
        s_header_start_us = 0;
        s_header_wait_started = false;
        s_header_wait_finished = false;
    }
    errno = saved_errno;
    return acquired;
}

void athom_favorites_transport_diag_client_reused(void)
{
    int saved_errno = errno;
    if (owned()) s_diag.client_reused = true;
    errno = saved_errno;
}

void athom_favorites_transport_diag_event(athom_favorites_transport_event_t event)
{
    int saved_errno = errno;
    if (owned()) {
        if (event == ATHOM_FAVORITES_REQUEST_HEADERS_SENT) {
            s_diag.request_headers_sent = true;
            s_diag.request_header_blocks = add_bounded(s_diag.request_header_blocks, 1U);
            /* Keep the most recent redirect/auth subrequest's header phase. */
            s_diag.complete_header_observed = false;
            s_diag.header_read_calls = 0;
            s_diag.last_header_read_result = 0;
            s_diag.last_header_read_errno = 0;
            s_diag.response_bytes_observed = 0;
            s_diag.parser_calls = 0;
            s_diag.parser_error = 0;
            s_diag.timeout_observed = false;
            s_diag.fin_reported = false;
            s_header_start_us = esp_timer_get_time();
            s_header_wait_started = true;
            s_header_wait_finished = false;
        } else if (event == ATHOM_FAVORITES_RESPONSE_HEADERS_COMPLETE) {
            s_diag.complete_header_observed = true;
            header_wait_finish();
        }
    }
    errno = saved_errno;
}

void athom_favorites_transport_diag_close_result(int32_t result)
{
    int saved_errno = errno;
    if (owned()) {
        s_diag.close_called = true;
        s_diag.close_result = result;
    }
    errno = saved_errno;
}

void athom_favorites_transport_diag_finish(
    bool acquired, int32_t result, athom_favorites_fetch_diagnostic_t *out)
{
    int saved_errno = errno;
    if (out != NULL) memset(out, 0, sizeof(*out));
    if (acquired && owned()) {
        header_wait_finish();
        s_diag.request_result = result;
        if (out != NULL) *out = s_diag;
        memset(&s_diag, 0, sizeof(s_diag));
        s_header_start_us = 0;
        s_header_wait_started = false;
        s_header_wait_finished = false;
        portENTER_CRITICAL(&s_mux);
        s_owner = NULL;
        portEXIT_CRITICAL(&s_mux);
    }
    errno = saved_errno;
}

int __real_esp_transport_connect(esp_transport_handle_t, const char *, int, int);
int __wrap_esp_transport_connect(esp_transport_handle_t t, const char *host,
                                 int port, int timeout_ms)
{
    int entry_errno = errno;
    bool capture = owned();
    errno = entry_errno;
    int result = __real_esp_transport_connect(t, host, port, timeout_ms);
    int saved_errno = errno;
    if (capture) {
        s_diag.connect_calls = add_bounded(s_diag.connect_calls, 1U);
        s_diag.connect_result = result;
    }
    errno = saved_errno;
    return result;
}

int __real_esp_transport_write(esp_transport_handle_t, const char *, int, int);
int __wrap_esp_transport_write(esp_transport_handle_t t, const char *data,
                               int length, int timeout_ms)
{
    int entry_errno = errno;
    bool capture = owned();
    errno = entry_errno;
    int result = __real_esp_transport_write(t, data, length, timeout_ms);
    int saved_errno = errno;
    if (capture) {
        if (s_diag.write_calls == 0U) {
            s_diag.connection_reuse_known = true;
            s_diag.connection_reused = s_diag.connect_calls == 0U;
        }
        s_diag.write_calls = add_bounded(s_diag.write_calls, 1U);
        s_diag.last_write_result = result;
        if (result > 0) s_diag.request_bytes_written =
            add_bounded(s_diag.request_bytes_written, (uint32_t)result);
    }
    errno = saved_errno;
    return result;
}

int __real_esp_transport_read(esp_transport_handle_t, char *, int, int);
int __wrap_esp_transport_read(esp_transport_handle_t t, char *data,
                              int length, int timeout_ms)
{
    int entry_errno = errno;
    bool capture = owned() && s_header_wait_started && !s_diag.complete_header_observed;
    errno = entry_errno;
    int result = __real_esp_transport_read(t, data, length, timeout_ms);
    int saved_errno = errno;
    if (capture) {
        s_diag.header_read_calls = add_bounded(s_diag.header_read_calls, 1U);
        s_diag.last_header_read_result = result;
        s_diag.last_header_read_errno = saved_errno;
        if (result > 0) s_diag.response_bytes_observed =
            add_bounded(s_diag.response_bytes_observed, (uint32_t)result);
        if (result == ERR_TCP_TRANSPORT_CONNECTION_TIMEOUT) s_diag.timeout_observed = true;
        if (result == ERR_TCP_TRANSPORT_CONNECTION_CLOSED_BY_FIN) s_diag.fin_reported = true;
    }
    errno = saved_errno;
    return result;
}

int __real_esp_transport_get_errno(esp_transport_handle_t);
int __wrap_esp_transport_get_errno(esp_transport_handle_t t)
{
    int entry_errno = errno;
    bool capture = owned();
    errno = entry_errno;
    int result = __real_esp_transport_get_errno(t);
    int saved_errno = errno;
    if (capture) {
        s_diag.transport_errno_calls = add_bounded(s_diag.transport_errno_calls, 1U);
        if (s_diag.transport_errno == 0 && result != 0) s_diag.transport_errno = result;
    }
    errno = saved_errno;
    return result;
}

size_t __real_http_parser_execute(http_parser *, const http_parser_settings *, const char *, size_t);
size_t __wrap_http_parser_execute(http_parser *parser, const http_parser_settings *settings,
                                  const char *data, size_t length)
{
    int entry_errno = errno;
    bool capture = owned() && s_header_wait_started && !s_diag.complete_header_observed;
    errno = entry_errno;
    size_t result = __real_http_parser_execute(parser, settings, data, length);
    int saved_errno = errno;
    if (capture) {
        s_diag.parser_calls = add_bounded(s_diag.parser_calls, 1U);
        int parser_error = (int)HTTP_PARSER_ERRNO(parser);
        if (s_diag.parser_error == 0 && parser_error != 0) {
            s_diag.parser_error = parser_error <= 127 ? parser_error : -1;
        }
    }
    errno = saved_errno;
    return result;
}

const char *athom_favorites_transport_diag_class(const athom_favorites_fetch_diagnostic_t *d)
{
    if (d == NULL || !d->valid) return "not_observed";
    if (d->request_result == 0 && d->complete_header_observed) return "ok";
    if (!d->request_headers_sent) return "before_headers_sent";
    if (d->complete_header_observed) return "after_headers_error";
    if (d->parser_error != 0) return "header_parse_error";
    if (d->timeout_observed) return "header_timeout";
    if (d->fin_reported) return "header_fin_reported";
    if (d->header_read_calls != 0U) return d->response_bytes_observed == 0U
        ? "header_zero_bytes_error" : "header_partial_error";
    return "unclassified";
}
