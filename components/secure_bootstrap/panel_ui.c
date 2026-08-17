#include "panel_ui.h"
#include "panel_homey_favorites.h"
#include "homey_panel_font_16.h"
#include "homey_panel_font_18.h"
#include "homey_panel_font_22.h"
#include "lvgl.h"
#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_timer.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 480
#define H 480
#define TOP_H 60
#define PAGE_H 392
#define IND_H 28
#define CARD_W 221
#define CARD_H 114
#define PATCH021_SETTINGS_DIRECT_CHILD_COUNT 12U

struct panel_ui {
    panel_ui_model_t *model;
    panel_ui_callbacks_t callbacks;
    lv_obj_t *screen;
    lv_obj_t *clock_label;
    lv_obj_t *date_label;
    lv_obj_t *connection_label;
    lv_obj_t *pager;
    lv_obj_t *pages[PANEL_UI_PAGE_COUNT];
    lv_obj_t *widget_title[PANEL_UI_WIDGET_COUNT];
    lv_obj_t *widget_status[PANEL_UI_WIDGET_COUNT];
    lv_obj_t *dots[PANEL_UI_PAGE_COUNT];
    lv_obj_t *settings_layer;
    lv_obj_t *settings_feedback;
    lv_obj_t *normal_brightness_label;
    lv_obj_t *dim_brightness_label;
    lv_obj_t *dim_timeout_label;
    lv_obj_t *off_timeout_label;
    lv_obj_t *background_label;
    lv_obj_t *choose_homey_button;
    lv_obj_t *wipe_homey_button;
    lv_obj_t *change_account_button;
    lv_obj_t *confirmation_layer;
    lv_obj_t *confirmation_text;
    lv_obj_t *homey_info_layer;
    lv_obj_t *wake_overlay;
    char clock_text[PANEL_UI_TIME_TEXT_MAX];
    char date_text[PANEL_UI_DATE_TEXT_MAX];
    panel_ui_connection_info_t connection;
    bool homey_data_ready;
    panel_power_state_t rendered_power;
    uint8_t rendered_brightness;
    bool active;
    bool destroying;
    bool patch018_gesture_active;
    uint32_t patch018_gesture_start_ms;
    int32_t patch018_gesture_start_x;
    bool patch021_settings_scroll_active;
    uint32_t patch021_settings_scroll_start_ms;
    int32_t patch021_settings_scroll_start_y;
};

static void render_view(panel_ui_t *ui);
static void render_settings(panel_ui_t *ui);
static void render_power(panel_ui_t *ui);

static bool panel_homey_actions_allowed(const panel_ui_t *ui)
{
    return ui != NULL && ui->homey_data_ready;
}

static void panel_homey_controls_set_enabled(panel_ui_t *ui)
{
    if (ui == NULL) return;
    lv_obj_t *controls[] = {
        ui->choose_homey_button,
        ui->wipe_homey_button,
        ui->change_account_button,
    };
    for (size_t index = 0U; index < sizeof(controls) / sizeof(controls[0]); ++index) {
        if (controls[index] == NULL) continue;
        if (ui->homey_data_ready) lv_obj_remove_state(controls[index], LV_STATE_DISABLED);
        else lv_obj_add_state(controls[index], LV_STATE_DISABLED);
    }
}

static lv_obj_t *label_new(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text != NULL ? text : "");
    lv_obj_set_style_text_color(label, lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_text_font(label, &homey_panel_font_22, 0);
    return label;
}

static void activity_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->destroying) return;
    panel_ui_register_activity(ui->model, (uint64_t)lv_tick_get());
    render_power(ui);
}

static lv_obj_t *button_new_sized(lv_obj_t *parent, const char *text,
                                  int32_t width, int32_t height,
                                  lv_event_cb_t callback, panel_ui_t *ui,
                                  lv_obj_t **label_out)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_add_event_cb(button, activity_event, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, ui);
    lv_obj_t *label = label_new(button, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(92));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label);
    if (label_out != NULL) *label_out = label;
    return button;
}

static lv_obj_t *settings_button_new(lv_obj_t *parent, const char *text,
                                     lv_event_cb_t callback, panel_ui_t *ui,
                                     lv_obj_t **label_out)
{
    return button_new_sized(parent, text, LV_PCT(100), 56, callback, ui, label_out);
}

static lv_obj_t *modal_button_new(lv_obj_t *parent, const char *text,
                                  lv_event_cb_t callback, panel_ui_t *ui)
{
    return button_new_sized(parent, text, 166, 54, callback, ui, NULL);
}

static lv_obj_t *hamburger_button_new(lv_obj_t *parent,
                                      lv_event_cb_t callback,
                                      panel_ui_t *ui)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, 52, 48);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_add_event_cb(button, activity_event, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, ui);

    static const int32_t offsets[] = {-9, 0, 9};
    for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        lv_obj_t *bar = lv_obj_create(button);
        lv_obj_set_size(bar, 26, 3);
        lv_obj_align(bar, LV_ALIGN_CENTER, 0, offsets[i]);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0xF4F7FA), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 2, 0);
        lv_obj_set_style_pad_all(bar, 0, 0);
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    }
    return button;
}

static uint8_t next_u8(uint8_t current, const uint8_t *values, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (values[i] == current) return values[(i + 1U) % count];
    }
    return values[0];
}

static uint32_t next_u32(uint32_t current, const uint32_t *values, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (values[i] == current) return values[(i + 1U) % count];
    }
    return values[0];
}

