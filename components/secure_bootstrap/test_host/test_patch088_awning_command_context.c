#include "panel_roller_model.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PANEL_HOMEY_IDENTITY_DIGEST_SIZE 32U
#define PANEL_HOMEY_ALIAS_STORE_MAX_ENTRIES 6U
#define PATCH085_CAPABILITY_ID "windowcoverings_set"
#define PATCH085_CAPABILITY_ALIAS "status"
#define ESP_OK 0
#define PANEL_HOMEY_READ_OK 0
#define PANEL_HOMEY_VALUE_NUMBER 2

typedef struct {
    char id[64];
    char remote_url[128];
} selected_homey_t;
typedef struct {
    selected_homey_t selected_homey;
    char homey_session_token[64];
} athom_cloud_state_t;

typedef struct {
    char raw_device_id[64];
    char raw_capability_id[64];
} panel_homey_alias_entry_t;

typedef struct {
    bool availability_known, available;
    int value_type;
    double number_value;
    bool position_binding_verified, position_contract_verified;
    bool position_getable, position_setable, position_step_present;
    double position_min, position_max, position_step;
} panel_homey_read_item_t;

typedef struct {
    bool present, fresh, selected_homey_digest_valid;
    uint32_t alias_generation, generation;
    uint8_t selected_homey_digest[32];
    size_t item_matches;
    panel_homey_read_item_t item;
} panel_homey_awning_snapshot_t;

typedef struct { int unused; } panel_homey_snapshot_store_t;
static panel_homey_snapshot_store_t s_device_snapshot_store;

typedef struct {
    uint8_t selected_homey_digest[32];
    uint32_t alias_generation, snapshot_generation;
    double current_position, minimum, maximum, step;
    bool step_present;
} athom_cloud_awning_command_context_t;

static panel_homey_awning_snapshot_t s_fixture;
static bool s_alias_target_valid = true;
static bool s_selected_digest_valid = true;
static uint32_t s_fixture_generation = 9U;

static int64_t esp_timer_get_time(void) { return 123000; }
static bool patch085_awning_target_supported(size_t widget)
{ return widget == 1U || widget == 2U; }
static bool panel_homey_alias_sha256(const char *id, uint8_t out[32])
{
    if (id == NULL || id[0] == '\0' || out == NULL || !s_selected_digest_valid) return false;
    memset(out, 0xA5, 32U);
    return true;
}
static bool patch085_alias_target(
    const athom_cloud_state_t *state, size_t widget,
    panel_homey_alias_entry_t *entry, uint8_t digest[32], uint32_t *generation)
{
    if (state == NULL || !patch085_awning_target_supported(widget) ||
        entry == NULL || digest == NULL || generation == NULL || !s_alias_target_valid) return false;
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->raw_device_id, "synthetic-device");
    strcpy(entry->raw_capability_id, PATCH085_CAPABILITY_ID);
    memset(digest, 0xA5, 32U);
    *generation = s_fixture.alias_generation;
    return true;
}
static int panel_homey_snapshot_read_awning(
    panel_homey_snapshot_store_t *store, const char *role, const char *capability,
    uint64_t now_ms, panel_homey_awning_snapshot_t *out)
{
    (void)store; (void)now_ms;
    if (role == NULL || capability == NULL || out == NULL) return -1;
    *out = s_fixture;
    return PANEL_HOMEY_READ_OK;
}

/* PATCH088_PRODUCTION_FUNCTION */

static void set_position(double position)
{
    memset(&s_fixture, 0, sizeof(s_fixture));
    s_fixture.present = true;
    s_fixture.fresh = true;
    s_fixture.selected_homey_digest_valid = true;
    memset(s_fixture.selected_homey_digest, 0xA5, 32U);
    s_fixture.alias_generation = 7U;
    s_fixture.generation = s_fixture_generation;
    s_fixture.item_matches = 1U;
    s_fixture.item.availability_known = true;
    s_fixture.item.available = true;
    s_fixture.item.value_type = PANEL_HOMEY_VALUE_NUMBER;
    s_fixture.item.number_value = position;
    s_fixture.item.position_binding_verified = true;
    s_fixture.item.position_contract_verified = true;
    s_fixture.item.position_getable = true;
    s_fixture.item.position_setable = true;
    s_fixture.item.position_min = 0.0;
    s_fixture.item.position_max = 1.0;
}

