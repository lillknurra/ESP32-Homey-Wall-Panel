#include "panel_clock.h"

#include <stdlib.h>

bool panel_clock_configure_timezone(void)
{
    if (setenv("TZ", PANEL_CLOCK_TZ_RULE, 1) != 0) {
        return false;
    }
    tzset();
    return true;
}

bool panel_clock_epoch_is_valid(time_t timestamp)
{
    return timestamp >= PANEL_CLOCK_MIN_VALID_UNIX_SECONDS;
}

bool panel_clock_render_is_due(
    const panel_clock_render_state_t *state,
    time_t timestamp,
    bool synchronized)
{
    if (state == NULL) {
        return false;
    }
    if (!state->initialized || state->synchronized != synchronized) {
        return true;
    }
    return synchronized && (timestamp / 60) != state->rendered_minute;
}

void panel_clock_render_committed(
    panel_clock_render_state_t *state,
    time_t timestamp,
    bool synchronized)
{
    if (state == NULL) {
        return;
    }
    state->initialized = true;
    state->synchronized = synchronized;
    state->rendered_minute = timestamp / 60;
}
