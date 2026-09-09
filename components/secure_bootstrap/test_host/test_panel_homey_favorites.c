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
    assert(!published.items[0].onoff_command_eligible);
    /* The public type contains only sanitized display/readiness fields.
     * Raw IDs are consumed only in the private JSON join layer. */
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


static panel_homey_favorites_public_t parse_candidate_public(const char *devices_json)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"candidate\"]}}";
    panel_homey_favorites_public_t published;
    assert(panel_homey_favorites_parse_and_publish(user, devices_json) ==
           PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_copy_public(&published));
    return published;
}

static void assert_candidate_published_eligibility(
    const char *devices_json,
    bool expected_eligible)
{
    panel_homey_favorites_public_t published =
        parse_candidate_public(devices_json);
    assert(published.count == 1U);
    assert(strcmp(published.items[0].name, "Candidate") == 0);
    assert(published.items[0].onoff_known);
    assert(published.items[0].onoff);
    assert(published.items[0].onoff_command_eligible == expected_eligible);
}

static void assert_candidate_not_published(const char *devices_json)
{
    panel_homey_favorites_public_t published =
        parse_candidate_public(devices_json);
    assert(published.count == 0U);
    assert(panel_homey_favorites_get_state() ==
           PANEL_HOMEY_FAVORITES_UNVERIFIED);
}

static void test_command_eligibility_exact_v1_shape(void)
{
    const char *devices =
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}";
    assert_candidate_published_eligibility(devices, true);

    const char *non_light_class =
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"class\":\"socket\",\"virtualClass\":\"sensor\","
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}";
    assert_candidate_published_eligibility(non_light_class, true);
}

static void test_command_eligibility_requires_available(void)
{
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\","
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":\"yes\","
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":false,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}",
        false);
}

static void test_command_eligibility_requires_exact_capabilities_membership(void)
{
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":{},"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"dim\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff.extra\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}",
        false);
}

static void test_command_eligibility_requires_direct_onoff_object(void)
{
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"legacy\":{\"id\":\"onoff\",\"value\":true,"
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}",
        false);

    assert_candidate_not_published(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],\"capabilitiesObj\":{\"onoff\":\"bad\"}}}");
}

static void test_command_eligibility_requires_boolean_value(void)
{
    assert_candidate_not_published(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}}}}");
    assert_candidate_not_published(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":\"true\","
        "\"type\":\"boolean\",\"getable\":true,\"setable\":true}}}}");
}

static void test_command_eligibility_requires_boolean_type_metadata(void)
{
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,"
        "\"getable\":true,\"setable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":true,"
        "\"getable\":true,\"setable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"number\","
        "\"getable\":true,\"setable\":true}}}}",
        false);
}

static void test_command_eligibility_requires_getable_true(void)
{
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"setable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":\"yes\",\"setable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":false,\"setable\":true}}}}",
        false);
}

static void test_command_eligibility_requires_setable_true(void)
{
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":\"yes\"}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":false}}}}",
        false);
}

static void test_command_eligibility_rejects_any_capabilities_options_presence(void)
{
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}},\"capabilitiesOptions\":null}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}},\"capabilitiesOptions\":{}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}},"
        "\"capabilitiesOptions\":{\"onoff\":{}}}}",
        false);
    assert_candidate_published_eligibility(
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}},"
        "\"capabilitiesOptions\":\"unexpected\"}}",
        false);
}

static void test_command_eligibility_preserves_read_only_display_when_false(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"candidate\"]}}";
    const char *devices =
        "{\"candidate\":{\"name\":\"Candidate\",\"available\":true,"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}";
    panel_homey_favorites_public_t published;
    panel_ui_model_t model;

    memset(&model, 0, sizeof(model));
    assert(panel_homey_favorites_parse_and_publish(user, devices) ==
           PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_copy_public(&published));
    assert(published.count == 1U);
    assert(!published.items[0].onoff_command_eligible);
    assert(panel_homey_favorites_apply_ui_model(&model));
    assert(model.widget_status[4] == PANEL_WIDGET_AVAILABLE);
    assert(model.widget_has_boolean[4]);
    assert(model.widget_boolean_value[4]);
}

