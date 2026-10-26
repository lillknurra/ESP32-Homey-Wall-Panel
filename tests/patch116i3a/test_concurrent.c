/* Host adapter stress test; no hardware or network, pthreads emulate FreeRTOS
 * dual-task access to the small resource cache. */
#include "panel_update_i3_probe.h"
#include "panel_update_i3a_web_diag.h"
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static _Thread_local uint32_t frame;
static atomic_bool writer_done;
static uint32_t samples_seen;
void test_log(const char *level, const char *tag, const char *format, ...)
{ (void)level; assert(strcmp(tag, "panel_update_observer") == 0); assert(format); }
size_t heap_caps_get_free_size(unsigned caps)
{ assert(caps == 1U); return 65536U + frame; }
size_t heap_caps_get_largest_free_block(unsigned caps)
{ assert(caps == 1U); return 32768U + frame; }
size_t heap_caps_get_minimum_free_size(unsigned caps)
{ assert(caps == 1U); return 16384U + frame; }
unsigned uxTaskGetStackHighWaterMark(void *task)
{ assert(task == NULL); return 4096U + frame; }
int esp_reset_reason(void) { return 1; }

static void *write_samples(void *ignored)
{
    (void)ignored;
    for (uint32_t i = 0; i < 1000U; ++i) {
        frame = i;
        panel_update_i3_owner_input_t in = {
            .monotonic_ms = UINT64_C(60000) * (uint64_t)i,
            .wifi_online = true,
            .ui_allocated = true,
            .homey_ready = (i & 1U) != 0U,
            .operator_trial_approved = false,
        };
        panel_update_i3_owner_active_tick(&in);
        if ((i % 7U) == 0U) sched_yield();
    }
    atomic_store(&writer_done, true);
    return NULL;
}
static void *read_samples(void *ignored)
{
    (void)ignored;
    uint64_t last = 0;
    size_t loops = 0;
    for (;;) {
        panel_update_i3a_snapshot_t s = {0};
        if (panel_update_i3a_snapshot_copy(&s)) {
            assert(s.monotonic_ms >= last);
            assert(s.internal_free_bytes >= 65536U);
            assert(s.internal_free_bytes - 32768U == s.internal_largest_block_bytes);
            assert(s.internal_free_bytes - 49152U == s.internal_minimum_free_bytes);
            assert(s.internal_free_bytes - 61440U == s.rotation_stack_margin_bytes);
            assert(s.reset_reason == 1U);
            assert(s.wifi_online && s.ui_allocated);
            last = s.monotonic_ms;
            ++samples_seen;
        }
        if (atomic_load(&writer_done) && loops >= 5000U) break;
        if (++loops % 13U == 0U) sched_yield();
    }
    return NULL;
}
int main(void)
{
    panel_update_i3a_snapshot_t snapshot = {0};
    assert(!panel_update_i3a_snapshot_copy(NULL));
    assert(!panel_update_i3a_snapshot_copy(&snapshot));
    pthread_t producer, consumer;
    assert(pthread_create(&producer, NULL, write_samples, NULL) == 0);
    assert(pthread_create(&consumer, NULL, read_samples, NULL) == 0);
    assert(pthread_join(producer, NULL) == 0);
    assert(pthread_join(consumer, NULL) == 0);
    assert(panel_update_i3a_snapshot_copy(&snapshot));
    assert(snapshot.monotonic_ms == UINT64_C(60000) * 999U);
    assert(snapshot.internal_free_bytes == 65536U + 999U);
    assert(samples_seen > 0U);
    puts("PATCH116I3A CONCURRENT SNAPSHOT PASS");
    return 0;
}
