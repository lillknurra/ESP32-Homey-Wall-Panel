#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "panel_ui_model.h"

typedef struct panel_ui panel_ui_t;

typedef enum {
    PANEL_UI_CONNECTION_UNKNOWN = 0,
    PANEL_UI_CONNECTION_OFFLINE,
    PANEL_UI_CONNECTION_CONNECTING,
    PANEL_UI_CONNECTION_CONNECTED,
} panel_ui_connection_state_t;

typedef struct {
    panel_ui_connection_state_t state;
    char display_name[64];
} panel_ui_connection_info_t;

typedef struct {
    void *context;
    void (*request_brightness)(void *, uint8_t);
    void (*request_wifi_reconfigure)(void *);
    void (*request_choose_homey)(void *);
    void (*request_homey_wipe)(void *);
    void (*request_change_athom_account)(void *);
    void (*settings_changed)(void *, const panel_ui_settings_t *);
} panel_ui_callbacks_t;

typedef struct {
    panel_ui_model_t *model;
    panel_ui_callbacks_t callbacks;
} panel_ui_config_t;

/* Caller holds BSP/LVGL display lock. No LVGL type is exposed here. */
bool panel_ui_create(panel_ui_t **ui_out, const panel_ui_config_t *config);
void panel_ui_destroy(panel_ui_t **ui);
bool panel_ui_activate(panel_ui_t *ui);
bool panel_ui_is_active(const panel_ui_t *ui);
bool panel_ui_refresh(panel_ui_t *ui);
bool panel_ui_set_time(panel_ui_t *ui, const struct tm *local_time, bool valid);
bool panel_ui_set_connection(panel_ui_t *ui, const panel_ui_connection_info_t *connection);
bool panel_ui_select_page(panel_ui_t *ui, uint8_t page_index, bool animate);
bool panel_ui_open_settings(panel_ui_t *ui);
bool panel_ui_close_settings(panel_ui_t *ui);
bool panel_ui_tick(panel_ui_t *ui, uint64_t now_ms);
bool panel_ui_handle_touch(panel_ui_t *ui, uint64_t now_ms);
bool panel_ui_reset_view(panel_ui_t *ui);
