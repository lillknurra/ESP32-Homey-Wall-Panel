#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "../panel_render_diag.c"

static _Atomic uint64_t s_clock_us = 1U;
static _Atomic bool s_writer_done = false;
int64_t esp_timer_get_time(void)
{
    return (int64_t)atomic_fetch_add(&s_clock_us, 1U);
}
size_t heap_caps_get_free_size(unsigned caps) { (void)caps; return 0U; }
size_t heap_caps_get_minimum_free_size(unsigned caps) { (void)caps; return 0U; }
void panel_diag_test_log(const char *tag, const char *format, ...)
{ (void)tag; (void)format; }
TaskHandle_t xTaskCreateStatic(TaskFunction_t task, const char *name,
    uint32_t depth, void *arg, UBaseType_t priority, StackType_t *stack,
    StaticTask_t *control)
{ (void)task; (void)name; (void)depth; (void)arg; (void)priority; (void)stack; (void)control; return NULL; }
void vTaskDelay(uint32_t ticks) { (void)ticks; }
void vTaskSuspend(TaskHandle_t task) { (void)task; }

static void *writer(void *arg)
{
    const unsigned count = *(const unsigned *)arg;
    for (unsigned i = 0U; i < count; ++i) {
        panel_render_diag_vsync();
        panel_render_diag_decision(20U, 24U, false, false, true);
        panel_render_diag_restarted();
    }
    atomic_store(&s_writer_done, true);
    return NULL;
}
static void *reader(void *arg)
{
    (void)arg;
    do {
        panel_diag_metrics_t m = {0};
        uint64_t observed = 0U;
        assert(panel_render_diag_snapshot(&m, &observed));
        assert(m.shortfall == m.restart_decisions);
        assert(m.restart_paths <= m.restart_decisions);
        assert(m.last_deficit_us <= observed && m.last_restart_us <= observed);
    } while (!atomic_load(&s_writer_done));
    return NULL;
}

int main(void)
{
    panel_diag_metrics_t metrics = {0};
    uint64_t observed_at_us = 0U;
    unsigned samples = 0U;
    assert(!panel_render_diag_snapshot(NULL, &observed_at_us));
    assert(!panel_render_diag_snapshot(&metrics, NULL));
    for (unsigned i = 0U; i < 256U; ++i) {
        const uint8_t phase = (uint8_t)((i + 1U) & 63U);
        const bool sample = panel_diag_bounce_sample_due(phase);
        assert(sample == (((i + 1U) % 64U) == 0U));
        if (sample) ++samples;
    }
    assert(samples == 4U);
    panel_render_diag_bounce_sample(700U);
    panel_render_diag_vsync();
    panel_render_diag_decision(20U, 24U, false, false, true);
    panel_render_diag_restarted();
    const uint32_t ring_count_before = s_state.count;
    assert(panel_render_diag_snapshot(&metrics, &observed_at_us));
    assert(metrics.eof == 20U && metrics.bounce_samples == 1U && metrics.bounce_max_us == 700U);
    assert(metrics.shortfall == 1U && metrics.startup_shortfall == 1U);
    assert(metrics.restart_decisions == 1U && metrics.restart_paths == 1U);
    assert(metrics.last_actual == 20U && metrics.expected == 24U);
    assert(metrics.last_deficit_us != 0U && metrics.last_restart_us != 0U);
    assert(s_state.count == ring_count_before); /* Snapshot neither drains nor resets. */
    panel_diag_metrics_t repeated = {0};
    uint64_t repeated_at = 0U;
    assert(panel_render_diag_snapshot(&repeated, &repeated_at));
    assert(repeated.eof == metrics.eof && repeated.shortfall == metrics.shortfall);
    assert(s_state.count == ring_count_before);

    const unsigned iterations = 5000U;
    pthread_t write_thread, read_thread;
    assert(pthread_create(&write_thread, NULL, writer, (void *)&iterations) == 0);
    assert(pthread_create(&read_thread, NULL, reader, NULL) == 0);
    assert(pthread_join(write_thread, NULL) == 0);
    assert(pthread_join(read_thread, NULL) == 0);
    assert(panel_render_diag_snapshot(&metrics, &observed_at_us));
    assert(metrics.shortfall == iterations + 1U);
    assert(metrics.restart_decisions == iterations + 1U);
    assert(metrics.restart_paths == iterations + 1U);
    assert(metrics.eof == (iterations + 1U) * 20U);
    assert(metrics.dropped_events > 0U);

    panel_diag_state_t policy_state = {0};
    panel_diag_decision(&policy_state, 1U, 40U, 24U, false, true, true);
    panel_diag_decision(&policy_state, 2U, 80U, 24U, false, true, true);
    panel_diag_decision(&policy_state, 3U, 120U, 24U, false, true, true);
    assert(policy_state.metrics.eof == 120U);
    assert(policy_state.metrics.shortfall == 0U);

    panel_diag_state_t overflow_state = {0};
    overflow_state.metrics.eof = UINT32_MAX - 10U;
    panel_diag_decision(&overflow_state, 1U, 20U, 24U, false, false, false);
    assert(overflow_state.metrics.eof == 9U);

    panel_diag_state_t policy_overflow_state = {0};
    panel_diag_decision(&policy_overflow_state, 1U, UINT32_MAX - 5U, 24U,
                        false, true, false);
    panel_diag_decision(&policy_overflow_state, 2U, 3U, 24U,
                        false, true, false);
    assert(policy_overflow_state.metrics.eof == 3U);
    puts("PANEL_RENDER_DIAG_SNAPSHOT_ATOMIC_NONDESTRUCTIVE CONCURRENCY PASS");
    return 0;
}
