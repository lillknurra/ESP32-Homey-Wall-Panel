#include "panel_ui.h"
#include "panel_ui_layout.h"
#include "panel_homey_favorites.h"
#include "panel_roller_model.h"
#include "homey_panel_font_22.h"
#include "lvgl.h"
#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_timer.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PANEL_UI_QR_UPDATE
#define PANEL_UI_QR_UPDATE(qr, payload, length) lv_qrcode_update((qr), (payload), (length))
#endif

#define W 480
#define H 480
#define TOP_H 60
#define NAV_BUTTON_W 52
#define NAV_BUTTON_H 48
#define NAV_MARGIN 6
#define NAV_ICON_W 26
#define NAV_ICON_H 21
#define PAGE_H 392
#define IND_H 28
#define ROLLER_CARD_W PANEL_UI_FAVORITE_CARD_WIDTH
#define ROLLER_CARD_H PANEL_UI_FAVORITE_CARD_HEIGHT
#define LIGHT_CARD_W PANEL_UI_FAVORITE_CARD_WIDTH
#define LIGHT_CARD_H PANEL_UI_FAVORITE_CARD_HEIGHT
#define PATCH021_SETTINGS_DIRECT_CHILD_COUNT 2U
#define THEME_LABEL_LIMIT 32U

typedef enum {
    THEME_TEXT_PRIMARY = 0,
    THEME_TEXT_ACCENT,
} theme_text_role_t;

typedef struct {
    lv_obj_t *label;
    theme_text_role_t role;
} theme_label_t;

typedef struct {
    uint32_t screen;
    uint32_t card;
    uint32_t settings;
    uint32_t dialog;
    uint32_t primary_text;
    uint32_t accent_text;
    uint32_t track;
    uint32_t roller_fill;
    uint32_t border;
    uint32_t card_border;
    uint32_t light_icon_border;
    uint32_t confirmation_border;
    uint32_t disabled_symbol;
    uint32_t roller_arrow;
    uint32_t disabled_control;
    uint32_t disabled_control_border;
} theme_palette_t;

static const lv_point_precise_t ROLLER_ARROW_UP_POINTS[] = {
    {0, 11}, {10, 0}, {19, 11},
};
static const lv_point_precise_t ROLLER_ARROW_DOWN_POINTS[] = {
    {0, 0}, {10, 11}, {19, 0},
};
/* Keep the disabled awning_1 glyphs at their pre-Patch093 geometry. */
static const lv_point_precise_t DISABLED_MARKIS_ARROW_UP_POINTS[] = {
    {0, 7}, {7, 0}, {14, 7},
};
static const lv_point_precise_t DISABLED_MARKIS_ARROW_DOWN_POINTS[] = {
    {0, 0}, {7, 7}, {14, 0},
};

typedef enum { MODAL_NONE, MODAL_HELP, MODAL_ERROR, MODAL_WIFI, MODAL_INFORMATION } modal_kind_t;

typedef struct {
    panel_ui_t *ui;
    uint8_t widget_index;
    bool lower_half;
} roller_touch_context_t;

