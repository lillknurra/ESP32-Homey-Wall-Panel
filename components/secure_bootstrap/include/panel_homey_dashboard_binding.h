#pragma once

#include "panel_homey_read_snapshot.h"
#include "panel_ui_model.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_HOMEY_DASHBOARD_BINDING_COUNT PANEL_UI_WIDGET_COUNT
#define PANEL_HOMEY_DASHBOARD_ALIAS_AWNING_1 "awning_1"
#define PANEL_HOMEY_DASHBOARD_ALIAS_AWNING_2 "awning_2"
#define PANEL_HOMEY_DASHBOARD_ALIAS_AWNING_3 "awning_3"
#define PANEL_HOMEY_DASHBOARD_ALIAS_SECURITY "security"
#define PANEL_HOMEY_DASHBOARD_ALIAS_LIGHT_1 "light_1"
#define PANEL_HOMEY_DASHBOARD_ALIAS_LIGHT_2 "light_2"

#define PANEL_HOMEY_DASHBOARD_CAPABILITY_STATUS "status"
#define PANEL_HOMEY_DASHBOARD_CAPABILITY_ACTIVE "active"
#define PANEL_HOMEY_DASHBOARD_CAPABILITY_ON "on"

typedef enum {
    PANEL_HOMEY_DASHBOARD_APPLY_INVALID = 0,
    PANEL_HOMEY_DASHBOARD_APPLY_UNCHANGED,
    PANEL_HOMEY_DASHBOARD_APPLY_UPDATED,
} panel_homey_dashboard_apply_result_t;

typedef struct {
    panel_widget_status_t status;
    bool has_boolean;
    bool boolean_value;
} panel_homey_dashboard_widget_t;

typedef struct panel_homey_dashboard_state {
    uint32_t generation;
    bool generation_valid;
    bool stale;
    panel_homey_dashboard_widget_t widgets[PANEL_HOMEY_DASHBOARD_BINDING_COUNT];
} panel_homey_dashboard_state_t;

void panel_homey_dashboard_state_init(panel_homey_dashboard_state_t *state);

const char *panel_homey_dashboard_device_alias(size_t widget_index);
const char *panel_homey_dashboard_capability_alias(size_t widget_index);

panel_homey_dashboard_apply_result_t panel_homey_dashboard_apply_snapshot(
    panel_homey_read_result_t snapshot_result,
    const panel_homey_read_snapshot_t *snapshot,
    uint64_t now_ms,
    panel_homey_dashboard_state_t *state);
