#include "panel_startup_status.h"

#include <assert.h>
#include <stdio.h>

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
}

static void test_ready_hold_survives_transient_loading_state(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 1000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_READY, 3000U) ==
           PANEL_STARTUP_ACTION_HOMEY_READY);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 4999U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 5000U) ==
           PANEL_STARTUP_ACTION_HOMEY_LOADING);
}

static void test_error_is_never_held_behind_success_message(void)
{
    panel_startup_status_t status;
    panel_startup_status_init(&status);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 1000U) ==
           PANEL_STARTUP_ACTION_WIFI_CONNECTED);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_ERROR, 1100U) ==
           PANEL_STARTUP_ACTION_HOMEY_ERROR);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 1200U) ==
           PANEL_STARTUP_ACTION_NONE);
    assert(panel_startup_status_tick(&status, true, PANEL_STARTUP_HOMEY_LOADING, 3000U) ==
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

int main(void)
{
    test_success_stages_are_minimum_two_seconds();
    test_ready_hold_survives_transient_loading_state();
    test_error_is_never_held_behind_success_message();
    test_loading_appears_after_wifi_hold_without_sleep();
    test_failed_render_retries_without_sleep_or_lost_minimum();
    puts("PATCH105_STARTUP_STATUS_PASS");
    return 0;
}
