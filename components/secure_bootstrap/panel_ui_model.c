#include "panel_ui_model.h"
#include "panel_homey_dashboard_binding.h"

#include <stdio.h>
#include <string.h>

static const char *const PAGE_TITLES[PANEL_UI_PAGE_COUNT] = {
    "Favoriter",
    "Sida 2",
    "Sida 3",
};

static const char *const WIDGET_TITLES[PANEL_UI_WIDGET_COUNT] = {
    "Markis 1",
    "Markis 2",
    "Markis 3",
    "Verisure",
    "Belysning 1",
    "Belysning 2",
};

static const char *const WEEKDAYS_SV[7] = {
    "sön", "mån", "tis", "ons", "tors", "fre", "lör",
};

static const char *const MONTHS_SV[12] = {
    "januari", "februari", "mars", "april", "maj", "juni",
    "juli", "augusti", "september", "oktober", "november", "december",
};

static uint8_t nearest_u8_lower_tie(uint8_t value, const uint8_t *allowed, size_t count)
{
    uint8_t best = allowed[0];
    unsigned best_distance = value > best ? (unsigned)(value - best) : (unsigned)(best - value);
    for (size_t index = 1U; index < count; ++index) {
        const uint8_t candidate = allowed[index];
        const unsigned distance = value > candidate ? (unsigned)(value - candidate) : (unsigned)(candidate - value);
        if (distance < best_distance) {
            best = candidate;
            best_distance = distance;
        }
    }
    return best;
}

static uint8_t normalize_dimmed_brightness(uint8_t value, uint8_t normal_brightness)
{
    uint8_t normalized = value < 20U ? 10U : (value < 40U ? 30U : 50U);
    if (normalized <= normal_brightness) {
        return normalized;
    }
    return normal_brightness < 30U ? 10U : (normal_brightness < 50U ? 30U : 50U);
}

static uint32_t nearest_u32_lower_tie(uint32_t value, const uint32_t *allowed, size_t count)
{
    uint32_t best = allowed[0];
    uint64_t best_distance = value > best ? (uint64_t)value - best : (uint64_t)best - value;
    for (size_t index = 1U; index < count; ++index) {
        const uint32_t candidate = allowed[index];
        const uint64_t distance = value > candidate ? (uint64_t)value - candidate : (uint64_t)candidate - value;
        if (distance < best_distance) {
            best = candidate;
            best_distance = distance;
        }
    }
    return best;
}

static bool valid_view(panel_ui_view_t view)
{
    return view == PANEL_UI_VIEW_DASHBOARD ||
           view == PANEL_UI_VIEW_SETTINGS ||
           view == PANEL_UI_VIEW_CONFIRMATION;
}

void panel_ui_settings_defaults(panel_ui_settings_t *settings)
{
    if (settings == NULL) {
        return;
    }
    memset(settings, 0, sizeof(*settings));
    settings->normal_brightness = PANEL_UI_DEFAULT_NORMAL_BRIGHTNESS;
    settings->dimmed_brightness = PANEL_UI_DEFAULT_DIMMED_BRIGHTNESS;
    settings->dim_after_seconds = PANEL_UI_DEFAULT_DIM_AFTER_SECONDS;
    settings->off_after_seconds = PANEL_UI_OFF_DISABLED;
    settings->wake_on_touch = PANEL_UI_DEFAULT_WAKE_ON_TOUCH;
    settings->background_mode = PANEL_BACKGROUND_OFF;
    (void)snprintf(settings->timezone_id, sizeof(settings->timezone_id), "%s",
                   PANEL_UI_TIMEZONE_EUROPE_STOCKHOLM);
}

void panel_ui_settings_normalize(panel_ui_settings_t *settings)
{
    static const uint8_t normal_levels[] = {20U, 40U, 60U, 80U, 100U};
    static const uint32_t dim_timeouts[] = {10U, 30U, 60U};
    static const uint32_t off_timeouts[] = {60U, 300U, 1200U};
    if (settings == NULL) return;

    settings->normal_brightness = settings->normal_brightness == 0U
        ? PANEL_UI_DEFAULT_NORMAL_BRIGHTNESS
        : nearest_u8_lower_tie(
            settings->normal_brightness, normal_levels, sizeof(normal_levels) / sizeof(normal_levels[0]));
    settings->dimmed_brightness = normalize_dimmed_brightness(
        settings->dimmed_brightness, settings->normal_brightness);

    settings->dim_after_seconds = nearest_u32_lower_tie(
        settings->dim_after_seconds, dim_timeouts, sizeof(dim_timeouts) / sizeof(dim_timeouts[0]));
    if (settings->off_after_seconds != PANEL_UI_OFF_DISABLED) {
        settings->off_after_seconds = nearest_u32_lower_tie(
            settings->off_after_seconds, off_timeouts, sizeof(off_timeouts) / sizeof(off_timeouts[0]));
        if (settings->off_after_seconds <= settings->dim_after_seconds) {
            size_t index = 0U;
            while (index < sizeof(off_timeouts) / sizeof(off_timeouts[0]) &&
                   off_timeouts[index] <= settings->dim_after_seconds) index++;
            settings->off_after_seconds = index < sizeof(off_timeouts) / sizeof(off_timeouts[0])
                ? off_timeouts[index] : PANEL_UI_OFF_DISABLED;
        }
    }
    settings->wake_on_touch = PANEL_UI_DEFAULT_WAKE_ON_TOUCH;
    if (settings->background_mode != PANEL_BACKGROUND_OFF &&
        settings->background_mode != PANEL_BACKGROUND_BUILT_IN)
        settings->background_mode = PANEL_BACKGROUND_OFF;
    if (strcmp(settings->timezone_id, PANEL_UI_TIMEZONE_EUROPE_STOCKHOLM) != 0)
        (void)snprintf(settings->timezone_id, sizeof(settings->timezone_id), "%s",
                       PANEL_UI_TIMEZONE_EUROPE_STOCKHOLM);
}

