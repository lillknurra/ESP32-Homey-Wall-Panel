#include "panel_update_i8_handoff_policy.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>

static panel_update_i7_admission_t healthy(void)
{
    return (panel_update_i7_admission_t){
        .trial_separately_authorized = true, /* HOST SIMULATION ONLY */
        .wifi_stable = true,
        .normal_boot_settled = true,
        .homey_ready = true,
        .homey_network_phase_idle = true,
        .homey_command_queue_empty = true,
        .homey_tls_closed = true,
        .cloud_tls_closed = true,
        .other_outbound_tls_quiescent = true,
        .internal_free_bytes = PANEL_UPDATE_I7_MIN_INTERNAL_FREE_BYTES,
        .internal_largest_bytes = PANEL_UPDATE_I7_MIN_INTERNAL_LARGEST_BYTES,
        .owner_stack_margin_bytes = PANEL_UPDATE_I7_MIN_OWNER_STACK_MARGIN_BYTES,
    };
}

static panel_update_i8_capabilities_t supported_mock(void)
{
    return (panel_update_i8_capabilities_t){
        .shared_lock_covers_all_homey_phases = true,
        .dns_callback_nonblocking = true,
        .dns_abort_drains_or_tombstones_callbacks = true,
        .tls_progress_nonblocking = true,
        .http_progress_nonblocking = true,
        .abort_is_cooperative = true,
        .abort_never_reconnects = true,
        .cleanup_destroys_all_io_resources = true,
        .worst_single_call_ms = PANEL_UPDATE_I8_MAX_ATTESTED_CALL_MS,
        .worst_cleanup_ack_ms = PANEL_UPDATE_I8_MAX_ATTESTED_CLEANUP_MS,
    };
}

static void start(panel_update_i8_session_t *s,
                  panel_update_i7_arbiter_t *a, uint32_t gen)
{
    panel_update_i7_admission_t e = healthy();
    panel_update_i8_capabilities_t c = supported_mock();
    panel_update_i7_init(a);
    assert(panel_update_i8_reset_terminal(s));
    assert(panel_update_i8_begin(s, a, gen, 1000U, 5000U, &e, &c));
    assert(a->ota_transport_outstanding);
    assert(a->phase == PANEL_UPDATE_I7_OTA_ACTIVE);
}

static void test_capability_and_ownership_failclosed(void)
{
    panel_update_i8_session_t s = {0};
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    panel_update_i8_capabilities_t c = supported_mock();
    assert(panel_update_i8_reset_terminal(&s));
    panel_update_i7_init(&a);
#define MUST_REJECT() do { \
    assert(!panel_update_i8_begin(&s, &a, 1U, 1000U, 5000U, &e, &c)); \
    assert(a.phase == PANEL_UPDATE_I7_IDLE && !a.ota_transport_outstanding); \
} while (0)
    c.shared_lock_covers_all_homey_phases = false; MUST_REJECT(); c = supported_mock();
    c.dns_callback_nonblocking = false; MUST_REJECT(); c = supported_mock();
    c.dns_abort_drains_or_tombstones_callbacks = false; MUST_REJECT(); c = supported_mock();
    c.tls_progress_nonblocking = false; MUST_REJECT(); c = supported_mock();
    c.http_progress_nonblocking = false; MUST_REJECT(); c = supported_mock();
    c.abort_is_cooperative = false; MUST_REJECT(); c = supported_mock();
    c.abort_never_reconnects = false; MUST_REJECT(); c = supported_mock();
    c.cleanup_destroys_all_io_resources = false; MUST_REJECT(); c = supported_mock();
    c.worst_single_call_ms = 0; MUST_REJECT(); c = supported_mock();
    c.worst_single_call_ms++; MUST_REJECT(); c = supported_mock();
    c.worst_cleanup_ack_ms = 0; MUST_REJECT(); c = supported_mock();
    c.worst_cleanup_ack_ms++; MUST_REJECT(); c = supported_mock();
    assert(!panel_update_i8_begin(&s, &a, 0U, 1000U, 5000U, &e, &c));
    assert(!panel_update_i8_begin(&s, &a, 1U, 1000U, 0U, &e, &c));
    assert(!panel_update_i8_begin(&s, &a, 1U, 1000U,
                                  PANEL_UPDATE_I8_MAX_LEASE_MS + 1U, &e, &c));
    assert(!panel_update_i8_begin(&s, &a, 1U, UINT64_MAX, 5000U, &e, &c));
    e.homey_command_queue_empty = false; MUST_REJECT(); e = healthy();
    assert(panel_update_i7_homey_try_acquire(&a));
    assert(!panel_update_i8_begin(&s, &a, 1U, 1000U, 5000U, &e, &c));
    assert(a.phase == PANEL_UPDATE_I7_HOMEY_OWNED);
    assert(panel_update_i7_homey_release(&a));
