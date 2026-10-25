#!/usr/bin/env python3
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components/secure_bootstrap"
LVGL_SOURCE = Path(os.environ.get(
    "PATCH107_LVGL_SOURCE", ROOT / "managed_components/lvgl__lvgl"))
lock = (ROOT / "dependencies.lock").read_text(encoding="utf-8")
locked = re.search(r"(?ms)^  lvgl/lvgl:\n.*?^    component_hash: ([0-9a-f]{64})", lock)
assert locked, "LVGL component hash missing from dependency lock"
assert (LVGL_SOURCE / ".component_hash").read_text(encoding="utf-8").strip() == locked.group(1)
assert "version: 9.6.0~1" in (LVGL_SOURCE / "idf_component.yml").read_text(encoding="utf-8")
source = (COMPONENT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
writer = source[source.index("static bool set_startup_display_text("):
                source.index("static bool startup_status_active_get(")]
assert "panel_startup_compose_visible_text" in writer
assert "lv_label_set_text(s_detail, visible_status_text)" in writer
assert "lv_obj_add_flag(s_title, LV_OBJ_FLAG_HIDDEN)" in writer
assert "lv_obj_remove_flag(s_detail, LV_OBJ_FLAG_HIDDEN)" in writer
assert "lv_obj_align(s_detail, LV_ALIGN_TOP_MID" in writer
assert "lv_obj_set_style_pad_top(" in writer
assert "lv_obj_set_height(s_detail, (lv_coord_t)status_area_height)" in writer
assert "lv_obj_add_flag(s_title, LV_OBJ_FLAG_HIDDEN)" in writer
normal_view = source[source.index("static void set_display_text("):
                     source.index("static bool set_startup_display_text(")]
assert "lv_obj_remove_flag(s_title, LV_OBJ_FLAG_HIDDEN)" in normal_view
assert "lv_obj_set_style_pad_top(s_detail, 0, 0)" in normal_view
assert "lv_obj_set_height(s_detail, LV_SIZE_CONTENT)" in normal_view

with tempfile.TemporaryDirectory(prefix="patch107-startup-visible-") as temp:
    output = Path(temp)
    stubs = output / "stubs"
    stubs.mkdir()
    cmake = f'''cmake_minimum_required(VERSION 3.16)
project(patch107_startup_visible_render C CXX)
set(LV_CONF_SKIP ON CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
add_subdirectory("{LVGL_SOURCE}" lvgl)
target_compile_definitions(lvgl PUBLIC LV_COLOR_DEPTH=16 LV_FONT_MONTSERRAT_22=1 LV_FONT_MONTSERRAT_28=1 LV_USE_QRCODE=1 LV_USE_LOG=0 LV_MEM_SIZE=262144 LV_DRAW_BUF_STRIDE_ALIGN=1)
target_compile_options(lvgl PRIVATE -w)
add_executable(startup_visible_render
  "{COMPONENT / 'panel_startup_status.c'}"
  "{COMPONENT / 'homey_panel_font_22.c'}"
  "{COMPONENT / 'test_host/test_patch107_startup_visible_render.c'}")
target_include_directories(startup_visible_render PRIVATE "{COMPONENT / 'include'}")
target_link_libraries(startup_visible_render PRIVATE lvgl m)
'''
    (output / "CMakeLists.txt").write_text(cmake, encoding="utf-8")
    configure = subprocess.run(
        ["cmake", "-S", str(output), "-B", str(output / "build"), "-G", "Ninja"],
        text=True, capture_output=True)
    assert configure.returncode == 0, configure.stderr[-6000:]
    build = subprocess.run(
        ["cmake", "--build", str(output / "build"), "-j", "8"],
        text=True, capture_output=True)
    assert build.returncode == 0, build.stderr[-6000:]
    render = subprocess.run(
        [str(output / "build/startup_visible_render")], text=True, capture_output=True)
    assert render.returncode == 0, render.stdout + render.stderr
    assert "PATCH107_STARTUP_DETAIL_PIXEL_RENDER_PASS" in render.stdout

print("PATCH107_STARTUP_VISIBLE_RENDER_TESTS PASS")
