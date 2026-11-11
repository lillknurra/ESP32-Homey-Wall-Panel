#include <assert.h>
#include "panel_homey_boot_liveness.h"

int main(void)
{
    panel_homey_boot_liveness_t state = {0};
    assert(panel_homey_boot_liveness_tick(&state, false, true, 0) ==
           PANEL_HOMEY_LIVENESS_IDLE);
    assert(panel_homey_boot_liveness_tick(&state, true, false, 10000) ==
           PANEL_HOMEY_LIVENESS_IDLE);
    assert(panel_homey_boot_liveness_tick(&state, true, true, 10000) ==
           PANEL_HOMEY_LIVENESS_ATTEMPT);
    assert(panel_homey_boot_liveness_tick(&state, true, true, 14999) ==
           PANEL_HOMEY_LIVENESS_WAIT);
    assert(panel_homey_boot_liveness_tick(&state, true, true, 15000) ==
           PANEL_HOMEY_LIVENESS_ATTEMPT);
    assert(panel_homey_boot_liveness_tick(&state, true, true, 20000) ==
           PANEL_HOMEY_LIVENESS_ATTEMPT);
    assert(state.attempts == PANEL_HOMEY_BOOT_LIVENESS_MAX_ATTEMPTS);
    assert(panel_homey_boot_liveness_tick(&state, true, true, 25000) ==
           PANEL_HOMEY_LIVENESS_EXHAUSTED);
    assert(panel_homey_boot_liveness_tick(&state, true, true, 30000) ==
           PANEL_HOMEY_LIVENESS_IDLE);
    return 0;
}
