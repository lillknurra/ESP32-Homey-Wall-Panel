#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Pure accounting only: never changes a driver decision. Single-panel diagnostic. */
enum { PANEL_DIAG_SHORTFALL = 1, PANEL_DIAG_REQUEST = 2, PANEL_DIAG_POLICY = 4 };
enum { PANEL_DIAG_EVENT_DEFICIT = 1, PANEL_DIAG_EVENT_RESTART = 2,
       PANEL_DIAG_EVENT_FLASH = 3, PANEL_DIAG_EVENT_ALLOC = 4 };
#define PANEL_DIAG_EVENTS 16U
typedef struct {
    uint64_t us;
    uint32_t kind, a, b;
} panel_diag_event_t;
typedef struct {
    uint64_t last_vsync_us, last_deficit_us, last_restart_us;
    uint32_t eof, vsync, shortfall, startup_shortfall, request_calls;
    uint32_t restart_decisions, restart_paths, last_reason, last_actual, expected;
    uint32_t vsync_min_us, vsync_max_us, bounce_samples, bounce_max_us;
    uint32_t flash_count, flash_max_us, alloc_failures, dropped_events;
} panel_diag_metrics_t;
typedef struct {
    panel_diag_metrics_t metrics;
    panel_diag_event_t events[PANEL_DIAG_EVENTS];
    uint32_t head, count;
} panel_diag_state_t;

static inline __attribute__((always_inline)) void panel_diag_event(panel_diag_state_t *s, uint64_t us,
                                    uint32_t kind, uint32_t a, uint32_t b)
{
    if (s->count == PANEL_DIAG_EVENTS) {
        s->metrics.dropped_events++;
        return; /* Preserve earliest pending evidence; report loss explicitly. */
    }
    s->events[(s->head + s->count) % PANEL_DIAG_EVENTS] =
        (panel_diag_event_t){us, kind, a, b};
    s->count++;
}
static inline __attribute__((always_inline)) bool panel_diag_pop(panel_diag_state_t *s, panel_diag_event_t *out)
{
    if (!s->count) return false;
    *out = s->events[s->head];
    s->head = (s->head + 1U) % PANEL_DIAG_EVENTS;
    s->count--;
    return true;
}
static inline __attribute__((always_inline)) void panel_diag_vsync(panel_diag_state_t *s, uint64_t us)
{
    panel_diag_metrics_t *m = &s->metrics;
    if (m->vsync && us >= m->last_vsync_us) {
        uint32_t dt = (uint32_t)(us - m->last_vsync_us);
        if (!m->vsync_min_us || dt < m->vsync_min_us) m->vsync_min_us = dt;
        if (dt > m->vsync_max_us) m->vsync_max_us = dt;
    }
    m->vsync++;
    m->last_vsync_us = us;
}
static inline __attribute__((always_inline)) void panel_diag_decision(panel_diag_state_t *s, uint64_t us,
                                       uint32_t actual, uint32_t expected,
                                       bool requested, bool policy, bool restart)
{
    panel_diag_metrics_t *m = &s->metrics;
    /* The policy-on branch never evaluates EOF; mirror that exact distinction. */
    bool deficit = !policy && actual < expected;
    uint32_t reason = (deficit ? PANEL_DIAG_SHORTFALL : 0U) |
        (requested ? PANEL_DIAG_REQUEST : 0U) | (policy ? PANEL_DIAG_POLICY : 0U);
    m->last_actual = actual;
    m->expected = expected;
    if (deficit) {
        m->shortfall++;
        if (m->vsync <= 2U) m->startup_shortfall++;
        m->last_deficit_us = us;
        panel_diag_event(s, us, PANEL_DIAG_EVENT_DEFICIT, actual, expected);
    }
    if (restart) {
        m->restart_decisions++;
        m->last_reason = reason;
    }
}
static inline __attribute__((always_inline)) void panel_diag_restarted(panel_diag_state_t *s, uint64_t us)
{
    s->metrics.restart_paths++;
    s->metrics.last_restart_us = us;
    panel_diag_event(s, us, PANEL_DIAG_EVENT_RESTART, s->metrics.last_reason, 0U);
}
