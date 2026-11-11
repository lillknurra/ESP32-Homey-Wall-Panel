#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
test_dir = Path(__file__).resolve().parent

with tempfile.TemporaryDirectory(prefix="patch075-widget-") as temp_dir:
    binary = Path(temp_dir) / "test_panel_roller_model"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(root / "include"), str(root / "panel_roller_model.c"),
        str(test_dir / "test_panel_roller_model.c"), "-o", str(binary),
        "-lm",
    ], check=True)
    subprocess.run([str(binary)], check=True)

ui = (root / "panel_ui.c").read_text(encoding="utf-8")
model = (root / "panel_ui_model.c").read_text(encoding="utf-8")
binding = (root / "panel_homey_dashboard_binding.c").read_text(encoding="utf-8")
snapshot = (root / "panel_homey_read_snapshot.c").read_text(encoding="utf-8")
header = (root / "include" / "panel_ui_model.h").read_text(encoding="utf-8")

assert "#define W 480" in ui and "#define H 480" in ui
assert "#define PANEL_UI_PAGE_COUNT 3U" in header
assert '"Balkongmarkis"' in model and '"Ytterbelysning"' in model
assert '"Dekorbelysning"' in model and '"Verisure"' in model
assert '"Status ej verifierad"' in ui
assert "ui->date_label" not in ui and "lv_label_set_text(ui->date_label" not in ui
assert '"Ansluter"' in ui and '"Ansluten"' in ui
assert "navigation_button_new(top, NAV_HAMBURGER, open_settings_event, ui)" in ui
assert "lv_obj_set_size(card, PANEL_UI_WIDGET_LAYOUT[index].width, PANEL_UI_WIDGET_LAYOUT[index].height)" in ui
assert "lv_obj_set_size(card, LIGHT_CARD_W, LIGHT_CARD_H)" in ui
assert "lv_obj_set_pos(card, placement->x, placement->y)" in ui
markis_controls = ui[ui.index("static lv_obj_t *create_markis_control"):ui.index("static lv_obj_t *create_roller_card")]
assert "lv_button_create(card)" in markis_controls
assert "stop_ring" in markis_controls and "stop_mark" in markis_controls
assert "lv_line_create(control)" in markis_controls
assert "LV_STATE_DISABLED" in markis_controls
assert '"UPP"' in ui and '"STOPP"' in ui and '"NED"' in ui
assert "lv_obj_add_event_cb(control" not in markis_controls
roller_zone = ui[ui.index("static lv_obj_t *create_roller_zone"):ui.index("static lv_obj_t *create_markis_control")]
assert "lv_obj_add_event_cb(zone, roller_feedback_zone_event" in roller_zone
assert "if (index == 0U)" in ui[ui.index("static lv_obj_t *create_roller_card"):ui.index("static lv_obj_t *create_light_card")]
markis_render = ui[ui.index("static void render_roller_widget"):ui.index("static void render_light_icon")]
assert "index == 0U" in markis_render
assert "lv_obj_add_flag(ui->widget_status[index], LV_OBJ_FLAG_HIDDEN)" in markis_render
assert "lv_obj_add_flag(ui->roller_fills[index][line], LV_OBJ_FLAG_HIDDEN)" in markis_render
assert "create_roller_zone(card, ui, index, false, true, 24)" in ui
assert "create_roller_zone(card, ui, index, true, false, 116)" in ui
assert "request_light_toggle" not in ui[ui.index("static lv_obj_t *create_roller_zone"):ui.index("static lv_obj_t *create_light_card")]
roller_zone = ui[ui.index("static lv_obj_t *create_roller_zone"):ui.index("static lv_obj_t *create_roller_card")]
assert "lv_line_create(zone)" in roller_zone
assert "lv_line_set_points" in roller_zone
assert "lv_obj_center(arrow)" in roller_zone
assert "label_new(zone" not in roller_zone
assert '"↑"' not in ui and '"↓"' not in ui
assert "ROLLER_ARROW_UP_POINTS" in ui and "ROLLER_ARROW_DOWN_POINTS" in ui
assert "lv_obj_set_size(zone, ROLLER_CARD_W - 12, 44)" in roller_zone
assert "lv_obj_set_pos(zone, 6, y)" in roller_zone
assert '"Otillgänglig"' in ui
assert '"Åtgärd krävs"' in ui and '"Okänd"' in ui
assert 'text = "Pågår"' not in ui
assert "ROLLER_PRESENTATION_POSITION" in markis_render
assert "ui->roller_guard_state[1U] = PANEL_UI_AWNING_GUARD_UNINITIALIZED" in ui
assert "ui->roller_guard_state[2U] = PANEL_UI_AWNING_GUARD_UNINITIALIZED" in ui
assert "lv_obj_add_flag(ui->roller_lines[index][line], LV_OBJ_FLAG_HIDDEN)" in ui
assert "const bool availability_known = cJSON_IsBool(available_json);" in snapshot
assert "item->availability_known = availability_known;" in snapshot
assert "LV_STATE_DISABLED" in ui
assert "panel_roller_model_request_step" not in ui
assert "request_light_toggle" in ui

