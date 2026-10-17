#include "panel_homey_dashboard_binding.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static panel_homey_read_snapshot_t fixture(void)
{
    panel_homey_read_snapshot_t snapshot = {0};
    snapshot.generation = 4U;
    snapshot.captured_at_ms = 1000U;
    snapshot.item_count = 3U;
    for (size_t i = 0U; i < 3U; ++i) {
        panel_homey_read_item_t *item = &snapshot.items[i];
        (void)snprintf(item->device_alias, sizeof(item->device_alias), "awning_%u", (unsigned)i + 1U);
        (void)snprintf(item->capability_alias, sizeof(item->capability_alias), "status");
        item->availability_known = true;
        item->available = true;
        item->position_binding_verified = true;
        item->value_type = PANEL_HOMEY_VALUE_NUMBER;
        item->number_value = 0.99;
    }
    return snapshot;
}

static panel_ui_model_t apply(const panel_homey_read_snapshot_t *snapshot, uint64_t now)
{
    panel_homey_dashboard_state_t state;
    panel_homey_dashboard_state_init(&state);
    assert(panel_homey_dashboard_apply_snapshot(PANEL_HOMEY_READ_OK, snapshot, now, &state)
           == PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);
    /* Favorites own widgets 4/5 and must not be overwritten by this path. */
    model.widget_status[4] = PANEL_WIDGET_AVAILABLE;
    model.widget_has_boolean[4] = true;
    model.widget_boolean_value[4] = true;
    model.widget_status[5] = PANEL_WIDGET_AVAILABLE;
    model.widget_has_boolean[5] = true;
    model.widget_boolean_value[5] = false;
    assert(panel_ui_apply_homey_dashboard_state(&model, &state));
    assert(!model.roller_position_valid[0]);
    assert(!model.roller_readback_binding_verified[0]);
    assert(model.widget_has_boolean[4] && model.widget_boolean_value[4]);
    assert(model.widget_has_boolean[5] && !model.widget_boolean_value[5]);
    return model;
}

int main(void)
{
    static const double values[] = {0.99, 1.0, 0.9, 0.875, 0.75, 0.63, 0.5, 0.25, 0.01, 0.0};
    static const uint8_t steps[] = {0U, 0U, 0U, 1U, 1U, 1U, 2U, 3U, 4U, 4U};
    for (size_t n = 0U; n < sizeof(values) / sizeof(values[0]); ++n) {
        panel_homey_read_snapshot_t snapshot = fixture();
        for (size_t role = 1U; role <= 2U; ++role) snapshot.items[role].number_value = values[n];
        panel_ui_model_t model = apply(&snapshot, 1000U);
        for (size_t role = 1U; role <= 2U; ++role) {
            assert(model.roller_position_valid[role]);
            assert(model.roller_position_step[role] == steps[n]);
            assert(model.roller_readback_value[role] == values[n]);
            /* Preserve the raw readback: display rounding grants no write authority. */
            uint8_t from = 0U, to = 0U;
            double target = 0.0;
            if (!panel_roller_position_is_five_step_level(values[n])) {
                assert(!panel_roller_next_step_target(model.roller_readback_value[role],
                    PANEL_ROLLER_DIRECTION_DOWN, PANEL_ROLLER_POSITION_DIRECTION_INVERTED,
                    false, 0.0, &from, &to, &target));
                assert(!panel_roller_next_step_target(model.roller_readback_value[role],
                    PANEL_ROLLER_DIRECTION_UP, PANEL_ROLLER_POSITION_DIRECTION_INVERTED,
                    false, 0.0, &from, &to, &target));
            }
        }
    }
    const double invalid[] = {NAN, INFINITY, -INFINITY, -0.01, 1.01};
    for (size_t n = 0U; n < sizeof(invalid) / sizeof(invalid[0]); ++n) {
        panel_homey_read_snapshot_t snapshot = fixture();
        snapshot.items[1].number_value = snapshot.items[2].number_value = invalid[n];
        panel_ui_model_t model = apply(&snapshot, 1000U);
        assert(!model.roller_position_valid[1] && !model.roller_position_valid[2]);
    }
    for (unsigned mode = 0U; mode < 6U; ++mode) {
        panel_homey_read_snapshot_t snapshot = fixture();
        for (size_t role = 1U; role <= 2U; ++role) {
            if (mode == 0U) snapshot.items[role].position_binding_verified = false;
            if (mode == 1U) snapshot.items[role].available = false;
            if (mode == 2U) snapshot.items[role].availability_known = false;
            if (mode == 3U) snapshot.items[role].value_type = PANEL_HOMEY_VALUE_NONE;
            if (mode == 4U) snapshot.items[role].value_type = PANEL_HOMEY_VALUE_BOOL;
        }
        panel_ui_model_t model = apply(&snapshot, mode == 5U
            ? 1001U + PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS : 1000U);
        assert(!model.roller_position_valid[1] && !model.roller_position_valid[2]);
    }
    puts("PATCH086_DISPLAY_READBACK PASS: both roles, raw values, rounding, invalid/stale, strict write separation, Markis disabled, Favorites ownership");
    return 0;
}
