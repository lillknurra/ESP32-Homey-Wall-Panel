#include "panel_startup_title_diag.h"

#include <stddef.h>
#include <string.h>

panel_startup_title_diag_id_t panel_startup_title_diag_classify(
    const char *title,
    const char *detail)
{
    if (title == NULL) return PANEL_STARTUP_TITLE_DIAG_UNKNOWN;

    if (strcmp(title, "Ansluter till Wi-Fi...") == 0) {
        if (detail != NULL && strcmp(detail, "Kontrollerar sparad konfiguration") == 0) {
            return PANEL_STARTUP_TITLE_DIAG_WIFI_CONNECTING;
        }
        if (detail != NULL && strcmp(detail, "Återansluter till sparat nätverk") == 0) {
            return PANEL_STARTUP_TITLE_DIAG_WIFI_DISCONNECTED;
        }
    } else if (strcmp(title, "Wi-Fi ansluten") == 0 && (detail == NULL || detail[0] == '\0')) {
        return PANEL_STARTUP_TITLE_DIAG_WIFI_CONNECTED;
    } else if (strcmp(title, "Hämtar Homey-data...") == 0) {
        return PANEL_STARTUP_TITLE_DIAG_HOMEY_LOADING;
    } else if (strcmp(title, "Hämtningen tar längre tid än väntat") == 0) {
        return PANEL_STARTUP_TITLE_DIAG_HOMEY_LONG_WAIT;
    } else if (strcmp(title, "Homey READY") == 0 && (detail == NULL || detail[0] == '\0')) {
        return PANEL_STARTUP_TITLE_DIAG_HOMEY_READY;
    } else if (strcmp(title, "Homey-data kunde inte hämtas") == 0) {
        return PANEL_STARTUP_TITLE_DIAG_HOMEY_ERROR;
    }

    return PANEL_STARTUP_TITLE_DIAG_UNKNOWN;
}

const char *panel_startup_title_diag_id_name(panel_startup_title_diag_id_t id)
{
    switch (id) {
    case PANEL_STARTUP_TITLE_DIAG_WIFI_CONNECTING: return "wifi_connecting";
    case PANEL_STARTUP_TITLE_DIAG_WIFI_DISCONNECTED: return "wifi_disconnected";
    case PANEL_STARTUP_TITLE_DIAG_WIFI_CONNECTED: return "wifi_connected";
    case PANEL_STARTUP_TITLE_DIAG_HOMEY_LOADING: return "homey_loading";
    case PANEL_STARTUP_TITLE_DIAG_HOMEY_LONG_WAIT: return "homey_long_wait";
    case PANEL_STARTUP_TITLE_DIAG_HOMEY_READY: return "homey_ready";
    case PANEL_STARTUP_TITLE_DIAG_HOMEY_ERROR: return "homey_error";
    case PANEL_STARTUP_TITLE_DIAG_UNKNOWN:
    default: return "unknown";
    }
}

bool panel_startup_title_diag_log_transition(
    panel_startup_title_diag_log_gate_t *gate,
    panel_startup_title_diag_id_t id)
{
    if (gate == NULL || gate->transition_count >= PANEL_STARTUP_TITLE_DIAG_MAX_TRANSITIONS) {
        return false;
    }
    if (gate->has_last_id && gate->last_id == id) return false;
    gate->has_last_id = true;
    gate->last_id = id;
    gate->transition_count++;
    return true;
}

bool panel_startup_title_diag_clip_rect(
    const panel_startup_title_diag_rect_t *input,
    int32_t screen_width,
    int32_t screen_height,
    panel_startup_title_diag_rect_t *output)
{
    if (input == NULL || output == NULL || input->width <= 0 || input->height <= 0 ||
        screen_width <= 0 || screen_height <= 0) return false;

    const int32_t x1 = input->x < 0 ? 0 : input->x;
    const int32_t y1 = input->y < 0 ? 0 : input->y;
    const int32_t input_x2 = input->x + input->width - 1;
    const int32_t input_y2 = input->y + input->height - 1;
    const int32_t x2 = input_x2 >= screen_width ? screen_width - 1 : input_x2;
    const int32_t y2 = input_y2 >= screen_height ? screen_height - 1 : input_y2;
    if (x1 > x2 || y1 > y2) return false;

    output->x = x1;
    output->y = y1;
    output->width = x2 - x1 + 1;
    output->height = y2 - y1 + 1;
    return true;
}

bool panel_startup_title_diag_intersects_flush(
    const panel_startup_title_diag_rect_t *rect,
    int32_t x1,
    int32_t y1,
    int32_t x2,
    int32_t y2)
{
    if (rect == NULL || rect->width <= 0 || rect->height <= 0 || x1 > x2 || y1 > y2) {
        return false;
    }
    const int32_t rect_x2 = rect->x + rect->width - 1;
    const int32_t rect_y2 = rect->y + rect->height - 1;
    return rect->x <= x2 && rect_x2 >= x1 && rect->y <= y2 && rect_y2 >= y1;
}
