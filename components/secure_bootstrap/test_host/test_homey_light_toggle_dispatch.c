#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../athom_cloud_client.c"
#include "../athom_oauth_runtime.c"

static panel_homey_alias_runtime_t configured_runtime(void)
{
    panel_homey_alias_runtime_t runtime;
    memset(&runtime, 0, sizeof(runtime));
    runtime.configured = true;
    runtime.record.entry_count = 2U;

    runtime.record.entries[0].dashboard_binding_index = 4U;
    strcpy(runtime.record.entries[0].raw_device_id, "synthetic-light-a");
    strcpy(runtime.record.entries[0].raw_capability_id, "onoff");

    runtime.record.entries[1].dashboard_binding_index = 5U;
    strcpy(runtime.record.entries[1].raw_device_id, "synthetic-light-b");
    strcpy(runtime.record.entries[1].raw_capability_id, "onoff");
    return runtime;
}

static void test_dispatch_gate(void)
{
    assert(patch037_light_toggle_dispatch_gate(4U, true, true));
    assert(patch037_light_toggle_dispatch_gate(5U, true, true));

    assert(!patch037_light_toggle_dispatch_gate(4U, false, true));
    assert(!patch037_light_toggle_dispatch_gate(5U, false, true));
    assert(!patch037_light_toggle_dispatch_gate(4U, true, false));
    assert(!patch037_light_toggle_dispatch_gate(5U, true, false));

    for (size_t widget = 0U; widget < 4U; ++widget) {
        assert(!patch037_light_toggle_dispatch_gate(widget, true, true));
    }
    assert(!patch037_light_toggle_dispatch_gate(6U, true, true));
    assert(!patch037_light_toggle_dispatch_gate((size_t)-1, true, true));
}

static void test_private_target_resolution(void)
{
    panel_homey_alias_runtime_t runtime = configured_runtime();
    char device_id[PANEL_HOMEY_RAW_DEVICE_ID_MAX];

    assert(patch037_copy_private_light_target(
               &runtime, 4U, device_id, sizeof(device_id)) ==
           PATCH037_PRIVATE_TARGET_OK);
    assert(strcmp(device_id, "synthetic-light-a") == 0);

    assert(patch037_copy_private_light_target(
               &runtime, 5U, device_id, sizeof(device_id)) ==
           PATCH037_PRIVATE_TARGET_OK);
    assert(strcmp(device_id, "synthetic-light-b") == 0);

    runtime.record.entry_count = 1U;
    assert(patch037_copy_private_light_target(
               &runtime, 5U, device_id, sizeof(device_id)) ==
           PATCH037_PRIVATE_TARGET_NOT_FOUND);

    runtime = configured_runtime();
    strcpy(runtime.record.entries[0].raw_capability_id, "dim");
    assert(patch037_copy_private_light_target(
               &runtime, 4U, device_id, sizeof(device_id)) ==
           PATCH037_PRIVATE_TARGET_INVALID);

    runtime = configured_runtime();
    runtime.configured = false;
    assert(patch037_copy_private_light_target(
               &runtime, 4U, device_id, sizeof(device_id)) ==
           PATCH037_PRIVATE_TARGET_NOT_FOUND);
}

static void test_fixed_endpoint_and_boolean_body(void)
{
    char path[PATCH037_LIGHT_WRITE_PATH_MAX];
    char body[PATCH037_LIGHT_WRITE_BODY_MAX];

    assert(patch037_build_light_write_path(
        "synthetic light/a", path, sizeof(path)));
    assert(strcmp(
        path,
        "/api/manager/devices/device/synthetic%20light%2Fa/capability/onoff") == 0);

    assert(patch037_build_light_write_body(true, body, sizeof(body)));
    assert(strcmp(body, "{\"value\":true}") == 0);
    assert(strstr(body, "opts") == NULL);
    assert(strstr(body, "transactionId") == NULL);

    assert(patch037_build_light_write_body(false, body, sizeof(body)));
    assert(strcmp(body, "{\"value\":false}") == 0);
    assert(strstr(body, "opts") == NULL);
    assert(strstr(body, "transactionId") == NULL);
}

