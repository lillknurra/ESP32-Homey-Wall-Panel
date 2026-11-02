#include "panel_homey_boot_recovery.h"

#include <stddef.h>

bool panel_homey_boot_recovery_try_begin(
    panel_homey_boot_recovery_state_t *state,
    const panel_homey_boot_recovery_inputs_t *inputs)
{
    if (state == NULL || inputs == NULL || state->scheduler_running ||
        inputs->scheduler_running || inputs->inventory_reserved ||
        !inputs->auth_restore_complete || !inputs->wifi_online ||
        !inputs->selected_homey_present || !inputs->session_valid ||
        inputs->verified_fresh_snapshot) {
        return false;
    }
    state->scheduler_running = true;
    return true;
}

void panel_homey_boot_recovery_finish(panel_homey_boot_recovery_state_t *state)
{
    if (state != NULL) state->scheduler_running = false;
}

panel_homey_boot_timeout_action_t panel_homey_boot_recovery_timeout_action(
    bool verified_fresh_snapshot, bool inventory_reserved)
{
    return verified_fresh_snapshot || inventory_reserved
        ? PANEL_HOMEY_BOOT_TIMEOUT_KEEP_STATE
        : PANEL_HOMEY_BOOT_TIMEOUT_MARK_ERROR;
}