static const uint8_t NORMAL_LEVELS[] = {20U, 40U, 60U, 80U, 100U};
static const uint8_t DIMMED_LEVELS[] = {10U, 30U, 50U};
static const uint32_t DIM_TIMEOUTS[] = {10U, 30U, 60U};
static const uint32_t OFF_TIMEOUTS[] = {60U, 300U, 1200U, PANEL_UI_OFF_DISABLED};

static lv_obj_t *create_read_only_card(panel_ui_t *ui, lv_obj_t *parent, size_t index)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(card, CARD_W, CARD_H);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x162735), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_90, 0);
    ui->widget_title[index] = label_new(card, panel_ui_widget_title_for_model(ui->model, index));
    lv_obj_align(ui->widget_title[index], LV_ALIGN_TOP_LEFT, 4, 2);
    char status_text[32];
    if (!panel_ui_widget_display_text(
            ui->model, index, status_text, sizeof(status_text))) {
        (void)snprintf(status_text, sizeof(status_text), "%s", "Okänd");
    }
    ui->widget_status[index] = label_new(card, status_text);
    lv_obj_align(ui->widget_status[index], LV_ALIGN_BOTTOM_LEFT, 4, -4);
    return card;
}

static void dots_render(panel_ui_t *ui)
{
    for (size_t i = 0; i < PANEL_UI_PAGE_COUNT; ++i) {
        lv_obj_set_style_bg_opa(ui->dots[i],
            i == ui->model->active_page ? LV_OPA_COVER : LV_OPA_40, 0);
    }
}

static void pager_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_SCROLL_END) return;
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->destroying) return;
    int32_t x = lv_obj_get_scroll_x(ui->pager);
    (void)panel_ui_set_active_page(ui->model, (x + W / 2) / W);
    dots_render(ui);
}

static void patch018_clear_gesture(panel_ui_t *ui)
{
    ui->patch018_gesture_active = false;
    ui->patch018_gesture_start_ms = 0U;
    ui->patch018_gesture_start_x = 0;
}

static void patch018_swipe_gesture_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->destroying) return;

    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SCROLL_BEGIN) {
        if (ui->patch018_gesture_active) return;

        lv_indev_t *indev = lv_indev_active();
        if (lv_indev_get_state(indev) != LV_INDEV_STATE_PRESSED) return;

        ui->patch018_gesture_active = true;
        ui->patch018_gesture_start_ms = (uint32_t)lv_tick_get();
        ui->patch018_gesture_start_x = lv_obj_get_scroll_x(ui->pager);
#ifdef ESP_PLATFORM
        ESP_LOGI(
            "panel_ui",
            "PATCH018_SWIPE_BEGIN start_x=%ld",
            (long)ui->patch018_gesture_start_x);
        ESP_LOGI(
            "panel_ui",
            "PATCH021_UI_SCROLL surface=dashboard phase=begin start_x=%ld "
            "active_page=%u privacy=sanitized",
            (long)ui->patch018_gesture_start_x,
            (unsigned)ui->model->active_page);
#endif
        return;
    }

    if (code != LV_EVENT_SCROLL_END || !ui->patch018_gesture_active) return;

    lv_indev_t *indev = lv_indev_active();
    if (lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED) return;

    const uint32_t ended_ms = (uint32_t)lv_tick_get();
    const uint32_t elapsed_ms = ended_ms - ui->patch018_gesture_start_ms;
    const int32_t end_x = lv_obj_get_scroll_x(ui->pager);
#ifdef ESP_PLATFORM
    ESP_LOGI(
        "panel_ui",
        "PATCH018_SWIPE_END elapsed_ms=%lu start_x=%ld end_x=%ld resolved_page=%u",
        (unsigned long)elapsed_ms,
        (long)ui->patch018_gesture_start_x,
        (long)end_x,
        (unsigned)ui->model->active_page);
    ESP_LOGI(
        "panel_ui",
        "PATCH021_UI_SCROLL surface=dashboard phase=end elapsed_ms=%lu "
        "start_x=%ld end_x=%ld resolved_page=%u privacy=sanitized",
        (unsigned long)elapsed_ms,
        (long)ui->patch018_gesture_start_x,
        (long)end_x,
        (unsigned)ui->model->active_page);
#endif
    patch018_clear_gesture(ui);
}

static void patch021_settings_scroll_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->destroying || ui->settings_layer == NULL) return;

    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SCROLL_BEGIN) {
        if (ui->patch021_settings_scroll_active) return;

        lv_indev_t *indev = lv_indev_active();
        if (indev != NULL && lv_indev_get_state(indev) != LV_INDEV_STATE_PRESSED) return;

        ui->patch021_settings_scroll_active = true;
        ui->patch021_settings_scroll_start_ms = (uint32_t)lv_tick_get();
        ui->patch021_settings_scroll_start_y = lv_obj_get_scroll_y(ui->settings_layer);
#ifdef ESP_PLATFORM
        ESP_LOGI(
            "panel_ui",
            "PATCH021_UI_SCROLL surface=settings phase=begin start_y=%ld "
            "direct_children=%u privacy=sanitized",
            (long)ui->patch021_settings_scroll_start_y,
            (unsigned)PATCH021_SETTINGS_DIRECT_CHILD_COUNT);
#endif
        return;
    }

    if (code != LV_EVENT_SCROLL_END || !ui->patch021_settings_scroll_active) return;

    lv_indev_t *indev = lv_indev_active();
    if (indev != NULL && lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED) return;

    const uint32_t ended_ms = (uint32_t)lv_tick_get();
    const uint32_t elapsed_ms = ended_ms - ui->patch021_settings_scroll_start_ms;
    const int32_t end_y = lv_obj_get_scroll_y(ui->settings_layer);
