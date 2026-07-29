#include "panel_ui_model.h"

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

static uint8_t clamp_brightness(unsigned value)
{
    return value > 100U ? 100U : (uint8_t)value;
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
    settings->wake_on_touch = true;
    settings->background_mode = PANEL_BACKGROUND_OFF;
    (void)snprintf(settings->timezone_id, sizeof(settings->timezone_id), "%s",
                   PANEL_UI_TIMEZONE_EUROPE_STOCKHOLM);
}

void panel_ui_settings_normalize(panel_ui_settings_t *settings)
{
    if (settings == NULL) {
        return;
    }
    settings->normal_brightness = clamp_brightness(settings->normal_brightness);
    settings->dimmed_brightness = clamp_brightness(settings->dimmed_brightness);
    if (settings->normal_brightness == 0U) {
        settings->normal_brightness = PANEL_UI_DEFAULT_NORMAL_BRIGHTNESS;
    }
    if (settings->dimmed_brightness > settings->normal_brightness) {
        settings->dimmed_brightness = settings->normal_brightness;
    }
    if (settings->dim_after_seconds != 0U) {
        if (settings->dim_after_seconds < PANEL_UI_TIMEOUT_MIN_SECONDS) {
            settings->dim_after_seconds = PANEL_UI_TIMEOUT_MIN_SECONDS;
        } else if (settings->dim_after_seconds > PANEL_UI_TIMEOUT_MAX_SECONDS) {
            settings->dim_after_seconds = PANEL_UI_TIMEOUT_MAX_SECONDS;
        }
    }
    if (settings->off_after_seconds != PANEL_UI_OFF_DISABLED) {
        if (settings->off_after_seconds < PANEL_UI_TIMEOUT_MIN_SECONDS) {
            settings->off_after_seconds = PANEL_UI_TIMEOUT_MIN_SECONDS;
        } else if (settings->off_after_seconds > PANEL_UI_TIMEOUT_MAX_SECONDS) {
            settings->off_after_seconds = PANEL_UI_TIMEOUT_MAX_SECONDS;
        }
        if (settings->dim_after_seconds != 0U &&
            settings->off_after_seconds <= settings->dim_after_seconds) {
            if (settings->dim_after_seconds >
                PANEL_UI_TIMEOUT_MAX_SECONDS - PANEL_UI_TIMEOUT_MIN_SECONDS) {
                settings->off_after_seconds = PANEL_UI_OFF_DISABLED;
            } else {
                settings->off_after_seconds =
                    settings->dim_after_seconds + PANEL_UI_TIMEOUT_MIN_SECONDS;
            }
        }
    }
    if (settings->background_mode != PANEL_BACKGROUND_OFF &&
        settings->background_mode != PANEL_BACKGROUND_BUILT_IN) {
        settings->background_mode = PANEL_BACKGROUND_OFF;
    }
    if (strcmp(settings->timezone_id, PANEL_UI_TIMEZONE_EUROPE_STOCKHOLM) != 0) {
        (void)snprintf(settings->timezone_id, sizeof(settings->timezone_id), "%s",
                       PANEL_UI_TIMEZONE_EUROPE_STOCKHOLM);
    }
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
    if (model->settings.wake_on_touch) {
        panel_ui_register_activity(model, now_ms);
    }
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
