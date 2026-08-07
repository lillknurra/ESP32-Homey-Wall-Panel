#include "panel_ui_model.h"
#include "panel_homey_dashboard_binding.h"
#include "panel_ui_store.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void test_defaults_and_widgets(void)
{
    panel_ui_model_t model;
    panel_ui_model_init(&model, 1000U);
    assert(PANEL_UI_PAGE_COUNT == 3U);
    assert(PANEL_UI_WIDGET_COUNT == 6U);
    assert(model.active_page == 0U);
    assert(model.view == PANEL_UI_VIEW_DASHBOARD);
    assert(model.settings.wake_on_touch);
    assert(strcmp(panel_ui_page_title(0), "Favoriter") == 0);
    assert(strcmp(panel_ui_page_title(1), "Sida 2") == 0);
    assert(strcmp(panel_ui_page_title(2), "Sida 3") == 0);
    assert(strcmp(panel_ui_widget_title(0), "Markis 1") == 0);
    assert(strcmp(panel_ui_widget_title(1), "Markis 2") == 0);
    assert(strcmp(panel_ui_widget_title(2), "Markis 3") == 0);
    assert(strcmp(panel_ui_widget_title(3), "Verisure") == 0);
    assert(strcmp(panel_ui_widget_title(4), "Belysning 1") == 0);
    assert(strcmp(panel_ui_widget_title(5), "Belysning 2") == 0);
    assert(strcmp(panel_ui_widget_title(6), "") == 0);
    for (size_t i = 0; i < PANEL_UI_WIDGET_COUNT; ++i) {
        assert(model.widget_status[i] == PANEL_WIDGET_UNCONFIGURED);
    }
    assert(strcmp(panel_ui_widget_status_text(PANEL_WIDGET_UNCONFIGURED), "Ej konfigurerad") == 0);
    assert(strcmp(panel_ui_widget_status_text(PANEL_WIDGET_UNKNOWN), "Okänd") == 0);
    assert(strcmp(panel_ui_widget_status_text(PANEL_WIDGET_AVAILABLE), "Tillgänglig") == 0);
    assert(strcmp(panel_ui_widget_status_text(PANEL_WIDGET_UNAVAILABLE), "Otillgänglig") == 0);
}

static void test_page_and_view_bounds(void)
{
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);
    assert(panel_ui_set_active_page(&model, -9) == 0U);
    assert(panel_ui_set_active_page(&model, 1) == 1U);
    assert(panel_ui_set_active_page(&model, 99) == 2U);
    panel_ui_set_view(&model, PANEL_UI_VIEW_SETTINGS);
    assert(model.view == PANEL_UI_VIEW_SETTINGS);
    panel_ui_set_view(&model, (panel_ui_view_t)99);
    assert(model.view == PANEL_UI_VIEW_DASHBOARD);
}

