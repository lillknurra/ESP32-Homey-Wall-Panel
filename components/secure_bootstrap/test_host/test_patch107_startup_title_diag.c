#include "panel_startup_title_diag.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    assert(panel_startup_title_diag_classify(
        "Ansluter till Wi-Fi...", "Kontrollerar sparad konfiguration") ==
        PANEL_STARTUP_TITLE_DIAG_WIFI_CONNECTING);
    assert(panel_startup_title_diag_classify(
        "Ansluter till Wi-Fi...", "Återansluter till sparat nätverk") ==
        PANEL_STARTUP_TITLE_DIAG_WIFI_DISCONNECTED);
    assert(panel_startup_title_diag_classify("Homey READY", "") ==
        PANEL_STARTUP_TITLE_DIAG_HOMEY_READY);
    assert(panel_startup_title_diag_classify("private arbitrary text", "secret") ==
        PANEL_STARTUP_TITLE_DIAG_UNKNOWN);
    assert(strcmp(panel_startup_title_diag_id_name(PANEL_STARTUP_TITLE_DIAG_UNKNOWN),
        "unknown") == 0);

    panel_startup_title_diag_log_gate_t gate = {0};
    assert(panel_startup_title_diag_log_transition(&gate,
        PANEL_STARTUP_TITLE_DIAG_WIFI_CONNECTED));
    assert(!panel_startup_title_diag_log_transition(&gate,
        PANEL_STARTUP_TITLE_DIAG_WIFI_CONNECTED));
    assert(panel_startup_title_diag_log_transition(&gate,
        PANEL_STARTUP_TITLE_DIAG_HOMEY_LOADING));
    for (unsigned i = gate.transition_count; i < PANEL_STARTUP_TITLE_DIAG_MAX_TRANSITIONS; ++i) {
        assert(panel_startup_title_diag_log_transition(&gate,
            (panel_startup_title_diag_id_t)(i + 1U)));
    }
    assert(gate.transition_count == PANEL_STARTUP_TITLE_DIAG_MAX_TRANSITIONS);
    assert(!panel_startup_title_diag_log_transition(&gate,
        PANEL_STARTUP_TITLE_DIAG_HOMEY_ERROR));

    const panel_startup_title_diag_rect_t title = {.x = -10, .y = 470, .width = 40, .height = 30};
    panel_startup_title_diag_rect_t visible = {0};
    assert(panel_startup_title_diag_clip_rect(&title, 480, 480, &visible));
    assert(visible.x == 0 && visible.y == 470 && visible.width == 30 && visible.height == 10);
    assert(panel_startup_title_diag_intersects_flush(&visible, 20, 470, 25, 475));
    assert(!panel_startup_title_diag_intersects_flush(&visible, 31, 470, 40, 479));
    assert(!panel_startup_title_diag_clip_rect(&title, 5, 5, &visible));
    return 0;
}
