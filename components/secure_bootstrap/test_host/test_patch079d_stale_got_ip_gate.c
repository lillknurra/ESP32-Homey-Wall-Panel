#include "secure_bootstrap.h"
#include <assert.h>
#include <stdio.h>

static void candidate_start(secure_bootstrap_wifi_context_t *wifi,
                            secure_bootstrap_wifi_session_t *session,
                            uint32_t *generation)
{
    secure_bootstrap_wifi_context_init(wifi);
    secure_bootstrap_wifi_session_init(session);
    assert(secure_bootstrap_wifi_transition(
        wifi, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING);
    *generation = secure_bootstrap_wifi_session_begin(session);
    assert(*generation != 0U);
    assert(secure_bootstrap_wifi_session_activate(session, *generation));
    assert(secure_bootstrap_wifi_transition(
        wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
    assert(secure_bootstrap_wifi_session_begin_candidate(session, *generation));
}

static void test_old_queued_got_ip_cannot_commit_unarmed_candidate(void)
{
    secure_bootstrap_wifi_context_t wifi;
    secure_bootstrap_wifi_session_t session;
    uint32_t generation = 0U;
    candidate_start(&wifi, &session, &generation);

    /* Before the fence, even a queued A GOT_IP is rejected by production FSM. */
    assert(!secure_bootstrap_wifi_candidate_arm_connect(
        &wifi, &session, generation, false, true));
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE);
    assert(session.phase == SECURE_BOOTSTRAP_SESSION_CANDIDATE);

    /* A true association check is not enough until both disconnect and queue
     * drain have been established and the current candidate is armed. */
    assert(!secure_bootstrap_wifi_candidate_arm_connect(
        &wifi, &session, generation, true, false));
    assert(!wifi.candidate_connect_armed);
    assert(secure_bootstrap_wifi_candidate_arm_connect(
        &wifi, &session, generation, true, true));
    assert(secure_bootstrap_wifi_candidate_got_ip_verified(
        &wifi, &session, generation, false) == SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(session.phase == SECURE_BOOTSTRAP_SESSION_CANDIDATE);
    assert(secure_bootstrap_wifi_candidate_got_ip_verified(
        &wifi, &session, generation, true) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST);
    assert(session.phase == SECURE_BOOTSTRAP_SESSION_COMMIT_RESERVED);
    assert(wifi.state == SECURE_BOOTSTRAP_WIFI_PERSISTING);
}

static void test_cancel_wins_during_association_verification(void)
{
    secure_bootstrap_wifi_context_t wifi;
    secure_bootstrap_wifi_session_t session;
    uint32_t generation = 0U;
    candidate_start(&wifi, &session, &generation);
    assert(secure_bootstrap_wifi_candidate_arm_connect(
        &wifi, &session, generation, true, true));
    assert(secure_bootstrap_wifi_session_reserve_cancel(&session, generation));
    assert(secure_bootstrap_wifi_candidate_got_ip_verified(
        &wifi, &session, generation, true) == SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(session.phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED);
    assert(wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE);
}

static void test_generation_rotation_does_not_arm_old_candidate(void)
{
    secure_bootstrap_wifi_context_t wifi;
    secure_bootstrap_wifi_session_t session;
    uint32_t old_generation = 0U;
    candidate_start(&wifi, &session, &old_generation);
    assert(secure_bootstrap_wifi_session_reserve_cancel(&session, old_generation));
    assert(secure_bootstrap_wifi_session_finish_cancel(&session, old_generation, true));
    uint32_t new_generation = 0U;
    secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    assert(secure_bootstrap_wifi_session_current(&session, &new_generation, &phase));
    assert(new_generation != old_generation);
    assert(phase == SECURE_BOOTSTRAP_SESSION_ACTIVE);
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, new_generation));
    assert(!secure_bootstrap_wifi_candidate_arm_connect(
        &wifi, &session, old_generation, true, true));
    assert(secure_bootstrap_wifi_candidate_arm_connect(
        &wifi, &session, new_generation, true, true));
    /* Equal SSID/IP cannot substitute for a current, verified association:
     * the caller must pass false until it verifies the live candidate config. */
    assert(secure_bootstrap_wifi_candidate_got_ip_verified(
        &wifi, &session, new_generation, false) == SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(session.phase == SECURE_BOOTSTRAP_SESSION_CANDIDATE);
}

int main(void)
{
    test_old_queued_got_ip_cannot_commit_unarmed_candidate();
    test_cancel_wins_during_association_verification();
    test_generation_rotation_does_not_arm_old_candidate();
    puts("PASS: production Wi-Fi candidate GOT_IP gate interleavings");
    return 0;
}
