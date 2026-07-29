#include "panel_ui.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 480
#define H 480
#define TOP_H 56
#define PAGE_H 396
#define IND_H 28
#define CARD_W 221
#define CARD_H 116

struct panel_ui {
    panel_ui_model_t *model;
    panel_ui_callbacks_t callbacks;
    lv_obj_t *screen;
    lv_obj_t *clock_label;
    lv_obj_t *date_label;
    lv_obj_t *connection_label;
    lv_obj_t *pager;
    lv_obj_t *pages[PANEL_UI_PAGE_COUNT];
    lv_obj_t *widget_status[PANEL_UI_WIDGET_COUNT];
    lv_obj_t *dots[PANEL_UI_PAGE_COUNT];
    lv_obj_t *settings_layer;
    lv_obj_t *settings_text;
    lv_obj_t *confirmation_layer;
    lv_obj_t *confirmation_text;
    lv_obj_t *wake_overlay;
    char clock_text[PANEL_UI_TIME_TEXT_MAX];
    char date_text[PANEL_UI_DATE_TEXT_MAX];
    panel_ui_connection_info_t connection;
    panel_power_state_t rendered_power;
    bool active;
    bool destroying;
};

static void render_view(panel_ui_t *ui);
static void render_settings(panel_ui_t *ui);
static void render_power(panel_ui_t *ui);

static lv_obj_t *label_new(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text != NULL ? text : "");
    lv_obj_set_style_text_color(label, lv_color_hex(0xF4F7FA), 0);
    return label;
}

static lv_obj_t *button_new(lv_obj_t *parent, const char *text,
                            lv_event_cb_t callback, panel_ui_t *ui)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_height(button, 44);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, ui);
    lv_obj_t *label = label_new(button, text);
    lv_obj_center(label);
    return button;
}

static lv_obj_t *create_read_only_card(panel_ui_t *ui, lv_obj_t *parent, size_t index)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(card, CARD_W, CARD_H);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x162735), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_90, 0);
    lv_obj_t *title = label_new(card, panel_ui_widget_title(index));
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 4);
    ui->widget_status[index] =
        label_new(card, panel_ui_widget_status_text(ui->model->widget_status[index]));
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
    unsigned v = (unsigned)ui->model->settings.normal_brightness + 5U;
    ui->model->settings.normal_brightness = (uint8_t)(v > 100U ? 10U : v);
    notify_settings(ui);
}

static void dim_brightness_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    unsigned v = (unsigned)ui->model->settings.dimmed_brightness + 5U;
    ui->model->settings.dimmed_brightness = (uint8_t)(v > 100U ? 0U : v);
    notify_settings(ui);
}

static void dim_timeout_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    uint32_t v = ui->model->settings.dim_after_seconds;
    ui->model->settings.dim_after_seconds = v >= 600U ? 10U : v + 10U;
    notify_settings(ui);
}

static void off_timeout_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    uint32_t v = ui->model->settings.off_after_seconds;
    ui->model->settings.off_after_seconds =
        v == PANEL_UI_OFF_DISABLED ? 300U :
        (v >= 1800U ? PANEL_UI_OFF_DISABLED : v + 300U);
    notify_settings(ui);
}

static void wake_toggle_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    ui->model->settings.wake_on_touch = !ui->model->settings.wake_on_touch;
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
    if (ui != NULL && ui->callbacks.request_wifi_reconfigure != NULL)
        ui->callbacks.request_wifi_reconfigure(ui->callbacks.context);
}

static void choose_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    lv_label_set_text(ui->settings_text, "Funktionen aktiveras i nästa steg");
    if (ui->callbacks.request_choose_homey != NULL)
        ui->callbacks.request_choose_homey(ui->callbacks.context);
}

static void wipe_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
    panel_ui_request_confirmation(ui->model, PANEL_CONFIRM_HOMEY_WIPE);
    render_view(ui);
}