#ifdef ESP_PLATFORM
    ESP_LOGI(
        "panel_ui",
        "PATCH021_UI_SCROLL surface=settings phase=end elapsed_ms=%lu "
        "start_y=%ld end_y=%ld direct_children=%u privacy=sanitized",
        (unsigned long)elapsed_ms,
        (long)ui->patch021_settings_scroll_start_y,
        (long)end_y,
        (unsigned)PATCH021_SETTINGS_DIRECT_CHILD_COUNT);
#endif
    ui->patch021_settings_scroll_active = false;
    ui->patch021_settings_scroll_start_ms = 0U;
    ui->patch021_settings_scroll_start_y = 0;
}

static void open_settings_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui != NULL) (void)panel_ui_open_settings(ui);
}

static void close_settings_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui != NULL) (void)panel_ui_close_settings(ui);
}

static void notify_settings(panel_ui_t *ui)
{
    panel_ui_settings_normalize(&ui->model->settings);
    if (ui->callbacks.settings_changed != NULL) {
        ui->callbacks.settings_changed(ui->callbacks.context, &ui->model->settings);
    }
    render_settings(ui);
    render_power(ui);
}

static void normal_brightness_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    ui->model->settings.normal_brightness = next_u8(
        ui->model->settings.normal_brightness, NORMAL_LEVELS,
        sizeof(NORMAL_LEVELS) / sizeof(NORMAL_LEVELS[0]));
    notify_settings(ui);
}

static void dim_brightness_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    ui->model->settings.dimmed_brightness = next_u8(
        ui->model->settings.dimmed_brightness, DIMMED_LEVELS,
        sizeof(DIMMED_LEVELS) / sizeof(DIMMED_LEVELS[0]));
    notify_settings(ui);
}

static void dim_timeout_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    ui->model->settings.dim_after_seconds = next_u32(
        ui->model->settings.dim_after_seconds, DIM_TIMEOUTS,
        sizeof(DIM_TIMEOUTS) / sizeof(DIM_TIMEOUTS[0]));
    notify_settings(ui);
}

static void off_timeout_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    ui->model->settings.off_after_seconds = next_u32(
        ui->model->settings.off_after_seconds, OFF_TIMEOUTS,
        sizeof(OFF_TIMEOUTS) / sizeof(OFF_TIMEOUTS[0]));
    notify_settings(ui);
}

static void background_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    ui->model->settings.background_mode =
        ui->model->settings.background_mode == PANEL_BACKGROUND_OFF
        ? PANEL_BACKGROUND_BUILT_IN : PANEL_BACKGROUND_OFF;
    notify_settings(ui);
}

static void wifi_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    lv_label_set_text(ui->settings_feedback, "Öppnar Wi-Fi-inställning...");
    bool present = ui->callbacks.request_wifi_reconfigure != NULL;
    if (ui->callbacks.interaction_trace != NULL)
        ui->callbacks.interaction_trace(ui->callbacks.context, PANEL_UI_TRACE_WIFI_CLICK, present);
    if (present) ui->callbacks.request_wifi_reconfigure(ui->callbacks.context);
}

static void homey_info_close_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui != NULL && ui->homey_info_layer != NULL)
        lv_obj_add_flag(ui->homey_info_layer, LV_OBJ_FLAG_HIDDEN);
}

static void choose_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (!panel_homey_actions_allowed(ui)) return;
    if (ui->homey_info_layer != NULL) {
        lv_obj_remove_flag(ui->homey_info_layer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui->homey_info_layer);
    }
    bool present = ui->callbacks.request_choose_homey != NULL;
    if (ui->callbacks.interaction_trace != NULL)
        ui->callbacks.interaction_trace(ui->callbacks.context, PANEL_UI_TRACE_CHOOSE_HOMEY_CLICK, present);
    if (present) ui->callbacks.request_choose_homey(ui->callbacks.context);
}

static void wipe_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (!panel_homey_actions_allowed(ui)) return;
    panel_ui_request_confirmation(ui->model, PANEL_CONFIRM_HOMEY_WIPE);
    render_view(ui);
}

static void account_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (!panel_homey_actions_allowed(ui)) return;
    panel_ui_request_confirmation(ui->model, PANEL_CONFIRM_ATHOM_ACCOUNT_CHANGE);
    render_view(ui);
}

static void cancel_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    panel_ui_cancel_confirmation(ui->model);
    render_view(ui);
}

static void accept_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || !panel_homey_actions_allowed(ui)) return;
    panel_confirmed_action_t action = panel_ui_accept_confirmation(ui->model);
    render_view(ui);
    lv_label_set_text(ui->settings_feedback, "Funktionen aktiveras i nästa steg");
    if (action == PANEL_CONFIRMED_ACTION_HOMEY_WIPE &&
        ui->callbacks.request_homey_wipe != NULL)
        ui->callbacks.request_homey_wipe(ui->callbacks.context);
    if (action == PANEL_CONFIRMED_ACTION_ATHOM_ACCOUNT_CHANGE &&
        ui->callbacks.request_change_athom_account != NULL)
        ui->callbacks.request_change_athom_account(ui->callbacks.context);
}

static void wake_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui != NULL) (void)panel_ui_process_touch(ui, (uint64_t)lv_tick_get());
    lv_event_stop_processing(event);
}

