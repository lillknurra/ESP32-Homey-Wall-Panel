#!/usr/bin/env python3
"""Compile the exact queue_rollback_from_event() body from Q1 and Q2."""
import pathlib
import os
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
Q1 = "cd99d5d1c849f4655e35803f8be4800c9ed3dac8"

HARNESS = r'''#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "panel_wifi_transaction.h"

typedef int portMUX_TYPE;
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
#define pdPASS 1
#define TAG "host"
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
enum { DEFERRED_WIFI_WORK_ROLLBACK = 3 };
enum { SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED = 10 };
enum { WIFI_PERSIST_STAGE_VERIFY_CONNECT = 2, WIFI_PERSIST_STAGE_IDLE = 0 };
typedef struct { uint32_t generation; int kind; uint32_t actions; } deferred_provisioning_request_t;
static int s_wifi_session_mux;
static bool s_persist_active = true;
static bool s_persist_task_scheduled = true;
static int s_persist_stage = WIFI_PERSIST_STAGE_VERIFY_CONNECT;
static uint32_t s_persist_session_generation = 41U;
static bool s_wifi_txn_marker_present = true;
static panel_wifi_txn_marker_t s_wifi_txn_marker = {
    PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING, true, 41U};
static bool s_deferred_rollback_queued;
static bool s_wifi_recovery_quarantine;
static bool s_wifi_recovery_backup_valid = true;
static bool s_wifi_recovery_can_restore;
static bool s_wifi_recovery_can_clear;
static uint32_t s_wifi_recovery_generation;
static panel_wifi_recovery_slot_t s_wifi_recovery_slot;
static bool s_deferred_provisioning_worker_ready = true;
static void *s_deferred_provisioning_queue = (void *)1;
static int sends;
static int failed_transitions;
static int false_finishes;
static int send_result = pdPASS;
static bool claim_during_send;
static int xQueueSend(void *queue, const void *item, unsigned wait)
{ (void)queue; (void)item; (void)wait; ++sends; return send_result; }
static int xQueueSendToFront(void *queue, const void *item, unsigned wait)
{
    (void)queue; (void)item; (void)wait; ++sends;
    if (claim_during_send) assert(panel_wifi_recovery_claim_rollback(
        &s_wifi_recovery_slot, 41U));
    return send_result;
}
static int wifi_transition_serialized(int event)
{ (void)event; ++failed_transitions; return 0; }
static bool secure_bootstrap_wifi_session_finish_commit(void *session, uint32_t gen)
{ (void)session; (void)gen; ++false_finishes; return true; }
static int s_wifi_session;
'''

MAIN = r'''
int main(int argc, char **argv)
{
    assert(argc == 2);
    const char *mode = argv[1];
    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    if (strcmp(mode, "worker-unavailable") == 0) s_deferred_provisioning_worker_ready = false;
    if (strcmp(mode, "queue-full") == 0) send_result = 0;
    if (strcmp(mode, "claim-race") == 0) { send_result = 0; claim_during_send = true; }
    if (strcmp(mode, "stale") == 0) s_persist_session_generation = 42U;
    if (strcmp(mode, "duplicate") == 0) s_deferred_rollback_queued = true;
    queue_rollback_from_event(41U);
#ifdef Q1_BASELINE
    assert(strcmp(mode, "worker-unavailable") == 0 || strcmp(mode, "queue-full") == 0);
    /* The original function reaches this assertion only after it has already
     * falsely finished the persistence session. */
    assert(false_finishes == 0);
#else
    if (strcmp(mode, "worker-unavailable") == 0 || strcmp(mode, "queue-full") == 0) {
        assert(s_persist_active);
        assert(s_persist_stage == WIFI_PERSIST_STAGE_VERIFY_CONNECT);
        assert(s_wifi_txn_marker_present);
        assert(s_wifi_recovery_slot.owner == PANEL_WIFI_RECOVERY_OWNER_PENDING);
        assert(false_finishes == 0);
    } else if (strcmp(mode, "claim-race") == 0) {
        assert(s_wifi_recovery_slot.owner == PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING);
        assert(s_persist_active && false_finishes == 0);
        assert(failed_transitions == 0);
    } else if (strcmp(mode, "stale") == 0 || strcmp(mode, "duplicate") == 0) {
        assert(sends == 0 && false_finishes == 0 && s_persist_active);
    } else if (strcmp(mode, "queued") == 0) {
        assert(sends == 1 && s_wifi_recovery_slot.owner == PANEL_WIFI_RECOVERY_OWNER_PENDING);
        assert(s_persist_active && false_finishes == 0);
        queue_rollback_from_event(41U);
        assert(sends == 1);
    } else {
        return 2;
    }
#endif
    return 0;
}
'''


def extract_function(source: str) -> str:
    marker = "static void queue_rollback_from_event(uint32_t generation)"
    start = source.find(marker)
    if start < 0:
        raise RuntimeError("queue_rollback_from_event not found")
    brace = source.find("{", start)
    depth = 0
    for i in range(brace, len(source)):
        if source[i] == "{": depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise RuntimeError("unbalanced function body")


def build_and_run(source: str, tmp: pathlib.Path, label: str, baseline=False):
    body = extract_function(source)
    c_file = tmp / f"{label}.c"
    binary = tmp / label
    c_file.write_text(HARNESS + body + MAIN)
    command = [
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-variable",
        "-Wno-unused-function", "-pedantic",
        "-I", str(ROOT / "include"), str(c_file),
        str(ROOT / "panel_wifi_transaction.c"), "-o", str(binary),
    ]
    if os.environ.get("Q2_HOST_SANITIZERS") == "1":
        command[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    if baseline:
        command.insert(1, "-DQ1_BASELINE")
    subprocess.run(command, check=True)
    outcomes = {}
    modes = ("worker-unavailable", "queue-full") if baseline else (
        "worker-unavailable", "queue-full", "claim-race", "stale", "duplicate", "queued")
    for mode in modes:
        result = subprocess.run([str(binary), mode], capture_output=True)
        outcomes[mode] = result.returncode
    if baseline:
        assert any(code != 0 for code in outcomes.values()), outcomes
    else:
        assert all(code == 0 for code in outcomes.values()), outcomes
    return outcomes


with tempfile.TemporaryDirectory(prefix="q2-rollback-production-") as temp:
    temp_path = pathlib.Path(temp)
    baseline = subprocess.run([
        "git", "show", f"{Q1}:components/secure_bootstrap/secure_bootstrap_esp.c"
    ], cwd=REPO, text=True, capture_output=True, check=True).stdout
    current = (ROOT / "secure_bootstrap_esp.c").read_text()
    baseline_results = build_and_run(baseline, temp_path, "q1_baseline", True)
    fixed_results = build_and_run(current, temp_path, "q2_current")
    print("PASS: Q1 actual production function fails rollback ownership assertions:", baseline_results)
    print("PASS: Q2 actual production function preserves enqueue/claim ownership:", fixed_results)
