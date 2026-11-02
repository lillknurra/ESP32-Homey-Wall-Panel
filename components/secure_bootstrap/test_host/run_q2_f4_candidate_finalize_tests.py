#!/usr/bin/env python3
"""Compile the real deferred finalize branch and transaction coordinator."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(os.environ.get("Q2_SOURCE_ROOT", Path(__file__).resolve().parents[1]))
ESP = ROOT / "secure_bootstrap_esp.c"
TXN = ROOT / "panel_wifi_transaction.c"
HEADER = ROOT / "include" / "panel_wifi_transaction.h"

def extract_block(source: str, anchor: str) -> str:
    start = source.index(anchor)
    brace = source.index("{", start)
    depth = 0
    for end in range(brace, len(source)):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
            if depth == 0:
                return source[start:end + 1]
    raise AssertionError(f"unterminated block {anchor}")

def extract_function(source: str, signature: str) -> str:
    search = 0
    while True:
        start = source.index(signature, search)
        brace = source.find("{", start + len(signature))
        semicolon = source.find(";", start + len(signature))
        if brace >= 0 and (semicolon < 0 or brace < semicolon):
            break
        search = start + len(signature)
    depth = 0
    for end in range(brace, len(source)):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
            if depth == 0:
                return source[start:end + 1]
    raise AssertionError(f"unterminated function {signature}")

source = ESP.read_text()
branch_anchor = "if (request.kind == DEFERRED_WIFI_WORK_FINALIZE_CANDIDATE)"
branch = extract_block(source, branch_anchor).replace("continue;", "break;")
marker_write = extract_function(source, "static panel_wifi_txn_result_t txn_marker_write(")
marker_clear = extract_function(source, "static panel_wifi_txn_result_t txn_marker_clear(")
header = HEADER.read_text()
if "panel_wifi_txn_finish_candidate" not in TXN.read_text():
    raise SystemExit("real candidate finalization function missing")
if 'marker_update=%s runtime_quarantine=true first_install_clear=%s' not in source:
    raise SystemExit("explicit marker-update failure reporting missing from production branch")

fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "panel_wifi_transaction.h"
typedef int esp_err_t;
typedef int portMUX_TYPE;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_ARG (-2)
#define ESP_ERR_INVALID_STATE (-3)
#define SECURE_BOOTSTRAP_SESSION_COMMITTING 2
#define WIFI_PERSIST_STAGE_VERIFY_CONNECT 3
#define WIFI_PERSIST_STAGE_IDLE 0
#define SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_GOT_IP 1
#define SECURE_BOOTSTRAP_WIFI_EVENT_RECOVERY_REQUIRED 2
#define DEFERRED_WIFI_WORK_FINALIZE_CANDIDATE 7
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
#define TAG "host"
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
static portMUX_TYPE s_wifi_session_mux;
static bool s_persist_active=true, s_persist_task_scheduled=true;
static unsigned s_persist_stage=WIFI_PERSIST_STAGE_VERIFY_CONNECT;
static uint32_t s_persist_session_generation=81, s_wifi_recovery_generation=81;
static bool s_deferred_finalize_queued=true;
static bool s_wifi_txn_marker_present=true;
static panel_wifi_recovery_slot_t s_wifi_recovery_slot;
static bool s_wifi_recovery_quarantine=false;
static bool s_wifi_recovery_backup_valid=true;
static bool s_wifi_recovery_can_restore=true, s_wifi_recovery_can_clear=false;
static bool s_candidate_valid=true;
static unsigned s_zero_calls, s_apply_calls, s_publish_ready_calls;
static unsigned s_marker_clear_calls, s_marker_write_calls;
static int s_marker_write_error=ESP_FAIL;
static bool s_marker_durable_present=true;
static bool s_marker_clear_erase_fails;
static bool s_marker_clear_readback_fails;
static int s_wifi_state;
static panel_wifi_txn_marker_t s_wifi_txn_marker={PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING,true,81};
typedef struct {uint32_t generation;int phase;} test_session_t;
typedef test_session_t secure_bootstrap_wifi_session_t;
typedef int secure_bootstrap_session_phase_t;
typedef struct {unsigned char payload[64];} wifi_config_t;
static wifi_config_t s_candidate_config;
static test_session_t s_wifi_session={81,SECURE_BOOTSTRAP_SESSION_COMMITTING};
typedef struct {int kind;uint32_t generation;} request_t;
static request_t request={DEFERRED_WIFI_WORK_FINALIZE_CANDIDATE,81};
static const char *esp_err_to_name(int e){return e==ESP_OK?"ESP_OK":"ESP_FAIL";}
static void secure_zero(void *p,size_t n){volatile unsigned char *b=p;while(n--)*b++=0;s_zero_calls++;}
static bool secure_bootstrap_wifi_session_finish_commit(test_session_t *s,uint32_t g){if(s->generation!=g||s->phase!=SECURE_BOOTSTRAP_SESSION_COMMITTING)return false;s->phase=0;return true;}
static unsigned wifi_transition_serialized(int event){if(event==SECURE_BOOTSTRAP_WIFI_EVENT_RECOVERY_REQUIRED)s_wifi_state=9;if(event==SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_GOT_IP){s_wifi_state=4;s_publish_ready_calls++;}return 0;}
static void apply_actions(unsigned a){(void)a;s_apply_calls++;}
static int wifi_txn_marker_clear_verified(uint32_t generation){assert(generation==81);s_marker_clear_calls++;if(s_marker_clear_erase_fails)return ESP_FAIL;s_marker_durable_present=false;if(s_marker_clear_readback_fails)return ESP_FAIL;return ESP_OK;}
static int wifi_txn_marker_write_verified(const panel_wifi_txn_marker_t *m){assert(m->generation==81);s_marker_write_calls++;if(s_marker_write_error!=ESP_OK)return s_marker_write_error;s_wifi_txn_marker=*m;s_wifi_txn_marker_present=true;s_marker_durable_present=true;return ESP_OK;}
static int txn_backup_clear(void *ctx){(void)ctx;s_wifi_recovery_backup_valid=false;return ESP_OK;}
static int txn_backup_prepare(void *ctx,bool prev){(void)ctx;(void)prev;return ESP_OK;}
static int txn_marker_write_recovery_takeover(const panel_wifi_txn_marker_t *m){(void)m;return ESP_FAIL;}
static int txn_executor_available(void *ctx,uint32_t g){(void)ctx;(void)g;return true;}
static int txn_candidate(void *ctx,uint32_t g){(void)ctx;(void)g;return ESP_OK;}
static int txn_restore(void *ctx,uint32_t g){(void)ctx;(void)g;return ESP_OK;}
static int txn_clear_first(void *ctx,uint32_t g){(void)ctx;(void)g;return ESP_OK;}
static int txn_marker_clear(void *context,uint32_t generation);
static int txn_marker_write(void *context,const panel_wifi_txn_marker_t *marker);
static int txn_marker_clear_callback(void *ctx,uint32_t generation){(void)ctx;return txn_marker_clear(NULL,generation);}
static int txn_marker_write_callback(void *ctx,const panel_wifi_txn_marker_t *m){(void)ctx;return txn_marker_write(NULL,m);}
static bool txn_executor_callback(void *ctx,uint32_t g){(void)ctx;(void)g;return true;}
static int txn_candidate_callback(void *ctx,uint32_t g){(void)ctx;(void)g;return ESP_OK;}
static int txn_restore_callback(void *ctx,uint32_t g){(void)ctx;(void)g;return ESP_OK;}
static int txn_first_clear_callback(void *ctx,uint32_t g){(void)ctx;(void)g;return ESP_OK;}
static panel_wifi_txn_ops_t wifi_txn_ops(void){return (panel_wifi_txn_ops_t){.context=&s_wifi_state,.backup_prepare_verified=txn_backup_prepare,.marker_write_verified=txn_marker_write_callback,.marker_clear_verified=txn_marker_clear_callback,.backup_clear=txn_backup_clear,.executor_available=txn_executor_callback,.candidate_persist_verified=txn_candidate_callback,.restore_previous_verified=txn_restore_callback,.clear_first_install_verified=txn_first_clear_callback};}
'''
# Construct a small real callback surface using the extracted production adapters.
fixture += "\n" + marker_write + "\n" + marker_clear + "\n"
main = r'''
static void run_branch(void){while(1){
BRANCH
break;}}
static void reset_case(bool erase_fails,bool readback_fails){
 s_persist_active=true;s_persist_task_scheduled=true;s_persist_stage=WIFI_PERSIST_STAGE_VERIFY_CONNECT;
 s_persist_session_generation=81;s_wifi_recovery_generation=81;s_deferred_finalize_queued=true;
 s_wifi_txn_marker_present=true;s_wifi_recovery_quarantine=false;s_wifi_recovery_backup_valid=true;
 s_wifi_recovery_can_restore=true;s_wifi_recovery_can_clear=false;s_candidate_valid=true;
 s_marker_clear_calls=s_marker_write_calls=s_zero_calls=s_apply_calls=s_publish_ready_calls=0;
 s_marker_write_error=ESP_FAIL;s_marker_durable_present=true;s_marker_clear_erase_fails=erase_fails;s_marker_clear_readback_fails=readback_fails;
 s_wifi_state=0;s_wifi_session=(test_session_t){81,SECURE_BOOTSTRAP_SESSION_COMMITTING};
 request=(request_t){DEFERRED_WIFI_WORK_FINALIZE_CANDIDATE,81};
}
int main(void){
 reset_case(false,true); /* marker erase committed, readback failed; stronger status write fails */
 run_branch();
 assert(s_wifi_state==9);
 assert(s_marker_clear_calls==1 && !s_marker_durable_present);
 assert(s_marker_write_calls==1);
 assert(s_wifi_recovery_quarantine);
 assert(s_wifi_recovery_can_restore && !s_wifi_recovery_can_clear);
 assert(s_persist_active && s_persist_task_scheduled);
 assert(s_wifi_session.phase==SECURE_BOOTSTRAP_SESSION_COMMITTING);
 assert(s_candidate_valid && s_publish_ready_calls==0 && s_apply_calls==0);
 reset_case(true,false); /* erase failed; original durable marker remains */
 run_branch();
 assert(s_wifi_state==9 && s_marker_durable_present);
 assert(s_wifi_recovery_quarantine && s_wifi_recovery_can_restore);
 assert(s_persist_active && s_wifi_session.phase==SECURE_BOOTSTRAP_SESSION_COMMITTING);
 assert(s_publish_ready_calls==0 && s_apply_calls==0);
 reset_case(false,true); /* first-install ambiguity cannot grant clear permission */
 s_wifi_txn_marker.previous_config_present=false;
 s_wifi_recovery_backup_valid=false;
 s_wifi_recovery_can_restore=false;
 run_branch();
 assert(!s_marker_durable_present && s_wifi_recovery_quarantine);
 assert(!s_wifi_recovery_can_clear && !s_wifi_recovery_can_restore);
 assert(s_persist_active && s_publish_ready_calls==0);
 reset_case(true,false);
 s_wifi_txn_marker.previous_config_present=false;
 s_wifi_recovery_backup_valid=false;
 s_wifi_recovery_can_restore=false;
 s_marker_write_error=ESP_OK; /* verified status marker permits explicit clear */
 run_branch();
 assert(s_marker_durable_present && s_wifi_recovery_quarantine);
 assert(s_wifi_recovery_can_clear);
 puts("PASS F4 actual finalize branch: quarantine, evidence and ownership retained");
 return 0;
}
'''
# Baseline mode restores the original unsafe decision while retaining the rest
# of the exact extracted production branch and real transaction coordinator.
mode=os.environ.get("Q2_F4_BASELINE","0")
corrected_branch=branch
baseline_marker_block = """                /* Finalization may have erased the marker durably before a
                 * failed readback. Enter runtime quarantine before trying to
                 * persist the stronger recovery marker. Do not offer
                 * first-install clearing unless that marker is verified. */
                portENTER_CRITICAL(&s_wifi_session_mux);
                s_wifi_recovery_quarantine = true;
                s_wifi_recovery_generation = request.generation;
                s_wifi_recovery_can_restore =
                    s_wifi_txn_marker.previous_config_present &&
                    s_wifi_recovery_backup_valid;
                s_wifi_recovery_can_clear = false;
                portEXIT_CRITICAL(&s_wifi_session_mux);
                panel_wifi_txn_marker_t failed = s_wifi_txn_marker;
                failed.state = PANEL_WIFI_TXN_RECOVERY_REQUIRED;
                const esp_err_t marker_update_result =
                    txn_marker_write(NULL, &failed);
                if (marker_update_result == ESP_OK &&
                    !failed.previous_config_present &&
                    !s_wifi_recovery_backup_valid) {
                    portENTER_CRITICAL(&s_wifi_session_mux);
                    s_wifi_recovery_can_clear = true;
                    portEXIT_CRITICAL(&s_wifi_session_mux);
                }
