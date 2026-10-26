#include "panel_update_i7_admission_policy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static panel_update_i7_admission_t healthy(void)
{
    const panel_update_i7_admission_t e = {
        .trial_separately_authorized = true,
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
    return e;
}

static void test_fail_closed_gates(void)
{
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    panel_update_i7_init(&a);
    assert(!panel_update_i7_ota_try_reserve(&a, 1U, NULL));
    assert(!panel_update_i7_ota_try_reserve(&a, 0U, &e));
#define GATE_FAIL(field) do { e = healthy(); e.field = false; \
    assert(!panel_update_i7_ota_try_reserve(&a, 1U, &e)); } while (0)
    GATE_FAIL(trial_separately_authorized);
    GATE_FAIL(wifi_stable);
    GATE_FAIL(normal_boot_settled);
    GATE_FAIL(homey_ready);
    GATE_FAIL(homey_network_phase_idle);
    GATE_FAIL(homey_command_queue_empty);
    GATE_FAIL(homey_tls_closed);
    GATE_FAIL(cloud_tls_closed);
    GATE_FAIL(other_outbound_tls_quiescent);
#undef GATE_FAIL
    e = healthy(); e.internal_free_bytes--;
    assert(!panel_update_i7_ota_try_reserve(&a, 1U, &e));
    e = healthy(); e.internal_largest_bytes--;
    assert(!panel_update_i7_ota_try_reserve(&a, 1U, &e));
    e = healthy(); e.owner_stack_margin_bytes--;
    assert(!panel_update_i7_ota_try_reserve(&a, 1U, &e));
    assert(a.phase == PANEL_UPDATE_I7_IDLE && a.last_generation == 0U);
    e = healthy();
    assert(panel_update_i7_ota_try_reserve(&a, 1U, &e));
    assert(a.phase == PANEL_UPDATE_I7_OTA_RESERVED);
    assert(panel_update_i7_ota_release(&a, 1U));
}

static void test_homey_first_and_stale_generations(void)
{
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    panel_update_i7_init(&a);
    assert(panel_update_i7_homey_try_acquire(&a));
    assert(!panel_update_i7_homey_try_acquire(&a)); /* No nested lock */
    assert(!panel_update_i7_ota_try_reserve(&a, 1U, &e));
    assert(panel_update_i7_homey_release(&a));
    assert(!panel_update_i7_homey_release(&a));
    assert(panel_update_i7_ota_try_reserve(&a, 1U, &e));
    assert(!panel_update_i7_ota_try_reserve(&a, 2U, &e));
    assert(!panel_update_i7_ota_begin(&a, 2U, &e));
    assert(!panel_update_i7_ota_release(&a, 2U));
    assert(panel_update_i7_ota_begin(&a, 1U, &e));
    assert(!panel_update_i7_ota_begin(&a, 1U, &e));
    assert(panel_update_i7_ota_checkpoint(&a, 1U, &e));
    assert(panel_update_i7_ota_release(&a, 1U));
    assert(!panel_update_i7_ota_try_reserve(&a, 1U, &e));
    assert(panel_update_i7_ota_try_reserve(&a, 2U, &e));
    assert(panel_update_i7_ota_release(&a, 2U));
}

static void test_homey_interrupts_queued_ota(void)
{
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    panel_update_i7_init(&a);
    assert(panel_update_i7_ota_try_reserve(&a, 10U, &e));
    assert(!panel_update_i7_homey_try_acquire(&a));
    assert(a.homey_pending && a.phase == PANEL_UPDATE_I7_OTA_CANCEL_PENDING);
    assert(!panel_update_i7_ota_begin(&a, 10U, &e));
    assert(!panel_update_i7_ota_claim_transport(&a, 10U));
    assert(panel_update_i7_ota_release(&a, 10U));
    assert(!panel_update_i7_ota_try_reserve(&a, 11U, &e));
    assert(panel_update_i7_homey_try_acquire(&a));
    assert(!a.homey_pending);
    assert(panel_update_i7_homey_release(&a));
    assert(panel_update_i7_ota_try_reserve(&a, 11U, &e));
    assert(panel_update_i7_ota_release(&a, 11U));
}