static void test_settings_normalization(void)
{
    panel_ui_settings_t settings;
    panel_ui_settings_defaults(&settings);
    assert(settings.normal_brightness == 80U);
    assert(settings.dimmed_brightness == 30U);
    assert(settings.dim_after_seconds == 60U);
    assert(settings.off_after_seconds == PANEL_UI_OFF_DISABLED);
    assert(settings.wake_on_touch);

    static const struct { uint8_t input; uint8_t expected; } normal_cases[] = {
        {0U,80U},{20U,20U},{30U,20U},{31U,40U},{50U,40U},{51U,60U},
        {70U,60U},{71U,80U},{85U,80U},{90U,80U},{91U,100U},{100U,100U},{255U,100U},
    };
    for (size_t i = 0U; i < sizeof(normal_cases) / sizeof(normal_cases[0]); ++i) {
        panel_ui_settings_defaults(&settings);
        settings.normal_brightness = normal_cases[i].input;
        panel_ui_settings_normalize(&settings);
        assert(settings.normal_brightness == normal_cases[i].expected);
    }

    static const struct { uint8_t input; uint8_t expected; } dim_cases[] = {
        {0U,10U},{9U,10U},{10U,10U},{19U,10U},{20U,30U},{29U,30U},
        {30U,30U},{39U,30U},{40U,50U},{49U,50U},{50U,50U},{100U,50U},{255U,50U},
    };
    for (size_t i = 0U; i < sizeof(dim_cases) / sizeof(dim_cases[0]); ++i) {
        panel_ui_settings_defaults(&settings);
        settings.normal_brightness = 100U;
        settings.dimmed_brightness = dim_cases[i].input;
        panel_ui_settings_normalize(&settings);
        assert(settings.dimmed_brightness == dim_cases[i].expected);
    }

    panel_ui_settings_defaults(&settings);
    settings.normal_brightness = 20U;
    settings.dimmed_brightness = 40U;
    panel_ui_settings_normalize(&settings);
    assert(settings.normal_brightness == 20U);
    assert(settings.dimmed_brightness == 10U);

    panel_ui_settings_defaults(&settings);
    settings.normal_brightness = 0U;
    settings.dimmed_brightness = 100U;
    panel_ui_settings_normalize(&settings);
    assert(settings.normal_brightness == 80U);
    assert(settings.dimmed_brightness == 50U);

    settings.wake_on_touch = false;
    panel_ui_settings_normalize(&settings);
    assert(settings.wake_on_touch);
}

static void test_power_and_touch(void)
{
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);
    model.settings.dim_after_seconds = 2U;
    model.settings.off_after_seconds = 5U;
    panel_ui_tick(&model, 1999U);
    assert(model.power_state == PANEL_POWER_ACTIVE);
    panel_ui_tick(&model, 2000U);
    assert(model.power_state == PANEL_POWER_DIMMED);
    assert(panel_ui_handle_touch(&model, 2500U));
    assert(model.power_state == PANEL_POWER_ACTIVE);
    assert(model.last_activity_ms == 2500U);
    panel_ui_tick(&model, 7500U);
    assert(model.power_state == PANEL_POWER_OFF);
    assert(panel_ui_handle_touch(&model, 8000U));
    assert(model.power_state == PANEL_POWER_ACTIVE);
    assert(model.last_activity_ms == 8000U);
    assert(!panel_ui_handle_touch(&model, 8100U));
    assert(model.last_activity_ms == 8100U);

    model.settings.wake_on_touch = false;
    panel_ui_settings_normalize(&model.settings);
    assert(model.settings.wake_on_touch);

    model.power_state = PANEL_POWER_DIMMED;
    model.last_activity_ms = 9000U;
    assert(panel_ui_handle_touch(&model, 10000U));
    assert(model.power_state == PANEL_POWER_ACTIVE);
    assert(model.last_activity_ms == 10000U);
    assert(!panel_ui_handle_touch(&model, 10100U));
    assert(model.last_activity_ms == 10100U);

    model.settings.wake_on_touch = false;
    model.power_state = PANEL_POWER_OFF;
    assert(panel_ui_handle_touch(&model, 11000U));
    assert(model.power_state == PANEL_POWER_ACTIVE);
    assert(model.last_activity_ms == 11000U);
    assert(!panel_ui_handle_touch(&model, 11100U));
    assert(model.last_activity_ms == 11100U);
}

