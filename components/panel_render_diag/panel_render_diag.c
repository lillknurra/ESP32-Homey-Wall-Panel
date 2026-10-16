#include "panel_render_diag.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

/* No PSRAM allocation, persistence or callback registration. */
static DRAM_ATTR panel_diag_state_t s_state;
static DRAM_ATTR portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static StackType_t s_report_stack[4096 / sizeof(StackType_t)];
static StaticTask_t s_report_task;
static bool s_started;

bool IRAM_ATTR panel_render_diag_eof(void)
{
    portENTER_CRITICAL_ISR(&s_mux);
    const bool sample = (++s_state.metrics.eof % 64U) == 0U;
    portEXIT_CRITICAL_ISR(&s_mux);
    return sample;
}
void IRAM_ATTR panel_render_diag_bounce_sample(uint32_t elapsed_us)
{
    portENTER_CRITICAL_ISR(&s_mux);
    s_state.metrics.bounce_samples++;
    if (elapsed_us > s_state.metrics.bounce_max_us) s_state.metrics.bounce_max_us = elapsed_us;
    portEXIT_CRITICAL_ISR(&s_mux);
}
void IRAM_ATTR panel_render_diag_vsync(void)
{
    const uint64_t us = (uint64_t)esp_timer_get_time();
    portENTER_CRITICAL_ISR(&s_mux);
    panel_diag_vsync(&s_state, us);
    portEXIT_CRITICAL_ISR(&s_mux);
}
void IRAM_ATTR panel_render_diag_decision(uint32_t actual, uint32_t expected,
                                         bool requested, bool policy, bool restart)
{
    portENTER_CRITICAL_ISR(&s_mux);
    panel_diag_decision(&s_state, s_state.metrics.last_vsync_us,
                        actual, expected, requested, policy, restart);
    portEXIT_CRITICAL_ISR(&s_mux);
}
void IRAM_ATTR panel_render_diag_restart_requested(void)
{
    portENTER_CRITICAL_SAFE(&s_mux);
    s_state.metrics.request_calls++;
    portEXIT_CRITICAL_SAFE(&s_mux);
}
void IRAM_ATTR panel_render_diag_restarted(void)
{
    const uint64_t us = (uint64_t)esp_timer_get_time();
    portENTER_CRITICAL_ISR(&s_mux);
    panel_diag_restarted(&s_state, us);
    portEXIT_CRITICAL_ISR(&s_mux);
}
void panel_render_diag_flash_interval(uint64_t start_us, uint32_t elapsed_us, int result)
{
    portENTER_CRITICAL(&s_mux);
    s_state.metrics.flash_count++;
    if (elapsed_us > s_state.metrics.flash_max_us) s_state.metrics.flash_max_us = elapsed_us;
    panel_diag_event(&s_state, start_us, PANEL_DIAG_EVENT_FLASH, elapsed_us, (uint32_t)result);
    portEXIT_CRITICAL(&s_mux);
}
void IRAM_ATTR panel_render_diag_alloc_failure(size_t bytes, uint32_t caps)
{
    /* Called by the existing owning allocator hook; never replace that hook. */
    const uint64_t us = (uint64_t)esp_timer_get_time();
    portENTER_CRITICAL_SAFE(&s_mux);
    s_state.metrics.alloc_failures++;
    panel_diag_event(&s_state, us, PANEL_DIAG_EVENT_ALLOC, (uint32_t)bytes, caps);
    portEXIT_CRITICAL_SAFE(&s_mux);
}
static void report_task(void *arg)
{
    (void)arg;
    /* Hard bound: one aggregate line per second for 15 minutes after startup.
     * Capture must occur within this window; no reset/rearm is implied. */
    for (unsigned report = 0; report < 900U; ++report) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        panel_diag_metrics_t m;
        portENTER_CRITICAL(&s_mux);
        m = s_state.metrics;
        portEXIT_CRITICAL(&s_mux);
        char events[200];
        size_t used = 0;
        events[0] = '\0';
        for (unsigned i = 0; i < 4U; ++i) {
            panel_diag_event_t event;
            portENTER_CRITICAL(&s_mux);
            bool present = panel_diag_pop(&s_state, &event);
            portEXIT_CRITICAL(&s_mux);
            if (!present) break;
            int n = snprintf(events + used, sizeof(events) - used, "%s%u,%llu,%u,%u",
                             used ? ";" : "", (unsigned)event.kind,
                             (unsigned long long)event.us, (unsigned)event.a, (unsigned)event.b);
            if (n < 0 || (size_t)n >= sizeof(events) - used) break;
            used += (size_t)n;
        }
        ESP_LOGI("RGB_DIAG", "V32 us=%llu eof=%u vs=%u sf=%u startup_sf=%u req=%u decisions=%u paths=%u actual=%u expected=%u vs_min=%u vs_max=%u samples=%u bounce_max=%u flash=%u flash_max=%u alloc=%u dropped=%u internal=%u minimum=%u psram=%u events=[%s]",
                 (unsigned long long)esp_timer_get_time(), (unsigned)m.eof, (unsigned)m.vsync,
                 (unsigned)m.shortfall, (unsigned)m.startup_shortfall, (unsigned)m.request_calls,
                 (unsigned)m.restart_decisions, (unsigned)m.restart_paths, (unsigned)m.last_actual,
                 (unsigned)m.expected, (unsigned)m.vsync_min_us, (unsigned)m.vsync_max_us,
                 (unsigned)m.bounce_samples, (unsigned)m.bounce_max_us, (unsigned)m.flash_count,
                 (unsigned)m.flash_max_us, (unsigned)m.alloc_failures, (unsigned)m.dropped_events,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT), events);
    }
    /* Keep the static task dormant: deleting static storage is unnecessary. */
    vTaskSuspend(NULL);
}
void panel_render_diag_start(void)
{
    if (s_started) return;
    s_started = true;
    if (!xTaskCreateStatic(report_task, "rgb_diag", sizeof(s_report_stack),
                           NULL, 0, s_report_stack, &s_report_task)) {
        ESP_LOGE("RGB_DIAG", "V32 reporter_start=FAIL");
    }
}
