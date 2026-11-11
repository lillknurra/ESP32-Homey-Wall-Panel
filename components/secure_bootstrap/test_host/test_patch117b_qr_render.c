#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "lvgl.h"
#include "panel_wifi_qr.h"
#include "homey_panel_font_22.h"

#define DISPLAY_W 480
#define DISPLAY_H 480

static uint16_t draw_buf[DISPLAY_W * 96];
static uint16_t frame_buf[DISPLAY_W * DISPLAY_H];
static lv_display_t *display;

static unsigned qr_pixel_index(const lv_draw_buf_t *buffer, int32_t x, int32_t y)
{
    const uint8_t *pixels = buffer->data + 8U;
    const uint8_t byte = pixels[(size_t)y * buffer->header.stride + (size_t)x / 8U];
    return (unsigned)((byte >> (7U - ((unsigned)x & 7U))) & 1U);
}

static lv_color32_t qr_palette_color(const lv_draw_buf_t *buffer, unsigned index)
{
    const lv_color32_t *palette = (const lv_color32_t *)buffer->data;
    return palette[index];
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *pixels)
{
    const int32_t width = area->x2 - area->x1 + 1;
    for (int32_t y = area->y1; y <= area->y2; ++y) {
        memcpy(&frame_buf[(size_t)y * DISPLAY_W + area->x1],
               pixels + (size_t)(y - area->y1) * (size_t)width * sizeof(uint16_t),
               (size_t)width * sizeof(uint16_t));
    }
    lv_display_flush_ready(disp);
}

static void write_ppm(const char *path, const lv_area_t *area)
{
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    fprintf(file, "P6\n%d %d\n255\n", area->x2 - area->x1 + 1,
            area->y2 - area->y1 + 1);
    for (int32_t y = area->y1; y <= area->y2; ++y) {
        for (int32_t x = area->x1; x <= area->x2; ++x) {
            const uint16_t pixel = frame_buf[(size_t)y * DISPLAY_W + x];
            const uint8_t rgb[3] = {
                (uint8_t)((((pixel >> 11U) & 0x1fU) * 255U) / 31U),
                (uint8_t)((((pixel >> 5U) & 0x3fU) * 255U) / 63U),
                (uint8_t)(((pixel & 0x1fU) * 255U) / 31U),
            };
            assert(fwrite(rgb, sizeof(rgb), 1U, file) == 1U);
        }
    }
    assert(fclose(file) == 0);
}

static void assert_widget_geometry(lv_obj_t *frame, lv_obj_t *qr, int32_t size)
{
    const lv_draw_buf_t *buffer = lv_canvas_get_draw_buf(qr);
    assert(buffer != NULL);
    assert(buffer->header.w == (uint32_t)size && buffer->header.h == (uint32_t)size);
    assert(lv_obj_get_width(qr) == size && lv_obj_get_height(qr) == size);
    const int32_t pad = panel_wifi_qr_frame_padding(size);
    assert(pad >= 4 * (size / 25));
    assert(lv_obj_get_width(frame) == size + 2 * pad);
    assert(lv_obj_get_height(frame) == size + 2 * pad);

    int32_t min_x = size, min_y = size, max_x = -1, max_y = -1;
    for (int32_t y = 0; y < size; ++y) {
        for (int32_t x = 0; x < size; ++x) {
            if (qr_pixel_index(buffer, x, y) == 1U) {
                if (x < min_x) min_x = x;
                if (y < min_y) min_y = y;
                if (x > max_x) max_x = x;
                if (y > max_y) max_y = y;
            }
        }
    }
    assert(min_x >= 0 && min_y >= 0 && max_x < size && max_y < size);
    assert(min_x < max_x && min_y < max_y);
    const int32_t padding = panel_wifi_qr_frame_padding(size);
    const int32_t minimum_quiet_pixels = 4 * (size / 25);
    assert(padding + min_x >= minimum_quiet_pixels);
    assert(padding + min_y >= minimum_quiet_pixels);
    assert(padding + (size - 1 - max_x) >= minimum_quiet_pixels);
    assert(padding + (size - 1 - max_y) >= minimum_quiet_pixels);
    const lv_color32_t dark = qr_palette_color(buffer, 1U);
    assert(dark.red < 16U && dark.green < 16U && dark.blue < 16U);
    const lv_color32_t light = qr_palette_color(buffer, 0U);
    assert(light.red > 240U && light.green > 240U && light.blue > 240U);

    lv_area_t frame_area;
    lv_obj_get_coords(frame, &frame_area);
    assert(frame_area.x1 >= 0 && frame_area.y1 >= 0);
    assert(frame_area.x2 < DISPLAY_W && frame_area.y2 < DISPLAY_H);
}