static void test_command_eligibility_two_valid_favorites_preserve_order(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"second\",\"first\"]}}";
    const char *devices =
        "{\"first\":{\"name\":\"First\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}}},"
        "\"second\":{\"name\":\"Second\",\"available\":true,\"class\":\"socket\","
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":false,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}}}}";
    panel_homey_favorites_public_t published;

    assert(panel_homey_favorites_parse_and_publish(user, devices) ==
           PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_copy_public(&published));
    assert(published.count == 2U);
    assert(strcmp(published.items[0].name, "Second") == 0);
    assert(strcmp(published.items[1].name, "First") == 0);
    assert(published.items[0].onoff_command_eligible);
    assert(published.items[1].onoff_command_eligible);
    assert(!published.items[0].onoff);
    assert(published.items[1].onoff);
}


typedef struct {
    const char *device_a;
    const char *device_a_alias;
    const char *device_b;
    const char *device_b_alias;
    const char *accepted_raw_capability;
    const char *returned_capability_alias;
    bool force_result;
    panel_homey_read_result_t forced_result;
    size_t calls;
    bool saw_exact_onoff;
} authorization_provider_context_t;

static bool authorization_copy_text(char *out, size_t capacity, const char *value)
{
    if (out == NULL || capacity == 0U || value == NULL) return false;
    size_t length = strlen(value);
    if (length + 1U > capacity) return false;
    memcpy(out, value, length + 1U);
    return true;
}

static panel_homey_read_result_t authorization_provider_resolve(
    void *context,
    const char *raw_device_id,
    const char *raw_capability_id,
    char *device_alias_out,
    size_t device_alias_capacity,
    char *capability_alias_out,
    size_t capability_alias_capacity)
{
    authorization_provider_context_t *ctx = context;
    if (device_alias_out != NULL && device_alias_capacity > 0U) device_alias_out[0] = '\0';
    if (capability_alias_out != NULL && capability_alias_capacity > 0U) capability_alias_out[0] = '\0';
    if (ctx == NULL || raw_device_id == NULL || raw_capability_id == NULL) {
        return PANEL_HOMEY_READ_INVALID;
    }

    ctx->calls++;
    if (strcmp(raw_capability_id, "onoff") == 0) ctx->saw_exact_onoff = true;
    if (ctx->force_result) return ctx->forced_result;

    const char *accepted_capability =
        ctx->accepted_raw_capability != NULL ? ctx->accepted_raw_capability : "onoff";
    if (strcmp(raw_capability_id, accepted_capability) != 0) {
        return PANEL_HOMEY_READ_NOT_FOUND;
    }

    const char *device_alias = NULL;
    if (ctx->device_a != NULL && strcmp(raw_device_id, ctx->device_a) == 0) {
        device_alias = ctx->device_a_alias;
    } else if (ctx->device_b != NULL && strcmp(raw_device_id, ctx->device_b) == 0) {
        device_alias = ctx->device_b_alias;
    } else {
        return PANEL_HOMEY_READ_NOT_FOUND;
    }

    const char *capability_alias = ctx->returned_capability_alias != NULL
        ? ctx->returned_capability_alias
        : "on";
    if (!authorization_copy_text(device_alias_out, device_alias_capacity, device_alias) ||
        !authorization_copy_text(capability_alias_out, capability_alias_capacity, capability_alias)) {
        return PANEL_HOMEY_READ_OVERFLOW;
    }
    return PANEL_HOMEY_READ_OK;
}

static authorization_provider_context_t authorization_provider_valid(void)
{
    authorization_provider_context_t ctx = {0};
    ctx.device_a = "first";
    ctx.device_a_alias = "light_1";
    ctx.device_b = "second";
    ctx.device_b_alias = "light_2";
    ctx.accepted_raw_capability = "onoff";
    ctx.returned_capability_alias = "on";
    return ctx;
}

static panel_homey_alias_provider_t authorization_provider(
    authorization_provider_context_t *ctx)
{
    panel_homey_alias_provider_t provider = {
        .context = ctx,
        .resolve = authorization_provider_resolve,
    };
    return provider;
}

