#include "panel_homey_dashboard_binding.h"

#include <string.h>

typedef struct {
    const char *device_alias;
    const char *capability_alias;
    bool boolean_allowed;
} dashboard_binding_t;

static const dashboard_binding_t BINDINGS[PANEL_HOMEY_DASHBOARD_BINDING_COUNT] = {
    {PANEL_HOMEY_DASHBOARD_ALIAS_AWNING_1, PANEL_HOMEY_DASHBOARD_CAPABILITY_STATUS, false},
    {PANEL_HOMEY_DASHBOARD_ALIAS_AWNING_2, PANEL_HOMEY_DASHBOARD_CAPABILITY_STATUS, false},
    {PANEL_HOMEY_DASHBOARD_ALIAS_AWNING_3, PANEL_HOMEY_DASHBOARD_CAPABILITY_STATUS, false},
    {PANEL_HOMEY_DASHBOARD_ALIAS_SECURITY, PANEL_HOMEY_DASHBOARD_CAPABILITY_ACTIVE, true},
    {PANEL_HOMEY_DASHBOARD_ALIAS_LIGHT_1, PANEL_HOMEY_DASHBOARD_CAPABILITY_ON, true},
    {PANEL_HOMEY_DASHBOARD_ALIAS_LIGHT_2, PANEL_HOMEY_DASHBOARD_CAPABILITY_ON, true},
};

static bool alias_valid(const char *value, size_t capacity)
{
    if (value == NULL || capacity == 0U) {
        return false;
    }

    size_t length = strnlen(value, capacity);
    if (length == 0U || length >= capacity) {
        return false;
    }

    for (size_t i = 0U; i < length; ++i) {
        const unsigned char c = (unsigned char)value[i];
        if (!((c >= (unsigned char)'a' && c <= (unsigned char)'z') ||
              (c >= (unsigned char)'0' && c <= (unsigned char)'9') ||
              c == (unsigned char)'_')) {
            return false;
        }
    }

    return true;
}

static void clear_widgets(
    panel_homey_dashboard_state_t *state,
    panel_widget_status_t status)
{
    for (size_t i = 0U; i < PANEL_HOMEY_DASHBOARD_BINDING_COUNT; ++i) {
        state->widgets[i].status = status;
        state->widgets[i].has_boolean = false;
        state->widgets[i].boolean_value = false;
    }
}

static bool state_equal(
    const panel_homey_dashboard_state_t *left,
    const panel_homey_dashboard_state_t *right)
{
    if (left->generation != right->generation ||
        left->generation_valid != right->generation_valid ||
        left->stale != right->stale) {
        return false;
    }

    for (size_t i = 0U; i < PANEL_HOMEY_DASHBOARD_BINDING_COUNT; ++i) {
        if (left->widgets[i].status != right->widgets[i].status ||
            left->widgets[i].has_boolean != right->widgets[i].has_boolean ||
            left->widgets[i].boolean_value != right->widgets[i].boolean_value) {
            return false;
        }
    }

    return true;
}

static int binding_index(
    const char *device_alias,
    const char *capability_alias)
{
    for (size_t i = 0U; i < PANEL_HOMEY_DASHBOARD_BINDING_COUNT; ++i) {
        if (strcmp(device_alias, BINDINGS[i].device_alias) == 0 &&
            strcmp(capability_alias, BINDINGS[i].capability_alias) == 0) {
            return (int)i;
        }
    }

    return -1;
}

void panel_homey_dashboard_state_init(panel_homey_dashboard_state_t *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    clear_widgets(state, PANEL_WIDGET_UNCONFIGURED);
}

const char *panel_homey_dashboard_device_alias(size_t widget_index)
{
    if (widget_index >= PANEL_HOMEY_DASHBOARD_BINDING_COUNT) {
        return NULL;
    }

    return BINDINGS[widget_index].device_alias;
}

