/* Patch116G — deterministic policy-only OTA transport phases.
 * All actual DNS/TLS/HTTP/flash operations belong to a future verified adapter.
 * Default-disabled firmware does not compile/link this source.
 */
#include "panel_ota_async_policy.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

static bool duration_valid(uint32_t ms, uint32_t total_ms)
{
    return ms >= 100U && ms <= total_ms;
}

static uint32_t phase_limit(const panel_ota_async_state_t *s)
{
    switch (s->phase) {
    case PANEL_OTA_ASYNC_DNS: return s->limits.dns_ms;
    case PANEL_OTA_ASYNC_TLS: return s->limits.tls_ms;
    case PANEL_OTA_ASYNC_REQUEST: return s->limits.request_ms;
    case PANEL_OTA_ASYNC_HEADERS: return s->limits.headers_ms;
    case PANEL_OTA_ASYNC_PREAMBLE: return s->limits.preamble_ms;
    case PANEL_OTA_ASYNC_BODY: return s->limits.body_ms;
    case PANEL_OTA_ASYNC_VERIFY: return s->limits.verify_ms;
    case PANEL_OTA_ASYNC_BOOT_SELECT: return s->limits.boot_select_ms;
    case PANEL_OTA_ASYNC_DONE:
    case PANEL_OTA_ASYNC_FAILED: return 0U;
    }
    return 0U;
}

static panel_ota_async_outcome_t fail(panel_ota_async_state_t *s,
                                      panel_ota_async_reason_t reason)
{
    s->reason = reason;
    s->phase = PANEL_OTA_ASYNC_FAILED;
    return PANEL_OTA_ASYNC_REJECTED;
}

static void advance(panel_ota_async_state_t *s, panel_ota_async_phase_t phase,
                    uint64_t now_us)
{
    s->phase = phase;
    s->phase_started_us = now_us;
    s->phase_events = 0U;
}

bool panel_ota_async_begin(panel_ota_async_state_t *out,
                           const panel_ota_async_limits_t *limits,
                           uint32_t generation, uint64_t now_us)
{
    if (out == NULL || limits == NULL || generation == 0U ||
        limits->total_ms < 1000U || limits->total_ms > PANEL_OTA_ASYNC_MAX_TOTAL_MS ||
        !duration_valid(limits->dns_ms, limits->total_ms) ||
        !duration_valid(limits->tls_ms, limits->total_ms) ||
        !duration_valid(limits->request_ms, limits->total_ms) ||
        !duration_valid(limits->headers_ms, limits->total_ms) ||
        !duration_valid(limits->preamble_ms, limits->total_ms) ||
        !duration_valid(limits->body_ms, limits->total_ms) ||
        !duration_valid(limits->verify_ms, limits->total_ms) ||
        !duration_valid(limits->boot_select_ms, limits->total_ms)) return false;
    memset(out, 0, sizeof(*out));
    out->phase = PANEL_OTA_ASYNC_DNS;
    out->limits = *limits;
    out->generation = generation;
    out->started_us = now_us;
    out->phase_started_us = now_us;
    out->last_us = now_us;
    return true;
}

static bool gates_in_time(const panel_ota_async_state_t *s, uint64_t now_us)
{
    return s != NULL && now_us >= s->last_us &&
           now_us >= s->started_us && now_us >= s->phase_started_us &&
           now_us - s->started_us < (uint64_t)s->limits.total_ms * UINT64_C(1000) &&
           now_us - s->phase_started_us < (uint64_t)phase_limit(s) * UINT64_C(1000);
}

bool panel_ota_async_can_begin_flash(const panel_ota_async_state_t *state,
                                      uint64_t now_us, bool authorized, bool pre_display)
{
    return authorized && pre_display && gates_in_time(state, now_us) && state->phase == PANEL_OTA_ASYNC_BODY &&
           state->expected_image_length > PANEL_OTA_ASYNC_PREAMBLE_BYTES &&
           state->bytes_received >= PANEL_OTA_ASYNC_PREAMBLE_BYTES;
}

bool panel_ota_async_can_select_boot(const panel_ota_async_state_t *state,
                                    uint64_t now_us, bool authorized, bool pre_display)
{
    return authorized && pre_display && gates_in_time(state, now_us) && state->phase == PANEL_OTA_ASYNC_BOOT_SELECT &&
           state->expected_image_length > PANEL_OTA_ASYNC_PREAMBLE_BYTES &&
           state->bytes_received == state->expected_image_length;
}

