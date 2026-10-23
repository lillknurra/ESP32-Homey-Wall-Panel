#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lvgl.h"
#include "homey_panel_font_22.h"
#include "panel_startup_status.h"

static uint16_t draw_buffer[480U * 160U];
static uint16_t frame_buffer[480U * 480U];
static unsigned flush_count;
static int flush_y_min;
static int flush_y_max;

static void flush_to_memory(
    lv_display_t *display,
    const lv_area_t *area,
    uint8_t *pixels)
{
    assert(area->x1 >= 0 && area->y1 >= 0);
    assert(area->x2 < 480 && area->y2 < 480);
    const unsigned width = (unsigned)(area->x2 - area->x1 + 1);
    const uint16_t *source = (const uint16_t *)pixels;
    for (int y = area->y1; y <= area->y2; ++y) {
        memcpy(&frame_buffer[(size_t)y * 480U + (size_t)area->x1],
               source, width * sizeof(*source));
        source += width;
    }
    if (flush_count == 0U || area->y1 < flush_y_min) flush_y_min = area->y1;
    if (flush_count == 0U || area->y2 > flush_y_max) flush_y_max = area->y2;
    flush_count++;
    lv_display_flush_ready(display);
}

static void assert_detail_status_renders(
    lv_display_t *display,
    lv_obj_t *label,
    const char *primary,
    const char *secondary,
    bool allow_initial_layout_clear)
{
    char visible_text[192];
    assert(panel_startup_compose_visible_text(
        visible_text, sizeof(visible_text), primary, secondary));
    lv_label_set_text(label, visible_text);
    lv_obj_set_style_pad_top(label, 0, 0);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_obj_update_layout(label);
    const uint16_t text_height = (uint16_t)lv_obj_get_height(label);

    panel_startup_layout_t layout;
    assert(panel_startup_layout_compute(
        480U, 266U, 16U, 24U, 0U,
        text_height, 0U, &layout));
    const uint16_t area_top = 282U;
    const uint16_t area_height = 174U;
    assert(text_height <= area_height);
    lv_obj_set_style_pad_top(label, (lv_coord_t)(area_height - text_height), 0);
    lv_obj_set_height(label, (lv_coord_t)area_height);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, (lv_coord_t)area_top);
    lv_obj_update_layout(label);

    lv_area_t area;
    lv_obj_get_coords(label, &area);
    assert(area.x1 >= 18 && area.x2 <= 461);
    assert(area.y1 == 282 && area.y2 == 455);

    flush_count = 0U;
    flush_y_min = 480;
    flush_y_max = -1;
    lv_obj_invalidate(label);
    lv_refr_now(display);
    assert(flush_count != 0U);
    if (flush_y_min < (allow_initial_layout_clear ? 48 : 276) ||
        flush_y_max > 461 ||
        flush_y_max - flush_y_min >= (allow_initial_layout_clear ? 420 : 200)) {
        fprintf(stderr, "flush=%d..%d label=%d..%d initial=%d text=%s\n",
                flush_y_min, flush_y_max, area.y1, area.y2,
                allow_initial_layout_clear, visible_text);
    }
    assert(flush_y_min >= (allow_initial_layout_clear ? 48 : 276));
    assert(flush_y_max <= 461);
    assert(flush_y_max - flush_y_min < (allow_initial_layout_clear ? 420 : 200));
    assert(!(flush_y_min == 0 && flush_y_max == 479));

    size_t visible_pixels = 0U;
    for (int y = area.y1; y <= area.y2; ++y) {
        for (int x = area.x1; x <= area.x2; ++x) {
            if (frame_buffer[(size_t)y * 480U + (size_t)x] != 0U) {
                visible_pixels++;
            }
        }
    }
    assert(visible_pixels != 0U);
}

int main(void)
{
    lv_init();
    lv_display_t *display = lv_display_create(480, 480);
    assert(display != NULL);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buffer, NULL,
        sizeof(draw_buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush_to_memory);

    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_t *s_detail = lv_label_create(screen);
    assert(s_detail != NULL);
    lv_obj_set_width(s_detail, 444);
    lv_label_set_long_mode(s_detail, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_detail, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_detail, lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_text_opa(s_detail, LV_OPA_COVER, 0);
    lv_obj_set_style_text_line_space(s_detail, 5, 0);
    lv_obj_set_style_text_font(s_detail, &homey_panel_font_22, 0);
    lv_obj_set_height(s_detail, LV_SIZE_CONTENT);
    lv_obj_align(s_detail, LV_ALIGN_TOP_MID, 0, 58);
    lv_refr_now(display);

    assert_detail_status_renders(display, s_detail,
        "Ansluter till Wi-Fi...", "Kontrollerar sparad konfiguration", true);
    assert_detail_status_renders(display, s_detail, "Wi-Fi ansluten", "", false);
    assert_detail_status_renders(display, s_detail,
        "Hämtar Homey-data...", "Väntar på verifierad live-data", false);
    assert_detail_status_renders(display, s_detail,
        "Hämtningen tar längre tid än väntat", "Försöker igen...", false);
    assert_detail_status_renders(display, s_detail, "Homey READY", "", false);
    assert_detail_status_renders(display, s_detail,
        "Homey-data kunde inte hämtas",
        "Kontrollera anslutningen eller Homey-inloggningen", false);
    assert_detail_status_renders(display, s_detail,
        "Ansluter till Wi-Fi...", "Återansluter till sparat nätverk", false);

    lv_display_delete(display);
    lv_deinit();
    puts("PATCH107_STARTUP_DETAIL_PIXEL_RENDER_PASS");
    return 0;
}
