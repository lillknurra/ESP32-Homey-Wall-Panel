#include "athom_provisioning_portal.h"
#include "athom_endpoints.h"
#include "athom_homey_client.h"
#include "athom_http_esp.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef ESP_PLATFORM
#include "esp_http_server.h"

static httpd_handle_t server;

static athom_provisioning_portal_t *portal_from(httpd_req_t *req) {
    return req ? (athom_provisioning_portal_t *)req->user_ctx : NULL;
}

static esp_err_t send_json(httpd_req_t *req, const char *status, const char *json) {
    if (status) httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t read_body(httpd_req_t *req, char *body, size_t size) {
    if (!req || !body || size == 0 || req->content_len <= 0 || (size_t)req->content_len >= size)
        return ESP_FAIL;
    int received = httpd_req_recv(req, body, req->content_len);
    if (received != req->content_len) return ESP_FAIL;
    body[received] = '\0';
    return ESP_OK;
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static bool form_value(const char *body, const char *key, char *out, size_t out_size) {
    if (!body || !key || !out || out_size == 0) return false;
    size_t key_len = strlen(key);
    const char *p = body;
    while (*p) {
        if ((p == body || p[-1] == '&') && strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            p += key_len + 1;
            size_t used = 0;
            while (*p && *p != '&') {
                char c = *p++;
                if (c == '+') c = ' ';
                else if (c == '%' && p[0] && p[1]) {
                    int hi = hex_value(p[0]), lo = hex_value(p[1]);
                    if (hi < 0 || lo < 0) return false;
                    c = (char)((hi << 4) | lo);
                    p += 2;
                }
                if (used + 1 >= out_size) return false;
                out[used++] = c;
            }
            out[used] = '\0';
            return used > 0;
        }
        p = strchr(p, '&');
        if (!p) break;
        ++p;
    }
    return false;
}

static bool live_allowed(athom_provisioning_portal_t *portal) {
    return portal && portal->running && portal->explicit_live_authorized;
}

static esp_err_t health(httpd_req_t *req) {
    athom_provisioning_portal_t *portal = portal_from(req);
    char json[160];
    snprintf(json, sizeof(json),
        "{\"status\":\"ready\",\"live_authorized\":%s,\"state\":%d}",
        live_allowed(portal) ? "true" : "false",
        portal && portal->provisioning ? (int)portal->provisioning->state : -1);
    return send_json(req, NULL, json);
}

static esp_err_t authorize_live(httpd_req_t *req) {
    char body[96];
    if (read_body(req, body, sizeof(body)) != ESP_OK || strcmp(body, "confirm=I_UNDERSTAND") != 0)
        return send_json(req, "403 Forbidden", "{\"error\":\"explicit_local_confirmation_required\"}");
    athom_provisioning_portal_t *portal = portal_from(req);
    portal->explicit_live_authorized = true;
    return send_json(req, NULL, "{\"live_authorized\":true}");
}

static esp_err_t configure(httpd_req_t *req) {
    athom_provisioning_portal_t *portal = portal_from(req);
    if (!live_allowed(portal))
        return send_json(req, "403 Forbidden", "{\"error\":\"explicit_local_authorization_required\"}");
    char body[1024], client_id[ATHOM_MAX_CLIENT_ID], client_secret[ATHOM_MAX_CLIENT_SECRET];
    char redirect_uri[ATHOM_MAX_REDIRECT_URI], state[ATHOM_MAX_OAUTH_STATE];
    if (read_body(req, body, sizeof(body)) != ESP_OK ||
        !form_value(body, "client_id", client_id, sizeof(client_id)) ||
        !form_value(body, "client_secret", client_secret, sizeof(client_secret)) ||
        !form_value(body, "redirect_uri", redirect_uri, sizeof(redirect_uri)))
        return send_json(req, "400 Bad Request", "{\"error\":\"invalid_configuration\"}");
    athom_status_t status = athom_provisioning_start(portal->provisioning,
        client_id, client_secret, redirect_uri, (int64_t)time(NULL), state, sizeof(state));
    memset(client_secret, 0, sizeof(client_secret));
    memset(body, 0, sizeof(body));
    if (status != ATHOM_OK)
        return send_json(req, "400 Bad Request", "{\"error\":\"configuration_rejected\"}");
    char e_client[ATHOM_MAX_CLIENT_ID * 3], e_redirect[ATHOM_MAX_REDIRECT_URI * 3], e_state[ATHOM_MAX_OAUTH_STATE * 3];
    if (athom_url_encode(client_id, e_client, sizeof(e_client)) != ATHOM_OK ||
        athom_url_encode(redirect_uri, e_redirect, sizeof(e_redirect)) != ATHOM_OK ||
        athom_url_encode(state, e_state, sizeof(e_state)) != ATHOM_OK)
        return send_json(req, "500 Internal Server Error", "{\"error\":\"url_generation_failed\"}");
    char json[1600];
    int written = snprintf(json, sizeof(json),
        "{\"authorization_url\":\"https://api.athom.com/oauth2/authorise?authorization_type=code&client_id=%s&redirect_uri=%s&state=%s\"}",
        e_client, e_redirect, e_state);
    if (written < 0 || (size_t)written >= sizeof(json))
        return send_json(req, "500 Internal Server Error", "{\"error\":\"authorization_url_too_long\"}");
    return send_json(req, NULL, json);
}

static esp_err_t oauth_callback(httpd_req_t *req) {
    athom_provisioning_portal_t *portal = portal_from(req);
    if (!live_allowed(portal))
        return send_json(req, "403 Forbidden", "{\"error\":\"explicit_local_authorization_required\"}");
    char query[1024], state[ATHOM_MAX_OAUTH_STATE], code[ATHOM_MAX_AUTH_CODE];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "state", state, sizeof(state)) != ESP_OK ||
        httpd_query_key_value(query, "code", code, sizeof(code)) != ESP_OK)
        return send_json(req, "400 Bad Request", "{\"error\":\"missing_code_or_state\"}");
    athom_status_t status = athom_provisioning_submit_code(portal->provisioning,
        state, code, (int64_t)time(NULL));
    memset(code, 0, sizeof(code));
    if (status != ATHOM_OK)
        return send_json(req, "400 Bad Request", "{\"error\":\"oauth_exchange_failed\"}");
    return send_json(req, NULL, "{\"oauth\":\"authorized\"}");
}

