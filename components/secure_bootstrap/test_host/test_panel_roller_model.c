#include "panel_roller_model.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void test_position_mapping_direction_and_validation(void)
{
    static const double positions[] = {0.0, 0.25, 0.5, 0.75, 1.0};
    for (uint8_t step = 0U; step <= PANEL_ROLLER_MAX_STEP; ++step) {
        uint8_t actual_step = 99U;
        assert(panel_roller_position_to_step(
            positions[step], PANEL_ROLLER_POSITION_DIRECTION_NORMAL,
            &actual_step));
        assert(actual_step == step);

        assert(panel_roller_position_to_step(
            positions[step], PANEL_ROLLER_POSITION_DIRECTION_INVERTED,
            &actual_step));
        assert(actual_step == PANEL_ROLLER_MAX_STEP - step);
        double target = -1.0;
        assert(panel_roller_step_to_position(
            step, PANEL_ROLLER_POSITION_DIRECTION_INVERTED, &target));
        assert(fabs(target - positions[PANEL_ROLLER_MAX_STEP - step]) < 1e-12);
    }
    uint8_t step = 99U;
    assert(!panel_roller_position_to_step(
        0.5, PANEL_ROLLER_POSITION_DIRECTION_UNKNOWN, &step));
    assert(!panel_roller_position_to_step(
        NAN, PANEL_ROLLER_POSITION_DIRECTION_NORMAL, &step));
    assert(!panel_roller_position_to_step(
        INFINITY, PANEL_ROLLER_POSITION_DIRECTION_NORMAL, &step));
    assert(!panel_roller_position_to_step(
        1.01, PANEL_ROLLER_POSITION_DIRECTION_NORMAL, &step));
    assert(panel_roller_position_to_step(
        0.125, PANEL_ROLLER_POSITION_DIRECTION_NORMAL, &step));
    assert(step == 1U); /* Half-step rounds toward the next step. */
    assert(panel_roller_position_is_five_step_level(0.0));
    assert(panel_roller_position_is_five_step_level(0.25));
    assert(panel_roller_position_is_five_step_level(0.5));
    assert(panel_roller_position_is_five_step_level(0.75));
    assert(panel_roller_position_is_five_step_level(1.0));
    assert(panel_roller_position_is_five_step_level(0.7500005));
    assert(!panel_roller_position_is_five_step_level(0.125));
    assert(!panel_roller_position_is_five_step_level(0.74));
    assert(!panel_roller_position_is_five_step_level(NAN));
}

static void test_awning_command_gate_and_step_targets(void)
{
    panel_roller_command_gate_t gate = {
        .homey_session_valid = true,
        .selected_homey_match = true,
        .active_alias_binding = true,
        .alias_generation_matches = true,
        .capability_pair_verified = true,
        .capability_writable = true,
        .snapshot_fresh = true,
        .availability_known = true,
        .available = true,
        .position_valid = true,
        .position = 1.0,
        .minimum = 0.0,
        .maximum = 1.0,
        .step_present = false,
    };
    assert(panel_roller_command_gate_allows(&gate));
    for (unsigned field = 0U; field < 10U; ++field) {
        panel_roller_command_gate_t invalid = gate;
        switch (field) {
        case 0: invalid.homey_session_valid = false; break;
        case 1: invalid.selected_homey_match = false; break;
        case 2: invalid.active_alias_binding = false; break;
        case 3: invalid.alias_generation_matches = false; break;
        case 4: invalid.capability_pair_verified = false; break;
        case 5: invalid.capability_writable = false; break;
        case 6: invalid.snapshot_fresh = false; break;
        case 7: invalid.availability_known = false; break;
        case 8: invalid.available = false; break;
        case 9: invalid.position_valid = false; break;
        }
        assert(!panel_roller_command_gate_allows(&invalid));
    }
    assert(!panel_roller_target_compatible(-0.01, 0.0, 1.0, false, 0.0));
    assert(!panel_roller_target_compatible(1.01, 0.0, 1.0, false, 0.0));
    assert(panel_roller_target_compatible(0.75, 0.0, 1.0, false, 0.0));
    assert(panel_roller_target_compatible(0.75, 0.0, 1.0, true, 0.25));
    assert(!panel_roller_target_compatible(0.75, 0.0, 1.0, true, 0.3));
    assert(!panel_roller_target_compatible(0.75, 0.0, 1.0, true, 0.0));

    static const double shown_levels[] = {1.0, 0.75, 0.5, 0.25, 0.0};
    for (uint8_t display_step = 0U; display_step <= PANEL_ROLLER_MAX_STEP; ++display_step) {
        const double current = 1.0 - (double)display_step / 4.0;
        uint8_t actual_step = 99U, target_step = 99U;
        double target = -1.0;
        if (display_step < PANEL_ROLLER_MAX_STEP) {
            assert(panel_roller_next_step_target(current, PANEL_ROLLER_DIRECTION_DOWN,
                PANEL_ROLLER_POSITION_DIRECTION_INVERTED, false, 0.0,
                &actual_step, &target_step, &target));
            assert(actual_step == display_step && target_step == display_step + 1U);
            assert(fabs(target - shown_levels[target_step]) < 1e-12);
        } else {
            assert(!panel_roller_next_step_target(current, PANEL_ROLLER_DIRECTION_DOWN,
                PANEL_ROLLER_POSITION_DIRECTION_INVERTED, false, 0.0,
                &actual_step, &target_step, &target));
        }
        if (display_step > 0U) {
            assert(panel_roller_next_step_target(current, PANEL_ROLLER_DIRECTION_UP,
                PANEL_ROLLER_POSITION_DIRECTION_INVERTED, false, 0.0,
                &actual_step, &target_step, &target));
            assert(target_step == display_step - 1U);
            assert(fabs(target - shown_levels[target_step]) < 1e-12);
        } else {
            assert(!panel_roller_next_step_target(current, PANEL_ROLLER_DIRECTION_UP,
                PANEL_ROLLER_POSITION_DIRECTION_INVERTED, false, 0.0,
                &actual_step, &target_step, &target));
        }
    }
    assert(!panel_roller_next_step_target(NAN, PANEL_ROLLER_DIRECTION_DOWN,
        PANEL_ROLLER_POSITION_DIRECTION_INVERTED, false, 0.0, NULL, NULL, NULL));
    uint8_t actual_step = 0U, target_step = 0U;
    double target = 0.0;
    assert(!panel_roller_next_step_target(0.74, PANEL_ROLLER_DIRECTION_UP,
        PANEL_ROLLER_POSITION_DIRECTION_INVERTED, false, 0.0,
        &actual_step, &target_step, &target));
}