static const char *authorization_devices_two(void)
{
    return
        "{\"first\":{\"name\":\"First\",\"available\":true,\"class\":\"socket\","
        "\"virtualClass\":\"sensor\",\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}}},"
        "\"second\":{\"name\":\"Second\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":false,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}}}}";
}

static const char *authorization_devices_one(void)
{
    return
        "{\"first\":{\"name\":\"First\",\"available\":true,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}}}}";
}

static panel_homey_favorites_public_t authorization_parse(
    const char *user_json,
    const char *devices_json,
    const panel_homey_alias_provider_t *provider)
{
    panel_homey_favorites_public_t published;
    assert(panel_homey_favorites_parse_and_publish_with_alias_provider(
        user_json,
        devices_json,
        provider) == PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_copy_public(&published));
    return published;
}

static void test_light_toggle_authorization_valid_widget4_widget5(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\",\"second\"]}}";
    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = authorization_provider(&ctx);
    panel_homey_favorites_public_t published =
        authorization_parse(user, authorization_devices_two(), &provider);

    assert(strcmp(panel_homey_dashboard_device_alias(4U), "light_1") == 0);
    assert(strcmp(panel_homey_dashboard_capability_alias(4U), "on") == 0);
    assert(strcmp(panel_homey_dashboard_device_alias(5U), "light_2") == 0);
    assert(strcmp(panel_homey_dashboard_capability_alias(5U), "on") == 0);
    assert(published.state == PANEL_HOMEY_FAVORITES_VALID_CONFIGURED);
    assert(published.count == 2U);
    assert(published.items[0].onoff_command_eligible);
    assert(published.items[1].onoff_command_eligible);
    assert(published.items[0].light_toggle_authorized);
    assert(published.items[1].light_toggle_authorized);
    assert(ctx.calls == 2U);
    assert(ctx.saw_exact_onoff);
}

static void test_light_toggle_authorization_no_provider_or_missing_resolver(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\",\"second\"]}}";
    panel_homey_favorites_public_t published =
        authorization_parse(user, authorization_devices_two(), NULL);
    assert(!published.items[0].light_toggle_authorized);
    assert(!published.items[1].light_toggle_authorized);

    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = {.context = &ctx, .resolve = NULL};
    published = authorization_parse(user, authorization_devices_two(), &provider);
    assert(!published.items[0].light_toggle_authorized);
    assert(!published.items[1].light_toggle_authorized);
}

static void test_light_toggle_authorization_provider_failures(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\"]}}";
    const panel_homey_read_result_t failures[] = {
        PANEL_HOMEY_READ_NOT_CONFIGURED,
        PANEL_HOMEY_READ_NOT_FOUND,
        PANEL_HOMEY_READ_DUPLICATE,
        PANEL_HOMEY_READ_INVALID,
        PANEL_HOMEY_READ_OVERFLOW,
        PANEL_HOMEY_READ_STALE,
    };

    for (size_t i = 0U; i < sizeof(failures) / sizeof(failures[0]); ++i) {
        authorization_provider_context_t ctx = authorization_provider_valid();
        ctx.force_result = true;
        ctx.forced_result = failures[i];
        panel_homey_alias_provider_t provider = authorization_provider(&ctx);
        panel_homey_favorites_public_t published =
            authorization_parse(user, authorization_devices_one(), &provider);
        assert(published.items[0].onoff_command_eligible);
        assert(!published.items[0].light_toggle_authorized);
    }
}

static void test_light_toggle_authorization_device_and_capability_mismatch(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\"]}}";

    authorization_provider_context_t device_mismatch = authorization_provider_valid();
    device_mismatch.device_a = "other";
    panel_homey_alias_provider_t provider = authorization_provider(&device_mismatch);
    panel_homey_favorites_public_t published =
        authorization_parse(user, authorization_devices_one(), &provider);
    assert(!published.items[0].light_toggle_authorized);

    authorization_provider_context_t capability_mismatch = authorization_provider_valid();
    capability_mismatch.accepted_raw_capability = "dim";
    provider = authorization_provider(&capability_mismatch);
    published = authorization_parse(user, authorization_devices_one(), &provider);
    assert(capability_mismatch.saw_exact_onoff);
    assert(!published.items[0].light_toggle_authorized);

    authorization_provider_context_t alias_mismatch = authorization_provider_valid();
    alias_mismatch.returned_capability_alias = "active";
    provider = authorization_provider(&alias_mismatch);
    published = authorization_parse(user, authorization_devices_one(), &provider);
    assert(!published.items[0].light_toggle_authorized);
}