struct panel_ui {
    panel_ui_model_t *model;
    panel_ui_callbacks_t callbacks;
    lv_obj_t *screen;
    lv_obj_t *clock_label;
    lv_obj_t *connection_label;
    lv_obj_t *pager;
    lv_obj_t *pages[PANEL_UI_PAGE_COUNT];
    lv_obj_t *widget_card[PANEL_UI_WIDGET_COUNT];
    lv_obj_t *widget_title[PANEL_UI_WIDGET_COUNT];
    lv_obj_t *widget_status[PANEL_UI_WIDGET_COUNT];
    lv_obj_t *widget_icon[PANEL_UI_WIDGET_COUNT];
    lv_obj_t *widget_icon_base[PANEL_UI_WIDGET_COUNT];
    lv_obj_t *roller_lines[3][4];
    lv_obj_t *roller_fills[3][4];
    lv_obj_t *roller_arrows[3][2];
    lv_obj_t *roller_arrow_highlights[3][2];
    lv_obj_t *disabled_markis_controls[3];
    lv_obj_t *disabled_markis_symbols[3];
    lv_obj_t *disabled_markis_stop_marks[3];
    lv_obj_t *disabled_markis_captions[3];
    roller_touch_context_t roller_touch[3][2];
    lv_timer_t *roller_feedback_timer;
    uint8_t roller_feedback_widget;
    bool roller_feedback_lower_half;
    bool roller_feedback_active;
    bool light_toggle_pending[PANEL_HOMEY_FAVORITE_LIMIT];
    lv_obj_t *dots[PANEL_UI_PAGE_COUNT];
    lv_obj_t *settings_layer;
    lv_obj_t *settings_header;
    lv_obj_t *settings_header_title;
    lv_obj_t *settings_back_button;
    lv_obj_t *settings_content;
    lv_obj_t *settings_main_body;
    lv_obj_t *settings_advanced_body;
    lv_obj_t *status_overlay;
    lv_obj_t *status_dialog;
    lv_obj_t *status_title;
    lv_obj_t *status_body;
    lv_obj_t *status_text;
    lv_obj_t *status_close;
    lv_obj_t *status_close_footer;
    lv_obj_t *status_error_icon;
    lv_obj_t *information_refresh;
    panel_information_kind_t information_kind;
    lv_obj_t *wifi_ssid;
    lv_obj_t *wifi_password;
    lv_obj_t *wifi_cancel;
    lv_obj_t *wifi_qr;
    lv_obj_t *wifi_qr_original_parent;
    int32_t wifi_qr_original_x;
    int32_t wifi_qr_original_y;
    int32_t wifi_qr_original_width;
    int32_t wifi_qr_original_height;
    bool wifi_qr_original_hidden;
    bool wifi_cancel_allowed;
    modal_kind_t modal_kind;
    bool save_pending;
    bool deferred_settings_error;
    lv_obj_t *homey_connection_label;
    lv_obj_t *normal_brightness_label;
    lv_obj_t *dim_brightness_label;
    lv_obj_t *dim_timeout_label;
    lv_obj_t *off_timeout_label;
    lv_obj_t *background_label;
    theme_label_t theme_labels[THEME_LABEL_LIMIT];
    size_t theme_label_count;
    uint8_t rendered_theme;
    lv_obj_t *choose_homey_button;
    lv_obj_t *wipe_homey_button;
    lv_obj_t *change_account_button;
    lv_obj_t *confirmation_layer;
    lv_obj_t *confirmation_text;
    lv_obj_t *wake_overlay;
    char clock_text[PANEL_UI_TIME_TEXT_MAX];
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

/* Presentation checks run under the caller's existing LVGL display lock.
 * Authoritative model/readiness updates are never delayed or cached here. */
static void set_label_text_if_changed(lv_obj_t *label, const char *text)
{
    const char *next = text != NULL ? text : "";
    const char *current = lv_label_get_text(label);
    if (current == NULL || strcmp(current, next) != 0) lv_label_set_text(label, next);
}

static void status_modal_show(panel_ui_t *ui, modal_kind_t kind,
                              const char *title, const char *text);
static void status_modal_create(panel_ui_t *ui);
static void status_modal_foreground(panel_ui_t *ui);
static void patch018_clear_gesture(panel_ui_t *ui);
static void panel_ui_flush_deferred_save_error(panel_ui_t *ui);
static void roller_feedback_cancel(panel_ui_t *ui);
static void roller_feedback_set_arrow(panel_ui_t *ui, bool highlighted);

#define ROLLER_TOUCH_FEEDBACK_MS 550U
#define ROLLER_ARROW_HIGHLIGHT_W 38
#define ROLLER_ARROW_HIGHLIGHT_H 30

static const theme_palette_t DARK_PALETTE = {
    .screen = 0x071018, .card = 0x162735, .settings = 0x0D1B25,
    .dialog = 0x102432, .primary_text = 0xF4F7FA,
    .accent_text = 0x8BD3F7, .track = 0x435562, .roller_fill = 0x8BD3F7,
    .border = 0x435562,
    .card_border = 0xE0E0E0, .light_icon_border = 0xF4F7FA,
    .confirmation_border = 0xD8E8F2, .disabled_symbol = 0xD9E2E8,
    .roller_arrow = 0x80919E, .disabled_control = 0x203746,
    .disabled_control_border = 0x506575,
};
static const theme_palette_t LIGHT_PALETTE = {
    .screen = 0xF1F4F6, .card = 0xFFFFFF, .settings = 0xE9EEF1,
    .dialog = 0xFFFFFF, .primary_text = 0x111820,
    .accent_text = 0x005B80, .track = 0xC4CDD3, .roller_fill = 0x2388B3,
    .border = 0x263746,
    .card_border = 0x263746, .light_icon_border = 0x263746,
    .confirmation_border = 0x263746, .disabled_symbol = 0x111820,
    .roller_arrow = 0x52616B, .disabled_control = 0xE2E8EC,
    .disabled_control_border = 0x63717A,
};

static void set_bg_color_if_changed(lv_obj_t *obj, lv_color_t color)
{
    if (obj == NULL) return;
    if (!lv_color_eq(lv_obj_get_style_bg_color(obj, LV_PART_MAIN), color)) {
        lv_obj_set_style_bg_color(obj, color, 0);
    }
}

static void theme_label_register(panel_ui_t *ui, lv_obj_t *label, theme_text_role_t role)
{
    if (ui != NULL && label != NULL && ui->theme_label_count < THEME_LABEL_LIMIT) {
        ui->theme_labels[ui->theme_label_count++] = (theme_label_t){ label, role };
    }
}

static void theme_apply(panel_ui_t *ui)
{
    if (ui == NULL || ui->screen == NULL) return;
    const uint8_t mode = (uint8_t)ui->model->settings.background_mode;
    if (ui->rendered_theme == mode) return;
    const bool light = mode == (uint8_t)PANEL_THEME_LIGHT;
    const theme_palette_t *palette = light ? &LIGHT_PALETTE : &DARK_PALETTE;

    set_bg_color_if_changed(ui->screen, lv_color_hex(palette->screen));
    if (lv_obj_get_style_bg_grad_dir(ui->screen, 0) != LV_GRAD_DIR_NONE)
        lv_obj_set_style_bg_grad_dir(ui->screen, LV_GRAD_DIR_NONE, 0);
    set_bg_color_if_changed(ui->settings_layer, lv_color_hex(palette->settings));
    set_bg_color_if_changed(ui->status_dialog, lv_color_hex(palette->dialog));
    set_bg_color_if_changed(ui->confirmation_layer, lv_color_hex(palette->dialog));
    if (ui->status_dialog != NULL) {
        lv_obj_set_style_border_color(ui->status_dialog, lv_color_hex(palette->border), 0);
        lv_obj_set_style_border_width(ui->status_dialog, light ? 2 : 0, 0);
    }
    if (ui->confirmation_layer != NULL)
        lv_obj_set_style_border_color(ui->confirmation_layer,
                                      lv_color_hex(palette->confirmation_border), 0);

    for (size_t index = 0U; index < PANEL_UI_WIDGET_COUNT; ++index) {
        lv_obj_t *card = ui->widget_card[index];
        if (card != NULL) {
            set_bg_color_if_changed(card, lv_color_hex(palette->card));
            lv_obj_set_style_border_color(card, lv_color_hex(palette->card_border), 0);
        }
        if (index < 3U) {
            for (size_t line = 0U; line < 4U; ++line) {
                set_bg_color_if_changed(ui->roller_lines[index][line],
                                        lv_color_hex(palette->track));
                set_bg_color_if_changed(ui->roller_fills[index][line],
                                        lv_color_hex(palette->roller_fill));
            }
        } else if (index == 4U || index == 5U) {
            if (ui->widget_icon[index] != NULL)
                lv_obj_set_style_border_color(ui->widget_icon[index],
                                               lv_color_hex(palette->light_icon_border), 0);
        }
    }
    for (size_t index = 0U; index < 3U; ++index) {
        if (ui->roller_arrows[index][0] != NULL)
            lv_obj_set_style_line_color(ui->roller_arrows[index][0],
                                        lv_color_hex(palette->roller_arrow), 0);
        if (ui->roller_arrows[index][1] != NULL)
            lv_obj_set_style_line_color(ui->roller_arrows[index][1],
                                        lv_color_hex(palette->roller_arrow), 0);
        if (ui->disabled_markis_controls[index] != NULL) {
            set_bg_color_if_changed(ui->disabled_markis_controls[index],
                                    lv_color_hex(palette->disabled_control));
            lv_obj_set_style_border_color(ui->disabled_markis_controls[index],
                                           lv_color_hex(palette->disabled_control_border), 0);
        }
        if (ui->disabled_markis_symbols[index] != NULL) {
            lv_obj_set_style_line_color(ui->disabled_markis_symbols[index],
                                         lv_color_hex(palette->disabled_symbol), 0);
            lv_obj_set_style_border_color(ui->disabled_markis_symbols[index],
                                           lv_color_hex(palette->disabled_symbol), 0);
        }
        if (ui->disabled_markis_stop_marks[index] != NULL)
            set_bg_color_if_changed(ui->disabled_markis_stop_marks[index],
                                    lv_color_hex(palette->disabled_symbol));
        if (ui->disabled_markis_captions[index] != NULL)
            lv_obj_set_style_text_color(ui->disabled_markis_captions[index],
                                         lv_color_hex(palette->disabled_symbol), 0);
    }
    for (size_t index = 0U; index < PANEL_UI_PAGE_COUNT; ++index) {
        if (ui->dots[index] != NULL)
            set_bg_color_if_changed(ui->dots[index], lv_color_hex(light ? 0x263746 : 0xFFFFFF));
    }
    for (size_t index = 0U; index < ui->theme_label_count; ++index) {
        const theme_label_t *entry = &ui->theme_labels[index];
        if (entry->label == NULL) continue;
        const uint32_t color = entry->role == THEME_TEXT_ACCENT
            ? palette->accent_text : palette->primary_text;
        lv_obj_set_style_text_color(entry->label, lv_color_hex(color), 0);
    }
    ui->rendered_theme = mode;
    if (ui->roller_feedback_active) {
        roller_feedback_set_arrow(ui, true);
    }
}

static void render_view(panel_ui_t *ui);
static void render_settings(panel_ui_t *ui);
static void render_power(panel_ui_t *ui);

static bool panel_homey_actions_allowed(const panel_ui_t *ui)
{
    return ui != NULL && ui->homey_data_ready && ui->modal_kind == MODAL_NONE;
}

static bool panel_ui_render_status_text(
    const panel_ui_t *ui,
    size_t widget_index,
    char *buffer,
    size_t buffer_size)
{
    if (ui == NULL || widget_index >= PANEL_UI_WIDGET_COUNT ||
        buffer == NULL || buffer_size == 0U) {
        return false;
    }

    if (widget_index >= 4U &&
        widget_index < 4U + PANEL_HOMEY_FAVORITE_LIMIT &&
        ui->light_toggle_pending[widget_index - 4U]) {
        int written = snprintf(buffer, buffer_size, "%s", "Väntar...");
        return written >= 0 && (size_t)written < buffer_size;
    }

    /* PATCH028A: keep the model's default UNCONFIGURED state hidden before readiness. */
    if (!ui->homey_data_ready) {
        int written = snprintf(buffer, buffer_size, "%s", "Okänd");
        return written >= 0 && (size_t)written < buffer_size;
    }

    return panel_ui_widget_display_text(ui->model, widget_index, buffer, buffer_size);
}

static void render_roller_widget(panel_ui_t *ui, size_t index)
{
    if (ui == NULL || index >= 3U || ui->widget_status[index] == NULL) return;
    if (index == 0U) {
        for (size_t line = 0U; line < 4U; ++line) {
            if (ui->roller_lines[index][line] != NULL)
                lv_obj_add_flag(ui->roller_lines[index][line], LV_OBJ_FLAG_HIDDEN);
            if (ui->roller_fills[index][line] != NULL)
                lv_obj_add_flag(ui->roller_fills[index][line], LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_add_flag(ui->widget_status[index], LV_OBJ_FLAG_HIDDEN);
        return;
    }
    const panel_widget_status_t status = ui->model->widget_status[index];
    const bool matched = status == PANEL_WIDGET_AVAILABLE || status == PANEL_WIDGET_UNAVAILABLE;
    const panel_roller_display_state_t display = panel_roller_display_state(
        matched,
        ui->homey_data_ready && !ui->model->homey_snapshot_stale,
        matched,
        status == PANEL_WIDGET_AVAILABLE,
        ui->model->roller_position_valid[index],
        ui->model->roller_position_step[index]);

    for (size_t line = 0U; line < 4U; ++line) {
        if (ui->roller_lines[index][line] != NULL) {
            if (display == PANEL_ROLLER_DISPLAY_POSITION) {
                lv_obj_remove_flag(ui->roller_lines[index][line], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(ui->roller_lines[index][line], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (ui->roller_fills[index][line] != NULL) {
            if (display == PANEL_ROLLER_DISPLAY_POSITION &&
                line < panel_roller_filled_lines(ui->model->roller_position_step[index])) {
                lv_obj_remove_flag(ui->roller_fills[index][line], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(ui->roller_fills[index][line], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    const char *text = display == PANEL_ROLLER_DISPLAY_UNAVAILABLE
        ? "Otillgänglig"
        : (display == PANEL_ROLLER_DISPLAY_POSITION ? "" : "Okänd");
    set_label_text_if_changed(ui->widget_status[index], text);
    if (lv_obj_get_style_text_font(ui->widget_status[index], 0) != &homey_panel_font_22)
        lv_obj_set_style_text_font(ui->widget_status[index], &homey_panel_font_22, 0);
    if (lv_obj_get_style_text_align(ui->widget_status[index], 0) != LV_TEXT_ALIGN_CENTER)
        lv_obj_set_style_text_align(ui->widget_status[index], LV_TEXT_ALIGN_CENTER, 0);
    if (display == PANEL_ROLLER_DISPLAY_POSITION) {
        lv_obj_add_flag(ui->widget_status[index], LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(ui->widget_status[index], LV_OBJ_FLAG_HIDDEN);
    }
}

static void render_light_icon(panel_ui_t *ui, size_t index)
{
    if (ui == NULL || index < 4U || index > 5U || ui->widget_icon[index] == NULL) return;
    uint32_t color = 0x596A78U;
    if (ui->light_toggle_pending[index - 4U]) {
        color = 0x8493A0U;
    } else if (ui->homey_data_ready && ui->model->widget_status[index] == PANEL_WIDGET_AVAILABLE &&
               ui->model->widget_has_boolean[index]) {
        color = ui->model->widget_boolean_value[index] ? 0xFFD166U : 0x687985U;
    }
    set_bg_color_if_changed(ui->widget_icon[index], lv_color_hex(color));
    if (ui->widget_icon_base[index] != NULL) {
        set_bg_color_if_changed(ui->widget_icon_base[index], lv_color_hex(color));
    }
}

static lv_obj_t *label_new(panel_ui_t *ui, lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    set_label_text_if_changed(label, text != NULL ? text : "");
    lv_obj_set_style_text_color(label, lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_text_font(label, &homey_panel_font_22, 0);
    theme_label_register(ui, label, THEME_TEXT_PRIMARY);
    return label;
}

static lv_obj_t *button_label_new(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    set_label_text_if_changed(label, text != NULL ? text : "");
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
    lv_obj_t *label = button_label_new(button, text);
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
    lv_obj_t *button = button_new_sized(parent, text, LV_PCT(100),
                                        PANEL_UI_SETTINGS_BUTTON_HEIGHT,
                                        callback, ui, label_out);
    const lv_color_t normal = lv_obj_get_style_bg_color(button, LV_PART_MAIN);
    lv_obj_set_style_bg_color(
        button, lv_color_darken(normal, 36), LV_PART_MAIN | LV_STATE_PRESSED);
    return button;
}

static lv_obj_t *settings_unavailable_row_new(
    lv_obj_t *parent, const char *text, lv_event_cb_t callback, panel_ui_t *ui)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, LV_PCT(100), 56);
    lv_obj_add_state(button, LV_STATE_DISABLED);
    lv_obj_set_style_shadow_width(button, 0, 0);
    if (callback != NULL) {
        lv_obj_add_event_cb(button, activity_event, LV_EVENT_PRESSED, ui);
        lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, ui);
    }
    lv_obj_t *label = button_label_new(button, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(92));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_center(label);
    return button;
}

static lv_obj_t *settings_section_new(panel_ui_t *ui, lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = label_new(ui, parent, text);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_height(label, 36);
    lv_obj_set_style_text_font(label, &homey_panel_font_22, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x8BD3F7), 0);
    lv_obj_set_style_pad_top(label, 8, 0);
    if (ui != NULL && ui->theme_label_count > 0U)
        ui->theme_labels[ui->theme_label_count - 1U].role = THEME_TEXT_ACCENT;
    return label;
}

static lv_obj_t *modal_button_new(lv_obj_t *parent, const char *text,
                                  lv_event_cb_t callback, panel_ui_t *ui)
{
    return button_new_sized(parent, text, 166, 54, callback, ui, NULL);
}

typedef enum { NAV_HAMBURGER, NAV_CLOSE, NAV_BACK } nav_icon_t;

static const lv_point_precise_t NAV_CLOSE_A[] = {{0, 0}, {25, 20}};
static const lv_point_precise_t NAV_CLOSE_B[] = {{0, 20}, {25, 0}};
static const lv_point_precise_t MENU_NEXT_POINTS[] = {{0, 0}, {10, 8}, {0, 16}};
static const lv_point_precise_t MENU_BACK_POINTS[] = {{10, 0}, {0, 8}, {10, 16}};

static void icon_line_new(lv_obj_t *parent, const lv_point_precise_t *points, uint32_t count)
{
    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, points, count);
    lv_obj_set_style_line_width(line, 3, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0xF4F7FA), 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *navigation_button_new(lv_obj_t *parent, nav_icon_t icon,
                                      lv_event_cb_t callback,
                                      panel_ui_t *ui)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, NAV_BUTTON_W, NAV_BUTTON_H);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_add_event_cb(button, activity_event, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, ui);

    if (icon != NAV_HAMBURGER) {
        lv_obj_t *canvas = lv_obj_create(button);
        lv_obj_remove_style_all(canvas);
        lv_obj_set_size(canvas, icon == NAV_BACK ? 11 : NAV_ICON_W,
                        icon == NAV_BACK ? 17 : NAV_ICON_H);
        lv_obj_center(canvas);
        lv_obj_remove_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
        if (icon == NAV_CLOSE) {
            icon_line_new(canvas, NAV_CLOSE_A, 2);
            icon_line_new(canvas, NAV_CLOSE_B, 2);
        } else {
            icon_line_new(canvas, MENU_BACK_POINTS, 3);
        }
        return button;
    }
    static const int32_t offsets[] = {-9, 0, 9};
    for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        lv_obj_t *bar = lv_obj_create(button);
        lv_obj_set_size(bar, NAV_ICON_W, 3);
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

static void status_modal_foreground(panel_ui_t *ui)
{
    if (ui->status_overlay == NULL || ui->modal_kind == MODAL_NONE ||
        ui->model->power_state != PANEL_POWER_ACTIVE) return;
    if (lv_obj_get_child(ui->screen, -1) != ui->status_overlay)
        lv_obj_move_foreground(ui->status_overlay);
}

static void status_modal_close_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind == MODAL_NONE) return;
    ui->modal_kind = MODAL_NONE;
    lv_obj_add_flag(ui->status_overlay, LV_OBJ_FLAG_HIDDEN);
    panel_ui_flush_deferred_save_error(ui);
}

static void wifi_cancel_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_WIFI || !ui->wifi_cancel_allowed) return;
    if (ui->callbacks.request_wifi_cancel == NULL) {
        status_modal_show(ui, MODAL_ERROR, "Fel", "Wi-Fi-ändringen kunde inte avbrytas.");
        return;
    }
    ui->wifi_cancel_allowed = false;
    lv_obj_add_state(ui->wifi_cancel, LV_STATE_DISABLED);
    lv_obj_t *label = lv_obj_get_child(ui->wifi_cancel, 0);
    if (label != NULL) set_label_text_if_changed(label, "Avbryter...");
    ui->callbacks.request_wifi_cancel(ui->callbacks.context);
}

static void status_modal_show(panel_ui_t *ui, modal_kind_t kind,
                              const char *title, const char *text)
{
    if (ui == NULL || ui->status_overlay == NULL) return;
    roller_feedback_cancel(ui);
    if (ui->modal_kind == MODAL_NONE) {
        /* Stop only background scroll inertia, at its current position. */
        lv_obj_stop_scroll_anim(ui->settings_content);
        lv_obj_stop_scroll_anim(ui->pager);
        patch018_clear_gesture(ui);
        ui->patch021_settings_scroll_active = false;
        lv_indev_t *input = lv_indev_get_next(NULL);
        while (input != NULL) {
            if (lv_indev_get_display(input) == lv_obj_get_display(ui->screen)) {
                const bool pressed = lv_indev_get_state(input) == LV_INDEV_STATE_PRESSED;
                lv_indev_reset(input, NULL);
                if (pressed) lv_indev_wait_release(input);
            }
            input = lv_indev_get_next(input);
        }
    }
    ui->modal_kind = kind;
    if (kind == MODAL_INFORMATION) lv_obj_remove_flag(ui->information_refresh, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui->information_refresh, LV_OBJ_FLAG_HIDDEN);
    set_label_text_if_changed(ui->status_title, title);
    set_label_text_if_changed(ui->status_text, text);
    lv_obj_set_style_text_align(ui->status_text,
        kind == MODAL_HELP || kind == MODAL_ERROR || kind == MODAL_INFORMATION ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->status_text,
        kind == MODAL_HELP || kind == MODAL_ERROR || kind == MODAL_INFORMATION ? LV_ALIGN_TOP_LEFT : LV_ALIGN_CENTER, 0, 0);
    lv_obj_scroll_to_y(ui->status_body, 0, LV_ANIM_OFF);
    lv_obj_remove_flag(ui->status_close, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui->status_close_footer, LV_OBJ_FLAG_HIDDEN);
    if (kind == MODAL_WIFI) {
        lv_obj_set_size(ui->status_body, 384, 168);
        lv_obj_set_height(ui->status_text, 52);
        lv_obj_set_width(ui->status_text, LV_PCT(100));
        lv_obj_align(ui->status_text, LV_ALIGN_TOP_LEFT, 0, 0);
        if (ui->wifi_qr != NULL) {
            lv_obj_set_size(ui->wifi_qr, 148, 148);
            lv_obj_align(ui->wifi_qr, LV_ALIGN_TOP_MID, 0, 56);
            lv_obj_remove_flag(ui->wifi_qr, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_remove_flag(ui->wifi_ssid, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui->wifi_password, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_width(ui->wifi_ssid, LV_PCT(100));
        lv_obj_set_height(ui->wifi_ssid, 32);
        lv_obj_align(ui->wifi_ssid, LV_ALIGN_TOP_LEFT, 0, 210);
        lv_obj_set_width(ui->wifi_password, LV_PCT(100));
        lv_obj_set_height(ui->wifi_password, 32);
        lv_obj_align(ui->wifi_password, LV_ALIGN_TOP_LEFT, 0, 244);
        if (ui->wifi_cancel_allowed) {
            lv_obj_remove_flag(ui->wifi_cancel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_state(ui->wifi_cancel, LV_STATE_DISABLED);
            lv_obj_t *label = lv_obj_get_child(ui->wifi_cancel, 0);
            if (label != NULL) set_label_text_if_changed(label, "Avbryt Wi-Fi-ändring");
        } else {
            lv_obj_add_flag(ui->wifi_cancel, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (ui->wifi_qr != NULL) lv_obj_add_flag(ui->wifi_qr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->wifi_ssid, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->wifi_password, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->wifi_cancel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_height(ui->status_text, LV_SIZE_CONTENT);
    }
    lv_obj_add_flag(ui->status_error_icon, LV_OBJ_FLAG_HIDDEN);
    if (kind == MODAL_ERROR) lv_obj_remove_flag(ui->status_error_icon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui->status_overlay, LV_OBJ_FLAG_HIDDEN);
    status_modal_foreground(ui);
}

static void panel_ui_flush_deferred_save_error(panel_ui_t *ui)
{
    if (ui == NULL || !ui->deferred_settings_error ||
        ui->modal_kind != MODAL_NONE) return;
    ui->deferred_settings_error = false;
    status_modal_show(ui, MODAL_ERROR, "Fel", "Kunde inte spara");
}

static void information_read(panel_ui_t *ui)
{
    panel_information_t info = {0};
    char text[640];
    bool available = ui->callbacks.read_information != NULL &&
        ui->callbacks.read_information(ui->callbacks.context, ui->information_kind, &info);
    if (!available || !panel_information_format(ui->information_kind, &info, text, sizeof(text)))
        (void)snprintf(text, sizeof(text), "Ej tillgänglig");
    set_label_text_if_changed(ui->status_text, text);
}

static void information_refresh_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_INFORMATION) return;
    information_read(ui);
}

static void information_open(panel_ui_t *ui, panel_information_kind_t kind)
{
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    ui->information_kind = kind;
    status_modal_show(ui, MODAL_INFORMATION, panel_information_title(kind), "Ej tillgänglig");
    information_read(ui);
}

#define INFORMATION_EVENT(name, kind) \
    static void name(lv_event_t *event) { information_open(lv_event_get_user_data(event), kind); }
INFORMATION_EVENT(wifi_information_event, PANEL_INFORMATION_WIFI)
INFORMATION_EVENT(homey_information_event, PANEL_INFORMATION_HOMEY)
INFORMATION_EVENT(firmware_information_event, PANEL_INFORMATION_FIRMWARE)
INFORMATION_EVENT(time_information_event, PANEL_INFORMATION_TIME)
INFORMATION_EVENT(system_information_event, PANEL_INFORMATION_SYSTEM)
#undef INFORMATION_EVENT

static lv_obj_t *status_icon_box(lv_obj_t *parent)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, 28, 28);
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

static void status_modal_create(panel_ui_t *ui)
{
    ui->status_overlay = lv_obj_create(ui->screen);
    lv_obj_remove_style_all(ui->status_overlay);
    lv_obj_set_size(ui->status_overlay, W, H);
    lv_obj_set_pos(ui->status_overlay, 0, 0);
    lv_obj_set_style_bg_color(ui->status_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui->status_overlay, LV_OPA_50, 0);
    lv_obj_add_flag(ui->status_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(ui->status_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ui->status_overlay, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_add_event_cb(ui->status_overlay, activity_event, LV_EVENT_PRESSED, ui);

    ui->status_dialog = lv_obj_create(ui->status_overlay);
    lv_obj_set_size(ui->status_dialog, 416, 360);
    lv_obj_center(ui->status_dialog);
    lv_obj_set_style_bg_color(ui->status_dialog, lv_color_hex(0x102432), 0);
    lv_obj_set_style_bg_opa(ui->status_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(ui->status_dialog, 16, 0);
    lv_obj_set_style_border_width(ui->status_dialog, 0, 0);
    lv_obj_set_style_radius(ui->status_dialog, 16, 0);
    lv_obj_remove_flag(ui->status_dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ui->status_dialog, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_add_event_cb(ui->status_dialog, activity_event, LV_EVENT_PRESSED, ui);
    ui->status_title = label_new(ui, ui->status_dialog, "");
    lv_obj_set_width(ui->status_title, 312);
    lv_obj_align(ui->status_title, LV_ALIGN_TOP_LEFT, 0, 10);
    ui->status_close = navigation_button_new(ui->status_dialog, NAV_CLOSE,
                                            status_modal_close_event, ui);
    lv_obj_align(ui->status_close, LV_ALIGN_TOP_RIGHT, 0, 0);
    ui->status_error_icon = status_icon_box(ui->status_dialog);
    lv_obj_t *bar = lv_obj_create(ui->status_error_icon);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 3, 16);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xFFB4A9), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_t *dot = lv_obj_create(ui->status_error_icon);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 3, 3);
    lv_obj_align(dot, LV_ALIGN_TOP_MID, 0, 23);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFFB4A9), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    ui->status_body = lv_obj_create(ui->status_dialog);
    lv_obj_remove_style_all(ui->status_body);
    lv_obj_set_size(ui->status_body, 384, 168);
    lv_obj_set_pos(ui->status_body, 0, 98);
    lv_obj_set_scroll_dir(ui->status_body, LV_DIR_VER);
    lv_obj_add_flag(ui->status_body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ui->status_body, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_add_event_cb(ui->status_body, activity_event, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(ui->status_body, activity_event, LV_EVENT_SCROLL_BEGIN, ui);
    ui->status_text = label_new(ui, ui->status_body, "");
    lv_obj_set_width(ui->status_text, LV_PCT(100));
    lv_label_set_long_mode(ui->status_text, LV_LABEL_LONG_WRAP);
    ui->status_close_footer = button_new_sized(ui->status_dialog, "Stäng", 154, 52,
                                               status_modal_close_event, ui, NULL);
    lv_obj_align(ui->status_close_footer, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    ui->information_refresh = button_new_sized(ui->status_dialog, "Uppdatera", 180, 52,
                                               information_refresh_event, ui, NULL);
    lv_obj_align(ui->information_refresh, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_flag(ui->information_refresh, LV_OBJ_FLAG_HIDDEN);
    ui->wifi_cancel = button_new_sized(ui->status_dialog, "Avbryt Wi-Fi-ändring", 204, 52,
                                       wifi_cancel_event, ui, NULL);
    lv_obj_align(ui->wifi_cancel, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_flag(ui->wifi_cancel, LV_OBJ_FLAG_HIDDEN);
    ui->wifi_ssid = label_new(ui, ui->status_body, "");
    lv_label_set_long_mode(ui->wifi_ssid, LV_LABEL_LONG_WRAP);
    ui->wifi_password = label_new(ui, ui->status_body, "");
    lv_label_set_long_mode(ui->wifi_password, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(ui->wifi_ssid, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->wifi_password, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->status_overlay, LV_OBJ_FLAG_HIDDEN);
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

static bool panel_light_widget_index(size_t widget_index)
{
    return widget_index == 4U || widget_index == 5U;
}

static bool panel_any_light_toggle_pending(const panel_ui_t *ui)
{
    return ui != NULL &&
        (ui->light_toggle_pending[0] || ui->light_toggle_pending[1]);
}

static bool panel_light_card_actionable(
    const panel_ui_t *ui, size_t widget_index)
{
    if (ui == NULL || !panel_light_widget_index(widget_index) ||
        ui->callbacks.request_light_toggle == NULL ||
        !ui->homey_data_ready ||
        panel_any_light_toggle_pending(ui) ||
        ui->model->widget_status[widget_index] != PANEL_WIDGET_AVAILABLE ||
        !ui->model->widget_has_boolean[widget_index]) {
        return false;
    }
    return panel_homey_favorites_light_toggle_execution_ready(
        widget_index, ui->homey_data_ready);
}

static void panel_light_card_update_interaction(
    panel_ui_t *ui, size_t widget_index)
{
    if (ui == NULL || !panel_light_widget_index(widget_index) ||
        ui->widget_card[widget_index] == NULL) {
        return;
    }
    if (panel_light_card_actionable(ui, widget_index)) {
        lv_obj_add_flag(ui->widget_card[widget_index], LV_OBJ_FLAG_CLICKABLE);
    } else {
        lv_obj_remove_flag(ui->widget_card[widget_index], LV_OBJ_FLAG_CLICKABLE);
    }
}

static void panel_light_card_event_for_index(
    lv_event_t *event, size_t widget_index)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_NONE || !panel_light_card_actionable(ui, widget_index)) return;

    const bool requested_value = !ui->model->widget_boolean_value[widget_index];
    const bool queued = ui->callbacks.request_light_toggle(
        ui->callbacks.context, widget_index, requested_value);
#ifdef ESP_PLATFORM
    ESP_LOGI(
        "panel_ui",
        "PATCH038_LIGHT_UI widget=%u queue_result=%s optimistic_state=no privacy=sanitized",
        (unsigned)widget_index, queued ? "queued" : "rejected");
#endif
    if (!queued) return;

    ui->light_toggle_pending[widget_index - 4U] = true;
    for (size_t index = 4U; index < 6U; ++index) {
        panel_light_card_update_interaction(ui, index);
    }
    render_light_icon(ui, widget_index);
    if (ui->widget_status[widget_index] != NULL) {
        set_label_text_if_changed(ui->widget_status[widget_index], "Väntar...");
    }
}

static void panel_light_widget4_event(lv_event_t *event)
{
    panel_light_card_event_for_index(event, 4U);
}

static void panel_light_widget5_event(lv_event_t *event)
{
    panel_light_card_event_for_index(event, 5U);
}

static void roller_feedback_set_arrow(panel_ui_t *ui, bool highlighted)
{
    if (ui == NULL || ui->roller_feedback_widget < 1U ||
        ui->roller_feedback_widget > 2U) return;
    const bool light = ui->model->settings.background_mode == PANEL_THEME_LIGHT;
    const theme_palette_t *palette = light ? &LIGHT_PALETTE : &DARK_PALETTE;
    const size_t direction = ui->roller_feedback_lower_half ? 1U : 0U;
    lv_obj_t *arrow = ui->roller_arrows[ui->roller_feedback_widget][direction];
    lv_obj_t *highlight = ui->roller_arrow_highlights[ui->roller_feedback_widget][direction];
    if (arrow != NULL) {
        const uint32_t arrow_color = highlighted
            ? (light ? 0xFFFFFFU : 0x071018U)
            : palette->roller_arrow;
        const lv_color_t color = lv_color_hex(arrow_color);
        if (!lv_color_eq(lv_obj_get_style_line_color(arrow, 0), color))
            lv_obj_set_style_line_color(arrow, color, 0);
    }
    if (highlight != NULL) {
        if (highlighted) {
            set_bg_color_if_changed(highlight, lv_color_hex(palette->roller_fill));
        }
        const lv_opa_t opa = highlighted ? LV_OPA_COVER : LV_OPA_TRANSP;
        if (lv_obj_get_style_bg_opa(highlight, 0) != opa)
            lv_obj_set_style_bg_opa(highlight, opa, 0);
    }
}

static void roller_feedback_cancel(panel_ui_t *ui)
{
    if (ui == NULL) return;
    lv_timer_t *timer = ui->roller_feedback_timer;
    ui->roller_feedback_timer = NULL;
    if (ui->roller_feedback_active) roller_feedback_set_arrow(ui, false);
    ui->roller_feedback_active = false;
    if (timer != NULL) lv_timer_delete(timer);
}

static void roller_feedback_timer_cb(lv_timer_t *timer)
{
    if (timer == NULL) return;
    panel_ui_t *ui = lv_timer_get_user_data(timer);
    if (ui == NULL || ui->destroying || !ui->roller_feedback_active ||
        ui->roller_feedback_timer != timer) {
        lv_timer_delete(timer);
        return;
    }
    ui->roller_feedback_timer = NULL;
    roller_feedback_set_arrow(ui, false);
    ui->roller_feedback_active = false;
    lv_timer_delete(timer);
}

static void roller_feedback_start(
    panel_ui_t *ui, uint8_t widget_index, bool lower_half)
{
    if (ui == NULL || widget_index < 1U || widget_index > 2U ||
        ui->modal_kind != MODAL_NONE || ui->destroying) return;
    roller_feedback_cancel(ui);
    ui->roller_feedback_widget = widget_index;
    ui->roller_feedback_lower_half = lower_half;
    ui->roller_feedback_active = true;
    roller_feedback_set_arrow(ui, true);
    ui->roller_feedback_timer = lv_timer_create(
        roller_feedback_timer_cb, ROLLER_TOUCH_FEEDBACK_MS, ui);
    if (ui->roller_feedback_timer == NULL) {
        roller_feedback_set_arrow(ui, false);
        ui->roller_feedback_active = false;
    }
}

static void roller_feedback_zone_event(lv_event_t *event)
{
    if (event == NULL || lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    roller_touch_context_t *context = lv_event_get_user_data(event);
    if (context == NULL || context->ui == NULL) return;
    roller_feedback_start(context->ui, context->widget_index,
                          context->lower_half);
    if (context->widget_index == 1U || context->widget_index == 2U) {
        const int8_t direction = context->lower_half ? 1 : -1;
        if (context->ui->callbacks.request_roller_step != NULL) {
            (void)context->ui->callbacks.request_roller_step(
                context->ui->callbacks.context,
                context->widget_index, direction);
        }
    }
}

static lv_obj_t *create_roller_zone(lv_obj_t *card, panel_ui_t *ui,
                                    size_t widget_index, bool lower_half,
                                    bool upward, int32_t y)
{
    lv_obj_t *zone = lv_button_create(card);
    lv_obj_set_size(zone, ROLLER_CARD_W - 12, 44);
    lv_obj_set_pos(zone, 6, y);
    lv_obj_set_style_bg_opa(zone, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(zone, LV_OPA_TRANSP,
                            LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(zone, 0, 0);
    lv_obj_set_style_shadow_width(zone, 0, 0);
    roller_touch_context_t *context =
        &ui->roller_touch[widget_index][lower_half ? 1U : 0U];
    context->ui = ui;
    context->widget_index = (uint8_t)widget_index;
    context->lower_half = lower_half;
    lv_obj_add_event_cb(zone, roller_feedback_zone_event,
                        LV_EVENT_CLICKED, context);
    const size_t direction = lower_half ? 1U : 0U;
    lv_obj_t *highlight = lv_obj_create(zone);
    lv_obj_remove_style_all(highlight);
    lv_obj_set_size(highlight, ROLLER_ARROW_HIGHLIGHT_W,
                    ROLLER_ARROW_HIGHLIGHT_H);
    lv_obj_center(highlight);
    lv_obj_set_style_bg_color(highlight, lv_color_hex(DARK_PALETTE.roller_fill), 0);
    lv_obj_set_style_bg_opa(highlight, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(highlight, 0, 0);
    lv_obj_set_style_radius(highlight, 6, 0);
    lv_obj_set_clickable(highlight, false);
    lv_obj_set_scrollable(highlight, false);
    ui->roller_arrow_highlights[widget_index][direction] = highlight;
    lv_obj_t *arrow = lv_line_create(zone);
    ui->roller_arrows[widget_index][direction] = arrow;
    lv_line_set_points(arrow,
        upward ? ROLLER_ARROW_UP_POINTS : ROLLER_ARROW_DOWN_POINTS, 3U);
    lv_obj_set_style_line_color(arrow, lv_color_hex(0x80919E), 0);
    lv_obj_set_style_line_opa(arrow, LV_OPA_COVER, 0);
    lv_obj_set_style_line_width(arrow, 3, 0);
    lv_obj_set_style_line_rounded(arrow, true, 0);
    lv_obj_center(arrow);
    return zone;
}

static lv_obj_t *create_markis_control(
    panel_ui_t *ui, size_t control_index, lv_obj_t *card, int32_t x,
    const char *label, bool upward, bool stop)
{
    lv_obj_t *control = lv_button_create(card);
    ui->disabled_markis_controls[control_index] = control;
    lv_obj_set_size(control, PANEL_UI_MARKIS_CONTROL_WIDTH, PANEL_UI_MARKIS_CONTROL_HEIGHT);
    lv_obj_set_pos(control, x, PANEL_UI_MARKIS_CONTROL_Y);
    lv_obj_set_style_pad_all(control, 0, 0);
    lv_obj_add_state(control, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(control, lv_color_hex(0x203746), 0);
    lv_obj_set_style_bg_opa(control, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(control, lv_color_hex(0x506575), 0);
    lv_obj_set_style_border_width(control, 1, 0);
    lv_obj_set_style_radius(control, 10, 0);
    lv_obj_set_style_shadow_width(control, 0, 0);

    if (stop) {
        lv_obj_t *stop_ring = lv_obj_create(control);
        lv_obj_remove_style_all(stop_ring);
        lv_obj_set_size(stop_ring, 24, 24);
        lv_obj_align(stop_ring, LV_ALIGN_TOP_MID, 0, 8);
        lv_obj_set_style_radius(stop_ring, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_color(stop_ring, lv_color_hex(0xD9E2E8), 0);
        lv_obj_set_style_border_width(stop_ring, 2, 0);
        lv_obj_set_style_bg_opa(stop_ring, LV_OPA_TRANSP, 0);
        lv_obj_remove_flag(stop_ring, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(stop_ring, LV_OBJ_FLAG_SCROLLABLE);
        ui->disabled_markis_symbols[control_index] = stop_ring;
        lv_obj_t *stop_mark = lv_obj_create(stop_ring);
        lv_obj_remove_style_all(stop_mark);
        lv_obj_set_size(stop_mark, 9, 9);
        lv_obj_center(stop_mark);
        lv_obj_set_style_radius(stop_mark, 2, 0);
        lv_obj_set_style_bg_color(stop_mark, lv_color_hex(0xD9E2E8), 0);
        lv_obj_set_style_bg_opa(stop_mark, LV_OPA_COVER, 0);
        lv_obj_remove_flag(stop_mark, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(stop_mark, LV_OBJ_FLAG_SCROLLABLE);
        ui->disabled_markis_stop_marks[control_index] = stop_mark;
    } else {
        lv_obj_t *arrow = lv_line_create(control);
        lv_line_set_points(arrow,
            upward ? DISABLED_MARKIS_ARROW_UP_POINTS :
                     DISABLED_MARKIS_ARROW_DOWN_POINTS, 3U);
        lv_obj_set_style_line_color(arrow, lv_color_hex(0xD9E2E8), 0);
        lv_obj_set_style_line_opa(arrow, LV_OPA_COVER, 0);
        lv_obj_set_style_line_width(arrow, 3, 0);
        lv_obj_set_style_line_rounded(arrow, true, 0);
        lv_obj_align(arrow, LV_ALIGN_TOP_MID, 0, 12);
        ui->disabled_markis_symbols[control_index] = arrow;
    }
    lv_obj_t *caption = lv_label_create(control);
    set_label_text_if_changed(caption, label);
    lv_obj_set_style_text_color(caption, lv_color_hex(0xD9E2E8), 0);
    lv_obj_set_style_text_font(caption, &homey_panel_font_22, 0);
    lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(caption, PANEL_UI_MARKIS_CONTROL_WIDTH - 8);
    lv_obj_align(caption, LV_ALIGN_BOTTOM_MID, 0, -3);
    lv_obj_remove_flag(caption, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(caption, LV_OBJ_FLAG_SCROLLABLE);
    ui->disabled_markis_captions[control_index] = caption;
    return control;
}

static lv_obj_t *create_roller_card(panel_ui_t *ui, lv_obj_t *parent, size_t index)
{
    lv_obj_t *card = lv_obj_create(parent);
    ui->widget_card[index] = card;
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(card, PANEL_UI_WIDGET_LAYOUT[index].width, PANEL_UI_WIDGET_LAYOUT[index].height);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x162735), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_90, 0);
    ui->widget_title[index] = label_new(ui, card, panel_ui_widget_title_for_model(ui->model, index));
    lv_obj_set_width(ui->widget_title[index], PANEL_UI_WIDGET_LAYOUT[index].width - 8);
    lv_obj_set_style_text_font(ui->widget_title[index],
        &homey_panel_font_22, 0);
    lv_obj_set_style_text_align(ui->widget_title[index], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->widget_title[index], LV_ALIGN_TOP_MID, 0, index == 0U ? 24 : 3);

    if (index == 0U) {
        /* Markis is direction-only in the Homey app. Controls are visibly
         * represented but disabled until a separately verified command path exists. */
        (void)create_markis_control(ui, 0U, card, PANEL_UI_MARKIS_CONTROL_X[0], "UPP", true, false);
        (void)create_markis_control(ui, 1U, card, PANEL_UI_MARKIS_CONTROL_X[1], "STOPP", false, true);
        (void)create_markis_control(ui, 2U, card, PANEL_UI_MARKIS_CONTROL_X[2], "NED", false, false);
    } else {
        (void)create_roller_zone(card, ui, index, false, true, 24);
        (void)create_roller_zone(card, ui, index, true, false, 116);
    }

    ui->widget_status[index] = label_new(ui, card, "Okänd");
    lv_obj_set_size(ui->widget_status[index], ROLLER_CARD_W - 10, 64);
    lv_obj_set_style_text_font(ui->widget_status[index], &homey_panel_font_22, 0);
    lv_obj_set_style_text_align(ui->widget_status[index], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->widget_status[index], LV_ALIGN_CENTER, 0, 0);

    for (size_t line = 0U; line < 4U; ++line) {
        lv_obj_t *bar = lv_obj_create(card);
        ui->roller_lines[index][line] = bar;
        lv_obj_set_size(bar, 56, 10);
        lv_obj_align(bar, LV_ALIGN_CENTER, 0, (int32_t)line * 12 - 14);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x435562), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 2, 0);
        lv_obj_set_style_pad_all(bar, 0, 0);
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *fill = lv_obj_create(card);
        ui->roller_fills[index][line] = fill;
        lv_obj_set_size(fill, 56, 10);
        lv_obj_align(fill, LV_ALIGN_CENTER, 0, (int32_t)line * 12 - 14);
        lv_obj_set_style_bg_color(fill, lv_color_hex(0x8BD3F7), 0);
        lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(fill, 0, 0);
        lv_obj_set_style_radius(fill, 2, 0);
        lv_obj_set_style_pad_all(fill, 0, 0);
        lv_obj_remove_flag(fill, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(fill, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(fill, LV_OBJ_FLAG_HIDDEN);
    }
    if (index == 0U) {
        /* Markis has no verified numeric position. Keep the five-step field
         * absent rather than implying a physical position. */
        lv_obj_add_flag(ui->widget_status[index], LV_OBJ_FLAG_HIDDEN);
    }
    render_roller_widget(ui, index);
    return card;
}

static lv_obj_t *create_light_card(panel_ui_t *ui, lv_obj_t *parent, size_t index)
{
    lv_obj_t *card = lv_obj_create(parent);
    ui->widget_card[index] = card;
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(card, LIGHT_CARD_W, LIGHT_CARD_H);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x162735), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_90, 0);
    ui->widget_title[index] = label_new(ui, card, panel_ui_widget_title_for_model(ui->model, index));
    lv_obj_set_width(ui->widget_title[index], LIGHT_CARD_W - 12);
    lv_obj_set_style_text_align(ui->widget_title[index], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->widget_title[index], LV_ALIGN_TOP_MID, 0, 7);

    ui->widget_icon[index] = lv_obj_create(card);
    lv_obj_set_size(ui->widget_icon[index], 46, 46);
    lv_obj_set_style_radius(ui->widget_icon[index], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ui->widget_icon[index], 3, 0);
    lv_obj_set_style_border_color(ui->widget_icon[index], lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_bg_color(ui->widget_icon[index], lv_color_hex(0x596A78), 0);
    lv_obj_set_style_bg_opa(ui->widget_icon[index], LV_OPA_COVER, 0);
    lv_obj_align(ui->widget_icon[index], LV_ALIGN_CENTER, 0, -2);
    lv_obj_remove_flag(ui->widget_icon[index], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(ui->widget_icon[index], LV_OBJ_FLAG_SCROLLABLE);

    ui->widget_icon_base[index] = lv_obj_create(card);
    lv_obj_set_size(ui->widget_icon_base[index], 22, 8);
    lv_obj_set_style_radius(ui->widget_icon_base[index], 3, 0);
    lv_obj_set_style_bg_color(ui->widget_icon_base[index], lv_color_hex(0x596A78), 0);
    lv_obj_set_style_bg_opa(ui->widget_icon_base[index], LV_OPA_COVER, 0);
    lv_obj_align(ui->widget_icon_base[index], LV_ALIGN_CENTER, 0, 28);
    lv_obj_remove_flag(ui->widget_icon_base[index], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(ui->widget_icon_base[index], LV_OBJ_FLAG_SCROLLABLE);

    char status_text[32];
    if (!panel_ui_render_status_text(ui, index, status_text, sizeof(status_text))) {
        (void)snprintf(status_text, sizeof(status_text), "%s", "Okänd");
    }
    ui->widget_status[index] = label_new(ui, card, status_text);
    lv_obj_set_width(ui->widget_status[index], LIGHT_CARD_W - 12);
    lv_obj_set_style_text_font(ui->widget_status[index], &homey_panel_font_22, 0);
    lv_obj_set_style_text_align(ui->widget_status[index], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->widget_status[index], LV_ALIGN_BOTTOM_MID, 0, -6);
    if (index == 4U) {
        lv_obj_add_event_cb(card, panel_light_widget4_event, LV_EVENT_CLICKED, ui);
    } else if (index == 5U) {
        lv_obj_add_event_cb(card, panel_light_widget5_event, LV_EVENT_CLICKED, ui);
    }
    panel_light_card_update_interaction(ui, index);
    render_light_icon(ui, index);
    return card;
}

static lv_obj_t *create_security_card(panel_ui_t *ui, lv_obj_t *parent)
{
    const size_t index = 3U;
    lv_obj_t *card = lv_obj_create(parent);
    ui->widget_card[index] = card;
    lv_obj_set_size(card, PANEL_UI_WIDGET_LAYOUT[index].width, PANEL_UI_WIDGET_LAYOUT[index].height);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x162735), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_90, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_CLICKABLE);
    ui->widget_title[index] = label_new(ui, card, panel_ui_widget_title_for_model(ui->model, index));
    lv_obj_set_width(ui->widget_title[index], 392);
    lv_obj_set_style_text_align(ui->widget_title[index], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->widget_title[index], LV_ALIGN_TOP_MID, 0, 24);
    ui->widget_status[index] = label_new(ui, card, "Status ej verifierad");
    lv_obj_set_width(ui->widget_status[index], 392);
    lv_obj_set_style_text_font(ui->widget_status[index], &homey_panel_font_22, 0);
    lv_obj_set_style_text_align(ui->widget_status[index], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(ui->widget_status[index]);
    return card;
}

static void dots_render(panel_ui_t *ui)
{
    for (size_t i = 0; i < PANEL_UI_PAGE_COUNT; ++i) {
        const lv_opa_t opacity = i == ui->model->active_page ? LV_OPA_COVER : LV_OPA_40;
        if (lv_obj_get_style_bg_opa(ui->dots[i], 0) != opacity)
            lv_obj_set_style_bg_opa(ui->dots[i], opacity, 0);
    }
}

static void pager_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->destroying) return;
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SCROLL_BEGIN) {
        roller_feedback_cancel(ui);
        return;
    }
    if (code != LV_EVENT_SCROLL_END) return;
    int32_t x = lv_obj_get_scroll_x(ui->pager);
    (void)panel_ui_set_active_page(ui->model, (x + W / 2) / W);
    dots_render(ui);
#ifdef ESP_PLATFORM
    ESP_LOGI("panel_ui", "V32_UI us=%llu event=page_end page=%u",
             (unsigned long long)esp_timer_get_time(), (unsigned)ui->model->active_page);
#endif
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
    if (ui == NULL || ui->destroying || ui->settings_content == NULL) return;

    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SCROLL_BEGIN) {
        if (ui->patch021_settings_scroll_active) return;

        lv_indev_t *indev = lv_indev_active();
        if (indev != NULL && lv_indev_get_state(indev) != LV_INDEV_STATE_PRESSED) return;

        ui->patch021_settings_scroll_active = true;
        ui->patch021_settings_scroll_start_ms = (uint32_t)lv_tick_get();
        ui->patch021_settings_scroll_start_y = lv_obj_get_scroll_y(ui->settings_content);
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
    const int32_t end_y = lv_obj_get_scroll_y(ui->settings_content);
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

static void open_advanced_settings_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    panel_ui_open_advanced_settings_model(ui->model);
#ifdef ESP_PLATFORM
    ESP_LOGI("panel_ui", "V32_UI us=%llu event=advanced_open",
             (unsigned long long)esp_timer_get_time());
#endif
    set_label_text_if_changed(ui->settings_header_title, "Avancerat");
    lv_obj_add_flag(ui->settings_main_body, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui->settings_advanced_body, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_scroll_dir(ui->settings_content, LV_DIR_VER);
    lv_obj_add_flag(ui->settings_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ui->settings_back_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_scroll_to_y(ui->settings_content, 0, LV_ANIM_OFF);
}

static void back_settings_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    panel_ui_back_to_settings_model(ui->model);
#ifdef ESP_PLATFORM
    ESP_LOGI("panel_ui", "V32_UI us=%llu event=advanced_back",
             (unsigned long long)esp_timer_get_time());
#endif
    set_label_text_if_changed(ui->settings_header_title, "Inställningar");
    lv_obj_add_flag(ui->settings_advanced_body, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui->settings_main_body, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_scroll_dir(ui->settings_content, LV_DIR_NONE);
    lv_obj_remove_flag(ui->settings_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui->settings_back_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_scroll_to_y(ui->settings_content, 0, LV_ANIM_OFF);
}

static void notify_settings(panel_ui_t *ui)
{
    panel_ui_settings_normalize(&ui->model->settings);
    if (ui->callbacks.settings_changed != NULL) {
        ui->callbacks.settings_changed(ui->callbacks.context, &ui->model->settings);
    } else {
        status_modal_show(ui, MODAL_ERROR, "Fel", "Inställningarna kunde inte sparas.");
    }
    render_settings(ui);
    render_power(ui);
}

static void normal_brightness_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    ui->model->settings.normal_brightness = next_u8(
        ui->model->settings.normal_brightness, NORMAL_LEVELS,
        sizeof(NORMAL_LEVELS) / sizeof(NORMAL_LEVELS[0]));
    notify_settings(ui);
}

static void dim_brightness_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    ui->model->settings.dimmed_brightness = next_u8(
        ui->model->settings.dimmed_brightness, DIMMED_LEVELS,
        sizeof(DIMMED_LEVELS) / sizeof(DIMMED_LEVELS[0]));
    notify_settings(ui);
}

static void dim_timeout_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    ui->model->settings.dim_after_seconds = next_u32(
        ui->model->settings.dim_after_seconds, DIM_TIMEOUTS,
        sizeof(DIM_TIMEOUTS) / sizeof(DIM_TIMEOUTS[0]));
    notify_settings(ui);
}

static void off_timeout_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    ui->model->settings.off_after_seconds = next_u32(
        ui->model->settings.off_after_seconds, OFF_TIMEOUTS,
        sizeof(OFF_TIMEOUTS) / sizeof(OFF_TIMEOUTS[0]));
    notify_settings(ui);
}

static void background_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    ui->model->settings.background_mode =
        ui->model->settings.background_mode == PANEL_THEME_DARK
        ? PANEL_THEME_LIGHT : PANEL_THEME_DARK;
    notify_settings(ui);
}

static void wifi_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    if (ui->save_pending) {
        status_modal_show(ui, MODAL_ERROR, "Fel",
                          "Vänta tills inställningen är sparad.");
        return;
    }
    bool present = ui->callbacks.request_wifi_reconfigure != NULL;
    if (ui->callbacks.interaction_trace != NULL)
        ui->callbacks.interaction_trace(ui->callbacks.context, PANEL_UI_TRACE_WIFI_CLICK, present);
    if (present) {
        status_modal_show(ui, MODAL_HELP, "Wi-Fi", "Öppnar Wi-Fi-inställning...");
        ui->callbacks.request_wifi_reconfigure(ui->callbacks.context);
    } else {
        status_modal_show(ui, MODAL_ERROR, "Fel", "Wi-Fi-inställningen kunde inte öppnas.");
    }
}

static void choose_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (!panel_homey_actions_allowed(ui)) return;
    status_modal_show(ui, MODAL_HELP, "Välj annan Homey",
        "Funktionen aktiveras i nästa steg.\nIngen Homey-anslutning ändras nu.");
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
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return;
    panel_ui_cancel_confirmation(ui->model);
    render_view(ui);
}

static void accept_event(lv_event_t *event)
{
    panel_ui_t *ui = lv_event_get_user_data(event);
    if (ui == NULL || !panel_homey_actions_allowed(ui)) return;
    panel_confirmed_action_t action = panel_ui_accept_confirmation(ui->model);
    render_view(ui);
    status_modal_show(ui, MODAL_HELP, "Information", "Funktionen aktiveras i nästa steg");
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
    set_label_text_if_changed(ui->connection_label, state);
}

static void render_settings(panel_ui_t *ui)
{
    char text[96];
    if (ui->homey_connection_label != NULL) {
        set_label_text_if_changed(ui->homey_connection_label,
            ui->homey_data_ready ? "Homey-anslutning · Redo" :
                                  "Homey-anslutning · Inte redo");
    }
    (void)snprintf(text, sizeof(text), "Normal ljusstyrka: %u %%", (unsigned)ui->model->settings.normal_brightness);
    set_label_text_if_changed(ui->normal_brightness_label, text);
    (void)snprintf(text, sizeof(text), "Dämpad ljusstyrka: %u %%", (unsigned)ui->model->settings.dimmed_brightness);
    set_label_text_if_changed(ui->dim_brightness_label, text);
    (void)snprintf(text, sizeof(text), "Dämpningstid: %s",
        ui->model->settings.dim_after_seconds == 60U ? "1 min" :
        ui->model->settings.dim_after_seconds == 30U ? "30 s" : "10 s");
    set_label_text_if_changed(ui->dim_timeout_label, text);
    (void)snprintf(text, sizeof(text), "Släckningstid: %s",
        ui->model->settings.off_after_seconds == PANEL_UI_OFF_DISABLED ? "Alltid på" :
        ui->model->settings.off_after_seconds == 1200U ? "20 min" :
        ui->model->settings.off_after_seconds == 300U ? "5 min" : "1 min");
    set_label_text_if_changed(ui->off_timeout_label, text);
    (void)snprintf(text, sizeof(text), "Bakgrund: %s",
        ui->model->settings.background_mode == PANEL_THEME_LIGHT ? "Ljust" : "Mörkt");
    set_label_text_if_changed(ui->background_label, text);
    theme_apply(ui);
}

static void render_view(panel_ui_t *ui)
{
    bool settings = ui->model->view == PANEL_UI_VIEW_SETTINGS;
    bool confirm = ui->model->view == PANEL_UI_VIEW_CONFIRMATION;
    if (settings || confirm) lv_obj_remove_flag(ui->settings_layer, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui->settings_layer, LV_OBJ_FLAG_HIDDEN);
    if (confirm) {
        lv_obj_remove_flag(ui->confirmation_layer, LV_OBJ_FLAG_HIDDEN);
        set_label_text_if_changed(ui->confirmation_text,
            ui->model->confirmation == PANEL_CONFIRM_HOMEY_WIPE
            ? "Radera Homey-anslutning\n\nFunktionen aktiveras i ett senare steg.\nIngen anslutning ändras nu."
            : "Byt Athom-konto\n\nFunktionen aktiveras i ett senare steg.\nInget konto ändras nu.");
    } else lv_obj_add_flag(ui->confirmation_layer, LV_OBJ_FLAG_HIDDEN);
    status_modal_foreground(ui);
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
        if (index < 3U) {
            render_roller_widget(ui, index);
            continue;
        }
        if (ui->widget_status[index] == NULL) continue;
        char status_text[32];
        if (!panel_ui_render_status_text(
                ui, index, status_text, sizeof(status_text))) {
            (void)snprintf(status_text, sizeof(status_text), "%s", "Okänd");
        }
        set_label_text_if_changed(ui->widget_status[index], status_text);
        render_light_icon(ui, index);
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
    ui->rendered_theme = UINT8_MAX;
    (void)snprintf(ui->clock_text, sizeof(ui->clock_text), "--:--");

    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(ui->screen, 0, 0);
    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0x071018), 0);

    lv_obj_t *top = lv_obj_create(ui->screen);
    lv_obj_set_pos(top, 0, 0);
    lv_obj_set_size(top, W, TOP_H);
    lv_obj_set_style_pad_all(top, NAV_MARGIN, 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_shadow_width(top, 0, 0);
    ui->clock_label = label_new(ui, top, ui->clock_text);
    lv_obj_align(ui->clock_label, LV_ALIGN_LEFT_MID, 12, 0);
    ui->connection_label = label_new(ui, top, "Okänd");
    lv_obj_set_width(ui->connection_label, 190);
    lv_obj_align(ui->connection_label, LV_ALIGN_RIGHT_MID, -64, 0);
    lv_obj_set_style_text_align(ui->connection_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *settings = navigation_button_new(top, NAV_HAMBURGER, open_settings_event, ui);
    lv_obj_align(settings, LV_ALIGN_RIGHT_MID, 0, 0);

    ui->pager = lv_obj_create(ui->screen);
    lv_obj_set_pos(ui->pager, 0, TOP_H);
    lv_obj_set_size(ui->pager, W, PAGE_H);
    lv_obj_set_style_pad_all(ui->pager, 0, 0);
    lv_obj_set_style_pad_column(ui->pager, 0, 0);
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
    lv_obj_add_event_cb(ui->pager, pager_event, LV_EVENT_SCROLL_BEGIN, ui);
    lv_obj_add_event_cb(ui->pager, pager_event, LV_EVENT_SCROLL_END, ui);
    lv_obj_add_event_cb(ui->pager, patch018_swipe_gesture_event, LV_EVENT_SCROLL_END, ui);

    for (size_t i = 0; i < PANEL_UI_PAGE_COUNT; ++i) {
        ui->pages[i] = lv_obj_create(ui->pager);
        lv_obj_set_size(ui->pages[i], W, PAGE_H);
        lv_obj_set_style_pad_all(ui->pages[i], 0, 0);
        lv_obj_set_style_border_width(ui->pages[i], 0, 0);
        lv_obj_set_style_bg_opa(ui->pages[i], LV_OPA_TRANSP, 0);
        lv_obj_add_flag(ui->pages[i], LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_remove_flag(ui->pages[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Create every logical widget exactly once, on its visual page. */
    for (size_t index = 0U; index < PANEL_UI_WIDGET_COUNT; ++index) {
        const panel_ui_widget_layout_t *placement = &PANEL_UI_WIDGET_LAYOUT[index];
        lv_obj_t *card = index < 3U
            ? create_roller_card(ui, ui->pages[placement->page], index)
            : (index == 3U ? create_security_card(ui, ui->pages[placement->page])
                           : create_light_card(ui, ui->pages[placement->page], index));
        lv_obj_set_pos(card, placement->x, placement->y);
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
    lv_obj_set_pos(ui->settings_layer, 0, 0);
    lv_obj_set_style_bg_color(ui->settings_layer, lv_color_hex(0x0D1B25), 0);
    lv_obj_set_style_bg_opa(ui->settings_layer, LV_OPA_COVER, 0);
    lv_obj_remove_flag(ui->settings_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(ui->settings_layer, 0, 0);
    lv_obj_set_style_border_width(ui->settings_layer, 0, 0);
    lv_obj_set_style_shadow_width(ui->settings_layer, 0, 0);
    lv_obj_remove_flag(ui->settings_layer, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_event_cb(ui->settings_layer, activity_event, LV_EVENT_PRESSED, ui);
    ui->settings_header = lv_obj_create(ui->settings_layer);
    lv_obj_set_size(ui->settings_header, W, TOP_H);
    lv_obj_set_pos(ui->settings_header, 0, 0);
    lv_obj_set_style_bg_opa(ui->settings_header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui->settings_header, 0, 0);
    lv_obj_set_style_pad_all(ui->settings_header, NAV_MARGIN, 0);
    lv_obj_remove_flag(ui->settings_header, LV_OBJ_FLAG_SCROLLABLE);
    ui->settings_back_button = navigation_button_new(
        ui->settings_header, NAV_BACK, back_settings_event, ui);
    lv_obj_align(ui->settings_back_button, LV_ALIGN_LEFT_MID, 0, 0);
    ui->settings_header_title = label_new(ui, ui->settings_header, "Inställningar");
    lv_obj_set_width(ui->settings_header_title, W - 2 * (NAV_BUTTON_W + 2 * NAV_MARGIN));
    lv_obj_set_style_text_align(ui->settings_header_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(ui->settings_header_title);
    lv_obj_t *close = navigation_button_new(ui->settings_header, NAV_CLOSE,
                                            close_settings_event, ui);
    lv_obj_align(close, LV_ALIGN_RIGHT_MID, 0, 0);

    ui->settings_content = lv_obj_create(ui->settings_layer);
    lv_obj_set_size(ui->settings_content, W - 24, PANEL_UI_SETTINGS_CONTENT_HEIGHT);
    lv_obj_set_pos(ui->settings_content, 12, 76);
    lv_obj_set_style_bg_opa(ui->settings_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui->settings_content, 0, 0);
    lv_obj_set_style_pad_all(ui->settings_content, 0, 0);
    lv_obj_set_style_pad_row(ui->settings_content, 6, 0);
    lv_obj_set_flex_flow(ui->settings_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui->settings_content, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(ui->settings_content, LV_DIR_NONE);
    lv_obj_remove_flag(ui->settings_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ui->settings_content, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_event_cb(ui->settings_content, activity_event, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(ui->settings_content, activity_event, LV_EVENT_SCROLL_BEGIN, ui);
    lv_obj_add_event_cb(ui->settings_content, patch021_settings_scroll_event, LV_EVENT_SCROLL_BEGIN, ui);
    lv_obj_add_event_cb(ui->settings_content, patch021_settings_scroll_event, LV_EVENT_SCROLL_END, ui);

    ui->settings_main_body = lv_obj_create(ui->settings_content);
    lv_obj_set_size(ui->settings_main_body, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(ui->settings_main_body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui->settings_main_body, 0, 0);
    lv_obj_set_style_pad_all(ui->settings_main_body, 0, 0);
    lv_obj_set_style_pad_row(ui->settings_main_body, PANEL_UI_SETTINGS_ROW_GAP, 0);
    lv_obj_set_flex_flow(ui->settings_main_body, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(ui->settings_main_body, LV_OBJ_FLAG_SCROLLABLE);
    settings_button_new(ui->settings_main_body, "Normal ljusstyrka", normal_brightness_event, ui, &ui->normal_brightness_label);
    settings_button_new(ui->settings_main_body, "Dämpad ljusstyrka", dim_brightness_event, ui, &ui->dim_brightness_label);
    settings_button_new(ui->settings_main_body, "Dämpningstid", dim_timeout_event, ui, &ui->dim_timeout_label);
    settings_button_new(ui->settings_main_body, "Släckningstid", off_timeout_event, ui, &ui->off_timeout_label);
    settings_button_new(ui->settings_main_body, "Bakgrund", background_event, ui, &ui->background_label);
    lv_obj_t *advanced = settings_button_new(ui->settings_main_body, "Avancerat",
                        open_advanced_settings_event, ui, NULL);
    lv_obj_t *next_icon = lv_obj_create(advanced);
    lv_obj_remove_style_all(next_icon);
    lv_obj_set_size(next_icon, 11, 17);
    lv_obj_align(next_icon, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_remove_flag(next_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(next_icon, LV_OBJ_FLAG_SCROLLABLE);
    icon_line_new(next_icon, MENU_NEXT_POINTS, 3);
    ui->settings_advanced_body = lv_obj_create(ui->settings_content);
    lv_obj_set_size(ui->settings_advanced_body, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(ui->settings_advanced_body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui->settings_advanced_body, 0, 0);
    lv_obj_set_style_pad_all(ui->settings_advanced_body, 0, 0);
    lv_obj_set_style_pad_row(ui->settings_advanced_body, 6, 0);
    lv_obj_set_flex_flow(ui->settings_advanced_body, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(ui->settings_advanced_body, LV_OBJ_FLAG_SCROLLABLE);
    settings_section_new(ui, ui->settings_advanced_body, "ANSLUTNINGAR");
    settings_button_new(ui->settings_advanced_body, "Wi-Fi-status", wifi_information_event, ui, NULL);
    settings_button_new(ui->settings_advanced_body, "Ändra Wi-Fi", wifi_event, ui, NULL);
    settings_button_new(ui->settings_advanced_body, "Homey-anslutning", homey_information_event, ui, &ui->homey_connection_label);
    lv_obj_set_size(ui->homey_connection_label, LV_PCT(100), 56);
    settings_unavailable_row_new(ui->settings_advanced_body, "Välj annan Homey · Ej tillgänglig", choose_event, ui);
    settings_unavailable_row_new(ui->settings_advanced_body, "Byt Athom-konto · Ej tillgänglig", account_event, ui);
    settings_section_new(ui, ui->settings_advanced_body, "SYSTEM");
    settings_button_new(ui->settings_advanced_body, "Firmwareversion", firmware_information_event, ui, NULL);
    settings_button_new(ui->settings_advanced_body, "Tidssynkronisering", time_information_event, ui, NULL);
    settings_button_new(ui->settings_advanced_body, "Systemdiagnostik", system_information_event, ui, NULL);
    settings_unavailable_row_new(ui->settings_advanced_body, "Radera Homey-anslutning · Ej tillgänglig", wipe_event, ui);
    lv_obj_add_flag(ui->settings_advanced_body, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->settings_back_button, LV_OBJ_FLAG_HIDDEN);
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
    ui->confirmation_text = label_new(ui, ui->confirmation_layer, "");
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

    status_modal_create(ui);
    /* Apply persisted theme before the first dashboard frame, not only after
     * the Settings view is opened. */
    theme_apply(ui);

    *out = ui;
    return panel_ui_refresh(ui);
}

void panel_ui_destroy(panel_ui_t **ptr)
{
    if (ptr == NULL || *ptr == NULL) return;
    panel_ui_t *ui = *ptr;
    ui->destroying = true;
    roller_feedback_cancel(ui);
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
        ui->settings_content != NULL && lv_obj_is_scrolling(ui->settings_content);
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
        if (i < 3U) {
            render_roller_widget(ui, i);
            continue;
        }
        if (ui->widget_title[i] == NULL || ui->widget_status[i] == NULL) continue;
        const char *title = panel_ui_widget_title_for_model(ui->model, i);
        set_label_text_if_changed(ui->widget_title[i], title != NULL ? title : "");
        char status_text[32];
        if (!panel_ui_render_status_text(
                ui, i, status_text, sizeof(status_text))) {
            (void)snprintf(status_text, sizeof(status_text), "%s", "Okänd");
        }
        set_label_text_if_changed(ui->widget_status[i], status_text);
        render_light_icon(ui, i);
        panel_light_card_update_interaction(ui, i);
    }
    set_label_text_if_changed(ui->clock_label, ui->clock_text);
    render_connection(ui);
    render_settings(ui);
    render_view(ui);
    render_power(ui);

#ifdef ESP_PLATFORM
    const uint64_t page_start_us = (uint64_t)esp_timer_get_time();
    /* Preserve the user-owned pager offset while LVGL is settling a swipe. */
    const bool page_reassert = !pager_scrolling_start &&
        lv_obj_get_scroll_x(ui->pager) != (int32_t)ui->model->active_page * W;
#else
    const bool page_reassert = !lv_obj_is_scrolling(ui->pager) &&
        lv_obj_get_scroll_x(ui->pager) != (int32_t)ui->model->active_page * W;
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
        ui->settings_content != NULL && lv_obj_is_scrolling(ui->settings_content);
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
    set_label_text_if_changed(ui->clock_label, ui->clock_text);
    return a;
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
    for (size_t index = 0U; index < 3U; ++index) render_roller_widget(ui, index);
    render_settings(ui);
    for (size_t index = 3U; index < 6U; ++index) {
        char status_text[32];
        if (!panel_ui_render_status_text(ui, index, status_text, sizeof(status_text))) {
            (void)snprintf(status_text, sizeof(status_text), "%s", "Okänd");
        }
        if (ui->widget_status[index] != NULL) set_label_text_if_changed(ui->widget_status[index], status_text);
        render_light_icon(ui, index);
    }
    panel_light_card_update_interaction(ui, 4U);
    panel_light_card_update_interaction(ui, 5U);
    if (!ready && ui->model->view == PANEL_UI_VIEW_CONFIRMATION) {
        panel_ui_cancel_confirmation(ui->model);
        render_view(ui);
    }
    return true;
}

bool panel_ui_set_light_toggle_pending(
    panel_ui_t *ui, size_t widget_index, bool pending)
{
    if (ui == NULL || !panel_light_widget_index(widget_index)) return false;
    const size_t slot = widget_index - 4U;
    if (ui->light_toggle_pending[slot] == pending) {
        /* Authority can change while the visible Favorite values stay equal.
         * The rotation task calls this even without a presentation refresh. */
        panel_light_card_update_interaction(ui, 4U);
        panel_light_card_update_interaction(ui, 5U);
        return true;
    }

    ui->light_toggle_pending[slot] = pending;
    if (ui->widget_status[widget_index] != NULL) {
        char status_text[32];
        if (!panel_ui_render_status_text(
                ui, widget_index, status_text, sizeof(status_text))) {
            (void)snprintf(status_text, sizeof(status_text), "%s", "Okänd");
        }
        set_label_text_if_changed(ui->widget_status[widget_index], status_text);
    }
    render_light_icon(ui, widget_index);
    panel_light_card_update_interaction(ui, 4U);
    panel_light_card_update_interaction(ui, 5U);
    return true;
}

bool panel_ui_select_page(panel_ui_t *ui, uint8_t page, bool animate)
{
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return false;
    const uint8_t previous_page = ui->model->active_page;
    uint8_t normalized = panel_ui_set_active_page(ui->model, page);
    if (normalized != previous_page) {
        roller_feedback_cancel(ui);
    }
    lv_obj_scroll_to_x(ui->pager, (int32_t)normalized * W,
        animate ? LV_ANIM_ON : LV_ANIM_OFF);
    dots_render(ui);
    return normalized == page;
}

bool panel_ui_open_settings(panel_ui_t *ui)
{
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return false;
    roller_feedback_cancel(ui);
#ifdef ESP_PLATFORM
    ESP_LOGI("panel_ui", "V32_UI us=%llu event=settings_open",
             (unsigned long long)esp_timer_get_time());
#endif
    panel_ui_open_settings_model(ui->model);
    set_label_text_if_changed(ui->settings_header_title, "Inställningar");
    lv_obj_add_flag(ui->settings_advanced_body, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ui->settings_main_body, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_scroll_dir(ui->settings_content, LV_DIR_NONE);
    lv_obj_remove_flag(ui->settings_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui->settings_back_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_scroll_to_y(ui->settings_content, 0, LV_ANIM_OFF);
    render_settings(ui);
    render_view(ui);
    return true;
}

bool panel_ui_close_settings(panel_ui_t *ui)
{
    if (ui == NULL || ui->modal_kind != MODAL_NONE) return false;
#ifdef ESP_PLATFORM
    ESP_LOGI("panel_ui", "V32_UI us=%llu event=settings_close",
             (unsigned long long)esp_timer_get_time());
#endif
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
    if (ui == NULL || ui->save_pending) return false;
    const char *text = "Wi-Fi-inställningen kunde inte öppnas.";
    modal_kind_t kind = MODAL_ERROR;
    if (result == PANEL_UI_WIFI_RECONFIGURE_OPENED) {
        kind = MODAL_HELP;
        text = "Wi-Fi-inställning öppnad\n\nAnslut till panelens Wi-Fi-nätverk\noch öppna 192.168.4.1.";
    } else if (result == PANEL_UI_WIFI_RECONFIGURE_BLOCKED) {
        text = "Wi-Fi-inställningen är redan öppen eller kan inte startas i nuvarande läge.";
    } else if (result == PANEL_UI_WIFI_CANCEL_FAILED) {
        text = "Wi-Fi-ändringen kunde inte stängas säkert. Sparade inställningar har inte ändrats.";
    }
    status_modal_show(ui, kind, kind == MODAL_HELP ? "Wi-Fi" : "Fel", text);
    return true;
}

static void clear_ui_secret(char *value, size_t size)
{
    volatile char *p = value;
    while (size-- != 0U) *p++ = '\0';
}

bool panel_ui_update_wifi_provisioning(
    panel_ui_t *ui, void *qr_object, const char *ssid, const char *password,
    bool cancel_allowed)
{
    if (ui == NULL || qr_object == NULL || ssid == NULL || password == NULL) return false;
    lv_obj_t *qr = (lv_obj_t *)qr_object;
    if (ui->wifi_qr == NULL) {
        ui->wifi_qr = qr;
        ui->wifi_qr_original_parent = lv_obj_get_parent(qr);
        ui->wifi_qr_original_x = lv_obj_get_x(qr);
        ui->wifi_qr_original_y = lv_obj_get_y(qr);
        ui->wifi_qr_original_width = lv_obj_get_width(qr);
        ui->wifi_qr_original_height = lv_obj_get_height(qr);
        ui->wifi_qr_original_hidden = lv_obj_has_flag(qr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_parent(qr, ui->status_body);
    } else if (ui->wifi_qr != qr) {
        return false;
    }
    char ssid_text[64];
    char password_text[80];
    char qr_payload[128];
    int ssid_length = snprintf(ssid_text, sizeof(ssid_text), "Wi-Fi: %s", ssid);
    int password_length = snprintf(password_text, sizeof(password_text), "Lösenord: %s", password);
    bool valid = ssid_length >= 0 && (size_t)ssid_length < sizeof(ssid_text) &&
        password_length >= 0 && (size_t)password_length < sizeof(password_text);
    int qr_length = -1;
    if (valid && password[0] != '\0') {
        qr_length = snprintf(qr_payload, sizeof(qr_payload),
            "WIFI:T:WPA;S:%s;P:%s;;", ssid, password);
        valid = qr_length > 0 && (size_t)qr_length < sizeof(qr_payload);
        if (valid) valid = PANEL_UI_QR_UPDATE(qr, qr_payload, (uint32_t)qr_length) == LV_RESULT_OK;
    }
    if (valid) {
        set_label_text_if_changed(ui->wifi_ssid, ssid_text);
        set_label_text_if_changed(ui->wifi_password, password_text);
        ui->wifi_cancel_allowed = cancel_allowed;
        if (cancel_allowed && password[0] != '\0') {
            lv_obj_remove_flag(qr, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        /* A failed update must never leave the preceding session visible as
         * though its QR/password matched the current AP. Keep the AP itself
         * untouched and fail closed only in the presentation layer. */
        lv_obj_add_flag(qr, LV_OBJ_FLAG_HIDDEN);
        set_label_text_if_changed(ui->wifi_ssid, "");
        set_label_text_if_changed(ui->wifi_password, "");
        ui->wifi_cancel_allowed = false;
        status_modal_show(ui, MODAL_ERROR, "Fel",
            "Wi-Fi-uppgifterna kunde inte uppdateras. Öppna dialogen igen.");
    }
    clear_ui_secret(ssid_text, sizeof(ssid_text));
    clear_ui_secret(password_text, sizeof(password_text));
    clear_ui_secret(qr_payload, sizeof(qr_payload));
    return valid;
}

bool panel_ui_present_wifi_provisioning(
    panel_ui_t *ui, void *qr_object, bool commit_reserved)
{
    if (ui == NULL || ui->wifi_qr == NULL || ui->wifi_qr != (lv_obj_t *)qr_object) return false;
    status_modal_show(ui, MODAL_WIFI, "Ändra Wi-Fi", commit_reserved
        ? "Nätverket sparas. Avbryt är inte möjligt just nu."
        : "Anslut telefonen via QR eller Wi-Fi-namnet. Öppna sedan http://192.168.4.1.");
    if (commit_reserved) {
        if (ui->wifi_qr != NULL) lv_obj_add_flag(ui->wifi_qr, LV_OBJ_FLAG_HIDDEN);
        set_label_text_if_changed(ui->wifi_ssid, "");
        set_label_text_if_changed(ui->wifi_password, "");
    }
    return ui->modal_kind == MODAL_WIFI;
}

bool panel_ui_set_wifi_cancel_pending(panel_ui_t *ui, bool pending, bool commit_reserved)
{
    if (ui == NULL) return false;
    if (commit_reserved) {
        ui->wifi_cancel_allowed = false;
        status_modal_show(ui, MODAL_WIFI, "Ändra Wi-Fi",
            "Nätverket sparas. Avbryt är inte möjligt just nu.");
        if (ui->wifi_qr != NULL) {
            (void)lv_qrcode_update(ui->wifi_qr, " ", 1U);
            lv_obj_add_flag(ui->wifi_qr, LV_OBJ_FLAG_HIDDEN);
        }
        set_label_text_if_changed(ui->wifi_ssid, "");
        set_label_text_if_changed(ui->wifi_password, "");
        return true;
    }
    if (pending) {
        ui->wifi_cancel_allowed = false;
        if (ui->wifi_cancel != NULL) {
            lv_obj_add_state(ui->wifi_cancel, LV_STATE_DISABLED);
            lv_obj_add_flag(ui->wifi_cancel, LV_OBJ_FLAG_HIDDEN);
        }
        set_label_text_if_changed(ui->status_text,
            "Avbryter Wi-Fi-ändringen och återställer föregående anslutning...");
    } else {
        ui->wifi_cancel_allowed = true;
        if (ui->wifi_cancel != NULL) {
            lv_obj_remove_state(ui->wifi_cancel, LV_STATE_DISABLED);
            if (ui->modal_kind == MODAL_WIFI) lv_obj_remove_flag(ui->wifi_cancel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_t *label = lv_obj_get_child(ui->wifi_cancel, 0);
            if (label != NULL) set_label_text_if_changed(label, "Avbryt Wi-Fi-ändring");
        }
    }
    return true;
}

bool panel_ui_clear_wifi_provisioning(panel_ui_t *ui, void *qr_object)
{
    if (ui == NULL) return false;
    lv_obj_t *qr = (lv_obj_t *)qr_object;
    if (ui->wifi_qr != NULL && qr != NULL && ui->wifi_qr == qr) {
        (void)lv_qrcode_update(qr, " ", 1U);
        lv_obj_add_flag(qr, LV_OBJ_FLAG_HIDDEN);
        if (ui->wifi_qr_original_parent != NULL) {
            lv_obj_set_parent(qr, ui->wifi_qr_original_parent);
            lv_obj_set_pos(qr, ui->wifi_qr_original_x, ui->wifi_qr_original_y);
            lv_obj_set_size(qr, ui->wifi_qr_original_width, ui->wifi_qr_original_height);
            if (!ui->wifi_qr_original_hidden) lv_obj_remove_flag(qr, LV_OBJ_FLAG_HIDDEN);
        }
        ui->wifi_qr = NULL;
        ui->wifi_qr_original_parent = NULL;
    }
    set_label_text_if_changed(ui->wifi_ssid, "");
    set_label_text_if_changed(ui->wifi_password, "");
    ui->wifi_cancel_allowed = false;
    if (ui->modal_kind == MODAL_WIFI) {
        ui->modal_kind = MODAL_NONE;
        lv_obj_add_flag(ui->status_overlay, LV_OBJ_FLAG_HIDDEN);
        panel_ui_flush_deferred_save_error(ui);
    }
    return true;
}

bool panel_ui_set_settings_save_status(
    panel_ui_t *ui, panel_ui_settings_save_status_t status)
{
    if (ui == NULL) return false;
    if (status == PANEL_UI_SETTINGS_SAVE_SAVING) {
        ui->save_pending = true;
    } else if (status == PANEL_UI_SETTINGS_SAVE_SAVED) {
        /* A stale/unpaired success notification must never invent a saved operation. */
        if (!ui->save_pending) return false;
        ui->save_pending = false;
    } else if (status == PANEL_UI_SETTINGS_SAVE_FAILED) {
        ui->save_pending = false;
        if (ui->modal_kind == MODAL_NONE) {
            status_modal_show(ui, MODAL_ERROR, "Fel", "Kunde inte spara");
        } else if (ui->modal_kind != MODAL_ERROR) {
            ui->deferred_settings_error = true;
        } else {
            set_label_text_if_changed(ui->status_title, "Fel");
            set_label_text_if_changed(ui->status_text, "Kunde inte spara");
        }
    } else {
        return false;
    }
    return true;
}

bool panel_ui_restore_settings(
    panel_ui_t *ui, const panel_ui_settings_t *confirmed_settings)
{
    if (ui == NULL || confirmed_settings == NULL) return false;
    ui->model->settings = *confirmed_settings;
    panel_ui_settings_normalize(&ui->model->settings);
    render_settings(ui);

    uint8_t brightness = ui->model->settings.normal_brightness;
    if (ui->model->power_state == PANEL_POWER_DIMMED)
        brightness = ui->model->settings.dimmed_brightness;
    else if (ui->model->power_state == PANEL_POWER_OFF)
        brightness = 0U;
    if (ui->rendered_brightness != brightness ||
        ui->rendered_power != ui->model->power_state) {
        ui->rendered_brightness = brightness;
        ui->rendered_power = ui->model->power_state;
        if (ui->callbacks.request_brightness != NULL)
            ui->callbacks.request_brightness(ui->callbacks.context, brightness);
    }
    return true;
}

bool panel_ui_reset_view(panel_ui_t *ui)
{
    if (ui == NULL) return false;
    panel_ui_set_view(ui->model, PANEL_UI_VIEW_DASHBOARD);
    return panel_ui_refresh(ui);
}
