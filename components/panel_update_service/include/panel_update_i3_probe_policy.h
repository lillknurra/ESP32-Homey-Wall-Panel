/* Patch116I3: pure-C, one-owner passive update discovery experiment gate.
 * It never initiates networking, writes flash, or authorizes OTA.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define PANEL_UPDATE_I3_SAMPLE_MS UINT64_C(60000)
#define PANEL_UPDATE_I3_BOOT_GRACE_MS UINT64_C(180000)
#define PANEL_UPDATE_I3_MIN_INTERNAL_FREE (64U * 1024U)
#define PANEL_UPDATE_I3_MIN_LARGEST_BLOCK (24U * 1024U)
#define PANEL_UPDATE_I3_MIN_OWNER_STACK_MARGIN 3072U
#define PANEL_UPDATE_I3_REQUIRED_HEALTHY_SAMPLES 3U

typedef struct {
    uint64_t now_ms;
    uint32_t internal_free_bytes;
    uint32_t internal_largest_bytes;
    uint32_t internal_minimum_free_bytes;
    uint32_t owner_stack_margin_bytes;
    bool wifi_online;
    bool ui_allocated;
    bool homey_ready; /* observational only, NOT Homey approval or a command */
    bool operator_trial_approved; /* independent explicit operator gate */
} panel_update_i3_sample_t;

typedef struct {
    uint64_t initialized_ms;
    uint64_t previous_ms;
    uint64_t last_sample_ms;
    uint32_t count;
    uint32_t consecutive_healthy;
    uint32_t lowest_internal_free;
    uint32_t lowest_largest_block;
    uint32_t lowest_owner_stack_margin;
    bool initialized;
    bool stopped;
    bool trial_claimed; /* terminal once per boot, even if worker bootstrap fails */
} panel_update_i3_policy_t;

typedef struct {
    bool sampled;
    bool sample_healthy;
    bool one_shot_trial_request;
} panel_update_i3_decision_t;

void panel_update_i3_policy_init(panel_update_i3_policy_t *policy);
/* Caller: exactly one task, no reentrancy. No clocks/heap/network inside. */
panel_update_i3_decision_t panel_update_i3_policy_step(
    panel_update_i3_policy_t *policy,
    const panel_update_i3_sample_t *sample,
    bool one_shot_trial_compiled);
