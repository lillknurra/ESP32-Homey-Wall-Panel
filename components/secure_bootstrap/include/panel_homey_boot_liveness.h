#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PANEL_HOMEY_BOOT_LIVENESS_MAX_ATTEMPTS 3U
#define PANEL_HOMEY_BOOT_LIVENESS_INTERVAL_MS 5000U

typedef struct {
    uint8_t attempts;
    bool attempted;
    bool exhausted_reported;
    uint64_t last_attempt_ms;
} panel_homey_boot_liveness_t;

typedef enum {
    PANEL_HOMEY_LIVENESS_IDLE = 0,
    PANEL_HOMEY_LIVENESS_WAIT,
    PANEL_HOMEY_LIVENESS_ATTEMPT,
    PANEL_HOMEY_LIVENESS_EXHAUSTED,
} panel_homey_liveness_action_t;

panel_homey_liveness_action_t panel_homey_boot_liveness_tick(
    panel_homey_boot_liveness_t *state,
    bool initialization_failed,
    bool wifi_online,
    uint64_t now_ms);
