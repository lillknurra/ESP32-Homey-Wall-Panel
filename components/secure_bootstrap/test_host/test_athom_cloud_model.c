#include "athom_cloud_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void fill_token(char *out, size_t length, char seed)
{
    for (size_t i = 0U; i < length; ++i) out[i] = (char)(seed + (i % 20U));
    out[length] = '\0';
}

int main(void)
{
    assert(strcmp(athom_alias_activation_classification(false, PANEL_HOMEY_ALIAS_STORE_OK), "not_attempted") == 0);
    assert(strcmp(athom_alias_activation_classification(true, PANEL_HOMEY_ALIAS_STORE_OK), "ok") == 0);
    assert(strcmp(athom_alias_activation_classification(true, PANEL_HOMEY_ALIAS_STORE_NOT_FOUND), "not_found") == 0);
    assert(strcmp(athom_alias_activation_classification(true, PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED), "not_configured") == 0);
    assert(strcmp(athom_alias_activation_classification(true, PANEL_HOMEY_ALIAS_STORE_INVALID), "invalid") == 0);
    assert(strcmp(athom_alias_activation_classification(true, PANEL_HOMEY_ALIAS_STORE_IO_ERROR), "io_error") == 0);
    assert(strcmp(athom_alias_activation_classification(true, PANEL_HOMEY_ALIAS_STORE_VERIFY_ERROR), "verify_error") == 0);
    panel_homey_snapshot_inspection_t inspection = {0};
    panel_homey_snapshot_publish_inspection_t publish_inspection = {0};
    inspection.result = PANEL_HOMEY_READ_NOT_FOUND;
    char awning_json[ATHOM_HOMEY_AWNING_SNAPSHOT_JSON_MAX];
    assert(athom_homey_awning_snapshot_json(
        awning_json, sizeof(awning_json), &inspection, &publish_inspection, false,
        PANEL_HOMEY_ALIAS_STORE_OK));
    assert(strstr(awning_json, "\"present\":false") != NULL);
    assert(strstr(awning_json, "\"generation_valid\":false") != NULL);
    assert(strstr(awning_json, "\"alias_activation\":\"not_attempted\"") != NULL);
    assert(strstr(awning_json,
        "\"last_publish\":{\"attempted\":false,\"result\":\"not_attempted\",\"age_ms\":null}") != NULL);
    assert(strstr(awning_json, "\"available\":null") != NULL);
    assert(!athom_homey_awning_snapshot_json(
        awning_json, 512U, &inspection, &publish_inspection, false,
        PANEL_HOMEY_ALIAS_STORE_OK));

    memset(&inspection, 0, sizeof(inspection));
    publish_inspection.attempted = true;
    publish_inspection.result = PANEL_HOMEY_READ_OK;
    publish_inspection.age_valid = true;
    publish_inspection.age_ms = 7U;
    inspection.present = true;
    inspection.fresh = true;
    inspection.result = PANEL_HOMEY_READ_OK;
    inspection.age_ms = 25U;
    inspection.snapshot.generation = 41U;
    assert(athom_homey_awning_snapshot_json(
        awning_json, sizeof(awning_json), &inspection, &publish_inspection, true,
        PANEL_HOMEY_ALIAS_STORE_OK));
    assert(strstr(awning_json, "\"result\":\"ok\"") != NULL);
    assert(strstr(awning_json, "\"fresh\":true") != NULL);
    assert(strstr(awning_json, "\"generation_valid\":true") != NULL);
    assert(strstr(awning_json, "\"generation\":41") != NULL);
    assert(strstr(awning_json, "\"age_ms\":25") != NULL);
    assert(strstr(awning_json, "\"alias_activation\":\"ok\"") != NULL);
    assert(strstr(awning_json,
        "\"last_publish\":{\"attempted\":true,\"result\":\"ok\",\"age_ms\":7}") != NULL);

    memset(&inspection, 0, sizeof(inspection));
    publish_inspection.result = PANEL_HOMEY_READ_INVALID;
    publish_inspection.age_ms = 3U;
    inspection.present = true;
    inspection.fresh = false;
    inspection.result = PANEL_HOMEY_READ_STALE;
    inspection.age_ms = 120001U;
    inspection.snapshot.generation = 42U;
    inspection.snapshot.item_count = 1U;
    strcpy(inspection.snapshot.items[0].device_alias, "awning_2");
    strcpy(inspection.snapshot.items[0].capability_alias, "status");
    inspection.snapshot.items[0].available = true;
    inspection.snapshot.awning_match_stages[0] = (panel_homey_awning_match_stages_t){
        PANEL_HOMEY_MATCH_VALUE_FALSE, PANEL_HOMEY_MATCH_VALUE_UNKNOWN,
        PANEL_HOMEY_MATCH_VALUE_UNKNOWN, PANEL_HOMEY_MATCH_VALUE_FALSE};
    inspection.snapshot.awning_match_stages[1] = (panel_homey_awning_match_stages_t){
        PANEL_HOMEY_MATCH_VALUE_TRUE, PANEL_HOMEY_MATCH_VALUE_TRUE,
        PANEL_HOMEY_MATCH_VALUE_TRUE, PANEL_HOMEY_MATCH_VALUE_TRUE};
    inspection.snapshot.awning_match_stages[2] = (panel_homey_awning_match_stages_t){
        PANEL_HOMEY_MATCH_VALUE_TRUE, PANEL_HOMEY_MATCH_VALUE_TRUE,
        PANEL_HOMEY_MATCH_VALUE_FALSE, PANEL_HOMEY_MATCH_VALUE_FALSE};
    assert(athom_homey_awning_snapshot_json(
        awning_json, sizeof(awning_json), &inspection, &publish_inspection, true,
        PANEL_HOMEY_ALIAS_STORE_VERIFY_ERROR));
    assert(strstr(awning_json, "\"result\":\"stale\"") != NULL);
    assert(strstr(awning_json, "\"generation\":42") != NULL);
    assert(strstr(awning_json, "\"age_ms\":120001") != NULL);
    assert(strstr(awning_json, "\"alias_activation\":\"verify_error\"") != NULL);
    assert(strstr(awning_json, "\"alias\":\"awning_2\",\"matched\":true,\"available\":true") != NULL);
    assert(strstr(awning_json, "\"alias\":\"awning_2\",\"matched\":true,\"available\":true,\"binding_entry_present\":true,\"device_present\":true,\"capability_present\":true") != NULL);
    assert(strstr(awning_json, "\"item_count\":1") != NULL);
    assert(strstr(awning_json, "\"alias\":\"awning_1\",\"matched\":false,\"available\":null") != NULL);
    assert(strstr(awning_json, "\"alias\":\"awning_3\",\"matched\":false,\"available\":null") != NULL);
    assert(strstr(awning_json, "\"alias\":\"awning_1\",\"matched\":false,\"available\":null,\"binding_entry_present\":false,\"device_present\":null,\"capability_present\":null") != NULL);
    assert(strstr(awning_json, "\"alias\":\"awning_3\",\"matched\":false,\"available\":null,\"binding_entry_present\":true,\"device_present\":true,\"capability_present\":false") != NULL);
    inspection.snapshot.items[0].available = false;
    assert(athom_homey_awning_snapshot_json(
        awning_json, sizeof(awning_json), &inspection, &publish_inspection, true,
        PANEL_HOMEY_ALIAS_STORE_VERIFY_ERROR));
    assert(strstr(awning_json, "\"alias\":\"awning_2\",\"matched\":true,\"available\":false") != NULL);
    assert(strstr(awning_json, "_id") == NULL && strstr(awning_json, "digest") == NULL);
    assert(strstr(awning_json, "PRIVATE_DEVICE_FIXTURE") == NULL);
    assert(strstr(awning_json, "PRIVATE_CAPABILITY_FIXTURE") == NULL);
    assert(!athom_homey_awning_snapshot_json(awning_json, 4U, &inspection,
        &publish_inspection, true,
        PANEL_HOMEY_ALIAS_STORE_OK));

    static const struct {
        panel_homey_read_result_t result;
        const char *name;
    } publish_results[] = {
        {PANEL_HOMEY_READ_OK, "ok"},
        {PANEL_HOMEY_READ_NOT_CONFIGURED, "not_configured"},
        {PANEL_HOMEY_READ_NOT_FOUND, "not_found"},
        {PANEL_HOMEY_READ_DUPLICATE, "duplicate"},
        {PANEL_HOMEY_READ_OVERFLOW, "overflow"},
        {PANEL_HOMEY_READ_INVALID, "invalid"},
        {PANEL_HOMEY_READ_STALE, "invalid"},
        {(panel_homey_read_result_t)999, "invalid"},
    };
    for (size_t i = 0U; i < sizeof(publish_results) / sizeof(publish_results[0]); ++i) {
        publish_inspection.result = publish_results[i].result;
        assert(athom_homey_awning_snapshot_json(
            awning_json, sizeof(awning_json), &inspection, &publish_inspection,
            true, PANEL_HOMEY_ALIAS_STORE_OK));
        char expected[64];
        (void)snprintf(expected, sizeof(expected), "\"result\":\"%s\"",
            publish_results[i].name);
        assert(strstr(awning_json, expected) != NULL);
    }
    panel_homey_snapshot_publish_inspection_t publish_before = publish_inspection;
    char json_before[sizeof(awning_json)];
    memcpy(json_before, awning_json, sizeof(awning_json));
    memset(awning_json, 0, sizeof(awning_json));
    assert(athom_homey_awning_snapshot_json(
        awning_json, sizeof(awning_json), &inspection, &publish_inspection,
        true, PANEL_HOMEY_ALIAS_STORE_OK));
    assert(memcmp(&publish_inspection, &publish_before, sizeof(publish_before)) == 0);
    assert(strcmp(awning_json, json_before) == 0);
    athom_token_set_t tokens = {0};
    fill_token(tokens.refresh_token, 700U, 'a');
    char access[1200];
    char rotated[900];
    fill_token(access, 1199U, 'A');
    fill_token(rotated, 899U, 'k');

    assert(athom_token_set_apply_refresh(&tokens, access, NULL, 3600U));
    assert(strlen(tokens.access_token) == 1199U);
    assert(strlen(tokens.refresh_token) == 700U);
    assert(athom_token_set_apply_refresh(&tokens, access, rotated, 7200U));
    assert(strlen(tokens.refresh_token) == 899U);

    athom_homey_list_t list = {0};
    list.count = 3U;
    strcpy(list.items[0].id, "homey-a");
    strcpy(list.items[0].name, "Mamma");
    strcpy(list.items[0].local_url_secure, "https://secure.local");
    strcpy(list.items[0].local_url, "http://local");
    strcpy(list.items[0].remote_url, "https://remote");

    strcpy(list.items[1].id, "homey-b");
    strcpy(list.items[1].name, "Sommarhus");
    strcpy(list.items[1].local_url, "http://local-b");
    strcpy(list.items[1].remote_url, "https://remote-b");

    strcpy(list.items[2].id, "homey-c");
    strcpy(list.items[2].name, "Reserv");
    strcpy(list.items[2].remote_url, "https://remote-c");

    assert(strcmp(athom_homey_preferred_url(&list.items[0]),
                  "https://secure.local") == 0);
    assert(strcmp(athom_homey_preferred_url(&list.items[1]),
                  "http://local-b") == 0);
    assert(strcmp(athom_homey_preferred_url(&list.items[2]),
                  "https://remote-c") == 0);

    athom_homey_t none = {0};
    assert(athom_homey_preferred_url(&none) == NULL);
    assert(athom_homey_find_exact(&list, "homey-b") == &list.items[1]);
    assert(athom_homey_find_exact(&list, "HOMEY-B") == NULL);

    char status[4096];
    assert(athom_homey_status_json(
        status, sizeof(status), "ready", &list, &list.items[0], 5U, 12U));
    assert(strstr(status, "\"name\":\"Mamma\"") != NULL);
    assert(strstr(status, "access_token") == NULL);
    assert(strstr(status, "refresh_token") == NULL);
    assert(strstr(status, "client_secret") == NULL);
    assert(strstr(status, "session_token") == NULL);

    puts("ATHOM_CLOUD_MODEL_HOST_TEST PASS");
    return 0;
}
