#include "panel_startup_status.h"

#include <string.h>

void panel_startup_status_init(panel_startup_status_t *status)
{
    if (status != NULL) memset(status, 0, sizeof(*status));
}

panel_startup_action_t panel_startup_status_tick(
    panel_startup_status_t *status,
    bool wifi_online,
    panel_startup_homey_state_t homey_state,
    uint64_t now_ms)
{
    if (status == NULL) return PANEL_STARTUP_ACTION_NONE;

    /* Surface failures immediately, even if a success message is being held. */
    if (homey_state == PANEL_STARTUP_HOMEY_ERROR) {
        const bool newly_visible = status->last_action != PANEL_STARTUP_ACTION_HOMEY_ERROR;
        status->last_action = PANEL_STARTUP_ACTION_HOMEY_ERROR;
        status->homey_ready_shown = false;
        if (wifi_online && !status->wifi_status_shown) {
            status->wifi_status_shown = true;
            status->wifi_status_shown_at_ms = now_ms;
        }
        return newly_visible ? PANEL_STARTUP_ACTION_HOMEY_ERROR : PANEL_STARTUP_ACTION_NONE;
    }
    if (!wifi_online) return PANEL_STARTUP_ACTION_NONE;

    if (!status->wifi_status_shown) {
        status->wifi_status_shown = true;
        status->wifi_status_shown_at_ms = now_ms;
        status->last_action = PANEL_STARTUP_ACTION_WIFI_CONNECTED;
        return PANEL_STARTUP_ACTION_WIFI_CONNECTED;
    }

    if (now_ms < status->wifi_status_shown_at_ms ||
        now_ms - status->wifi_status_shown_at_ms < PANEL_STARTUP_STATUS_MIN_MS) {
        return PANEL_STARTUP_ACTION_NONE;
    }

    if (status->homey_ready_shown &&
        (now_ms < status->homey_ready_shown_at_ms ||
         now_ms - status->homey_ready_shown_at_ms < PANEL_STARTUP_STATUS_MIN_MS)) {
        return PANEL_STARTUP_ACTION_NONE;
    }

    if (homey_state == PANEL_STARTUP_HOMEY_READY) {
        if (!status->homey_ready_shown) {
            status->homey_ready_shown = true;
            status->homey_ready_shown_at_ms = now_ms;
            status->last_action = PANEL_STARTUP_ACTION_HOMEY_READY;
            return PANEL_STARTUP_ACTION_HOMEY_READY;
        }
        status->last_action = PANEL_STARTUP_ACTION_SHOW_DASHBOARD;
        return PANEL_STARTUP_ACTION_SHOW_DASHBOARD;
    }

    status->homey_ready_shown = false;
    if (status->last_action != PANEL_STARTUP_ACTION_HOMEY_LOADING) {
        status->last_action = PANEL_STARTUP_ACTION_HOMEY_LOADING;
        return PANEL_STARTUP_ACTION_HOMEY_LOADING;
    }
    return PANEL_STARTUP_ACTION_NONE;
}

void panel_startup_status_render_failed(
    panel_startup_status_t *status,
    panel_startup_action_t action)
{
    if (status == NULL) return;
    if (action == PANEL_STARTUP_ACTION_WIFI_CONNECTED) {
        status->wifi_status_shown = false;
        status->wifi_status_shown_at_ms = 0U;
        status->last_action = PANEL_STARTUP_ACTION_NONE;
    } else if (action == PANEL_STARTUP_ACTION_HOMEY_READY) {
        status->homey_ready_shown = false;
        status->homey_ready_shown_at_ms = 0U;
        status->last_action = PANEL_STARTUP_ACTION_HOMEY_LOADING;
    } else if (action == PANEL_STARTUP_ACTION_HOMEY_ERROR) {
        status->last_action = PANEL_STARTUP_ACTION_NONE;
    } else if (action == PANEL_STARTUP_ACTION_HOMEY_LOADING) {
        status->last_action = PANEL_STARTUP_ACTION_WIFI_CONNECTED;
    }
}
