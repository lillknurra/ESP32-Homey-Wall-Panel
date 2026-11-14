#!/usr/bin/env python3
"""Deterministic tests for the production generation-scoped terminal owner."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(os.environ.get("Q2_SOURCE_ROOT", Path(__file__).resolve().parents[1]))
TXN = ROOT / "panel_wifi_transaction.c"
INCLUDE = ROOT / "include"

SOURCE = r'''#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "panel_wifi_transaction.h"

#define DEADLINE 30000U
#define T0 1000ULL

static void prepare_verify(panel_wifi_recovery_slot_t *s, uint32_t gen)
{
    panel_wifi_recovery_slot_init(s);
    assert(panel_wifi_terminal_begin_prepare(s, gen, T0, DEADLINE));
    assert(panel_wifi_terminal_admit_candidate(s, gen, T0 + 1));
    assert(panel_wifi_terminal_begin_verify(s, gen, T0 + 2, DEADLINE));
}

static void test_http_stop_timeout_fences_late_candidate_write(void)
{
    panel_wifi_recovery_slot_t s;
    panel_wifi_recovery_slot_init(&s);
    assert(panel_wifi_terminal_begin_prepare(&s, 1, T0, DEADLINE));
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&s, 1, T0 + DEADLINE,
        DEADLINE, &r));
    assert(r == PANEL_WIFI_TERMINAL_TIMEOUT_CLAIMED);
    assert(s.owner == PANEL_WIFI_RECOVERY_OWNER_TIMED_OUT);
    assert(!panel_wifi_terminal_admit_candidate(&s, 1, T0 + DEADLINE + 1));
}

static void test_success_before_deadline(void)
{
    panel_wifi_recovery_slot_t s;
    prepare_verify(&s, 2);
    assert(panel_wifi_terminal_queue_finalize(&s, 2, T0 + 3));
    assert(panel_wifi_terminal_claim_finalize(&s, 2));
    assert(panel_wifi_terminal_admit_finalize(&s, 2, T0 + 4));
    assert(s.owner == PANEL_WIFI_RECOVERY_OWNER_FINALIZE_DURABLE);
    assert(panel_wifi_recovery_finish(&s, 2,
        PANEL_WIFI_RECOVERY_OWNER_FINALIZE_DURABLE));
}

static void test_got_ip_deadline_boundary(void)
{
    panel_wifi_recovery_slot_t before, after;
    prepare_verify(&before, 3);
    prepare_verify(&after, 4);
    assert(panel_wifi_terminal_queue_finalize(&before, 3, T0 + 2 + DEADLINE - 1));
    assert(!panel_wifi_terminal_queue_finalize(&after, 4, T0 + 2 + DEADLINE));
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&after, 4, T0 + 2 + DEADLINE,
        DEADLINE, &r));
    assert(r == PANEL_WIFI_TERMINAL_TIMEOUT_CLAIMED);
}

static void test_timeout_queued_or_claimed_revokes_admission(void)
{
    panel_wifi_recovery_slot_t queued, claimed;
    prepare_verify(&queued, 5);
    prepare_verify(&claimed, 6);
    assert(panel_wifi_terminal_queue_finalize(&queued, 5, T0 + 3));
    assert(panel_wifi_terminal_queue_finalize(&claimed, 6, T0 + 3));
    assert(panel_wifi_terminal_claim_finalize(&claimed, 6));
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&queued, 5, T0 + 2 + DEADLINE,
        DEADLINE, &r));
    assert(panel_wifi_terminal_timeout(&claimed, 6, T0 + 2 + DEADLINE,
        DEADLINE, &r));
    assert(!panel_wifi_terminal_claim_finalize(&queued, 5));
    assert(!panel_wifi_terminal_admit_finalize(&claimed, 6, T0 + 2 + DEADLINE + 1));
}

static void test_timeout_wins_immediately_before_durable_admission(void)
{
    panel_wifi_recovery_slot_t s;
    prepare_verify(&s, 7);
    assert(panel_wifi_terminal_queue_finalize(&s, 7, T0 + 3));
    assert(panel_wifi_terminal_claim_finalize(&s, 7));
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&s, 7, T0 + 2 + DEADLINE,
        DEADLINE, &r));
    assert(!panel_wifi_terminal_admit_finalize(&s, 7, T0 + 2 + DEADLINE));
}

static void test_durable_finalize_is_never_interrupted(void)
{
    panel_wifi_recovery_slot_t s;
    prepare_verify(&s, 8);
    assert(panel_wifi_terminal_queue_finalize(&s, 8, T0 + 3));
    assert(panel_wifi_terminal_claim_finalize(&s, 8));
    assert(panel_wifi_terminal_admit_finalize(&s, 8, T0 + 4));
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&s, 8, T0 + 4 + DEADLINE,
        DEADLINE, &r));
    assert(r == PANEL_WIFI_TERMINAL_TIMEOUT_DURABLE_BLOCKED);
    assert(s.owner == PANEL_WIFI_RECOVERY_OWNER_FINALIZE_DURABLE);
    assert(panel_wifi_recovery_finish(&s, 8,
        PANEL_WIFI_RECOVERY_OWNER_FINALIZE_DURABLE));
}

static void test_disconnect_rollback_timeout_race(void)
{
    panel_wifi_recovery_slot_t timed, durable;
    prepare_verify(&timed, 9);
    prepare_verify(&durable, 10);
    assert(panel_wifi_terminal_publish_rollback(&timed, 9, T0 + 3));
    assert(panel_wifi_recovery_claim_rollback(&timed, 9));
    assert(panel_wifi_terminal_publish_rollback(&durable, 10, T0 + 3));
    assert(panel_wifi_recovery_claim_rollback(&durable, 10));
    assert(panel_wifi_terminal_admit_rollback(&durable, 10, T0 + 4));
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&timed, 9, T0 + 2 + DEADLINE,
        DEADLINE, &r));
    assert(!panel_wifi_terminal_admit_rollback(&timed, 9, T0 + 2 + DEADLINE));
    assert(panel_wifi_terminal_timeout(&durable, 10, T0 + 4 + DEADLINE,
        DEADLINE, &r));
    assert(r == PANEL_WIFI_TERMINAL_TIMEOUT_DURABLE_BLOCKED);
    assert(durable.owner == PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_DURABLE);
}

static void test_queued_rollback_and_stale_generation(void)
{
    panel_wifi_recovery_slot_t s;
    prepare_verify(&s, 11);
    assert(panel_wifi_terminal_publish_rollback(&s, 11, T0 + 3));
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&s, 11, T0 + 2 + DEADLINE,
        DEADLINE, &r));
    assert(!panel_wifi_recovery_claim_rollback(&s, 11));
    assert(!panel_wifi_terminal_admit_rollback(&s, 12, T0 + DEADLINE));
}

static void test_rollback_claim_at_deadline_cannot_drop_owner(void)
{
    panel_wifi_recovery_slot_t s;
    prepare_verify(&s, 12);
    assert(panel_wifi_terminal_publish_rollback(&s, 12, T0 + 3));
    const uint64_t boundary = T0 + 2 + DEADLINE;
    assert(!panel_wifi_terminal_claim_rollback(&s, 12, boundary));
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&s, 12, boundary, DEADLINE, &r));
    assert(r == PANEL_WIFI_TERMINAL_TIMEOUT_CLAIMED);
    assert(s.owner == PANEL_WIFI_RECOVERY_OWNER_TIMED_OUT);
    assert(!panel_wifi_terminal_admit_rollback(&s, 12, boundary));
    assert(!panel_wifi_recovery_finish(
        &s, 12, PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING));
}

static void test_timeout_and_rollback_compete_for_one_owner(void)
{
    panel_wifi_recovery_slot_t event_first, timeout_first;
    prepare_verify(&event_first, 16);
    prepare_verify(&timeout_first, 17);
    const uint64_t boundary = T0 + 2 + DEADLINE;
    assert(!panel_wifi_terminal_publish_rollback(&event_first, 16, boundary));
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&event_first, 16, boundary, DEADLINE, &r));
    assert(r == PANEL_WIFI_TERMINAL_TIMEOUT_CLAIMED);
    assert(!panel_wifi_terminal_claim_rollback(&event_first, 16, boundary));
    assert(panel_wifi_terminal_publish_rollback(&timeout_first, 17, T0 + 3));
    assert(panel_wifi_terminal_timeout(&timeout_first, 17, boundary, DEADLINE, &r));
    assert(timeout_first.owner == PANEL_WIFI_RECOVERY_OWNER_TIMED_OUT);
    assert(!panel_wifi_terminal_claim_rollback(&timeout_first, 17, boundary));
}

static void test_manual_recovery_arbitration(void)
{
    panel_wifi_recovery_slot_t s;
    prepare_verify(&s, 13);
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&s, 13, T0 + 2 + DEADLINE,
        DEADLINE, &r));
    assert(panel_wifi_recovery_claim_manual(&s, 13));
    assert(s.owner == PANEL_WIFI_RECOVERY_OWNER_MANUAL_SERVICE);
    assert(panel_wifi_recovery_finish(&s, 13,
        PANEL_WIFI_RECOVERY_OWNER_MANUAL_SERVICE));

    prepare_verify(&s, 14);
    assert(panel_wifi_terminal_queue_finalize(&s, 14, T0 + 3));
    assert(panel_wifi_terminal_claim_finalize(&s, 14));
    assert(panel_wifi_terminal_admit_finalize(&s, 14, T0 + 4));
    assert(!panel_wifi_recovery_claim_manual(&s, 14));
}

static void test_candidate_failure_transfers_same_owner_to_rollback(void)
{
    panel_wifi_recovery_slot_t s;
    panel_wifi_recovery_slot_init(&s);
    assert(panel_wifi_terminal_begin_prepare(&s, 15, T0, DEADLINE));
    assert(panel_wifi_terminal_admit_candidate(&s, 15, T0 + 1));
    assert(panel_wifi_terminal_admit_rollback(&s, 15, T0 + 2));
    assert(s.owner == PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_DURABLE);
}

static void test_disconnect_updates_link_only_for_current_generation_owner(void)
{
    panel_wifi_recovery_slot_t s, disconnect_first;
    bool online = true;
    prepare_verify(&s, 18);
    assert(panel_wifi_terminal_observe_disconnect(&s, 18, &online));
    assert(!online);
    assert(panel_wifi_terminal_publish_rollback(&s, 18, T0 + 3));

    /* A GOT_IP may already have queued finalize. Link state still follows the
     * current disconnect, while exclusive terminal ownership remains finalize. */
    online = true;
    prepare_verify(&s, 19);
    assert(panel_wifi_terminal_queue_finalize(&s, 19, T0 + 3));
    assert(panel_wifi_terminal_observe_disconnect(&s, 19, &online));
    assert(!online);
    assert(!panel_wifi_terminal_publish_rollback(&s, 19, T0 + 4));
    assert(s.owner == PANEL_WIFI_RECOVERY_OWNER_FINALIZE_QUEUED);

    /* Reverse event order: disconnect publishes rollback first, so a later
     * GOT_IP cannot queue or claim finalize for that generation. */
    prepare_verify(&disconnect_first, 23);
    online = true;
    assert(panel_wifi_terminal_observe_disconnect(
        &disconnect_first, 23, &online));
    assert(!online);
    assert(panel_wifi_terminal_publish_rollback(
        &disconnect_first, 23, T0 + 3));
    assert(!panel_wifi_terminal_queue_finalize(
        &disconnect_first, 23, T0 + 4));
    assert(disconnect_first.owner == PANEL_WIFI_RECOVERY_OWNER_PENDING);

    /* A late same-generation disconnect updates actual link status but cannot
     * reclaim a timeout or publish rollback. */
    online = true;
    prepare_verify(&s, 20);
    panel_wifi_terminal_timeout_result_t r;
    assert(panel_wifi_terminal_timeout(&s, 20, T0 + 2 + DEADLINE,
        DEADLINE, &r));
    assert(s.owner == PANEL_WIFI_RECOVERY_OWNER_TIMED_OUT);
    assert(panel_wifi_terminal_observe_disconnect(&s, 20, &online));
    assert(!online);
    assert(!panel_wifi_terminal_publish_rollback(&s, 20, T0 + 2 + DEADLINE));
    online = true;
    assert(!panel_wifi_terminal_observe_disconnect(&s, 21, &online));
    assert(online);
    assert(panel_wifi_recovery_finish(&s, 20,
        PANEL_WIFI_RECOVERY_OWNER_TIMED_OUT));
    assert(!panel_wifi_terminal_observe_disconnect(&s, 20, &online));

    /* A disconnect reported while rollback owns durable work only updates the
     * observed link state; it cannot alter that owner. */
    prepare_verify(&s, 24);
    assert(panel_wifi_terminal_publish_rollback(&s, 24, T0 + 3));
    assert(panel_wifi_terminal_claim_rollback(&s, 24, T0 + 4));
    assert(panel_wifi_terminal_admit_rollback(&s, 24, T0 + 5));
    online = true;
    assert(panel_wifi_terminal_observe_disconnect(&s, 24, &online));
    assert(!online);
    assert(s.owner == PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_DURABLE);

    /* Driver disconnects during candidate persistence are fenced from the
     * verify-only adapter, before the verification association is armed. */
    panel_wifi_recovery_slot_init(&s);
    assert(panel_wifi_terminal_begin_prepare(&s, 25, T0, DEADLINE));
    assert(panel_wifi_terminal_admit_candidate(&s, 25, T0 + 1));
    online = true;
    assert(!panel_wifi_terminal_observe_disconnect(&s, 25, &online));
    assert(online);

    /* Reusing the terminal slot for a newer generation rejects the old event. */
    panel_wifi_recovery_slot_init(&s);
    assert(panel_wifi_terminal_begin_prepare(&s, 22, T0, DEADLINE));
    online = true;
    assert(!panel_wifi_terminal_observe_disconnect(&s, 20, &online));
    assert(!panel_wifi_terminal_observe_disconnect(&s, 22, &online));
    assert(online);
}

int main(void)
{
    test_http_stop_timeout_fences_late_candidate_write();
    test_success_before_deadline();
    test_got_ip_deadline_boundary();
    test_timeout_queued_or_claimed_revokes_admission();
    test_timeout_wins_immediately_before_durable_admission();
    test_durable_finalize_is_never_interrupted();
    test_disconnect_rollback_timeout_race();
    test_queued_rollback_and_stale_generation();
    test_rollback_claim_at_deadline_cannot_drop_owner();
    test_timeout_and_rollback_compete_for_one_owner();
    test_manual_recovery_arbitration();
    test_candidate_failure_transfers_same_owner_to_rollback();
    test_disconnect_updates_link_only_for_current_generation_owner();
    puts("PASS: production terminal owner arbitration and deterministic deadline boundaries");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="patch120d4-owner-") as tmp:
    source = Path(tmp) / "terminal_owner_test.c"
    binary = Path(tmp) / "terminal_owner_test"
    source.write_text(SOURCE)
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(INCLUDE), str(source), str(TXN), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)

esp = (ROOT / "secure_bootstrap_esp.c").read_text()
required = (
    "panel_wifi_terminal_begin_prepare(",
    "panel_wifi_terminal_admit_candidate(",
    "panel_wifi_terminal_begin_verify(",
    "panel_wifi_terminal_queue_finalize(",
    "panel_wifi_terminal_claim_finalize(",
    "panel_wifi_terminal_claim_rollback(",
    "panel_wifi_terminal_admit_finalize(",
    "panel_wifi_terminal_admit_rollback(",
    "panel_wifi_terminal_timeout(",
    "panel_wifi_terminal_observe_disconnect(",
    "wifi_terminal_supervisor_task",
)
missing = [symbol for symbol in required if symbol not in esp]
if missing:
    raise SystemExit("production integration missing: " + ", ".join(missing))
print("PASS: production integration uses shared prepare, event, durable-admission, and supervisor gates")

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
    raise AssertionError(f"unterminated production helper {signature}")

rollback = extract_function(esp, "static bool queue_rollback_from_event(")
assert "panel_wifi_terminal_observe_disconnect(" in rollback
assert "&s_ip_obtained" in rollback
event_handler = extract_function(esp, "static void wifi_event(")
verify_disconnect = event_handler[event_handler.index(
    "if (persist_active && persist_stage == WIFI_PERSIST_STAGE_VERIFY_CONNECT)",
    event_handler.index("if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)")
):]
verify_disconnect = verify_disconnect[:verify_disconnect.index("return;") + len("return;")]
assert "if (queue_rollback_from_event(generation))" in verify_disconnect
assert "phone_provisioning_on_wifi_offline();" in verify_disconnect
assert "cancel_sntp_start_request();" in verify_disconnect
print("PASS: verify-disconnect link/offline effects are gated by current generation ownership")
worker = extract_function(esp, "static void deferred_provisioning_task(")
assert "secure_bootstrap_wifi_finalize_verified_candidate(" in worker
apply_actions = extract_function(esp, "static esp_err_t apply_actions(uint32_t actions)")
show_online = apply_actions[apply_actions.index(
    "if (actions & SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE)"):]
assert "link_online = s_ip_obtained;" in show_online
assert "if (!link_online)" in show_online
print("PASS: finalize cannot republish online/Homey state after an admitted verify disconnect")

claim_helper = extract_function(esp,
    "static bool claim_pending_rollback_locked(uint32_t generation, uint64_t now_ms)")
integration = r'''#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "panel_wifi_transaction.h"
#define WIFI_PERSIST_STAGE_VERIFY_CONNECT 3
static bool s_persist_active=true;
static uint32_t s_persist_session_generation=21U;
static int s_persist_stage=WIFI_PERSIST_STAGE_VERIFY_CONNECT;
static panel_wifi_recovery_slot_t s_wifi_recovery_slot;
'''
integration += claim_helper + r'''
int main(void) {
    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    s_wifi_recovery_slot.generation=21U;
    s_wifi_recovery_slot.owner=PANEL_WIFI_RECOVERY_OWNER_PENDING;
    s_wifi_recovery_slot.deadline_ms=30000U;
    assert(claim_pending_rollback_locked(21U,29999U));
    assert(s_wifi_recovery_slot.owner==PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING);
    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    s_wifi_recovery_slot.generation=21U;
    s_wifi_recovery_slot.owner=PANEL_WIFI_RECOVERY_OWNER_PENDING;
    s_wifi_recovery_slot.deadline_ms=30000U;
    panel_wifi_terminal_timeout_result_t result;
    assert(!claim_pending_rollback_locked(21U,30000U));
    assert(panel_wifi_terminal_timeout(&s_wifi_recovery_slot,21U,30000U,30000U,&result));
    assert(s_wifi_recovery_slot.owner==PANEL_WIFI_RECOVERY_OWNER_TIMED_OUT);
    assert(!panel_wifi_recovery_finish(&s_wifi_recovery_slot,21U,
        PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING));
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="patch120d4-rollback-claim-") as tmp:
    source = Path(tmp) / "claim_test.c"
    binary = Path(tmp) / "claim_test"
    source.write_text(integration)
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(INCLUDE), str(source), str(TXN), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
print("PASS: production rollback claim rejects deadline boundary without dropping terminal owner")

supervisor = extract_function(esp,
    "static void wifi_terminal_supervisor_task(void *arg)")
watch = r'''#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include "panel_wifi_transaction.h"
typedef int portMUX_TYPE;
typedef enum { WIFI_PERSIST_STAGE_IDLE=0, WIFI_PERSIST_STAGE_HTTP_STOP,
               WIFI_PERSIST_STAGE_VERIFY_CONNECT } wifi_persist_stage_t;
typedef enum { WIFI_TERMINAL_UI_NONE=0, WIFI_TERMINAL_UI_HTTP_STALLED,
               WIFI_TERMINAL_UI_VERIFY_TIMEOUT,
               WIFI_TERMINAL_UI_DURABLE_BLOCKED } wifi_terminal_ui_state_t;
typedef struct { int state; } wifi_context_t;
#define WIFI_DURABLE_STALL_REPORT_MS 30000U
#define WIFI_TERMINAL_MONITOR_INTERVAL_MS 100U
#define SECURE_BOOTSTRAP_WIFI_EVENT_RECOVERY_REQUIRED 1
#define pdMS_TO_TICKS(ms) (ms)
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
#define TAG "host"
static void log_sink(const char *tag,const char *format,...)
{ (void)tag; (void)format; }
#define ESP_LOGE(...) log_sink(__VA_ARGS__)
static jmp_buf exit_after_iteration;
static uint64_t mock_now_ms;
static int64_t esp_timer_get_time(void) { return (int64_t)(mock_now_ms*1000U); }
static void vTaskDelay(unsigned ticks) { (void)ticks; longjmp(exit_after_iteration,1); }
static uint32_t secure_bootstrap_wifi_transition(wifi_context_t *wifi,int event)
{ (void)event; wifi->state=9; return 0U; }
static portMUX_TYPE s_wifi_session_mux;
static panel_wifi_recovery_slot_t s_wifi_recovery_slot;
static bool s_persist_active=true;
static wifi_persist_stage_t s_persist_stage=WIFI_PERSIST_STAGE_IDLE;
static uint32_t s_wifi_event_epoch=1U;
static bool s_deferred_finalize_queued=true, s_deferred_rollback_queued=true;
static bool s_wifi_recovery_quarantine;
static bool s_wifi_recovery_ui_rendered;
static uint32_t s_wifi_recovery_generation;
static bool s_wifi_recovery_can_restore, s_wifi_recovery_can_clear;
static bool s_wifi_txn_marker_present;
static panel_wifi_txn_marker_t s_wifi_txn_marker;
static bool s_wifi_recovery_backup_valid;
static wifi_terminal_ui_state_t s_wifi_terminal_ui_state=WIFI_TERMINAL_UI_NONE;
static bool s_http_stop_in_progress;
static uint64_t s_http_stop_started_ms;
static wifi_context_t s_wifi;
static void wifi_terminal_supervisor_task(void *arg);
static void run_once(void)
{ if(setjmp(exit_after_iteration)==0) wifi_terminal_supervisor_task(NULL); }
'''
watch += supervisor + r'''
int main(void) {
    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    mock_now_ms=0;
    assert(panel_wifi_terminal_begin_prepare(&s_wifi_recovery_slot,31U,0U,30000U));
    s_persist_stage=WIFI_PERSIST_STAGE_HTTP_STOP;s_persist_active=true;
    s_http_stop_in_progress=true;s_http_stop_started_ms=500U;
    mock_now_ms=30500U;run_once();
    assert(s_wifi_recovery_slot.owner==PANEL_WIFI_RECOVERY_OWNER_TIMED_OUT);
    assert(s_wifi_terminal_ui_state==WIFI_TERMINAL_UI_HTTP_STALLED);
    assert(s_wifi_recovery_quarantine && s_wifi_terminal_ui_state!=WIFI_TERMINAL_UI_NONE);
    assert(!panel_wifi_terminal_admit_candidate(&s_wifi_recovery_slot,31U,mock_now_ms));

    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    mock_now_ms=0;
    s_http_stop_in_progress=false;s_http_stop_started_ms=0U;
    assert(panel_wifi_terminal_begin_prepare(&s_wifi_recovery_slot,32U,0U,30000U));
    assert(panel_wifi_terminal_admit_candidate(&s_wifi_recovery_slot,32U,1U));
    assert(panel_wifi_terminal_begin_verify(&s_wifi_recovery_slot,32U,2U,30000U));
    s_persist_stage=WIFI_PERSIST_STAGE_VERIFY_CONNECT;s_persist_active=true;
    s_wifi_txn_marker_present=true;
    s_wifi_txn_marker=(panel_wifi_txn_marker_t){PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING,true,32U};
    s_wifi_recovery_backup_valid=true;s_wifi_recovery_quarantine=false;
    s_deferred_finalize_queued=true;s_deferred_rollback_queued=true;
    mock_now_ms=30002U;run_once();
    assert(s_wifi_recovery_slot.owner==PANEL_WIFI_RECOVERY_OWNER_TIMED_OUT);
    assert(s_wifi_terminal_ui_state==WIFI_TERMINAL_UI_VERIFY_TIMEOUT);
    assert(s_wifi_recovery_quarantine && s_wifi_txn_marker_present);
    assert(!s_deferred_finalize_queued && !s_deferred_rollback_queued);
    assert(!s_wifi_recovery_can_clear && s_wifi_recovery_can_restore);

    panel_wifi_recovery_slot_init(&s_wifi_recovery_slot);
    mock_now_ms=0;
    s_wifi_terminal_ui_state=WIFI_TERMINAL_UI_NONE;
    assert(panel_wifi_terminal_begin_prepare(&s_wifi_recovery_slot,33U,0U,30000U));
    assert(panel_wifi_terminal_admit_candidate(&s_wifi_recovery_slot,33U,1U));
    assert(panel_wifi_terminal_begin_verify(&s_wifi_recovery_slot,33U,2U,30000U));
    assert(panel_wifi_terminal_queue_finalize(&s_wifi_recovery_slot,33U,3U));
    assert(panel_wifi_terminal_claim_finalize(&s_wifi_recovery_slot,33U));
    assert(panel_wifi_terminal_admit_finalize(&s_wifi_recovery_slot,33U,4U));
    s_wifi_recovery_quarantine=false;s_wifi_terminal_ui_state=WIFI_TERMINAL_UI_NONE;
    mock_now_ms=30004U;run_once();
    assert(s_wifi_recovery_slot.owner==PANEL_WIFI_RECOVERY_OWNER_FINALIZE_DURABLE);
    assert(s_wifi_terminal_ui_state==WIFI_TERMINAL_UI_DURABLE_BLOCKED);
    assert(s_wifi_recovery_quarantine && !s_wifi_recovery_can_restore &&
           !s_wifi_recovery_can_clear);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="patch120d4-supervisor-") as tmp:
    source = Path(tmp) / "supervisor_test.c"
    binary = Path(tmp) / "supervisor_test"
    source.write_text(watch)
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(INCLUDE), str(source), str(TXN), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
print("PASS: production supervisor detects HTTP/verify timeout and durable blockage without interrupting ownership")
