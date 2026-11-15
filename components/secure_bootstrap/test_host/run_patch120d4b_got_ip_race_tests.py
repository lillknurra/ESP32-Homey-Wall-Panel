#!/usr/bin/env python3
"""Deterministic queue/worker interleavings around candidate GOT_IP publication."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(os.environ.get("Q2_SOURCE_ROOT", Path(__file__).resolve().parents[1]))
ESP = ROOT / "secure_bootstrap_esp.c"


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
    raise AssertionError(f"unterminated function: {signature}")


source = ESP.read_text()
queue_function = extract_function(source, "static bool queue_candidate_finalize_from_event(")
mode = os.environ.get("PATCH120D4B_BASELINE", "0")
if mode == "1":
    publication = """    /* Publish the verified STA/IP state under the same lock the worker uses
     * before making the request visible to another core. */
    if (owns || already) {
        s_ip_obtained = secure_bootstrap_sta_link_apply_event(
            s_ip_obtained, SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP);
    }
"""
    if queue_function.count(publication) != 1:
        raise SystemExit("cannot reconstruct the pre-fix production queue helper")
    queue_function = queue_function.replace(publication, "", 1)
fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "secure_bootstrap.h"
#include "panel_wifi_transaction.h"

typedef int32_t BaseType_t;
typedef uint32_t TickType_t;
typedef int portMUX_TYPE;
typedef void *QueueHandle_t;
#define pdPASS 1
#define pdFAIL 0
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define TAG "host"
#define WIFI_PERSIST_STAGE_VERIFY_CONNECT 3
#define DEFERRED_WIFI_WORK_FINALIZE_CANDIDATE 4

typedef struct {
    uint32_t generation;
    int kind;
    uint32_t actions;
} deferred_provisioning_request_t;

static portMUX_TYPE s_wifi_session_mux;
static bool s_persist_active = true;
static int s_persist_stage = WIFI_PERSIST_STAGE_VERIFY_CONNECT;
static uint32_t s_persist_session_generation = 91U;
static bool s_wifi_txn_marker_present = true;
static panel_wifi_txn_marker_t s_wifi_txn_marker = {
    .state = PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING,
    .previous_config_present = true,
    .generation = 91U,
};
static panel_wifi_recovery_slot_t s_wifi_recovery_slot;
static bool s_deferred_finalize_queued;
static bool s_deferred_provisioning_worker_ready = true;
static QueueHandle_t s_deferred_provisioning_queue = (void *)1;
static bool s_ip_obtained;
static secure_bootstrap_wifi_context_t s_wifi;
static uint32_t queue_send_count;
static uint32_t worker_count;
static uint32_t marker_clear_count;
static bool immediate_worker;
static bool queue_failure;
static bool disconnect_during_durable;
static bool worker_saw_online;
static uint32_t worker_actions;
static uint64_t now_us = 1000U * 1000U;

static int64_t esp_timer_get_time(void) { return (int64_t)now_us; }

static void run_worker(void)
{
    worker_count++;
    assert(panel_wifi_terminal_claim_finalize(&s_wifi_recovery_slot, 91U));
    assert(panel_wifi_terminal_admit_finalize(&s_wifi_recovery_slot, 91U,
        (uint64_t)(esp_timer_get_time() / 1000LL)));
    if (disconnect_during_durable) {
        assert(panel_wifi_terminal_observe_disconnect(
            &s_wifi_recovery_slot, 91U, &s_ip_obtained));
    }
    worker_saw_online = s_ip_obtained;
    worker_actions = secure_bootstrap_wifi_finalize_verified_candidate(
        &s_wifi, s_ip_obtained);
    marker_clear_count++;
    s_wifi_txn_marker_present = false;
    assert(panel_wifi_recovery_finish(&s_wifi_recovery_slot, 91U,
        PANEL_WIFI_RECOVERY_OWNER_FINALIZE_DURABLE));
}

static BaseType_t xQueueSendToFront(
    QueueHandle_t queue, const void *item, TickType_t wait)
{
    (void)item;
    (void)wait;
    assert(queue == s_deferred_provisioning_queue);
    queue_send_count++;
    if (queue_failure) return pdFAIL;
    if (immediate_worker) run_worker();
    return pdPASS;
}

static uint64_t now_ms(void) { return (uint64_t)(esp_timer_get_time() / 1000LL); }

static bool queue_candidate_finalize_from_event(uint32_t generation);

static void prepare(void)
{
    s_persist_active = true;
    s_persist_stage = WIFI_PERSIST_STAGE_VERIFY_CONNECT;
    s_persist_session_generation = 91U;
    s_wifi_txn_marker_present = true;
    s_wifi_txn_marker.generation = 91U;
    s_deferred_finalize_queued = false;
    s_deferred_provisioning_worker_ready = true;
    s_deferred_provisioning_queue = (void *)1;
    s_ip_obtained = false;
    queue_send_count = worker_count = marker_clear_count = 0U;
    immediate_worker = queue_failure = disconnect_during_durable = false;
    worker_saw_online = false;
    worker_actions = 0U;
    now_us = 1000U * 1000U;
    secure_bootstrap_wifi_context_init(&s_wifi);
    s_wifi.state = SECURE_BOOTSTRAP_WIFI_PERSIST_VERIFY_CONNECTING;
    s_wifi.saved_config_present = true;
    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    assert(panel_wifi_terminal_begin_prepare(&s_wifi_recovery_slot, 91U,
        now_ms(), 30000U));
    assert(panel_wifi_terminal_admit_candidate(&s_wifi_recovery_slot, 91U,
        now_ms() + 1U));
    assert(panel_wifi_terminal_begin_verify(&s_wifi_recovery_slot, 91U,
        now_ms() + 2U, 30000U));
}

#if !PATCH120D4B_BASELINE
static void test_worker_after_event_handler(void)
{
    prepare();
    assert(queue_candidate_finalize_from_event(91U));
    assert(s_ip_obtained);
    assert(worker_count == 0U);
    run_worker();
    assert(worker_saw_online);
    assert((worker_actions & SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE) != 0U);
    assert((worker_actions & SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT) == 0U);
}

static void test_disconnect_before_finalize(void)
{
    prepare();
    assert(queue_candidate_finalize_from_event(91U));
    assert(panel_wifi_terminal_observe_disconnect(
        &s_wifi_recovery_slot, 91U, &s_ip_obtained));
    run_worker();
    assert(!worker_saw_online);
    assert((worker_actions & SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE) == 0U);
    assert((worker_actions & SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT) != 0U);
    assert(s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED);
}

static void test_disconnect_during_durable_finalize(void)
{
    prepare();
    immediate_worker = true;
    disconnect_during_durable = true;
    assert(queue_candidate_finalize_from_event(91U));
    assert(!worker_saw_online);
    assert((worker_actions & SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE) == 0U);
    assert(s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED);
    assert(s_wifi_recovery_slot.owner == PANEL_WIFI_RECOVERY_OWNER_NONE);
    assert(marker_clear_count == 1U);
}

static void test_timeout_and_rollback_reject_late_got_ip(void)
{
    prepare();
    panel_wifi_terminal_timeout_result_t timeout_result;
    assert(panel_wifi_terminal_timeout(&s_wifi_recovery_slot, 91U,
        now_ms() + 30002U, 30000U, &timeout_result));
    assert(timeout_result == PANEL_WIFI_TERMINAL_TIMEOUT_CLAIMED);
    assert(!queue_candidate_finalize_from_event(91U));
    assert(!s_ip_obtained && queue_send_count == 0U && worker_count == 0U);
    assert(s_wifi_txn_marker_present && marker_clear_count == 0U);

    prepare();
    assert(panel_wifi_terminal_publish_rollback(
        &s_wifi_recovery_slot, 91U, now_ms() + 3U));
    assert(!queue_candidate_finalize_from_event(91U));
    assert(!s_ip_obtained && queue_send_count == 0U && worker_count == 0U);
    assert(s_wifi_txn_marker_present && marker_clear_count == 0U);
}

static void test_queue_failure_and_duplicate_event(void)
{
    prepare();
    queue_failure = true;
    assert(!queue_candidate_finalize_from_event(91U));
    assert(s_ip_obtained);
    assert(worker_count == 0U && marker_clear_count == 0U);
    assert(s_wifi_txn_marker_present);

    prepare();
    assert(queue_candidate_finalize_from_event(91U));
    assert(queue_candidate_finalize_from_event(91U));
    assert(queue_send_count == 1U && worker_count == 0U);
    run_worker();
    assert(worker_count == 1U && marker_clear_count == 1U);
    assert(worker_saw_online);
}
#endif
'''
main = r'''
int main(void)
{
    prepare();
    immediate_worker = true;
    const bool accepted = queue_candidate_finalize_from_event(91U);
    assert(accepted);
#if PATCH120D4B_BASELINE
    /* This is the pre-fix caller ordering in wifi_event(): publication only
     * after queue_candidate_finalize_from_event() has returned. */
    s_ip_obtained = true;
    assert(worker_count == 1U);
    assert(!worker_saw_online);
    assert((worker_actions & SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT) != 0U);
    assert((worker_actions & SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE) == 0U);
    assert(s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED);
    puts("EXPECTED_BASELINE_FAILURE: immediate finalize worker saw stale offline link and chose saved-network retry");
#else
    assert(worker_count == 1U);
    assert(worker_saw_online);
    assert((worker_actions & SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE) != 0U);
    assert((worker_actions & SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT) == 0U);
    assert(s_wifi.state == SECURE_BOOTSTRAP_WIFI_ONLINE);
    assert(s_ip_obtained);
    assert(marker_clear_count == 1U);
    test_worker_after_event_handler();
    test_disconnect_before_finalize();
    test_disconnect_during_durable_finalize();
    test_timeout_and_rollback_reject_late_got_ip();
    test_queue_failure_and_duplicate_event();
    puts("PASS: immediate worker observes accepted GOT_IP publication");
#endif
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="patch120d4b-got-ip-") as temp:
    temp = Path(temp)
    harness = temp / "got_ip.c"
    binary = temp / "got_ip"
    harness.write_text(fixture + "\n" + queue_function + "\n" + main)
    cmd = [
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        f"-DPATCH120D4B_BASELINE={1 if mode == '1' else 0}",
        "-I", str(ROOT / "include"), str(harness),
        str(ROOT / "secure_bootstrap_logic.c"),
        str(ROOT / "panel_wifi_transaction.c"), "-o", str(binary),
    ]
    subprocess.run(cmd, check=True)
    subprocess.run([str(binary)], check=True)

event_handler = extract_function(source, "static void wifi_event(void *arg")
verify_path = event_handler[event_handler.index("if (persist_verify)"):]
verify_path = verify_path[:verify_path.index("        bool candidate = false;")]
assert "queue_candidate_finalize_from_event(generation)" in verify_path
assert "s_ip_obtained =" not in verify_path
queue_start = source.index("static bool queue_candidate_finalize_from_event(")
queue_end = source.index("\nstatic bool queue_wifi_event_actions", queue_start)
queue_body = source[queue_start:queue_end]
if mode != "1":
    assert queue_body.index("s_ip_obtained =") < queue_body.index("portEXIT_CRITICAL(&s_wifi_session_mux)")
    assert queue_body.index("portEXIT_CRITICAL(&s_wifi_session_mux)") < queue_body.index("xQueueSendToFront")
    assert source.count("s_ip_obtained =") == 2
print("PASS: GOT_IP link publication is terminal-owner admitted and synchronized before queue notification")
