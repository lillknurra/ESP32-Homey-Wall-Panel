#include "secure_bootstrap.h"
#include <assert.h>
#include <stdio.h>

static void first_install_candidate_start(secure_bootstrap_wifi_context_t *wifi,
                                          secure_bootstrap_wifi_session_t *session,
                                          uint32_t generation)
{
    assert(secure_bootstrap_wifi_session_begin_candidate(session, generation));
    assert(secure_bootstrap_wifi_transition(
        wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED) &
        SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
}

int main(void)
{
    secure_bootstrap_wifi_context_t wifi;
    secure_bootstrap_wifi_context_init(&wifi);
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED) &
        SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING);

    secure_bootstrap_wifi_session_t session;
    secure_bootstrap_wifi_session_init(&session);
    uint32_t original = secure_bootstrap_wifi_session_begin(&session);
    assert(original != 0U && secure_bootstrap_wifi_session_activate(&session, original));
    first_install_candidate_start(&wifi, &session, original);
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED) &
        SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
    assert(wifi.retry_count != 0U);
    assert(secure_bootstrap_wifi_session_reserve_cancel(&session, original));

#ifdef PATCH079B_PRE_FIX
    /* Exact old no-saved worker behavior: finish/reopen the session without
     * applying USER_CANCEL to the production Wi-Fi FSM. */
    assert(secure_bootstrap_wifi_session_finish_cancel(&session, original, true));
#else
    assert(secure_bootstrap_wifi_cancel_first_install(&wifi, &session, original));
#endif

    uint32_t fresh_generation = 0U;
    secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    assert(secure_bootstrap_wifi_session_current(&session, &fresh_generation, &phase));
    assert(fresh_generation != original && phase == SECURE_BOOTSTRAP_SESSION_ACTIVE);

#ifdef PATCH079B_PRE_FIX
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, fresh_generation));
    const uint32_t actions = secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
    assert((actions & SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT) == 0U);
    assert(wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE);
    puts("EXPECTED_FAIL_BEFORE_FIX: stale candidate FSM rejects resubmission CONNECT");
#else
    assert(wifi.state == SECURE_BOOTSTRAP_WIFI_PROVISIONING);
    assert(!wifi.candidate_active && wifi.retry_count == 0U);
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, fresh_generation));
    const uint32_t actions = secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
    assert((actions & SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT) != 0U);
    assert(wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE);
    puts("PASS: first-install cancel rotates session and resubmission produces CONNECT");
#endif
    return 0;
}