void panel_ui_model_init(panel_ui_model_t *model, uint64_t now_ms)
{
    if (model == NULL) {
        return;
    }
    memset(model, 0, sizeof(*model));
    panel_ui_settings_defaults(&model->settings);
    model->view = PANEL_UI_VIEW_DASHBOARD;
    model->power_state = PANEL_POWER_ACTIVE;
    model->confirmation = PANEL_CONFIRM_NONE;
    model->last_activity_ms = now_ms;
    for (size_t index = 0; index < PANEL_UI_WIDGET_COUNT; ++index) {
        model->widget_status[index] = PANEL_WIDGET_UNCONFIGURED;
    }
}

uint8_t panel_ui_set_active_page(panel_ui_model_t *model, int page_index)
{
    int normalized = page_index;
    if (normalized < 0) {
        normalized = 0;
    } else if (normalized >= (int)PANEL_UI_PAGE_COUNT) {
        normalized = (int)PANEL_UI_PAGE_COUNT - 1;
    }
    if (model != NULL) {
        model->active_page = (uint8_t)normalized;
    }
    return (uint8_t)normalized;
}

void panel_ui_set_view(panel_ui_model_t *model, panel_ui_view_t view)
{
    if (model == NULL) {
        return;
    }
    model->view = valid_view(view) ? view : PANEL_UI_VIEW_DASHBOARD;
    if (model->view != PANEL_UI_VIEW_CONFIRMATION) {
        model->confirmation = PANEL_CONFIRM_NONE;
    }
}

void panel_ui_register_activity(panel_ui_model_t *model, uint64_t now_ms)
{
    if (model == NULL) {
        return;
    }
    model->last_activity_ms = now_ms;
    model->power_state = PANEL_POWER_ACTIVE;
}

void panel_ui_tick(panel_ui_model_t *model, uint64_t now_ms)
{
    if (model == NULL || now_ms < model->last_activity_ms) {
        return;
    }
    const uint64_t idle_ms = now_ms - model->last_activity_ms;
    const uint64_t dim_ms = (uint64_t)model->settings.dim_after_seconds * 1000ULL;
    const uint64_t off_ms = (uint64_t)model->settings.off_after_seconds * 1000ULL;

    if (model->settings.off_after_seconds != PANEL_UI_OFF_DISABLED && idle_ms >= off_ms) {
        model->power_state = PANEL_POWER_OFF;
    } else if (model->settings.dim_after_seconds != 0U && idle_ms >= dim_ms) {
        model->power_state = PANEL_POWER_DIMMED;
    } else {
        model->power_state = PANEL_POWER_ACTIVE;
    }
}

bool panel_ui_handle_touch(panel_ui_model_t *model, uint64_t now_ms)
{
    if (model == NULL) {
        return false;
    }
    if (model->power_state == PANEL_POWER_ACTIVE) {
        panel_ui_register_activity(model, now_ms);
        return false;
    }
    panel_ui_register_activity(model, now_ms);
    return true;
}

void panel_ui_request_confirmation(panel_ui_model_t *model, panel_confirmation_t confirmation)
{
    if (model == NULL) {
        return;
    }
    if (confirmation != PANEL_CONFIRM_HOMEY_WIPE &&
        confirmation != PANEL_CONFIRM_ATHOM_ACCOUNT_CHANGE) {
        model->confirmation = PANEL_CONFIRM_NONE;
        model->view = PANEL_UI_VIEW_DASHBOARD;
        return;
    }
    model->confirmation = confirmation;
    model->view = PANEL_UI_VIEW_CONFIRMATION;
}

void panel_ui_cancel_confirmation(panel_ui_model_t *model)
{
    if (model == NULL) {
        return;
    }
    model->confirmation = PANEL_CONFIRM_NONE;
    model->view = PANEL_UI_VIEW_SETTINGS;
}

