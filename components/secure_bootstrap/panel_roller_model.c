#include "panel_roller_model.h"

#include <math.h>
#include <string.h>

bool panel_roller_position_to_step(
    double position,
    panel_roller_position_direction_t direction,
    uint8_t *step_out)
{
    if (step_out == NULL || !isfinite(position) || position < 0.0 || position > 1.0 ||
        (direction != PANEL_ROLLER_POSITION_DIRECTION_NORMAL &&
         direction != PANEL_ROLLER_POSITION_DIRECTION_INVERTED)) {
        return false;
    }
    const double normalized = direction == PANEL_ROLLER_POSITION_DIRECTION_NORMAL
        ? position : 1.0 - position;
    *step_out = (uint8_t)floor(normalized * (double)PANEL_ROLLER_MAX_STEP + 0.5);
    return *step_out <= PANEL_ROLLER_MAX_STEP;
}

panel_roller_display_state_t panel_roller_display_state(
    bool binding_verified,
    bool snapshot_fresh,
    bool availability_known,
    bool available,
    bool position_verified,
    uint8_t position_step)
{
    if (!binding_verified || !snapshot_fresh) {
        return PANEL_ROLLER_DISPLAY_UNKNOWN;
    }
    if (availability_known && !available) {
        return PANEL_ROLLER_DISPLAY_UNAVAILABLE;
    }
    if (!availability_known || !available || !position_verified ||
        position_step > PANEL_ROLLER_MAX_STEP) {
        return PANEL_ROLLER_DISPLAY_UNKNOWN;
    }
    return PANEL_ROLLER_DISPLAY_POSITION;
}

uint8_t panel_roller_filled_lines(uint8_t step)
{
    return step <= PANEL_ROLLER_MAX_STEP ? step : 0U;
}

void panel_roller_model_init(panel_roller_model_t *model)
{
    if (model != NULL) memset(model, 0, sizeof(*model));
}

void panel_roller_model_apply_authoritative(
    panel_roller_model_t *model,
    bool binding_verified,
    bool snapshot_fresh,
    bool availability_known,
    bool available,
    bool position_verified,
    uint8_t position_step)
{
    if (model == NULL) return;
    model->binding_verified = binding_verified;
    model->snapshot_fresh = snapshot_fresh;
    model->availability_known = availability_known;
    model->available = available;
    if (!binding_verified || !snapshot_fresh || !availability_known || !available ||
        !position_verified || position_step > PANEL_ROLLER_MAX_STEP) {
        model->position_verified = false;
        model->actual_step = 0U;
        model->desired_valid = false;
        model->command_in_flight = false;
        return;
    }
    model->position_verified = true;
    model->actual_step = position_step;
    if (!model->command_in_flight) {
        model->desired_step = position_step;
        model->desired_valid = true;
    }
    model->error = false;
}

panel_roller_request_result_t panel_roller_model_request_step(
    panel_roller_model_t *model,
    panel_roller_direction_t direction,
    uint64_t now_ms)
{
    if (model == NULL ||
        (direction != PANEL_ROLLER_DIRECTION_UP && direction != PANEL_ROLLER_DIRECTION_DOWN) ||
        !model->binding_verified || !model->snapshot_fresh ||
        !model->availability_known || !model->available || !model->position_verified) {
        return PANEL_ROLLER_REQUEST_REJECTED;
    }
    if (model->last_request_valid &&
        (now_ms < model->last_request_ms ||
         now_ms - model->last_request_ms < PANEL_ROLLER_DEBOUNCE_MS)) {
        return PANEL_ROLLER_REQUEST_DEBOUNCED;
    }
    if (model->command_in_flight) return PANEL_ROLLER_REQUEST_REJECTED;

    const uint8_t base = model->desired_valid ? model->desired_step : model->actual_step;
    uint8_t target = base;
    if (direction == PANEL_ROLLER_DIRECTION_UP) {
        if (target == 0U) return PANEL_ROLLER_REQUEST_REJECTED;
        target--;
    } else {
        if (target >= PANEL_ROLLER_MAX_STEP) return PANEL_ROLLER_REQUEST_REJECTED;
        target++;
    }
    model->desired_step = target;
    model->desired_valid = true;
    model->command_in_flight = true;
    model->command_started_ms = now_ms;
    model->last_request_ms = now_ms;
    model->last_request_valid = true;
    model->error = false;
    return PANEL_ROLLER_REQUEST_ACCEPTED;
}

void panel_roller_model_complete(
    panel_roller_model_t *model,
    bool success,
    bool actual_position_verified,
    uint8_t actual_step)
{
    if (model == NULL || !model->command_in_flight) return;
    model->command_in_flight = false;
    if (!success || !actual_position_verified || actual_step > PANEL_ROLLER_MAX_STEP) {
        model->error = true;
        model->position_verified = false;
        model->desired_valid = false;
        return;
    }
    model->actual_step = actual_step;
    model->position_verified = true;
    model->desired_valid = false;
    model->error = false;
}

void panel_roller_model_tick(panel_roller_model_t *model, uint64_t now_ms)
{
    if (model == NULL || !model->command_in_flight || now_ms < model->command_started_ms) return;
    if (now_ms - model->command_started_ms >= PANEL_ROLLER_COMMAND_TIMEOUT_MS) {
        model->command_in_flight = false;
        model->position_verified = false;
        model->desired_valid = false;
        model->error = true;
    }
}
