#!/usr/bin/env python3
"""Real LVGL partial-render regression, with no device or network access."""
from pathlib import Path
import argparse
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser()
parser.add_argument('--output-dir', type=Path)
args = parser.parse_args()
output = args.output_dir or Path(tempfile.mkdtemp(prefix='PATCH076_V30_HOST_', dir=Path.home() / 'Downloads'))
output.mkdir(parents=True, exist_ok=True)
stubs = output / 'stubs'
stubs.mkdir(exist_ok=True)
(stubs / 'esp_log.h').write_text('#pragma once\n#include <stdio.h>\n#define ESP_LOGI(tag, ...) do { (void)(tag); if (0) printf(__VA_ARGS__); } while (0)\n')
(stubs / 'esp_timer.h').write_text('#pragma once\n#include <stdint.h>\nstatic inline int64_t esp_timer_get_time(void) { return (int64_t)lv_tick_get() * 1000; }\n')
sources = ['panel_ui_model.c', 'panel_homey_dashboard_binding.c', 'panel_roller_model.c', 'panel_ui_store.c',
           'homey_panel_font_16.c', 'homey_panel_font_18.c', 'homey_panel_font_22.c',
           'test_host/test_patch076_v30_render.c']
component = root / 'components/secure_bootstrap'
cmake = f'''cmake_minimum_required(VERSION 3.16)
project(patch076_v30_host C CXX)
set(LV_CONF_SKIP ON CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
add_subdirectory("{root / 'managed_components/lvgl__lvgl'}" lvgl)
# Host objects contain 64-bit pointers; reserve a larger host-only LVGL pool.
# This does not validate or change the firmware heap budget.
target_compile_definitions(lvgl PUBLIC LV_FONT_MONTSERRAT_22=1 LV_FONT_MONTSERRAT_28=1 LV_USE_QRCODE=1 LV_USE_LOG=0 LV_MEM_SIZE=262144 LV_DRAW_BUF_STRIDE_ALIGN=1)
add_executable(render_test {' '.join(chr(34) + str(component / s) + chr(34) for s in sources)})
target_include_directories(render_test PRIVATE "{component / 'include'}" "{stubs}")
target_link_libraries(render_test PRIVATE lvgl m)
'''
(output / 'CMakeLists.txt').write_text(cmake)
for command in (['cmake', '-S', str(output), '-B', str(output / 'build'), '-G', 'Ninja'],
                ['cmake', '--build', str(output / 'build'), '-j', '8'],
                [str(output / 'build/render_test')]):
    subprocess.run(command, check=True)
print('PATCH076_V30_RENDER_RUNNER PASS')
