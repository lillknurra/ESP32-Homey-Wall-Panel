#include "athom_cloud_client.h"
#ifdef ESP_PLATFORM

static const char *s_diagnostic_stage = "idle";
static esp_err_t s_diagnostic_error = ESP_OK;
static int s_diagnostic_http_status;

static void diagnostic_set(
    const char *stage,
    esp_err_t error)
{
    s_diagnostic_stage = stage != NULL ? stage : "unknown";
    s_diagnostic_error = error;
    s_diagnostic_http_status = 0;
}

static void diagnostic_set_http(
    const char *stage,
    esp_err_t error,
    int http_status)
{
    s_diagnostic_stage = stage != NULL ? stage : "unknown";
    s_diagnostic_error = error;
    s_diagnostic_http_status = http_status;
}

const char *athom_cloud_diagnostic_stage(void)
{
    return s_diagnostic_stage;
}

esp_err_t athom_cloud_diagnostic_error(void)
{
    return s_diagnostic_error;
}

int athom_cloud_diagnostic_http_status(void)
{
    return s_diagnostic_http_status;
}

#include "athom_oauth_config.h"
#include "athom_oauth_flow.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_tls.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include <errno.h>
#include <time.h>
#include "mbedtls/base64.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ATHOM_TOKEN_URL "https://api.athom.com/oauth2/token"
#define ATHOM_USER_URL "https://api.athom.com/user/me"
#define HTTP_BODY_MAX 65536U
#define HTTP_INVENTORY_BODY_MAX 524288U
#define HTTP_TIMEOUT_MS 15000

static const char *TAG = "athom_cloud";

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    size_t maximum;
    bool overflow;
} response_buffer_t;

static void zero_secure(void *buffer, size_t size)
{
    volatile unsigned char *p = buffer;
    while (size--) *p++ = 0U;
}

static bool copy_optional(char *dst, size_t cap, const cJSON *object, const char *key)
{
    cJSON *value = cJSON_GetObjectItemCaseSensitive((cJSON *)object, key);
    if (!cJSON_IsString(value) || value->valuestring == NULL) {
        dst[0] = '\0';
        return true;
    }
    size_t n = strlen(value->valuestring);
    if (n >= cap) return false;
    memcpy(dst, value->valuestring, n + 1U);
    return true;
}

static esp_err_t event_handler(esp_http_client_event_t *event)
{
    response_buffer_t *buffer = event != NULL ? event->user_data : NULL;
    if (event == NULL || buffer == NULL) return ESP_OK;

    switch (event->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "ATHOM_NET http_event=error");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "ATHOM_NET http_event=connected");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "ATHOM_NET http_event=disconnected");
        break;
    default:
        break;
    }

    if (event->event_id == HTTP_EVENT_ON_DATA &&
        event->data_len > 0) {
        size_t incoming = (size_t)event->data_len;

        if (incoming >= buffer->maximum ||
            buffer->length >
                buffer->maximum - incoming - 1U) {
            buffer->overflow = true;
            return ESP_ERR_NO_MEM;
        }

        size_t required =
            buffer->length + incoming + 1U;

        if (required > buffer->capacity) {
            size_t new_capacity = buffer->capacity;

            while (new_capacity < required) {
                if (new_capacity >= buffer->maximum) {
                    buffer->overflow = true;
                    return ESP_ERR_NO_MEM;
                }

                if (new_capacity >
                    buffer->maximum / 2U) {
                    new_capacity = buffer->maximum;
                } else {
                    new_capacity *= 2U;
                }
            }

            char *resized = realloc(
                buffer->data,
                new_capacity);

            if (resized == NULL) {
                buffer->overflow = true;
                return ESP_ERR_NO_MEM;
            }

            memset(
                resized + buffer->capacity,
                0,
                new_capacity - buffer->capacity);

            buffer->data = resized;
            buffer->capacity = new_capacity;
        }

        memcpy(
            buffer->data + buffer->length,
            event->data,
            incoming);

        buffer->length += incoming;
        buffer->data[buffer->length] = 0;
    }

    return ESP_OK;
}

