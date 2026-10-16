#include "secure_bootstrap.h"
#include <assert.h>
#include <stdio.h>

static void test_cancel_wins_before_candidate_ip(void)
{
    secure_bootstrap_wifi_session_t session;
    secure_bootstrap_wifi_session_init(&session);
    uint32_t generation = secure_bootstrap_wifi_session_begin(&session);
    assert(generation != 0U);
    assert(secure_bootstrap_wifi_session_activate(&session, generation));
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, generation));
    assert(secure_bootstrap_wifi_session_reserve_cancel(&session, generation));
    assert(!secure_bootstrap_wifi_session_reserve_commit(&session, generation));
    assert(!secure_bootstrap_wifi_session_is_live(&session, generation));
    assert(secure_bootstrap_wifi_session_finish_cancel(&session, generation, false));
    assert(!secure_bootstrap_wifi_session_begin_candidate(&session, generation));
}

static void test_commit_reservation_wins_and_cannot_be_cancelled(void)
{
    secure_bootstrap_wifi_session_t session;
    secure_bootstrap_wifi_session_init(&session);
    uint32_t generation = secure_bootstrap_wifi_session_begin(&session);
    assert(secure_bootstrap_wifi_session_activate(&session, generation));
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, generation));
    assert(secure_bootstrap_wifi_session_reserve_commit(&session, generation));
    assert(!secure_bootstrap_wifi_session_reserve_cancel(&session, generation));
    assert(secure_bootstrap_wifi_session_commit_started(&session, generation));
    assert(!secure_bootstrap_wifi_session_reserve_cancel(&session, generation));
    assert(secure_bootstrap_wifi_session_finish_commit(&session, generation));
    assert(!secure_bootstrap_wifi_session_is_live(&session, generation));
}

static void test_task_create_failure_returns_candidate_for_cancel(void)
{
    secure_bootstrap_wifi_session_t session;
    secure_bootstrap_wifi_session_init(&session);
    uint32_t generation = secure_bootstrap_wifi_session_begin(&session);
    assert(secure_bootstrap_wifi_session_activate(&session, generation));
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, generation));
    assert(secure_bootstrap_wifi_session_reserve_commit(&session, generation));
    assert(secure_bootstrap_wifi_session_commit_start_failed(&session, generation));
    assert(secure_bootstrap_wifi_session_reserve_cancel(&session, generation));
    assert(secure_bootstrap_wifi_session_finish_cancel(&session, generation, true));
    uint32_t next_generation = 0U;
    secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    assert(secure_bootstrap_wifi_session_current(
        &session, &next_generation, &phase));
    assert(next_generation != generation);
    assert(phase == SECURE_BOOTSTRAP_SESSION_ACTIVE);
}

static void test_cancel_task_create_failure_releases_reservation(void)
{
    secure_bootstrap_wifi_session_t session;
    secure_bootstrap_wifi_session_init(&session);
    uint32_t generation = secure_bootstrap_wifi_session_begin(&session);
    assert(secure_bootstrap_wifi_session_activate(&session, generation));
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, generation));
    assert(secure_bootstrap_wifi_session_reserve_cancel(&session, generation));
    assert(secure_bootstrap_wifi_session_cancel_start_failed(&session, generation));
    assert(secure_bootstrap_wifi_session_reserve_commit(&session, generation));
}

static void test_production_wifi_state_cancel_actions(void)
{
    secure_bootstrap_wifi_context_t context;
    secure_bootstrap_wifi_context_init(&context);
    (void)secure_bootstrap_wifi_transition(
        &context, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED);
    (void)secure_bootstrap_wifi_transition(
        &context, SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP);
    (void)secure_bootstrap_wifi_transition(
        &context, SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE);
    (void)secure_bootstrap_wifi_transition(
        &context, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
    const uint32_t actions = secure_bootstrap_wifi_transition(
        &context, SECURE_BOOTSTRAP_WIFI_EVENT_USER_CANCEL);
    assert(actions == (SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING |
        SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED |
        SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT));
    assert(context.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED);
    assert(!context.candidate_active && context.saved_config_present);

    secure_bootstrap_wifi_context_init(&context);
    (void)secure_bootstrap_wifi_transition(
        &context, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED);
    (void)secure_bootstrap_wifi_transition(
        &context, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
    assert(secure_bootstrap_wifi_transition(
        &context, SECURE_BOOTSTRAP_WIFI_EVENT_USER_CANCEL) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(context.state == SECURE_BOOTSTRAP_WIFI_PROVISIONING);
    assert(!context.candidate_active && !context.saved_config_present);
}

int main(void)
{
    test_cancel_wins_before_candidate_ip();
    test_commit_reservation_wins_and_cannot_be_cancelled();
    test_task_create_failure_returns_candidate_for_cancel();
    test_cancel_task_create_failure_releases_reservation();
    test_production_wifi_state_cancel_actions();
    puts("PASS: Patch079 Wi-Fi cancel/commit arbitration");
    return 0;
}
