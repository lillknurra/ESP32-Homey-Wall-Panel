#include "panel_homey_dashboard_binding.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static panel_homey_read_snapshot_t snapshot_empty(uint32_t generation, uint64_t captured_at_ms)
{
    panel_homey_read_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.generation = generation;
    snapshot.captured_at_ms = captured_at_ms;
    return snapshot;
}

static void add_item(
    panel_homey_read_snapshot_t *snapshot,
    const char *device_alias,
    const char *capability_alias,
    bool available,
    panel_homey_value_type_t value_type,
    bool bool_value)
{
    assert(snapshot != NULL);
    assert(snapshot->item_count < PANEL_HOMEY_SNAPSHOT_MAX_ITEMS);
    panel_homey_read_item_t *item = &snapshot->items[snapshot->item_count++];
    assert(snprintf(item->device_alias, sizeof(item->device_alias), "%s", device_alias) > 0);
    assert(snprintf(item->capability_alias, sizeof(item->capability_alias), "%s", capability_alias) > 0);
    item->available = available;
    item->value_type = value_type;
    item->bool_value = bool_value;
}

static void test_null_arguments(void)
{
    panel_homey_dashboard_state_t state;
    panel_homey_dashboard_state_init(&state);
    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, NULL, 0U, &state) == PANEL_HOMEY_DASHBOARD_APPLY_INVALID);
    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, NULL, 0U, NULL) == PANEL_HOMEY_DASHBOARD_APPLY_INVALID);
    panel_homey_dashboard_state_init(NULL);
}

static void test_not_configured_and_empty(void)
{
    panel_homey_dashboard_state_t state;
    panel_homey_dashboard_state_init(&state);

    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_NOT_CONFIGURED, NULL, 0U, &state) ==
        PANEL_HOMEY_DASHBOARD_APPLY_UNCHANGED);

    panel_homey_read_snapshot_t snapshot = snapshot_empty(1U, 100U);
    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, &snapshot, 100U, &state) ==
        PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);

    for (size_t i = 0U; i < PANEL_UI_WIDGET_COUNT; ++i) {
        assert(state.widgets[i].status == PANEL_WIDGET_UNKNOWN);
        assert(!state.widgets[i].has_boolean);
    }
}

static void test_all_six_and_boolean_policy(void)
{
    panel_homey_dashboard_state_t state;
    panel_homey_dashboard_state_init(&state);
    panel_homey_read_snapshot_t snapshot = snapshot_empty(7U, 1000U);

    add_item(&snapshot, "awning_1", "status", true, PANEL_HOMEY_VALUE_BOOL, true);
    add_item(&snapshot, "awning_2", "status", false, PANEL_HOMEY_VALUE_NONE, false);
    add_item(&snapshot, "awning_3", "status", true, PANEL_HOMEY_VALUE_NONE, false);
    add_item(&snapshot, "security", "active", true, PANEL_HOMEY_VALUE_BOOL, true);
    add_item(&snapshot, "light_1", "on", true, PANEL_HOMEY_VALUE_BOOL, false);
    add_item(&snapshot, "light_2", "on", true, PANEL_HOMEY_VALUE_BOOL, true);

    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, &snapshot, 1000U, &state) ==
        PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);

    assert(state.widgets[0].status == PANEL_WIDGET_AVAILABLE);
    assert(!state.widgets[0].has_boolean);
    assert(state.widgets[1].status == PANEL_WIDGET_UNAVAILABLE);
    assert(!state.widgets[1].has_boolean);
    assert(state.widgets[3].has_boolean && state.widgets[3].boolean_value);
    assert(state.widgets[4].has_boolean && !state.widgets[4].boolean_value);
    assert(state.widgets[5].has_boolean && state.widgets[5].boolean_value);
}

static void test_unknown_reordered_and_capability_mismatch(void)
{
    panel_homey_dashboard_state_t first;
    panel_homey_dashboard_state_t second;
    panel_homey_dashboard_state_init(&first);
    panel_homey_dashboard_state_init(&second);

    panel_homey_read_snapshot_t a = snapshot_empty(4U, 500U);
    add_item(&a, "light_1", "on", true, PANEL_HOMEY_VALUE_BOOL, true);
    add_item(&a, "extra_device", "on", true, PANEL_HOMEY_VALUE_BOOL, true);
    add_item(&a, "security", "wrong_capability", true, PANEL_HOMEY_VALUE_BOOL, true);

    panel_homey_read_snapshot_t b = snapshot_empty(4U, 500U);
    add_item(&b, "security", "wrong_capability", true, PANEL_HOMEY_VALUE_BOOL, true);
    add_item(&b, "extra_device", "on", true, PANEL_HOMEY_VALUE_BOOL, true);
    add_item(&b, "light_1", "on", true, PANEL_HOMEY_VALUE_BOOL, true);

    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, &a, 500U, &first) == PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, &b, 500U, &second) == PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
    assert(memcmp(&first, &second, sizeof(first)) == 0);
    assert(first.widgets[4].status == PANEL_WIDGET_AVAILABLE);
    assert(first.widgets[3].status == PANEL_WIDGET_UNKNOWN);
}

