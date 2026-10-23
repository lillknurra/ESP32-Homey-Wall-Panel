#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PANEL_STARTUP_STATUS_MIN_MS 2000U

typedef enum {
    PANEL_STARTUP_HOMEY_LOADING = 0,
    PANEL_STARTUP_HOMEY_READY,
    PANEL_STARTUP_HOMEY_ERROR,
} panel_startup_homey_state_t;

typedef enum {
    PANEL_STARTUP_ACTION_NONE = 0,
    PANEL_STARTUP_ACTION_WIFI_CONNECTED,
    PANEL_STARTUP_ACTION_WIFI_DISCONNECTED,
    PANEL_STARTUP_ACTION_HOMEY_LOADING,
    PANEL_STARTUP_ACTION_HOMEY_READY,
    PANEL_STARTUP_ACTION_HOMEY_ERROR,
    PANEL_STARTUP_ACTION_SHOW_DASHBOARD,
} panel_startup_action_t;

typedef struct {
    bool wifi_status_shown;
    uint64_t wifi_status_shown_at_ms;
    bool homey_ready_shown;
    uint64_t homey_ready_shown_at_ms;
    panel_startup_action_t last_action;
} panel_startup_status_t;

typedef struct {
    uint16_t primary_y;
    uint16_t primary_height;
    uint16_t secondary_y;
    uint16_t secondary_height;
} panel_startup_layout_t;

void panel_startup_status_init(panel_startup_status_t *status);
panel_startup_action_t panel_startup_status_tick(
    panel_startup_status_t *status,
    bool wifi_online,
    panel_startup_homey_state_t homey_state,
    uint64_t now_ms);
void panel_startup_status_render_failed(
    panel_startup_status_t *status,
    panel_startup_action_t action);

bool panel_startup_layout_compute(
    uint16_t screen_height,
    uint16_t reserved_bottom,
    uint16_t reserved_gap,
    uint16_t bottom_margin,
    uint16_t text_gap,
    uint16_t primary_height,
    uint16_t secondary_height,
    panel_startup_layout_t *layout);
