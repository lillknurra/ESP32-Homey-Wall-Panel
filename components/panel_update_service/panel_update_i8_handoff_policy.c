/* Patch116I8: pure C, host-test-only asynchronous handoff state machine.
 * Intentionally absent from production ESP-IDF CMakeLists.txt.
 * There is no live DNS, HTTPS, Homey, OTA installer or flash path here.
 */
#include "panel_update_i8_handoff_policy.h"
#include <string.h>

#if defined(ESP_PLATFORM) && (!defined(PANEL_UPDATE_I8_AUDIT_ONLY) || PANEL_UPDATE_I8_AUDIT_ONLY != 1)
#error "Patch116I8 is offline-only; unauthorized native transport integration"
#endif

static bool capabilities_verified(const panel_update_i8_capabilities_t *c)
{
    return c != NULL && c->shared_lock_covers_all_homey_phases &&
           c->dns_callback_nonblocking &&
           c->dns_abort_drains_or_tombstones_callbacks &&
           c->tls_progress_nonblocking && c->http_progress_nonblocking &&
           c->abort_is_cooperative && c->abort_never_reconnects &&
           c->cleanup_destroys_all_io_resources &&
           c->worst_single_call_ms > 0U &&
           c->worst_single_call_ms <= PANEL_UPDATE_I8_MAX_ATTESTED_CALL_MS &&
           c->worst_cleanup_ack_ms > 0U &&
           c->worst_cleanup_ack_ms <= PANEL_UPDATE_I8_MAX_ATTESTED_CLEANUP_MS;
}

static bool is_terminal(panel_update_i8_phase_t p)
{
    return p == PANEL_UPDATE_I8_IDLE || p == PANEL_UPDATE_I8_FINISHED ||
           p == PANEL_UPDATE_I8_CANCELLED;
}

bool panel_update_i8_reset_terminal(panel_update_i8_session_t *s)
{
    if (s == NULL || !is_terminal(s->phase)) return false;
    memset(s, 0, sizeof(*s));
    return true;
}

bool panel_update_i8_begin(panel_update_i8_session_t *s,
                           panel_update_i7_arbiter_t *a,
                           uint32_t gen, uint64_t now_ms,
                           uint32_t total_budget_ms,
                           const panel_update_i7_admission_t *admission,
                           const panel_update_i8_capabilities_t *c)
{
    if (s == NULL || a == NULL || s->phase != PANEL_UPDATE_I8_IDLE ||
        !capabilities_verified(c) || gen == 0U ||
        total_budget_ms == 0U || total_budget_ms > PANEL_UPDATE_I8_MAX_LEASE_MS ||
        now_ms > UINT64_MAX - (uint64_t)total_budget_ms) return false;
    if (!panel_update_i7_ota_try_reserve(a, gen, admission)) return false;
    if (!panel_update_i7_ota_begin(a, gen, admission) ||
        !panel_update_i7_ota_claim_transport(a, gen)) {
        (void)panel_update_i7_ota_request_cancel(a, gen);
        /* No action was emitted; no I/O started, so release is safe. */
        (void)panel_update_i7_ota_release(a, gen);
        return false;
    }
    memset(s, 0, sizeof(*s));
    s->phase = PANEL_UPDATE_I8_NEED_DNS;
    s->generation = gen;
    s->started_ms = now_ms;
    s->last_ms = now_ms;
    s->total_deadline_ms = now_ms + total_budget_ms;
    s->cleanup_bound_ms = c->worst_cleanup_ack_ms;
    return true;
}

bool panel_update_i8_request_cancel(panel_update_i8_session_t *s,
                                    panel_update_i7_arbiter_t *a)
{
    if (s == NULL || a == NULL || is_terminal(s->phase) ||
        s->generation == 0U) return false;
    if (!panel_update_i7_ota_request_cancel(a, s->generation)) return false;
    s->cancelled = true;
    /* If a CLOSE is already in-flight, do not race it with a second close.
     * Completion is no longer allowed to count as success. */
    if (s->phase != PANEL_UPDATE_I8_NEED_CLOSE &&
        s->phase != PANEL_UPDATE_I8_WAIT_CLOSE &&
        s->phase != PANEL_UPDATE_I8_WAIT_ABORT &&
        s->phase != PANEL_UPDATE_I8_QUARANTINED) {
        s->phase = PANEL_UPDATE_I8_NEED_ABORT;
    }
    return true;
}