static void capture(lv_obj_t *frame, lv_obj_t *qr, int32_t size,
                    const char *ssid, const char *password, const char *path,
                    bool relayout)
{
    if (relayout) assert(panel_wifi_qr_layout_frame(frame, qr, size));
    assert(panel_wifi_qr_encode(qr, ssid, password));
    assert_widget_geometry(frame, qr, size);
    lv_obj_set_hidden(frame, false);
    lv_obj_set_hidden(qr, false);
    lv_obj_update_layout(lv_screen_active());
    lv_refr_now(display);
    lv_area_t area;
    lv_obj_get_coords(frame, &area);
    write_ppm(path, &area);
}

static void test_payload_validation(void)
{
    char payload[PANEL_WIFI_QR_MAX_PAYLOAD_SIZE];
    size_t length = 0U;
    assert(panel_wifi_qr_build_payload(
        "AP;Lab,Unit\"", "PASS\\;WORD8", payload, sizeof(payload), &length));
    assert(strcmp(payload,
        "WIFI:T:WPA;S:AP\\;Lab\\,Unit\\\";P:PASS\\\\\\;WORD8;;") == 0);
    assert(length == strlen(payload));
    assert(!panel_wifi_qr_build_payload("", "PASSWORD", payload, sizeof(payload), &length));
    assert(payload[0] == '\0' && length == 0U);
    assert(!panel_wifi_qr_build_payload("SSID", "short", payload, sizeof(payload), &length));
    assert(payload[0] == '\0' && length == 0U);
    assert(!panel_wifi_qr_build_payload("SSID", "PASSWORD", payload, 8U, &length));
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    char path[1024];
    test_payload_validation();

    lv_init();
    display = lv_display_create(DISPLAY_W, DISPLAY_H);
    assert(display != NULL);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush_cb);
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x071018), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_obj_t *instruction = lv_label_create(screen);
    lv_obj_t *caption = lv_label_create(screen);
    lv_obj_t *code = lv_label_create(screen);
    assert(title != NULL && instruction != NULL && caption != NULL && code != NULL);
    lv_label_set_text(title, "Säker Wi-Fi-installation");
    lv_label_set_text(instruction, "Skanna QR för att ansluta\nÖppna 192.168.4.1");
    lv_label_set_text(caption, "Panelkod · giltig 05:00");
    lv_label_set_text(code, "SYNTHETIC88");
    lv_obj_set_style_text_font(title, &homey_panel_font_22, 0);
    lv_obj_set_style_text_font(instruction, &homey_panel_font_22, 0);
    lv_obj_set_style_text_font(caption, &homey_panel_font_22, 0);
    lv_obj_set_style_text_font(code, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_color(instruction, lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_text_color(caption, lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_text_color(code, lv_color_white(), 0);
    lv_obj_t *startup_frame = lv_obj_create(screen);
    lv_obj_t *qr = lv_qrcode_create(startup_frame);
    assert(startup_frame != NULL && qr != NULL);
    assert(panel_wifi_qr_layout_startup(
        screen, title, instruction, caption, code, startup_frame, qr));
    snprintf(path, sizeof(path), "%s/first-install.ppm", argv[1]);
    capture(startup_frame, qr, PANEL_WIFI_QR_STARTUP_SIZE,
            "HomeyPanel-Setup", "SYNTHETIC88AB", path, false);
    snprintf(path, sizeof(path), "%s/startup-screen.ppm", argv[1]);
    const lv_area_t full_screen = {0, 0, DISPLAY_W - 1, DISPLAY_H - 1};
    write_ppm(path, &full_screen);

    snprintf(path, sizeof(path), "%s/wifi-change.ppm", argv[1]);
    capture(startup_frame, qr, PANEL_WIFI_QR_DIALOG_SIZE,
            "HomeyPanel-Setup", "SYNTHETIC88AB", path, true);

    snprintf(path, sizeof(path), "%s/rotated-escaped.ppm", argv[1]);
    capture(startup_frame, qr, PANEL_WIFI_QR_DIALOG_SIZE,
            "AP;Lab,Unit\"", "PASS\\;WORD8", path, true);

    const char max_ssid[33] = ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;";
    const char max_password[64] = ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;";
    snprintf(path, sizeof(path), "%s/max-credentials.ppm", argv[1]);
    capture(startup_frame, qr, PANEL_WIFI_QR_DIALOG_SIZE,
            max_ssid, max_password, path, true);

    for (unsigned i = 0; i < 20U; ++i) {
        assert(panel_wifi_qr_encode(qr, "HomeyPanel-Setup", "ROTATED88ABCD"));
        assert(panel_wifi_qr_layout_frame(startup_frame, qr, PANEL_WIFI_QR_DIALOG_SIZE));
        assert_widget_geometry(startup_frame, qr, PANEL_WIFI_QR_DIALOG_SIZE);
    }
    (void)lv_qrcode_update(qr, " ", 1U);
    lv_obj_set_hidden(startup_frame, true);
    lv_obj_set_hidden(qr, true);
    assert(lv_obj_is_hidden(startup_frame));
    assert(lv_obj_is_hidden(qr));
    lv_obj_delete(screen);
    puts("PATCH117B_REAL_LVGL_QR_WIDGET PASS");
    return 0;
}
