#include "panel_update_i3_probe.h"
#include <assert.h>
int main(void) {
    panel_update_i3_owner_input_t s = {.monotonic_ms = 999999,.wifi_online=true,.ui_allocated=true,.homey_ready=true};
    panel_update_i3_owner_tick(&s);
    return 0;
}
