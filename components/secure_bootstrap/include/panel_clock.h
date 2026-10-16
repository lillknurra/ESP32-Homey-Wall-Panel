#ifndef PANEL_CLOCK_H
#define PANEL_CLOCK_H

#include <stdbool.h>
#include <time.h>

#define PANEL_CLOCK_TZ_RULE "CET-1CEST,M3.5.0/2,M10.5.0/3"
#define PANEL_CLOCK_MIN_VALID_UNIX_SECONDS ((time_t)1704067200)

typedef struct {
    bool initialized;
    bool synchronized;
    time_t rendered_minute;
} panel_clock_render_state_t;

bool panel_clock_configure_timezone(void);
bool panel_clock_epoch_is_valid(time_t timestamp);
bool panel_clock_render_is_due(
    const panel_clock_render_state_t *state,
    time_t timestamp,
    bool synchronized);
void panel_clock_render_committed(
    panel_clock_render_state_t *state,
    time_t timestamp,
    bool synchronized);

#endif