static void test_stale_and_generation(void)
{
    panel_homey_dashboard_state_t state;
    panel_homey_dashboard_state_init(&state);

    panel_homey_read_snapshot_t snapshot = snapshot_empty(9U, 1000U);
    add_item(&snapshot, "light_1", "on", true, PANEL_HOMEY_VALUE_BOOL, true);

    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, &snapshot, 1000U, &state) ==
        PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, &snapshot, 1000U, &state) ==
        PANEL_HOMEY_DASHBOARD_APPLY_UNCHANGED);

    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK,
        &snapshot,
        1000U + PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS + 1U,
        &state) == PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
    assert(state.stale);
    assert(state.widgets[4].status == PANEL_WIDGET_UNAVAILABLE);
    assert(!state.widgets[4].has_boolean);
}

static void test_duplicate_and_invalid_alias_fail_closed(void)
{
    panel_homey_dashboard_state_t state;
    panel_homey_dashboard_state_init(&state);

    panel_homey_read_snapshot_t duplicate = snapshot_empty(2U, 0U);
    add_item(&duplicate, "light_1", "on", true, PANEL_HOMEY_VALUE_BOOL, true);
    add_item(&duplicate, "light_1", "on", true, PANEL_HOMEY_VALUE_BOOL, false);
    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, &duplicate, 0U, &state) ==
        PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
    for (size_t i = 0U; i < PANEL_UI_WIDGET_COUNT; ++i) {
        assert(state.widgets[i].status == PANEL_WIDGET_UNKNOWN);
        assert(!state.widgets[i].has_boolean);
    }

    panel_homey_read_snapshot_t invalid = snapshot_empty(3U, 0U);
    add_item(&invalid, "light-1", "on", true, PANEL_HOMEY_VALUE_BOOL, true);
    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_OK, &invalid, 0U, &state) ==
        PANEL_HOMEY_DASHBOARD_APPLY_INVALID);
}

static void test_alias_table(void)
{
    static const char *const expected_device[] = {
        "awning_1", "awning_2", "awning_3", "security", "light_1", "light_2"
    };
    static const char *const expected_capability[] = {
        "status", "status", "status", "active", "on", "on"
    };

    for (size_t i = 0U; i < PANEL_UI_WIDGET_COUNT; ++i) {
        assert(strcmp(panel_homey_dashboard_device_alias(i), expected_device[i]) == 0);
        assert(strcmp(panel_homey_dashboard_capability_alias(i), expected_capability[i]) == 0);
    }
    assert(panel_homey_dashboard_device_alias(PANEL_UI_WIDGET_COUNT) == NULL);
    assert(panel_homey_dashboard_capability_alias(PANEL_UI_WIDGET_COUNT) == NULL);
}

static void test_non_ok_snapshot_results_fail_safe(void)
{
    panel_homey_dashboard_state_t state;
    panel_homey_dashboard_state_init(&state);

    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_NOT_FOUND, NULL, 0U, &state) ==
        PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
    for (size_t i = 0U; i < PANEL_UI_WIDGET_COUNT; ++i) {
        assert(state.widgets[i].status == PANEL_WIDGET_UNKNOWN);
        assert(!state.widgets[i].has_boolean);
    }

    assert(panel_homey_dashboard_apply_snapshot(
        PANEL_HOMEY_READ_STALE, NULL, 0U, &state) ==
        PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
    assert(state.stale);
    for (size_t i = 0U; i < PANEL_UI_WIDGET_COUNT; ++i) {
        assert(state.widgets[i].status == PANEL_WIDGET_UNAVAILABLE);
        assert(!state.widgets[i].has_boolean);
    }

    const panel_homey_read_result_t degraded_results[] = {
        PANEL_HOMEY_READ_INVALID,
        PANEL_HOMEY_READ_OVERFLOW,
        PANEL_HOMEY_READ_DUPLICATE,
    };
    for (size_t result_index = 0U;
         result_index < sizeof(degraded_results) / sizeof(degraded_results[0]);
         ++result_index) {
        assert(panel_homey_dashboard_apply_snapshot(
            degraded_results[result_index], NULL, 0U, &state) ==
            PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
        assert(!state.stale);
        for (size_t i = 0U; i < PANEL_UI_WIDGET_COUNT; ++i) {
            assert(state.widgets[i].status == PANEL_WIDGET_UNKNOWN);
            assert(!state.widgets[i].has_boolean);
        }
        panel_homey_dashboard_state_init(&state);
        state.stale = true;
    }
}

int main(void)
{
    test_non_ok_snapshot_results_fail_safe();
    test_null_arguments();
    test_not_configured_and_empty();
    test_all_six_and_boolean_policy();
    test_unknown_reordered_and_capability_mismatch();
    test_stale_and_generation();
    test_duplicate_and_invalid_alias_fail_closed();
    test_alias_table();
    puts("PANEL_HOMEY_DASHBOARD_BINDING_TESTS PASS");
    return 0;
}
