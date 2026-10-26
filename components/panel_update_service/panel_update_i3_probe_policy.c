#include "panel_update_i3_probe_policy.h"
#include <string.h>
#include <limits.h>

void panel_update_i3_policy_init(panel_update_i3_policy_t *p)
{
    if (p) memset(p, 0, sizeof(*p));
}

static bool good_margin(const panel_update_i3_sample_t *s)
{
    return s->wifi_online && s->ui_allocated && s->homey_ready &&
        s->internal_free_bytes >= PANEL_UPDATE_I3_MIN_INTERNAL_FREE &&
        s->internal_largest_bytes >= PANEL_UPDATE_I3_MIN_LARGEST_BLOCK &&
        s->owner_stack_margin_bytes >= PANEL_UPDATE_I3_MIN_OWNER_STACK_MARGIN;
}

panel_update_i3_decision_t panel_update_i3_policy_step(
    panel_update_i3_policy_t *p, const panel_update_i3_sample_t *s,
    bool one_shot_trial_compiled)
{
    panel_update_i3_decision_t d = {0};
    if (!p || !s || s->now_ms == UINT64_MAX || p->stopped) return d;
    if (!p->initialized) {
        p->initialized = true;
        p->initialized_ms = s->now_ms;
        p->previous_ms = s->now_ms;
        p->last_sample_ms = s->now_ms;
        p->lowest_internal_free = UINT32_MAX;
        p->lowest_largest_block = UINT32_MAX;
        p->lowest_owner_stack_margin = UINT32_MAX;
        return d; /* First sample after a full interval, never at startup. */
    }
    if (s->now_ms < p->previous_ms) {
        p->stopped = true; /* clock regression: permanent fail closed */
        return d;
    }
    p->previous_ms = s->now_ms;
    if (s->now_ms - p->last_sample_ms < PANEL_UPDATE_I3_SAMPLE_MS) return d;
    p->last_sample_ms = s->now_ms;
    if (p->count == UINT32_MAX) { p->stopped = true; return d; }
    ++p->count;
    if (s->internal_free_bytes < p->lowest_internal_free)
        p->lowest_internal_free = s->internal_free_bytes;
    if (s->internal_largest_bytes < p->lowest_largest_block)
        p->lowest_largest_block = s->internal_largest_bytes;
    if (s->owner_stack_margin_bytes < p->lowest_owner_stack_margin)
        p->lowest_owner_stack_margin = s->owner_stack_margin_bytes;
    d.sampled = true;
    d.sample_healthy = good_margin(s);
    if (d.sample_healthy) {
        if (p->consecutive_healthy < PANEL_UPDATE_I3_REQUIRED_HEALTHY_SAMPLES)
            ++p->consecutive_healthy;
    } else {
        p->consecutive_healthy = 0;
    }
    /* Never schedule a live trial unless it was separately compiled and its
     * first attempt is within this single boot. This model is not consent. */
    if (one_shot_trial_compiled && s->operator_trial_approved &&
        !p->trial_claimed && d.sample_healthy &&
        s->now_ms - p->initialized_ms >= PANEL_UPDATE_I3_BOOT_GRACE_MS &&
        p->consecutive_healthy >= PANEL_UPDATE_I3_REQUIRED_HEALTHY_SAMPLES) {
        p->trial_claimed = true;
        d.one_shot_trial_request = true;
    }
    return d;
}