static esp_err_t homeys(httpd_req_t *req) {
    athom_provisioning_portal_t *portal = portal_from(req);
    if (!live_allowed(portal))
        return send_json(req, "403 Forbidden", "{\"error\":\"explicit_local_authorization_required\"}");
    athom_homey_list_t list;
    athom_status_t status = athom_homey_list(portal->provisioning->auth, (int64_t)time(NULL), &list);
    if (status != ATHOM_OK) return send_json(req, "502 Bad Gateway", "{\"error\":\"homey_list_failed\"}");
    char json[4096]; size_t used = 0;
    used += (size_t)snprintf(json + used, sizeof(json) - used, "{\"homeys\":[");
    for (size_t i = 0; i < list.count && used + 256 < sizeof(json); ++i) {
        used += (size_t)snprintf(json + used, sizeof(json) - used,
            "%s{\"id\":\"%s\",\"name\":\"%s\",\"online\":%s}",
            i ? "," : "", list.items[i].id, list.items[i].display_name,
            list.items[i].online ? "true" : "false");
    }
    snprintf(json + used, sizeof(json) - used, "]}");
    return send_json(req, NULL, json);
}

static esp_err_t select_homey(httpd_req_t *req) {
    athom_provisioning_portal_t *portal = portal_from(req);
    if (!live_allowed(portal))
        return send_json(req, "403 Forbidden", "{\"error\":\"explicit_local_authorization_required\"}");
    char body[256], homey_id[ATHOM_MAX_HOMEY_ID];
    if (read_body(req, body, sizeof(body)) != ESP_OK ||
        !form_value(body, "homey_id", homey_id, sizeof(homey_id)))
        return send_json(req, "400 Bad Request", "{\"error\":\"missing_homey_id\"}");
    athom_homey_list_t list;
    athom_status_t status = athom_homey_list(portal->provisioning->auth, (int64_t)time(NULL), &list);
    if (status == ATHOM_OK) status = athom_provisioning_select_homey(
        portal->provisioning, homey_id, &list, ATHOM_DISCOVERY_CLOUD);
    if (status != ATHOM_OK)
        return send_json(req, "400 Bad Request", "{\"error\":\"homey_selection_failed\"}");
    return send_json(req, NULL, "{\"homey_selected\":true}");
}