panel_update_i8_homey_result_t panel_update_i8_homey_arrived(
    panel_update_i8_session_t *s, panel_update_i7_arbiter_t *a)
{
    if (a == NULL) return PANEL_UPDATE_I8_HOMEY_BUSY_OTHER;
    if (panel_update_i7_homey_try_acquire(a))
        return PANEL_UPDATE_I8_HOMEY_ACQUIRED;
    if (a->homey_pending && s != NULL && !is_terminal(s->phase) &&
        s->generation == a->generation) {
        (void)panel_update_i8_request_cancel(s, a);
        return PANEL_UPDATE_I8_HOMEY_DEFERRED_RETRY_REQUIRED;
    }
    return PANEL_UPDATE_I8_HOMEY_BUSY_OTHER;
}

static bool budget_expired(const panel_update_i8_session_t *s, uint64_t now_ms)
{
    /* Monotonic rollback is an error; do not advance transport on it. */
    return now_ms < s->last_ms || now_ms >= s->total_deadline_ms;
}

panel_update_i8_action_t panel_update_i8_next(
    panel_update_i8_session_t *s, panel_update_i7_arbiter_t *a,
    uint64_t now_ms, const panel_update_i7_admission_t *admission)
{
    if (s == NULL || a == NULL || is_terminal(s->phase))
        return PANEL_UPDATE_I8_NO_ACTION;
    if (s->phase == PANEL_UPDATE_I8_QUARANTINED)
        return PANEL_UPDATE_I8_NO_ACTION;

    if (s->phase == PANEL_UPDATE_I8_WAIT_ABORT ||
        s->phase == PANEL_UPDATE_I8_WAIT_CLOSE) {
        if (now_ms < s->last_ms || now_ms < s->cleanup_requested_ms ||
            now_ms - s->cleanup_requested_ms >= s->cleanup_bound_ms) {
            /* Never release a live or possibly blocked transport on timeout. */
            s->phase = PANEL_UPDATE_I8_QUARANTINED;
            s->cancelled = true;
            (void)panel_update_i7_ota_request_cancel(a, s->generation);
            return PANEL_UPDATE_I8_NO_ACTION;
        }
    }
    if (budget_expired(s, now_ms))
        (void)panel_update_i8_request_cancel(s, a);
    if (a->homey_pending || a->phase == PANEL_UPDATE_I7_OTA_CANCEL_PENDING)
        (void)panel_update_i8_request_cancel(s, a);

    if (now_ms >= s->last_ms) s->last_ms = now_ms;

    switch (s->phase) {
    case PANEL_UPDATE_I8_NEED_DNS:
    case PANEL_UPDATE_I8_NEED_TLS:
    case PANEL_UPDATE_I8_NEED_HTTP:
        if (!panel_update_i7_ota_checkpoint(a, s->generation, admission)) {
            (void)panel_update_i8_request_cancel(s, a);
            /* Deliberate fall-through to the abort decision below. */
            break;
        }
        if (s->phase == PANEL_UPDATE_I8_NEED_DNS) {
            s->phase = PANEL_UPDATE_I8_WAIT_DNS;
            return PANEL_UPDATE_I8_START_ASYNC_DNS;
        }
        if (s->phase == PANEL_UPDATE_I8_NEED_TLS) {
            s->phase = PANEL_UPDATE_I8_WAIT_TLS;
            return PANEL_UPDATE_I8_START_ASYNC_TLS;
        }
        s->phase = PANEL_UPDATE_I8_WAIT_HTTP;
        return PANEL_UPDATE_I8_START_ASYNC_HTTP_GET;
    default:
        break;
    }
    if (s->phase == PANEL_UPDATE_I8_NEED_CLOSE) {
        s->cleanup_requested_ms = s->last_ms;
        s->phase = PANEL_UPDATE_I8_WAIT_CLOSE;
        return PANEL_UPDATE_I8_CLOSE_TRANSPORT;
    }
    if (s->phase == PANEL_UPDATE_I8_NEED_ABORT) {
        s->cleanup_requested_ms = s->last_ms;
        s->phase = PANEL_UPDATE_I8_WAIT_ABORT;
        return PANEL_UPDATE_I8_ABORT_AND_CLOSE;
    }
    return PANEL_UPDATE_I8_NO_ACTION;
}

