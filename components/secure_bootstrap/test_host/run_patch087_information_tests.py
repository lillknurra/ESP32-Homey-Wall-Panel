#!/usr/bin/env python3
"""Actual formatter/provider/LVGL tests. Offline synthetic fixtures; unique outputs."""
from pathlib import Path
import argparse, os, re, subprocess, tempfile
p=argparse.ArgumentParser();p.add_argument('--output-dir',type=Path);args=p.parse_args()
root=Path(__file__).resolve().parents[3]; c=root/'components/secure_bootstrap'
out=args.output_dir or Path(tempfile.mkdtemp(prefix='PATCH087_HOST_',dir=Path.home()/'Downloads'))
if args.output_dir:out.mkdir(parents=True,exist_ok=False)
lvgl=Path(os.environ.get('PATCH089_LVGL_SOURCE',root/'managed_components/lvgl__lvgl'))
lock=(root/'dependencies.lock').read_text(encoding='utf-8')
component_hash=(lvgl/'.component_hash').read_text(encoding='utf-8').strip() if lvgl.is_dir() else ''
component_yml=(lvgl/'idf_component.yml').read_text(encoding='utf-8') if lvgl.is_dir() else ''
match=re.search(r'(?ms)^  lvgl/lvgl:\n.*?^    component_hash: ([0-9a-f]{64})\n.*?^    version: ([^\n]+)',lock)
assert match and component_hash==match.group(1), 'LVGL source does not match dependency lock'
assert 'version: 9.6.0~1' in component_yml, 'unexpected LVGL component version'
def run(cmd):subprocess.run([str(s) for s in cmd],check=True)
run(['cc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',c/'include',c/'panel_information.c',c/'test_host/test_patch087_information.c','-o',out/'format'])
run([out/'format'])
esp=(c/'secure_bootstrap_esp.c').read_text();start=esp.index('static bool panel_read_information(');brace=esp.index('{',start);end=brace+1;depth=1
while depth:depth+=(esp[end]=='{')-(esp[end]=='}');end+=1
provider=esp[start:end]
prefix=r'''
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "panel_information.h"
#include "secure_bootstrap.h"
typedef struct {char version[32],date[16],time[16],idf_ver[32];} esp_app_desc_t;
static esp_app_desc_t app={.version="fixture-version",.idf_ver="v6.0.1"};
static panel_information_snapshot_t s_information_snapshot;
static bool s_panel_timezone_ready=true, online=true, synced=true, session=true;
static unsigned state=2, heap_reads, app_reads, wifi_reads;
static int64_t esp_timer_get_time(void){return 123000000LL;}
static bool phone_provisioning_wifi_online(void){wifi_reads++;return online;}
static bool wifi_session_snapshot(uint32_t *g,secure_bootstrap_session_phase_t *p)
{*g=2;*p=SECURE_BOOTSTRAP_SESSION_ACTIVE;return session;}
static unsigned athom_oauth_runtime_homey_data_state(void){return state;}
static const esp_app_desc_t *esp_app_get_description(void){app_reads++;return &app;}
static bool sntp_time_is_available(void){return synced;}
static bool panel_clock_epoch_is_valid(time_t t){(void)t;return true;}
#define MALLOC_CAP_INTERNAL 1U
#define MALLOC_CAP_8BIT 2U
#define MALLOC_CAP_SPIRAM 4U
static size_t heap_caps_get_free_size(uint32_t c){heap_reads++;return c==3?10000:6000000;}
static size_t heap_caps_get_minimum_free_size(uint32_t c){(void)c;heap_reads++;return 500;}
'''
suffix=r'''
int main(void){
 panel_information_t i;assert(!panel_read_information(NULL,PANEL_INFORMATION_COUNT,&i));
 assert(!panel_read_information(NULL,PANEL_INFORMATION_WIFI,NULL));
 assert(panel_read_information(NULL,PANEL_INFORMATION_WIFI,&i));assert(i.wifi_online&&i.provisioning_active&&heap_reads==0&&app_reads==0);
 online=false;session=false;assert(panel_read_information(NULL,PANEL_INFORMATION_WIFI,&i));assert(!i.wifi_online&&!i.provisioning_active);
 panel_information_observe_snapshot(&s_information_snapshot,PANEL_HOMEY_READ_OK,122000,7);
 assert(panel_read_information(NULL,PANEL_INFORMATION_HOMEY,&i));assert(i.homey_state==2&&i.snapshot.generation==7&&!i.wifi_online);
 state=3;assert(panel_read_information(NULL,PANEL_INFORMATION_HOMEY,&i));assert(i.homey_state==3);
 assert(panel_read_information(NULL,PANEL_INFORMATION_FIRMWARE,&i));assert(!strcmp(i.version,"fixture-version")&&app_reads==1&&heap_reads==0);
 assert(panel_read_information(NULL,PANEL_INFORMATION_TIME,&i));assert(i.time_synchronized&&i.swedish_timezone);
 synced=false;s_panel_timezone_ready=false;assert(panel_read_information(NULL,PANEL_INFORMATION_TIME,&i));assert(!i.time_synchronized&&!i.swedish_timezone);
 assert(panel_read_information(NULL,PANEL_INFORMATION_SYSTEM,&i));assert(i.now_ms==123000&&i.internal_free==10000&&i.internal_minimum==500&&i.psram_free==6000000&&heap_reads==3);
 puts("PATCH087_ACTUAL_RUNTIME_PROVIDER_ALLOWLIST PASS");
}
'''
(out/'provider.c').write_text(prefix+provider+suffix)
run(['cc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',c/'include',c/'panel_information.c',out/'provider.c','-o',out/'provider'])
run([out/'provider'])
sources=['panel_information.c','panel_ui_model.c','panel_homey_dashboard_binding.c','panel_roller_model.c','panel_ui_store.c','homey_panel_font_16.c','homey_panel_font_18.c','homey_panel_font_22.c','test_host/test_patch087_information_ui.c']
(out/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.16)
project(patch087_host C CXX)
set(LV_CONF_SKIP ON CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
add_subdirectory("{lvgl}" lvgl)
target_compile_definitions(lvgl PUBLIC LV_FONT_MONTSERRAT_22=1 LV_FONT_MONTSERRAT_28=1 LV_USE_QRCODE=1 LV_USE_LOG=0 LV_MEM_SIZE=262144 LV_DRAW_BUF_STRIDE_ALIGN=1)
add_executable(info_ui {' '.join('"'+str(c/s)+'"' for s in sources)})
target_include_directories(info_ui PRIVATE "{c/'include'}")
target_compile_definitions(info_ui PRIVATE _POSIX_C_SOURCE=200809L)
target_link_libraries(info_ui PRIVATE lvgl m)
''')
run(['cmake','-S',out,'-B',out/'build','-G','Ninja'])
run(['cmake','--build',out/'build','-j','8'])
subprocess.run([str(out/'build/info_ui')],check=True,cwd=out)
print('PATCH087_TARGETED_TESTS PASS')