static void test_light_toggle_authorization_cross_slot_and_swapped_order(void)
{
    const char *normal_user =
        "{\"properties\":{\"favoriteDevices\":[\"first\",\"second\"]}}";
    authorization_provider_context_t cross = authorization_provider_valid();
    cross.device_a_alias = "light_2";
    cross.device_b_alias = "light_1";
    panel_homey_alias_provider_t provider = authorization_provider(&cross);
    panel_homey_favorites_public_t published =
        authorization_parse(normal_user, authorization_devices_two(), &provider);
    assert(!published.items[0].light_toggle_authorized);
    assert(!published.items[1].light_toggle_authorized);

    const char *swapped_user =
        "{\"properties\":{\"favoriteDevices\":[\"second\",\"first\"]}}";
    authorization_provider_context_t correct = authorization_provider_valid();
    provider = authorization_provider(&correct);
    published = authorization_parse(swapped_user, authorization_devices_two(), &provider);
    assert(!published.items[0].light_toggle_authorized);
    assert(!published.items[1].light_toggle_authorized);
}

static void test_light_toggle_authorization_duplicate_favorite_fail_closed(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\",\"first\"]}}";
    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = authorization_provider(&ctx);
    panel_homey_favorites_public_t published =
        authorization_parse(user, authorization_devices_one(), &provider);
    assert(published.state == PANEL_HOMEY_FAVORITES_VALID_CONFIGURED);
    assert(published.count == 2U);
    assert(!published.items[0].light_toggle_authorized);
    assert(!published.items[1].light_toggle_authorized);
}

static void test_light_toggle_authorization_unverified_clears_all(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"missing\",\"first\"]}}";
    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = authorization_provider(&ctx);
    panel_homey_favorites_public_t published =
        authorization_parse(user, authorization_devices_one(), &provider);
    assert(published.state == PANEL_HOMEY_FAVORITES_UNVERIFIED);
    assert(published.count == 1U);
    assert(!published.items[0].light_toggle_authorized);
    assert(!published.items[1].light_toggle_authorized);
}

static void test_light_toggle_authorization_requires_command_eligibility_and_preserves_display(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\"]}}";
    const char *devices =
        "{\"first\":{\"name\":\"First\",\"available\":true,"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}";
    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = authorization_provider(&ctx);
    panel_homey_favorites_public_t published = authorization_parse(user, devices, &provider);
    assert(!published.items[0].onoff_command_eligible);
    assert(!published.items[0].light_toggle_authorized);

    panel_ui_model_t model;
    memset(&model, 0, sizeof(model));
    assert(panel_homey_favorites_apply_ui_model(&model));
    assert(strcmp(model.widget_title[4], "First") == 0);
    assert(model.widget_status[4] == PANEL_WIDGET_AVAILABLE);
    assert(model.widget_has_boolean[4]);
    assert(model.widget_boolean_value[4]);
}

static void test_light_toggle_authorization_legacy_parser_is_unauthorized(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\",\"second\"]}}";
    panel_homey_favorites_public_t published;
    assert(panel_homey_favorites_parse_and_publish(user, authorization_devices_two()) ==
           PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_copy_public(&published));
    assert(published.items[0].onoff_command_eligible);
    assert(published.items[1].onoff_command_eligible);
    assert(!published.items[0].light_toggle_authorized);
    assert(!published.items[1].light_toggle_authorized);
}

