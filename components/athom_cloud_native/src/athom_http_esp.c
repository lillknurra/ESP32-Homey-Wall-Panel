#include "athom_http_esp.h"
#include "athom_endpoints.h"
#include "athom_protocol.h"
#include "athom_redaction.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef ESP_PLATFORM
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#endif

typedef struct {
    athom_http_executor_fn executor;
    void *executor_context;
    athom_homey_list_t cached_homeys;
    athom_homey_connection_t connections[ATHOM_MAX_HOMEYS];
    char session_token[ATHOM_MAX_SESSION_TOKEN];
    char session_homey_id[ATHOM_MAX_HOMEY_ID];
} athom_http_esp_context_t;

static athom_http_esp_context_t g_context;

static void secure_free_response(athom_http_response_t *response) {
    if (!response) return;
    if (response->data) {
        athom_secure_zero(response->data, response->capacity);
        free(response->data);
    }
    memset(response, 0, sizeof(*response));
}

void athom_http_response_free(athom_http_response_t *response) {
    secure_free_response(response);
}

athom_status_t athom_url_encode(
    const char *input, char *output, size_t output_size) {
    static const char hex[] = "0123456789ABCDEF";
    if (!input || !output || output_size == 0) return ATHOM_ERR_ARGUMENT;
    size_t used = 0;
    for (const unsigned char *p = (const unsigned char *)input; *p; ++p) {
        bool unreserved = isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~';
        size_t required = unreserved ? 1U : 3U;
        if (used + required + 1U > output_size) return ATHOM_ERR_RESPONSE;
        if (unreserved) output[used++] = (char)*p;
        else {
            output[used++] = '%';
            output[used++] = hex[*p >> 4];
            output[used++] = hex[*p & 0x0F];
        }
    }
    output[used] = '\0';
    return ATHOM_OK;
}

athom_status_t athom_http_validate_response_size(size_t current, size_t incoming) {
    if (current > ATHOM_HTTP_MAX_RESPONSE_BYTES ||
        incoming > ATHOM_HTTP_MAX_RESPONSE_BYTES - current) return ATHOM_ERR_RESPONSE;
    return ATHOM_OK;
}

static bool host_ends_with(const char *host, size_t host_len, const char *suffix) {
    size_t suffix_len = strlen(suffix);
    return host_len >= suffix_len &&
        memcmp(host + host_len - suffix_len, suffix, suffix_len) == 0;
}

bool athom_http_url_is_official(const char *url) {
    static const char prefix[] = "https://";
    if (!url || strncmp(url, prefix, sizeof(prefix) - 1) != 0) return false;
    const char *host = url + sizeof(prefix) - 1;
    const char *path = strchr(host, '/');
    size_t host_len = path ? (size_t)(path - host) : strlen(host);
    if (host_len == 0 || memchr(host, '@', host_len) || memchr(host, ':', host_len)) return false;
    if (host_len == strlen("api.athom.com") && memcmp(host, "api.athom.com", host_len) == 0)
        return true;
    return host_ends_with(host, host_len, ".homeypro.net") ||
           host_ends_with(host, host_len, ".connect.athom.com") ||
           host_ends_with(host, host_len, ".homey.homeylocal.com");
}

#ifdef ESP_PLATFORM
static athom_status_t append_response(
    athom_http_response_t *response, const void *data, size_t size) {
    if (!response || (!data && size)) return ATHOM_ERR_ARGUMENT;
    if (athom_http_validate_response_size(response->length, size) != ATHOM_OK)
        return ATHOM_ERR_RESPONSE;
    size_t required = response->length + size + 1U;
    if (required > response->capacity) {
        size_t next = response->capacity ? response->capacity * 2U : 2048U;
        while (next < required && next < ATHOM_HTTP_MAX_RESPONSE_BYTES + 1U) next *= 2U;
        if (next > ATHOM_HTTP_MAX_RESPONSE_BYTES + 1U) next = ATHOM_HTTP_MAX_RESPONSE_BYTES + 1U;
        if (next < required) return ATHOM_ERR_RESPONSE;
        char *grown = realloc(response->data, next);
        if (!grown) return ATHOM_ERR_TRANSPORT;
        response->data = grown;
        response->capacity = next;
    }
    if (size) memcpy(response->data + response->length, data, size);
    response->length += size;
    response->data[response->length] = '\0';
    return ATHOM_OK;
}