#undef MUST_REJECT
}

static void test_complete_https_cleanup_and_replay(void)
{
    panel_update_i8_session_t s = {0};
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    start(&s, &a, 17U);
    assert(panel_update_i8_next(&s, &a, 1000U, &e) == PANEL_UPDATE_I8_START_ASYNC_DNS);
    assert(!panel_update_i8_reset_terminal(&s)); /* live I/O cannot be erased */
    assert(a.ota_transport_outstanding);
    assert(panel_update_i8_next(&s, &a, 1001U, &e) == PANEL_UPDATE_I8_NO_ACTION);
    assert(!panel_update_i8_event(&s, &a, 18U, PANEL_UPDATE_I8_DNS_RESOLVED, 1002U, false));
    assert(!panel_update_i8_event(&s, &a, 17U, PANEL_UPDATE_I8_TLS_VERIFIED, 1002U, false));
    assert(panel_update_i8_event(&s, &a, 17U, PANEL_UPDATE_I8_DNS_RESOLVED, 1003U, false));
    assert(panel_update_i8_next(&s, &a, 1004U, &e) == PANEL_UPDATE_I8_START_ASYNC_TLS);
    assert(panel_update_i8_event(&s, &a, 17U, PANEL_UPDATE_I8_TLS_VERIFIED, 1005U, false));
    assert(panel_update_i8_next(&s, &a, 1006U, &e) == PANEL_UPDATE_I8_START_ASYNC_HTTP_GET);
    assert(panel_update_i8_event(&s, &a, 17U, PANEL_UPDATE_I8_HTTP_COMPLETE, 1007U, false));
    assert(panel_update_i8_next(&s, &a, 1008U, &e) == PANEL_UPDATE_I8_CLOSE_TRANSPORT);
    assert(a.ota_transport_outstanding);
    assert(!panel_update_i8_event(&s, &a, 17U, PANEL_UPDATE_I8_CLEANUP_FINISHED, 1009U, false));
    assert(a.ota_transport_outstanding);
    assert(!panel_update_i8_event(&s, &a, 16U, PANEL_UPDATE_I8_CLEANUP_FINISHED, 1009U, true));
    assert(a.ota_transport_outstanding);
    assert(panel_update_i8_event(&s, &a, 17U, PANEL_UPDATE_I8_CLEANUP_FINISHED, 1010U, true));
    assert(s.phase == PANEL_UPDATE_I8_FINISHED);
    assert(a.phase == PANEL_UPDATE_I7_IDLE && !a.ota_transport_outstanding);
    /* Only verified terminal sessions can be reinitialized for a new generation. */
    assert(!panel_update_i8_begin(&s, &a, 18U, 1020U, 5000U, &e,
                                  &(panel_update_i8_capabilities_t){0}));
    assert(!panel_update_i8_event(&s, &a, 17U, PANEL_UPDATE_I8_CLEANUP_FINISHED, 1010U, true));
    assert(panel_update_i8_next(&s, &a, 1011U, &e) == PANEL_UPDATE_I8_NO_ACTION);
    assert(panel_update_i7_homey_try_acquire(&a));
    assert(panel_update_i7_homey_release(&a));
    assert(panel_update_i8_reset_terminal(&s));
    panel_update_i8_capabilities_t c = supported_mock();
    assert(panel_update_i8_begin(&s, &a, 18U, 1030U, 5000U, &e, &c));
    assert(panel_update_i8_request_cancel(&s, &a));
    assert(panel_update_i8_next(&s, &a, 1031U, &e) == PANEL_UPDATE_I8_ABORT_AND_CLOSE);
    assert(panel_update_i8_event(&s, &a, 18U, PANEL_UPDATE_I8_CLEANUP_FINISHED, 1032U, true));
}

