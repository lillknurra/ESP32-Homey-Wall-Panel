#include "panel_ota_async_policy.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); abort(); \
} } while (0)

static panel_ota_async_limits_t limits(void)
{
    return (panel_ota_async_limits_t){
        .total_ms = 60000U, .dns_ms = 10000U, .tls_ms = 12000U,
        .request_ms = 5000U, .headers_ms = 8000U, .preamble_ms = 8000U,
        .body_ms = 45000U, .verify_ms = 10000U, .boot_select_ms = 5000U,
    };
}

static panel_ota_async_state_t fresh(void)
{
    panel_ota_async_state_t s;
    panel_ota_async_limits_t l = limits();
    CHECK(panel_ota_async_begin(&s, &l, 773U, UINT64_C(100000000)));
    CHECK(s.phase == PANEL_OTA_ASYNC_DNS);
    CHECK(!panel_ota_async_can_begin_flash(&s, s.last_us, true, true));
    CHECK(!panel_ota_async_can_select_boot(&s, s.last_us, true, true));
    return s;
}

static panel_ota_async_outcome_t send_event(panel_ota_async_state_t *s,
                                             panel_ota_async_event_type_t t,
                                             uint32_t size, uint64_t at)
{
    const panel_ota_async_event_t ev = {t, s->generation, size};
    return panel_ota_async_step(s, &ev, at, true, true);
}

static panel_ota_async_outcome_t send_event_now(panel_ota_async_state_t *s,
                                                 panel_ota_async_event_type_t t,
                                                 uint32_t size)
{
    return send_event(s, t, size, s->last_us);
}

static void preamble(panel_ota_async_state_t *s, uint32_t length)
{
    CHECK(send_event_now(s, PANEL_OTA_ASYNC_DNS_RESOLVED, 0) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(s->phase == PANEL_OTA_ASYNC_TLS);
    CHECK(!panel_ota_async_can_begin_flash(s, s->last_us, true, true));
    CHECK(send_event_now(s, PANEL_OTA_ASYNC_TLS_VERIFIED, 0) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(send_event_now(s, PANEL_OTA_ASYNC_REQUEST_SENT, 0) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(send_event_now(s, PANEL_OTA_ASYNC_HEADERS_ACCEPTED, length) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(!panel_ota_async_can_begin_flash(s, s->last_us, true, true));
    CHECK(send_event_now(s, PANEL_OTA_ASYNC_PREAMBLE_VERIFIED, 0) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(panel_ota_async_can_begin_flash(s, s->last_us, true, true));
    CHECK(!panel_ota_async_can_begin_flash(s, s->last_us, false, true));
    CHECK(!panel_ota_async_can_begin_flash(s, s->last_us, true, false));
    CHECK(!panel_ota_async_can_begin_flash(s, s->phase_started_us +
        (uint64_t)s->limits.body_ms * 1000ULL, true, true));
}

static void test_success_and_order(void)
{
    panel_ota_async_state_t s = fresh();
    preamble(&s, 4096U);
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_BODY_BYTES, 1000U) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_BODY_BYTES, 4096U - 288U - 1000U) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(s.phase == PANEL_OTA_ASYNC_VERIFY);
    CHECK(!panel_ota_async_can_select_boot(&s, s.last_us, true, true));
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_IMAGE_VALIDATED, 0U) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(panel_ota_async_can_select_boot(&s, s.last_us, true, true));
    CHECK(!panel_ota_async_can_select_boot(&s, s.last_us, false, true));
    CHECK(!panel_ota_async_can_select_boot(&s, s.last_us, true, false));
    CHECK(!panel_ota_async_can_select_boot(&s, s.phase_started_us +
        (uint64_t)s.limits.boot_select_ms * 1000ULL, true, true));
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_BOOT_SELECTED, 0U) == PANEL_OTA_ASYNC_FINISHED);
    CHECK(s.phase == PANEL_OTA_ASYNC_DONE);
    CHECK(!panel_ota_async_can_select_boot(&s, s.last_us, true, true));
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_POLL, 0U) == PANEL_OTA_ASYNC_FINISHED);
    CHECK(!panel_ota_async_can_begin_flash(&s, s.last_us, true, true));
}