typedef struct { athom_http_response_t *response; bool overflow; } event_context_t;

static esp_err_t on_http_event(esp_http_client_event_t *event) {
    event_context_t *ctx = event ? event->user_data : NULL;
    if (!ctx || !ctx->response) return ESP_FAIL;
    if (event->event_id == HTTP_EVENT_ON_DATA && event->data_len > 0) {
        if (append_response(ctx->response, event->data, (size_t)event->data_len) != ATHOM_OK) {
            ctx->overflow = true;
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

static esp_http_client_method_t method_from_text(const char *method) {
    if (strcmp(method, "GET") == 0) return HTTP_METHOD_GET;
    if (strcmp(method, "POST") == 0) return HTTP_METHOD_POST;
    return HTTP_METHOD_MAX;
}

static athom_status_t esp_executor(
    void *executor_context, const char *method, const char *url,
    const char *authorization, const char *content_type, const char *body,
    athom_http_response_t *response) {
    (void)executor_context;
    if (!method || !url || !response || !athom_http_url_is_official(url))
        return ATHOM_ERR_ARGUMENT;
    esp_http_client_method_t native_method = method_from_text(method);
    if (native_method == HTTP_METHOD_MAX) return ATHOM_ERR_ARGUMENT;
    memset(response, 0, sizeof(*response));
    event_context_t events = {.response = response, .overflow = false};
    esp_http_client_config_t config = {
        .url = url,
        .method = native_method,
        .event_handler = on_http_event,
        .user_data = &events,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
        .buffer_size = 2048,
        .buffer_size_tx = 2048,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ATHOM_ERR_TRANSPORT;
    if (authorization) esp_http_client_set_header(client, "Authorization", authorization);
    if (content_type) esp_http_client_set_header(client, "Content-Type", content_type);
    if (body) esp_http_client_set_post_field(client, body, (int)strlen(body));
    esp_err_t err = esp_http_client_perform(client);
    response->status_code = esp_http_client_get_status_code(client);
    response->tls_verified = err == ESP_OK;
    esp_http_client_cleanup(client);
    if (events.overflow) return ATHOM_ERR_RESPONSE;
    if (err != ESP_OK) return ATHOM_ERR_TRANSPORT;
    return ATHOM_OK;
}
#endif

static const char BASE64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static athom_status_t base64_encode(
    const unsigned char *input, size_t length, char *out, size_t out_size) {
    size_t needed = ((length + 2U) / 3U) * 4U + 1U;
    if (!input || !out || out_size < needed) return ATHOM_ERR_RESPONSE;
    size_t i = 0, o = 0;
    while (i < length) {
        size_t start = i;
        unsigned a = input[i++];
        unsigned b = i < length ? input[i++] : 0U;
        unsigned c = i < length ? input[i++] : 0U;
        size_t remaining = length - start;
        unsigned triple = (a << 16) | (b << 8) | c;
        out[o++] = BASE64[(triple >> 18) & 63U];
        out[o++] = BASE64[(triple >> 12) & 63U];
        out[o++] = remaining > 1U ? BASE64[(triple >> 6) & 63U] : '=';
        out[o++] = remaining > 2U ? BASE64[triple & 63U] : '=';
    }
    out[o] = '\0';
    return ATHOM_OK;
}

static athom_status_t basic_header(
    const char *client_id, const char *client_secret, char *output, size_t output_size) {
    if (!client_id || !client_secret || !output) return ATHOM_ERR_ARGUMENT;
    char raw[ATHOM_MAX_CLIENT_ID + ATHOM_MAX_CLIENT_SECRET + 2U];
    int written = snprintf(raw, sizeof(raw), "%s:%s", client_id, client_secret);
    if (written < 0 || (size_t)written >= sizeof(raw)) return ATHOM_ERR_ARGUMENT;
    char encoded[512];
    athom_status_t status = base64_encode((const unsigned char *)raw, strlen(raw), encoded, sizeof(encoded));
    athom_secure_zero(raw, sizeof(raw));
    if (status != ATHOM_OK || strlen(encoded) + 7U > output_size) {
        athom_secure_zero(encoded, sizeof(encoded));
        return ATHOM_ERR_RESPONSE;
    }
    snprintf(output, output_size, "Basic %s", encoded);
    athom_secure_zero(encoded, sizeof(encoded));
    return ATHOM_OK;
}

static athom_status_t bearer_header(const char *token, char *output, size_t output_size) {
    if (!token || !token[0] || !output || strlen(token) + 8U > output_size)
        return ATHOM_ERR_ARGUMENT;
    snprintf(output, output_size, "Bearer %s", token);
    return ATHOM_OK;
}

static athom_status_t execute(
    athom_http_esp_context_t *ctx, const char *method, const char *url,
    const char *authorization, const char *content_type, const char *body,
    athom_http_response_t *response) {
    if (!ctx || !ctx->executor || !athom_http_url_is_official(url)) return ATHOM_ERR_TRANSPORT;
    athom_status_t status = ctx->executor(ctx->executor_context, method, url,
        authorization, content_type, body, response);
    if (status != ATHOM_OK) return status;
    if (!response->tls_verified) return ATHOM_ERR_TRANSPORT;
    if (response->status_code < 200 || response->status_code >= 300) return ATHOM_ERR_HTTP_STATUS;
    if (response->length > ATHOM_HTTP_MAX_RESPONSE_BYTES) return ATHOM_ERR_RESPONSE;
    return ATHOM_OK;
}

static athom_status_t token_exchange(
    athom_http_esp_context_t *ctx, const athom_credentials_t *client,
    const char *grant_type, const char *grant_value_name, const char *grant_value,
    athom_credentials_t *updated) {
    if (!ctx || !client || !grant_type || !grant_value_name || !grant_value || !updated)
        return ATHOM_ERR_ARGUMENT;
    char encoded_value[ATHOM_HTTP_MAX_FORM_BYTES / 2U];
    if (athom_url_encode(grant_value, encoded_value, sizeof(encoded_value)) != ATHOM_OK)
        return ATHOM_ERR_RESPONSE;
    char body[ATHOM_HTTP_MAX_FORM_BYTES];
    int n = snprintf(body, sizeof(body), "grant_type=%s&%s=%s", grant_type, grant_value_name, encoded_value);
    athom_secure_zero(encoded_value, sizeof(encoded_value));
    if (n < 0 || (size_t)n >= sizeof(body)) { athom_secure_zero(body, sizeof(body)); return ATHOM_ERR_SESSION_RESPONSE; }
    char auth[520];
    athom_status_t status = basic_header(client->client_id, client->client_secret, auth, sizeof(auth));
    if (status != ATHOM_OK) { athom_secure_zero(body, sizeof(body)); return status; }
    athom_http_response_t response = {0};
    const athom_endpoint_definition_t *endpoint = athom_endpoint_get(ATHOM_ENDPOINT_TOKEN);
    status = execute(ctx, "POST", endpoint->path, auth,
        "application/x-www-form-urlencoded", body, &response);
    athom_secure_zero(auth, sizeof(auth));
    athom_secure_zero(body, sizeof(body));
    if (status == ATHOM_OK) {
        athom_credentials_t candidate = *client;
        status = athom_parse_token_json(response.data, response.length, (int64_t)time(NULL), &candidate);
        if (status == ATHOM_OK) *updated = candidate;
        athom_secure_zero(&candidate, sizeof(candidate));
    }
    secure_free_response(&response);
    return status;
}

static athom_status_t exchange_authorization_code(
    athom_http_transport_t *transport, const athom_credentials_t *client,
    const char *authorization_code, athom_credentials_t *updated) {
    if (!transport || !transport->context || !authorization_code || !authorization_code[0])
        return ATHOM_ERR_ARGUMENT;
    return token_exchange(transport->context, client, "authorization_code",
        "authorization_code", authorization_code, updated);
}

static athom_status_t refresh_access_token(
    athom_http_transport_t *transport, const athom_credentials_t *current,
    athom_credentials_t *updated) {
    if (!transport || !transport->context || !current || !current->refresh_token[0])
        return ATHOM_ERR_ARGUMENT;
    athom_credentials_t candidate = *current;
    athom_status_t status = token_exchange(transport->context, current, "refresh_token",
        "refresh_token", current->refresh_token, &candidate);
    if (status == ATHOM_OK) *updated = candidate;
    athom_secure_zero(&candidate, sizeof(candidate));
    return status;
}

static athom_status_t list_homeys(
    athom_http_transport_t *transport, const char *access_token,
    athom_homey_list_t *out) {
    if (!transport || !transport->context || !access_token || !out) return ATHOM_ERR_ARGUMENT;
    athom_http_esp_context_t *ctx = transport->context;
    char auth[ATHOM_MAX_TOKEN + 8U];
    athom_status_t status = bearer_header(access_token, auth, sizeof(auth));
    if (status != ATHOM_OK) return status;
    athom_http_response_t response = {0};
    const athom_endpoint_definition_t *endpoint = athom_endpoint_get(ATHOM_ENDPOINT_USER_ME);
    status = execute(ctx, "GET", endpoint->path, auth, NULL, NULL, &response);
    athom_secure_zero(auth, sizeof(auth));
    if (status == ATHOM_OK) {
        athom_homey_list_t parsed;
        athom_homey_connection_t connections[ATHOM_MAX_HOMEYS];
        status = athom_parse_user_homeys_json(response.data, response.length, &parsed,
            connections, ATHOM_MAX_HOMEYS);
        if (status == ATHOM_OK) {
            for (size_t i = 0; i < parsed.count; ++i) {
                if (!connections[i].remote_url[0] || !athom_http_url_is_official(connections[i].remote_url)) {
                    status = ATHOM_ERR_RESPONSE;
                    break;
                }
            }
        }
        if (status == ATHOM_OK) {
            ctx->cached_homeys = parsed;
            memcpy(ctx->connections, connections, sizeof(connections));
            *out = parsed;
        }
        athom_secure_zero(connections, sizeof(connections));
    }
    secure_free_response(&response);
    return status;
}

static int cached_homey_index(athom_http_esp_context_t *ctx, const char *homey_id) {
    if (!ctx || !homey_id) return -1;
    for (size_t i = 0; i < ctx->cached_homeys.count; ++i)
        if (strcmp(ctx->cached_homeys.items[i].id, homey_id) == 0) return (int)i;
    return -1;
}

static athom_status_t build_remote_url(
    const char *base, const char *path, char *out, size_t out_size) {
    if (!base || !path || !out || !athom_http_url_is_official(base) || path[0] != '/')
        return ATHOM_ERR_ARGUMENT;
    size_t base_len = strlen(base);
    while (base_len > 0 && base[base_len - 1] == '/') --base_len;
    int n = snprintf(out, out_size, "%.*s%s", (int)base_len, base, path);
    return n < 0 || (size_t)n >= out_size ? ATHOM_ERR_RESPONSE : ATHOM_OK;
}

static athom_status_t create_session(
    athom_http_esp_context_t *ctx, const char *access_token,
    const char *homey_id, const char *remote_url) {
    char cloud_auth[ATHOM_MAX_TOKEN + 8U];
    athom_status_t status = bearer_header(access_token, cloud_auth, sizeof(cloud_auth));
    if (status != ATHOM_OK) return status;
    athom_http_response_t delegation = {0};
    const athom_endpoint_definition_t *delegation_endpoint =
        athom_endpoint_get(ATHOM_ENDPOINT_DELEGATION_TOKEN);
    status = execute(ctx, "POST", delegation_endpoint->path, cloud_auth, NULL, NULL, &delegation);
    athom_secure_zero(cloud_auth, sizeof(cloud_auth));
    if (status != ATHOM_OK) { secure_free_response(&delegation); return status; }
    char delegation_token[ATHOM_MAX_DELEGATION_TOKEN] = {0};
    status = athom_parse_json_string(delegation.data, delegation.length,
        delegation_token, sizeof(delegation_token));
    secure_free_response(&delegation);
    if (status != ATHOM_OK) { athom_secure_zero(delegation_token, sizeof(delegation_token)); return status; }
    char escaped_delegation[ATHOM_MAX_DELEGATION_TOKEN * 2U] = {0};
    status = athom_json_escape_string(delegation_token, escaped_delegation, sizeof(escaped_delegation));
    athom_secure_zero(delegation_token, sizeof(delegation_token));
    if (status != ATHOM_OK) { athom_secure_zero(escaped_delegation, sizeof(escaped_delegation)); return status; }
    char body[ATHOM_MAX_DELEGATION_TOKEN * 2U + 32U] = {0};
    int n = snprintf(body, sizeof(body), "{\"token\":\"%s\"}", escaped_delegation);
    athom_secure_zero(escaped_delegation, sizeof(escaped_delegation));
    if (n < 0 || (size_t)n >= sizeof(body)) return ATHOM_ERR_RESPONSE;
    char login_url[ATHOM_HTTP_MAX_URL_BYTES];
    status = build_remote_url(remote_url,
        athom_endpoint_get(ATHOM_ENDPOINT_HOMEY_LOGIN)->path,
        login_url, sizeof(login_url));
    if (status != ATHOM_OK) { athom_secure_zero(body, sizeof(body)); return status; }
    athom_http_response_t login = {0};
    status = execute(ctx, "POST", login_url, NULL, "application/json", body, &login);
    athom_secure_zero(body, sizeof(body));
    if (status == ATHOM_OK) {
        char session[ATHOM_MAX_SESSION_TOKEN] = {0};
        status = athom_parse_json_string(login.data, login.length, session, sizeof(session));
        if (status == ATHOM_OK) {
            athom_secure_zero(ctx->session_token, sizeof(ctx->session_token));
            snprintf(ctx->session_token, sizeof(ctx->session_token), "%s", session);
            athom_secure_zero(ctx->session_homey_id, sizeof(ctx->session_homey_id));
            snprintf(ctx->session_homey_id, sizeof(ctx->session_homey_id), "%s", homey_id);
        }
        athom_secure_zero(session, sizeof(session));
    }
    secure_free_response(&login);
    return status;
}

static athom_status_t inventory_get(
    athom_http_esp_context_t *ctx, const char *remote_url, const char *path,
    size_t *count, bool allow_reauth, const char *access_token, const char *homey_id) {
    char url[ATHOM_HTTP_MAX_URL_BYTES];
    athom_status_t status = build_remote_url(remote_url, path, url, sizeof(url));
    if (status != ATHOM_OK) return status;
    char auth[ATHOM_MAX_SESSION_TOKEN + 8U];
    status = bearer_header(ctx->session_token, auth, sizeof(auth));
    if (status != ATHOM_OK) return status;
    athom_http_response_t response = {0};
    status = ctx->executor(ctx->executor_context, "GET", url, auth, NULL, NULL, &response);
    athom_secure_zero(auth, sizeof(auth));
    if (status == ATHOM_OK && response.tls_verified && response.status_code == 401 && allow_reauth) {
        secure_free_response(&response);
        status = create_session(ctx, access_token, homey_id, remote_url);
        if (status != ATHOM_OK) return status;
        return inventory_get(ctx, remote_url, path, count, false, access_token, homey_id);
    }
    if (status != ATHOM_OK || !response.tls_verified || response.status_code < 200 || response.status_code >= 300) {
        secure_free_response(&response);
        return status == ATHOM_OK ? ATHOM_ERR_HTTP_STATUS : status;
    }
    if (count) status = athom_count_top_level_members(response.data, response.length, count);
    secure_free_response(&response);
    return status;
}

static athom_status_t read_inventory(
    athom_http_transport_t *transport, const char *access_token,
    const char *homey_id, athom_discovery_strategy_t strategy,
    athom_inventory_summary_t *out) {
    if (!transport || !transport->context || !access_token || !homey_id || !out)
        return ATHOM_ERR_ARGUMENT;
    if (strategy != ATHOM_DISCOVERY_CLOUD && strategy != ATHOM_DISCOVERY_REMOTE_FORWARDED)
        return ATHOM_ERR_ARGUMENT;
    athom_http_esp_context_t *ctx = transport->context;
    int index = cached_homey_index(ctx, homey_id);
    if (index < 0) return ATHOM_ERR_HOMEY_NOT_FOUND;
    const char *remote_url = ctx->connections[index].remote_url;
    if (ctx->session_homey_id[0] && strcmp(ctx->session_homey_id, homey_id) != 0) {
        athom_secure_zero(ctx->session_token, sizeof(ctx->session_token));
        athom_secure_zero(ctx->session_homey_id, sizeof(ctx->session_homey_id));
    }
    athom_status_t status = create_session(ctx, access_token, homey_id, remote_url);
    if (status != ATHOM_OK) return status;
    memset(out, 0, sizeof(*out));
    const athom_endpoint_id_t ids[] = {
        ATHOM_ENDPOINT_SYSTEM_INFO, ATHOM_ENDPOINT_ZONES, ATHOM_ENDPOINT_DEVICES,
        ATHOM_ENDPOINT_FLOWS, ATHOM_ENDPOINT_ADVANCED_FLOWS, ATHOM_ENDPOINT_MOODS
    };
    size_t *counts[] = {NULL, &out->zones, &out->devices, &out->flows,
        &out->advanced_flows, &out->moods};
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        const athom_endpoint_definition_t *endpoint = athom_endpoint_get(ids[i]);
        if (!endpoint || !endpoint->read_only || strcmp(endpoint->method, "GET") != 0)
            return ATHOM_ERR_RESPONSE;
        status = inventory_get(ctx, remote_url, endpoint->path, counts[i], true,
            access_token, homey_id);
        if (status != ATHOM_OK) return status;
    }
    return ATHOM_OK;
}

static void invalidate_session(athom_http_transport_t *transport) {
    if (!transport || !transport->context) return;
    athom_http_esp_context_t *ctx = transport->context;
    athom_secure_zero(ctx->session_token, sizeof(ctx->session_token));
    athom_secure_zero(ctx->session_homey_id, sizeof(ctx->session_homey_id));
}

static const athom_http_transport_vtable_t VTABLE = {
    .exchange_authorization_code = exchange_authorization_code,
    .refresh_access_token = refresh_access_token,
    .list_homeys = list_homeys,
    .read_inventory = read_inventory,
    .invalidate_session = invalidate_session,
};

athom_status_t athom_http_esp_init(athom_http_transport_t *transport) {
    if (!transport) return ATHOM_ERR_ARGUMENT;
    memset(&g_context, 0, sizeof(g_context));
#ifdef ESP_PLATFORM
    g_context.executor = esp_executor;
#endif
    transport->vtable = &VTABLE;
    transport->context = &g_context;
    return ATHOM_OK;
}

athom_status_t athom_http_esp_set_executor(
    athom_http_transport_t *transport, athom_http_executor_fn executor,
    void *executor_context) {
    if (!transport || transport->vtable != &VTABLE || !transport->context || !executor)
        return ATHOM_ERR_ARGUMENT;
    athom_http_esp_context_t *ctx = transport->context;
    ctx->executor = executor;
    ctx->executor_context = executor_context;
    return ATHOM_OK;
}
