#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "panel_ui_model.h"
#include "panel_information.h"

typedef struct panel_ui panel_ui_t;

typedef enum {
    PANEL_UI_CONNECTION_UNKNOWN = 0,
    PANEL_UI_CONNECTION_OFFLINE,
    PANEL_UI_CONNECTION_CONNECTING,
    PANEL_UI_CONNECTION_CONNECTED,
} panel_ui_connection_state_t;

typedef enum {
    PANEL_UI_AWNING_GUARD_READY = 0,
    PANEL_UI_AWNING_GUARD_UNINITIALIZED,
    PANEL_UI_AWNING_GUARD_WRITE_PENDING,
    PANEL_UI_AWNING_GUARD_QUARANTINED,
    PANEL_UI_AWNING_GUARD_UNKNOWN,
} panel_ui_awning_guard_state_t;

typedef struct {
    panel_ui_connection_state_t state;
    char display_name[64];
} panel_ui_connection_info_t;

typedef enum {
    PANEL_UI_TRACE_WIFI_CLICK = 0,
    PANEL_UI_TRACE_CHOOSE_HOMEY_CLICK,
} panel_ui_trace_event_t;

typedef void (*panel_ui_interaction_trace_fn)(
    void *context, panel_ui_trace_event_t event, bool callback_present);

typedef enum {
    PANEL_UI_WIFI_RECONFIGURE_OPENED = 0,
    PANEL_UI_WIFI_RECONFIGURE_BLOCKED,
    PANEL_UI_WIFI_RECONFIGURE_FAILED,
    PANEL_UI_WIFI_CANCEL_FAILED,
    PANEL_UI_WIFI_RECONFIGURE_RESTORED,
} panel_ui_wifi_reconfigure_result_t;

typedef enum {
    PANEL_UI_SETTINGS_SAVE_SAVING = 0,
    PANEL_UI_SETTINGS_SAVE_SAVED,
    PANEL_UI_SETTINGS_SAVE_FAILED,
} panel_ui_settings_save_status_t;

typedef enum {
    PANEL_UI_ROLLER_ACCEPTED = 0,
    PANEL_UI_ROLLER_QUEUED_NEXT,
    PANEL_UI_ROLLER_REJECTED_OPPOSITE,
    PANEL_UI_ROLLER_DEBOUNCED,
    PANEL_UI_ROLLER_LIMIT_REACHED,
    PANEL_UI_ROLLER_BUSY,
    PANEL_UI_ROLLER_NOT_READY,
    PANEL_UI_ROLLER_FAILED,
} panel_ui_roller_request_result_t;

typedef struct {
    void *context;
    void (*request_brightness)(void *, uint8_t);
    void (*request_wifi_reconfigure)(void *);
    void (*request_wifi_cancel)(void *);
    void (*request_choose_homey)(void *);
    void (*request_homey_wipe)(void *);
    void (*request_change_athom_account)(void *);
    bool (*request_firmware_update)(void *);
    bool (*request_light_toggle)(void *, size_t widget_index, bool value);
    /* direction is -1 (UP) or +1 (DOWN); callback must only enqueue. */
    panel_ui_roller_request_result_t (*request_roller_step)(
        void *, size_t widget_index, int8_t direction,
        uint8_t *pending_bar_mask, uint8_t *queued_count, bool *failed);
    void (*settings_changed)(void *, const panel_ui_settings_t *);
    /* Called only on open/manual refresh under the existing display lock.
     * Must be bounded, read-only and must not acquire the display lock again. */
    bool (*read_information)(void *, panel_information_kind_t, panel_information_t *);
    /* Local-only first-commissioning review/confirm surface. Review is
     * read-only and produces sanitized context; confirm may only enqueue. */
    bool (*review_awning_commission)(void *, char *out, size_t capacity);
    bool (*confirm_awning_commission)(void *);
    void (*cancel_awning_commission_review)(void *);
    panel_ui_interaction_trace_fn interaction_trace;
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
bool panel_ui_set_homey_data_ready(panel_ui_t *ui, bool ready);
bool panel_ui_set_light_toggle_pending(
    panel_ui_t *ui, size_t widget_index, bool pending);
/* Presentation direction from the production worker snapshot, never a tap or
 * guard-state guess. Call with zero when no authoritative command is active. */
bool panel_ui_set_roller_active_direction(
    panel_ui_t *ui, size_t widget_index, int8_t direction);
bool panel_ui_set_roller_interaction(
    panel_ui_t *ui, size_t widget_index,
    uint8_t pending_bar_mask, uint8_t queued_count, bool failed);
bool panel_ui_set_roller_guard_state(
    panel_ui_t *ui, size_t widget_index,
    panel_ui_awning_guard_state_t guard_state);
bool panel_ui_select_page(panel_ui_t *ui, uint8_t page_index, bool animate);
bool panel_ui_open_settings(panel_ui_t *ui);
bool panel_ui_close_settings(panel_ui_t *ui);
bool panel_ui_update_inactivity(panel_ui_t *ui, uint64_t now_ms);
bool panel_ui_process_touch(panel_ui_t *ui, uint64_t now_ms);
bool panel_ui_set_wifi_reconfigure_result(
    panel_ui_t *ui, panel_ui_wifi_reconfigure_result_t result);
bool panel_ui_present_wifi_provisioning(
    panel_ui_t *ui, void *qr_object, bool commit_reserved);
bool panel_ui_update_wifi_provisioning(
    panel_ui_t *ui, void *qr_object, void *qr_frame,
    const char *ssid, const char *password, bool cancel_allowed);
bool panel_ui_set_wifi_cancel_pending(panel_ui_t *ui, bool pending, bool commit_reserved);
bool panel_ui_clear_wifi_provisioning(panel_ui_t *ui, void *qr_object);
bool panel_ui_set_settings_save_status(
    panel_ui_t *ui, panel_ui_settings_save_status_t status);
bool panel_ui_restore_settings(
    panel_ui_t *ui, const panel_ui_settings_t *confirmed_settings);
bool panel_ui_reset_view(panel_ui_t *ui);
