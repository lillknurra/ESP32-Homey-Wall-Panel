/* Patch116I1: informational manifest retrieval ONLY. NO firmware/OTA writer. */
#pragma once
#include "panel_update_manifest.h"
#include "panel_update_check.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PANEL_UPDATE_FETCH_DISABLED = 0,
    PANEL_UPDATE_FETCH_OK,
    PANEL_UPDATE_FETCH_REJECTED,
    PANEL_UPDATE_FETCH_NETWORK_ERROR,
    PANEL_UPDATE_FETCH_TIMEOUT,
    PANEL_UPDATE_FETCH_INCOMPLETE,
} panel_update_fetch_result_t;

/* Fixed HTTPS endpoints only; source is configured in firmware, never in a manifest.
 * The caller owns this call for its full lifetime; DO NOT force-delete its task.
 * esp_http_client can block in DNS/TLS/header calls beyond the logical deadline.
 * Therefore the real network feature is disabled by default until bounded
 * cancellation/ownership and runtime resource limits are verified.
 */
panel_update_fetch_result_t panel_update_https_fetch(
    const char *channel,
    char *body,
    size_t body_capacity,
    size_t *body_length);

/* Manual, read-only bridge. Does NOT schedule a task or make OTA install possible.
 * The 'generation' must have been returned by panel_update_check_start().
 * All network and read operations finish before completion is published.
 */
panel_update_fetch_result_t panel_update_https_fetch_and_complete(
    panel_update_check_t *check,
    uint32_t generation,
    const panel_update_local_t *local);
