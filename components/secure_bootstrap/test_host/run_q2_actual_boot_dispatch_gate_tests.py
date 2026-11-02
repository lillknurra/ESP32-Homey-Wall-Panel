#!/usr/bin/env python3
"""Compile the exact startup dispatch fragment from secure_bootstrap_start()."""
from pathlib import Path
import os
import subprocess
import tempfile


ROOT = Path(os.environ.get("Q2_SOURCE_ROOT", Path(__file__).resolve().parents[1]))
SOURCE = (ROOT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
LOGIC = ROOT / "secure_bootstrap_logic.c"
HEADER = ROOT / "include"


def extract_function(src: str, signature: str) -> str:
    start = src.index(signature)
    brace = src.index("{", start)
    depth = 0
    for end in range(brace, len(src)):
        if src[end] == "{":
            depth += 1
        elif src[end] == "}":
            depth -= 1
            if depth == 0:
                return src[start:end + 1]
    raise AssertionError(f"unterminated function {signature}")


start_fn = extract_function(SOURCE, "secure_bootstrap_status_t secure_bootstrap_start(")
load_at = start_fn.index("(void)wifi_recovery_boot_load();")
start_at = start_fn.index("ESP_ERROR_CHECK(esp_wifi_start());", load_at)
dispatch_start = start_fn.index("uint32_t actions = s_wifi_recovery_quarantine", start_at)
dispatch_end = start_fn.index("(void)apply_actions(actions);", dispatch_start) + len("(void)apply_actions(actions);")
fragment = start_fn[dispatch_start:dispatch_end]
if not (load_at < start_at < dispatch_start):
    raise SystemExit("recovery boot load/startup dispatch order changed")

fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "secure_bootstrap.h"
#define TAG "host"
#define ESP_LOGI(...) ((void)0)
static secure_bootstrap_wifi_context_t s_wifi;
static bool s_wifi_recovery_quarantine;
static bool s_saved_valid;
static unsigned s_connect_calls;
static uint32_t s_actions;
static uint32_t wifi_transition_serialized(secure_bootstrap_wifi_event_t event)
{ return secure_bootstrap_wifi_transition(&s_wifi,event); }
static int apply_actions(uint32_t actions)
{ s_actions=actions;if(actions & SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT)++s_connect_calls;return 0; }
static uint32_t production_startup_dispatch(void)
{
'''
main = r'''
'''
wrapper_end = r'''
    return actions;
}
int main(void)
{
    secure_bootstrap_wifi_context_init(&s_wifi);
    (void)secure_bootstrap_wifi_transition(&s_wifi,
        SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_RECOVERY_REQUIRED);
    s_wifi_recovery_quarantine=true;s_saved_valid=true;
    assert(production_startup_dispatch()==SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(s_actions==SECURE_BOOTSTRAP_WIFI_ACTION_NONE && s_connect_calls==0U);
    assert(secure_bootstrap_wifi_transition(&s_wifi,
        SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP)==SECURE_BOOTSTRAP_WIFI_ACTION_NONE);
    assert(s_wifi.state==SECURE_BOOTSTRAP_WIFI_RECOVERY_REQUIRED && s_connect_calls==0U);
    secure_bootstrap_wifi_context_init(&s_wifi);
    s_wifi_recovery_quarantine=false;s_saved_valid=true;s_connect_calls=0U;s_actions=0U;
    assert(production_startup_dispatch()==SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT);
    assert(s_connect_calls==1U);
    puts("PASS exact startup dispatch fragment: quarantine suppresses saved connect; normal saved boot connects");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="q2-boot-dispatch-") as td:
    td = Path(td)
    c = td / "boot_gate.c"
    exe = td / "boot_gate"
    c.write_text(fixture + fragment + wrapper_end + main, encoding="utf-8")
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                    "-I", str(HEADER), str(c), str(LOGIC), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