static void test_homey_priority_at_each_stage(void)
{
    const panel_update_i8_action_t starts[] = {
        PANEL_UPDATE_I8_START_ASYNC_DNS,
        PANEL_UPDATE_I8_START_ASYNC_TLS,
        PANEL_UPDATE_I8_START_ASYNC_HTTP_GET,
        PANEL_UPDATE_I8_CLOSE_TRANSPORT,
    };
    panel_update_i7_admission_t e = healthy();
    for (unsigned stage = 0; stage < 4; ++stage) {
        panel_update_i8_session_t s = {0};
        panel_update_i7_arbiter_t a;
        start(&s, &a, (uint32_t)(100U + stage));
        for (unsigned i = 0; i <= stage; ++i) {
            assert(panel_update_i8_next(&s, &a, 1000U + 2U*i, &e) == starts[i]);
            if (i < stage) {
                const panel_update_i8_event_t events[] = {
                    PANEL_UPDATE_I8_DNS_RESOLVED,
                    PANEL_UPDATE_I8_TLS_VERIFIED,
                    PANEL_UPDATE_I8_HTTP_COMPLETE,
                };
                assert(panel_update_i8_event(&s, &a, s.generation, events[i],
                                             1001U + 2U*i, false));
            }
        }
        /* Caller retains the requested awning command; no silent drop. */
        const bool homey_request_retained = true;
        assert(panel_update_i8_homey_arrived(&s, &a) ==
               PANEL_UPDATE_I8_HOMEY_DEFERRED_RETRY_REQUIRED);
        assert(homey_request_retained && a.homey_pending);
        assert(!panel_update_i8_event(&s, &a, s.generation,
                                      PANEL_UPDATE_I8_HTTP_COMPLETE, 1010U, false));
        const panel_update_i8_action_t handoff =
            panel_update_i8_next(&s, &a, 1011U, &e);
        assert(handoff == (stage == 3U ? PANEL_UPDATE_I8_NO_ACTION :
                          PANEL_UPDATE_I8_ABORT_AND_CLOSE));
        assert(!panel_update_i7_homey_try_acquire(&a));
        assert(a.phase == PANEL_UPDATE_I7_OTA_CANCEL_PENDING);
        assert(a.ota_transport_outstanding);
        assert(!panel_update_i8_event(&s, &a, s.generation,
                                      PANEL_UPDATE_I8_CLEANUP_FINISHED, 1012U, false));
        assert(panel_update_i8_event(&s, &a, s.generation,
                                     PANEL_UPDATE_I8_CLEANUP_FINISHED, 1013U, true));
        assert(s.phase == PANEL_UPDATE_I8_CANCELLED);
        assert(panel_update_i8_homey_arrived(&s, &a) ==
               PANEL_UPDATE_I8_HOMEY_ACQUIRED);
        assert(panel_update_i7_homey_release(&a));
    }
}

