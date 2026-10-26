/* Patch116I7 - host-test-only Homey-first network admission model.
 * Intentionally omitted from the ESP-IDF component CMake source list.
 */
#include "panel_update_i7_admission_policy.h"
#include <string.h>

#if defined(ESP_PLATFORM) && (!defined(PANEL_UPDATE_I7_AUDIT_ONLY) || PANEL_UPDATE_I7_AUDIT_ONLY != 1)
#error "Patch116I7 is offline/audit-only; native runtime integration not approved"
#endif

static bool ota_phase(panel_update_i7_phase_t phase)
{
    return phase == PANEL_UPDATE_I7_OTA_RESERVED ||
           phase == PANEL_UPDATE_I7_OTA_ACTIVE ||
           phase == PANEL_UPDATE_I7_OTA_CANCEL_PENDING;
}

static bool eligible(const panel_update_i7_admission_t *e)
{
    return e != NULL && e->trial_separately_authorized &&
        e->wifi_stable && e->normal_boot_settled && e->homey_ready &&
        e->homey_network_phase_idle && e->homey_command_queue_empty &&
        e->homey_tls_closed && e->cloud_tls_closed &&
        e->other_outbound_tls_quiescent &&
        e->internal_free_bytes >= PANEL_UPDATE_I7_MIN_INTERNAL_FREE_BYTES &&
        e->internal_largest_bytes >= PANEL_UPDATE_I7_MIN_INTERNAL_LARGEST_BYTES &&
        e->owner_stack_margin_bytes >= PANEL_UPDATE_I7_MIN_OWNER_STACK_MARGIN_BYTES;
}

void panel_update_i7_init(panel_update_i7_arbiter_t *a)
{
    if (a != NULL) memset(a, 0, sizeof(*a));
}

bool panel_update_i7_homey_try_acquire(panel_update_i7_arbiter_t *a)
{
    if (a == NULL) return false;
    if (a->phase == PANEL_UPDATE_I7_IDLE) {
        a->phase = PANEL_UPDATE_I7_HOMEY_OWNED;
        a->homey_pending = false;
        return true;
    }
    if (ota_phase(a->phase)) {
        a->homey_pending = true;
        a->phase = PANEL_UPDATE_I7_OTA_CANCEL_PENDING;
    }
    return false;
}

bool panel_update_i7_homey_release(panel_update_i7_arbiter_t *a)
{
    if (a == NULL || a->phase != PANEL_UPDATE_I7_HOMEY_OWNED) return false;
    a->phase = PANEL_UPDATE_I7_IDLE;
    return true;
}

bool panel_update_i7_homey_withdraw_pending(panel_update_i7_arbiter_t *a)
{
    if (a == NULL || !a->homey_pending || a->phase == PANEL_UPDATE_I7_HOMEY_OWNED)
        return false;
    a->homey_pending = false;
    /* A cancelled OTA stays cancelled; never resurrect it. */
    return true;
}

bool panel_update_i7_ota_try_reserve(panel_update_i7_arbiter_t *a,
                                     uint32_t generation,
                                     const panel_update_i7_admission_t *e)
{
    if (a == NULL || a->phase != PANEL_UPDATE_I7_IDLE || a->homey_pending ||
        generation == 0U || generation <= a->last_generation || !eligible(e))
        return false;
    a->last_generation = generation;
    a->generation = generation;
    a->ota_transport_outstanding = false;
    a->phase = PANEL_UPDATE_I7_OTA_RESERVED;
    return true;
}

bool panel_update_i7_ota_begin(panel_update_i7_arbiter_t *a,
                                uint32_t generation,
                                const panel_update_i7_admission_t *e)
{
    if (a == NULL || generation == 0U || generation != a->generation ||
        a->phase != PANEL_UPDATE_I7_OTA_RESERVED) return false;
    if (a->homey_pending || !eligible(e)) {
        a->phase = PANEL_UPDATE_I7_OTA_CANCEL_PENDING;
        return false;
    }
    a->phase = PANEL_UPDATE_I7_OTA_ACTIVE;
    return true;
}

bool panel_update_i7_ota_checkpoint(panel_update_i7_arbiter_t *a,
                                     uint32_t generation,
                                     const panel_update_i7_admission_t *e)
{
    if (a == NULL || generation == 0U || generation != a->generation ||
        !ota_phase(a->phase)) return false;
    if (a->phase != PANEL_UPDATE_I7_OTA_ACTIVE || a->homey_pending || !eligible(e)) {
        a->phase = PANEL_UPDATE_I7_OTA_CANCEL_PENDING;
        return false;
    }
    return true;
}

bool panel_update_i7_ota_claim_transport(panel_update_i7_arbiter_t *a,
                                          uint32_t generation)
{
    if (a == NULL || a->phase != PANEL_UPDATE_I7_OTA_ACTIVE ||
        generation == 0U || generation != a->generation ||
        a->homey_pending || a->ota_transport_outstanding) return false;
    a->ota_transport_outstanding = true;
    return true;
}

bool panel_update_i7_ota_ack_cleanup(panel_update_i7_arbiter_t *a,
                                      uint32_t generation,
                                      bool cleanup_verified)
{
    if (a == NULL || !ota_phase(a->phase) ||
        generation == 0U || generation != a->generation ||
        !a->ota_transport_outstanding || !cleanup_verified) return false;
    a->ota_transport_outstanding = false;
    return true;
}

bool panel_update_i7_ota_request_cancel(panel_update_i7_arbiter_t *a,
                                         uint32_t generation)
{
    if (a == NULL || generation == 0U || generation != a->generation ||
        !ota_phase(a->phase)) return false;
    a->phase = PANEL_UPDATE_I7_OTA_CANCEL_PENDING;
    return true;
}

bool panel_update_i7_ota_release(panel_update_i7_arbiter_t *a,
                                  uint32_t generation)
{
    if (a == NULL || generation == 0U || generation != a->generation ||
        !ota_phase(a->phase) || a->ota_transport_outstanding) return false;
    a->phase = PANEL_UPDATE_I7_IDLE;
    a->generation = 0U;
    /* Preserve homey_pending until Homey itself acquires or withdraws. */
    return true;
}