static void test_reject_bad_order_and_length(void)
{
    panel_ota_async_state_t s = fresh();
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_BODY_BYTES, 123U) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(s.reason == PANEL_OTA_ASYNC_REASON_ORDER);
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_DNS_RESOLVED, 0U) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(!panel_ota_async_can_begin_flash(&s, s.last_us, true, true));
    s = fresh();
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_DNS_RESOLVED, 1U) == PANEL_OTA_ASYNC_REJECTED);
    s = fresh();
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_DNS_RESOLVED, 0U) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_TLS_VERIFIED, 0U) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_REQUEST_SENT, 0U) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_HEADERS_ACCEPTED, 288U) == PANEL_OTA_ASYNC_REJECTED);
    s = fresh();
    preamble(&s, 4000U);
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_BODY_BYTES, 4000U) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(!panel_ota_async_can_select_boot(&s, s.last_us, true, true));
    s = fresh();
    preamble(&s, 4000U);
    CHECK(send_event_now(&s, PANEL_OTA_ASYNC_IMAGE_VALIDATED, 0U) == PANEL_OTA_ASYNC_REJECTED);
}

static void test_timeout_and_stale(void)
{
    panel_ota_async_state_t s = fresh();
    const uint64_t t = s.last_us;
    const panel_ota_async_event_t old = {PANEL_OTA_ASYNC_DNS_RESOLVED, 772U, 0U};
    CHECK(panel_ota_async_step(&s, &old, t, true, true) == PANEL_OTA_ASYNC_IGNORE_STALE);
    CHECK(s.phase == PANEL_OTA_ASYNC_DNS);
    CHECK(panel_ota_async_step(&s, &old, t + 10000U, true, true) == PANEL_OTA_ASYNC_IGNORE_STALE);
    CHECK(send_event(&s, PANEL_OTA_ASYNC_DNS_RESOLVED, 0U, t + 9000U) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(s.reason == PANEL_OTA_ASYNC_REASON_TIMEOUT);  /* time reversed */

    s = fresh();
    CHECK(send_event(&s, PANEL_OTA_ASYNC_DNS_RESOLVED, 0U, t + 10000000U) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(s.reason == PANEL_OTA_ASYNC_REASON_TIMEOUT);
    s = fresh();
    CHECK(send_event(&s, PANEL_OTA_ASYNC_DNS_RESOLVED, 0U, t + 5000U) == PANEL_OTA_ASYNC_CONTINUE);
    CHECK(send_event(&s, PANEL_OTA_ASYNC_TLS_VERIFIED, 0U, t + 12005000U) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(s.reason == PANEL_OTA_ASYNC_REASON_TIMEOUT);

    s = fresh();
    CHECK(send_event(&s, PANEL_OTA_ASYNC_POLL, 0U, t + 60000000U) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(s.reason == PANEL_OTA_ASYNC_REASON_TIMEOUT);
}

static void test_auth_cancel_and_stale_generation(void)
{
    panel_ota_async_state_t s = fresh();
    panel_ota_async_event_t ev = {PANEL_OTA_ASYNC_DNS_RESOLVED, 772U, 0U};
    CHECK(panel_ota_async_step(&s, &ev, s.last_us, false, true) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(s.reason == PANEL_OTA_ASYNC_REASON_AUTH_REVOKED);
    s = fresh();
    CHECK(panel_ota_async_step(&s, &ev, s.last_us, true, false) == PANEL_OTA_ASYNC_REJECTED);
    s = fresh();
    ev.kind = PANEL_OTA_ASYNC_CANCEL;
    ev.generation = s.generation;
    CHECK(panel_ota_async_step(&s, &ev, s.last_us, true, true) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(s.reason == PANEL_OTA_ASYNC_REASON_CANCELLED);
    s = fresh();
    ev.kind = PANEL_OTA_ASYNC_IO_ERROR;
    CHECK(panel_ota_async_step(&s, &ev, s.last_us, true, true) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(s.reason == PANEL_OTA_ASYNC_REASON_IO);
}

static void test_limits(void)
{
    panel_ota_async_state_t s;
    panel_ota_async_limits_t l = limits();
    CHECK(!panel_ota_async_begin(NULL, &l, 773U, 0U));
    CHECK(!panel_ota_async_begin(&s, NULL, 773U, 0U));
    CHECK(!panel_ota_async_begin(&s, &l, 0U, 0U));
    l.dns_ms = l.total_ms + 1U;
    CHECK(!panel_ota_async_begin(&s, &l, 773U, 0U));
    l = limits();
    l.total_ms = PANEL_OTA_ASYNC_MAX_TOTAL_MS + 1U;
    CHECK(!panel_ota_async_begin(&s, &l, 773U, 0U));
    l = limits();
    l.tls_ms = 0U;
    CHECK(!panel_ota_async_begin(&s, &l, 773U, 0U));
}

static uint32_t random32(uint32_t *seed)
{
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return *seed;
}

static void test_deterministic_event_fuzz(void)
{
    uint32_t seed = UINT32_C(0x98ab50d4);
    for (unsigned trial = 0; trial < 1500U; ++trial) {
        panel_ota_async_state_t s = fresh();
        bool preamble_seen = false;
        bool image_seen = false;
        uint64_t t = s.started_us;
        for (unsigned n = 0; n < 100U; ++n) {
            uint32_t rnd = random32(&seed);
            const panel_ota_async_event_t ev = {
                (panel_ota_async_event_type_t)(rnd % (PANEL_OTA_ASYNC_IO_ERROR + 1)),
                rnd & 8U ? s.generation : s.generation - 1U,
                (rnd >> 10) & 0xfffU,
            };
            if (rnd & 32U) t += rnd & 0x3ffU;
            if (ev.generation == s.generation &&
                ev.kind == PANEL_OTA_ASYNC_PREAMBLE_VERIFIED &&
                s.phase == PANEL_OTA_ASYNC_PREAMBLE && ev.length == 0U)
                preamble_seen = true;
            if (ev.generation == s.generation &&
                ev.kind == PANEL_OTA_ASYNC_IMAGE_VALIDATED &&
                s.phase == PANEL_OTA_ASYNC_VERIFY && ev.length == 0U)
                image_seen = true;
            (void)panel_ota_async_step(&s, &ev, t, true, true);
            CHECK(!panel_ota_async_can_begin_flash(&s, s.last_us, true, true) || preamble_seen);
            CHECK(!panel_ota_async_can_select_boot(&s, s.last_us, true, true) || image_seen);
            if (s.phase == PANEL_OTA_ASYNC_FAILED || s.phase == PANEL_OTA_ASYNC_DONE) break;
        }
    }
}

static void test_frozen_clock_fails_closed(void)
{
    panel_ota_async_state_t s = fresh();
    panel_ota_async_event_t ev = {PANEL_OTA_ASYNC_POLL, s.generation, 0U};
    for (unsigned i = 0U; i < PANEL_OTA_ASYNC_MAX_EVENTS; ++i) {
        CHECK(panel_ota_async_step(&s, &ev, s.started_us, true, true) == PANEL_OTA_ASYNC_CONTINUE);
    }
    CHECK(panel_ota_async_step(&s, &ev, s.started_us, true, true) == PANEL_OTA_ASYNC_REJECTED);
    CHECK(s.reason == PANEL_OTA_ASYNC_REASON_EVENT_LIMIT);
}

int main(void)
{
    test_success_and_order();
    test_reject_bad_order_and_length();
    test_timeout_and_stale();
    test_auth_cancel_and_stale_generation();
    test_limits();
    test_deterministic_event_fuzz();
    test_frozen_clock_fails_closed();
    printf("PATCH116G_ASYNC_POLICY_TESTS=PASS assertions=%u\n", checks);
    return 0;
}
