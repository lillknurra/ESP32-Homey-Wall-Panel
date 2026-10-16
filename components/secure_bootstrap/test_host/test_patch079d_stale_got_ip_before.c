#include "secure_bootstrap.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    secure_bootstrap_wifi_context_t wifi;
    secure_bootstrap_wifi_session_t session;
    secure_bootstrap_wifi_context_init(&wifi);
    secure_bootstrap_wifi_session_init(&session);
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING);
    uint32_t generation = secure_bootstrap_wifi_session_begin(&session);
    assert(generation != 0U);
    assert(secure_bootstrap_wifi_session_activate(&session, generation));
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, generation));
    /* Patch079B accepted any current candidate GOT_IP and reserved commit;
     * this models a delayed A event arriving after B became global state. */
    assert(secure_bootstrap_wifi_session_reserve_commit(&session, generation));
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST);
    puts("EXPECTED_FAIL_BEFORE_FIX: stale GOT_IP decision reached PERSISTING");
    return 0;
}