static void test_cleanup_retains_ownership(void)
{
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    panel_update_i7_init(&a);
    assert(panel_update_i7_ota_try_reserve(&a, 42U, &e));
    assert(panel_update_i7_ota_begin(&a, 42U, &e));
    assert(panel_update_i7_ota_claim_transport(&a, 42U));
    assert(!panel_update_i7_ota_claim_transport(&a, 42U));
    assert(!panel_update_i7_homey_try_acquire(&a));
    assert(!panel_update_i7_ota_checkpoint(&a, 42U, &e));
    assert(a.phase == PANEL_UPDATE_I7_OTA_CANCEL_PENDING);
    assert(!panel_update_i7_ota_release(&a, 42U));
    assert(!panel_update_i7_ota_ack_cleanup(&a, 42U, false));
    assert(!panel_update_i7_ota_ack_cleanup(&a, 41U, true));
    assert(a.ota_transport_outstanding);
    assert(!panel_update_i7_ota_release(&a, 42U));
    assert(panel_update_i7_ota_ack_cleanup(&a, 42U, true));
    assert(!panel_update_i7_ota_ack_cleanup(&a, 42U, true));
    assert(panel_update_i7_ota_release(&a, 42U));
    assert(panel_update_i7_homey_try_acquire(&a));
    assert(panel_update_i7_homey_release(&a));
    assert(!panel_update_i7_ota_release(&a, 42U));
    assert(!panel_update_i7_ota_ack_cleanup(&a, 42U, true));
}

static void test_revocation_timeout_and_withdrawal(void)
{
    panel_update_i7_arbiter_t a;
    panel_update_i7_admission_t e = healthy();
    panel_update_i7_init(&a);
    assert(panel_update_i7_ota_try_reserve(&a, 5U, &e));
    e.wifi_stable = false;
    assert(!panel_update_i7_ota_begin(&a, 5U, &e));
    assert(a.phase == PANEL_UPDATE_I7_OTA_CANCEL_PENDING);
    assert(panel_update_i7_ota_release(&a, 5U));
    e = healthy();
    assert(panel_update_i7_ota_try_reserve(&a, 6U, &e));
    assert(panel_update_i7_ota_begin(&a, 6U, &e));
    assert(panel_update_i7_ota_claim_transport(&a, 6U));
    e.homey_command_queue_empty = false;
    assert(!panel_update_i7_ota_checkpoint(&a, 6U, &e));
    assert(!panel_update_i7_ota_release(&a, 6U));
    assert(panel_update_i7_ota_request_cancel(&a, 6U));
    assert(!panel_update_i7_ota_claim_transport(&a, 6U));
    assert(panel_update_i7_ota_ack_cleanup(&a, 6U, true));
    assert(panel_update_i7_ota_release(&a, 6U));
    e = healthy();
    assert(panel_update_i7_ota_try_reserve(&a, 7U, &e));
    assert(!panel_update_i7_homey_try_acquire(&a));
    assert(panel_update_i7_homey_withdraw_pending(&a));
    assert(!a.homey_pending);
    assert(!panel_update_i7_ota_begin(&a, 7U, &e)); /* Cancel is terminal */
    assert(panel_update_i7_ota_release(&a, 7U));
    assert(!panel_update_i7_homey_withdraw_pending(&a));
}

static void test_nulls(void)
{
    panel_update_i7_admission_t e = healthy();
    panel_update_i7_arbiter_t a;
    panel_update_i7_init(&a);
    panel_update_i7_init(NULL);
    assert(!panel_update_i7_homey_try_acquire(NULL));
    assert(!panel_update_i7_homey_release(NULL));
    assert(!panel_update_i7_ota_try_reserve(NULL, 1U, &e));
    assert(!panel_update_i7_ota_begin(NULL, 1U, &e));
    assert(!panel_update_i7_ota_claim_transport(NULL, 1U));
    assert(!panel_update_i7_ota_request_cancel(NULL, 1U));
    assert(!panel_update_i7_ota_ack_cleanup(NULL, 1U, true));
    assert(!panel_update_i7_ota_release(NULL, 1U));
    assert(!panel_update_i7_ota_checkpoint(NULL, 1U, &e));
    assert(!panel_update_i7_homey_withdraw_pending(NULL));
}

int main(void)
{
    test_fail_closed_gates();
    test_homey_first_and_stale_generations();
    test_homey_interrupts_queued_ota();
    test_cleanup_retains_ownership();
    test_revocation_timeout_and_withdrawal();
    test_nulls();
    puts("PASS: Patch116I7 offline Homey-first admission, revocation, cleanup, replay and default-off gates");
    return 0;
}
