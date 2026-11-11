#include "lwip/close_diag.h"
#include "lwip/sys.h"

#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

pthread_mutex_t s_test_sys_protect_mutex = PTHREAD_MUTEX_INITIALIZER;
static _Atomic uint32_t s_now;

uint32_t sys_now(void)
{
    return atomic_fetch_add_explicit(&s_now, 1U, memory_order_relaxed);
}

sys_thread_t xTaskGetCurrentTaskHandle(void)
{
    return pthread_self();
}

static void *record_many(void *unused)
{
    (void)unused;
    for (uint32_t i = 0U; i < 1000U; ++i) {
        lwip_close_diag_socket_event(
            LWIP_CLOSE_DIAG_RELEASE_CONFIRMED,
            LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND |
                LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED,
            0, 0);
    }
    return NULL;
}

static void *record_unrelated_close(void *arg)
{
    (void)arg;
    lwip_close_diag_socket_event(
        LWIP_CLOSE_DIAG_RELEASE_CONFIRMED,
        LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND |
            LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED,
        0, 0);
    return NULL;
}

static const lwip_close_diag_event_t *find_kind(
    const lwip_close_diag_snapshot_t *snapshot, uint8_t kind, uint32_t after)
{
    for (uint32_t i = 0U; i < snapshot->event_count; ++i) {
        if (snapshot->events[i].kind == kind &&
            snapshot->events[i].sequence > after) {
            return &snapshot->events[i];
        }
    }
    return NULL;
}