static void test_write_result_classification(void)
{
    assert(patch037_classify_light_write(true, true, 200) ==
           ATHOM_HOMEY_LIGHT_WRITE_ACCEPTED);
    assert(patch037_classify_light_write(true, true, 204) ==
           ATHOM_HOMEY_LIGHT_WRITE_ACCEPTED);
    assert(patch037_classify_light_write(true, true, 299) ==
           ATHOM_HOMEY_LIGHT_WRITE_ACCEPTED);

    assert(patch037_classify_light_write(true, true, 401) ==
           ATHOM_HOMEY_LIGHT_WRITE_UNAUTHORIZED);

    assert(patch037_classify_light_write(true, true, 300) ==
           ATHOM_HOMEY_LIGHT_WRITE_REJECTED);
    assert(patch037_classify_light_write(true, true, 400) ==
           ATHOM_HOMEY_LIGHT_WRITE_REJECTED);
    assert(patch037_classify_light_write(true, true, 403) ==
           ATHOM_HOMEY_LIGHT_WRITE_REJECTED);
    assert(patch037_classify_light_write(true, true, 500) ==
           ATHOM_HOMEY_LIGHT_WRITE_REJECTED);

    assert(patch037_classify_light_write(true, false, 0) ==
           ATHOM_HOMEY_LIGHT_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch037_classify_light_write(true, false, 200) ==
           ATHOM_HOMEY_LIGHT_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch037_classify_light_write(false, false, 0) ==
           ATHOM_HOMEY_LIGHT_WRITE_INTERNAL_ERROR);
    assert(patch037_classify_light_write(true, true, 0) ==
           ATHOM_HOMEY_LIGHT_WRITE_INTERNAL_ERROR);
}

static void test_runtime_result_mapping(void)
{
    assert(strcmp(
               patch037_light_write_result_name(ATHOM_HOMEY_LIGHT_WRITE_ACCEPTED),
               "accepted") == 0);
    assert(strcmp(
               patch037_light_toggle_dispatch_result_name(
                   ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED),
               "accepted") == 0);

    assert(patch037_map_light_write_result(
               ATHOM_HOMEY_LIGHT_WRITE_ACCEPTED) ==
           ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED);
    assert(patch037_map_light_write_result(
               ATHOM_HOMEY_LIGHT_WRITE_NOT_READY) ==
           ATHOM_LIGHT_TOGGLE_DISPATCH_NOT_READY);
    assert(patch037_map_light_write_result(
               ATHOM_HOMEY_LIGHT_WRITE_TARGET_NOT_FOUND) ==
           ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_NOT_FOUND);
    assert(patch037_map_light_write_result(
               ATHOM_HOMEY_LIGHT_WRITE_TARGET_INVALID) ==
           ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_INVALID);
    assert(patch037_map_light_write_result(
               ATHOM_HOMEY_LIGHT_WRITE_UNAUTHORIZED) ==
           ATHOM_LIGHT_TOGGLE_DISPATCH_UNAUTHORIZED);
    assert(patch037_map_light_write_result(
               ATHOM_HOMEY_LIGHT_WRITE_REJECTED) ==
           ATHOM_LIGHT_TOGGLE_DISPATCH_REJECTED);
    assert(patch037_map_light_write_result(
               ATHOM_HOMEY_LIGHT_WRITE_TRANSPORT_AMBIGUOUS) ==
           ATHOM_LIGHT_TOGGLE_DISPATCH_TRANSPORT_AMBIGUOUS);
    assert(patch037_map_light_write_result(
               ATHOM_HOMEY_LIGHT_WRITE_INTERNAL_ERROR) ==
           ATHOM_LIGHT_TOGGLE_DISPATCH_INTERNAL_ERROR);
}

int main(void)
{
    test_dispatch_gate();
    test_private_target_resolution();
    test_fixed_endpoint_and_boolean_body();
    test_write_result_classification();
    test_runtime_result_mapping();

    puts("PATCH037_HOMEY_LIGHT_TOGGLE_DISPATCH_TEST PASS");
    return 0;
}