static void test_time_formatting(void)
{
    char clock_text[PANEL_UI_TIME_TEXT_MAX];
    char date_text[PANEL_UI_DATE_TEXT_MAX];
    char exact_clock[6];
    char too_small_clock[5];
    char exact_unknown_date[22];
    char too_small_date[5];

    assert(!panel_ui_format_clock(false, NULL, NULL, sizeof(clock_text)));
    assert(!panel_ui_format_clock(false, NULL, clock_text, 0U));
    assert(!panel_ui_format_date_sv(false, NULL, NULL, sizeof(date_text)));
    assert(!panel_ui_format_date_sv(false, NULL, date_text, 0U));

    assert(panel_ui_format_clock(false, NULL, exact_clock, sizeof(exact_clock)));
    assert(strcmp(exact_clock, "--:--") == 0);
    assert(!panel_ui_format_clock(false, NULL, too_small_clock, sizeof(too_small_clock)));
    assert(too_small_clock[sizeof(too_small_clock) - 1U] == '\0');

    assert(panel_ui_format_date_sv(false, NULL, exact_unknown_date, sizeof(exact_unknown_date)));
    assert(strcmp(exact_unknown_date, "Tid ej synkroniserad") == 0);
    assert(!panel_ui_format_date_sv(false, NULL, too_small_date, sizeof(too_small_date)));
    assert(too_small_date[sizeof(too_small_date) - 1U] == '\0');

    assert(panel_ui_format_clock(false, NULL, clock_text, sizeof(clock_text)));
    assert(strcmp(clock_text, "--:--") == 0);
    assert(panel_ui_format_date_sv(false, NULL, date_text, sizeof(date_text)));
    assert(strcmp(date_text, "Tid ej synkroniserad") == 0);

    struct tm local_time = {0};
    local_time.tm_hour = 10;
    local_time.tm_min = 47;
    local_time.tm_wday = 3;
    local_time.tm_mday = 29;
    local_time.tm_mon = 6;
    assert(panel_ui_format_clock(true, &local_time, exact_clock, sizeof(exact_clock)));
    assert(strcmp(exact_clock, "10:47") == 0);
    assert(!panel_ui_format_clock(true, &local_time, too_small_clock, sizeof(too_small_clock)));
    assert(too_small_clock[sizeof(too_small_clock) - 1U] == '\0');
    assert(panel_ui_format_date_sv(true, &local_time, date_text, sizeof(date_text)));
    assert(strcmp(date_text, "ons 29 juli") == 0);
    assert(!panel_ui_format_date_sv(true, &local_time, too_small_date, sizeof(too_small_date)));
    assert(too_small_date[sizeof(too_small_date) - 1U] == '\0');
}

static void test_confirmations(void)
{
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);
    panel_ui_request_confirmation(&model, PANEL_CONFIRM_HOMEY_WIPE);
    assert(model.view == PANEL_UI_VIEW_CONFIRMATION);
    assert(panel_ui_accept_confirmation(&model) == PANEL_CONFIRMED_ACTION_HOMEY_WIPE);
    assert(model.view == PANEL_UI_VIEW_SETTINGS);
    assert(model.confirmation == PANEL_CONFIRM_NONE);

    panel_ui_request_confirmation(&model, PANEL_CONFIRM_ATHOM_ACCOUNT_CHANGE);
    panel_ui_cancel_confirmation(&model);
    assert(model.view == PANEL_UI_VIEW_SETTINGS);
    assert(panel_ui_accept_confirmation(&model) == PANEL_CONFIRMED_ACTION_NONE);
}

static void test_null_safety(void)
{
    panel_ui_settings_defaults(NULL);
    panel_ui_settings_normalize(NULL);
    panel_ui_model_init(NULL, 0U);
    panel_ui_set_view(NULL, PANEL_UI_VIEW_SETTINGS);
    panel_ui_register_activity(NULL, 0U);
    panel_ui_tick(NULL, 0U);
    assert(!panel_ui_handle_touch(NULL, 0U));
    panel_ui_request_confirmation(NULL, PANEL_CONFIRM_HOMEY_WIPE);
    panel_ui_cancel_confirmation(NULL);
    assert(panel_ui_accept_confirmation(NULL) == PANEL_CONFIRMED_ACTION_NONE);
}



static void store_rewrite_crc(uint8_t record[PANEL_UI_STORE_RECORD_SIZE])
{
    uint32_t crc = 0U;
    assert(panel_ui_store_crc32(record, 28U, &crc));
    record[28] = (uint8_t)(crc & 0xffU);
    record[29] = (uint8_t)((crc >> 8) & 0xffU);
    record[30] = (uint8_t)((crc >> 16) & 0xffU);
    record[31] = (uint8_t)((crc >> 24) & 0xffU);
}