static void account_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL) return;
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
    if (ui == NULL) return;
    panel_confirmed_action_t action = panel_ui_accept_confirmation(ui->model);
    render_view(ui);
    lv_label_set_text(ui->settings_text, "Funktionen aktiveras i nästa steg");
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
    char text[192];
    (void)snprintf(text, sizeof(text),
        "Normal: %u%%  Dämpad: %u%%\nDämpning: %lus  Släckning: %s\nWake touch: %s  Bakgrund: %s",
        (unsigned)ui->model->settings.normal_brightness,
        (unsigned)ui->model->settings.dimmed_brightness,
        (unsigned long)ui->model->settings.dim_after_seconds,
        ui->model->settings.off_after_seconds == PANEL_UI_OFF_DISABLED ? "Aldrig" : "Aktiv",
        ui->model->settings.wake_on_touch ? "På" : "Av",
        ui->model->settings.background_mode == PANEL_BACKGROUND_BUILT_IN ? "Inbyggd" : "Av");
    lv_label_set_text(ui->settings_text, text);
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
            ? "Radera Homey-anslutningen?\nWi-Fi bevaras."
            : "Byt Athom-konto?\nBefintlig anslutning behöver ersättas.");
    } else lv_obj_add_flag(ui->confirmation_layer, LV_OBJ_FLAG_HIDDEN);
}

static void render_power(panel_ui_t *ui)
{
    panel_power_state_t state = ui->model->power_state;
    uint8_t brightness = ui->model->settings.normal_brightness;
    if (state == PANEL_POWER_DIMMED) brightness = ui->model->settings.dimmed_brightness;
    else if (state == PANEL_POWER_OFF) brightness = 0U;
    if (state != ui->rendered_power && ui->callbacks.request_brightness != NULL) {
        ui->callbacks.request_brightness(ui->callbacks.context, brightness);
        ui->rendered_power = state;
    }
    if (state == PANEL_POWER_ACTIVE) lv_obj_add_flag(ui->wake_overlay, LV_OBJ_FLAG_HIDDEN);
    else {
        lv_obj_remove_flag(ui->wake_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui->wake_overlay);
    }
}

