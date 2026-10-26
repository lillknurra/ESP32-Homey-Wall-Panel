/* Patch116I3A: passive cached heap/stack diagnostics, no network or OTA APIs.
 * Only the pre-existing rotation task may publish. HTTP may only read.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_UPDATE_I3A_SAMPLE_STALE_MS UINT64_C(180000)
#define PANEL_UPDATE_I3A_MAX_FRAGMENT_BYTES 512U

typedef struct {
    uint64_t monotonic_ms;
    uint32_t internal_free_bytes;
    uint32_t internal_largest_block_bytes;
    uint32_t internal_minimum_free_bytes;
    uint32_t rotation_stack_margin_bytes;
    uint32_t reset_reason; /* Raw esp_reset_reason_t, diagnostic only. */
    bool wifi_online;
    bool ui_allocated;
    bool homey_ready;
} panel_update_i3a_snapshot_t;

/* Copies < 64 bytes under a short FreeRTOS critical section when active.
 * False means no sample has been taken. Caller owns the output thereafter. */
bool panel_update_i3a_snapshot_copy(panel_update_i3a_snapshot_t *out);

/* Pure C; appends to a previously valid, NUL-terminated root JSON object.
 * No heap, locks, network or Homey calls. On failure, *all* output bytes and
 * length remain unchanged. Caller may safely serve the original response.
 * It is exclusively for the existing diagnostic-only endpoint. */
bool panel_update_i3a_append_json(
    char *json, size_t capacity, size_t *length, uint64_t now_ms,
    bool have_sample, const panel_update_i3a_snapshot_t *sample);
