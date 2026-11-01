#include "panel_wifi_deferred.h"

#include <stddef.h>

void panel_wifi_deferred_owner_init(panel_wifi_deferred_owner_t *owner)
{
    if (owner == NULL) return;
    owner->queued_generation = 0U;
    owner->running_generation = 0U;
    owner->queued = false;
    owner->running = false;
}

bool panel_wifi_deferred_schedule(panel_wifi_deferred_owner_t *owner, uint32_t generation)
{
    if (owner == NULL || generation == 0U || owner->queued || owner->running) return false;
    owner->queued_generation = generation;
    owner->queued = true;
    return true;
}

void panel_wifi_deferred_invalidate(panel_wifi_deferred_owner_t *owner)
{
    if (owner == NULL) return;
    owner->queued = false;
    owner->queued_generation = 0U;
}

bool panel_wifi_deferred_begin(
    panel_wifi_deferred_owner_t *owner,
    uint32_t request_generation,
    uint32_t current_generation,
    bool wifi_online)
{
    if (owner == NULL || !owner->queued || owner->queued_generation != request_generation) {
        return false;
    }
    if (request_generation == 0U || request_generation != current_generation || wifi_online) {
        panel_wifi_deferred_invalidate(owner);
        return false;
    }
    owner->queued = false;
    owner->queued_generation = 0U;
    owner->running = true;
    owner->running_generation = request_generation;
    return true;
}

bool panel_wifi_deferred_still_current(
    const panel_wifi_deferred_owner_t *owner,
    uint32_t request_generation,
    uint32_t current_generation,
    bool wifi_online)
{
    return owner != NULL && owner->running &&
        owner->running_generation == request_generation &&
        request_generation != 0U && request_generation == current_generation &&
        !wifi_online;
}

void panel_wifi_deferred_finish(panel_wifi_deferred_owner_t *owner)
{
    if (owner == NULL) return;
    owner->running = false;
    owner->running_generation = 0U;
}
