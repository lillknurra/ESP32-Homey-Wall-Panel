#!/usr/bin/env python3
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="patch105-startup-") as temp:
    binary = Path(temp) / "test_patch105_startup_status"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "include"),
        str(ROOT / "panel_startup_status.c"),
        str(ROOT / "test_host" / "test_patch105_startup_status.c"),
        "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)

source = (ROOT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
logo_source = (ROOT / "panel_startup_logo.c").read_text(encoding="utf-8")
font_source = (ROOT / "homey_panel_font_27.c").read_text(encoding="utf-8")
font_header = (ROOT / "include" / "homey_panel_font_27.h").read_text(encoding="utf-8")
logo_data = (ROOT / "assets" / "panel_startup_logo_a8.bin").read_bytes()
assert '"panel_startup_status.c"' in cmake
assert '"panel_startup_logo.c"' in cmake
assert "target_add_binary_data" in cmake
assert len(logo_data) == 170 * 170
assert any(logo_data) and any(value == 0 for value in logo_data)
assert ".cf = LV_COLOR_FORMAT_A8" in logo_source
assert ".w = 170U" in logo_source and ".h = 170U" in logo_source
assert "lv_image_set_src(s_startup_logo, &panel_startup_logo_image)" in source
assert "STARTUP_LOGO_Y_OFFSET (-127)" in source
assert "TOUCH_Y 214" in source and "TOUCH_HEIGHT 52" in source
assert '"homey_panel_font_27.c"' in cmake
assert "LV_FONT_DECLARE(homey_panel_font_27)" in font_header
assert "Size: 27 px" in font_source and ".line_height = 32" in font_source
assert "homey_panel_font_27" in source
assert "lv_font_montserrat_28" not in source[source.index("static bool set_startup_display_text("):source.index("static void set_reconfigure_button_visible(")]
assert "lv_label_set_long_mode(s_title, LV_LABEL_LONG_WRAP)" in source
assert "lv_label_set_long_mode(s_detail, LV_LABEL_LONG_WRAP)" in source
assert "panel_startup_layout_compute" in source
assert "lv_obj_update_layout(s_title)" in source
assert "if (!layout_changed)" in source
assert "lv_obj_has_flag(s_detail, LV_OBJ_FLAG_HIDDEN)" in source
assert "lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, (lv_coord_t)layout.primary_y)" in source
assert "layout.secondary_y" in source and "STARTUP_STATUS_TEXT_GAP 8" in source
assert "STARTUP_STATUS_BOTTOM_MARGIN 24" in source
assert '"Wi-Fi ansluten"' in source
assert '"Homey READY"' in source
assert '"Hämtar Homey-data..."' in source
assert '"Hämtningen tar längre tid än väntat"' in source
assert '"Försöker igen..."' in source
assert '"Homey-data kunde inte hämtas"' in source
assert '"Kontrollera anslutningen eller Homey-inloggningen"' in source

# Generated LVGL cmap covers printable ASCII and exactly the Swedish letters.
font_codepoints = {int(cp, 16) for cp in re.findall(r'/\* U\+([0-9A-Fa-f]{4,}) "', font_source)}
assert all(ord(ch) in font_codepoints for ch in "ÅÄÖåäö")
status_strings = [
    "Wi-Fi ansluten", "Hämtar Homey-data...", "Väntar på verifierad live-data",
    "Hämtningen tar längre tid än väntat", "Försöker igen...", "Homey READY",
    "Homey-data kunde inte hämtas", "Kontrollera anslutningen eller Homey-inloggningen",
]
assert all(ord(ch) in font_codepoints for text in status_strings for ch in text)

# Check line wrapping against the generated LVGL advance-width table. The font
# uses 1/16 px advance units; its declared line height is 32 px.
dsc = font_source.split("static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {", 1)[1].split("\n};", 1)[0]
advances = [int(value) for value in re.findall(r"\.adv_w\s*=\s*(\d+)", dsc)]
swedish_glyph_ids = {cp: 96 + i for i, cp in enumerate((0xC4, 0xC5, 0xD6, 0xE4, 0xE5, 0xF6))}
def text_width(text):
    total = 0
    for char in text:
        cp = ord(char)
        glyph_id = cp - 31 if 0x20 <= cp <= 0x7E else swedish_glyph_ids[cp]
        total += advances[glyph_id] / 16
    return total
def wrapped_lines(text, max_width):
    lines = []
    current = ""
    for word in text.split(" "):
        candidate = word if not current else current + " " + word
        if current and text_width(candidate) > max_width:
            lines.append(current)
            current = word
        else:
            current = candidate
    if current:
        lines.append(current)
    return lines
primary = [status_strings[i] for i in (0, 1, 3, 5, 6)]
secondary = [status_strings[i] for i in (2, 4, 7)]
for text in primary:
    assert max(text_width(word) for word in text.split(" ")) <= 444
    assert len(wrapped_lines(text, 444)) * 32 <= 3 * 32
for text in secondary:
    assert max(text_width(word) for word in text.split(" ")) <= 444
    assert len(wrapped_lines(text, 444)) * 26 <= 3 * 26
status_pairs = [
    ("Wi-Fi ansluten", ""),
    ("Hämtar Homey-data...", "Väntar på verifierad live-data"),
    ("Hämtningen tar längre tid än väntat", "Försöker igen..."),
    ("Homey READY", ""),
    ("Homey-data kunde inte hämtas", "Kontrollera anslutningen eller Homey-inloggningen"),
]
for title, detail in status_pairs:
    primary_height = len(wrapped_lines(title, 444)) * 32
    detail_height = len(wrapped_lines(detail, 444)) * 26 if detail else 0
    primary_y = 480 - 24 - primary_height
    secondary_y = primary_y - 8 - detail_height
    assert primary_y + primary_height == 456
    assert detail_height == 0 or secondary_y + detail_height + 8 == primary_y
    assert secondary_y >= 282  # Wi-Fi button bottom (266) plus 16 px.
assert 480 // 2 - 127 - 85 == 28  # Logo top.
assert 480 // 2 - 127 + 85 == 198  # Logo bottom.
assert 214 - 198 == 16  # Logo-to-button visual gap.

assert "bsp_display_lock(0)" in source
assert "vTaskDelay" not in source[source.index("static void update_homey_data_boot_view("):source.index("static void set_reconfigure_button_visible(")]
assert "panel_show_startup_shell" not in source
