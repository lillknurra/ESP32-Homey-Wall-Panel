#include "panel_homey_boot_recovery.h"

#include <assert.h>
#include <stdio.h>

static panel_homey_boot_recovery_inputs_t eligible(void)
{
    return (panel_homey_boot_recovery_inputs_t){
        .auth_restore_complete = true,
        .wifi_online = true,
        .selected_homey_present = true,
        .session_valid = true,
    };
}

static void test_late_wifi_restarts_after_bounded_boot_window(void)
{
    panel_homey_boot_recovery_state_t state = {0};
    panel_homey_boot_recovery_inputs_t input = eligible();
    input.wifi_online = false;
    assert(!panel_homey_boot_recovery_try_begin(&state, &input));
    assert(!state.scheduler_running);

    /* The original scheduler's finite wait expires while there is no Wi-Fi.
     * A later GOT_IP event re-evaluates the same restored session. */
    assert(PANEL_HOMEY_BOOT_RECOVERY_WAIT_ATTEMPTS == 120U);
    assert(PANEL_HOMEY_BOOT_RECOVERY_WAIT_MS == 1000U);
    input.wifi_online = true;
    assert(panel_homey_boot_recovery_try_begin(&state, &input));
    assert(state.scheduler_running);
}

static void test_duplicate_online_events_are_idempotent(void)
{
    panel_homey_boot_recovery_state_t state = {0};
    const panel_homey_boot_recovery_inputs_t input = eligible();
    assert(panel_homey_boot_recovery_try_begin(&state, &input));
    assert(!panel_homey_boot_recovery_try_begin(&state, &input));
    panel_homey_boot_recovery_finish(&state);
    assert(!state.scheduler_running);
    /* A later reconnect may start one new bounded attempt. */
    assert(panel_homey_boot_recovery_try_begin(&state, &input));
}

static void test_readiness_and_ownership_gates(void)
{
    panel_homey_boot_recovery_state_t state = {0};
    panel_homey_boot_recovery_inputs_t input = eligible();
    input.auth_restore_complete = false;
    assert(!panel_homey_boot_recovery_try_begin(&state, &input));
    input = eligible(); input.selected_homey_present = false;
    assert(!panel_homey_boot_recovery_try_begin(&state, &input));
    input = eligible(); input.session_valid = false;
    assert(!panel_homey_boot_recovery_try_begin(&state, &input));
    input = eligible(); input.verified_fresh_snapshot = true;
    assert(!panel_homey_boot_recovery_try_begin(&state, &input));
    input = eligible(); input.inventory_reserved = true;
    assert(!panel_homey_boot_recovery_try_begin(&state, &input));
    input = eligible(); input.scheduler_running = true;
    assert(!panel_homey_boot_recovery_try_begin(&state, &input));
    assert(panel_homey_boot_recovery_timeout_action(false, false) ==
           PANEL_HOMEY_BOOT_TIMEOUT_MARK_ERROR);
    assert(panel_homey_boot_recovery_timeout_action(true, false) ==
           PANEL_HOMEY_BOOT_TIMEOUT_KEEP_STATE);
    assert(panel_homey_boot_recovery_timeout_action(false, true) ==
           PANEL_HOMEY_BOOT_TIMEOUT_KEEP_STATE);
}

int main(void)
{
    test_late_wifi_restarts_after_bounded_boot_window();
    test_duplicate_online_events_are_idempotent();
    test_readiness_and_ownership_gates();
    puts("PATCH117_BOOT_RECOVERY_POLICY PASS scenarios=3");
    return 0;
}
