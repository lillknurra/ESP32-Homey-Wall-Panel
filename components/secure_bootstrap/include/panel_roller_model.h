#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PANEL_ROLLER_STEP_COUNT 5U
#define PANEL_ROLLER_MAX_STEP 4U
#define PANEL_ROLLER_DEBOUNCE_MS 450ULL
#define PANEL_ROLLER_COMMAND_TIMEOUT_MS 10000ULL

typedef enum {
    PANEL_ROLLER_DISPLAY_UNKNOWN = 0,
    PANEL_ROLLER_DISPLAY_UNAVAILABLE,
    PANEL_ROLLER_DISPLAY_POSITION,
} panel_roller_display_state_t;

typedef enum {
    PANEL_ROLLER_DIRECTION_UP = -1,
    PANEL_ROLLER_DIRECTION_DOWN = 1,
} panel_roller_direction_t;

typedef enum {
    PANEL_ROLLER_POSITION_DIRECTION_UNKNOWN = 0,
    PANEL_ROLLER_POSITION_DIRECTION_NORMAL,
    PANEL_ROLLER_POSITION_DIRECTION_INVERTED,
} panel_roller_position_direction_t;

typedef enum {
    PANEL_ROLLER_REQUEST_REJECTED = 0,
    PANEL_ROLLER_REQUEST_ACCEPTED,
    PANEL_ROLLER_REQUEST_DEBOUNCED,
} panel_roller_request_result_t;

typedef struct {
    bool binding_verified;
    bool snapshot_fresh;
    bool availability_known;
    bool available;
    bool position_verified;
    uint8_t actual_step;
    uint8_t desired_step;
    bool desired_valid;
    bool command_in_flight;
    bool error;
    uint64_t last_request_ms;
    bool last_request_valid;
    uint64_t command_started_ms;
} panel_roller_model_t;

panel_roller_display_state_t panel_roller_display_state(
    bool binding_verified,
    bool snapshot_fresh,
    bool availability_known,
    bool available,
    bool position_verified,
    uint8_t position_step);

uint8_t panel_roller_filled_lines(uint8_t step);
bool panel_roller_position_to_step(
    double position,
    panel_roller_position_direction_t direction,
    uint8_t *step_out);
void panel_roller_model_init(panel_roller_model_t *model);
void panel_roller_model_apply_authoritative(
    panel_roller_model_t *model,
    bool binding_verified,
    bool snapshot_fresh,
    bool availability_known,
    bool available,
    bool position_verified,
    uint8_t position_step);
panel_roller_request_result_t panel_roller_model_request_step(
    panel_roller_model_t *model,
    panel_roller_direction_t direction,
    uint64_t now_ms);
void panel_roller_model_complete(
    panel_roller_model_t *model,
    bool success,
    bool actual_position_verified,
    uint8_t actual_step);
void panel_roller_model_tick(panel_roller_model_t *model, uint64_t now_ms);