bool panel_ui_create(panel_ui_t **out, const panel_ui_config_t *config)
{
    if (out == NULL || *out != NULL || config == NULL || config->model == NULL) return false;
    panel_ui_t *ui = calloc(1, sizeof(*ui));
    if (ui == NULL) return false;
    ui->model = config->model;
    ui->callbacks = config->callbacks;
    ui->rendered_power = (panel_power_state_t)255;
    (void)snprintf(ui->clock_text, sizeof(ui->clock_text), "--:--");
    (void)snprintf(ui->date_text, sizeof(ui->date_text), "Tid ej synkroniserad");

    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(ui->screen, 0, 0);
    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0x071018), 0);

    lv_obj_t *top = lv_obj_create(ui->screen);
    lv_obj_set_pos(top, 0, 0);
    lv_obj_set_size(top, W, TOP_H);
    lv_obj_set_style_pad_all(top, 8, 0);
    ui->clock_label = label_new(top, ui->clock_text);
    lv_obj_align(ui->clock_label, LV_ALIGN_TOP_LEFT, 6, 0);
    ui->date_label = label_new(top, ui->date_text);
    lv_obj_align(ui->date_label, LV_ALIGN_BOTTOM_LEFT, 6, 0);
    ui->connection_label = label_new(top, "Okänd");
    lv_obj_set_width(ui->connection_label, 270);
    lv_obj_align(ui->connection_label, LV_ALIGN_CENTER, 55, 0);
    lv_obj_set_style_text_align(ui->connection_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *settings = button_new(top, LV_SYMBOL_SETTINGS, open_settings_event, ui);
    lv_obj_set_size(settings, 44, 44);
    lv_obj_align(settings, LV_ALIGN_RIGHT_MID, 0, 0);

    ui->pager = lv_obj_create(ui->screen);
    lv_obj_set_pos(ui->pager, 0, TOP_H);
    lv_obj_set_size(ui->pager, W, PAGE_H);
    lv_obj_set_style_pad_all(ui->pager, 0, 0);
    lv_obj_set_scroll_dir(ui->pager, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(ui->pager, LV_SCROLL_SNAP_CENTER);
    lv_obj_add_flag(ui->pager, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_set_scrollbar_mode(ui->pager, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(ui->pager, LV_FLEX_FLOW_ROW);
    lv_obj_add_event_cb(ui->pager, pager_event, LV_EVENT_SCROLL_END, ui);

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
    lv_obj_set_flex_flow(indicator, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indicator, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(indicator, 10, 0);
    for (size_t i = 0; i < PANEL_UI_PAGE_COUNT; ++i) {
        ui->dots[i] = lv_obj_create(indicator);
        lv_obj_set_size(ui->dots[i], i == 0U ? 10 : 8, i == 0U ? 10 : 8);
        lv_obj_set_style_radius(ui->dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(ui->dots[i], lv_color_hex(0xFFFFFF), 0);
    }

    ui->settings_layer = lv_obj_create(ui->screen);
    lv_obj_set_size(ui->settings_layer, W, H);
    lv_obj_set_style_bg_color(ui->settings_layer, lv_color_hex(0x0D1B25), 0);
    lv_obj_set_style_bg_opa(ui->settings_layer, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(ui->settings_layer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ui->settings_layer, 12, 0);
    label_new(ui->settings_layer, "Inställningar");
    ui->settings_text = label_new(ui->settings_layer, "");
    button_new(ui->settings_layer, "Normal ljusstyrka", normal_brightness_event, ui);
    button_new(ui->settings_layer, "Dämpad ljusstyrka", dim_brightness_event, ui);
    button_new(ui->settings_layer, "Dämpningstid", dim_timeout_event, ui);
    button_new(ui->settings_layer, "Släckningstid", off_timeout_event, ui);
    button_new(ui->settings_layer, "Wake-on-touch", wake_toggle_event, ui);
    button_new(ui->settings_layer, "Bakgrund Av/Inbyggd", background_event, ui);
    button_new(ui->settings_layer, "Ändra Wi-Fi", wifi_event, ui);
    button_new(ui->settings_layer, "Välj annan Homey", choose_event, ui);
    button_new(ui->settings_layer, "Radera Homey-anslutning", wipe_event, ui);
    button_new(ui->settings_layer, "Byt Athom-konto", account_event, ui);
    button_new(ui->settings_layer, "Stäng", close_settings_event, ui);
    lv_obj_add_flag(ui->settings_layer, LV_OBJ_FLAG_HIDDEN);

    ui->confirmation_layer = lv_obj_create(ui->screen);
    lv_obj_set_size(ui->confirmation_layer, 400, 240);
    lv_obj_center(ui->confirmation_layer);
    ui->confirmation_text = label_new(ui->confirmation_layer, "");
    lv_obj_set_width(ui->confirmation_text, 340);
    lv_obj_align(ui->confirmation_text, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_t *cancel = button_new(ui->confirmation_layer, "Avbryt", cancel_event, ui);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_t *accept = button_new(ui->confirmation_layer, "Fortsätt", accept_event, ui);
    lv_obj_align(accept, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_flag(ui->confirmation_layer, LV_OBJ_FLAG_HIDDEN);

    ui->wake_overlay = lv_obj_create(ui->screen);
    lv_obj_set_size(ui->wake_overlay, W, H);
    lv_obj_set_style_bg_opa(ui->wake_overlay, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(ui->wake_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->wake_overlay, wake_event, LV_EVENT_PRESSED, ui);
    lv_obj_add_flag(ui->wake_overlay, LV_OBJ_FLAG_HIDDEN);
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
    for (size_t i = 0; i < PANEL_UI_WIDGET_COUNT; ++i)
        lv_label_set_text(ui->widget_status[i],
            panel_ui_widget_status_text(ui->model->widget_status[i]));
    lv_label_set_text(ui->clock_label, ui->clock_text);
    lv_label_set_text(ui->date_label, ui->date_text);
    render_connection(ui);
    render_settings(ui);
    render_view(ui);
    render_power(ui);
    return panel_ui_select_page(ui, ui->model->active_page, false);
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

bool panel_ui_reset_view(panel_ui_t *ui)
{
    if (ui == NULL) return false;
    panel_ui_set_view(ui->model, PANEL_UI_VIEW_DASHBOARD);
    return panel_ui_refresh(ui);
}
