#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

assert 'if(PATCH107_STARTUP_TITLE_DIAG OR "$ENV{PATCH107_STARTUP_TITLE_DIAG}")' in cmake
assert "PATCH107_STARTUP_TITLE_DIAG=1" in cmake
assert "#if defined(PATCH107_STARTUP_TITLE_DIAG)" in source
assert "bsp_display_lock(0)" in source[
    source.index("static bool set_startup_display_text("):source.index("static bool startup_status_active_get")
]
assert "lv_obj_get_screen(s_title) == lv_screen_active()" in source
assert "lv_obj_is_hidden(s_title)" in source
assert "lv_obj_get_style_text_font(s_title, LV_PART_MAIN)" in source
assert "strlen(rendered_text)" in source
assert "lv_obj_get_style_text_color(s_title, LV_PART_MAIN)" in source
assert "lv_obj_get_style_opa_recursive(s_title, LV_PART_MAIN)" in source
assert "lv_obj_get_coords(s_title, &capture->object_area)" in source
assert "lv_event_get_param(event)" in source
assert "area_source=LVGL_FLUSH_EVENT" in source
assert "lv_refr_now" not in source
assert "rendered_text," not in source
assert "PATCH107_STARTUP_TITLE_DIAG id=%s" in source
port_source = (ROOT.parents[1] / "managed_components" / "espressif__esp_lvgl_port" /
              "src" / "lvgl9" / "esp_lvgl_port.c").read_text(encoding="utf-8")
task_body = port_source[port_source.index("static void lvgl_port_task(void *arg)"):]
assert "if (lv_display_get_default() && lvgl_port_lock(0))" in task_body
assert "task_delay_ms = lv_timer_handler();" in task_body
assert "lvgl_port_unlock();" in task_body
bsp_source = (ROOT.parents[1] / "managed_components" /
              "waveshare__esp32_s3_touch_lcd_4b" /
              "esp32_s3_touch_lcd_4b.c").read_text(encoding="utf-8")
assert "return lvgl_port_lock(timeout_ms);" in bsp_source

with tempfile.TemporaryDirectory(prefix="patch107-title-diag-") as temp:
    binary = Path(temp) / "test_patch107_startup_title_diag"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "include"),
        str(ROOT / "panel_startup_title_diag.c"),
        str(ROOT / "test_host" / "test_patch107_startup_title_diag.c"),
        "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)

print("PATCH107_STARTUP_TITLE_DIAGNOSTICS PASS")