static esp_err_t http_request_limited(
    const char *url,
    esp_http_client_method_t method,
    const char *authorization,
    const char *content_type,
    const char *body,
    char **response_out,
    int *status_out,
    size_t response_maximum,
    size_t *response_capacity_out)
{
    if (url == NULL ||
        response_out == NULL ||
        status_out == NULL ||
        response_maximum < 2U) {
        return ESP_ERR_INVALID_ARG;
    }

    *response_out = NULL;
    *status_out = 0;

    if (response_capacity_out != NULL) {
        *response_capacity_out = 0U;
    }

    const char *host = "unknown";
    if (strstr(url, "api.athom.com") != NULL) host = "api.athom.com";

    time_t now = time(NULL);
    bool time_reasonable = now >= 1704067200;
    ESP_LOGI(TAG, "ATHOM_NET unix_time=%lld reasonable=%s",
             (long long)now, time_reasonable ? "true" : "false");

    ESP_LOGI(TAG, "ATHOM_NET dns_begin host=%s", host);
    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *resolved = NULL;
    int gai_rc = getaddrinfo(host, NULL, &hints, &resolved);

    if (gai_rc == 0 && resolved != NULL) {
        char address[INET6_ADDRSTRLEN] = {0};
        void *addr_ptr = NULL;

        if (resolved->ai_family == AF_INET) {
            addr_ptr =
                &((struct sockaddr_in *)resolved->ai_addr)->sin_addr;
        } else if (resolved->ai_family == AF_INET6) {
            addr_ptr =
                &((struct sockaddr_in6 *)resolved->ai_addr)->sin6_addr;
        }

        if (addr_ptr != NULL &&
            inet_ntop(
                resolved->ai_family,
                addr_ptr,
                address,
                sizeof(address)) != NULL) {
            ESP_LOGI(
                TAG,
                "ATHOM_NET dns_result=OK address=%s family=%d",
                address,
                resolved->ai_family);
        } else {
            ESP_LOGI(
                TAG,
                "ATHOM_NET dns_result=OK "
                "address=unavailable family=%d",
                resolved->ai_family);
        }

        freeaddrinfo(resolved);
    } else {
        ESP_LOGE(
            TAG,
            "ATHOM_NET dns_result=FAIL gai_rc=%d errno=%d",
            gai_rc,
            errno);

        if (resolved != NULL) {
            freeaddrinfo(resolved);
        }
    }

    size_t initial_capacity = HTTP_BODY_MAX;

    if (initial_capacity > response_maximum) {
        initial_capacity = response_maximum;
    }

    char *response = calloc(1U, initial_capacity);

    if (response == NULL) {
        return ESP_ERR_NO_MEM;
    }

    response_buffer_t buffer = {
        .data = response,
        .length = 0U,
        .capacity = initial_capacity,
        .maximum = response_maximum,
        .overflow = false,
    };

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = event_handler,
        .user_data = &buffer,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 2048,
        .buffer_size_tx = 2048,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (client == NULL) {
        zero_secure(buffer.data, buffer.capacity);
        free(buffer.data);
        return ESP_FAIL;
    }

    esp_http_client_set_method(client, method);

    if (authorization != NULL) {
        esp_http_client_set_header(
            client,
            "Authorization",
            authorization);
    }

    if (content_type != NULL) {
        esp_http_client_set_header(
            client,
            "Content-Type",
            content_type);
    }

    if (body != NULL) {
        esp_http_client_set_post_field(
            client,
            body,
            (int)strlen(body));
    }

    ESP_LOGI(
        TAG,
        "ATHOM_NET perform_begin host=%s timeout_ms=%d",
        host,
        HTTP_TIMEOUT_MS);

    esp_err_t err = esp_http_client_perform(client);
    int http_status =
        esp_http_client_get_status_code(client);

    int tls_error = 0;
    int tls_flags = 0;
    esp_err_t tls_query =
        esp_http_client_get_and_clear_last_tls_error(
            client,
            &tls_error,
            &tls_flags);

    ESP_LOGI(
        TAG,
        "ATHOM_NET perform_end err=%s err_code=0x%x "
        "http_status=%d tls_query=%s tls_error=%d "
        "tls_flags=0x%x",
        esp_err_to_name(err),
        (unsigned int)err,
        http_status,
        esp_err_to_name(tls_query),
        tls_error,
        (unsigned int)tls_flags);

    if (buffer.overflow) {
        ESP_LOGE(
            TAG,
            "ATHOM_NET response_overflow=true "
            "response_bytes=%u capacity=%u maximum=%u",
            (unsigned)buffer.length,
            (unsigned)buffer.capacity,
            (unsigned)buffer.maximum);

        zero_secure(buffer.data, buffer.capacity);
        free(buffer.data);
        buffer.data = NULL;
        err = ESP_ERR_NO_MEM;
    } else if (err == ESP_OK) {
        ESP_LOGI(
            TAG,
            "ATHOM_NET response_overflow=false "
            "response_bytes=%u capacity=%u maximum=%u",
            (unsigned)buffer.length,
            (unsigned)buffer.capacity,
            (unsigned)buffer.maximum);

        *status_out = http_status;
        *response_out = buffer.data;

        if (response_capacity_out != NULL) {
            *response_capacity_out = buffer.capacity;
        }
    } else {
        zero_secure(buffer.data, buffer.capacity);
        free(buffer.data);
        buffer.data = NULL;
    }

    esp_http_client_cleanup(client);
    return err;
}

