#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define PANEL_UI_PAGE_COUNT 3U
#define PANEL_UI_WIDGET_COUNT 6U
#define PANEL_UI_WIDGET_TITLE_MAX 96U
#define PANEL_UI_TIMEZONE_ID_MAX 32U
#define PANEL_UI_TIME_TEXT_MAX 8U
#define PANEL_UI_DATE_TEXT_MAX 32U
#define PANEL_UI_DEFAULT_NORMAL_BRIGHTNESS 80U
#define PANEL_UI_DEFAULT_DIMMED_BRIGHTNESS 30U
#define PANEL_UI_DEFAULT_DIM_AFTER_SECONDS 60U
#define PANEL_UI_DEFAULT_WAKE_ON_TOUCH true
#define PANEL_UI_TIMEOUT_MIN_SECONDS 10U
#define PANEL_UI_TIMEOUT_MAX_SECONDS 86400U
#define PANEL_UI_OFF_DISABLED 0U
#define PANEL_UI_TIMEZONE_EUROPE_STOCKHOLM "Europe/Stockholm"

typedef enum {
    PANEL_UI_VIEW_DASHBOARD = 0,
    PANEL_UI_VIEW_SETTINGS,
    PANEL_UI_VIEW_CONFIRMATION,
} panel_ui_view_t;

typedef enum {
    PANEL_WIDGET_UNCONFIGURED = 0,
    PANEL_WIDGET_UNKNOWN,
    PANEL_WIDGET_AVAILABLE,
    PANEL_WIDGET_UNAVAILABLE,
} panel_widget_status_t;

typedef enum {
    PANEL_POWER_ACTIVE = 0,
    PANEL_POWER_DIMMED,
    PANEL_POWER_OFF,
} panel_power_state_t;

typedef enum {
    PANEL_BACKGROUND_OFF = 0,
    PANEL_BACKGROUND_BUILT_IN,
} panel_background_mode_t;

typedef enum {
    PANEL_CONFIRM_NONE = 0,
    PANEL_CONFIRM_HOMEY_WIPE,
    PANEL_CONFIRM_ATHOM_ACCOUNT_CHANGE,
} panel_confirmation_t;

typedef enum {
    PANEL_CONFIRMED_ACTION_NONE = 0,
    PANEL_CONFIRMED_ACTION_HOMEY_WIPE,
    PANEL_CONFIRMED_ACTION_ATHOM_ACCOUNT_CHANGE,
} panel_confirmed_action_t;

typedef struct {
    uint8_t normal_brightness;
    uint8_t dimmed_brightness;
    uint32_t dim_after_seconds;
    uint32_t off_after_seconds;
    bool wake_on_touch;
    panel_background_mode_t background_mode;
    char timezone_id[PANEL_UI_TIMEZONE_ID_MAX];
} panel_ui_settings_t;

struct panel_homey_dashboard_state;

typedef struct {
    panel_ui_view_t view;
    uint8_t active_page;
    panel_power_state_t power_state;
    panel_confirmation_t confirmation;
    uint64_t last_activity_ms;
    panel_ui_settings_t settings;
    char widget_title[PANEL_UI_WIDGET_COUNT][PANEL_UI_WIDGET_TITLE_MAX];
    panel_widget_status_t widget_status[PANEL_UI_WIDGET_COUNT];
    bool widget_has_boolean[PANEL_UI_WIDGET_COUNT];
    bool widget_boolean_value[PANEL_UI_WIDGET_COUNT];
    uint32_t homey_generation;
    bool homey_generation_valid;
    bool homey_snapshot_stale;
} panel_ui_model_t;

void panel_ui_settings_defaults(panel_ui_settings_t *settings);
void panel_ui_settings_normalize(panel_ui_settings_t *settings);
void panel_ui_model_init(panel_ui_model_t *model, uint64_t now_ms);
uint8_t panel_ui_set_active_page(panel_ui_model_t *model, int page_index);
void panel_ui_set_view(panel_ui_model_t *model, panel_ui_view_t view);
void panel_ui_register_activity(panel_ui_model_t *model, uint64_t now_ms);
void panel_ui_tick(panel_ui_model_t *model, uint64_t now_ms);
bool panel_ui_handle_touch(panel_ui_model_t *model, uint64_t now_ms);
void panel_ui_request_confirmation(panel_ui_model_t *model, panel_confirmation_t confirmation);
void panel_ui_cancel_confirmation(panel_ui_model_t *model);
panel_confirmed_action_t panel_ui_accept_confirmation(panel_ui_model_t *model);
const char *panel_ui_page_title(size_t page_index);
const char *panel_ui_widget_title(size_t widget_index);
const char *panel_ui_widget_title_for_model(const panel_ui_model_t *model, size_t widget_index);
const char *panel_ui_widget_status_text(panel_widget_status_t status);
bool panel_ui_apply_homey_dashboard_state(
    panel_ui_model_t *model,
    const struct panel_homey_dashboard_state *state);
bool panel_ui_widget_display_text(
    const panel_ui_model_t *model,
    size_t widget_index,
    char *buffer,
    size_t buffer_size);
bool panel_ui_format_clock(bool synchronized, const struct tm *local_time, char *buffer, size_t buffer_size);
bool panel_ui_format_date_sv(bool synchronized, const struct tm *local_time, char *buffer, size_t buffer_size);
