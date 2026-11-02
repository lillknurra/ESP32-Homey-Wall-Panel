#include "panel_wifi_qr.h"

#include <string.h>

static void secure_clear(char *value, size_t size)
{
    volatile char *p = value;
    while (size-- != 0U) *p++ = '\0';
}

static bool append_bytes(char *output, size_t capacity, size_t *used,
                         const char *value, size_t length, bool escape)
{
    for (size_t i = 0; i < length; ++i) {
        const char ch = value[i];
        const bool needs_escape = escape &&
            (ch == '\\' || ch == ';' || ch == ',' || ch == '"');
        const size_t required = needs_escape ? 2U : 1U;
        if (*used > capacity || required >= capacity - *used) return false;
        if (needs_escape) output[(*used)++] = '\\';
        output[(*used)++] = ch;
    }
    return true;
}

bool panel_wifi_qr_build_payload(
    const char *ssid, const char *password,
    char *output, size_t output_capacity, size_t *output_length)
{
    static const char prefix[] = "WIFI:T:WPA;S:";
    static const char password_field[] = ";P:";
    static const char suffix[] = ";;";
    if (output_length != NULL) *output_length = 0U;
    if (output != NULL && output_capacity != 0U) secure_clear(output, output_capacity);
    if (ssid == NULL || password == NULL || output == NULL || output_length == NULL) return false;
    const size_t ssid_length = strnlen(ssid, 33U);
    const size_t password_length = strnlen(password, 64U);
    if (ssid_length == 0U || ssid_length > 32U ||
        password_length < 8U || password_length > 63U) return false;

    size_t used = 0U;
    bool ok = append_bytes(output, output_capacity, &used, prefix, sizeof(prefix) - 1U, false) &&
        append_bytes(output, output_capacity, &used, ssid, ssid_length, true) &&
        append_bytes(output, output_capacity, &used, password_field, sizeof(password_field) - 1U, false) &&
        append_bytes(output, output_capacity, &used, password, password_length, true) &&
        append_bytes(output, output_capacity, &used, suffix, sizeof(suffix) - 1U, false);
    if (!ok || used >= output_capacity) return false;
    output[used] = '\0';
    *output_length = used;
    return true;
}

bool panel_wifi_qr_set_size(lv_obj_t *qr, int32_t size)
{
    if (qr == NULL || size < 25) return false;
    lv_qrcode_set_quiet_zone(qr, true);
    lv_qrcode_set_size(qr, size);
    /* LVGL 9.6's QR size API reallocates the canvas buffer; it does not set
     * the lv_obj geometry. Keep both dimensions explicitly synchronized. */
    lv_obj_set_size(qr, size, size);
    lv_obj_update_layout(qr);
    lv_draw_buf_t *buffer = lv_canvas_get_draw_buf(qr);
    return buffer != NULL && buffer->header.w == (uint32_t)size &&
        buffer->header.h == (uint32_t)size &&
        lv_obj_get_width(qr) == size && lv_obj_get_height(qr) == size;
}

int32_t panel_wifi_qr_frame_padding(int32_t qr_size)
{
    if (qr_size < 25) return 0;
    /* LVGL's built-in quiet-zone encoder reserves at least two modules per
     * side. This adds four complete modules per side outside the QR canvas.
     * With quiet-zone matrix size >=25 modules, this is conservative for all
     * payload versions and leaves the standard four-module clear zone even
     * if a caller later renders against a clipped parent. */
    return (qr_size / 25) * 4;
}

