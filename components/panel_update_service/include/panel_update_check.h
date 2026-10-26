/* Patch116I: serialized, read-only check scheduler and result fencing.
 * NO network/flash implementation. Generation fencing is NOT callback lifetime ownership.
 */
#pragma once

#include "panel_update_manifest.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_UPDATE_START_DELAY_MS UINT64_C(90000)
#define PANEL_UPDATE_JITTER_MAX_MS UINT64_C(600000)
#define PANEL_UPDATE_PERIOD_MS UINT64_C(86400000)
#define PANEL_UPDATE_RETRY_MS UINT64_C(900000)
#define PANEL_UPDATE_CHECK_DEADLINE_MS UINT64_C(30000)

typedef struct {
    uint64_t next_due_ms;
    uint64_t started_ms;
    uint32_t generation;
    bool in_flight;
    bool fresh;
    panel_update_status_t status;
    panel_update_manifest_t manifest;
} panel_update_check_t;

/* Call on a single owning task only; no FreeRTOS task created by this component. */
void panel_update_check_init(panel_update_check_t *s, uint64_t now_ms,
                             uint32_t spread_seed);

/* 'normal_boot_settled' must be true before discovery (not Homey READY). */
bool panel_update_check_start(panel_update_check_t *s, uint64_t now_ms,
                              bool wifi_ready, bool normal_boot_settled,
                              bool manual_check, uint32_t *request_generation);

/* Immediately time out a hung request from caller's perspective.
 * Does NOT cancel an underlying TLS client or release its callback memory.
 */
bool panel_update_check_tick(panel_update_check_t *s, uint64_t now_ms);

/* Caller must attest TLS and exact pinned manifest origin after a bounded read.
 * Never treats the result as authorization to install.
 */
bool panel_update_check_complete(panel_update_check_t *s, uint32_t generation,
                                 uint64_t now_ms, bool source_verified,
                                 const char *body, size_t body_length,
                                 const panel_update_local_t *local);