static void test_timeout_quarantine_then_late_cleanup(void)
{
    panel_update_i8_session_t s = {0};
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    start(&s, &a, 22U);
    assert(panel_update_i8_next(&s, &a, 1000U, &e) == PANEL_UPDATE_I8_START_ASYNC_DNS);
    /* Logical deadline cannot free the in-flight DNS callback context. */
    assert(panel_update_i8_next(&s, &a, 6000U, &e) == PANEL_UPDATE_I8_ABORT_AND_CLOSE);
    assert(panel_update_i8_homey_arrived(&s, &a) ==
           PANEL_UPDATE_I8_HOMEY_DEFERRED_RETRY_REQUIRED);
    assert(panel_update_i8_next(&s, &a, 6100U, &e) == PANEL_UPDATE_I8_NO_ACTION);
    assert(s.phase == PANEL_UPDATE_I8_QUARANTINED);
    assert(a.ota_transport_outstanding && a.homey_pending);
    assert(!panel_update_i8_event(&s, &a, 21U,
                                  PANEL_UPDATE_I8_CLEANUP_FINISHED, 6101U, true));
    assert(!panel_update_i8_event(&s, &a, 22U,
                                  PANEL_UPDATE_I8_CLEANUP_FINISHED, 6102U, false));
    assert(!panel_update_i7_homey_try_acquire(&a));
    assert(panel_update_i8_event(&s, &a, 22U,
                                 PANEL_UPDATE_I8_CLEANUP_FINISHED, 6103U, true));
    assert(s.phase == PANEL_UPDATE_I8_CANCELLED);
    assert(panel_update_i8_homey_arrived(&s, &a) ==
           PANEL_UPDATE_I8_HOMEY_ACQUIRED);
    assert(panel_update_i7_homey_release(&a));
}

static void test_health_revoked_io_error_and_clock(void)
{
    panel_update_i8_session_t s = {0};
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    start(&s, &a, 30U);
    e.internal_free_bytes--;
    assert(panel_update_i8_next(&s, &a, 1000U, &e) == PANEL_UPDATE_I8_ABORT_AND_CLOSE);
    assert(!panel_update_i8_event(&s, &a, 30U, PANEL_UPDATE_I8_DNS_RESOLVED, 1001U, false));
    assert(panel_update_i8_event(&s, &a, 30U, PANEL_UPDATE_I8_CLEANUP_FINISHED, 1002U, true));
    assert(s.phase == PANEL_UPDATE_I8_CANCELLED);

    start(&s, &a, 31U);
    e = healthy();
    assert(panel_update_i8_next(&s, &a, 1000U, &e) == PANEL_UPDATE_I8_START_ASYNC_DNS);
    assert(panel_update_i8_event(&s, &a, 31U, PANEL_UPDATE_I8_IO_FAILED, 1001U, false));
    assert(panel_update_i8_next(&s, &a, 1002U, &e) == PANEL_UPDATE_I8_ABORT_AND_CLOSE);
    assert(panel_update_i8_event(&s, &a, 31U, PANEL_UPDATE_I8_CLEANUP_FINISHED, 1003U, true));

    start(&s, &a, 32U);
    assert(panel_update_i8_next(&s, &a, 1002U, &e) == PANEL_UPDATE_I8_START_ASYNC_DNS);
    assert(panel_update_i8_next(&s, &a, 1001U, &e) == PANEL_UPDATE_I8_ABORT_AND_CLOSE);
    assert(panel_update_i8_event(&s, &a, 32U, PANEL_UPDATE_I8_CLEANUP_FINISHED, 1003U, true));
    assert(s.phase == PANEL_UPDATE_I8_CANCELLED);
}