static void test_light_toggle_authorization_revoke_preserves_read_only_state(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\",\"second\"]}}";
    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = authorization_provider(&ctx);
    panel_homey_favorites_public_t before =
        authorization_parse(user, authorization_devices_two(), &provider);
    assert(before.items[0].light_toggle_authorized);
    assert(before.items[1].light_toggle_authorized);

    panel_homey_favorites_revoke_light_toggle_authorization();
    panel_homey_favorites_public_t after;
    assert(panel_homey_favorites_copy_public(&after));
    assert(after.state == before.state);
    assert(after.count == before.count);
    assert(strcmp(after.items[0].name, before.items[0].name) == 0);
    assert(strcmp(after.items[1].name, before.items[1].name) == 0);
    assert(after.items[0].available == before.items[0].available);
    assert(after.items[1].available == before.items[1].available);
    assert(after.items[0].onoff == before.items[0].onoff);
    assert(after.items[1].onoff == before.items[1].onoff);
    assert(after.items[0].onoff_command_eligible == before.items[0].onoff_command_eligible);
    assert(after.items[1].onoff_command_eligible == before.items[1].onoff_command_eligible);
    assert(!after.items[0].light_toggle_authorized);
    assert(!after.items[1].light_toggle_authorized);
}


static void test_light_toggle_execution_readiness_valid_widget4_widget5(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\",\"second\"]}}";
    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = authorization_provider(&ctx);
    panel_homey_favorites_public_t published =
        authorization_parse(user, authorization_devices_two(), &provider);

    assert(published.state == PANEL_HOMEY_FAVORITES_VALID_CONFIGURED);
    assert(published.count == 2U);
    assert(published.items[0].available);
    assert(published.items[1].available);
    assert(published.items[0].onoff_known);
    assert(published.items[1].onoff_known);
    assert(published.items[0].onoff_command_eligible);
    assert(published.items[1].onoff_command_eligible);
    assert(published.items[0].light_toggle_authorized);
    assert(published.items[1].light_toggle_authorized);
    assert(panel_homey_favorites_light_toggle_execution_ready(4U, true));
    assert(panel_homey_favorites_light_toggle_execution_ready(5U, true));
}

static void test_light_toggle_execution_readiness_requires_current_homey_data_ready(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\",\"second\"]}}";
    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = authorization_provider(&ctx);
    (void)authorization_parse(user, authorization_devices_two(), &provider);

    assert(panel_homey_favorites_light_toggle_execution_ready(4U, true));
    assert(!panel_homey_favorites_light_toggle_execution_ready(4U, false));
    assert(panel_homey_favorites_light_toggle_execution_ready(4U, true));
    assert(panel_homey_favorites_light_toggle_execution_ready(5U, true));
    assert(!panel_homey_favorites_light_toggle_execution_ready(5U, false));
}

static void test_light_toggle_execution_readiness_state_slot_and_widget_guards(void)
{
    assert(panel_homey_favorites_parse_and_publish(
               "{\"properties\":{\"favoriteDevices\":[]}}",
               "{}") == PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_get_state() == PANEL_HOMEY_FAVORITES_VALID_EMPTY);
    assert(!panel_homey_favorites_light_toggle_execution_ready(4U, true));
    assert(!panel_homey_favorites_light_toggle_execution_ready(5U, true));

    assert(panel_homey_favorites_parse_and_publish(
               "{\"properties\":{\"favoriteDevices\":[\"missing\"]}}",
               "{}") == PANEL_HOMEY_FAVORITES_OK);
    assert(panel_homey_favorites_get_state() == PANEL_HOMEY_FAVORITES_UNVERIFIED);
    assert(!panel_homey_favorites_light_toggle_execution_ready(4U, true));

    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\"]}}";
    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = authorization_provider(&ctx);
    panel_homey_favorites_public_t published =
        authorization_parse(user, authorization_devices_one(), &provider);
    assert(published.count == 1U);
    assert(panel_homey_favorites_light_toggle_execution_ready(4U, true));
    assert(!panel_homey_favorites_light_toggle_execution_ready(5U, true));
    assert(!panel_homey_favorites_light_toggle_execution_ready(0U, true));
    assert(!panel_homey_favorites_light_toggle_execution_ready(3U, true));
    assert(!panel_homey_favorites_light_toggle_execution_ready(6U, true));
    assert(!panel_homey_favorites_light_toggle_execution_ready(999U, true));
}