static esp_err_t http_request(
    const char *url,
    esp_http_client_method_t method,
    const char *authorization,
    const char *content_type,
    const char *body,
    char **response_out,
    int *status_out)
{
    return http_request_limited(
        url,
        method,
        authorization,
        content_type,
        body,
        response_out,
        status_out,
        HTTP_BODY_MAX,
        NULL);
}

static bool url_encode(const char *input, char *output, size_t capacity)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t used = 0U;
    if (input == NULL || output == NULL || capacity == 0U) return false;
    for (size_t i = 0U; input[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)input[i];
        bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                    c == '.' || c == '~';
        if (safe) {
            if (used + 1U >= capacity) return false;
            output[used++] = (char)c;
        } else {
            if (used + 3U >= capacity) return false;
            output[used++] = '%';
            output[used++] = hex[c >> 4];
            output[used++] = hex[c & 0x0FU];
        }
    }
    output[used] = '\0';
    return true;
}

static esp_err_t basic_authorization(
    const athom_oauth_client_config_t *config,
    char *output,
    size_t capacity)
{
    char joined[ATHOM_CLIENT_ID_MAX + ATHOM_CLIENT_SECRET_MAX + 2U];
    int written = snprintf(joined, sizeof(joined), "%s:%s",
                           config->client_id, config->client_secret);
    if (written <= 0 || (size_t)written >= sizeof(joined)) {
        zero_secure(joined, sizeof(joined));
        return ESP_ERR_INVALID_SIZE;
    }

    unsigned char encoded[640];
    size_t encoded_len = 0U;
    int rc = mbedtls_base64_encode(
        encoded, sizeof(encoded) - 1U, &encoded_len,
        (const unsigned char *)joined, (size_t)written);
    zero_secure(joined, sizeof(joined));
    if (rc != 0 || encoded_len + 7U >= capacity) {
        zero_secure(encoded, sizeof(encoded));
        return ESP_ERR_INVALID_SIZE;
    }
    encoded[encoded_len] = '\0';
    written = snprintf(output, capacity, "Basic %s", encoded);
    zero_secure(encoded, sizeof(encoded));
    return written > 0 && (size_t)written < capacity ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

static esp_err_t bearer_authorization(
    const char *token, char *output, size_t capacity)
{
    if (token == NULL || token[0] == '\0') return ESP_ERR_INVALID_ARG;
    int written = snprintf(output, capacity, "Bearer %s", token);
    return written > 0 && (size_t)written < capacity ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

static esp_err_t parse_token_response(
    const char *json,
    athom_token_set_t *tokens,
    bool refresh_flow)
{
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) return ESP_ERR_INVALID_RESPONSE;

    cJSON *access = cJSON_GetObjectItemCaseSensitive(root, "access_token");
    cJSON *refresh = cJSON_GetObjectItemCaseSensitive(root, "refresh_token");
    cJSON *expires = cJSON_GetObjectItemCaseSensitive(root, "expires_in");

    const char *access_value = cJSON_IsString(access) ? access->valuestring : NULL;
    const char *refresh_value = cJSON_IsString(refresh) ? refresh->valuestring : NULL;
    uint32_t expires_value = cJSON_IsNumber(expires) && expires->valuedouble > 0
        ? (uint32_t)expires->valuedouble : 3600U;

    bool ok = athom_token_set_apply_refresh(
        tokens, access_value,
        refresh_flow ? refresh_value : refresh_value,
        expires_value);
    cJSON_Delete(root);
    return ok ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t token_request(
    const char *grant_body,
    athom_token_set_t *tokens,
    bool refresh_flow)
{
    athom_oauth_client_config_t config;
    bool present = false;
    esp_err_t err = athom_oauth_client_config_load(&config, &present);
    if (err != ESP_OK || !present) {
        zero_secure(&config, sizeof(config));
        return err == ESP_OK ? ESP_ERR_NOT_FOUND : err;
    }

    char authorization[768];
    err = basic_authorization(&config, authorization, sizeof(authorization));
    zero_secure(&config, sizeof(config));
    if (err != ESP_OK) return err;

    char *response = NULL;
    int status = 0;

    diagnostic_set("oauth_token_request", ESP_OK);

    err = http_request(
        ATHOM_TOKEN_URL, HTTP_METHOD_POST, authorization,
        "application/x-www-form-urlencoded", grant_body,
        &response, &status);

    zero_secure(authorization, sizeof(authorization));

    if (err != ESP_OK) {
        diagnostic_set("oauth_token_request", err);
        return err;
    }

    diagnostic_set_http("oauth_token_http", ESP_OK, status);

    if (status < 200 || status >= 300) {
        esp_err_t status_error =
            status == 400 || status == 401
                ? ESP_ERR_INVALID_STATE
                : ESP_FAIL;

        diagnostic_set_http(
            "oauth_token_http",
            status_error,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return status_error;
    }

    diagnostic_set("oauth_token_parse", ESP_OK);
    err = parse_token_response(response, tokens, refresh_flow);

    if (err != ESP_OK) {
        diagnostic_set("oauth_token_parse", err);
    }

    zero_secure(response, HTTP_BODY_MAX);
    free(response);
    return err;
}

esp_err_t athom_cloud_exchange_code(
    const char *authorization_code,
    athom_cloud_state_t *state)
{
    if (authorization_code == NULL || state == NULL) return ESP_ERR_INVALID_ARG;

    char encoded_code[ATHOM_OAUTH_CODE_MAX * 3U];
    char encoded_redirect[ATHOM_REDIRECT_URI_MAX * 3U];
    if (!url_encode(authorization_code, encoded_code, sizeof(encoded_code)) ||
        !url_encode(ATHOM_REDIRECT_URI, encoded_redirect, sizeof(encoded_redirect))) {
        zero_secure(encoded_code, sizeof(encoded_code));
        zero_secure(encoded_redirect, sizeof(encoded_redirect));
        return ESP_ERR_INVALID_SIZE;
    }

    char body[2048];
    int written = snprintf(
        body, sizeof(body),
        "grant_type=authorization_code&code=%s&redirect_uri=%s",
        encoded_code, encoded_redirect);
    zero_secure(encoded_code, sizeof(encoded_code));
    zero_secure(encoded_redirect, sizeof(encoded_redirect));
    if (written <= 0 || (size_t)written >= sizeof(body)) {
        zero_secure(body, sizeof(body));
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t err = token_request(body, &state->tokens, false);
    zero_secure(body, sizeof(body));
    if (err == ESP_OK) {
        state->expires_at_s =
            (uint64_t)(esp_timer_get_time() / 1000000LL) +
            state->tokens.expires_in_s;
    }
    return err;
}

esp_err_t athom_cloud_refresh(athom_cloud_state_t *state)
{
    if (state == NULL || state->tokens.refresh_token[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    char encoded_refresh[ATHOM_TOKEN_MAX * 3U];
    if (!url_encode(state->tokens.refresh_token,
                    encoded_refresh, sizeof(encoded_refresh))) {
        zero_secure(encoded_refresh, sizeof(encoded_refresh));
        return ESP_ERR_INVALID_SIZE;
    }

    char *body = malloc(strlen(encoded_refresh) + 64U);
    if (body == NULL) {
        zero_secure(encoded_refresh, sizeof(encoded_refresh));
        return ESP_ERR_NO_MEM;
    }
    snprintf(body, strlen(encoded_refresh) + 64U,
             "grant_type=refresh_token&refresh_token=%s", encoded_refresh);
    zero_secure(encoded_refresh, sizeof(encoded_refresh));

    esp_err_t err = token_request(body, &state->tokens, true);
    zero_secure(body, strlen(body));
    free(body);
    if (err == ESP_OK) {
        state->expires_at_s =
            (uint64_t)(esp_timer_get_time() / 1000000LL) +
            state->tokens.expires_in_s;
    }
    return err;
}

static esp_err_t parse_homeys(const char *json, athom_homey_list_t *list)
{
    diagnostic_set("oauth_user_me_parse", ESP_OK);

    cJSON *root = cJSON_Parse(json);

    if (root == NULL) {
        diagnostic_set(
            "oauth_user_me_parse",
            ESP_ERR_INVALID_RESPONSE);
        return ESP_ERR_INVALID_RESPONSE;
    }

    diagnostic_set("oauth_homey_parse", ESP_OK);
    cJSON *homeys = cJSON_GetObjectItemCaseSensitive(root, "homeys");
    if (homeys == NULL) {
        cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "result");
        if (cJSON_IsObject(result)) {
            homeys = cJSON_GetObjectItemCaseSensitive(result, "homeys");
        }
    }

    memset(list, 0, sizeof(*list));
    if (cJSON_IsArray(homeys)) {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, homeys) {
            if (list->count >= ATHOM_HOMEY_MAX || !cJSON_IsObject(item)) break;
            athom_homey_t *homey = &list->items[list->count];
            if (!copy_optional(homey->id, sizeof(homey->id), item, "_id")) {
                cJSON_Delete(root);
                return ESP_ERR_INVALID_SIZE;
            }
            if (homey->id[0] == '\0' &&
                !copy_optional(homey->id, sizeof(homey->id), item, "id")) {
                cJSON_Delete(root);
                return ESP_ERR_INVALID_SIZE;
            }
            if (!copy_optional(homey->name, sizeof(homey->name), item, "name") ||
                !copy_optional(homey->platform, sizeof(homey->platform), item, "platform") ||
                !copy_optional(homey->local_url_secure, sizeof(homey->local_url_secure), item, "localUrlSecure") ||
                !copy_optional(homey->local_url, sizeof(homey->local_url), item, "localUrl") ||
                !copy_optional(homey->remote_url, sizeof(homey->remote_url), item, "remoteUrl")) {
                cJSON_Delete(root);
                return ESP_ERR_INVALID_SIZE;
            }
            if (homey->id[0] != '\0') list->count++;
        }
    } else if (cJSON_IsObject(homeys)) {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, homeys) {
            if (list->count >= ATHOM_HOMEY_MAX || !cJSON_IsObject(item)) break;
            athom_homey_t *homey = &list->items[list->count];
            if (!copy_optional(homey->id, sizeof(homey->id), item, "_id") ||
                !copy_optional(homey->name, sizeof(homey->name), item, "name") ||
                !copy_optional(homey->platform, sizeof(homey->platform), item, "platform") ||
                !copy_optional(homey->local_url_secure, sizeof(homey->local_url_secure), item, "localUrlSecure") ||
                !copy_optional(homey->local_url, sizeof(homey->local_url), item, "localUrl") ||
                !copy_optional(homey->remote_url, sizeof(homey->remote_url), item, "remoteUrl")) {
                cJSON_Delete(root);
                return ESP_ERR_INVALID_SIZE;
            }
            if (homey->id[0] != '\0') list->count++;
        }
    }
    cJSON_Delete(root);
    return list->count > 0U ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t athom_cloud_fetch_user_homeys(athom_cloud_state_t *state)
{
    if (state == NULL || state->tokens.access_token[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    char authorization[ATHOM_TOKEN_MAX + 16U];
    esp_err_t err = bearer_authorization(
        state->tokens.access_token, authorization, sizeof(authorization));
    if (err != ESP_OK) return err;

    char *response = NULL;
    int status = 0;

    diagnostic_set("oauth_user_me_request", ESP_OK);

    err = http_request(
        ATHOM_USER_URL, HTTP_METHOD_GET, authorization,
        NULL, NULL, &response, &status);

    zero_secure(authorization, sizeof(authorization));

    if (err != ESP_OK) {
        diagnostic_set("oauth_user_me_request", err);
        return err;
    }

    diagnostic_set_http("oauth_user_me_http", ESP_OK, status);

    if (status == 401) {
        diagnostic_set_http(
            "oauth_user_me_http",
            ESP_ERR_INVALID_STATE,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return ESP_ERR_INVALID_STATE;
    }

    if (status < 200 || status >= 300) {
        diagnostic_set_http(
            "oauth_user_me_http",
            ESP_FAIL,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return ESP_FAIL;
    }

    err = parse_homeys(response, &state->homeys);

    if (err != ESP_OK) {
        if (
            athom_cloud_diagnostic_stage() == NULL ||
            strcmp(
                athom_cloud_diagnostic_stage(),
                "oauth_user_me_parse") != 0
        ) {
            diagnostic_set("oauth_homey_parse", err);
        }
    } else {
        diagnostic_set("oauth_complete", ESP_OK);
    }

    zero_secure(response, HTTP_BODY_MAX);
    free(response);
    return err;
}

static esp_err_t parse_token_field(
    const char *json, const char *key, char *out, size_t capacity)
{
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) return ESP_ERR_INVALID_RESPONSE;
    cJSON *value = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsString(value)) {
        cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "result");
        if (cJSON_IsObject(result)) value = cJSON_GetObjectItemCaseSensitive(result, key);
    }
    if (!cJSON_IsString(value) || value->valuestring == NULL ||
        strlen(value->valuestring) >= capacity) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }
    strcpy(out, value->valuestring);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t parse_json_string_token(
    const char *json,
    char *out,
    size_t capacity)
{
    if (json == NULL || out == NULL || capacity == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (!cJSON_IsString(root) ||
        root->valuestring == NULL ||
        root->valuestring[0] == 0) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    size_t token_length = strlen(root->valuestring);

    if (token_length >= capacity) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(out, root->valuestring, token_length + 1U);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t delegation_token(
    const char *access_token, char *out, size_t capacity)
{
    char authorization[ATHOM_TOKEN_MAX + 16U];
    esp_err_t err = bearer_authorization(access_token, authorization, sizeof(authorization));
    if (err != ESP_OK) return err;
    char *response = NULL;
    int status = 0;
    diagnostic_set("delegation_request", ESP_OK);

    err = http_request(
        "https://api.athom.com/delegation/token?audience=homey",
        HTTP_METHOD_POST,
        authorization,
        "application/json",
        "{}",
        &response,
        &status);

    zero_secure(authorization, sizeof(authorization));

    if (err != ESP_OK) {
        diagnostic_set("delegation_request", err);
        return err;
    }

    diagnostic_set_http("delegation_http", ESP_OK, status);

    if (status < 200 || status >= 300) {
        diagnostic_set_http(
            "delegation_http",
            ESP_FAIL,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return ESP_FAIL;
    }

    diagnostic_set("delegation_parse", ESP_OK);
    err = parse_json_string_token(response, out, capacity);

    if (err != ESP_OK) {
        diagnostic_set("delegation_parse", err);
    }

    zero_secure(response, HTTP_BODY_MAX);
    free(response);
    return err;
}

static esp_err_t homey_login(
    const char *base_url,
    const char *delegation,
    char *session,
    size_t session_capacity)
{
    char url[ATHOM_HOMEY_URL_MAX + 64U];
    int written = snprintf(url, sizeof(url), "%s/api/manager/users/login", base_url);
    if (written <= 0 || (size_t)written >= sizeof(url)) return ESP_ERR_INVALID_SIZE;

    char *escaped = cJSON_PrintUnformatted(cJSON_CreateString(delegation));
    if (escaped == NULL) return ESP_ERR_NO_MEM;
    size_t body_len = strlen(escaped) + 16U;
    char *body = malloc(body_len);
    if (body == NULL) {
        zero_secure(escaped, strlen(escaped));
        free(escaped);
        return ESP_ERR_NO_MEM;
    }
    snprintf(body, body_len, "{\"token\":%s}", escaped);
    zero_secure(escaped, strlen(escaped));
    free(escaped);

    char *response = NULL;
    int status = 0;

    diagnostic_set("homey_login_request", ESP_OK);

    esp_err_t err = http_request(
        url,
        HTTP_METHOD_POST,
        NULL,
        "application/json",
        body,
        &response,
        &status);

    zero_secure(body, body_len);
    free(body);

    if (err != ESP_OK) {
        diagnostic_set("homey_login_request", err);
        return err;
    }

    diagnostic_set_http("homey_login_http", ESP_OK, status);

    if (status < 200 || status >= 300) {
        diagnostic_set_http(
            "homey_login_http",
            ESP_FAIL,
            status);

        zero_secure(response, HTTP_BODY_MAX);
        free(response);
        return ESP_FAIL;
    }

    diagnostic_set("homey_login_parse", ESP_OK);
    err = parse_json_string_token(
        response,
        session,
        session_capacity);

    if (err != ESP_OK) {
        diagnostic_set("homey_login_parse", err);
    }

    zero_secure(response, HTTP_BODY_MAX);
    free(response);
    return err;
}

esp_err_t athom_cloud_select_and_connect(
    athom_cloud_state_t *state,
    const char *homey_id)
{
    if (state == NULL || homey_id == NULL) {
        diagnostic_set("argument_validation", ESP_ERR_INVALID_ARG);
        return ESP_ERR_INVALID_ARG;
    }

    diagnostic_set("homey_lookup", ESP_OK);

    const athom_homey_t *selected =
        athom_homey_find_exact(&state->homeys, homey_id);

    if (selected == NULL) {
        diagnostic_set("homey_lookup", ESP_ERR_NOT_FOUND);
        return ESP_ERR_NOT_FOUND;
    }

    diagnostic_set("url_selection", ESP_OK);

    const char *urls[] = {
        selected->local_url_secure,
        selected->local_url,
        selected->remote_url,
    };

    const char *url_stages[] = {
        "homey_login_local_secure",
        "homey_login_local",
        "homey_login_remote",
    };

    bool has_url = false;

    for (size_t index = 0U;
         index < sizeof(urls) / sizeof(urls[0]);
         ++index) {
        if (urls[index] != NULL && urls[index][0] != 0) {
            has_url = true;
            break;
        }
    }

    if (!has_url) {
        diagnostic_set("url_selection", ESP_ERR_NOT_FOUND);
        return ESP_ERR_NOT_FOUND;
    }

    char delegation[ATHOM_TOKEN_MAX];
    esp_err_t err = delegation_token(
        state->tokens.access_token,
        delegation,
        sizeof(delegation));

    if (err != ESP_OK) {
        zero_secure(delegation, sizeof(delegation));
        return err;
    }

    char session[ATHOM_TOKEN_MAX];
    memset(session, 0, sizeof(session));

    size_t connected_index =
        sizeof(urls) / sizeof(urls[0]);

    for (size_t index = 0U;
         index < sizeof(urls) / sizeof(urls[0]);
         ++index) {
        if (urls[index] == NULL || urls[index][0] == 0) {
            continue;
        }

        diagnostic_set(url_stages[index], ESP_OK);

        err = homey_login(
            urls[index],
            delegation,
            session,
            sizeof(session));

        if (err == ESP_OK) {
            connected_index = index;
            break;
        }

        zero_secure(session, sizeof(session));

        if (err != ESP_ERR_HTTP_CONNECT) {
            break;
        }
    }

    zero_secure(delegation, sizeof(delegation));

    if (err != ESP_OK ||
        connected_index >= sizeof(urls) / sizeof(urls[0])) {
        zero_secure(session, sizeof(session));
        return err;
    }

    state->selected_homey = *selected;

    /*
     * Preserve the exact URL returned by /user/me that succeeded.
     * Clearing only higher-priority failed URLs ensures that
     * athom_homey_preferred_url() later returns the same URL for
     * inventory requests.
     */
    if (connected_index >= 1U) {
        state->selected_homey.local_url_secure[0] = 0;
    }

    if (connected_index >= 2U) {
        state->selected_homey.local_url[0] = 0;
    }
    memcpy(
        state->homey_session_token,
        session,
        strlen(session) + 1U);

    zero_secure(session, sizeof(session));
    diagnostic_set("session_ready", ESP_OK);
    return ESP_OK;
}

static esp_err_t count_collection(
    const char *base_url,
    const char *path,
    const char *session_token,
    size_t *count_out,
    size_t response_maximum)
{
    char url[ATHOM_HOMEY_URL_MAX + 128U];
    int written =
        snprintf(url, sizeof(url), "%s%s", base_url, path);

    if (written <= 0 ||
        (size_t)written >= sizeof(url)) {
        return ESP_ERR_INVALID_SIZE;
    }

    char authorization[ATHOM_TOKEN_MAX + 16U];
    esp_err_t err = bearer_authorization(
        session_token,
        authorization,
        sizeof(authorization));

    if (err != ESP_OK) return err;

    char *response = NULL;
    size_t response_capacity = 0U;
    int status = 0;

    err = http_request_limited(
        url,
        HTTP_METHOD_GET,
        authorization,
        NULL,
        NULL,
        &response,
        &status,
        response_maximum,
        &response_capacity);

    zero_secure(authorization, sizeof(authorization));

    if (err != ESP_OK) return err;

    if (status < 200 || status >= 300) {
        zero_secure(response, response_capacity);
        free(response);
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(response);

    zero_secure(response, response_capacity);
    free(response);

    if (root == NULL) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    cJSON *value =
        cJSON_GetObjectItemCaseSensitive(root, "result");

    if (value == NULL) value = root;

    size_t count = 0U;

    if (cJSON_IsArray(value)) {
        count = (size_t)cJSON_GetArraySize(value);
    } else if (cJSON_IsObject(value)) {
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, value) {
            count++;
        }
    }

    cJSON_Delete(root);
    *count_out = count;
    return ESP_OK;
}

esp_err_t athom_cloud_fetch_inventory(athom_cloud_state_t *state)
{
    if (state == NULL || state->homey_session_token[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    const char *base_url = athom_homey_preferred_url(&state->selected_homey);
    if (base_url == NULL) return ESP_ERR_NOT_FOUND;

    diagnostic_set("inventory_zones", ESP_OK);

    esp_err_t err = count_collection(
        base_url,
        "/api/manager/zones/zone",
        state->homey_session_token,
        &state->zone_count,
        HTTP_BODY_MAX);

    if (err != ESP_OK) {
        diagnostic_set("inventory_zones", err);
        return err;
    }

    diagnostic_set("inventory_devices", ESP_OK);

    err = count_collection(
        base_url,
        "/api/manager/devices/device",
        state->homey_session_token,
        &state->device_count,
        HTTP_INVENTORY_BODY_MAX);

    if (err != ESP_OK) {
        diagnostic_set("inventory_devices", err);
        return err;
    }

    diagnostic_set("inventory_complete", ESP_OK);
    return ESP_OK;
}
#endif
