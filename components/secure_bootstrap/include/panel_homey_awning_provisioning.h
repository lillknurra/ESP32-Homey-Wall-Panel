#pragma once
#include <stdbool.h>
#include "panel_homey_alias_store.h"

#define PANEL_HOMEY_AWNING_BINDING_COUNT 3U

typedef struct {
    const char *device_id;
    const char *capability_id;
} panel_homey_awning_binding_input_t;

panel_homey_alias_store_result_t panel_homey_awning_provisioning_merge(
    const panel_homey_alias_record_t *existing,
    const panel_homey_awning_binding_input_t bindings[PANEL_HOMEY_AWNING_BINDING_COUNT],
    panel_homey_alias_record_t *out);

bool panel_homey_awning_provisioning_access_allowed(
    bool wifi_online,
    bool homey_ready,
    bool homey_id_present);
