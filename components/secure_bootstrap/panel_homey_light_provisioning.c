#include "panel_homey_light_provisioning.h"
#include <string.h>
#include <stdio.h>

#define LIGHT_1_WIDGET_INDEX 4U
#define LIGHT_2_WIDGET_INDEX 5U

static bool private_identifier_valid(const char *value, size_t capacity)
{
    if (value == NULL) return false;
    size_t length = strnlen(value, capacity);
    if (length == 0U || length >= capacity) return false;
    for (size_t i = 0U; i < length; ++i) {
        unsigned char c = (unsigned char)value[i];
        if (c < 0x21U || c == 0x7fU) return false;
    }
    return true;
}

panel_homey_alias_store_result_t panel_homey_light_provisioning_build(
    const char *light_1_device_id,
    const char *light_1_capability_id,
    const char *light_2_device_id,
    const char *light_2_capability_id,
    panel_homey_alias_record_t *record)
{
    if (record == NULL ||
        !private_identifier_valid(light_1_device_id, PANEL_HOMEY_RAW_DEVICE_ID_MAX) ||
        !private_identifier_valid(light_1_capability_id, PANEL_HOMEY_RAW_CAPABILITY_ID_MAX) ||
        !private_identifier_valid(light_2_device_id, PANEL_HOMEY_RAW_DEVICE_ID_MAX) ||
        !private_identifier_valid(light_2_capability_id, PANEL_HOMEY_RAW_CAPABILITY_ID_MAX) ||
        (strcmp(light_1_device_id, light_2_device_id) == 0 &&
         strcmp(light_1_capability_id, light_2_capability_id) == 0)) {
        return PANEL_HOMEY_ALIAS_STORE_INVALID;
    }
    memset(record, 0, sizeof(*record));
    record->entry_count = 2U;
    record->entries[0].dashboard_binding_index = LIGHT_1_WIDGET_INDEX;
    record->entries[1].dashboard_binding_index = LIGHT_2_WIDGET_INDEX;
    (void)snprintf(record->entries[0].raw_device_id, sizeof(record->entries[0].raw_device_id), "%s", light_1_device_id);
    (void)snprintf(record->entries[0].raw_capability_id, sizeof(record->entries[0].raw_capability_id), "%s", light_1_capability_id);
    (void)snprintf(record->entries[1].raw_device_id, sizeof(record->entries[1].raw_device_id), "%s", light_2_device_id);
    (void)snprintf(record->entries[1].raw_capability_id, sizeof(record->entries[1].raw_capability_id), "%s", light_2_capability_id);
    return PANEL_HOMEY_ALIAS_STORE_OK;
}
bool panel_homey_light_runtime_ready(
    bool live_runtime_ready,
    bool synthetic_phone_ready)
{
    (void)synthetic_phone_ready;
    return live_runtime_ready;
}

bool panel_homey_light_provisioning_access_allowed(
    bool wifi_online,
    bool homey_ready,
    bool homey_id_present)
{
    return wifi_online && homey_ready && homey_id_present;
}