const char *panel_homey_dashboard_capability_alias(size_t widget_index)
{
    if (widget_index >= PANEL_HOMEY_DASHBOARD_BINDING_COUNT) {
        return NULL;
    }

    return BINDINGS[widget_index].capability_alias;
}

panel_homey_dashboard_apply_result_t panel_homey_dashboard_apply_snapshot(
    panel_homey_read_result_t snapshot_result,
    const panel_homey_read_snapshot_t *snapshot,
    uint64_t now_ms,
    panel_homey_dashboard_state_t *state)
{
    if (state == NULL) {
        return PANEL_HOMEY_DASHBOARD_APPLY_INVALID;
    }

    panel_homey_dashboard_state_t candidate;
    panel_homey_dashboard_state_init(&candidate);

    if (snapshot_result == PANEL_HOMEY_READ_NOT_CONFIGURED) {
        clear_widgets(&candidate, PANEL_WIDGET_UNCONFIGURED);
    } else if (snapshot_result == PANEL_HOMEY_READ_NOT_FOUND) {
        clear_widgets(&candidate, PANEL_WIDGET_UNKNOWN);
    } else if (snapshot_result == PANEL_HOMEY_READ_STALE) {
        candidate.stale = true;
        clear_widgets(&candidate, PANEL_WIDGET_UNAVAILABLE);
    } else if (snapshot_result == PANEL_HOMEY_READ_INVALID ||
               snapshot_result == PANEL_HOMEY_READ_OVERFLOW ||
               snapshot_result == PANEL_HOMEY_READ_DUPLICATE) {
        clear_widgets(&candidate, PANEL_WIDGET_UNKNOWN);
    } else if (snapshot_result != PANEL_HOMEY_READ_OK || snapshot == NULL ||
               snapshot->item_count > PANEL_HOMEY_SNAPSHOT_MAX_ITEMS) {
        return PANEL_HOMEY_DASHBOARD_APPLY_INVALID;
    } else {
        candidate.generation = snapshot->generation;
        candidate.generation_valid = true;

        if (now_ms < snapshot->captured_at_ms ||
            now_ms - snapshot->captured_at_ms > PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS) {
            candidate.stale = true;
            clear_widgets(&candidate, PANEL_WIDGET_UNAVAILABLE);
        } else {
            bool matched[PANEL_HOMEY_DASHBOARD_BINDING_COUNT] = {false};

            clear_widgets(&candidate, PANEL_WIDGET_UNKNOWN);

            for (size_t item_index = 0U; item_index < snapshot->item_count; ++item_index) {
                const panel_homey_read_item_t *item = &snapshot->items[item_index];

                if (!alias_valid(item->device_alias, sizeof(item->device_alias)) ||
                    !alias_valid(item->capability_alias, sizeof(item->capability_alias))) {
                    return PANEL_HOMEY_DASHBOARD_APPLY_INVALID;
                }

                int index = binding_index(item->device_alias, item->capability_alias);
                if (index < 0) {
                    continue;
                }

                const size_t widget_index = (size_t)index;
                if (matched[widget_index]) {
                    clear_widgets(&candidate, PANEL_WIDGET_UNKNOWN);
                    return state_equal(state, &candidate)
                        ? PANEL_HOMEY_DASHBOARD_APPLY_UNCHANGED
                        : ((*state = candidate), PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
                }

                matched[widget_index] = true;
                candidate.widgets[widget_index].status =
                    item->available ? PANEL_WIDGET_AVAILABLE : PANEL_WIDGET_UNAVAILABLE;

                if (item->available &&
                    BINDINGS[widget_index].boolean_allowed &&
                    item->value_type == PANEL_HOMEY_VALUE_BOOL) {
                    candidate.widgets[widget_index].has_boolean = true;
                    candidate.widgets[widget_index].boolean_value = item->bool_value;
                }
            }
        }
    }

    if (state_equal(state, &candidate)) {
        return PANEL_HOMEY_DASHBOARD_APPLY_UNCHANGED;
    }

    *state = candidate;
    return PANEL_HOMEY_DASHBOARD_APPLY_UPDATED;
}
