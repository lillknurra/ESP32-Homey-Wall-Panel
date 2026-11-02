#include "runtime_event_ring.h"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#define THREADS 6U
#define WRITES_PER_THREAD 500U

static void *writer(void *arg)
{
    const uintptr_t id = (uintptr_t)arg;
    for (uint32_t i = 0U; i < WRITES_PER_THREAD; ++i) {
        assert(runtime_event_ring_record_at(i + 1U, RUNTIME_EVENT_HTTP_PERFORM_END,
            RUNTIME_EVENT_STAGE_DEVICES, (int32_t)id, i, (uint32_t)id));
    }
    return NULL;
}

int main(void)
{
    runtime_event_ring_test_reset();
    runtime_event_ring_set_context(1234U, 7U, RUNTIME_EVENT_ORIGIN_PERIODIC);
    for (uint32_t i = 0U; i < RUNTIME_EVENT_RING_CAPACITY + 3U; ++i) {
        assert(runtime_event_ring_record_at(100U + i, RUNTIME_EVENT_STAGE_BEGIN,
            RUNTIME_EVENT_STAGE_ZONES, 0, i, 0U));
    }
    runtime_event_record_t page[RUNTIME_EVENT_RING_PAGE_MAX];
    runtime_event_ring_info_t info = {0};
    size_t count = runtime_event_ring_copy_latest(page, RUNTIME_EVENT_RING_PAGE_MAX, &info);
    assert(count == RUNTIME_EVENT_RING_PAGE_MAX);
    assert(info.count == RUNTIME_EVENT_RING_CAPACITY);
    assert(info.overwritten_count == 3U);
    assert(info.dropped_count == 0U);
    assert(info.runtime_id == 1234U && info.inventory_sequence == 7U);
    assert(page[0].sequence == 40U && page[count - 1U].sequence == 51U);
    assert(page[0].monotonic_ms < page[count - 1U].monotonic_ms);

    assert(!runtime_event_ring_record_at(999U, 0U, 0U, 0, 0U, 0U));
    (void)runtime_event_ring_copy_latest(page, RUNTIME_EVENT_RING_PAGE_MAX, &info);
    assert(info.dropped_count == 1U);

    count = runtime_event_ring_copy_page(page, 4U, 44U, &info);
    assert(count == 4U);
    assert(page[0].sequence == 40U && page[count - 1U].sequence == 43U);
    assert(info.page_first_sequence == 40U && info.page_last_sequence == 43U);

    runtime_event_ring_test_reset();
    runtime_event_ring_set_context(765U, 11U, RUNTIME_EVENT_ORIGIN_BOOT_AUTO);
    pthread_t threads[THREADS];
    for (uintptr_t i = 0U; i < THREADS; ++i) {
        assert(pthread_create(&threads[i], NULL, writer, (void *)i) == 0);
    }
    for (size_t i = 0U; i < THREADS; ++i) assert(pthread_join(threads[i], NULL) == 0);
    count = runtime_event_ring_copy_latest(page, RUNTIME_EVENT_RING_PAGE_MAX, &info);
    assert(count == RUNTIME_EVENT_RING_PAGE_MAX);
    assert(info.count == RUNTIME_EVENT_RING_CAPACITY);
    assert(info.overwritten_count == THREADS * WRITES_PER_THREAD - RUNTIME_EVENT_RING_CAPACITY);
    for (size_t i = 1U; i < count; ++i) {
        assert(page[i].sequence == page[i - 1U].sequence + 1U);
    }
    assert(page[count - 1U].runtime_id == 765U);
    assert(page[count - 1U].inventory_sequence == 11U);
    assert(page[count - 1U].origin == RUNTIME_EVENT_ORIGIN_BOOT_AUTO);

    puts("runtime event ring: capacity, overwrite, drop, order, context, and concurrency PASS");
    return 0;
}
