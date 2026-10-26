#include "panel_update_i3a_web_diag.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

bool panel_update_i3a_append_json(
    char *json, size_t capacity, size_t *length, uint64_t now_ms,
    bool have_sample, const panel_update_i3a_snapshot_t *sample)
{
    if (!json || !length || capacity < 3U || *length < 2U ||
        *length >= capacity || json[*length] != '\0' ||
        json[*length - 1U] != '}' || json[0] != '{') return false;

    char fragment[PANEL_UPDATE_I3A_MAX_FRAGMENT_BYTES];
    int written;
    const char *prefix = *length == 2U ? "" : ",";
    if (!have_sample || !sample || sample->monotonic_ms == UINT64_MAX ||
        now_ms == UINT64_MAX || now_ms < sample->monotonic_ms) {
        written = snprintf(fragment, sizeof(fragment),
                           "%s\"resource_probe\":{\"valid\":false}}", prefix);
    } else {
        const uint64_t age_ms = now_ms - sample->monotonic_ms;
        const bool stale = age_ms > PANEL_UPDATE_I3A_SAMPLE_STALE_MS;
        written = snprintf(fragment, sizeof(fragment),
            "%s\"resource_probe\":{\"valid\":true,\"stale\":%s,"
            "\"monotonic_ms\":%" PRIu64 ",\"age_ms\":%" PRIu64 ","
            "\"internal_free_bytes\":%" PRIu32 ","
            "\"internal_largest_block_bytes\":%" PRIu32 ","
            "\"historical_min_internal_free_bytes\":%" PRIu32 ","
            "\"rotation_stack_margin_bytes\":%" PRIu32 ","
            "\"wifi_online\":%s,\"ui_allocated\":%s,\"homey_ready\":%s,"
            "\"reset_reason\":%" PRIu32 "}}",
            prefix, stale ? "true" : "false", sample->monotonic_ms, age_ms,
            sample->internal_free_bytes, sample->internal_largest_block_bytes,
            sample->internal_minimum_free_bytes,
            sample->rotation_stack_margin_bytes,
            sample->wifi_online ? "true" : "false",
            sample->ui_allocated ? "true" : "false",
            sample->homey_ready ? "true" : "false",
            sample->reset_reason);
    }
    if (written <= 0 || (size_t)written >= sizeof(fragment) ||
        (size_t)written > capacity - *length) return false;

    /* Existing trailing brace is replaced by the fragment's comma, plus
     * a new final brace. Do not mutate the response until bounds are proven. */
    memcpy(json + *length - 1U, fragment, (size_t)written + 1U);
    *length = *length - 1U + (size_t)written;
    return true;
}
