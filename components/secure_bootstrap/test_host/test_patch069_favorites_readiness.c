#include "panel_homey_read_snapshot.h"
#include "panel_homey_favorites.h"
#include "panel_homey_dashboard_binding.h"
#include "panel_ui_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *raw_ids[] = {"fixture-a1", "fixture-a2", "fixture-a3", "fixture-security", "fixture-l1", "fixture-l2"};
static const char *caps[] = {"position", "position", "position", "alarm", "onoff", "onoff"};
static panel_homey_read_result_t capture(void *unused, panel_homey_alias_snapshot_t *out)
{
    (void)unused;
    memset(out, 0, sizeof(*out));
    out->configured = true; out->generation = 1U; out->entry_count = 6U;
    for (size_t i = 0; i < 6U; ++i) {
        panel_homey_alias_snapshot_entry_t *e = &out->entries[i];
        e->dashboard_binding_index = (uint8_t)i;
        strcpy(e->raw_device_id, raw_ids[i]); strcpy(e->raw_capability_id, caps[i]);
        strcpy(e->device_alias, panel_homey_dashboard_device_alias(i));
        strcpy(e->capability_alias, panel_homey_dashboard_capability_alias(i));
    }
    return PANEL_HOMEY_READ_OK;
}
static panel_homey_read_result_t resolve(void *unused, const char *device, const char *cap,
    char *alias, size_t alias_size, char *cap_alias, size_t cap_size)
{
    (void)unused;
    for (size_t i = 0; i < 6U; ++i) if (!strcmp(device, raw_ids[i]) && !strcmp(cap, caps[i])) {
        assert(strlen(panel_homey_dashboard_device_alias(i)) < alias_size);
        assert(strlen(panel_homey_dashboard_capability_alias(i)) < cap_size);
        strcpy(alias, panel_homey_dashboard_device_alias(i));
        strcpy(cap_alias, panel_homey_dashboard_capability_alias(i));
        return PANEL_HOMEY_READ_OK;
    }
    return PANEL_HOMEY_READ_NOT_FOUND;
}
static const char inventory[] =
    "{\"fixture-a1\":{\"id\":\"fixture-a1\",\"available\":true,\"capabilitiesObj\":{\"position\":{\"value\":0.1}}},"
    "\"fixture-a2\":{\"id\":\"fixture-a2\",\"available\":true,\"capabilitiesObj\":{\"position\":{\"value\":0.2}}},"
    "\"fixture-a3\":{\"id\":\"fixture-a3\",\"available\":true,\"capabilitiesObj\":{\"position\":{\"value\":0.3}}},"
    "\"fixture-security\":{\"id\":\"fixture-security\",\"available\":true,\"capabilitiesObj\":{\"alarm\":{\"value\":true}}},"
    "\"fixture-l1\":{\"id\":\"fixture-l1\",\"name\":\"Fixture Light 1\",\"available\":true,\"capabilitiesObj\":{\"onoff\":{\"value\":true,\"setable\":true}}},"
    "\"fixture-l2\":{\"id\":\"fixture-l2\",\"name\":\"Fixture Light 2\",\"available\":true,\"capabilitiesObj\":{\"onoff\":{\"value\":false,\"setable\":true}}}}";
int main(void)
{
    const panel_homey_alias_provider_t provider = {.capture=capture, .resolve=resolve};
    panel_homey_snapshot_store_t store;
    panel_homey_snapshot_store_init(&store, NULL, NULL, NULL);
    assert(panel_homey_snapshot_publish_json(&store, inventory, &provider, 1000U) == PANEL_HOMEY_READ_OK);
    assert(panel_homey_favorites_parse_and_publish_with_alias_provider(
        "{\"properties\":{\"favoriteDevices\":[\"fixture-l1\",\"fixture-l2\"]}}", inventory, &provider) == PANEL_HOMEY_FAVORITES_OK);
    panel_homey_read_snapshot_t before, after;
    assert(panel_homey_snapshot_copy(&store, 1100U, &before) == PANEL_HOMEY_READ_OK);
    panel_ui_model_t ui;
    panel_ui_model_init(&ui, 1000U);
    (void)panel_homey_favorites_apply_ui_model(&ui);
    assert(ui.widget_has_boolean[4] && ui.widget_has_boolean[5]);
    /* This is the actual production action on a Favorites transport failure. */
    panel_homey_favorites_clear();
    assert(panel_homey_snapshot_copy(&store, 1200U, &after) == PANEL_HOMEY_READ_OK);
    assert(memcmp(&before, &after, sizeof(before)) == 0);
    panel_homey_awning_diagnostic_t awnings[3];
    assert(panel_homey_dashboard_awning_diagnostics(&after, awnings));
    for (size_t i=0; i<3U; ++i) assert(awnings[i].matched && awnings[i].available);
    (void)panel_homey_favorites_apply_ui_model(&ui);
    for (size_t i=4; i<6U; ++i) {
        assert(ui.widget_status[i] == PANEL_WIDGET_UNKNOWN);
        assert(!ui.widget_has_boolean[i]);
        assert(!panel_homey_favorites_light_toggle_execution_ready(i, true));
    }
    panel_homey_dashboard_state_t dashboard;
    panel_homey_dashboard_state_init(&dashboard);
    (void)panel_homey_dashboard_apply_snapshot(PANEL_HOMEY_READ_OK, &after, 1200U, &dashboard);
    assert(dashboard.widgets[3].status == PANEL_WIDGET_AVAILABLE);
    assert(dashboard.widgets[3].has_boolean && dashboard.widgets[3].boolean_value);
    assert(panel_homey_snapshot_publish_json(&store, inventory, &provider, 2000U) == PANEL_HOMEY_READ_OK);
    assert(panel_homey_snapshot_copy(&store, 2100U, &after) == PANEL_HOMEY_READ_OK);
    assert(after.generation == before.generation + 1U);
    puts("PATCH069_REAL_SNAPSHOT_FAVORITES_ISOLATION PASS");
    return 0;
}
