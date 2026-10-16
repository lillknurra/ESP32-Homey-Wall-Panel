#include "secure_bootstrap.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t ssid[33];
} ap_record_fixture_t;

typedef struct {
    struct {
        uint8_t ssid[32];
        uint8_t password[64];
    } sta;
} candidate_fixture_t;

_Static_assert(offsetof(candidate_fixture_t, sta.password) == 32U,
               "fixture must model the IDF SSID/password adjacency");
_Static_assert(sizeof(((ap_record_fixture_t *)0)->ssid) == 33U,
               "IDF AP SSID width");
_Static_assert(sizeof(((candidate_fixture_t *)0)->sta.ssid) == 32U,
               "IDF STA SSID width");

/* Exact old expression from wifi_got_ip_matches_candidate(). */
static bool legacy_ssid_check(const ap_record_fixture_t *ap,
                              const candidate_fixture_t *candidate)
{
    return memcmp(ap->ssid, candidate->sta.ssid, sizeof(ap->ssid)) == 0;
}

static void set_ssids(ap_record_fixture_t *ap, candidate_fixture_t *candidate,
                      uint8_t active[32], const uint8_t *bytes, size_t length)
{
    memset(ap, 0, sizeof(*ap));
    memset(candidate, 0, sizeof(*candidate));
    memset(active, 0, 32U);
    assert(length > 0U && length <= 32U);
    memcpy(ap->ssid, bytes, length);
    ap->ssid[length] = 0U;
    memcpy(candidate->sta.ssid, bytes, length);
    memcpy(active, bytes, length);
}

static bool production_check(const ap_record_fixture_t *ap,
                             const candidate_fixture_t *candidate,
                             const uint8_t active[32])
{
    return secure_bootstrap_wifi_ssid_association_matches(
        ap->ssid, active, candidate->sta.ssid);
}

static void test_protected_password_reproduces_old_failure(void)
{
    const uint8_t ssid[] = "SyntheticProtected";
    ap_record_fixture_t ap;
    candidate_fixture_t candidate;
    uint8_t active[32];
    set_ssids(&ap, &candidate, active, ssid, sizeof(ssid) - 1U);
    memcpy(candidate.sta.password, "synthetic-only", sizeof("synthetic-only"));

    assert(!legacy_ssid_check(&ap, &candidate));
    assert(production_check(&ap, &candidate, active));
}

static void test_open_network(void)
{
    const uint8_t ssid[] = "OpenNet";
    ap_record_fixture_t ap;
    candidate_fixture_t candidate;
    uint8_t active[32];
    set_ssids(&ap, &candidate, active, ssid, sizeof(ssid) - 1U);
    assert(candidate.sta.password[0] == 0U);
    assert(legacy_ssid_check(&ap, &candidate));
    assert(production_check(&ap, &candidate, active));
}

static void test_lengths_1_31_32(void)
{
    const size_t lengths[] = {1U, 31U, 32U};
    for (size_t n = 0U; n < sizeof(lengths) / sizeof(lengths[0]); ++n) {
        uint8_t ssid[32];
        for (size_t i = 0U; i < lengths[n]; ++i) ssid[i] = (uint8_t)('A' + (i % 23U));
        ap_record_fixture_t ap;
        candidate_fixture_t candidate;
        uint8_t active[32];
        set_ssids(&ap, &candidate, active, ssid, lengths[n]);
        candidate.sta.password[0] = (uint8_t)'P';
        assert(production_check(&ap, &candidate, active));
    }
}

static void test_prefix_and_wrong_association_rejected(void)
{
    const uint8_t first[] = "same-prefix-A";
    const uint8_t second[] = "same-prefix-B";
    ap_record_fixture_t ap;
    candidate_fixture_t candidate;
    uint8_t active[32];
    set_ssids(&ap, &candidate, active, first, sizeof(first) - 1U);
    memset(ap.ssid, 0, sizeof(ap.ssid));
    memcpy(ap.ssid, second, sizeof(second) - 1U);
    assert(!production_check(&ap, &candidate, active));

    set_ssids(&ap, &candidate, active, first, sizeof(first) - 1U);
    memset(active, 0, sizeof(active));
    memcpy(active, second, sizeof(second) - 1U);
    assert(!production_check(&ap, &candidate, active));
}

static void test_candidate_commit_requires_real_ssid_check_and_session(void)
{
    const uint8_t ssid[] = "session-network";
    ap_record_fixture_t ap;
    candidate_fixture_t candidate;
    uint8_t active[32];
    set_ssids(&ap, &candidate, active, ssid, sizeof(ssid) - 1U);
    candidate.sta.password[0] = (uint8_t)'P';

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
    assert(secure_bootstrap_wifi_candidate_arm_connect(&wifi, &session, generation, true, true));

    bool ssid_verified = production_check(&ap, &candidate, active);
    assert(ssid_verified);
    assert(secure_bootstrap_wifi_candidate_got_ip_verified(
        &wifi, &session, generation + 1U, ssid_verified) == SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(secure_bootstrap_wifi_candidate_got_ip_verified(
        &wifi, &session, generation, ssid_verified) == SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST);

    secure_bootstrap_wifi_context_init(&wifi);
    secure_bootstrap_wifi_session_init(&session);
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED) ==
        SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING);
    generation = secure_bootstrap_wifi_session_begin(&session);
    assert(generation != 0U && secure_bootstrap_wifi_session_activate(&session, generation));
    assert(secure_bootstrap_wifi_transition(
        &wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED) == SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
    assert(secure_bootstrap_wifi_session_begin_candidate(&session, generation));
    assert(secure_bootstrap_wifi_candidate_arm_connect(&wifi, &session, generation, true, true));
    memset(ap.ssid, 0, sizeof(ap.ssid));
    memcpy(ap.ssid, "wrong-network", sizeof("wrong-network") - 1U);
    ssid_verified = production_check(&ap, &candidate, active);
    assert(!ssid_verified);
    assert(secure_bootstrap_wifi_candidate_got_ip_verified(
        &wifi, &session, generation, ssid_verified) == SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(session.phase == SECURE_BOOTSTRAP_SESSION_CANDIDATE);
}

int main(void)
{
    test_protected_password_reproduces_old_failure();
    test_open_network();
    test_lengths_1_31_32();
    test_prefix_and_wrong_association_rejected();
    test_candidate_commit_requires_real_ssid_check_and_session();
    puts("PASS: real production SSID comparator; legacy protected-network false negative reproduced");
    return 0;
}