static void test_light_toggle_execution_readiness_requires_item_authorities(void)
{
    const char *user =
        "{\"properties\":{\"favoriteDevices\":[\"first\"]}}";

    authorization_provider_context_t ctx = authorization_provider_valid();
    panel_homey_alias_provider_t provider = authorization_provider(&ctx);
    (void)authorization_parse(user, authorization_devices_one(), &provider);
    assert(panel_homey_favorites_light_toggle_execution_ready(4U, true));
    panel_homey_favorites_revoke_light_toggle_authorization();
    assert(!panel_homey_favorites_light_toggle_execution_ready(4U, true));

    const char *not_eligible =
        "{\"first\":{\"name\":\"First\",\"available\":true,"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true}}}}";
    ctx = authorization_provider_valid();
    provider = authorization_provider(&ctx);
    panel_homey_favorites_public_t published =
        authorization_parse(user, not_eligible, &provider);
    assert(published.state == PANEL_HOMEY_FAVORITES_VALID_CONFIGURED);
    assert(!published.items[0].onoff_command_eligible);
    assert(!published.items[0].light_toggle_authorized);
    assert(!panel_homey_favorites_light_toggle_execution_ready(4U, true));

    const char *unavailable =
        "{\"first\":{\"name\":\"First\",\"available\":false,"
        "\"capabilities\":[\"onoff\"],"
        "\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"type\":\"boolean\","
        "\"getable\":true,\"setable\":true}}}}";
    ctx = authorization_provider_valid();
    provider = authorization_provider(&ctx);
    published = authorization_parse(user, unavailable, &provider);
    assert(published.state == PANEL_HOMEY_FAVORITES_VALID_CONFIGURED);
    assert(!published.items[0].available);
    assert(!panel_homey_favorites_light_toggle_execution_ready(4U, true));

    panel_homey_favorites_clear();
    assert(panel_homey_favorites_copy_public(&published));
    assert(!published.items[0].onoff_known);
    assert(!panel_homey_favorites_light_toggle_execution_ready(4U, true));
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
    test_command_eligibility_exact_v1_shape();
    test_command_eligibility_requires_available();
    test_command_eligibility_requires_exact_capabilities_membership();
    test_command_eligibility_requires_direct_onoff_object();
    test_command_eligibility_requires_boolean_value();
    test_command_eligibility_requires_boolean_type_metadata();
    test_command_eligibility_requires_getable_true();
    test_command_eligibility_requires_setable_true();
    test_command_eligibility_rejects_any_capabilities_options_presence();
    test_command_eligibility_preserves_read_only_display_when_false();
    test_command_eligibility_two_valid_favorites_preserve_order();
    test_light_toggle_authorization_valid_widget4_widget5();
    test_light_toggle_authorization_no_provider_or_missing_resolver();
    test_light_toggle_authorization_provider_failures();
    test_light_toggle_authorization_device_and_capability_mismatch();
    test_light_toggle_authorization_cross_slot_and_swapped_order();
    test_light_toggle_authorization_duplicate_favorite_fail_closed();
    test_light_toggle_authorization_unverified_clears_all();
    test_light_toggle_authorization_requires_command_eligibility_and_preserves_display();
    test_light_toggle_authorization_legacy_parser_is_unauthorized();
    test_light_toggle_authorization_revoke_preserves_read_only_state();
    test_light_toggle_execution_readiness_valid_widget4_widget5();
    test_light_toggle_execution_readiness_requires_current_homey_data_ready();
    test_light_toggle_execution_readiness_state_slot_and_widget_guards();
    test_light_toggle_execution_readiness_requires_item_authorities();
    puts("PATCH017_PANEL_HOMEY_FAVORITES_TEST PASS");
    puts("PATCH034_HOMEY_ONOFF_COMMAND_ELIGIBILITY_TEST PASS");
    puts("PATCH035_HOMEY_LIGHT_TOGGLE_AUTHORIZATION_TEST PASS");
    puts("PATCH036_HOMEY_LIGHT_TOGGLE_EXECUTION_READINESS_TEST PASS");
    return 0;
}
