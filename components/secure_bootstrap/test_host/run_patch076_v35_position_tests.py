#!/usr/bin/env python3
"""Offline characterization; no firmware build, network, NVS or panel access."""
from pathlib import Path
import argparse, subprocess, tempfile, re
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--output-dir',type=Path);p.add_argument('--poll-source',type=Path,help='Exact offline baseline bootstrap source');args=p.parse_args()
out=args.output_dir or Path(tempfile.mkdtemp(prefix='PATCH076_V35_HOST_',dir=Path.home()/'Downloads'))
out.mkdir(parents=True,exist_ok=True);stubs=out/'stubs';stubs.mkdir(exist_ok=True)
(stubs/'esp_log.h').write_text('#pragma once\n#include <stdio.h>\n#define ESP_LOGI(tag, ...) do {(void)(tag);if(0)printf(__VA_ARGS__);}while(0)\n')
(stubs/'esp_timer.h').write_text('#pragma once\n#include <stdint.h>\nstatic inline int64_t esp_timer_get_time(void){return (int64_t)lv_tick_get()*1000;}\n')
component=root/'components/secure_bootstrap'
sources=['panel_ui_model.c','panel_homey_dashboard_binding.c','panel_roller_model.c','panel_ui_store.c','panel_homey_read_snapshot.c','panel_homey_alias_store.c','homey_panel_font_16.c','homey_panel_font_18.c','homey_panel_font_22.c','test_host/test_patch076_v35_position_lifecycle.c']
cjson=next((root/'managed_components/espressif__cjson').rglob('cJSON.c'))
# Compile the exact production poll function with controlled source/lock stubs.
esp=(args.poll_source or component/'secure_bootstrap_esp.c').read_text()
def extract_from(text, signature):
 start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
poll_stub = r"""
typedef enum {ATHOM_HOMEY_DATA_READY,ATHOM_HOMEY_DATA_ERROR} athom_homey_data_state_t;
static bool test_lock_allowed=true;
static bool s_panel_dashboard_visible=true;
static panel_ui_connection_state_t s_dashboard_connection_state=PANEL_UI_CONNECTION_CONNECTING;
static bool s_dashboard_homey_data_ready=false;
static uint64_t s_homey_dashboard_last_poll_ms;
static struct {unsigned homey_model_changed_count,homey_favorites_changed_count,homey_refresh_requests;} s_panel_perf;
#define s_panel_ui ui
#define s_panel_model model
#define s_homey_dashboard_state dashboard
#define HOMEY_DASHBOARD_POLL_INTERVAL_MS 1000
static athom_homey_data_state_t athom_oauth_runtime_homey_data_state(void){return ATHOM_HOMEY_DATA_READY;}
/* The Patch085 awning authority/readback hooks are outside this legacy V35
 * position-lifecycle test. Keep them inert in the source-extracted poll shim. */
static void athom_oauth_runtime_awning_snapshot_observed(void){}
static bool athom_oauth_runtime_awning_position_uncertain(size_t widget_index){(void)widget_index;return false;}
static bool phone_provisioning_wifi_online(void){return true;}
static panel_homey_read_result_t athom_cloud_copy_device_snapshot(uint64_t t,panel_homey_read_snapshot_t *s){return panel_homey_snapshot_copy(&store,t,s);}
static bool panel_display_lock_traced(unsigned timeout,bool homey,uint64_t *t){(void)timeout;(void)homey;*t=0;return test_lock_allowed;}
static void panel_display_unlock_traced(uint64_t t){(void)t;}
"""
for signature in ['static bool dashboard_snapshot_poll_allowed(', 'static bool dashboard_favorites_apply_allowed(', 'static bool dashboard_poll_requires_refresh(', 'static void poll_homey_dashboard_if_due(']:
 poll_stub += '\n'+extract_from(esp,signature)
