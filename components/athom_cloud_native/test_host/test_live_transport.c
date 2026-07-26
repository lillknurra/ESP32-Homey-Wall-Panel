#include "athom_http_esp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int step; int inventory_calls; } mock_t;

static void response(athom_http_response_t *r, int code, const char *body) {
    r->status_code = code; r->tls_verified = true; r->length = strlen(body);
    r->capacity = r->length + 1; r->data = malloc(r->capacity);
    assert(r->data); memcpy(r->data, body, r->capacity);
}

static athom_status_t mock_execute(void *opaque, const char *method, const char *url,
    const char *authorization, const char *content_type, const char *body,
    athom_http_response_t *out) {
    mock_t *m = opaque;
    assert(strncmp(url, "https://", 8) == 0);
    if (strstr(url, "/oauth2/token")) {
        assert(strcmp(method, "POST") == 0);
        assert(authorization && strncmp(authorization, "Basic ", 6) == 0);
        assert(content_type && strcmp(content_type, "application/x-www-form-urlencoded") == 0);
        assert(body && strstr(body, "grant_type="));
        response(out, 200, "{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r2\",\"expires_in\":\"3660\"}");
    } else if (strstr(url, "/user/me")) {
        assert(authorization && strncmp(authorization, "Bearer ", 7) == 0);
        response(out, 200, "{\"homeys\":[{\"_id\":\"h1\",\"name\":\"Home\",\"remoteUrl\":\"https://h1.homey.eu-west-1.homeypro.net\"}]}");
    } else if (strstr(url, "/delegation/token")) {
        response(out, 200, "\"delegation\"");
    } else if (strstr(url, "/api/manager/users/login")) {
        assert(content_type && strcmp(content_type, "application/json") == 0);
        assert(body && strstr(body, "delegation"));
        response(out, 200, "\"session\"");
    } else {
        assert(authorization && strcmp(authorization, "Bearer session") == 0);
        ++m->inventory_calls;
        response(out, 200, "{}");
    }
    ++m->step;
    return ATHOM_OK;
}

int main(void) {
    athom_http_transport_t transport;
    mock_t mock = {0};
    assert(athom_http_esp_init(&transport) == ATHOM_OK);
    assert(athom_http_esp_set_executor(&transport, mock_execute, &mock) == ATHOM_OK);
    athom_credentials_t client = {0}, updated = {0};
    snprintf(client.client_id, sizeof(client.client_id), "client");
    snprintf(client.client_secret, sizeof(client.client_secret), "secret");
    snprintf(client.redirect_uri, sizeof(client.redirect_uri), "http://device/oauth/callback");
    assert(transport.vtable->exchange_authorization_code(&transport, &client, "code", &updated) == ATHOM_OK);
    assert(strcmp(updated.refresh_token, "r2") == 0);
    athom_homey_list_t list;
    assert(transport.vtable->list_homeys(&transport, updated.access_token, &list) == ATHOM_OK);
    assert(list.count == 1);
    athom_inventory_summary_t inventory;
    assert(transport.vtable->read_inventory(&transport, updated.access_token, "h1", ATHOM_DISCOVERY_CLOUD, &inventory) == ATHOM_OK);
    assert(mock.inventory_calls == 6);
    assert(!athom_http_url_is_official("https://evil.example/?x=.homeypro.net/"));
    assert(!athom_http_url_is_official("https://user@synthetic.homeypro.net/"));
    assert(!athom_http_url_is_official("https://synthetic.homeypro.net:443/"));
    puts("PASS: executable native Athom transport mock chain");
    return 0;
}
