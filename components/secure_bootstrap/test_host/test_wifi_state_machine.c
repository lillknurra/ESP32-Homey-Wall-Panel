#include "secure_bootstrap.h"
#include <assert.h>
#include <stdio.h>

static void test_saved_boot_and_retry(void)
{
    secure_bootstrap_wifi_context_t c;
    secure_bootstrap_wifi_context_init(&c);
    assert(secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED) == SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED);
    for (unsigned i = 0U; i < SECURE_BOOTSTRAP_WIFI_MAX_RETRIES; ++i) {
        uint32_t action = secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED);
        if (i + 1U < SECURE_BOOTSTRAP_WIFI_MAX_RETRIES) assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
        else assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    }
    assert(c.state == SECURE_BOOTSTRAP_WIFI_SAVED_CONNECTION_FAILED);
}

static void test_persistent_success_path(void)
{
    secure_bootstrap_wifi_context_t c;
    secure_bootstrap_wifi_context_init(&c);
    assert(secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED) == SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING);
    assert(secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED) == SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
    c.candidate_connect_armed = true; /* Test the post-verification FSM transition. */
    uint32_t action = secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP);
    assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_PERSISTING);
    action = secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_OK);
    assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST_VERIFY);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_PERSIST_VERIFY_CONNECTING);
    action = secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_GOT_IP);
    assert((action & SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE) != 0U);
    assert((action & SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING) != 0U);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_ONLINE);
}

static void test_commit_failure_and_single_rollback(void)
{
    secure_bootstrap_wifi_context_t c;
    secure_bootstrap_wifi_context_init(&c);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
    c.candidate_connect_armed = true;
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP);
    uint32_t action = secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_FAILED);
    assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_START_ROLLBACK);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_ROLLING_BACK);
    assert(secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_FAILED) == SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    action = secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_OK);
    assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED);
}

static void test_verify_failure_without_saved_falls_back(void)
{
    secure_bootstrap_wifi_context_t c;
    secure_bootstrap_wifi_context_init(&c);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
    c.candidate_connect_armed = true;
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_OK);
    uint32_t action = secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_FAILED);
    assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_START_ROLLBACK);
    action = secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED);
    assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_PROVISIONING);
}

static void test_online_user_reconfigure(void)
{
    secure_bootstrap_wifi_context_t c;
    secure_bootstrap_wifi_context_init(&c);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP);

    uint32_t action = secure_bootstrap_wifi_transition(
        &c, SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE);
    assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_PROVISIONING);
    assert(c.saved_config_present);
    assert(!c.candidate_active);

    assert(secure_bootstrap_wifi_transition(
        &c, SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
}

static void test_reconfigure_open_failure_returns_online(void)
{
    secure_bootstrap_wifi_context_t c;
    secure_bootstrap_wifi_context_init(&c);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE);

    uint32_t action = secure_bootstrap_wifi_transition(
        &c, SECURE_BOOTSTRAP_WIFI_EVENT_PROVISIONING_OPEN_FAILED);
    assert(action == (SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING |
                      SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE));
    assert(c.state == SECURE_BOOTSTRAP_WIFI_ONLINE);
}

static void test_candidate_retry_exhaustion_restores_saved(void)
{
    secure_bootstrap_wifi_context_t c;
    secure_bootstrap_wifi_context_init(&c);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);

    uint32_t action = SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
    for (unsigned i = 0U; i < SECURE_BOOTSTRAP_WIFI_MAX_RETRIES; ++i) {
        action = secure_bootstrap_wifi_transition(
            &c, SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED);
        if (i + 1U < SECURE_BOOTSTRAP_WIFI_MAX_RETRIES) {
            assert(action == SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
        }
    }

    assert(action == (SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED |
                      SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT));
    assert(c.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED);
    assert(!c.candidate_active);
    assert(c.saved_config_present);
}

static void test_double_got_ip_does_not_restart_commit(void)
{
    secure_bootstrap_wifi_context_t c;
    secure_bootstrap_wifi_context_init(&c);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
    c.candidate_connect_armed = true;
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_OK);
    assert(secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP) == SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_PERSIST_VERIFY_CONNECTING);
}

static void test_wifi_cancel_commit_arbitration(void)
{
    secure_bootstrap_wifi_session_t session;
    secure_bootstrap_wifi_session_init(&session);
    uint32_t generation = secure_bootstrap_wifi_session_begin(&session);
    assert(generation != 0U);
    assert(secure_bootstrap_wifi_session_activate(&session, generation));
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, generation));

    /* Cancel wins the shared decision boundary: stale GOT_IP cannot reserve
     * a permanent commit afterwards. */
    assert(secure_bootstrap_wifi_session_reserve_cancel(&session, generation));
    assert(!secure_bootstrap_wifi_session_reserve_commit(&session, generation));
    assert(!secure_bootstrap_wifi_session_is_live(&session, generation));
    assert(secure_bootstrap_wifi_session_finish_cancel(&session, generation, true));
    uint32_t next_generation = 0U;
    secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    assert(secure_bootstrap_wifi_session_current(&session, &next_generation, &phase));
    assert(next_generation != generation && phase == SECURE_BOOTSTRAP_SESSION_ACTIVE);
    assert(!secure_bootstrap_wifi_session_begin_candidate(&session, generation));

    /* Commit wins: cancellation is rejected from reservation through worker
     * completion, and a stale callback cannot affect a later generation. */
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, next_generation));
    assert(secure_bootstrap_wifi_session_reserve_commit(&session, next_generation));
    assert(!secure_bootstrap_wifi_session_reserve_cancel(&session, next_generation));
    assert(secure_bootstrap_wifi_session_commit_started(&session, next_generation));
    assert(!secure_bootstrap_wifi_session_reserve_cancel(&session, next_generation));
    assert(secure_bootstrap_wifi_session_finish_commit(&session, next_generation));
    assert(!secure_bootstrap_wifi_session_is_live(&session, next_generation));
}

static void test_cancel_before_commit_and_saved_restore_decision(void)
{
    secure_bootstrap_wifi_context_t c;
    secure_bootstrap_wifi_context_init(&c);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
    uint32_t actions = secure_bootstrap_wifi_transition(
        &c, SECURE_BOOTSTRAP_WIFI_EVENT_USER_CANCEL);
    assert(actions == (SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING |
                       SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED |
                       SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT));
    assert(c.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED);
    assert(!c.candidate_active && c.saved_config_present);

    secure_bootstrap_wifi_context_init(&c);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED);
    secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
    actions = secure_bootstrap_wifi_transition(&c, SECURE_BOOTSTRAP_WIFI_EVENT_USER_CANCEL);
    assert(actions == SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(c.state == SECURE_BOOTSTRAP_WIFI_PROVISIONING);
    assert(!c.candidate_active && !c.saved_config_present);
}

int main(void)
{
    test_saved_boot_and_retry();
    test_persistent_success_path();
    test_commit_failure_and_single_rollback();
    test_verify_failure_without_saved_falls_back();
    test_online_user_reconfigure();
    test_reconfigure_open_failure_returns_online();
    test_candidate_retry_exhaustion_restores_saved();
    test_double_got_ip_does_not_restart_commit();
    test_wifi_cancel_commit_arbitration();
    test_cancel_before_commit_and_saved_restore_decision();
    puts("PASS: persistent Wi-Fi state-machine host tests");
    return 0;
}
