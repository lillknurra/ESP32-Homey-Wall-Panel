#include "panel_startup_status.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_success_stages_are_minimum_two_seconds(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, false, PANEL_STARTUP_HOMEY_READY, 100U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 1000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 2999U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 3000U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 4999U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 5000U) ==
           PANEL_STARTUP_ACTION_SHOW_DASHBOARD);
    panel_startup_status_render_failed(&status, PANEL_STARTUP_ACTION_SHOW_DASHBOARD);
    assert(!status.homey_ready_shown);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 5001U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);
}

static void test_ready_is_interrupted_when_verification_is_lost(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 1000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 3000U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 3500U) ==
           PANEL_STARTUP_ACTION_HOMEY_LOADING);
    assert(!status.homey_ready_shown);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 3600U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 5599U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 5600U) ==
           PANEL_STARTUP_ACTION_SHOW_DASHBOARD);
}

static void test_error_is_never_held_behind_success_message(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 1000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_ERROR, 1100U) ==
           PANEL_STARTUP_ACTION_HOMEY_ERROR);
    /* The error interrupted the Wi-Fi message before its two-second hold,
     * so recovery must render that milestone again. */
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 1200U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 3199U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 3200U) ==
           PANEL_STARTUP_ACTION_HOMEY_LOADING);
}

static void test_loading_appears_after_wifi_hold_without_sleep(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 2000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 3999U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 4000U) ==
           PANEL_STARTUP_ACTION_HOMEY_LOADING);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 4500U) ==
           PANEL_STARTUP_ACTION_NONE);
}

static void test_failed_render_retries_without_sleep_or_lost_minimum(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 500U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    panel_startup_status_render_failed(&status, PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 600U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 2599U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 2600U) ==
           PANEL_STARTUP_ACTION_HOMEY_LOADING);
    panel_startup_status_render_failed(&status, PANEL_STARTUP_ACTION_HOMEY_LOADING);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 2601U) ==
           PANEL_STARTUP_ACTION_HOMEY_LOADING);
}

static void test_wifi_loss_cancels_ready_and_restarts_milestones(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 1000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 3000U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);

    /* A lost link must interrupt the READY presentation immediately and
     * invalidate both success holds before a later reconnect. */
    assert(panel_startup_status_tick(&status, false, PANEL_STARTUP_HOMEY_READY, 3500U) ==
           PANEL_STARTUP_ACTION_WIFI_DISCONNECTED);
    assert(!status.homey_ready_shown);
    assert(!status.wifi_status_shown);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 5000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 6999U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 7000U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 8999U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 9000U) ==
           PANEL_STARTUP_ACTION_SHOW_DASHBOARD);
}

static void test_fast_transitions_keep_visible_status_and_both_holds(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    panel_startup_action_t visible = PANEL_STARTUP_ACTION_NONE;

    panel_startup_action_t action = panel_startup_status_tick(
        &status, true, PANEL_STARTUP_HOMEY_LOADING, 100U);
    assert(action == PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    visible = action;
    action = panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 2099U);
    if (action != PANEL_STARTUP_ACTION_NONE) visible = action;
    assert(visible == PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    action = panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 2100U);
    assert(action == PANEL_STARTUP_ACTION_HOMEY_LOADING);
    visible = action;
    action = panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 2200U);
    assert(action == PANEL_STARTUP_ACTION_HOMEY_READY);
    visible = action;
    action = panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 4199U);
    if (action != PANEL_STARTUP_ACTION_NONE) visible = action;
    assert(visible == PANEL_STARTUP_ACTION_HOMEY_READY);
    action = panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 4200U);
    assert(action == PANEL_STARTUP_ACTION_SHOW_DASHBOARD);
}

static void test_error_interrupts_ready_hold_immediately(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 0U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 2000U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_ERROR, 2100U) ==
           PANEL_STARTUP_ACTION_HOMEY_ERROR);
    assert(!status.homey_ready_shown);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 2200U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);
}

static void test_error_before_wifi_render_does_not_complete_wifi_milestone(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);

    /* ERROR is rendered immediately, but it must not stand in for the
     * Wi-Fi-connected milestone that has never been rendered. */
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_ERROR, 100U) ==
           PANEL_STARTUP_ACTION_HOMEY_ERROR);
    assert(!status.wifi_status_shown);

    /* Recovery after the old two-second window must still present Wi-Fi. */
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 3000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(status.wifi_status_shown);
    assert(status.wifi_status_shown_at_ms == 3000U);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 4999U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 5000U) ==
           PANEL_STARTUP_ACTION_HOMEY_LOADING);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 5001U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);
    assert(status.homey_ready_shown_at_ms == 5001U);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 7000U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 7001U) ==
           PANEL_STARTUP_ACTION_SHOW_DASHBOARD);
}