static void test_store_crc(void)
{
    static const uint8_t vector[] = "123456789";
    uint32_t crc = 0U;
    assert(panel_ui_store_crc32(vector, sizeof(vector) - 1U, &crc));
    assert(crc == UINT32_C(0xcbf43926));
    assert(panel_ui_store_crc32(NULL, 0U, &crc));
    assert(crc == 0U);
    assert(!panel_ui_store_crc32(NULL, 1U, &crc));
    assert(!panel_ui_store_crc32(vector, sizeof(vector), NULL));
}

static void test_store_roundtrip(void)
{
    panel_ui_settings_t settings;
    panel_ui_settings_defaults(&settings);
    panel_ui_settings_t original = settings;
    uint8_t record[PANEL_UI_STORE_RECORD_SIZE];
    assert(panel_ui_store_encode(&settings, 7U, record, sizeof(record)));
    assert(memcmp(&settings, &original, sizeof(settings)) == 0);

    panel_ui_settings_t decoded;
    uint32_t generation = 0U;
    assert(panel_ui_store_decode(record, sizeof(record), &decoded, &generation));
    assert(generation == 7U);
    assert(decoded.normal_brightness == settings.normal_brightness);
    assert(decoded.dimmed_brightness == settings.dimmed_brightness);
    assert(decoded.dim_after_seconds == settings.dim_after_seconds);
    assert(decoded.off_after_seconds == settings.off_after_seconds);
    assert(decoded.wake_on_touch == settings.wake_on_touch);
    assert(decoded.background_mode == settings.background_mode);
    assert(strcmp(decoded.timezone_id, PANEL_UI_TIMEZONE_EUROPE_STOCKHOLM) == 0);

    record[13] = 20U;
    store_rewrite_crc(record);
    assert(panel_ui_store_decode(record, sizeof(record), &decoded, &generation));
    assert(decoded.dimmed_brightness == 30U);
    assert(decoded.wake_on_touch);

    settings.normal_brightness = 85U;
    settings.dimmed_brightness = 17U;
    settings.dim_after_seconds = 120U;
    settings.off_after_seconds = 100U;
    assert(panel_ui_store_encode(&settings, 8U, record, sizeof(record)));
    assert(panel_ui_store_decode(record, sizeof(record), &decoded, &generation));
    assert(decoded.normal_brightness == 80U);
    assert(decoded.dimmed_brightness == 10U);
    assert(decoded.dim_after_seconds == 60U);
    assert(decoded.off_after_seconds == 300U);

    panel_ui_settings_defaults(&settings);
    assert(panel_ui_store_encode(&settings, 9U, record, sizeof(record)));
    record[14] &= (uint8_t)~UINT8_C(0x01);
    store_rewrite_crc(record);
    assert(panel_ui_store_decode(record, sizeof(record), &decoded, &generation));
    assert(generation == 9U);
    assert(decoded.wake_on_touch);

    assert(!panel_ui_store_encode(NULL, 1U, record, sizeof(record)));
    assert(!panel_ui_store_encode(&settings, 0U, record, sizeof(record)));
    assert(!panel_ui_store_encode(&settings, 1U, NULL, sizeof(record)));
    assert(!panel_ui_store_encode(&settings, 1U, record, sizeof(record) - 1U));
    assert(!panel_ui_store_encode(&settings, 1U, record, sizeof(record) + 1U));
    assert(!panel_ui_store_decode(NULL, sizeof(record), &decoded, &generation));
    assert(!panel_ui_store_decode(record, sizeof(record), NULL, &generation));
    assert(!panel_ui_store_decode(record, sizeof(record), &decoded, NULL));
    assert(!panel_ui_store_decode(record, sizeof(record) - 1U, &decoded, &generation));
    assert(!panel_ui_store_decode(record, sizeof(record) + 1U, &decoded, &generation));
}

