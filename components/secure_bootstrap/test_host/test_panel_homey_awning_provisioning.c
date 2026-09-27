#include "panel_homey_awning_provisioning.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const panel_homey_awning_binding_input_t BINDINGS[3] = {
    {"synthetic-awning-a", "synthetic-state-a"},
    {"synthetic-awning-b", "synthetic-state-b"},
    {"synthetic-awning-c", "synthetic-state-c"},
};

static void set_entry(
    panel_homey_alias_record_t *record,
    size_t position,
    uint8_t slot,
    const char *device,
    const char *capability)
{
    record->entries[position].dashboard_binding_index = slot;
    (void)snprintf(
        record->entries[position].raw_device_id,
        sizeof(record->entries[position].raw_device_id),
        "%s",
        device);
    (void)snprintf(
        record->entries[position].raw_capability_id,
        sizeof(record->entries[position].raw_capability_id),
        "%s",
        capability);
}

static void test_new_record(void)
{
    panel_homey_alias_record_t out = {0};

    assert(panel_homey_awning_provisioning_merge(
        NULL,
        BINDINGS,
        &out) == PANEL_HOMEY_ALIAS_STORE_OK);

    assert(out.entry_count == 3U);

    for (size_t i = 0U; i < 3U; ++i) {
        assert(out.entries[i].dashboard_binding_index == i);
        assert(strcmp(out.entries[i].raw_device_id, BINDINGS[i].device_id) == 0);
        assert(strcmp(out.entries[i].raw_capability_id, BINDINGS[i].capability_id) == 0);
    }
}

static void test_replaces_awnings_and_preserves_lights(void)
{
    panel_homey_alias_record_t existing = {0};
    panel_homey_alias_record_t out = {0};

    existing.generation = 19U;
    memset(
        existing.homey_identity_digest,
        0x5a,
        sizeof(existing.homey_identity_digest));

    existing.entry_count = 5U;
    set_entry(&existing, 0U, 0U, "old-awning-a", "old-capability-a");
    set_entry(&existing, 1U, 1U, "old-awning-b", "old-capability-b");
    set_entry(&existing, 2U, 2U, "old-awning-c", "old-capability-c");
    set_entry(&existing, 3U, 4U, "synthetic-light-a", "onoff");
    set_entry(&existing, 4U, 5U, "synthetic-light-b", "onoff");

    assert(panel_homey_awning_provisioning_merge(
        &existing,
        BINDINGS,
        &out) == PANEL_HOMEY_ALIAS_STORE_OK);

    assert(out.generation == 19U);
    assert(memcmp(
        out.homey_identity_digest,
        existing.homey_identity_digest,
        sizeof(out.homey_identity_digest)) == 0);

    assert(out.entry_count == 5U);
    assert(out.entries[0].dashboard_binding_index == 0U);
    assert(out.entries[1].dashboard_binding_index == 1U);
    assert(out.entries[2].dashboard_binding_index == 2U);
    assert(out.entries[3].dashboard_binding_index == 4U);
    assert(out.entries[4].dashboard_binding_index == 5U);
    assert(strcmp(out.entries[3].raw_device_id, "synthetic-light-a") == 0);
    assert(strcmp(out.entries[4].raw_device_id, "synthetic-light-b") == 0);
}

static void test_preserves_slot_three(void)
{
    panel_homey_alias_record_t existing = {0};
    panel_homey_alias_record_t out = {0};

    existing.entry_count = 3U;
    set_entry(&existing, 0U, 3U, "synthetic-security", "active");
    set_entry(&existing, 1U, 4U, "synthetic-light-a", "onoff");
    set_entry(&existing, 2U, 5U, "synthetic-light-b", "onoff");

    assert(panel_homey_awning_provisioning_merge(
        &existing,
        BINDINGS,
        &out) == PANEL_HOMEY_ALIAS_STORE_OK);

    assert(out.entry_count == 6U);
    assert(out.entries[3].dashboard_binding_index == 3U);
    assert(out.entries[4].dashboard_binding_index == 4U);
    assert(out.entries[5].dashboard_binding_index == 5U);
}

static void test_fail_closed(void)
{
    panel_homey_alias_record_t out = {0};

    panel_homey_awning_binding_input_t duplicate[3] = {
        {"same", "same-cap"},
        {"same", "same-cap"},
        {"other", "other-cap"},
    };

    assert(panel_homey_awning_provisioning_merge(
        NULL,
        duplicate,
        &out) == PANEL_HOMEY_ALIAS_STORE_INVALID);

    panel_homey_alias_record_t existing = {0};
    existing.entry_count = 1U;
    set_entry(&existing, 0U, 4U, "preserved", "onoff");

    panel_homey_awning_binding_input_t collision[3] = {
        {"preserved", "onoff"},
        {"other-a", "cap-a"},
        {"other-b", "cap-b"},
    };

    assert(panel_homey_awning_provisioning_merge(
        &existing,
        collision,
        &out) == PANEL_HOMEY_ALIAS_STORE_INVALID);

    panel_homey_awning_binding_input_t same_device_different_capability[3] = {
        {"same-device", "cap-a"},
        {"same-device", "cap-b"},
        {"other-device", "cap-c"},
    };

    assert(panel_homey_awning_provisioning_merge(
        NULL,
        same_device_different_capability,
        &out) == PANEL_HOMEY_ALIAS_STORE_INVALID);

    panel_homey_awning_binding_input_t invalid[3] = {
        {"", "cap-a"},
        {"other-a", "cap-b"},
        {"other-b", "cap-c"},
    };

    assert(panel_homey_awning_provisioning_merge(
        NULL,
        invalid,
        &out) == PANEL_HOMEY_ALIAS_STORE_INVALID);

    existing.entry_count = 2U;
    set_entry(&existing, 0U, 4U, "light-a", "onoff");
    set_entry(&existing, 1U, 4U, "light-b", "onoff");

    assert(panel_homey_awning_provisioning_merge(
        &existing,
        BINDINGS,
        &out) == PANEL_HOMEY_ALIAS_STORE_INVALID);

    assert(panel_homey_awning_provisioning_merge(
        NULL,
        NULL,
        &out) == PANEL_HOMEY_ALIAS_STORE_INVALID);

    assert(panel_homey_awning_provisioning_merge(
        NULL,
        BINDINGS,
        NULL) == PANEL_HOMEY_ALIAS_STORE_INVALID);
}

static void test_access_policy(void)
{
    assert(!panel_homey_awning_provisioning_access_allowed(false, false, false));
    assert(!panel_homey_awning_provisioning_access_allowed(false, true, true));
    assert(!panel_homey_awning_provisioning_access_allowed(true, false, true));
    assert(!panel_homey_awning_provisioning_access_allowed(true, true, false));
    assert(panel_homey_awning_provisioning_access_allowed(true, true, true));
}

int main(void)
{
    test_new_record();
    test_replaces_awnings_and_preserves_lights();
    test_preserves_slot_three();
    test_fail_closed();
    test_access_policy();

    puts("PATCH051_AWNING_PROVISIONING_HOST_TESTS=PASS");
    return 0;
}
