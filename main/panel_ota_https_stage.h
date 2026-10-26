#pragma once

#include <stdbool.h>

/* Patch116E is dormant unless the deliberately reviewed build enables it.
 * No web handler, caller, authentication system, RTC handoff or reboot is
 * supplied by this module. Authorization callbacks are INTEGRATION CONTRACTS,
 * not a replacement for a future authenticated administrator service.
 */
typedef bool (*panel_ota_access_check_fn)(void *context);
typedef struct {
    void *context;
    panel_ota_access_check_fn authenticated_operator_confirmed;
    panel_ota_access_check_fn pre_display_maintenance_quiescent;
} panel_ota_stage_access_t;

typedef enum {
    PANEL_OTA_STAGE_DISABLED = 0,
    PANEL_OTA_STAGE_DENIED,
    PANEL_OTA_STAGE_BAD_CONFIGURATION,
    PANEL_OTA_STAGE_TRANSPORT_ERROR,
    PANEL_OTA_STAGE_RELEASE_REJECTED,
    PANEL_OTA_STAGE_INCOMPLETE,
    PANEL_OTA_STAGE_FINALIZE_FAILED,
    PANEL_OTA_STAGE_STAGED_NO_REBOOT,
} panel_ota_stage_result_t;

/* ONLY STAGES a verified signed image to the inactive OTA slot. When compiled
 * enabled it can erase/write an OTA slot and change otadata on success.
 * It NEVER restarts, updates the factory image, operates Homey, or controls
 * awnings. Caller must run in a verified no-RGB maintenance environment.
 */
panel_ota_stage_result_t panel_ota_https_stage(const panel_ota_stage_access_t *access);