static esp_err_t run_inventory(httpd_req_t *req) {
    athom_provisioning_portal_t *portal = portal_from(req);
    if (!live_allowed(portal))
        return send_json(req, "403 Forbidden", "{\"error\":\"explicit_local_authorization_required\"}");
    athom_inventory_summary_t inventory;
    athom_status_t status = athom_homey_read_inventory(
        portal->provisioning->auth, (int64_t)time(NULL), &inventory);
    if (status != ATHOM_OK)
        return send_json(req, "502 Bad Gateway", "{\"error\":\"inventory_failed\"}");
    char json[256];
    snprintf(json, sizeof(json),
        "{\"inventory\":{\"zones\":%u,\"devices\":%u,\"flows\":%u,\"advanced_flows\":%u,\"moods\":%u}}",
        (unsigned)inventory.zones, (unsigned)inventory.devices,
        (unsigned)inventory.flows, (unsigned)inventory.advanced_flows,
        (unsigned)inventory.moods);
    return send_json(req, NULL, json);
}

static esp_err_t wipe(httpd_req_t *req) {
    athom_provisioning_portal_t *portal = portal_from(req);
    if (!live_allowed(portal))
        return send_json(req, "403 Forbidden", "{\"error\":\"explicit_local_authorization_required\"}");
    athom_status_t status = athom_provisioning_wipe(portal->provisioning);
    portal->explicit_live_authorized = false;
    return status == ATHOM_OK ? send_json(req, NULL, "{\"wiped\":true}") :
        send_json(req, "500 Internal Server Error", "{\"error\":\"wipe_failed\"}");
}

athom_status_t athom_provisioning_portal_start(athom_provisioning_portal_t *portal) {
    if (!portal || !portal->provisioning || !portal->provisioning->auth) return ATHOM_ERR_ARGUMENT;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;
    config.stack_size = 10240;
    if (httpd_start(&server, &config) != ESP_OK) return ATHOM_ERR_TRANSPORT;
    const httpd_uri_t handlers[] = {
        {.uri="/health", .method=HTTP_GET, .handler=health, .user_ctx=portal},
        {.uri="/live/authorize", .method=HTTP_POST, .handler=authorize_live, .user_ctx=portal},
        {.uri="/configure", .method=HTTP_POST, .handler=configure, .user_ctx=portal},
        {.uri="/oauth/callback", .method=HTTP_GET, .handler=oauth_callback, .user_ctx=portal},
        {.uri="/homeys", .method=HTTP_GET, .handler=homeys, .user_ctx=portal},
        {.uri="/homeys/select", .method=HTTP_POST, .handler=select_homey, .user_ctx=portal},
        {.uri="/inventory/run", .method=HTTP_POST, .handler=run_inventory, .user_ctx=portal},
        {.uri="/credentials/wipe", .method=HTTP_POST, .handler=wipe, .user_ctx=portal},
    };
    for (size_t i = 0; i < sizeof(handlers)/sizeof(handlers[0]); ++i)
        if (httpd_register_uri_handler(server, &handlers[i]) != ESP_OK) {
            httpd_stop(server); server = NULL; return ATHOM_ERR_TRANSPORT;
        }
    portal->explicit_live_authorized = false;
    portal->running = true;
    return ATHOM_OK;
}

athom_status_t athom_provisioning_portal_stop(athom_provisioning_portal_t *portal) {
    if (!portal) return ATHOM_ERR_ARGUMENT;
    if (server) { httpd_stop(server); server = NULL; }
    portal->running = false;
    portal->explicit_live_authorized = false;
    return ATHOM_OK;
}

athom_status_t athom_provisioning_portal_authorize_live(
    athom_provisioning_portal_t *portal, bool authorized) {
    if (!portal || !portal->running) return ATHOM_ERR_ARGUMENT;
    portal->explicit_live_authorized = authorized;
    return ATHOM_OK;
}
#else
athom_status_t athom_provisioning_portal_start(athom_provisioning_portal_t *portal) {
    if (!portal || !portal->provisioning) return ATHOM_ERR_ARGUMENT;
    portal->running = true;
    portal->explicit_live_authorized = false;
    return ATHOM_OK;
}
athom_status_t athom_provisioning_portal_stop(athom_provisioning_portal_t *portal) {
    if (!portal) return ATHOM_ERR_ARGUMENT;
    portal->running = false;
    portal->explicit_live_authorized = false;
    return ATHOM_OK;
}
athom_status_t athom_provisioning_portal_authorize_live(
    athom_provisioning_portal_t *portal, bool authorized) {
    if (!portal || !portal->running) return ATHOM_ERR_ARGUMENT;
    portal->explicit_live_authorized = authorized;
    return ATHOM_OK;
}
#endif