int main(void)
{
    athom_cloud_state_t state = {0};
    athom_cloud_awning_command_context_t context = {0};
    strcpy(state.selected_homey.id, "synthetic-selected-homey");
    strcpy(state.selected_homey.remote_url, "https://synthetic.invalid");
    strcpy(state.homey_session_token, "synthetic-session");
    static const struct { double actual, up, down; bool up_ok, down_ok; } cases[] = {
        {0.19, 0.53, 0.00, true, true},
        {0.37, 0.53, 0.00, true, true},
        {0.63, 1.00, 0.53, true, true},
        {0.99, 0.00, 0.75, false, true},
        {0.00, 0.25, 0.00, true, false},
    };
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        set_position(cases[i].actual);
        for (size_t widget = 1U; widget <= 2U; ++widget) {
            athom_cloud_awning_command_context_t context = {0};
            assert(athom_cloud_get_awning_command_context(&state, widget, &context));
            assert(fabs(context.current_position - cases[i].actual) < 1e-12);
            uint8_t current = 0U, target_step = 0U;
            double target = -1.0;
            const bool up = panel_roller_next_step_target(context.current_position,
                PANEL_ROLLER_DIRECTION_UP, PANEL_ROLLER_POSITION_DIRECTION_INVERTED,
                context.step_present, context.step, &current, &target_step, &target);
            assert(up == cases[i].up_ok);
            if (up) assert(fabs(target - cases[i].up) < 1e-12);
            const bool down = panel_roller_next_step_target(context.current_position,
                PANEL_ROLLER_DIRECTION_DOWN, PANEL_ROLLER_POSITION_DIRECTION_INVERTED,
                context.step_present, context.step, &current, &target_step, &target);
            assert(down == cases[i].down_ok);
            if (down) assert(fabs(target - cases[i].down) < 1e-12);
        }
    }

    const double invalid[] = {NAN, INFINITY, -INFINITY, -0.01, 1.01};
    for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        set_position(invalid[i]);
        memset(&context, 0, sizeof(context));
        assert(!athom_cloud_get_awning_command_context(&state, 1U, &context));
    }
    set_position(0.49);
    s_fixture.item.position_step_present = true;
    s_fixture.item.position_step = 0.25;
    assert(athom_cloud_get_awning_command_context(&state, 1U, &context));
    uint8_t current = 0U, target_step = 0U;
    double target = -1.0;
    assert(panel_roller_next_step_target(context.current_position,
        PANEL_ROLLER_DIRECTION_UP, PANEL_ROLLER_POSITION_DIRECTION_INVERTED,
        context.step_present, context.step, &current, &target_step, &target));
    assert(fabs(target - 0.75) < 1e-12 &&
           panel_roller_target_compatible(target, context.minimum, context.maximum,
                                          context.step_present, context.step));
    set_position(0.49);
    s_fixture.item.position_step_present = true;
    s_fixture.item.position_step = 0.30;
    assert(athom_cloud_get_awning_command_context(&state, 1U, &context));
    assert(!panel_roller_next_step_target(context.current_position,
        PANEL_ROLLER_DIRECTION_UP, PANEL_ROLLER_POSITION_DIRECTION_INVERTED,
        context.step_present, context.step, &current, &target_step, &target));

    set_position(0.49);
    s_fixture.fresh = false;
    memset(&context, 0, sizeof(context));
    assert(!athom_cloud_get_awning_command_context(&state, 1U, &context));
    set_position(0.49);
    s_fixture.item.position_binding_verified = false;
    assert(!athom_cloud_get_awning_command_context(&state, 2U, &context));
    set_position(0.49);
    s_fixture.item.available = false;
    assert(!athom_cloud_get_awning_command_context(&state, 2U, &context));
    set_position(0.49);
    s_alias_target_valid = false;
    assert(!athom_cloud_get_awning_command_context(&state, 1U, &context));
    s_alias_target_valid = true;
    s_selected_digest_valid = false;
    assert(!athom_cloud_get_awning_command_context(&state, 1U, &context));
    puts("PATCH088_AWNING_CONTEXT_CONTINUOUS_READBACK PASS");
    return 0;
}