static void test_late_handoff_during_successful_close(void)
{
    panel_update_i8_session_t s = {0};
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    start(&s, &a, 44U);
    assert(panel_update_i8_next(&s, &a, 1000U, &e) == PANEL_UPDATE_I8_START_ASYNC_DNS);
    assert(panel_update_i8_event(&s, &a, 44U, PANEL_UPDATE_I8_DNS_RESOLVED, 1001U, false));
    assert(panel_update_i8_next(&s, &a, 1002U, &e) == PANEL_UPDATE_I8_START_ASYNC_TLS);
    assert(panel_update_i8_event(&s, &a, 44U, PANEL_UPDATE_I8_TLS_VERIFIED, 1003U, false));
    assert(panel_update_i8_next(&s, &a, 1004U, &e) == PANEL_UPDATE_I8_START_ASYNC_HTTP_GET);
    assert(panel_update_i8_event(&s, &a, 44U, PANEL_UPDATE_I8_HTTP_COMPLETE, 1005U, false));
    assert(panel_update_i8_next(&s, &a, 1006U, &e) == PANEL_UPDATE_I8_CLOSE_TRANSPORT);
    /* Simulate direct I7 call before I8's Homey adapter receives notification. */
    assert(!panel_update_i7_homey_try_acquire(&a));
    assert(a.homey_pending);
    assert(panel_update_i8_event(&s, &a, 44U,
                                 PANEL_UPDATE_I8_CLEANUP_FINISHED, 1007U, true));
    assert(s.phase == PANEL_UPDATE_I8_CANCELLED); /* Never publish success. */
    assert(panel_update_i7_homey_try_acquire(&a));
    assert(panel_update_i7_homey_release(&a));
}

static void test_null_and_invalid_events(void)
{
    panel_update_i8_session_t s = {0};
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    panel_update_i8_capabilities_t c = supported_mock();
    assert(!panel_update_i8_reset_terminal(NULL));
    panel_update_i7_init(&a);
    assert(panel_update_i8_reset_terminal(&s));
    assert(!panel_update_i8_begin(NULL, &a, 1U, 1000U, 5000U, &e, &c));
    assert(!panel_update_i8_begin(&s, NULL, 1U, 1000U, 5000U, &e, &c));
    assert(!panel_update_i8_begin(&s, &a, 1U, 1000U, 5000U, &e, NULL));
    assert(panel_update_i8_next(NULL, &a, 1000U, &e) == PANEL_UPDATE_I8_NO_ACTION);
    assert(panel_update_i8_next(&s, NULL, 1000U, &e) == PANEL_UPDATE_I8_NO_ACTION);
    assert(!panel_update_i8_event(NULL, &a, 1U, PANEL_UPDATE_I8_DNS_RESOLVED, 1000U, false));
    assert(!panel_update_i8_event(&s, NULL, 1U, PANEL_UPDATE_I8_DNS_RESOLVED, 1000U, false));
    assert(!panel_update_i8_request_cancel(NULL, &a));
    assert(!panel_update_i8_request_cancel(&s, NULL));
    assert(panel_update_i8_homey_arrived(&s, NULL) == PANEL_UPDATE_I8_HOMEY_BUSY_OTHER);
    start(&s, &a, 51U);
    assert(!panel_update_i8_event(&s, &a, 51U,
                                  PANEL_UPDATE_I8_CLEANUP_FINISHED, 1001U, true));
    assert(!panel_update_i8_event(&s, &a, 51U, (panel_update_i8_event_t)99, 1001U, false));
    assert(a.ota_transport_outstanding);
    assert(panel_update_i8_next(&s, &a, 1002U, &e) == PANEL_UPDATE_I8_START_ASYNC_DNS);
    assert(panel_update_i8_request_cancel(&s, &a));
    assert(panel_update_i8_next(&s, &a, 1003U, &e) == PANEL_UPDATE_I8_ABORT_AND_CLOSE);
    assert(panel_update_i8_event(&s, &a, 51U,
                                 PANEL_UPDATE_I8_CLEANUP_FINISHED, 1004U, true));
}

int main(void)
{
    test_capability_and_ownership_failclosed();
    test_complete_https_cleanup_and_replay();
    test_homey_priority_at_each_stage();
    test_timeout_quarantine_then_late_cleanup();
    test_health_revoked_io_error_and_clock();
    test_late_handoff_during_successful_close();
    test_null_and_invalid_events();
    puts("PASS: Patch116I8 offline async DNS/HTTPS handoff, Homey priority, cleanup, quarantine and replay gates");
    return 0;
}