# Exercise the production dashboard-binding and UI-model path instead of
# asserting a particular stale-result source layout. This covers retaining a
# last-confirmed display value while invalidating current readback authority.
with tempfile.TemporaryDirectory(prefix="patch075-dashboard-") as temp_dir:
    binding_binary = Path(temp_dir) / "test_panel_homey_dashboard_binding"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(root / "include"),
        str(root / "panel_homey_dashboard_binding.c"),
        str(root / "panel_ui_model.c"),
        str(root / "panel_roller_model.c"),
        str(root / "panel_ui_store.c"),
        str(test_dir / "test_panel_homey_dashboard_binding.c"),
        "-lm", "-o", str(binding_binary),
    ], check=True)
    subprocess.run([str(binding_binary)], check=True)

font16 = (root / "homey_panel_font_16.c").read_text(encoding="utf-8")
font_map = font16[font16.index("static const uint16_t unicode_list_0[]"):font16.index("static const lv_font_fmt_txt_cmap_t cmaps[]")]
assert "0x00c4" in font_map.lower(), "Custom font must map U+00E4 lowercase ä."
glyph_section = font16[font16.index("static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[]"):font16.index("static const uint16_t unicode_list_0[]")]
glyphs = [tuple(map(int, match)) for match in __import__("re").findall(
    r"\{\.bitmap_index = (\d+), \.adv_w = (\d+), \.box_w = (\d+), \.box_h = (\d+), \.ofs_x = (-?\d+), \.ofs_y = (-?\d+)\}",
    glyph_section,
)]
unicode_offsets = [int(value, 16) for value in __import__("re").findall(r"0x([0-9a-fA-F]{4})", font_map)]
glyph_by_codepoint = {offset + 0x20: glyphs[index + 1] for index, offset in enumerate(unicode_offsets)}
message_width_px = sum(glyph_by_codepoint[ord(char)][1] for char in "Otillgänglig") / 16.0
assert message_width_px <= 138, f"Otillgänglig width {message_width_px}px exceeds the 138px status field"

font22 = (root / "homey_panel_font_22.c").read_text(encoding="utf-8")
font22_map = font22[font22.index("static const uint16_t unicode_list_0[]"):font22.index("static const lv_font_fmt_txt_cmap_t cmaps[]")]
font22_glyph_section = font22[font22.index("static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[]"):font22.index("static const uint16_t unicode_list_0[]")]
font22_glyphs = [tuple(map(int, match)) for match in __import__("re").findall(
    r"\{\.bitmap_index = (\d+), \.adv_w = (\d+), \.box_w = (\d+), \.box_h = (\d+), \.ofs_x = (-?\d+), \.ofs_y = (-?\d+)\}",
    font22_glyph_section,
)]
font22_offsets = [int(value, 16) for value in __import__("re").findall(r"0x([0-9a-fA-F]{4})", font22_map)]
font22_by_codepoint = {offset + 0x20: font22_glyphs[index + 1] for index, offset in enumerate(font22_offsets)}
for guard_text in ("Åtgärd krävs", "Okänd", "Otillgänglig"):
    width_px = sum(font22_by_codepoint[ord(char)][1] for char in guard_text) / 16.0
    assert width_px <= 210, f"{guard_text} width {width_px}px exceeds the 210px roller status field"

print("PATCH075_WIDGET_SOURCE_INVARIANTS PASS")
