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

    if (!wifi_online) {
        const bool had_online_status = status->wifi_status_shown ||
            status->homey_ready_shown ||
            status->last_action == PANEL_STARTUP_ACTION_WIFI_CONNECTED ||
            status->last_action == PANEL_STARTUP_ACTION_HOMEY_LOADING ||
            status->last_action == PANEL_STARTUP_ACTION_HOMEY_READY ||
            status->last_action == PANEL_STARTUP_ACTION_SHOW_DASHBOARD;
        status->wifi_status_shown = false;
        status->wifi_status_shown_at_ms = 0U;
        status->homey_ready_shown = false;
        status->homey_ready_shown_at_ms = 0U;
        if (homey_state == PANEL_STARTUP_HOMEY_ERROR) {
            const bool newly_visible = status->last_action != PANEL_STARTUP_ACTION_HOMEY_ERROR;
            status->last_action = PANEL_STARTUP_ACTION_HOMEY_ERROR;
            return newly_visible ? PANEL_STARTUP_ACTION_HOMEY_ERROR : PANEL_STARTUP_ACTION_NONE;
        }
        if (had_online_status) {
            status->last_action = PANEL_STARTUP_ACTION_WIFI_DISCONNECTED;
            return PANEL_STARTUP_ACTION_WIFI_DISCONNECTED;
        }
        status->last_action = PANEL_STARTUP_ACTION_NONE;
        return PANEL_STARTUP_ACTION_NONE;
    }

    /* Surface failures immediately, even if a success message is being held. */
    if (homey_state == PANEL_STARTUP_HOMEY_ERROR) {
        const bool newly_visible = status->last_action != PANEL_STARTUP_ACTION_HOMEY_ERROR;
        status->last_action = PANEL_STARTUP_ACTION_HOMEY_ERROR;
        status->homey_ready_shown = false;
        status->homey_ready_shown_at_ms = 0U;
        /* An error is not a rendered Wi-Fi-connected status. If it replaces
         * that status before its minimum visible interval elapsed, require
         * the Wi-Fi milestone to be shown again after recovery. */
        if (status->wifi_status_shown &&
            (now_ms < status->wifi_status_shown_at_ms ||
             now_ms - status->wifi_status_shown_at_ms < PANEL_STARTUP_STATUS_MIN_MS)) {
            status->wifi_status_shown = false;
            status->wifi_status_shown_at_ms = 0U;
        }
        return newly_visible ? PANEL_STARTUP_ACTION_HOMEY_ERROR : PANEL_STARTUP_ACTION_NONE;
    }
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

    if (status->homey_ready_shown && homey_state != PANEL_STARTUP_HOMEY_READY) {
        status->homey_ready_shown = false;
        status->homey_ready_shown_at_ms = 0U;
        status->last_action = PANEL_STARTUP_ACTION_HOMEY_LOADING;
        return PANEL_STARTUP_ACTION_HOMEY_LOADING;
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
    } else if (action == PANEL_STARTUP_ACTION_WIFI_DISCONNECTED) {
        status->last_action = PANEL_STARTUP_ACTION_WIFI_CONNECTED;
    } else if (action == PANEL_STARTUP_ACTION_SHOW_DASHBOARD) {
        status->homey_ready_shown = false;
        status->homey_ready_shown_at_ms = 0U;
        status->last_action = PANEL_STARTUP_ACTION_HOMEY_LOADING;
    }
}

bool panel_startup_layout_compute(
    uint16_t screen_height,
    uint16_t reserved_bottom,
    uint16_t reserved_gap,
    uint16_t bottom_margin,
    uint16_t text_gap,
    uint16_t primary_height,
    uint16_t secondary_height,
    panel_startup_layout_t *layout)
{
    if (layout == NULL || screen_height == 0U || primary_height == 0U ||
        bottom_margin > screen_height || text_gap > screen_height) {
        return false;
    }

    const uint32_t primary_bottom = (uint32_t)screen_height - bottom_margin;
    if (primary_height > primary_bottom) return false;
    const uint32_t primary_y = primary_bottom - primary_height;
    if (secondary_height > primary_y || text_gap > primary_y - secondary_height) {
        return false;
    }
    const uint32_t secondary_y = primary_y - text_gap - secondary_height;
    const uint32_t reserved_top = (uint32_t)reserved_bottom + reserved_gap;
    if (secondary_y < reserved_top) return false;

    layout->primary_y = (uint16_t)primary_y;
    layout->primary_height = primary_height;
    layout->secondary_y = (uint16_t)secondary_y;
    layout->secondary_height = secondary_height;
    return true;
}

void panel_startup_wait_update(
    panel_startup_wait_t *wait,
    bool wifi_online,
    bool verified_ready,
    uint64_t now_ms)
{
    if (wait == NULL) return;
    if (!wifi_online || verified_ready) {
        wait->active = false;
        wait->started_at_ms = 0U;
        return;
    }
    if (!wait->active) {
        wait->active = true;
        wait->started_at_ms = now_ms;
    }
}

bool panel_startup_wait_expired(
    const panel_startup_wait_t *wait,
    bool homey_error,
    uint64_t now_ms)
{
    return wait != NULL && wait->active && !homey_error &&
        now_ms >= wait->started_at_ms &&
        now_ms - wait->started_at_ms >= PANEL_STARTUP_LONG_WAIT_MS;
}

bool panel_startup_compose_visible_text(
    char *out,
    size_t out_capacity,
    const char *primary,
    const char *secondary)
{
    if (out == NULL || out_capacity == 0U) return false;
    out[0] = '\0';

    const char *first = primary != NULL ? primary : "";
    const char *second = secondary != NULL ? secondary : "";
    const size_t first_len = strlen(first);
    const size_t second_len = strlen(second);
    const size_t separator_len = first_len != 0U && second_len != 0U ? 1U : 0U;

    if (first_len >= out_capacity ||
        second_len > out_capacity - first_len - 1U ||
        separator_len > out_capacity - first_len - second_len - 1U) {
        return false;
    }

    memcpy(out, first, first_len);
    size_t used = first_len;
    if (separator_len != 0U) out[used++] = '\n';
    memcpy(out + used, second, second_len);
    out[used + second_len] = '\0';
    return true;
}
