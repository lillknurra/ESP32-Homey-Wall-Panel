/* Patch116I3: passive in-RAM telemetry, default-off in all normal firmware.
 * A guarded one-shot metadata request is COMPILE-AUDIT ONLY, not approved for
 * deployment. Neither mode authorizes an OTA write or Homey operation.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifndef PANEL_UPDATE_I3_OBSERVER_ENABLED
#define PANEL_UPDATE_I3_OBSERVER_ENABLED 0
#endif
#ifndef PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED
#define PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED 0
#endif
#if (PANEL_UPDATE_I3_OBSERVER_ENABLED != 0 && PANEL_UPDATE_I3_OBSERVER_ENABLED != 1) || \
    (PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED != 0 && PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED != 1)
#error "Patch116I3 feature values must be 0 or 1"
#endif
#if PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED && !PANEL_UPDATE_I3_OBSERVER_ENABLED
#error "Patch116I3 network experiment requires passive observer"
#endif

typedef struct {
    uint64_t monotonic_ms;
    bool wifi_online;
    bool ui_allocated;
    bool homey_ready;
    bool operator_trial_approved; /* always false at the real call site */
} panel_update_i3_owner_input_t;

/* All calls must come from the SAME pre-existing rotation task.
 * No new task, no flash, no persistent writes, no Wi-Fi or Homey calls.
 * Disabled default is a no-op and creates no network call site. */
/* Separate active ABI: if the call site is compiled ON but this component is
 * compiled OFF, the linker must fail instead of silently skipping telemetry. */
#if PANEL_UPDATE_I3_OBSERVER_ENABLED
void panel_update_i3_owner_active_tick(const panel_update_i3_owner_input_t *input);
#else
void panel_update_i3_owner_tick(const panel_update_i3_owner_input_t *input);
#endif