static void render_connection(panel_ui_t *ui)
{
    const char *state = "Okänd";
    if (ui->connection.state == PANEL_UI_CONNECTION_OFFLINE) state = "Frånkopplad";
    else if (ui->connection.state == PANEL_UI_CONNECTION_CONNECTING) state = "Ansluter";
    else if (ui->connection.state == PANEL_UI_CONNECTION_CONNECTED) state = "Ansluten";
    char text[96];
    if (ui->connection.display_name[0] != '\0')
        (void)snprintf(text, sizeof(text), "%s · %s", ui->connection.display_name, state);
    else
        (void)snprintf(text, sizeof(text), "%s", state);
    lv_label_set_text(ui->connection_label, text);
}

static void render_settings(panel_ui_t *ui)
{
    char text[96];
    (void)snprintf(text, sizeof(text), "Normal ljusstyrka: %u %%", (unsigned)ui->model->settings.normal_brightness);
    lv_label_set_text(ui->normal_brightness_label, text);
    (void)snprintf(text, sizeof(text), "Dämpad ljusstyrka: %u %%", (unsigned)ui->model->settings.dimmed_brightness);
    lv_label_set_text(ui->dim_brightness_label, text);
    (void)snprintf(text, sizeof(text), "Dämpningstid: %s",
        ui->model->settings.dim_after_seconds == 60U ? "1 min" :
        ui->model->settings.dim_after_seconds == 30U ? "30 s" : "10 s");
    lv_label_set_text(ui->dim_timeout_label, text);
    (void)snprintf(text, sizeof(text), "Släckningstid: %s",
        ui->model->settings.off_after_seconds == PANEL_UI_OFF_DISABLED ? "Alltid på" :
        ui->model->settings.off_after_seconds == 1200U ? "20 min" :
        ui->model->settings.off_after_seconds == 300U ? "5 min" : "1 min");
    lv_label_set_text(ui->off_timeout_label, text);
    (void)snprintf(text, sizeof(text), "Bakgrund: %s",
        ui->model->settings.background_mode == PANEL_BACKGROUND_BUILT_IN ? "Inbyggd" : "Av");
    lv_label_set_text(ui->background_label, text);
    if (ui->model->settings.background_mode == PANEL_BACKGROUND_BUILT_IN) {
        lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0x071A29), 0);
        lv_obj_set_style_bg_grad_color(ui->screen, lv_color_hex(0x16384A), 0);
        lv_obj_set_style_bg_grad_dir(ui->screen, LV_GRAD_DIR_VER, 0);
    } else {
        lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0x071018), 0);
        lv_obj_set_style_bg_grad_dir(ui->screen, LV_GRAD_DIR_NONE, 0);
    }
}

static void render_view(panel_ui_t *ui)
{
    bool settings = ui->model->view == PANEL_UI_VIEW_SETTINGS;
    bool confirm = ui->model->view == PANEL_UI_VIEW_CONFIRMATION;
    if (settings || confirm) lv_obj_remove_flag(ui->settings_layer, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui->settings_layer, LV_OBJ_FLAG_HIDDEN);
    if (confirm) {
        lv_obj_remove_flag(ui->confirmation_layer, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui->confirmation_text,
            ui->model->confirmation == PANEL_CONFIRM_HOMEY_WIPE
            ? "Radera Homey-anslutning\n\nFunktionen aktiveras i ett senare steg.\nIngen anslutning ändras nu."
            : "Byt Athom-konto\n\nFunktionen aktiveras i ett senare steg.\nInget konto ändras nu.");
    } else lv_obj_add_flag(ui->confirmation_layer, LV_OBJ_FLAG_HIDDEN);
}

