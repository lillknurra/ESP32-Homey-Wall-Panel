#include "athom_protocol.h"
#include "athom_runtime.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void assert_unchanged(const athom_credentials_t *a, const athom_credentials_t *b) {
    assert(memcmp(a, b, sizeof(*a)) == 0);
}

static athom_status_t parse_token_fail(const char *json, int64_t now) {
    athom_credentials_t original = {0};
    snprintf(original.access_token, sizeof(original.access_token), "%s", "synthetic-old-access");
    snprintf(original.refresh_token, sizeof(original.refresh_token), "%s", "synthetic-old-refresh");
    original.expires_at_epoch_s = 1234;
    athom_credentials_t candidate = original;
    athom_status_t s = athom_parse_token_json(json, strlen(json), now, &candidate);
    assert(s != ATHOM_OK);
    assert_unchanged(&original, &candidate);
    return s;
}

static void token_tests(void) {
    athom_credentials_t credentials = {0};
    const char *ok = "{\"token_type\":\"bearer\",\"access_token\":\"a1\",\"refresh_token\":\"r2\",\"expires_in\":3660}";
    assert(athom_parse_token_json(ok, strlen(ok), 1000, &credentials) == ATHOM_OK);
    assert(strcmp(credentials.access_token, "a1") == 0);
    assert(strcmp(credentials.refresh_token, "r2") == 0);
    assert(credentials.expires_at_epoch_s == 4660);
    const char *ok_quoted = "{\"token_type\":\"Bearer\",\"access_token\":\"a2\",\"refresh_token\":\"r3\",\"expires_in\":\"3600\"}";
    assert(athom_parse_token_json(ok_quoted, strlen(ok_quoted), 2000, &credentials) == ATHOM_OK);

    assert(parse_token_fail("{\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":3600}", 0) == ATHOM_ERR_TOKEN_RESPONSE);
    assert(parse_token_fail("{\"token_type\":\"bad\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":3600}", 0) == ATHOM_ERR_TOKEN_RESPONSE);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"\",\"refresh_token\":\"r\",\"expires_in\":3600}", 0) == ATHOM_ERR_TOKEN_RESPONSE);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"\",\"expires_in\":3600}", 0) == ATHOM_ERR_TOKEN_RESPONSE);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"token_type\":\"Bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":3600}", 0) == ATHOM_ERR_JSON_DUPLICATE_KEY);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"access_token\":\"b\",\"refresh_token\":\"r\",\"expires_in\":3600}", 0) == ATHOM_ERR_JSON_DUPLICATE_KEY);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"refresh_token\":\"x\",\"expires_in\":3600}", 0) == ATHOM_ERR_JSON_DUPLICATE_KEY);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":3600,\"expires_in\":3600}", 0) == ATHOM_ERR_JSON_DUPLICATE_KEY);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":59}", 0) == ATHOM_ERR_TOKEN_RESPONSE);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":86401}", 0) == ATHOM_ERR_TOKEN_RESPONSE);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":-1}", 0) == ATHOM_ERR_TOKEN_RESPONSE);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":3600}garbage", 0) == ATHOM_ERR_JSON_MALFORMED);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":3600", 0) == ATHOM_ERR_JSON_MALFORMED);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\\u0041\",\"refresh_token\":\"r\",\"expires_in\":3600}", 0) == ATHOM_ERR_JSON_MALFORMED);
    athom_credentials_t nested_candidate = credentials;
    const char *nested = "{\"nested\":{\"access_token\":\"bad\"},\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":3600}";
    assert(athom_parse_token_json(nested, strlen(nested), 0, &nested_candidate) == ATHOM_OK);
    assert(strcmp(nested_candidate.access_token, "a") == 0);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":999999999999999999999999}", 0) == ATHOM_ERR_VALUE_TOO_LARGE);
    assert(parse_token_fail("{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r\",\"expires_in\":60}", INT64_MAX) == ATHOM_ERR_VALUE_TOO_LARGE);
}

