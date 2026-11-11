#!/usr/bin/env python3
"""Exercise the exact production snapshot-confirmation fence with a mock read snapshot."""
from pathlib import Path
import subprocess
import tempfile

component = Path(__file__).resolve().parents[1]
source = (component / "athom_oauth_runtime.c").read_text(encoding="utf-8")
signature = "void athom_oauth_runtime_awning_snapshot_observed(void)"
start = source.find(signature)
if start < 0:
    raise SystemExit("production awning confirmation function missing")
brace = source.find("{", start)
depth = 0
end = None
for index in range(brace, len(source)):
    if source[index] == "{":
        depth += 1
    elif source[index] == "}":
        depth -= 1
        if depth == 0:
            end = index + 1
            break
if end is None:
    raise SystemExit("production awning confirmation function body incomplete")
production = source[start:end]

def extract_function(signature):
    function_start = source.find(signature)
    if function_start < 0:
        raise SystemExit(f"production helper missing: {signature}")
    function_brace = source.find("{", function_start)
    function_depth = 0
    function_end = None
    for index in range(function_brace, len(source)):
        if source[index] == "{":
            function_depth += 1
        elif source[index] == "}":
            function_depth -= 1
            if function_depth == 0:
                function_end = index + 1
                break
    if function_end is None:
        raise SystemExit(f"production helper body incomplete: {signature}")
    return source[function_start:function_end]

resolution = extract_function("static void awning_guard_resolve_on_worker(void)")

prefix = r'''
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PANEL_HOMEY_IDENTITY_DIGEST_SIZE 32U
#include "panel_roller_model.h"
typedef enum {
    PANEL_AWNING_GUARD_UNINITIALIZED = 0,
    PANEL_AWNING_GUARD_READY,
    PANEL_AWNING_GUARD_WRITE_PENDING,
    PANEL_AWNING_GUARD_QUARANTINED,
} panel_awning_guard_state_t;
typedef struct {
    uint8_t state;
    uint32_t sequence;
} panel_awning_guard_record_t;
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_STATE 0x103
static const char *esp_err_to_name(int err) { (void)err; return "mock"; }
#define ESP_LOGE(...) ((void)esp_err_to_name(ESP_OK))
typedef struct {
    uint8_t selected_homey_digest[32];
    uint32_t alias_generation, snapshot_generation;
    double current_position, minimum, maximum, step;
    bool step_present;
} athom_cloud_awning_command_context_t;
typedef struct { int unused; } athom_cloud_state_t;
static athom_cloud_state_t s_cloud;
static int s_refresh_job_mux;
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
static bool s_awning_motion_uncertain;
static bool s_awning_write_result_ready;
typedef enum {
    PATCH085_AWNING_OUTCOME_NOT_ATTEMPTED = 0,
    PATCH085_AWNING_OUTCOME_ACCEPTED,
    PATCH085_AWNING_OUTCOME_REJECTED,
    PATCH085_AWNING_OUTCOME_AMBIGUOUS,
} patch085_awning_outcome_t;
static patch085_awning_outcome_t s_awning_outcome;
static size_t s_awning_pending_widget;
static double s_awning_target_position;
static uint32_t s_awning_confirmation_after_generation;
static athom_cloud_awning_command_context_t s_awning_context;
static panel_roller_interaction_t s_awning_interaction[3];
static uint8_t s_awning_interaction_identity[3][PANEL_HOMEY_IDENTITY_DIGEST_SIZE];
static uint32_t s_awning_interaction_alias_generation[3];
static bool s_awning_next_dispatch_pending;
static size_t s_awning_next_dispatch_widget;
static panel_awning_guard_state_t s_awning_guard_state;
static panel_awning_guard_record_t s_awning_guard_record;
static bool s_awning_guard_resolution_pending;
static uint32_t s_awning_guard_resolution_sequence;
static uint8_t s_awning_guard_resolution_step;
static uint32_t s_awning_guard_resolution_generation;
static void awning_guard_quarantine_locked(void)
{
    s_awning_guard_state = PANEL_AWNING_GUARD_QUARANTINED;
    s_awning_guard_resolution_pending = false;
}
static athom_cloud_awning_command_context_t s_mock_current;
static bool s_mock_context_valid;
static unsigned s_mock_reads;
static int64_t esp_timer_get_time(void) { return 100000000LL; }
static void homey_command_worker_signal_deferred_work(void) { }
static int panel_awning_write_guard_resolve(
    const panel_awning_guard_record_t *expected,
    panel_awning_guard_record_t *ready)
{
    if (expected == NULL || ready == NULL ||
        expected->state != PANEL_AWNING_GUARD_WRITE_PENDING) return ESP_ERR_INVALID_STATE;
    *ready = *expected;
    ready->state = PANEL_AWNING_GUARD_READY;
    return ESP_OK;
}

static bool athom_cloud_get_awning_command_context(
    const athom_cloud_state_t *state, size_t widget,
    athom_cloud_awning_command_context_t *out)
{
    (void)state; (void)widget; s_mock_reads++;
    if (!s_mock_context_valid) return false;
    *out = s_mock_current;
    return true;
}

static void reset(uint32_t base_generation, uint32_t alias_generation,
                  double original, double target)
{
    memset(&s_awning_context, 0, sizeof(s_awning_context));
    memset(&s_mock_current, 0, sizeof(s_mock_current));
    s_awning_motion_uncertain = true;
    s_awning_guard_state = PANEL_AWNING_GUARD_WRITE_PENDING;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_WRITE_PENDING;
    s_awning_guard_record.sequence = 1U;
    s_awning_guard_resolution_pending = false;
    s_awning_write_result_ready = true;
    s_awning_outcome = PATCH085_AWNING_OUTCOME_ACCEPTED;
    s_awning_pending_widget = 1U;
    s_awning_target_position = target;
    s_awning_context.snapshot_generation = base_generation;
    s_awning_confirmation_after_generation = base_generation;
    s_awning_context.alias_generation = alias_generation;
    s_awning_context.current_position = original;
    memset(s_awning_context.selected_homey_digest, 0x5A, 32U);
    panel_roller_interaction_init(&s_awning_interaction[1]);
    uint8_t original_step = 0U, target_step = 0U;
    assert(panel_roller_position_to_step(original,
        PANEL_ROLLER_POSITION_DIRECTION_INVERTED, &original_step));
    assert(panel_roller_position_to_step(target,
        PANEL_ROLLER_POSITION_DIRECTION_INVERTED, &target_step));
    s_awning_interaction[1].active = true;
    s_awning_interaction[1].actual_valid = true;
    s_awning_interaction[1].actual_step = original_step;
    s_awning_interaction[1].active_target_step = target_step;
    s_awning_interaction[1].direction = -1;
    s_awning_interaction[1].base_generation = base_generation;
    s_mock_context_valid = true;
    s_mock_current.snapshot_generation = base_generation + 1U;
    s_mock_current.alias_generation = alias_generation;
    s_mock_current.current_position = target;
    memset(s_mock_current.selected_homey_digest, 0x5A, 32U);
    s_mock_reads = 0U;
}

'''

