#!/usr/bin/env python3
"""Real Favorites/LVGL host interleaving and presentation regression; offline only."""
from pathlib import Path
import argparse, subprocess, tempfile
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--output-dir',type=Path);p.add_argument('--baseline-dir',type=Path);p.add_argument('--measure-only',action='store_true');a=p.parse_args()
out=a.output_dir or Path(tempfile.mkdtemp(prefix='PATCH076_V37_HOST_',dir=Path.home()/'Downloads'));out.mkdir(parents=True,exist_ok=True)
stubs=out/'stubs';stubs.mkdir(exist_ok=True)
(stubs/'esp_log.h').write_text('#pragma once\n#include <stdio.h>\n#define ESP_LOGI(tag, ...) do {(void)(tag);if(0)printf(__VA_ARGS__);}while(0)\n')
(stubs/'esp_timer.h').write_text('#pragma once\n#include <stdint.h>\nstatic inline int64_t esp_timer_get_time(void){return (int64_t)lv_tick_get()*1000;}\n')
c=root/'components/secure_bootstrap'; baseline=a.baseline_dir
(stubs/'ui_actual.inc').write_text((baseline/'panel_ui.c' if baseline else c/'panel_ui.c').read_text())
# Execute the exact argument/identity boundary before any transport operation.
cloud=(c/'athom_cloud_client.c').read_text()
start=cloud.index('esp_err_t athom_cloud_select_and_connect(')
end=cloud.index('    diagnostic_set("homey_lookup", ESP_OK);',start)
prefix=cloud[start:end].replace('esp_err_t athom_cloud_select_and_connect(', 'static int select_identity_prefix(')
(stubs/'identity_actual.inc').write_text(prefix+'    return ESP_OK;\n}\n')

sources=[c/s for s in ['panel_ui_model.c','panel_homey_dashboard_binding.c','panel_roller_model.c','panel_ui_store.c','panel_information.c','homey_panel_font_16.c','homey_panel_font_18.c','homey_panel_font_22.c','test_host/test_patch076_v37_stability.c']]
sources += [baseline/'panel_homey_favorites.c' if baseline else c/'panel_homey_favorites.c',root/'managed_components/espressif__cjson/cJSON/cJSON.c']
cm=f'''cmake_minimum_required(VERSION 3.16)
project(v37_host C CXX)
set(LV_CONF_SKIP ON CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
add_subdirectory("{root/'managed_components/lvgl__lvgl'}" lvgl)
target_compile_definitions(lvgl PUBLIC LV_FONT_MONTSERRAT_22=1 LV_FONT_MONTSERRAT_28=1 LV_USE_QRCODE=1 LV_USE_LOG=0 LV_MEM_SIZE=262144 LV_DRAW_BUF_STRIDE_ALIGN=1)
add_executable(stability {' '.join('"'+str(s)+'"' for s in sources)})
target_compile_definitions(stability PRIVATE _POSIX_C_SOURCE=200809L)
target_include_directories(stability PRIVATE "{c/'include'}" "{stubs}" "{root/'managed_components/espressif__cjson/cJSON'}")
target_link_libraries(stability PRIVATE lvgl m)
'''
(out/'CMakeLists.txt').write_text(cm)
for cmd in [['cmake','-S',str(out),'-B',str(out/'build'),'-G','Ninja'],['cmake','--build',str(out/'build'),'-j','8'],[str(out/'build/stability')]+(['measure'] if a.measure_only else [])]:subprocess.run(cmd,check=True)