static void homey_tests(void) {
    athom_homey_list_t list = {0};
    athom_homey_connection_t connections[ATHOM_MAX_HOMEYS] = {0};
    const char *one = "{\"homeys\":[{\"_id\":\"h1\",\"name\":\"Home\",\"remoteUrl\":\"https://h1.homeypro.net\"}]}";
    assert(athom_parse_user_homeys_json(one, strlen(one), &list, connections, ATHOM_MAX_HOMEYS) == ATHOM_OK);
    assert(list.count == 1);
    athom_homey_list_t sentinel = list;
    athom_homey_connection_t sentinel_connections[ATHOM_MAX_HOMEYS];
    memcpy(sentinel_connections, connections, sizeof(connections));
    const char *empty = "{\"homeys\":[]}";
    assert(athom_parse_user_homeys_json(empty, strlen(empty), &list, connections, ATHOM_MAX_HOMEYS) == ATHOM_ERR_HOMEY_LIST_EMPTY);
    assert(memcmp(&list, &sentinel, sizeof(list)) == 0);
    assert(memcmp(connections, sentinel_connections, sizeof(connections)) == 0);
    const char *dupe = "{\"homeys\":[{\"_id\":\"a\",\"name\":\"A\",\"remoteUrl\":\"https://a.homeypro.net\"},{\"_id\":\"a\",\"name\":\"B\",\"remoteUrl\":\"https://b.homeypro.net\"}]}";
    assert(athom_parse_user_homeys_json(dupe, strlen(dupe), &list, connections, ATHOM_MAX_HOMEYS) == ATHOM_ERR_HOMEY_DUPLICATE);
    assert(memcmp(&list, &sentinel, sizeof(list)) == 0);
    const char *bad = "{\"homeys\":[{\"_id\":\"\",\"name\":\"A\",\"remoteUrl\":\"https://a.homeypro.net\"}]}";
    assert(athom_parse_user_homeys_json(bad, strlen(bad), &list, connections, ATHOM_MAX_HOMEYS) == ATHOM_ERR_RESPONSE);
    const char *trail = "{\"homeys\":[{\"_id\":\"a\",\"name\":\"A\",\"remoteUrl\":\"https://a.homeypro.net\"}]}x";
    assert(athom_parse_user_homeys_json(trail, strlen(trail), &list, connections, ATHOM_MAX_HOMEYS) == ATHOM_ERR_JSON_MALFORMED);
    char many[16384] = "{\"homeys\":[";
    for (int i = 0; i < 17; ++i) {
        char row[256];
        snprintf(row, sizeof(row), "%s{\"_id\":\"h%d\",\"name\":\"H\",\"remoteUrl\":\"https://h.homeypro.net\"}", i ? "," : "", i);
        strncat(many, row, sizeof(many) - strlen(many) - 1);
    }
    strncat(many, "]}", sizeof(many) - strlen(many) - 1);
    assert(athom_parse_user_homeys_json(many, strlen(many), &list, connections, ATHOM_MAX_HOMEYS) == ATHOM_ERR_VALUE_TOO_LARGE);
}

static void session_and_inventory_tests(void) {
    char token[64] = "unchanged";
    assert(athom_parse_json_string("\"delegation.jwt\"", 16, token, sizeof(token)) == ATHOM_OK);
    assert(strcmp(token, "delegation.jwt") == 0);
    snprintf(token, sizeof(token), "%s", "unchanged");
    assert(athom_parse_json_string("\"bad\" trailing", 14, token, sizeof(token)) == ATHOM_ERR_SESSION_RESPONSE);
    assert(strcmp(token, "unchanged") == 0);
    char escaped[64];
    assert(athom_json_escape_string("a\\\"b\\\\c", escaped, sizeof(escaped)) == ATHOM_OK);
    assert(strcmp(escaped, "a\\\\\\\"b\\\\\\\\c") == 0);
    size_t count = 0;
    assert(athom_count_top_level_members("{\"a\":{},\"b\":{}}", 15, &count) == ATHOM_OK && count == 2);
    assert(athom_count_top_level_members("[1,2,3]x", 8, &count) == ATHOM_ERR_JSON_MALFORMED);
}

static void evidence_tests(void) {
    char pseudo[32];
    assert(athom_pseudonymize("synthetic-homey-id", pseudo, sizeof(pseudo)) == ATHOM_OK);
    assert(athom_text_is_sanitized("homey=id-123 count=4"));
    assert(!athom_text_is_sanitized("access_token=synthetic-secret"));
    assert(!athom_text_is_sanitized("https://synthetic.homeypro.net"));
}

int main(void) {
    token_tests(); homey_tests(); session_and_inventory_tests(); evidence_tests();
    athom_runtime_status_t runtime;
    assert(athom_runtime_start(&runtime) == ATHOM_OK);
    assert(!runtime.live_gate_enabled);
    puts("PASS: executable Athom protocol host tests");
    return 0;
}
