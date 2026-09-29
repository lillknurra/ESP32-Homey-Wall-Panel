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
    inspection.result = PANEL_HOMEY_READ_NOT_FOUND;
    char awning_json[512];
    assert(athom_homey_awning_snapshot_json(
        awning_json, sizeof(awning_json), &inspection, false,
        PANEL_HOMEY_ALIAS_STORE_OK));
    assert(strstr(awning_json, "\"present\":false") != NULL);
    assert(strstr(awning_json, "\"generation_valid\":false") != NULL);
    assert(strstr(awning_json, "\"alias_activation\":\"not_attempted\"") != NULL);
    assert(strstr(awning_json, "\"available\":null") != NULL);

    memset(&inspection, 0, sizeof(inspection));
    inspection.present = true;
    inspection.fresh = false;
    inspection.result = PANEL_HOMEY_READ_STALE;
    inspection.age_ms = 120001U;
    inspection.snapshot.generation = 42U;
    inspection.snapshot.item_count = 1U;
    strcpy(inspection.snapshot.items[0].device_alias, "awning_2");
    strcpy(inspection.snapshot.items[0].capability_alias, "status");
    inspection.snapshot.items[0].available = true;
    assert(athom_homey_awning_snapshot_json(
        awning_json, sizeof(awning_json), &inspection, true,
        PANEL_HOMEY_ALIAS_STORE_VERIFY_ERROR));
    assert(strstr(awning_json, "\"result\":\"stale\"") != NULL);
    assert(strstr(awning_json, "\"generation\":42") != NULL);
    assert(strstr(awning_json, "\"age_ms\":120001") != NULL);
    assert(strstr(awning_json, "\"alias_activation\":\"verify_error\"") != NULL);
    assert(strstr(awning_json, "\"alias\":\"awning_2\",\"matched\":true,\"available\":true") != NULL);
    assert(strstr(awning_json, "_id") == NULL && strstr(awning_json, "digest") == NULL);
    assert(!athom_homey_awning_snapshot_json(awning_json, 4U, &inspection, true,
        PANEL_HOMEY_ALIAS_STORE_OK));
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