static void test_store_record_validation(void)
{
    panel_ui_settings_t settings;
    panel_ui_settings_defaults(&settings);
    uint8_t valid[PANEL_UI_STORE_RECORD_SIZE];
    assert(panel_ui_store_encode(&settings, 11U, valid, sizeof(valid)));

    panel_ui_settings_t sentinel;
    memset(&sentinel, 0x5a, sizeof(sentinel));
    panel_ui_settings_t output = sentinel;
    uint32_t generation = UINT32_C(0x5a5a5a5a);
    uint8_t record[PANEL_UI_STORE_RECORD_SIZE];

#define EXPECT_INVALID(MUTATION) do { \
    memcpy(record, valid, sizeof(record)); \
    MUTATION; \
    output = sentinel; \
    generation = UINT32_C(0x5a5a5a5a); \
    assert(!panel_ui_store_decode(record, sizeof(record), &output, &generation)); \
    assert(memcmp(&output, &sentinel, sizeof(output)) == 0); \
    assert(generation == UINT32_C(0x5a5a5a5a)); \
} while (0)

    EXPECT_INVALID(record[0] ^= 0x01U);
    EXPECT_INVALID(record[4] = 2U; store_rewrite_crc(record));
    EXPECT_INVALID(record[6] = 31U; store_rewrite_crc(record));
    EXPECT_INVALID(record[8] = 0U; record[9] = 0U; record[10] = 0U; record[11] = 0U; store_rewrite_crc(record));
    EXPECT_INVALID(record[14] |= 0x80U; store_rewrite_crc(record));
    EXPECT_INVALID(record[15] = 99U; store_rewrite_crc(record));
    EXPECT_INVALID(record[24] = 1U; store_rewrite_crc(record));
    EXPECT_INVALID(record[25] = 1U; store_rewrite_crc(record));
    EXPECT_INVALID(record[26] = 1U; store_rewrite_crc(record));
    EXPECT_INVALID(record[27] = 1U; store_rewrite_crc(record));

#undef EXPECT_INVALID
}

static panel_ui_store_candidate_t candidate(panel_ui_store_slot_t slot, bool valid, uint32_t generation)
{
    panel_ui_store_candidate_t value;
    memset(&value, 0, sizeof(value));
    value.slot = slot;
    value.valid = valid;
    value.generation = generation;
    return value;
}

static void test_store_slot_selection(void)
{
    panel_ui_store_candidate_t a = candidate(PANEL_UI_STORE_SLOT_A, false, 0U);
    panel_ui_store_candidate_t b = candidate(PANEL_UI_STORE_SLOT_B, false, 0U);
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_NONE) == PANEL_UI_STORE_SLOT_NONE);

    a = candidate(PANEL_UI_STORE_SLOT_A, true, 1U);
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_NONE) == PANEL_UI_STORE_SLOT_A);
    a.valid = false;
    b = candidate(PANEL_UI_STORE_SLOT_B, true, 2U);
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_NONE) == PANEL_UI_STORE_SLOT_B);

    a = candidate(PANEL_UI_STORE_SLOT_A, true, 10U);
    b = candidate(PANEL_UI_STORE_SLOT_B, true, 11U);
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_A) == PANEL_UI_STORE_SLOT_A);
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_B) == PANEL_UI_STORE_SLOT_B);
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_NONE) == PANEL_UI_STORE_SLOT_B);

    a.generation = 12U;
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_NONE) == PANEL_UI_STORE_SLOT_A);
    b.generation = 12U;
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_NONE) == PANEL_UI_STORE_SLOT_A);

    a.generation = UINT32_MAX;
    b.generation = 1U;
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_NONE) == PANEL_UI_STORE_SLOT_B);

    a.valid = false;
    assert(panel_ui_store_select_slot(&a, &b, PANEL_UI_STORE_SLOT_A) == PANEL_UI_STORE_SLOT_B);
    assert(panel_ui_store_select_slot(NULL, &b, PANEL_UI_STORE_SLOT_NONE) == PANEL_UI_STORE_SLOT_B);
}

