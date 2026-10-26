/* Patch116I: no sockets, DNS, TLS, flash, storage, reboot, or Homey dependencies. */
#include "panel_update_check.h"

#include <limits.h>
#include <string.h>

static uint64_t add_ms(uint64_t base, uint64_t delta)
{
    return base > UINT64_MAX - delta ? UINT64_MAX : base + delta;
}

void panel_update_check_init(panel_update_check_t *s, uint64_t now_ms,
                             uint32_t spread_seed)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->status = PANEL_UPDATE_REJECTED;
    s->next_due_ms = add_ms(now_ms, PANEL_UPDATE_START_DELAY_MS +
                           (spread_seed % PANEL_UPDATE_JITTER_MAX_MS));
}

static void failed(panel_update_check_t *s, uint64_t now_ms)
{
    s->in_flight = false;
    s->fresh = false;
    s->status = PANEL_UPDATE_REJECTED;
    memset(&s->manifest, 0, sizeof(s->manifest));
    s->next_due_ms = add_ms(now_ms, PANEL_UPDATE_RETRY_MS);
}

bool panel_update_check_start(panel_update_check_t *s, uint64_t now_ms,
                              bool wifi_ready, bool normal_boot_settled,
                              bool manual_check, uint32_t *request_generation)
{
    if (!s || !request_generation || s->in_flight || !wifi_ready ||
        !normal_boot_settled || (!manual_check && now_ms < s->next_due_ms)) return false;
    s->generation++;
    if (s->generation == 0U) s->generation = 1U;
    s->in_flight = true;
    s->fresh = false;
    s->started_ms = now_ms;
    *request_generation = s->generation;
    return true;
}

bool panel_update_check_tick(panel_update_check_t *s, uint64_t now_ms)
{
    if (!s || !s->in_flight) return false;
    if (now_ms < s->started_ms || now_ms - s->started_ms >= PANEL_UPDATE_CHECK_DEADLINE_MS) {
        failed(s, now_ms);
        return true;
    }
    return false;
}

bool panel_update_check_complete(panel_update_check_t *s, uint32_t generation,
                                 uint64_t now_ms, bool source_verified,
                                 const char *body, size_t body_length,
                                 const panel_update_local_t *local)
{
    if (!s || !s->in_flight || generation == 0U || generation != s->generation) return false;
    if (now_ms < s->started_ms || now_ms - s->started_ms >= PANEL_UPDATE_CHECK_DEADLINE_MS ||
        !source_verified || !local || !body || body_length == 0U ||
        body_length > PANEL_UPDATE_MANIFEST_MAX_BYTES) {
        failed(s, now_ms);
        return true;
    }
    panel_update_manifest_t m;
    if (!panel_update_manifest_parse(body, body_length, &m)) {
        failed(s, now_ms);
        return true;
    }
    const panel_update_status_t status = panel_update_manifest_evaluate(&m, local);
    if (status == PANEL_UPDATE_REJECTED) {
        failed(s, now_ms);
        return true;
    }
    s->manifest = m;
    s->status = status;
    s->fresh = true;
    s->in_flight = false;
    s->next_due_ms = add_ms(now_ms, PANEL_UPDATE_PERIOD_MS);
    return true;
}
