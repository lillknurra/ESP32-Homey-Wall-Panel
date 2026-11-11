#ifndef PANEL_WIFI_DEFERRED_H
#define PANEL_WIFI_DEFERRED_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t queued_generation;
    uint32_t running_generation;
    bool queued;
    bool running;
} panel_wifi_deferred_owner_t;

void panel_wifi_deferred_owner_init(panel_wifi_deferred_owner_t *owner);
bool panel_wifi_deferred_schedule(panel_wifi_deferred_owner_t *owner, uint32_t generation);
void panel_wifi_deferred_invalidate(panel_wifi_deferred_owner_t *owner);
bool panel_wifi_deferred_begin(
    panel_wifi_deferred_owner_t *owner,
    uint32_t request_generation,
    uint32_t current_generation,
    bool wifi_online);
bool panel_wifi_deferred_still_current(
    const panel_wifi_deferred_owner_t *owner,
    uint32_t request_generation,
    uint32_t current_generation,
    bool wifi_online);
void panel_wifi_deferred_finish(panel_wifi_deferred_owner_t *owner);

#endif