static bool ensure_wake_overlay(panel_ui_t *ui)
{
    if (ui == NULL || ui->screen == NULL || lv_screen_active() != ui->screen) return false;
    if (ui->wake_overlay != NULL && lv_obj_get_parent(ui->wake_overlay) != ui->screen) {
        lv_obj_delete(ui->wake_overlay);
        ui->wake_overlay = NULL;
    }
    if (ui->wake_overlay == NULL) {
        ui->wake_overlay = lv_obj_create(ui->screen);
        if (ui->wake_overlay == NULL) return false;
        lv_obj_remove_style_all(ui->wake_overlay);
        lv_obj_set_size(ui->wake_overlay, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_border_width(ui->wake_overlay, 0, 0);
        lv_obj_remove_flag(ui->wake_overlay, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(ui->wake_overlay, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui->wake_overlay, wake_event, LV_EVENT_PRESSED, ui);
    }
    return true;
}

static void refresh_active_dashboard(panel_ui_t *ui)
{
    /* Patch 017 v4.5: wake/power refresh must use the same display-text
     * formatter as the normal full refresh. panel_ui_widget_status_text()
     * collapses AVAILABLE+boolean to the generic "Tillgänglig" string,
     * which erased Tänd/Släckt visually after DIMMED -> ACTIVE wake. */
    for (size_t index = 0; index < PANEL_UI_WIDGET_COUNT; ++index) {
        char status_text[32];
        if (!panel_ui_widget_display_text(
                ui->model, index, status_text, sizeof(status_text))) {
            (void)snprintf(status_text, sizeof(status_text), "%s", "Okänd");
        }
        lv_label_set_text(ui->widget_status[index], status_text);
    }
    render_connection(ui);
    render_settings(ui);
    render_view(ui);
    dots_render(ui);
    lv_obj_scroll_to_x(ui->pager, (int32_t)ui->model->active_page * W, LV_ANIM_OFF);
}

static void render_power(panel_ui_t *ui)
{
    const panel_power_state_t state = ui->model->power_state;
    uint8_t brightness = ui->model->settings.normal_brightness;
    if (state == PANEL_POWER_DIMMED) brightness = ui->model->settings.dimmed_brightness;
    else if (state == PANEL_POWER_OFF) brightness = 0U;

    const bool transition = state != ui->rendered_power || brightness != ui->rendered_brightness;
    if (!transition) return;

    if (state == PANEL_POWER_OFF) {
        if (!ensure_wake_overlay(ui)) return;
        lv_obj_set_style_bg_color(ui->wake_overlay, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(ui->wake_overlay, LV_OPA_COVER, 0);
        lv_obj_remove_flag(ui->wake_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui->wake_overlay);
        lv_obj_invalidate(ui->wake_overlay);
        lv_obj_invalidate(ui->screen);
        lv_refr_now(NULL);
        if (ui->callbacks.request_brightness != NULL)
            ui->callbacks.request_brightness(ui->callbacks.context, 0U);
    } else if (state == PANEL_POWER_DIMMED) {
        if (!ensure_wake_overlay(ui)) return;
        lv_obj_set_style_bg_opa(ui->wake_overlay, LV_OPA_TRANSP, 0);
        lv_obj_remove_flag(ui->wake_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui->wake_overlay);
        if (ui->callbacks.request_brightness != NULL)
            ui->callbacks.request_brightness(ui->callbacks.context, brightness);
    } else {
        if (ui->callbacks.request_brightness != NULL)
            ui->callbacks.request_brightness(ui->callbacks.context, brightness);
        refresh_active_dashboard(ui);
        if (ui->wake_overlay != NULL) lv_obj_add_flag(ui->wake_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(ui->screen);
        lv_refr_now(NULL);
    }

    ui->rendered_power = state;
    ui->rendered_brightness = brightness;
}

bool panel_ui_create(panel_ui_t **out, const panel_ui_config_t *config)
{
    if (out == NULL || *out != NULL || config == NULL || config->model == NULL) return false;
    panel_ui_t *ui = calloc(1, sizeof(*ui));
    if (ui == NULL) return false;
    ui->model = config->model;
    ui->callbacks = config->callbacks;
    ui->rendered_power = (panel_power_state_t)255;
    ui->rendered_brightness = UINT8_MAX;
    (void)snprintf(ui->clock_text, sizeof(ui->clock_text), "--:--");
    (void)snprintf(ui->date_text, sizeof(ui->date_text), "Tid ej synkroniserad");

    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(ui->screen, 0, 0);
    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0x071018), 0);

    lv_obj_t *top = lv_obj_create(ui->screen);
    lv_obj_set_pos(top, 0, 0);
    lv_obj_set_size(top, W, TOP_H);
    lv_obj_set_style_pad_all(top, 6, 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_shadow_width(top, 0, 0);
    ui->clock_label = label_new(top, ui->clock_text);
    lv_obj_align(ui->clock_label, LV_ALIGN_TOP_LEFT, 6, 0);
    ui->date_label = label_new(top, ui->date_text);
    lv_obj_align(ui->date_label, LV_ALIGN_BOTTOM_LEFT, 6, 0);
    ui->connection_label = label_new(top, "Okänd");
    lv_obj_set_width(ui->connection_label, 250);
    lv_obj_align(ui->connection_label, LV_ALIGN_RIGHT_MID, -64, 0);
    lv_obj_set_style_text_align(ui->connection_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *settings = hamburger_button_new(top, open_settings_event, ui);
    lv_obj_align(settings, LV_ALIGN_RIGHT_MID, 0, 0);

    ui->pager = lv_obj_create(ui->screen);
    lv_obj_set_pos(ui->pager, 0, TOP_H);
    lv_obj_set_size(ui->pager, W, PAGE_H);
    lv_obj_set_style_pad_all(ui->pager, 0, 0);
    lv_obj_set_style_bg_opa(ui->pager, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui->pager, 0, 0);
    lv_obj_set_style_shadow_width(ui->pager, 0, 0);
    lv_obj_remove_flag(ui->pager, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_scroll_dir(ui->pager, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(ui->pager, LV_SCROLL_SNAP_CENTER);
    lv_obj_add_flag(ui->pager, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_set_scrollbar_mode(ui->pager, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(ui->pager, LV_FLEX_FLOW_ROW);
    lv_obj_add_event_cb(ui->pager, activity_event, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(ui->pager, activity_event, LV_EVENT_SCROLL_BEGIN, ui);
    lv_obj_add_event_cb(ui->pager, patch018_swipe_gesture_event, LV_EVENT_SCROLL_BEGIN, ui);
    lv_obj_add_event_cb(ui->pager, pager_event, LV_EVENT_SCROLL_END, ui);
    lv_obj_add_event_cb(ui->pager, patch018_swipe_gesture_event, LV_EVENT_SCROLL_END, ui);

    for (size_t i = 0; i < PANEL_UI_PAGE_COUNT; ++i) {
        ui->pages[i] = lv_obj_create(ui->pager);
        lv_obj_set_size(ui->pages[i], W, PAGE_H);
        lv_obj_set_style_pad_all(ui->pages[i], 14, 0);
        lv_obj_set_style_bg_opa(ui->pages[i], LV_OPA_TRANSP, 0);
        lv_obj_add_flag(ui->pages[i], LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_remove_flag(ui->pages[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_set_layout(ui->pages[0], LV_LAYOUT_GRID);
    static int32_t cols[] = {CARD_W, CARD_W, LV_GRID_TEMPLATE_LAST};
    static int32_t rows[] = {CARD_H, CARD_H, CARD_H, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(ui->pages[0], cols, rows);
    lv_obj_set_style_pad_column(ui->pages[0], 10, 0);
    lv_obj_set_style_pad_row(ui->pages[0], 10, 0);
    for (size_t i = 0; i < PANEL_UI_WIDGET_COUNT; ++i) {
        lv_obj_t *card = create_read_only_card(ui, ui->pages[0], i);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, (int32_t)(i % 2U), 1,
            LV_GRID_ALIGN_STRETCH, (int32_t)(i / 2U), 1);
    }
    for (size_t i = 1; i < PANEL_UI_PAGE_COUNT; ++i) {
        char text[48];
        (void)snprintf(text, sizeof(text), "Sida %u\nInte konfigurerad", (unsigned)(i + 1U));
        lv_obj_t *placeholder = label_new(ui->pages[i], text);
        lv_obj_set_style_text_align(placeholder, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(placeholder);
    }

    lv_obj_t *indicator = lv_obj_create(ui->screen);
    lv_obj_set_pos(indicator, 0, TOP_H + PAGE_H);
    lv_obj_set_size(indicator, W, IND_H);
    lv_obj_set_style_bg_opa(indicator, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(indicator, 0, 0);
    lv_obj_set_style_shadow_width(indicator, 0, 0);
    lv_obj_set_flex_flow(indicator, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indicator, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(indicator, 10, 0);
    lv_obj_remove_flag(indicator, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(indicator, LV_DIR_NONE);
    for (size_t i = 0; i < PANEL_UI_PAGE_COUNT; ++i) {
        ui->dots[i] = lv_obj_create(indicator);
        lv_obj_set_size(ui->dots[i], i == 0U ? 10 : 8, i == 0U ? 10 : 8);
        lv_obj_set_style_radius(ui->dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(ui->dots[i], lv_color_hex(0xFFFFFF), 0);
        lv_obj_remove_flag(ui->dots[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(ui->dots[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(ui->dots[i], LV_DIR_NONE);
    }

    ui->settings_layer = lv_obj_create(ui->screen);
    lv_obj_set_size(ui->settings_layer, W, H);
    lv_obj_set_style_bg_color(ui->settings_layer, lv_color_hex(0x0D1B25), 0);
    lv_obj_set_style_bg_opa(ui->settings_layer, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(ui->settings_layer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ui->settings_layer, 12, 0);
    lv_obj_set_style_border_width(ui->settings_layer, 0, 0);
    lv_obj_set_style_shadow_width(ui->settings_layer, 0, 0);
    lv_obj_remove_flag(ui->settings_layer, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_event_cb(ui->settings_layer, activity_event, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(ui->settings_layer, activity_event, LV_EVENT_SCROLL_BEGIN, ui);
    lv_obj_add_event_cb(ui->settings_layer, patch021_settings_scroll_event, LV_EVENT_SCROLL_BEGIN, ui);
    lv_obj_add_event_cb(ui->settings_layer, patch021_settings_scroll_event, LV_EVENT_SCROLL_END, ui);
    label_new(ui->settings_layer, "Inställningar");
    ui->settings_feedback = label_new(ui->settings_layer, "");
    lv_obj_set_width(ui->settings_feedback, LV_PCT(100));
    lv_obj_set_height(ui->settings_feedback, 116);
    lv_label_set_long_mode(ui->settings_feedback, LV_LABEL_LONG_WRAP);
    settings_button_new(ui->settings_layer, "Normal ljusstyrka", normal_brightness_event, ui, &ui->normal_brightness_label);
    settings_button_new(ui->settings_layer, "Dämpad ljusstyrka", dim_brightness_event, ui, &ui->dim_brightness_label);
    settings_button_new(ui->settings_layer, "Dämpningstid", dim_timeout_event, ui, &ui->dim_timeout_label);
    settings_button_new(ui->settings_layer, "Släckningstid", off_timeout_event, ui, &ui->off_timeout_label);
    settings_button_new(ui->settings_layer, "Bakgrund", background_event, ui, &ui->background_label);
    settings_button_new(ui->settings_layer, "Ändra Wi-Fi", wifi_event, ui, NULL);
    ui->choose_homey_button = settings_button_new(
        ui->settings_layer, "Välj annan Homey", choose_event, ui, NULL);
    ui->wipe_homey_button = settings_button_new(
        ui->settings_layer, "Radera Homey-anslutning", wipe_event, ui, NULL);
    ui->change_account_button = settings_button_new(
        ui->settings_layer, "Byt Athom-konto", account_event, ui, NULL);
    settings_button_new(ui->settings_layer, "Stäng", close_settings_event, ui, NULL);
    lv_obj_add_flag(ui->settings_layer, LV_OBJ_FLAG_HIDDEN);

    ui->confirmation_layer = lv_obj_create(ui->screen);
    lv_obj_set_size(ui->confirmation_layer, 420, 300);
    lv_obj_center(ui->confirmation_layer);
    lv_obj_remove_flag(ui->confirmation_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui->confirmation_layer, lv_color_hex(0x102432), 0);
    lv_obj_set_style_bg_opa(ui->confirmation_layer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui->confirmation_layer, lv_color_hex(0xD8E8F2), 0);
    lv_obj_set_style_border_width(ui->confirmation_layer, 2, 0);
    lv_obj_set_style_radius(ui->confirmation_layer, 16, 0);
    lv_obj_set_style_pad_all(ui->confirmation_layer, 18, 0);
    lv_obj_set_flex_flow(ui->confirmation_layer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui->confirmation_layer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    ui->confirmation_text = label_new(ui->confirmation_layer, "");
    lv_obj_set_width(ui->confirmation_text, LV_PCT(100));
    lv_label_set_long_mode(ui->confirmation_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(ui->confirmation_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *button_row = lv_obj_create(ui->confirmation_layer);
    lv_obj_set_size(button_row, LV_PCT(100), 62);
    lv_obj_set_style_bg_opa(button_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(button_row, 0, 0);
    lv_obj_set_style_pad_all(button_row, 0, 0);
    lv_obj_remove_flag(button_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    modal_button_new(button_row, "Avbryt", cancel_event, ui);
    modal_button_new(button_row, "Fortsätt", accept_event, ui);
    lv_obj_add_flag(ui->confirmation_layer, LV_OBJ_FLAG_HIDDEN);

    ui->homey_info_layer = lv_obj_create(ui->screen);
    lv_obj_set_size(ui->homey_info_layer, 420, 300);
    lv_obj_center(ui->homey_info_layer);
    lv_obj_remove_flag(ui->homey_info_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui->homey_info_layer, lv_color_hex(0x102432), 0);
    lv_obj_set_style_bg_opa(ui->homey_info_layer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui->homey_info_layer, lv_color_hex(0xD8E8F2), 0);
    lv_obj_set_style_border_width(ui->homey_info_layer, 2, 0);
    lv_obj_set_style_radius(ui->homey_info_layer, 16, 0);
    lv_obj_set_style_pad_all(ui->homey_info_layer, 18, 0);
    lv_obj_set_flex_flow(ui->homey_info_layer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui->homey_info_layer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *homey_info_text = label_new(ui->homey_info_layer,
        "Välj annan Homey\n\nFunktionen aktiveras i nästa steg.\nIngen Homey-anslutning ändras nu.");
    lv_obj_set_width(homey_info_text, LV_PCT(100));
    lv_label_set_long_mode(homey_info_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(homey_info_text, LV_TEXT_ALIGN_CENTER, 0);
    modal_button_new(ui->homey_info_layer, "Stäng", homey_info_close_event, ui);
    lv_obj_add_flag(ui->homey_info_layer, LV_OBJ_FLAG_HIDDEN);

    panel_homey_controls_set_enabled(ui);
    *out = ui;
    return panel_ui_refresh(ui);
}

void panel_ui_destroy(panel_ui_t **ptr)
{
    if (ptr == NULL || *ptr == NULL) return;
    panel_ui_t *ui = *ptr;
    ui->destroying = true;
    if (ui->screen != NULL) lv_obj_delete(ui->screen);
    free(ui);
    *ptr = NULL;
}

bool panel_ui_activate(panel_ui_t *ui)
{
    if (ui == NULL || ui->screen == NULL) return false;
    lv_screen_load(ui->screen);
    ui->active = true;
    return true;
}

bool panel_ui_is_active(const panel_ui_t *ui)
{
    return ui != NULL && ui->active && ui->screen == lv_screen_active();
}

bool panel_ui_refresh(panel_ui_t *ui)
{
    if (ui == NULL || ui->model == NULL) return false;

#ifdef ESP_PLATFORM
    const uint64_t refresh_start_us = (uint64_t)esp_timer_get_time();
    const bool pager_scrolling_start =
        ui->pager != NULL && lv_obj_is_scrolling(ui->pager);
    const bool settings_scrolling_start =
        ui->settings_layer != NULL && lv_obj_is_scrolling(ui->settings_layer);
    const uint64_t favorites_start_us = refresh_start_us;
#endif

    /* Patch 017 v4 render-boundary ownership guard. Favorite Devices are the
     * sole authority for widgets 4/5. Re-assert their published state before
     * any full LVGL refresh so unrelated dashboard/touch/view activity cannot
     * render a downgraded AVAILABLE-without-boolean state. */
    const bool favorites_changed = ui->homey_data_ready
        ? panel_homey_favorites_apply_ui_model(ui->model)
        : false;

#ifdef ESP_PLATFORM
    const uint64_t render_start_us = (uint64_t)esp_timer_get_time();
    const bool power_transition =
        ui->rendered_power != ui->model->power_state;
#endif

    for (size_t i = 0; i < PANEL_UI_WIDGET_COUNT; ++i) {
        const char *title = panel_ui_widget_title_for_model(ui->model, i);
        lv_label_set_text(ui->widget_title[i], title != NULL ? title : "");
        char status_text[32];
        if (!panel_ui_widget_display_text(
                ui->model, i, status_text, sizeof(status_text))) {
            (void)snprintf(status_text, sizeof(status_text), "%s", "Okänd");
        }
        lv_label_set_text(ui->widget_status[i], status_text);
    }
    lv_label_set_text(ui->clock_label, ui->clock_text);
    lv_label_set_text(ui->date_label, ui->date_text);
    render_connection(ui);
    render_settings(ui);
    render_view(ui);
    render_power(ui);

#ifdef ESP_PLATFORM
    const uint64_t page_start_us = (uint64_t)esp_timer_get_time();
    /* Preserve the user-owned pager offset while LVGL is settling a swipe. */
    const bool page_reassert = !pager_scrolling_start;
#else
    const bool page_reassert = true;
#endif
    bool page_selected = true;
    if (page_reassert) {
        page_selected = panel_ui_select_page(ui, ui->model->active_page, false);
    }
#ifdef ESP_PLATFORM
    const uint64_t refresh_end_us = (uint64_t)esp_timer_get_time();
    const bool pager_scrolling_end =
        ui->pager != NULL && lv_obj_is_scrolling(ui->pager);
    const bool settings_scrolling_end =
        ui->settings_layer != NULL && lv_obj_is_scrolling(ui->settings_layer);
    ESP_LOGI(
        "panel_ui",
        "PATCH024_RENDER_PATH phase=panel_refresh elapsed_us=%llu "
        "favorites_us=%llu render_us=%llu page_us=%llu "
        "pager_scrolling_start=%s pager_scrolling_end=%s "
        "settings_scrolling_start=%s settings_scrolling_end=%s "
        "favorites_changed=%s page_reassert=%s page_selected=%s "
        "power_transition=%s privacy=sanitized",
        (unsigned long long)(refresh_end_us - refresh_start_us),
        (unsigned long long)(render_start_us - favorites_start_us),
        (unsigned long long)(page_start_us - render_start_us),
        (unsigned long long)(refresh_end_us - page_start_us),
        pager_scrolling_start ? "true" : "false",
        pager_scrolling_end ? "true" : "false",
        settings_scrolling_start ? "true" : "false",
        settings_scrolling_end ? "true" : "false",
        favorites_changed ? "true" : "false",
        page_reassert ? "true" : "false",
        page_selected ? "true" : "false",
        power_transition ? "true" : "false");
#else
    (void)favorites_changed;
#endif
    return page_selected;
}

bool panel_ui_set_time(panel_ui_t *ui, const struct tm *time, bool valid)
{
    if (ui == NULL) return false;
    bool a = panel_ui_format_clock(valid, time, ui->clock_text, sizeof(ui->clock_text));
    bool b = panel_ui_format_date_sv(valid, time, ui->date_text, sizeof(ui->date_text));
    lv_label_set_text(ui->clock_label, ui->clock_text);
    lv_label_set_text(ui->date_label, ui->date_text);
    return a && b;
}

bool panel_ui_set_connection(panel_ui_t *ui, const panel_ui_connection_info_t *connection)
{
    if (ui == NULL || connection == NULL) return false;
    ui->connection = *connection;
    ui->connection.display_name[sizeof(ui->connection.display_name) - 1U] = '\0';
    render_connection(ui);
    return true;
}

bool panel_ui_set_homey_data_ready(panel_ui_t *ui, bool ready)
{
    if (ui == NULL) return false;
    ui->homey_data_ready = ready;
    panel_homey_controls_set_enabled(ui);
    if (!ready && ui->model->view == PANEL_UI_VIEW_CONFIRMATION) {
        panel_ui_cancel_confirmation(ui->model);
        render_view(ui);
    }
    return true;
}

bool panel_ui_select_page(panel_ui_t *ui, uint8_t page, bool animate)
{
    if (ui == NULL) return false;
    uint8_t normalized = panel_ui_set_active_page(ui->model, page);
    lv_obj_scroll_to_x(ui->pager, (int32_t)normalized * W,
        animate ? LV_ANIM_ON : LV_ANIM_OFF);
    dots_render(ui);
    return normalized == page;
}

bool panel_ui_open_settings(panel_ui_t *ui)
{
    if (ui == NULL) return false;
    panel_ui_set_view(ui->model, PANEL_UI_VIEW_SETTINGS);
    lv_label_set_text(ui->settings_feedback, "");
    render_settings(ui);
    render_view(ui);
    return true;
}

bool panel_ui_close_settings(panel_ui_t *ui)
{
    if (ui == NULL) return false;
    panel_ui_set_view(ui->model, PANEL_UI_VIEW_DASHBOARD);
    render_view(ui);
    return true;
}

bool panel_ui_update_inactivity(panel_ui_t *ui, uint64_t now_ms)
{
    if (ui == NULL) return false;
    panel_power_state_t before = ui->model->power_state;
    panel_ui_tick(ui->model, now_ms);
    render_power(ui);
    return before != ui->model->power_state;
}

bool panel_ui_process_touch(panel_ui_t *ui, uint64_t now_ms)
{
    if (ui == NULL) return false;
    bool consumed = panel_ui_handle_touch(ui->model, now_ms);
    render_power(ui);
    return consumed;
}

bool panel_ui_set_wifi_reconfigure_result(
    panel_ui_t *ui, panel_ui_wifi_reconfigure_result_t result)
{
    if (ui == NULL || ui->settings_feedback == NULL) return false;
    const char *text = "Wi-Fi-inställningen kunde inte öppnas.";
    if (result == PANEL_UI_WIFI_RECONFIGURE_OPENED) {
        text = "Wi-Fi-inställning öppnad\n\nAnslut till panelens Wi-Fi-nätverk\noch öppna 192.168.4.1.";
    } else if (result == PANEL_UI_WIFI_RECONFIGURE_BLOCKED) {
        text = "Wi-Fi-inställningen är redan öppen eller kan inte startas i nuvarande läge.";
    }
    lv_label_set_text(ui->settings_feedback, text);
    return true;
}

bool panel_ui_reset_view(panel_ui_t *ui)
{
    if (ui == NULL) return false;
    panel_ui_set_view(ui->model, PANEL_UI_VIEW_DASHBOARD);
    return panel_ui_refresh(ui);
}