static void test_homey_dashboard_model_integration(void)
{
    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);

    panel_homey_dashboard_state_t state;
    panel_homey_dashboard_state_init(&state);
    state.generation = 12U;
    state.generation_valid = true;
    state.widgets[4].status = PANEL_WIDGET_AVAILABLE;
    state.widgets[4].has_boolean = true;
    state.widgets[4].boolean_value = true;

    assert(panel_ui_apply_homey_dashboard_state(&model, &state));
    assert(model.homey_generation == 12U);
    assert(model.homey_generation_valid);
    assert(!model.homey_snapshot_stale);
    assert(model.widget_status[4] == PANEL_WIDGET_AVAILABLE);
    assert(model.widget_has_boolean[4]);
    assert(model.widget_boolean_value[4]);
    assert(!panel_ui_apply_homey_dashboard_state(&model, &state));

    char text[32];
    assert(panel_ui_widget_display_text(&model, 4U, text, sizeof(text)));
    assert(strcmp(text, "Tänd") == 0);

    state.widgets[4].boolean_value = false;
    assert(panel_ui_apply_homey_dashboard_state(&model, &state));
    assert(panel_ui_widget_display_text(&model, 4U, text, sizeof(text)));
    assert(strcmp(text, "Släckt") == 0);

    state.widgets[4].has_boolean = false;
    assert(panel_ui_apply_homey_dashboard_state(&model, &state));
    assert(panel_ui_widget_display_text(&model, 4U, text, sizeof(text)));
    assert(strcmp(text, "Tillgänglig") == 0);

    state.stale = true;
    state.widgets[4].status = PANEL_WIDGET_UNAVAILABLE;
    assert(panel_ui_apply_homey_dashboard_state(&model, &state));
    assert(model.homey_snapshot_stale);
    assert(panel_ui_widget_display_text(&model, 4U, text, sizeof(text)));
    assert(strcmp(text, "Otillgänglig") == 0);

    assert(!panel_ui_apply_homey_dashboard_state(NULL, &state));
    assert(!panel_ui_apply_homey_dashboard_state(&model, NULL));
    assert(!panel_ui_widget_display_text(NULL, 0U, text, sizeof(text)));
    assert(!panel_ui_widget_display_text(
        &model, PANEL_UI_WIDGET_COUNT, text, sizeof(text)));
}


static void test_light_specific_boolean_text(void)
{
    panel_ui_model_t model;
    char text[32];
    panel_ui_model_init(&model, 0U);
    model.widget_status[3] = PANEL_WIDGET_AVAILABLE;
    model.widget_has_boolean[3] = true;
    model.widget_boolean_value[3] = true;
    assert(panel_ui_widget_display_text(&model, 3U, text, sizeof(text)));
    assert(strcmp(text, "Aktiv") == 0);
    model.widget_status[4] = PANEL_WIDGET_AVAILABLE;
    model.widget_has_boolean[4] = true;
    model.widget_boolean_value[4] = true;
    assert(panel_ui_widget_display_text(&model, 4U, text, sizeof(text)));
    assert(strcmp(text, "Tänd") == 0);
    model.widget_boolean_value[4] = false;
    assert(panel_ui_widget_display_text(&model, 4U, text, sizeof(text)));
    assert(strcmp(text, "Släckt") == 0);
    model.widget_status[4] = PANEL_WIDGET_UNAVAILABLE;
    assert(panel_ui_widget_display_text(&model, 4U, text, sizeof(text)));
    assert(strcmp(text, "Otillgänglig") == 0);
}

int main(void)
{
    test_light_specific_boolean_text();
    test_homey_dashboard_model_integration();
    test_defaults_and_widgets();
    test_page_and_view_bounds();
    test_settings_normalization();
    test_power_and_touch();
    test_time_formatting();
    test_confirmations();
    test_null_safety();
    test_store_crc();
    test_store_roundtrip();
    test_store_record_validation();
    test_store_slot_selection();
    puts("PANEL_UI_MODEL_TESTS PASS");
    return 0;
}
