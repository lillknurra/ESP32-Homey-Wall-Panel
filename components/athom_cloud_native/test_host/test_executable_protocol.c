#include "athom_protocol.h"
#include "athom_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void token_tests(void) {
    athom_credentials_t credentials = {0};
    const char *ok =
        "{\"token_type\":\"bearer\",\"access_token\":\"a1\","
        "\"refresh_token\":\"r2\",\"expires_in\":\"3660\"}";
    assert(athom_parse_token_json(ok, strlen(ok), 1000, &credentials) == ATHOM_OK);
    assert(strcmp(credentials.access_token, "a1") == 0);
    assert(strcmp(credentials.refresh_token, "r2") == 0);
    assert(credentials.expires_at_epoch_s == 4660);

    const char *rotated =
        "{\"token_type\":\"bearer\",\"access_token\":\"a2\","
        "\"refresh_token\":\"r3\",\"expires_in\":3600}";
    assert(athom_parse_token_json(rotated, strlen(rotated), 2000, &credentials) == ATHOM_OK);
    assert(strcmp(credentials.refresh_token, "r3") == 0);

    const char *missing = "{\"token_type\":\"bearer\",\"access_token\":\"x\",\"expires_in\":3600}";
    assert(athom_parse_token_json(missing, strlen(missing), 0, &credentials) == ATHOM_ERR_RESPONSE);
    const char *bad_expiry =
        "{\"token_type\":\"bearer\",\"access_token\":\"x\","
        "\"refresh_token\":\"y\",\"expires_in\":0}";
    assert(athom_parse_token_json(bad_expiry, strlen(bad_expiry), 0, &credentials) == ATHOM_ERR_RESPONSE);
}

static void homey_tests(void) {
    const char *json =
        "{\"homeys\":[{\"_id\":\"h1\",\"name\":\"Home\","
        "\"remoteUrl\":\"https://h1.homey.eu-west-1.homeypro.net\"}]}";
    athom_homey_list_t list;
    athom_homey_connection_t connections[ATHOM_MAX_HOMEYS];
    assert(athom_parse_user_homeys_json(json, strlen(json), &list, connections,
        ATHOM_MAX_HOMEYS) == ATHOM_OK);
    assert(list.count == 1);
    assert(strcmp(list.items[0].id, "h1") == 0);
    assert(strstr(connections[0].remote_url, "homeypro.net"));

    const char *null_url =
        "{\"homeys\":[{\"_id\":\"h1\",\"name\":\"Home\",\"remoteUrl\":null}]}";
    assert(athom_parse_user_homeys_json(null_url, strlen(null_url), &list, connections,
        ATHOM_MAX_HOMEYS) == ATHOM_ERR_RESPONSE);

    char too_many[8192] = "{\"homeys\":[";
    for (int i = 0; i < 17; ++i) {
        char row[256];
        snprintf(row, sizeof(row),
            "%s{\"_id\":\"h%d\",\"name\":\"H\",\"remoteUrl\":\"https://h.homeypro.net\"}",
            i ? "," : "", i);
        strncat(too_many, row, sizeof(too_many) - strlen(too_many) - 1);
    }
    strncat(too_many, "]}", sizeof(too_many) - strlen(too_many) - 1);
    assert(athom_parse_user_homeys_json(too_many, strlen(too_many), &list, connections,
        ATHOM_MAX_HOMEYS) == ATHOM_ERR_RESPONSE);
}

static void session_and_inventory_tests(void) {
    char token[64];
    assert(athom_parse_json_string("\"delegation.jwt\"", 16, token, sizeof(token)) == ATHOM_OK);
    assert(strcmp(token, "delegation.jwt") == 0);
    assert(athom_parse_json_string("null", 4, token, sizeof(token)) == ATHOM_ERR_RESPONSE);

    size_t count = 0;
    assert(athom_count_top_level_members("{\"a\":{},\"b\":{}}", 15, &count) == ATHOM_OK);
    assert(count == 2);
    assert(athom_count_top_level_members("[1,2,3]", 7, &count) == ATHOM_OK);
    assert(count == 3);
}

static void evidence_tests(void) {
    char pseudo[32];
    assert(athom_pseudonymize("real-homey-id", pseudo, sizeof(pseudo)) == ATHOM_OK);
    assert(strncmp(pseudo, "id-", 3) == 0);
    assert(athom_text_is_sanitized("homey=id-123 count=4"));
    assert(!athom_text_is_sanitized("access_token=secret"));
    assert(!athom_text_is_sanitized("https://abc.homeypro.net"));
}

int main(void) {
    token_tests();
    homey_tests();
    session_and_inventory_tests();
    evidence_tests();
    athom_runtime_status_t runtime;
    assert(athom_runtime_start(&runtime) == ATHOM_OK);
    assert(!runtime.live_gate_enabled);
    assert(athom_runtime_set_live_gate(&runtime, true) == ATHOM_OK);
    assert(runtime.live_gate_enabled);
    puts("PASS: executable Athom protocol host tests");
    return 0;
}