"""
old_marker_block = """                panel_wifi_txn_marker_t failed = s_wifi_txn_marker;
                failed.state = PANEL_WIFI_TXN_RECOVERY_REQUIRED;
                (void)txn_marker_write(NULL, &failed);
"""
if branch.count(baseline_marker_block)!=1:
    raise SystemExit("actual F4 quarantine block not found exactly once")
baseline_branch=branch.replace(baseline_marker_block,old_marker_block,1)
selected=baseline_branch if mode=="1" else corrected_branch
main=main.replace("BRANCH",selected)
unit=fixture+"\n"+main
with tempfile.TemporaryDirectory(prefix="q2-f4-finalize-") as td:
    c=Path(td)/"f4.c"; exe=Path(td)/"f4"
    c.write_text(unit)
    cmd=["cc","-std=c11","-Wall","-Wextra","-Werror","-pedantic","-Wno-unused-function","-I",str(ROOT/"include"),str(c),str(TXN),"-o",str(exe)]
    if os.environ.get("Q2_HOST_SANITIZERS")=="1": cmd[1:1]=["-fsanitize=address,undefined","-fno-omit-frame-pointer"]
    subprocess.run(cmd,check=True)
    run=subprocess.run([str(exe)],capture_output=True,text=True)
    if mode=="1":
        if run.returncode==0: raise SystemExit("F4 baseline unexpectedly passed")
        print("EXPECTED BASELINE FAIL: runtime quarantine is false after finalize/status-marker failure")
    else:
        if run.returncode: raise SystemExit(run.stderr or run.stdout)
        print(run.stdout,end="")
