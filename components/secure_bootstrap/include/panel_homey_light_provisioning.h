#pragma once
#include <stdbool.h>
#include "panel_homey_alias_store.h"

panel_homey_alias_store_result_t panel_homey_light_provisioning_build(
    const char *light_1_device_id,
    const char *light_1_capability_id,
    const char *light_2_device_id,
    const char *light_2_capability_id,
    panel_homey_alias_record_t *record);

bool panel_homey_light_runtime_ready(
    bool live_runtime_ready,
    bool synthetic_phone_ready);

bool panel_homey_light_provisioning_access_allowed(
    bool wifi_online,
    bool homey_ready,
    bool homey_id_present);
