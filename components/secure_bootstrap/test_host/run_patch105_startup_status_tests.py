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
font_source = (ROOT / "homey_panel_font_22.c").read_text(encoding="utf-8")
font_header = (ROOT / "include" / "homey_panel_font_22.h").read_text(encoding="utf-8")
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
assert '"homey_panel_font_22.c"' in cmake
assert "LV_FONT_DECLARE(homey_panel_font_22)" in font_header
assert "generated at 22 px" in font_source and ".line_height = 26" in font_source
assert "homey_panel_font_22" in source
assert "lv_font_montserrat_28" not in source[source.index("static bool set_startup_display_text("):source.index("static void set_reconfigure_button_visible(")]
assert "lv_label_set_long_mode(s_title, LV_LABEL_LONG_WRAP)" in source
assert "lv_label_set_long_mode(s_detail, LV_LABEL_LONG_WRAP)" in source
assert "panel_startup_layout_compute" in source
assert "if (!layout_changed)" in source
assert "panel_startup_compose_visible_text" in source
assert "panel_startup_wait_update(" in source
assert "&s_homey_data_wait, wifi_online, verified_ready, now_ms" in source
assert "panel_startup_wait_expired(" in source
assert "PANEL_STARTUP_LONG_WAIT_MS 30000U" in (ROOT / "include" / "panel_startup_status.h").read_text(encoding="utf-8")
assert "lv_label_set_text(s_detail, visible_status_text)" in source
assert "const uint16_t status_area_top = TOUCH_Y + TOUCH_HEIGHT + STARTUP_WIFI_BUTTON_GAP" in source
assert "const uint16_t status_area_height = DISPLAY_HEIGHT -" in source
assert "lv_obj_set_style_pad_top(" in source
assert "lv_obj_set_height(s_detail, (lv_coord_t)status_area_height)" in source
assert "lv_obj_add_flag(s_title, LV_OBJ_FLAG_HIDDEN)" in source
assert "STARTUP_STATUS_BOTTOM_MARGIN 24" in source
assert '"Wi-Fi ansluten"' in source
assert '"Homey READY"' in source
assert '"Hämtar Homey-data..."' in source
assert '"Hämtningen tar längre tid än väntat"' in source
assert '"Försöker igen..."' in source
assert '"Homey-data kunde inte hämtas"' in source
assert '"Kontrollera anslutningen eller Homey-inloggningen"' in source

# Generated LVGL sparse cmap includes printable ASCII and Swedish letters.
font_cmap = font_source.split("static const uint16_t unicode_list_0[] = {", 1)[1].split("\n};", 1)[0]
font_codepoints = {0x20 + int(cp, 16) for cp in re.findall(r"0x([0-9A-Fa-f]{4})", font_cmap)}
assert all(ord(ch) in font_codepoints for ch in "ÅÄÖåäö")
status_strings = [
    "Wi-Fi ansluten", "Hämtar Homey-data...", "Väntar på verifierad live-data",
    "Hämtningen tar längre tid än väntat", "Försöker igen...", "Homey READY",
    "Homey-data kunde inte hämtas", "Kontrollera anslutningen eller Homey-inloggningen",
]
assert all(ord(ch) in font_codepoints for text in status_strings for ch in text)

assert "panel_startup_compose_visible_text" in source
assert 480 // 2 - 127 - 85 == 28  # Logo top.
assert 480 // 2 - 127 + 85 == 198  # Logo bottom.
assert 214 - 198 == 16  # Logo-to-button visual gap.

assert "bsp_display_lock(0)" in source
assert "vTaskDelay" not in source[source.index("static void update_homey_data_boot_view("):source.index("static void set_reconfigure_button_visible(")]
assert "panel_show_startup_shell" not in source

# Every automatically restored/live Homey label previously wrote through the
# generic display callback and replaced the startup labels outside the timer.
display_gate = source[source.index("static void panel_provisioning_display_update("):source.index("static esp_err_t apply_actions(")]
assert "startup_status_active_get()" in display_gate
assert "set_display_text(title, detail, display_code)" in display_gate
assert 'phone_provisioning_set_display_callback(panel_provisioning_display_update)' in source
assert 'detail, "Status: Ansluten"' in display_gate
assert 'detail, "Status: READY"' in display_gate
assert "startup_status_active_set(false)" in source[source.index("static bool panel_show_dashboard("):source.index("static void update_homey_data_boot_view(")]
assert "state == ATHOM_HOMEY_DATA_READY && snapshot_fresh" in source
assert '"Ansluter till Wi-Fi...", "Återansluter till sparat nätverk"' in source
assert "PANEL_STARTUP_ACTION_WIFI_DISCONNECTED" in source
assert "panel_startup_status_render_failed(\n                &s_startup_status, PANEL_STARTUP_ACTION_SHOW_DASHBOARD)" in source