bool panel_wifi_qr_layout_frame(lv_obj_t *frame, lv_obj_t *qr, int32_t qr_size)
{
    const int32_t padding = panel_wifi_qr_frame_padding(qr_size);
    if (frame == NULL || qr == NULL || padding < 4 ||
        lv_obj_get_parent(qr) != frame || !panel_wifi_qr_set_size(qr, qr_size)) return false;
    const int32_t outer_size = qr_size + (padding * 2);
    lv_obj_remove_style_all(frame);
    lv_obj_set_style_bg_color(frame, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(frame, 0, 0);
    lv_obj_set_style_radius(frame, 0, 0);
    lv_obj_set_scrollable(frame, false);
    lv_obj_set_scroll_chain(frame, false);
    lv_obj_set_size(frame, outer_size, outer_size);
    lv_obj_set_pos(qr, padding, padding);
    lv_obj_set_style_bg_color(qr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(qr, LV_OPA_COVER, 0);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    lv_obj_update_layout(frame);
    return lv_obj_get_width(frame) == outer_size && lv_obj_get_height(frame) == outer_size;
}

bool panel_wifi_qr_layout_startup(
    lv_obj_t *screen, lv_obj_t *title, lv_obj_t *instruction,
    lv_obj_t *code_caption, lv_obj_t *code, lv_obj_t *frame, lv_obj_t *qr)
{
    if (screen == NULL || title == NULL || instruction == NULL ||
        code_caption == NULL || code == NULL || frame == NULL || qr == NULL ||
        lv_obj_get_parent(frame) != screen || lv_obj_get_parent(qr) != frame)
        return false;
    const int32_t width = lv_display_get_horizontal_resolution(lv_obj_get_display(screen));
    const int32_t height = lv_display_get_vertical_resolution(lv_obj_get_display(screen));
    if (width < 300 || height < 300) return false;
    lv_obj_t *labels[] = {title, instruction, code_caption, code};
    for (size_t i = 0U; i < sizeof(labels) / sizeof(labels[0]); ++i) {
        if (lv_obj_get_screen(labels[i]) != screen) return false;
        lv_obj_set_width(labels[i], width - 36);
        lv_obj_set_height(labels[i], LV_SIZE_CONTENT);
    }
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_align(instruction, LV_ALIGN_TOP_MID, 0, 58);
    lv_obj_align(code_caption, LV_ALIGN_TOP_MID, 0, 134);
    lv_obj_align(code, LV_ALIGN_TOP_MID, 0, 168);
    if (!panel_wifi_qr_layout_frame(frame, qr, PANEL_WIFI_QR_STARTUP_SIZE)) return false;
    lv_obj_align(frame, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_update_layout(screen);

    lv_area_t screen_area, title_area, instruction_area, caption_area, code_area, frame_area;
    lv_obj_get_coords(screen, &screen_area);
    lv_obj_get_coords(title, &title_area);
    lv_obj_get_coords(instruction, &instruction_area);
    lv_obj_get_coords(code_caption, &caption_area);
    lv_obj_get_coords(code, &code_area);
    lv_obj_get_coords(frame, &frame_area);
    const lv_area_t *content[] = {
        &title_area, &instruction_area, &caption_area, &code_area, &frame_area,
    };
    for (size_t i = 0U; i < sizeof(content) / sizeof(content[0]); ++i) {
        if (content[i]->x1 < screen_area.x1 || content[i]->y1 < screen_area.y1 ||
            content[i]->x2 > screen_area.x2 || content[i]->y2 > screen_area.y2)
            return false;
    }
    return title_area.y2 < instruction_area.y1 &&
        instruction_area.y2 < caption_area.y1 && caption_area.y2 < code_area.y1 &&
        code_area.y2 < frame_area.y1;
}

bool panel_wifi_qr_encode(lv_obj_t *qr, const char *ssid, const char *password)
{
    char payload[PANEL_WIFI_QR_MAX_PAYLOAD_SIZE];
    size_t payload_length = 0U;
    const bool built = panel_wifi_qr_build_payload(
        ssid, password, payload, sizeof(payload), &payload_length);
    const bool encoded = built && qr != NULL &&
        lv_qrcode_update(qr, payload, (uint32_t)payload_length) == LV_RESULT_OK;
    secure_clear(payload, sizeof(payload));
    return encoded;
}