static bool cleanup_finished(panel_update_i8_session_t *s,
                             panel_update_i7_arbiter_t *a,
                             uint64_t now_ms, bool verified)
{
    /* The Homey request may arrive directly through I7 after CLOSE began.
     * Snapshot priority BEFORE releasing I7; otherwise phase becomes IDLE and
     * a successful HTTP result could be published despite a pending awning. */
    const bool homey_interrupted = a->homey_pending ||
        a->phase == PANEL_UPDATE_I7_OTA_CANCEL_PENDING;
    if (!verified || !panel_update_i7_ota_ack_cleanup(a, s->generation, true))
        return false;
    if (!panel_update_i7_ota_release(a, s->generation)) {
        s->phase = PANEL_UPDATE_I8_QUARANTINED;
        s->cancelled = true;
        return false;
    }
    /* Completion after a deadline, priority interruption or quarantine may
     * release resources but can NEVER publish a successful OTA result. */
    const bool success = !homey_interrupted && !s->cancelled && s->http_completed &&
        s->phase == PANEL_UPDATE_I8_WAIT_CLOSE &&
        now_ms < s->total_deadline_ms &&
        now_ms >= s->cleanup_requested_ms &&
        now_ms - s->cleanup_requested_ms < s->cleanup_bound_ms;
    s->phase = success ? PANEL_UPDATE_I8_FINISHED : PANEL_UPDATE_I8_CANCELLED;
    return true;
}

bool panel_update_i8_event(
    panel_update_i8_session_t *s, panel_update_i7_arbiter_t *a,
    uint32_t gen, panel_update_i8_event_t event,
    uint64_t now_ms, bool cleanup_verified)
{
    if (s == NULL || a == NULL || gen == 0U || gen != s->generation ||
        is_terminal(s->phase)) return false;
    const bool cleanup_phase = s->phase == PANEL_UPDATE_I8_WAIT_ABORT ||
        s->phase == PANEL_UPDATE_I8_WAIT_CLOSE ||
        s->phase == PANEL_UPDATE_I8_QUARANTINED;
    if (event == PANEL_UPDATE_I8_CLEANUP_FINISHED && cleanup_phase) {
        if (now_ms < s->last_ms) return false;
        /* Allow eventual cleanup of quarantined work, but not its result. */
        s->last_ms = now_ms;
        return cleanup_finished(s, a, now_ms, cleanup_verified);
    }
    if (now_ms < s->last_ms || now_ms >= s->total_deadline_ms ||
        a->homey_pending || a->phase != PANEL_UPDATE_I7_OTA_ACTIVE) {
        (void)panel_update_i8_request_cancel(s, a);
        return false;
    }
    s->last_ms = now_ms;
    if (event == PANEL_UPDATE_I8_IO_FAILED &&
        (s->phase == PANEL_UPDATE_I8_WAIT_DNS ||
         s->phase == PANEL_UPDATE_I8_WAIT_TLS ||
         s->phase == PANEL_UPDATE_I8_WAIT_HTTP)) {
        (void)panel_update_i8_request_cancel(s, a);
        return true;
    }
    if (event == PANEL_UPDATE_I8_DNS_RESOLVED &&
        s->phase == PANEL_UPDATE_I8_WAIT_DNS) {
        s->phase = PANEL_UPDATE_I8_NEED_TLS;
        return true;
    }
    if (event == PANEL_UPDATE_I8_TLS_VERIFIED &&
        s->phase == PANEL_UPDATE_I8_WAIT_TLS) {
        s->phase = PANEL_UPDATE_I8_NEED_HTTP;
        return true;
    }
    if (event == PANEL_UPDATE_I8_HTTP_COMPLETE &&
        s->phase == PANEL_UPDATE_I8_WAIT_HTTP) {
        s->http_completed = true;
        s->phase = PANEL_UPDATE_I8_NEED_CLOSE;
        return true;
    }
    return false;
}
