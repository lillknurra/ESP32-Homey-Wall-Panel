#!/usr/bin/env python3
"""Guard Wi-Fi boot/finalization against legacy unrelated NVS deletion."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(os.environ.get("Q2_SOURCE_ROOT", Path(__file__).resolve().parents[1]))
SOURCE_PATH = ROOT / "secure_bootstrap_esp.c"
SOURCE = SOURCE_PATH.read_text(encoding="utf-8")


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for end in range(brace, len(source)):
        depth += source[end] == "{"
        depth -= source[end] == "}"
        if depth == 0:
            return source[start:end + 1]
    raise AssertionError(f"unterminated function: {signature}")


start = function_body(SOURCE, "secure_bootstrap_status_t secure_bootstrap_start(")
worker = function_body(SOURCE, "static void deferred_provisioning_task(void *arg)")
physical_wipe = function_body(SOURCE, "static void wipe_task(void *arg)")
start_calls = start.count("legacy_sentinel_clear();")
worker_calls = worker.count("legacy_sentinel_clear();")
wipe_calls = physical_wipe.count("legacy_sentinel_clear();")
if start_calls != 0 or worker_calls != 0 or wipe_calls != 1:
    raise SystemExit(
        "unrelated legacy NVS cleanup is reachable from ordinary Wi-Fi boot/finalization: "
        f"start={start_calls}, deferred_finalize={worker_calls}, explicit_wipe={wipe_calls}"
    )

helper = function_body(SOURCE, "static void legacy_sentinel_clear(void)\n{")
fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
typedef int esp_err_t; typedef int nvs_handle_t;
#define ESP_OK 0
#define NVS_READWRITE 1
#define WIFI_SENTINEL_NAMESPACE "p007diag"
#define WIFI_SENTINEL_KEY "commit_seen"
static char opened_namespace[32], erased_key[32];
static unsigned namespace_open_count, erase_count, commit_count;
static unsigned homey_sentinel=0x13579bdfU, alias_sentinel=0x2468ace0U;
static esp_err_t nvs_open(const char *ns,int mode,nvs_handle_t *handle)
{ (void)mode;strncpy(opened_namespace,ns,sizeof(opened_namespace)-1U);++namespace_open_count;*handle=1;return ESP_OK; }
static esp_err_t nvs_erase_key(nvs_handle_t handle,const char *key)
{ (void)handle;strncpy(erased_key,key,sizeof(erased_key)-1U);++erase_count;return ESP_OK; }
static esp_err_t nvs_commit(nvs_handle_t handle) { (void)handle;++commit_count;return ESP_OK; }
static void nvs_close(nvs_handle_t handle) { (void)handle; }
#define ESP_LOGI(...) ((void)0)
#define TAG "host"
'''
main = r'''
int main(void)
{
    legacy_sentinel_clear();
    assert(namespace_open_count==1U && erase_count==1U && commit_count==1U);
    assert(strcmp(opened_namespace,"p007diag")==0);
    assert(strcmp(erased_key,"commit_seen")==0);
    assert(homey_sentinel==0x13579bdfU && alias_sentinel==0x2468ace0U);
    puts("PASS legacy helper scope is separate; ordinary boot/finalization do not call it");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="q2-nvs-namespace-") as td:
    td = Path(td)
    c = td / "scope.c"
    exe = td / "scope"
    c.write_text(fixture + helper + main, encoding="utf-8")
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                    str(c), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