(stubs/'poll_actual.inc').write_text(poll_stub)
cmake=f'''cmake_minimum_required(VERSION 3.16)
project(v35_host C CXX)
set(LV_CONF_SKIP ON CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
add_subdirectory("{root/'managed_components/lvgl__lvgl'}" lvgl)
target_compile_definitions(lvgl PUBLIC LV_FONT_MONTSERRAT_22=1 LV_FONT_MONTSERRAT_28=1 LV_USE_QRCODE=1 LV_USE_LOG=0 LV_MEM_SIZE=262144 LV_DRAW_BUF_STRIDE_ALIGN=1)
add_executable(lifecycle {' '.join('"'+str(component/s)+'"' for s in sources)} "{cjson}")
target_compile_definitions(lifecycle PRIVATE _POSIX_C_SOURCE=200809L)
target_include_directories(lifecycle PRIVATE "{component/'include'}" "{stubs}" "{cjson.parent}")
target_link_libraries(lifecycle PRIVATE lvgl m)
'''
(out/'CMakeLists.txt').write_text(cmake)
for cmd in (['cmake','-S',str(out),'-B',str(out/'build'),'-G','Ninja'],['cmake','--build',str(out/'build'),'-j','8'],[str(out/'build/lifecycle')]):subprocess.run(cmd,check=True)
# Run unchanged source-extracted periodic scheduling functions with synthetic queue results.
text=(component/'athom_oauth_runtime.c').read_text()
def extract(signature):
 start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
parts=[extract('static bool periodic_refresh_scheduler_should_attempt('),extract('static uint64_t periodic_refresh_scheduler_defer_ms('),extract('static void periodic_refresh_scheduler_record_queue_result(')]
block=text[text.index('typedef struct {\n    bool snapshot_seen;'):text.index('static bool periodic_refresh_scheduler_should_attempt(')]
macros='\n'.join(re.findall(r'^#define PERIODIC_REFRESH_(?:INTERVAL|BUSY_DEFER|COOLDOWN)_MS .*$',text,re.M))
source='#include <assert.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include <stddef.h>\n#include <stdio.h>\ntypedef enum {ATHOM_REFRESH_QUEUE_OK,ATHOM_REFRESH_QUEUE_BUSY,ATHOM_REFRESH_QUEUE_NOT_READY,ATHOM_REFRESH_QUEUE_FAILED} athom_refresh_queue_result_t;\n'+macros+'\n'+block+'\n'.join(parts)+'''
int main(void){
 periodic_refresh_scheduler_state_t s={0};
 assert(!periodic_refresh_scheduler_should_attempt(&s,false,0,999999,999999));
 assert(!periodic_refresh_scheduler_should_attempt(&s,true,1000,59999,60999));
 assert(periodic_refresh_scheduler_should_attempt(&s,true,1000,60000,61000));
 periodic_refresh_scheduler_record_queue_result(&s,ATHOM_REFRESH_QUEUE_BUSY,61000);
 assert(!periodic_refresh_scheduler_should_attempt(&s,true,1000,64999,65999));
 assert(periodic_refresh_scheduler_should_attempt(&s,true,1000,65000,66000));
 periodic_refresh_scheduler_record_queue_result(&s,ATHOM_REFRESH_QUEUE_NOT_READY,66000);
 assert(periodic_refresh_scheduler_should_attempt(&s,true,1000,70000,71000));
 periodic_refresh_scheduler_record_queue_result(&s,ATHOM_REFRESH_QUEUE_OK,71000);
 assert(!periodic_refresh_scheduler_should_attempt(&s,true,1000,99999,100999));
 assert(periodic_refresh_scheduler_should_attempt(&s,true,1000,100000,101000));
 periodic_refresh_scheduler_record_queue_result(&s,ATHOM_REFRESH_QUEUE_FAILED,101000);
 assert(periodic_refresh_scheduler_should_attempt(&s,true,1000,130000,131000));
 assert(!periodic_refresh_scheduler_should_attempt(&s,true,132000,0,132000));
 assert(periodic_refresh_scheduler_should_attempt(&s,true,132000,60000,192000));
 puts("V35_REAL_PERIODIC_SCHEDULER_DEFER_AND_RECOVERY PASS");return 0;
}
'''
(out/'scheduler.c').write_text(source)
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(out/'scheduler.c'),'-o',str(out/'scheduler')],check=True)
subprocess.run([str(out/'scheduler')],check=True)
