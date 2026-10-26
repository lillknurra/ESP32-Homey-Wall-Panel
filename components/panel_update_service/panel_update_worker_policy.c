#include "panel_update_worker_policy.h"
#include <string.h>

static bool inside_deadline(const panel_update_worker_policy_t *s, uint64_t now_ms)
{
    return now_ms >= s->started_ms &&
           now_ms - s->started_ms < PANEL_UPDATE_WORKER_DEADLINE_MS;
}

void panel_update_worker_policy_init(panel_update_worker_policy_t *s)
{
    if (s) memset(s, 0, sizeof(*s));
}

bool panel_update_worker_policy_queue(panel_update_worker_policy_t *s,
    uint32_t generation, uint64_t now_ms,
    bool wifi_ready, bool boot_settled, bool internal_heap_adequate)
{
    if (!s || s->phase != PANEL_UPDATE_WORKER_IDLE || s->worker_owns_transport ||
        !wifi_ready || !boot_settled || !internal_heap_adequate ||
        generation == 0U || generation <= s->last_generation || now_ms == UINT64_MAX) {
        return false;
    }
    s->generation = generation;
    s->last_generation = generation;
    s->started_ms = now_ms;
    s->result_ok = false;
    s->phase = PANEL_UPDATE_WORKER_QUEUED;
    return true;
}

bool panel_update_worker_policy_worker_begin(panel_update_worker_policy_t *s, uint32_t generation)
{
    if (!s || s->generation != generation || generation == 0U) return false;
    if (s->phase == PANEL_UPDATE_WORKER_QUARANTINED && !s->worker_owns_transport) {
        /* The queued task never touched the network: dequeue and retire tombstone. */
        s->phase = PANEL_UPDATE_WORKER_IDLE;
        return false;
    }
    if (s->phase != PANEL_UPDATE_WORKER_QUEUED || s->worker_owns_transport) return false;
    s->worker_owns_transport = true;
    s->phase = PANEL_UPDATE_WORKER_RUNNING;
    return true;
}

bool panel_update_worker_policy_tick(panel_update_worker_policy_t *s, uint64_t now_ms)
{
    if (!s) return false;
    const bool active = s->phase == PANEL_UPDATE_WORKER_QUEUED ||
                        s->phase == PANEL_UPDATE_WORKER_RUNNING ||
                        s->phase == PANEL_UPDATE_WORKER_FINISHED_OK ||
                        s->phase == PANEL_UPDATE_WORKER_FINISHED_ERROR;
    if (!active || inside_deadline(s, now_ms)) return false;
    s->result_ok = false;
    /* Even a queued notification cannot be recalled; worker must drain it. */
    if (s->phase == PANEL_UPDATE_WORKER_FINISHED_OK ||
        s->phase == PANEL_UPDATE_WORKER_FINISHED_ERROR) {
        s->phase = PANEL_UPDATE_WORKER_IDLE;
    } else {
        s->phase = PANEL_UPDATE_WORKER_QUARANTINED;
    }
    return true;
}

bool panel_update_worker_policy_worker_finish(panel_update_worker_policy_t *s,
    uint32_t generation, uint64_t now_ms, bool fetch_success)
{
    if (!s || generation == 0U || s->generation != generation ||
        !s->worker_owns_transport) return false;
    s->worker_owns_transport = false;
    if (s->phase != PANEL_UPDATE_WORKER_RUNNING || !inside_deadline(s, now_ms)) {
        s->phase = PANEL_UPDATE_WORKER_IDLE;
        s->result_ok = false;
        return false; /* stale result is intentionally discarded */
    }
    s->result_ok = fetch_success;
    s->phase = fetch_success ? PANEL_UPDATE_WORKER_FINISHED_OK :
                                 PANEL_UPDATE_WORKER_FINISHED_ERROR;
    return true;
}

bool panel_update_worker_policy_take_begin(panel_update_worker_policy_t *s,
    uint32_t generation, uint64_t now_ms, bool *fetch_success)
{
    if (!s || !fetch_success || generation == 0U || s->generation != generation ||
        s->worker_owns_transport ||
        (s->phase != PANEL_UPDATE_WORKER_FINISHED_OK &&
         s->phase != PANEL_UPDATE_WORKER_FINISHED_ERROR)) return false;
    if (!inside_deadline(s, now_ms)) {
        s->phase = PANEL_UPDATE_WORKER_IDLE;
        s->result_ok = false;
        return false;
    }
    *fetch_success = s->result_ok;
    s->phase = PANEL_UPDATE_WORKER_READING;
    return true;
}

bool panel_update_worker_policy_take_end(panel_update_worker_policy_t *s,
    uint32_t generation)
{
    if (!s || generation == 0U || s->generation != generation ||
        s->phase != PANEL_UPDATE_WORKER_READING || s->worker_owns_transport) return false;
    s->result_ok = false;
    s->phase = PANEL_UPDATE_WORKER_IDLE;
    return true;
}