static void test_display_states_and_transitions(void)
{
    panel_roller_display_state_t state = panel_roller_display_state(
        true, true, true, true, true, 2U);
    assert(state == PANEL_ROLLER_DISPLAY_POSITION);
    assert(panel_roller_filled_lines(0U) == 0U);
    assert(panel_roller_filled_lines(1U) == 1U);
    assert(panel_roller_filled_lines(2U) == 2U);
    assert(panel_roller_filled_lines(3U) == 3U);
    assert(panel_roller_filled_lines(4U) == 4U);
    assert(panel_roller_filled_lines(5U) == 0U);

    state = panel_roller_display_state(true, true, true, true, false, 0U);
    assert(state == PANEL_ROLLER_DISPLAY_UNKNOWN);
    state = panel_roller_display_state(true, true, false, false, false, 0U);
    assert(state == PANEL_ROLLER_DISPLAY_UNKNOWN);
    state = panel_roller_display_state(true, true, true, false, false, 0U);
    assert(state == PANEL_ROLLER_DISPLAY_UNAVAILABLE);
    state = panel_roller_display_state(true, false, true, false, false, 0U);
    assert(state == PANEL_ROLLER_DISPLAY_UNKNOWN);
    state = panel_roller_display_state(false, true, true, false, false, 0U);
    assert(state == PANEL_ROLLER_DISPLAY_UNKNOWN);
    state = panel_roller_display_state(true, true, true, true, true, 5U);
    assert(state == PANEL_ROLLER_DISPLAY_UNKNOWN);
}

static void test_step_model_bounds_debounce_and_failure(void)
{
    panel_roller_model_t model;
    panel_roller_model_init(&model);
    assert(panel_roller_model_request_step(
        &model, PANEL_ROLLER_DIRECTION_UP, 1000U) == PANEL_ROLLER_REQUEST_REJECTED);

    panel_roller_model_apply_authoritative(&model, true, true, true, true, true, 0U);
    assert(model.position_verified && model.actual_step == 0U);
    assert(panel_roller_model_request_step(
        &model, PANEL_ROLLER_DIRECTION_UP, 1000U) == PANEL_ROLLER_REQUEST_REJECTED);
    assert(panel_roller_model_request_step(
        &model, PANEL_ROLLER_DIRECTION_DOWN, 1000U) == PANEL_ROLLER_REQUEST_ACCEPTED);
    assert(model.command_in_flight && model.desired_step == 1U && model.actual_step == 0U);
    assert(panel_roller_model_request_step(
        &model, PANEL_ROLLER_DIRECTION_DOWN, 1100U) == PANEL_ROLLER_REQUEST_DEBOUNCED);
    assert(panel_roller_model_request_step(
        &model, PANEL_ROLLER_DIRECTION_DOWN, 1500U) == PANEL_ROLLER_REQUEST_REJECTED);
    panel_roller_model_complete(&model, true, true, 1U);
    assert(!model.command_in_flight && model.actual_step == 1U && model.position_verified);

    assert(panel_roller_model_request_step(
        &model, PANEL_ROLLER_DIRECTION_DOWN, 1600U) == PANEL_ROLLER_REQUEST_ACCEPTED);
    panel_roller_model_complete(&model, false, false, 0U);
    assert(model.error && !model.position_verified && !model.desired_valid);

    panel_roller_model_apply_authoritative(&model, true, true, true, true, true, 1U);
    panel_roller_model_apply_authoritative(&model, true, true, true, true, true, 4U);
    assert(panel_roller_model_request_step(
        &model, PANEL_ROLLER_DIRECTION_DOWN, 3000U) == PANEL_ROLLER_REQUEST_REJECTED);
    assert(panel_roller_model_request_step(
        &model, PANEL_ROLLER_DIRECTION_UP, 3500U) == PANEL_ROLLER_REQUEST_ACCEPTED);
    panel_roller_model_tick(&model, 3500U + PANEL_ROLLER_COMMAND_TIMEOUT_MS);
    assert(!model.command_in_flight && !model.position_verified && model.error);

    panel_roller_model_apply_authoritative(&model, true, true, true, true, true, 2U);
    panel_roller_model_apply_authoritative(&model, true, false, true, true, true, 2U);
    assert(!model.position_verified && !model.desired_valid && !model.command_in_flight);
}

int main(void)
{
    test_display_states_and_transitions();
    test_position_mapping_direction_and_validation();
    test_awning_command_gate_and_step_targets();
    test_step_model_bounds_debounce_and_failure();
    puts("PATCH075_ROLLER_MODEL_TESTS PASS");
    return 0;
}