panel_ota_async_outcome_t panel_ota_async_step(
    panel_ota_async_state_t *s, const panel_ota_async_event_t *ev,
    uint64_t now_us, bool authorized, bool pre_display)
{
    if (s == NULL || ev == NULL) return PANEL_OTA_ASYNC_REJECTED;
    if (s->phase == PANEL_OTA_ASYNC_DONE) return PANEL_OTA_ASYNC_FINISHED;
    if (s->phase == PANEL_OTA_ASYNC_FAILED) return PANEL_OTA_ASYNC_REJECTED;
    if (!authorized || !pre_display) return fail(s, PANEL_OTA_ASYNC_REASON_AUTH_REVOKED);
    if (now_us < s->last_us || now_us < s->started_us ||
        now_us - s->started_us >= (uint64_t)s->limits.total_ms * UINT64_C(1000) ||
        now_us < s->phase_started_us ||
        now_us - s->phase_started_us >= (uint64_t)phase_limit(s) * UINT64_C(1000))
        return fail(s, PANEL_OTA_ASYNC_REASON_TIMEOUT);
    s->last_us = now_us;
    if (s->event_count >= PANEL_OTA_ASYNC_MAX_EVENTS ||
        s->phase_events >= PANEL_OTA_ASYNC_MAX_EVENTS)
        return fail(s, PANEL_OTA_ASYNC_REASON_EVENT_LIMIT);
    ++s->event_count;
    ++s->phase_events;
    if (ev->generation != s->generation) return PANEL_OTA_ASYNC_IGNORE_STALE;
    if (ev->kind == PANEL_OTA_ASYNC_CANCEL) return fail(s, PANEL_OTA_ASYNC_REASON_CANCELLED);
    if (ev->kind == PANEL_OTA_ASYNC_IO_ERROR) return fail(s, PANEL_OTA_ASYNC_REASON_IO);
    if (ev->kind == PANEL_OTA_ASYNC_POLL) {
        if (ev->length != 0U) return fail(s, PANEL_OTA_ASYNC_REASON_INVALID_INPUT);
        return PANEL_OTA_ASYNC_CONTINUE;
    }
    switch (s->phase) {
    case PANEL_OTA_ASYNC_DNS:
        if (ev->kind == PANEL_OTA_ASYNC_DNS_RESOLVED && ev->length == 0U) {
            advance(s, PANEL_OTA_ASYNC_TLS, now_us); break;
        }
        return fail(s, PANEL_OTA_ASYNC_REASON_ORDER);
    case PANEL_OTA_ASYNC_TLS:
        if (ev->kind == PANEL_OTA_ASYNC_TLS_VERIFIED && ev->length == 0U) {
            advance(s, PANEL_OTA_ASYNC_REQUEST, now_us); break;
        }
        return fail(s, PANEL_OTA_ASYNC_REASON_ORDER);
    case PANEL_OTA_ASYNC_REQUEST:
        if (ev->kind == PANEL_OTA_ASYNC_REQUEST_SENT && ev->length == 0U) {
            advance(s, PANEL_OTA_ASYNC_HEADERS, now_us); break;
        }
        return fail(s, PANEL_OTA_ASYNC_REASON_ORDER);
    case PANEL_OTA_ASYNC_HEADERS:
        if (ev->kind == PANEL_OTA_ASYNC_HEADERS_ACCEPTED &&
            ev->length > PANEL_OTA_ASYNC_PREAMBLE_BYTES &&
            ev->length <= PANEL_OTA_ASYNC_MAX_IMAGE_BYTES) {
            s->expected_image_length = ev->length;
            advance(s, PANEL_OTA_ASYNC_PREAMBLE, now_us); break;
        }
        return fail(s, PANEL_OTA_ASYNC_REASON_TRANSFER);
    case PANEL_OTA_ASYNC_PREAMBLE:
        if (ev->kind == PANEL_OTA_ASYNC_PREAMBLE_VERIFIED && ev->length == 0U) {
            s->bytes_received = PANEL_OTA_ASYNC_PREAMBLE_BYTES;
            advance(s, PANEL_OTA_ASYNC_BODY, now_us); break;
        }
        return fail(s, PANEL_OTA_ASYNC_REASON_ORDER);
    case PANEL_OTA_ASYNC_BODY:
        if (ev->kind == PANEL_OTA_ASYNC_BODY_BYTES && ev->length > 0U &&
            ev->length <= s->expected_image_length - s->bytes_received) {
            s->bytes_received += ev->length;
            if (s->bytes_received == s->expected_image_length)
                advance(s, PANEL_OTA_ASYNC_VERIFY, now_us);
            break;
        }
        return fail(s, PANEL_OTA_ASYNC_REASON_TRANSFER);
    case PANEL_OTA_ASYNC_VERIFY:
        if (ev->kind == PANEL_OTA_ASYNC_IMAGE_VALIDATED && ev->length == 0U) {
            /* Native stage must call esp_ota_end(), including signature check,
             * BEFORE it emits IMAGE_VALIDATED; this is not a cryptographic check. */
            advance(s, PANEL_OTA_ASYNC_BOOT_SELECT, now_us); break;
        }
        return fail(s, PANEL_OTA_ASYNC_REASON_ORDER);
    case PANEL_OTA_ASYNC_BOOT_SELECT:
        if (ev->kind == PANEL_OTA_ASYNC_BOOT_SELECTED && ev->length == 0U) {
            s->phase = PANEL_OTA_ASYNC_DONE;
            return PANEL_OTA_ASYNC_FINISHED;
        }
        return fail(s, PANEL_OTA_ASYNC_REASON_ORDER);
    case PANEL_OTA_ASYNC_DONE: return PANEL_OTA_ASYNC_FINISHED;
    case PANEL_OTA_ASYNC_FAILED: return PANEL_OTA_ASYNC_REJECTED;
    }
    return PANEL_OTA_ASYNC_CONTINUE;
}
