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

prefix = r'''
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PANEL_HOMEY_IDENTITY_DIGEST_SIZE 32U
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
static size_t s_awning_pending_widget;
static double s_awning_target_position;
static uint32_t s_awning_confirmation_after_generation;
static bool s_awning_reconcile_unchanged_allowed;
static athom_cloud_awning_command_context_t s_awning_context;
static athom_cloud_awning_command_context_t s_mock_current;
static bool s_mock_context_valid;
static unsigned s_mock_reads;

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
    s_awning_write_result_ready = true;
    s_awning_pending_widget = 1U;
    s_awning_target_position = target;
    s_awning_context.snapshot_generation = base_generation;
    s_awning_confirmation_after_generation = base_generation;
    s_awning_context.alias_generation = alias_generation;
    s_awning_context.current_position = original;
    memset(s_awning_context.selected_homey_digest, 0x5A, 32U);
    s_awning_reconcile_unchanged_allowed = false;
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

    reset(30U, 9U, 0.50, 0.75);
    s_mock_current.current_position = 0.50;
    s_awning_reconcile_unchanged_allowed = true;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(!s_awning_motion_uncertain);

    reset(40U, 10U, 0.50, 0.75);
    s_mock_context_valid = false;
    assert(s_awning_motion_uncertain && !s_mock_context_valid);
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);
    puts("PATCH085_AWNING_CONFIRMATION_TESTS PASS");
    return 0;
}
'''

with tempfile.TemporaryDirectory() as temporary:
    cfile = Path(temporary) / "patch085_confirmation_test.c"
    binary = Path(temporary) / "patch085_confirmation_test"
    cfile.write_text(prefix + production + "\n" + suffix, encoding="utf-8")
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        str(cfile), "-lm", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
