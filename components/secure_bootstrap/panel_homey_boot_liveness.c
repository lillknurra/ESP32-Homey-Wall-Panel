#include "panel_homey_boot_liveness.h"

panel_homey_liveness_action_t panel_homey_boot_liveness_tick(
    panel_homey_boot_liveness_t *state,
    bool initialization_failed,
    bool wifi_online,
    uint64_t now_ms)
{
    if (state == 0 || !initialization_failed || !wifi_online)
        return PANEL_HOMEY_LIVENESS_IDLE;
    if (state->attempts >= PANEL_HOMEY_BOOT_LIVENESS_MAX_ATTEMPTS) {
        if (state->exhausted_reported) return PANEL_HOMEY_LIVENESS_IDLE;
        state->exhausted_reported = true;
        return PANEL_HOMEY_LIVENESS_EXHAUSTED;
    }
    if (state->attempted && now_ms >= state->last_attempt_ms &&
        now_ms - state->last_attempt_ms < PANEL_HOMEY_BOOT_LIVENESS_INTERVAL_MS)
        return PANEL_HOMEY_LIVENESS_WAIT;

    state->attempted = true;
    state->last_attempt_ms = now_ms;
    state->attempts++;
    return PANEL_HOMEY_LIVENESS_ATTEMPT;
}