static void test_error_interrupts_wifi_hold_and_recovery_restarts_it(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 1000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_ERROR, 1500U) ==
           PANEL_STARTUP_ACTION_HOMEY_ERROR);
    assert(!status.wifi_status_shown);
    assert(status.wifi_status_shown_at_ms == 0U);

    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 4000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(status.wifi_status_shown_at_ms == 4000U);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 5999U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 6000U) ==
           PANEL_STARTUP_ACTION_HOMEY_LOADING);
}

static void test_long_wait_uses_verified_ready_not_runtime_ready(void)
{
    panel_startup_wait_t wait = {0};

    /* Runtime READY with a missing/stale snapshot is not verified readiness.
     * The long-wait clock must start and eventually produce feedback. */
    panel_startup_wait_update(&wait, true, false, 1000U);
    assert(wait.active && wait.started_at_ms == 1000U);
    assert(!panel_startup_wait_expired(&wait, false, 30999U));
    assert(panel_startup_wait_expired(&wait, false, 31000U));
    assert(!panel_startup_wait_expired(&wait, true, 31000U));

    /* A verified fresh READY or Wi-Fi loss ends that wait interval. */
    panel_startup_wait_update(&wait, true, true, 32000U);
    assert(!wait.active && wait.started_at_ms == 0U);
    panel_startup_wait_update(&wait, true, false, 33000U);
    assert(wait.active && wait.started_at_ms == 33000U);
    panel_startup_wait_update(&wait, false, false, 34000U);
    assert(!wait.active && wait.started_at_ms == 0U);
}

static void test_primary_and_secondary_share_visible_label_text(void)
{
    char visible[128];
    assert(panel_startup_compose_visible_text(
        visible, sizeof(visible), "Hämtar Homey-data...",
        "Väntar på verifierad live-data"));
    assert(strcmp(visible,
        "Hämtar Homey-data...\nVäntar på verifierad live-data") == 0);
    assert(panel_startup_compose_visible_text(
        visible, sizeof(visible), "Homey READY", ""));
    assert(strcmp(visible, "Homey READY") == 0);
    assert(panel_startup_compose_visible_text(
        visible, sizeof(visible), NULL, "Återansluter till sparat nätverk"));
    assert(strcmp(visible, "Återansluter till sparat nätverk") == 0);
    assert(!panel_startup_compose_visible_text(
        visible, 8U, "Homey READY", "Försöker igen..."));
    assert(visible[0] == '\0');
}

int main(void)
{
    panel_startup_layout_t layout;
    assert(panel_startup_layout_compute(480U, 266U, 16U, 24U, 8U,
                                        32U, 0U, &layout));
    assert(layout.primary_y == 424U);
    assert(layout.primary_y + layout.primary_height == 456U);

    /* Wrapped primary and secondary labels must remain separate and clear
     * the Wi-Fi button by at least 16 px. */
    assert(panel_startup_layout_compute(480U, 266U, 16U, 24U, 8U,
                                        64U, 26U, &layout));
    assert(layout.secondary_y + layout.secondary_height + 8U == layout.primary_y);
    assert(layout.primary_y + layout.primary_height == 456U);
    assert(layout.secondary_y >= 282U);
    assert(panel_startup_layout_compute(480U, 266U, 16U, 24U, 8U,
                                        96U, 52U, &layout));
    assert(layout.secondary_y >= 282U);
    assert(!panel_startup_layout_compute(480U, 266U, 16U, 24U, 8U,
                                         128U, 64U, &layout));
    assert(!panel_startup_layout_compute(480U, 266U, 16U, 24U, 8U,
                                         32U, 0U, NULL));

    test_success_stages_are_minimum_two_seconds();
    test_ready_is_interrupted_when_verification_is_lost();
    test_error_is_never_held_behind_success_message();
    test_loading_appears_after_wifi_hold_without_sleep();
    test_failed_render_retries_without_sleep_or_lost_minimum();
    test_wifi_loss_cancels_ready_and_restarts_milestones();
    test_fast_transitions_keep_visible_status_and_both_holds();
    test_error_interrupts_ready_hold_immediately();
    test_error_before_wifi_render_does_not_complete_wifi_milestone();
    test_error_interrupts_wifi_hold_and_recovery_restarts_it();
    test_long_wait_uses_verified_ready_not_runtime_ready();
    test_primary_and_secondary_share_visible_label_text();
    puts("PATCH105_STARTUP_STATUS_PASS");
    return 0;
}
