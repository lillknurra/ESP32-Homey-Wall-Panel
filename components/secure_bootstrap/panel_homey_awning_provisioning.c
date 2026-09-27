#include "panel_homey_awning_provisioning.h"

#include <stdio.h>
#include <string.h>

#define AWNING_FIRST_WIDGET_INDEX 0U
#define AWNING_LAST_WIDGET_INDEX 2U

static bool private_identifier_valid(const char *value, size_t capacity)
{
    if (value == NULL) {
        return false;
    }

    const size_t length = strnlen(value, capacity);
    if (length == 0U || length >= capacity) {
        return false;
    }

    for (size_t i = 0U; i < length; ++i) {
        const unsigned char c = (unsigned char)value[i];
        if (c < 0x21U || c == 0x7fU) {
            return false;
        }
    }

    return true;
}

static bool same_pair(
    const char *left_device,
    const char *left_capability,
    const char *right_device,
    const char *right_capability)
{
    return strcmp(left_device, right_device) == 0 &&
           strcmp(left_capability, right_capability) == 0;
}

static bool entry_valid(const panel_homey_alias_entry_t *entry)
{
    return entry != NULL &&
           entry->dashboard_binding_index < PANEL_HOMEY_ALIAS_STORE_MAX_ENTRIES &&
           private_identifier_valid(
               entry->raw_device_id,
               sizeof(entry->raw_device_id)) &&
           private_identifier_valid(
               entry->raw_capability_id,
               sizeof(entry->raw_capability_id));
}

static bool append_entry(
    panel_homey_alias_record_t *out,
    uint8_t index,
    const char *device_id,
    const char *capability_id)
{
    if (out->entry_count >= PANEL_HOMEY_ALIAS_STORE_MAX_ENTRIES) {
        return false;
    }

    panel_homey_alias_entry_t *entry = &out->entries[out->entry_count++];
    entry->dashboard_binding_index = index;

    const int device_written = snprintf(
        entry->raw_device_id,
        sizeof(entry->raw_device_id),
        "%s",
        device_id);
    const int capability_written = snprintf(
        entry->raw_capability_id,
        sizeof(entry->raw_capability_id),
        "%s",
        capability_id);

    return device_written > 0 &&
           (size_t)device_written < sizeof(entry->raw_device_id) &&
           capability_written > 0 &&
           (size_t)capability_written < sizeof(entry->raw_capability_id);
}

panel_homey_alias_store_result_t panel_homey_awning_provisioning_merge(
    const panel_homey_alias_record_t *existing,
    const panel_homey_awning_binding_input_t bindings[PANEL_HOMEY_AWNING_BINDING_COUNT],
    panel_homey_alias_record_t *out)
{
    if (bindings == NULL || out == NULL) {
        return PANEL_HOMEY_ALIAS_STORE_INVALID;
    }

    for (size_t i = 0U; i < PANEL_HOMEY_AWNING_BINDING_COUNT; ++i) {
        if (!private_identifier_valid(
                bindings[i].device_id,
                PANEL_HOMEY_RAW_DEVICE_ID_MAX) ||
            !private_identifier_valid(
                bindings[i].capability_id,
                PANEL_HOMEY_RAW_CAPABILITY_ID_MAX)) {
            return PANEL_HOMEY_ALIAS_STORE_INVALID;
        }

        for (size_t j = 0U; j < i; ++j) {
            if (strcmp(
                    bindings[i].device_id,
                    bindings[j].device_id) == 0) {
                return PANEL_HOMEY_ALIAS_STORE_INVALID;
            }
        }
    }

    const panel_homey_alias_entry_t *preserved[3] = {0};

    if (existing != NULL) {
        if (existing->entry_count > PANEL_HOMEY_ALIAS_STORE_MAX_ENTRIES) {
            return PANEL_HOMEY_ALIAS_STORE_INVALID;
        }

        bool used[PANEL_HOMEY_ALIAS_STORE_MAX_ENTRIES] = {false};

        for (size_t i = 0U; i < existing->entry_count; ++i) {
            const panel_homey_alias_entry_t *entry = &existing->entries[i];

            if (!entry_valid(entry) ||
                used[entry->dashboard_binding_index]) {
                return PANEL_HOMEY_ALIAS_STORE_INVALID;
            }

            used[entry->dashboard_binding_index] = true;

            for (size_t j = 0U; j < i; ++j) {
                const panel_homey_alias_entry_t *prior = &existing->entries[j];
                if (same_pair(
                        entry->raw_device_id,
                        entry->raw_capability_id,
                        prior->raw_device_id,
                        prior->raw_capability_id)) {
                    return PANEL_HOMEY_ALIAS_STORE_INVALID;
                }
            }

            if (entry->dashboard_binding_index > AWNING_LAST_WIDGET_INDEX) {
                preserved[entry->dashboard_binding_index - 3U] = entry;
            }
        }
    }

    for (size_t i = 0U; i < PANEL_HOMEY_AWNING_BINDING_COUNT; ++i) {
        for (size_t slot = 0U; slot < 3U; ++slot) {
            const panel_homey_alias_entry_t *entry = preserved[slot];

            if (entry != NULL &&
                same_pair(
                    bindings[i].device_id,
                    bindings[i].capability_id,
                    entry->raw_device_id,
                    entry->raw_capability_id)) {
                return PANEL_HOMEY_ALIAS_STORE_INVALID;
            }
        }
    }

    memset(out, 0, sizeof(*out));

    if (existing != NULL) {
        out->generation = existing->generation;
        memcpy(
            out->homey_identity_digest,
            existing->homey_identity_digest,
            sizeof(out->homey_identity_digest));
    }

    for (size_t i = 0U; i < PANEL_HOMEY_AWNING_BINDING_COUNT; ++i) {
        if (!append_entry(
                out,
                (uint8_t)(AWNING_FIRST_WIDGET_INDEX + i),
                bindings[i].device_id,
                bindings[i].capability_id)) {
            memset(out, 0, sizeof(*out));
            return PANEL_HOMEY_ALIAS_STORE_INVALID;
        }
    }

    for (size_t slot = 0U; slot < 3U; ++slot) {
        const panel_homey_alias_entry_t *entry = preserved[slot];

        if (entry != NULL &&
            !append_entry(
                out,
                entry->dashboard_binding_index,
                entry->raw_device_id,
                entry->raw_capability_id)) {
            memset(out, 0, sizeof(*out));
            return PANEL_HOMEY_ALIAS_STORE_INVALID;
        }
    }

    return PANEL_HOMEY_ALIAS_STORE_OK;
}

bool panel_homey_awning_provisioning_access_allowed(
    bool wifi_online,
    bool homey_ready,
    bool homey_id_present)
{
    return wifi_online && homey_ready && homey_id_present;
}
