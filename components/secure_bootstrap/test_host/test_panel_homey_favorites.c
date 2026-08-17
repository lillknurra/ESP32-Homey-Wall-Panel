#include "panel_homey_favorites.h"
#include "panel_homey_dashboard_binding.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void apply(const char *user_json, const char *devices_json, panel_ui_model_t *model)
{
    memset(model, 0, sizeof(*model));
    assert(panel_homey_favorites_parse_and_publish(user_json, devices_json) == PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_apply_ui_model(model));
}

static void test_authoritative_order_differs_from_inventory(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[\"dev-b\",\"dev-a\"]}}";
    const char *devices = "{\"dev-a\":{\"name\":\"A\",\"capabilitiesObj\":{\"onoff\":{\"value\":true}}},\"dev-b\":{\"name\":\"B\",\"capabilitiesObj\":{\"onoff\":{\"value\":false}}}}";
    panel_ui_model_t model;
    apply(user, devices, &model);
    assert(strcmp(model.widget_title[4], "B") == 0);
    assert(strcmp(model.widget_title[5], "A") == 0);
    assert(model.widget_boolean_value[4] == false);
    assert(model.widget_boolean_value[5] == true);
}

static void test_exact_id_join_and_legacy_heuristic_cannot_override(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[\"wanted\"]}}";
    const char *devices = "[{\"id\":\"legacy\",\"name\":\"Legacy\",\"favorite\":true,\"isFavorite\":true,\"settings\":{\"favorite\":true},\"capabilitiesObj\":{\"onoff\":{\"value\":true}}},{\"id\":\"wanted\",\"name\":\"Wanted\",\"capabilitiesObj\":{\"onoff\":{\"value\":false}}}]";
    panel_ui_model_t model;
    apply(user, devices, &model);
    assert(strcmp(model.widget_title[4], "Wanted") == 0);
    assert(strcmp(model.widget_title[5], "Belysning 2") == 0);
}

static void test_first_two_compatible_skip_non_onoff(void)
{
    const char *user = "{\"result\":{\"properties\":{\"favoriteDevices\":[\"cover\",\"light1\",\"alarm\",\"light2\",\"light3\"]}}}";
    const char *devices = "{\"result\":{\"cover\":{\"name\":\"Cover\",\"capabilitiesObj\":{\"windowcoverings_state\":{\"value\":\"up\"}}},\"light1\":{\"name\":\"Light 1\",\"capabilitiesObj\":{\"onoff\":{\"value\":false}}},\"alarm\":{\"name\":\"Alarm\"},\"light2\":{\"name\":\"Light 2\",\"capabilitiesObj\":{\"onoff\":{\"value\":true}}},\"light3\":{\"name\":\"Light 3\",\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}}";
    panel_ui_model_t model;
    apply(user, devices, &model);
    assert(panel_homey_favorites_get_state() == PANEL_HOMEY_FAVORITES_VALID_CONFIGURED);
    assert(strcmp(model.widget_title[4], "Light 1") == 0);
    assert(strcmp(model.widget_title[5], "Light 2") == 0);
}

static void test_unavailable_preserved(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[\"off\"]}}";
    const char *devices = "{\"off\":{\"name\":\"Unavailable\",\"available\":false,\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}";
    panel_ui_model_t model;
    apply(user, devices, &model);
    assert(strcmp(model.widget_title[4], "Unavailable") == 0);
    assert(model.widget_status[4] == PANEL_WIDGET_UNAVAILABLE);
    assert(model.widget_has_boolean[4] == false);
}

static void test_bounded_and_deterministic(void)
{
    char user[4096];
    char devices[16384];
    size_t u = 0U, d = 0U;
    u += (size_t)snprintf(user + u, sizeof(user) - u, "{\"properties\":{\"favoriteDevices\":[");
    d += (size_t)snprintf(devices + d, sizeof(devices) - d, "{");
    for (size_t i = 0U; i < 70U; ++i) {
        u += (size_t)snprintf(user + u, sizeof(user) - u, "%s\"d%02u\"", i ? "," : "", (unsigned)i);
        d += (size_t)snprintf(devices + d, sizeof(devices) - d,
                              "%s\"d%02u\":{\"name\":\"D%02u\",\"capabilitiesObj\":{\"onoff\":{\"value\":%s}}}",
                              i ? "," : "", (unsigned)i, (unsigned)i, i == 1U ? "true" : "false");
    }
    (void)snprintf(user + u, sizeof(user) - u, "]}}");
    (void)snprintf(devices + d, sizeof(devices) - d, "}");
    panel_ui_model_t model;
    apply(user, devices, &model);
    assert(strcmp(model.widget_title[4], "D00") == 0);
    assert(strcmp(model.widget_title[5], "D01") == 0);
}

static void test_public_model_contains_no_ids(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[\"private-device-id-123\"]}}";
    const char *devices = "{\"private-device-id-123\":{\"name\":\"Public Name\",\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}";
    panel_homey_favorites_public_t published;
    assert(panel_homey_favorites_parse_and_publish(user, devices) == PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_copy_public(&published));
    assert(published.count == 1U);
    assert(strcmp(published.items[0].name, "Public Name") == 0);
    assert(published.items[0].available);
    assert(published.items[0].onoff_known);
    assert(published.items[0].onoff);
    /* The public type contains only name/availability/onoff fields. Raw IDs
     * are consumed only in the private JSON join layer. */
}

static void test_ui_apply_reports_only_real_changes(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[\"light1\",\"light2\"]}}";
    const char *devices = "{\"light1\":{\"name\":\"Ytterbelysning\",\"capabilitiesObj\":{\"onoff\":{\"value\":false}}},\"light2\":{\"name\":\"Dekorbelysning\",\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}";
    panel_ui_model_t model;
    memset(&model, 0, sizeof(model));
    assert(panel_homey_favorites_parse_and_publish(user, devices) == PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_apply_ui_model(&model));
    assert(strcmp(model.widget_title[4], "Ytterbelysning") == 0);
    assert(strcmp(model.widget_title[5], "Dekorbelysning") == 0);
    assert(model.widget_status[4] == PANEL_WIDGET_AVAILABLE);
    assert(model.widget_status[5] == PANEL_WIDGET_AVAILABLE);
    assert(model.widget_has_boolean[4]);
    assert(model.widget_has_boolean[5]);
    assert(model.widget_boolean_value[4] == false);
    assert(model.widget_boolean_value[5] == true);
    assert(!panel_homey_favorites_apply_ui_model(&model));

    (void)snprintf(model.widget_title[4], sizeof(model.widget_title[4]), "%s", "Belysning 1");
    model.widget_status[4] = PANEL_WIDGET_UNCONFIGURED;
    assert(panel_homey_favorites_apply_ui_model(&model));
    assert(strcmp(model.widget_title[4], "Ytterbelysning") == 0);
    assert(model.widget_status[4] == PANEL_WIDGET_AVAILABLE);
}

static void test_dashboard_snapshot_cannot_erase_favorite_boolean_state(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[\"light1\",\"light2\"]}}";
    const char *devices = "{\"light1\":{\"name\":\"Ytterbelysning\",\"capabilitiesObj\":{\"onoff\":{\"value\":false}}},\"light2\":{\"name\":\"Dekorbelysning\",\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}";

    panel_ui_model_t model;
    panel_ui_model_init(&model, 0U);
    assert(panel_homey_favorites_parse_and_publish(user, devices) == PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_apply_ui_model(&model));
    assert(model.widget_has_boolean[4]);
    assert(model.widget_has_boolean[5]);
    assert(model.widget_boolean_value[4] == false);
    assert(model.widget_boolean_value[5] == true);

    panel_homey_dashboard_state_t dashboard;
    panel_homey_dashboard_state_init(&dashboard);
    dashboard.generation = 123U;
    dashboard.generation_valid = true;
    dashboard.widgets[4].status = PANEL_WIDGET_AVAILABLE;
    dashboard.widgets[4].has_boolean = false;
    dashboard.widgets[5].status = PANEL_WIDGET_AVAILABLE;
    dashboard.widgets[5].has_boolean = false;

    assert(panel_ui_apply_homey_dashboard_state(&model, &dashboard));

    /* Favorite-owned widgets must survive the alias snapshot unchanged. */
    assert(strcmp(model.widget_title[4], "Ytterbelysning") == 0);
    assert(strcmp(model.widget_title[5], "Dekorbelysning") == 0);
    assert(model.widget_status[4] == PANEL_WIDGET_AVAILABLE);
    assert(model.widget_status[5] == PANEL_WIDGET_AVAILABLE);
    assert(model.widget_has_boolean[4]);
    assert(model.widget_has_boolean[5]);
    assert(model.widget_boolean_value[4] == false);
    assert(model.widget_boolean_value[5] == true);
}

static void test_unverified_matching_failure_is_unknown(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[\"stale\",\"light1\"]}}";
    const char *devices = "{\"light1\":{\"name\":\"Light 1\",\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}";
    panel_ui_model_t model;
    apply(user, devices, &model);
    assert(panel_homey_favorites_get_state() == PANEL_HOMEY_FAVORITES_UNVERIFIED);
    assert(model.widget_status[4] == PANEL_WIDGET_UNKNOWN);
    assert(model.widget_status[5] == PANEL_WIDGET_UNKNOWN);
    assert(!model.widget_has_boolean[4]);
    assert(!model.widget_has_boolean[5]);
}

static void test_valid_empty_is_unconfigured(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[]}}";
    const char *devices = "{}";
    panel_ui_model_t model;
    apply(user, devices, &model);
    assert(panel_homey_favorites_get_state() == PANEL_HOMEY_FAVORITES_VALID_EMPTY);
    assert(model.widget_status[4] == PANEL_WIDGET_UNCONFIGURED);
    assert(model.widget_status[5] == PANEL_WIDGET_UNCONFIGURED);
    assert(strcmp(model.widget_title[4], "Belysning 1") == 0);
    assert(strcmp(model.widget_title[5], "Belysning 2") == 0);
}

static void test_missing_or_malformed_favorites_are_unverified(void)
{
    const char *devices = "{}";
    panel_ui_model_t model;

    memset(&model, 0, sizeof(model));
    assert(panel_homey_favorites_parse_and_publish("{\"properties\":{}}", devices) != PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_get_state() == PANEL_HOMEY_FAVORITES_UNVERIFIED);
    assert(panel_homey_favorites_apply_ui_model(&model));
    assert(model.widget_status[4] == PANEL_WIDGET_UNKNOWN);
    assert(model.widget_status[5] == PANEL_WIDGET_UNKNOWN);

    memset(&model, 0, sizeof(model));
    assert(panel_homey_favorites_parse_and_publish("not-json", devices) != PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_get_state() == PANEL_HOMEY_FAVORITES_UNVERIFIED);
    assert(panel_homey_favorites_apply_ui_model(&model));
    assert(model.widget_status[4] == PANEL_WIDGET_UNKNOWN);
    assert(model.widget_status[5] == PANEL_WIDGET_UNKNOWN);
}

static void test_capability_failure_is_unverified(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[\"light1\"]}}";
    const char *devices = "{\"light1\":{\"name\":\"Light 1\",\"capabilitiesObj\":{\"onoff\":{\"value\":\"not-bool\"}}}}";
    panel_ui_model_t model;
    apply(user, devices, &model);
    assert(panel_homey_favorites_get_state() == PANEL_HOMEY_FAVORITES_UNVERIFIED);
    assert(model.widget_status[4] == PANEL_WIDGET_UNKNOWN);
    assert(model.widget_status[5] == PANEL_WIDGET_UNKNOWN);
}

static void test_dim_wake_preserves_favorite_display_text(void)
{
    const char *user = "{\"properties\":{\"favoriteDevices\":[\"light1\",\"light2\"]}}";
    const char *devices = "{\"light1\":{\"name\":\"Ytterbelysning\",\"capabilitiesObj\":{\"onoff\":{\"value\":false}}},\"light2\":{\"name\":\"Dekorbelysning\",\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}";
    panel_ui_model_t model;
    char text[32];

    panel_ui_model_init(&model, 0U);
    assert(panel_homey_favorites_parse_and_publish(user, devices) == PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_apply_ui_model(&model));

    assert(panel_ui_widget_display_text(&model, 4U, text, sizeof(text)));
    assert(strcmp(text, "Släckt") == 0);
    assert(panel_ui_widget_display_text(&model, 5U, text, sizeof(text)));
    assert(strcmp(text, "Tänd") == 0);

    model.settings.dim_after_seconds = 10U;
    panel_ui_tick(&model, 10000U);
    assert(model.power_state == PANEL_POWER_DIMMED);
    assert(panel_ui_handle_touch(&model, 11000U));
    assert(model.power_state == PANEL_POWER_ACTIVE);

    panel_homey_dashboard_state_t dashboard;
    panel_homey_dashboard_state_init(&dashboard);
    dashboard.generation = 456U;
    dashboard.generation_valid = true;
    dashboard.widgets[4].status = PANEL_WIDGET_AVAILABLE;
    dashboard.widgets[4].has_boolean = false;
    dashboard.widgets[5].status = PANEL_WIDGET_AVAILABLE;
    dashboard.widgets[5].has_boolean = false;
    assert(panel_ui_apply_homey_dashboard_state(&model, &dashboard));

    /* Favorite ownership and display formatter must survive dim/wake/poll. */
    assert(model.widget_has_boolean[4]);
    assert(model.widget_has_boolean[5]);
    assert(panel_ui_widget_display_text(&model, 4U, text, sizeof(text)));
    assert(strcmp(text, "Släckt") == 0);
    assert(panel_ui_widget_display_text(&model, 5U, text, sizeof(text)));
    assert(strcmp(text, "Tänd") == 0);
}

int main(void)
{
    test_authoritative_order_differs_from_inventory();
    test_exact_id_join_and_legacy_heuristic_cannot_override();
    test_first_two_compatible_skip_non_onoff();
    test_unverified_matching_failure_is_unknown();
    test_valid_empty_is_unconfigured();
    test_missing_or_malformed_favorites_are_unverified();
    test_capability_failure_is_unverified();
    test_unavailable_preserved();
    test_bounded_and_deterministic();
    test_public_model_contains_no_ids();
    test_ui_apply_reports_only_real_changes();
    test_dashboard_snapshot_cannot_erase_favorite_boolean_state();
    test_dim_wake_preserves_favorite_display_text();
    puts("PATCH017_PANEL_HOMEY_FAVORITES_TEST PASS");
    return 0;
}
