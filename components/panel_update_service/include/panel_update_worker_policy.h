/* Patch116I2: one-owner, read-only update discovery quarantine policy.
 * Pure C, caller serializes transitions. NEVER an OTA authorization.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define PANEL_UPDATE_WORKER_DEADLINE_MS UINT64_C(30000)

typedef enum {
    PANEL_UPDATE_WORKER_IDLE = 0,
    PANEL_UPDATE_WORKER_QUEUED,
    PANEL_UPDATE_WORKER_RUNNING,
    PANEL_UPDATE_WORKER_QUARANTINED,
    PANEL_UPDATE_WORKER_FINISHED_OK,
    PANEL_UPDATE_WORKER_FINISHED_ERROR,
    PANEL_UPDATE_WORKER_READING,
} panel_update_worker_phase_t;

typedef struct {
    panel_update_worker_phase_t phase;
    uint32_t generation;
    uint32_t last_generation;
    uint64_t started_ms;
    bool worker_owns_transport;
    bool result_ok;
} panel_update_worker_policy_t;

void panel_update_worker_policy_init(panel_update_worker_policy_t *s);
/* Preconditions are attested by owner and must not be derived from OTA metadata. */
bool panel_update_worker_policy_queue(panel_update_worker_policy_t *s,
    uint32_t generation, uint64_t now_ms,
    bool wifi_ready, bool boot_settled, bool internal_heap_adequate);
/* Must be invoked by the actual worker after dequeueing the generation. */
bool panel_update_worker_policy_worker_begin(panel_update_worker_policy_t *s, uint32_t generation);
/* Mark caller-visible timeout; does not cancel task or free any network resources. */
bool panel_update_worker_policy_tick(panel_update_worker_policy_t *s, uint64_t now_ms);
/* Called ONLY after fetch returned and fully closed/cleaned its transport. */
bool panel_update_worker_policy_worker_finish(panel_update_worker_policy_t *s,
    uint32_t generation, uint64_t now_ms, bool fetch_success);
/* Owner claims a completed immutable buffer, copies it outside the lock. */
bool panel_update_worker_policy_take_begin(panel_update_worker_policy_t *s,
    uint32_t generation, uint64_t now_ms, bool *fetch_success);
bool panel_update_worker_policy_take_end(panel_update_worker_policy_t *s,
    uint32_t generation);
/* Neither this policy nor its adapter grants a reboot, write or install. */