suffix = r'''
int main(void)
{
    reset(7U, 2U, 0.50, 0.75);
    s_awning_write_result_ready = false;
    s_mock_current.snapshot_generation = 7U;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain && s_mock_reads == 0U);
    s_awning_write_result_ready = true;
    s_awning_confirmation_after_generation = 8U; /* Snapshot seen during the PUT. */

    s_mock_current.snapshot_generation = 8U;
    s_mock_current.current_position = 0.50;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);

    s_mock_current.current_position = 0.75;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain); /* Same observed generation is fenced. */
    s_mock_current.snapshot_generation = 9U;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(!s_awning_motion_uncertain && s_awning_pending_widget == 0U);

    reset(12U, 4U, 0.50, 0.75);
    s_mock_current.alias_generation = 5U;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);

    reset(20U, 8U, 0.50, 0.75);
    s_mock_current.selected_homey_digest[0] ^= 1U;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);

    /* HTTP rejection plus unchanged or matching position is not sufficient
     * terminal evidence; it cannot request durable READY resolution. */
    reset(30U, 9U, 0.50, 0.75);
    s_awning_outcome = PATCH085_AWNING_OUTCOME_REJECTED;
    s_mock_current.snapshot_generation = 31U;
    s_mock_current.current_position = 0.50;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);
    assert(!s_awning_guard_resolution_pending);
    s_mock_current.snapshot_generation = 32U;
    s_mock_current.current_position = 0.75;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain && !s_awning_guard_resolution_pending);

    reset(40U, 10U, 0.50, 0.75);
    s_mock_context_valid = false;
    assert(s_awning_motion_uncertain && !s_mock_context_valid);
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);

    /* 0.50 exactly confirms the requested target. */
    reset(45U, 10U, 0.25, 0.50);
    s_mock_current.snapshot_generation = 46U;
    s_mock_current.current_position = 0.50;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(!s_awning_motion_uncertain);

    /* A successful command response does not establish position by itself.
     * A fresh 0.49 readback is within the bounded 0.015 target tolerance. */
    reset(50U, 11U, 0.25, 0.50);
    s_awning_confirmation_after_generation = 50U;
    s_mock_current.snapshot_generation = 51U;
    s_mock_current.current_position = 0.49;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(!s_awning_motion_uncertain);
    assert(s_awning_pending_widget == 0U);
    assert(fabs(s_mock_current.current_position - 0.49) < 1e-12);

    /* 0.47 is outside the 0.015 tolerance and cannot confirm the target. */
    reset(55U, 12U, 0.25, 0.50);
    s_mock_current.snapshot_generation = 56U;
    s_mock_current.current_position = 0.47;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);

    /* A lower-generation readback is stale even when numerically near target. */
    reset(60U, 13U, 0.25, 0.50);
    s_mock_current.snapshot_generation = 60U;
    s_mock_current.current_position = 0.49;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);
    puts("PATCH085_AWNING_CONFIRMATION_TESTS PASS");
    return 0;
}
'''

suffix = suffix.replace(
    "athom_oauth_runtime_awning_snapshot_observed();",
    "observe();",
)
observer = r'''
static void observe(void)
{
    athom_oauth_runtime_awning_snapshot_observed();
    awning_guard_resolve_on_worker();
}
'''

with tempfile.TemporaryDirectory() as temporary:
    cfile = Path(temporary) / "patch085_confirmation_test.c"
    binary = Path(temporary) / "patch085_confirmation_test"
    cfile.write_text(prefix + production + "\n" + resolution + "\n" + observer + "\n" + suffix,
                     encoding="utf-8")
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        str(cfile), str(component / "panel_roller_model.c"),
        "-I", str(component / "include"), "-lm", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