int main(void)
{
    lwip_close_diag_socket_event(LWIP_CLOSE_DIAG_RELEASE_CONFIRMED,
        LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND |
            LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED, 0, 0);
    lwip_close_diag_socket_event(LWIP_CLOSE_DIAG_RELEASE_FAILED,
        LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND, -1, 11);
    lwip_close_diag_socket_event(LWIP_CLOSE_DIAG_NO_SOCKET_ENTRY, 0U, -1, 9);
    lwip_close_diag_transport_event(LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_BEGIN,
        LWIP_CLOSE_DIAG_CLOSE_ATTEMPT_EXPECTED,
        LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED, -1, 0);

    lwip_close_diag_snapshot_t first = {0};
    lwip_close_diag_snapshot(&first);
    assert(first.event_count == 4U && first.newest_sequence == 4U);
    assert(first.overwritten_count == 0U);
    assert(first.events[0].outcome == LWIP_CLOSE_DIAG_RELEASE_CONFIRMED);
    assert(first.events[0].flags & LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED);
    assert(first.events[1].outcome == LWIP_CLOSE_DIAG_RELEASE_FAILED);
    assert(first.events[1].error_number == 11);
    assert(first.events[2].outcome == LWIP_CLOSE_DIAG_NO_SOCKET_ENTRY);
    assert(first.events[3].outcome == LWIP_CLOSE_DIAG_CLOSE_ATTEMPT_EXPECTED);

    /* Homey TLS close: socket event inherits the active task scope. */
    const uint32_t before_homey = first.newest_sequence;
    const uint32_t homey_id = lwip_close_diag_scope_begin();
    assert(homey_id != 0U);
    lwip_close_diag_transport_event(
        LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_BEGIN,
        LWIP_CLOSE_DIAG_CLOSE_ATTEMPT_EXPECTED,
        LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED |
            LWIP_CLOSE_DIAG_FLAG_TLS_OBJECT_PRESENT |
            LWIP_CLOSE_DIAG_FLAG_TLS_SOCKET_VALID,
        0, 0);
    lwip_close_diag_socket_event(
        LWIP_CLOSE_DIAG_RELEASE_CONFIRMED,
        LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND |
            LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED,
        0, 0);

    /* A local diagnostic server close on another task is uncorrelated. */
    pthread_t diagnostic;
    assert(pthread_create(&diagnostic, NULL, record_unrelated_close, NULL) == 0);
    assert(pthread_join(diagnostic, NULL) == 0);

    lwip_close_diag_transport_event(
        LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_END,
        LWIP_CLOSE_DIAG_CLOSE_RESULT,
        LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED |
            LWIP_CLOSE_DIAG_FLAG_TLS_OBJECT_PRESENT |
            LWIP_CLOSE_DIAG_FLAG_TLS_SOCKET_VALID,
        0, 0);
    lwip_close_diag_scope_end(homey_id);
    lwip_close_diag_snapshot_t correlated = {0};
    lwip_close_diag_snapshot(&correlated);
    const lwip_close_diag_event_t *homey_begin = find_kind(
        &correlated, LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_BEGIN, before_homey);
    const lwip_close_diag_event_t *homey_end = find_kind(
        &correlated, LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_END, before_homey);
    assert(homey_begin != NULL && homey_end != NULL);
    assert(homey_begin->correlation_id == homey_id);
    assert(homey_end->correlation_id == homey_id);
    bool saw_correlated_socket_release = false;
    bool saw_uncorrelated_interleaved_close = false;
    for (uint32_t i = 0U; i < correlated.event_count; ++i) {
        const lwip_close_diag_event_t *event = &correlated.events[i];
        if (event->sequence <= before_homey || event->kind != LWIP_CLOSE_DIAG_SOCKET_CLOSE) continue;
        saw_correlated_socket_release |= event->correlation_id == homey_id &&
            event->outcome == LWIP_CLOSE_DIAG_RELEASE_CONFIRMED;
        saw_uncorrelated_interleaved_close |= event->correlation_id == 0U;
    }
    assert(saw_correlated_socket_release && saw_uncorrelated_interleaved_close);

    /* A wrapper -1 with no socket event remains visibly a secondary close. */
    const uint32_t before_secondary = correlated.newest_sequence;
    const uint32_t secondary_id = lwip_close_diag_scope_begin();
    assert(secondary_id != 0U);
    lwip_close_diag_transport_event(
        LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_BEGIN,
        LWIP_CLOSE_DIAG_CLOSE_ATTEMPT_EXPECTED,
        LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED, 0, 0);
    lwip_close_diag_transport_event(
        LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_END,
        LWIP_CLOSE_DIAG_CLOSE_RESULT,
        LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED, -1, 0);
    lwip_close_diag_scope_end(secondary_id);
    lwip_close_diag_snapshot(&correlated);
    bool secondary_socket_event = false;
    for (uint32_t i = 0U; i < correlated.event_count; ++i) {
        secondary_socket_event |= correlated.events[i].sequence > before_secondary &&
            correlated.events[i].kind == LWIP_CLOSE_DIAG_SOCKET_CLOSE;
    }
    homey_end = find_kind(&correlated, LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_END, before_secondary);
    assert(!secondary_socket_event && homey_end != NULL &&
        homey_end->correlation_id == secondary_id && homey_end->result == -1);

    /* A genuine lower close failure remains correlated and explicit. */
    const uint32_t before_failed_close = correlated.newest_sequence;
    const uint32_t failed_id = lwip_close_diag_scope_begin();
    assert(failed_id != 0U);
    lwip_close_diag_transport_event(
        LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_BEGIN,
        LWIP_CLOSE_DIAG_OPERATION_STARTED,
        LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED |
            LWIP_CLOSE_DIAG_FLAG_TLS_OBJECT_PRESENT,
        0, 0);
    lwip_close_diag_socket_event(
        LWIP_CLOSE_DIAG_RELEASE_FAILED,
        LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND,
        -1, 11);
    lwip_close_diag_transport_event(
        LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_END,
        LWIP_CLOSE_DIAG_OPERATION_FAILED,
        LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED, -1, 0);
    lwip_close_diag_scope_end(failed_id);
    lwip_close_diag_snapshot(&correlated);
    bool saw_correlated_release_failure = false;
    for (uint32_t i = 0U; i < correlated.event_count; ++i) {
        const lwip_close_diag_event_t *event = &correlated.events[i];
        saw_correlated_release_failure |= event->sequence > before_failed_close &&
            event->correlation_id == failed_id &&
            event->outcome == LWIP_CLOSE_DIAG_RELEASE_FAILED &&
            event->error_number == 11;
    }
    assert(saw_correlated_release_failure);

    /* Nested scopes suppress correlation instead of inheriting the parent. */
    const uint32_t before_nested = correlated.newest_sequence;
    const uint32_t parent_id = lwip_close_diag_scope_begin();
    assert(parent_id != 0U);
    assert(lwip_close_diag_scope_begin() == 0U);
    lwip_close_diag_socket_event(LWIP_CLOSE_DIAG_RELEASE_CONFIRMED,
        LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND |
            LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED, 0, 0);
    lwip_close_diag_scope_end(0U);
    lwip_close_diag_socket_event(LWIP_CLOSE_DIAG_RELEASE_CONFIRMED,
        LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND |
            LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED, 0, 0);
    lwip_close_diag_scope_end(parent_id);
    lwip_close_diag_snapshot(&correlated);
    bool nested_suppressed = false;
    bool parent_restored = false;
    for (uint32_t i = 0U; i < correlated.event_count; ++i) {
        const lwip_close_diag_event_t *event = &correlated.events[i];
        if (event->sequence <= before_nested || event->kind != LWIP_CLOSE_DIAG_SOCKET_CLOSE) continue;
        nested_suppressed |= event->correlation_id == 0U;
        parent_restored |= event->correlation_id == parent_id;
    }
    assert(nested_suppressed && parent_restored);

    /* Capture after ring overwrite cannot silently claim the old event. */
    const uint32_t target_id = lwip_close_diag_scope_begin();
    assert(target_id != 0U);
    lwip_close_diag_transport_event(
        LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_BEGIN,
        LWIP_CLOSE_DIAG_OPERATION_STARTED, 0U, 0, 0);
    lwip_close_diag_scope_end(target_id);
    for (uint32_t i = 0U; i < LWIP_CLOSE_DIAG_CAPACITY + 1U; ++i) {
        lwip_close_diag_socket_event(LWIP_CLOSE_DIAG_RELEASE_CONFIRMED,
            LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND |
                LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED, 0, 0);
    }
    lwip_close_diag_snapshot(&correlated);
    bool stale_marker_retained = false;
    for (uint32_t i = 0U; i < correlated.event_count; ++i) {
        stale_marker_retained |= correlated.events[i].correlation_id == target_id;
    }
    assert(!stale_marker_retained && correlated.overwritten_count > 0U);

    lwip_close_diag_snapshot_t repeated = {0};
    lwip_close_diag_snapshot(&repeated);
    assert(repeated.newest_sequence == correlated.newest_sequence);
    assert(repeated.event_count == correlated.event_count);
    assert(repeated.events[0].sequence == correlated.events[0].sequence);
    const uint32_t before_stress = repeated.newest_sequence;

    pthread_t a, b;
    assert(pthread_create(&a, NULL, record_many, NULL) == 0);
    assert(pthread_create(&b, NULL, record_many, NULL) == 0);
    assert(pthread_join(a, NULL) == 0 && pthread_join(b, NULL) == 0);
    lwip_close_diag_snapshot(&repeated);
    assert(repeated.newest_sequence == before_stress + 2000U);
    assert(repeated.event_count == LWIP_CLOSE_DIAG_CAPACITY);
    assert(repeated.overwritten_count == repeated.newest_sequence - LWIP_CLOSE_DIAG_CAPACITY);
    for (uint32_t i = 1U; i < repeated.event_count; ++i) {
        assert(repeated.events[i].sequence == repeated.events[i - 1U].sequence + 1U);
    }
    puts("CLOSE_DIAG_RING=PASS");
    puts("SNAPSHOT_NONDESTRUCTIVE=PASS");
    puts("CONCURRENT_WRITES_AND_READ=PASS");
    puts("BOUNDED_OVERWRITE_ACCOUNTING=PASS");
    puts("HOMEY_TASK_CORRELATION=PASS");
    puts("INTERLEAVED_OTHER_TASK_CLOSE_UNCORRELATED=PASS");
    puts("SECONDARY_CLOSE_WITHOUT_SOCKET_EVENT=PASS");
    puts("CORRELATED_FAILED_CLOSE=PASS");
    puts("NESTED_SCOPE_FAILS_UNCORRELATED=PASS");
    puts("RING_OVERWRITE_MARKED_AMBIGUOUS=PASS");
    return 0;
}