panel_confirmed_action_t panel_ui_accept_confirmation(panel_ui_model_t *model)
{
    if (model == NULL) {
        return PANEL_CONFIRMED_ACTION_NONE;
    }
    panel_confirmed_action_t action = PANEL_CONFIRMED_ACTION_NONE;
    if (model->confirmation == PANEL_CONFIRM_HOMEY_WIPE) {
        action = PANEL_CONFIRMED_ACTION_HOMEY_WIPE;
    } else if (model->confirmation == PANEL_CONFIRM_ATHOM_ACCOUNT_CHANGE) {
        action = PANEL_CONFIRMED_ACTION_ATHOM_ACCOUNT_CHANGE;
    }
    model->confirmation = PANEL_CONFIRM_NONE;
    model->view = PANEL_UI_VIEW_SETTINGS;
    return action;
}

const char *panel_ui_page_title(size_t page_index)
{
    return page_index < PANEL_UI_PAGE_COUNT ? PAGE_TITLES[page_index] : "";
}

const char *panel_ui_widget_title(size_t widget_index)
{
    return widget_index < PANEL_UI_WIDGET_COUNT ? WIDGET_TITLES[widget_index] : "";
}

const char *panel_ui_widget_status_text(panel_widget_status_t status)
{
    switch (status) {
    case PANEL_WIDGET_UNCONFIGURED:
        return "Ej konfigurerad";
    case PANEL_WIDGET_UNKNOWN:
        return "Okänd";
    case PANEL_WIDGET_AVAILABLE:
        return "Tillgänglig";
    case PANEL_WIDGET_UNAVAILABLE:
        return "Otillgänglig";
    default:
        return "Okänd";
    }
}

bool panel_ui_apply_homey_dashboard_state(
    panel_ui_model_t *model,
    const struct panel_homey_dashboard_state *state)
{
    if (model == NULL || state == NULL) {
        return false;
    }

    bool changed =
        model->homey_generation != state->generation ||
        model->homey_generation_valid != state->generation_valid ||
        model->homey_snapshot_stale != state->stale;

    for (size_t index = 0U; index < PANEL_UI_WIDGET_COUNT; ++index) {
        if (model->widget_status[index] != state->widgets[index].status ||
            model->widget_has_boolean[index] != state->widgets[index].has_boolean ||
            model->widget_boolean_value[index] != state->widgets[index].boolean_value) {
            changed = true;
        }
        model->widget_status[index] = state->widgets[index].status;
        model->widget_has_boolean[index] = state->widgets[index].has_boolean;
        model->widget_boolean_value[index] = state->widgets[index].boolean_value;
    }

    model->homey_generation = state->generation;
    model->homey_generation_valid = state->generation_valid;
    model->homey_snapshot_stale = state->stale;
    return changed;
}

bool panel_ui_widget_display_text(
    const panel_ui_model_t *model,
    size_t widget_index,
    char *buffer,
    size_t buffer_size)
{
    if (model == NULL || widget_index >= PANEL_UI_WIDGET_COUNT ||
        buffer == NULL || buffer_size == 0U) {
        return false;
    }

    const char *text = panel_ui_widget_status_text(model->widget_status[widget_index]);
    if (model->widget_status[widget_index] == PANEL_WIDGET_AVAILABLE &&
        model->widget_has_boolean[widget_index]) {
        text = model->widget_boolean_value[widget_index] ? "Aktiv" : "Inaktiv";
    }

    int written = snprintf(buffer, buffer_size, "%s", text);
    return written >= 0 && (size_t)written < buffer_size;
}

bool panel_ui_format_clock(bool synchronized, const struct tm *local_time,
                           char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0U) {
        return false;
    }
    int written;
    if (!synchronized || local_time == NULL) {
        written = snprintf(buffer, buffer_size, "--:--");
    } else {
        written = snprintf(buffer, buffer_size, "%02d:%02d",
                           local_time->tm_hour, local_time->tm_min);
    }
    return written >= 0 && (size_t)written < buffer_size;
}

bool panel_ui_format_date_sv(bool synchronized, const struct tm *local_time,
                             char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0U) {
        return false;
    }
    int written;
    if (!synchronized || local_time == NULL ||
        local_time->tm_wday < 0 || local_time->tm_wday > 6 ||
        local_time->tm_mon < 0 || local_time->tm_mon > 11) {
        written = snprintf(buffer, buffer_size, "Tid ej synkroniserad");
    } else {
        written = snprintf(buffer, buffer_size, "%s %d %s",
                           WEEKDAYS_SV[local_time->tm_wday],
                           local_time->tm_mday,
                           MONTHS_SV[local_time->tm_mon]);
    }
    return written >= 0 && (size_t)written < buffer_size;
}
