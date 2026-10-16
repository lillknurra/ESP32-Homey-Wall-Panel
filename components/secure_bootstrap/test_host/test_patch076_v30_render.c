/* Exercise the production ESP refresh branch with the real LVGL renderer.
 * The flush sink is synchronous CPU memory, so this test cannot prove RGB
 * DMA, PSRAM bandwidth, VSYNC or hardware stability. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "lvgl.h"
#include "panel_homey_favorites.h"

bool panel_homey_favorites_apply_ui_model(panel_ui_model_t *model)
{
    (void)model;
    return false;
}

bool panel_homey_favorites_light_toggle_execution_ready(size_t index, bool ready)
{
    (void)index;
    (void)ready;
    return false;
}

/* Include the renderer to inspect its private pager without adding a
 * production diagnostic API or changing the public UI contract. */
#define ESP_PLATFORM
#include "../panel_ui.c"

static uint16_t draw_pixels[480 * 160];
static uint16_t frame_pixels[480 * 480];
static uint16_t before_redraw[480 * 480];
static unsigned compared_frames;

static void flush_memory(lv_display_t *display, const lv_area_t *area, uint8_t *pixels)
{
    assert(area->x1 >= 0 && area->y1 >= 0);
    assert(area->x2 < 480 && area->y2 < 480);
    const unsigned width = (unsigned)(area->x2 - area->x1 + 1);
    const uint16_t *source = (const uint16_t *)pixels;
    for (int y = area->y1; y <= area->y2; ++y) {
        memcpy(&frame_pixels[y * 480 + area->x1], source, width * sizeof(*source));
        source += width;
    }
    lv_display_flush_ready(display);
}

static void assert_incremental_matches_full(panel_ui_t *ui, lv_display_t *display)
{
    lv_refr_now(display);
    memcpy(before_redraw, frame_pixels, sizeof(frame_pixels));
    lv_obj_invalidate(ui->screen);
    lv_refr_now(display);
    size_t differences = 0;
    for (size_t i = 0; i < 480 * 480; ++i)
        if (before_redraw[i] != frame_pixels[i]) ++differences;
    if (differences != 0) fprintf(stderr, "INCREMENTAL_FULL_DIFF frame=%u pixels=%zu\n", compared_frames, differences);
    assert(differences == 0);
    ++compared_frames;
}

int main(void)
{
    lv_init();
    lv_display_t *display = lv_display_create(480, 480);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_pixels, NULL, sizeof(draw_pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush_memory);
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0);
    panel_ui_t *ui = NULL;
    const panel_ui_config_t config = {.model = &model};
    assert(panel_ui_create(&ui, &config));
    assert(panel_ui_activate(ui));
    assert(panel_ui_set_homey_data_ready(ui, true));
    /* Save status is non-modal; success clears pending without constructing SAVED. */
    assert(!panel_ui_set_settings_save_status(ui, PANEL_UI_SETTINGS_SAVE_SAVED));
    assert(panel_ui_set_settings_save_status(ui, PANEL_UI_SETTINGS_SAVE_SAVING));
    assert(ui->modal_kind == MODAL_NONE && ui->save_pending);
    assert(lv_obj_has_flag(ui->status_overlay, LV_OBJ_FLAG_HIDDEN));
    lv_obj_update_layout(ui->screen);
    assert_incremental_matches_full(ui, display);
    assert(panel_ui_set_settings_save_status(ui, PANEL_UI_SETTINGS_SAVE_SAVED));
    assert(ui->modal_kind == MODAL_NONE && !ui->save_pending);
    assert(lv_obj_has_flag(ui->status_overlay, LV_OBJ_FLAG_HIDDEN));
    assert_incremental_matches_full(ui, display);
    assert(!panel_ui_set_settings_save_status(ui, PANEL_UI_SETTINGS_SAVE_SAVED));
    assert(panel_ui_set_settings_save_status(ui, PANEL_UI_SETTINGS_SAVE_SAVING));
    assert(panel_ui_set_settings_save_status(ui, PANEL_UI_SETTINGS_SAVE_FAILED));
    assert(ui->modal_kind == MODAL_ERROR && !ui->save_pending);
    lv_obj_send_event(ui->status_close_footer, LV_EVENT_CLICKED, NULL);
    assert(ui->modal_kind == MODAL_NONE);
    assert(lv_obj_has_flag(ui->status_overlay, LV_OBJ_FLAG_HIDDEN));
    const panel_ui_connection_info_t connection = {.state = PANEL_UI_CONNECTION_CONNECTED};
    assert(panel_ui_set_connection(ui, &connection));
    lv_obj_update_layout(ui->screen);
    assert_incremental_matches_full(ui, display);
    for (unsigned target = 1; target <= 3; ++target) {
        const unsigned page = target % 3;
        /* A user scroll keeps the model on the old page until SCROLL_END. */
        lv_obj_scroll_to_x(ui->pager, page * 480, LV_ANIM_ON);
        for (unsigned tick = 0; tick < 40; ++tick) {
            lv_tick_inc(16);
            lv_timer_handler();
            const int32_t offset = lv_obj_get_scroll_x(ui->pager);
            const bool scrolling = lv_obj_is_scrolling(ui->pager);
            const struct tm time = {.tm_hour = 12, .tm_min = (int)(tick % 60)};
            assert(panel_ui_set_time(ui, &time, true));
            /* Sanitized model updates only; no Homey calls or commands. */
            for (unsigned index = 1; index < 3; ++index) {
                model.widget_status[index] = PANEL_WIDGET_AVAILABLE;
                model.roller_readback_binding_verified[index] = true;
                model.roller_position_valid[index] = true;
                model.roller_position_step[index] = tick % 5;
            }
            for (unsigned index = 4; index < 6; ++index) {
                model.widget_status[index] = PANEL_WIDGET_AVAILABLE;
                model.widget_has_boolean[index] = true;
                model.widget_boolean_value[index] = (tick % 2) != 0;
            }
            for (unsigned refresh = 0; refresh < 3; ++refresh) assert(panel_ui_refresh(ui));
            if (scrolling) assert(lv_obj_get_scroll_x(ui->pager) == offset);
            assert_incremental_matches_full(ui, display);
        }
        assert(model.active_page == page);
        assert(lv_obj_get_scroll_x(ui->pager) == (int32_t)page * 480);
    }
    for (unsigned transition = 0; transition < 8; ++transition) {
        assert(panel_ui_open_settings(ui));
        for (unsigned refresh = 0; refresh < 3; ++refresh) assert(panel_ui_refresh(ui));
        assert_incremental_matches_full(ui, display);
        assert(panel_ui_close_settings(ui));
        for (unsigned refresh = 0; refresh < 3; ++refresh) assert(panel_ui_refresh(ui));
        assert_incremental_matches_full(ui, display);
    }
    panel_ui_destroy(&ui);
    lv_display_delete(display);
    lv_deinit();
    printf("PATCH076_V30_REAL_LVGL_RENDER PASS frames=%u\n", compared_frames);
    return 0;
}
