#include "athom_endpoints.h"
#include "athom_http_esp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    char encoded[128];
    assert(athom_url_encode("a b&c=/", encoded, sizeof(encoded)) == ATHOM_OK);
    assert(strcmp(encoded, "a%20b%26c%3D%2F") == 0);
    assert(athom_url_encode("x", encoded, 1) == ATHOM_ERR_RESPONSE);

    assert(athom_http_validate_response_size(0, ATHOM_HTTP_MAX_RESPONSE_BYTES) == ATHOM_OK);
    assert(athom_http_validate_response_size(
        ATHOM_HTTP_MAX_RESPONSE_BYTES, 1) == ATHOM_ERR_RESPONSE);

    assert(athom_http_url_is_official("https://api.athom.com/oauth2/token"));
    assert(athom_http_url_is_official("https://id.homeypro.net/api/manager/system/"));
    assert(!athom_http_url_is_official("https://evil.example/api"));

    assert(athom_endpoint_is_allowed("GET", "/api/manager/devices/device"));
    assert(athom_endpoint_is_allowed("GET", "/api/manager/zones/zone"));
    assert(!athom_endpoint_is_allowed("PUT", "/api/manager/devices/device/x/capability/onoff"));
    assert(athom_endpoint_is_mutation("POST", "/api/manager/flow/flow/x/trigger"));

    const athom_endpoint_definition_t *moods =
        athom_endpoint_get(ATHOM_ENDPOINT_MOODS);
    assert(moods && moods->read_only);
    assert(strcmp(moods->scope, "homey.mood.readonly") == 0);

    puts("PASS: Athom endpoint and transport contract");
    return 0;
}
