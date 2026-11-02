#pragma once

#include <stdbool.h>

#define PANEL_HOMEY_BOOT_RECOVERY_WAIT_ATTEMPTS 120U
#define PANEL_HOMEY_BOOT_RECOVERY_WAIT_MS 1000U

typedef struct {
    bool auth_restore_complete;
    bool wifi_online;
    bool selected_homey_present;
    bool session_valid;
    bool verified_fresh_snapshot;
    bool scheduler_running;
    bool inventory_reserved;
} panel_homey_boot_recovery_inputs_t;

typedef struct {
    bool scheduler_running;
} panel_homey_boot_recovery_state_t;

typedef enum {
    PANEL_HOMEY_BOOT_TIMEOUT_KEEP_STATE = 0,
    PANEL_HOMEY_BOOT_TIMEOUT_MARK_ERROR,
} panel_homey_boot_timeout_action_t;

/* Single-owner gate: duplicate online events cannot start parallel schedulers.
 * A completed bounded attempt can be started again after a later online event. */
bool panel_homey_boot_recovery_try_begin(
    panel_homey_boot_recovery_state_t *state,
    const panel_homey_boot_recovery_inputs_t *inputs);

void panel_homey_boot_recovery_finish(panel_homey_boot_recovery_state_t *state);

panel_homey_boot_timeout_action_t panel_homey_boot_recovery_timeout_action(
    bool verified_fresh_snapshot, bool inventory_reserved);
