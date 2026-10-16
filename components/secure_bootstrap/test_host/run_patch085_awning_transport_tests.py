#!/usr/bin/env python3
"""Compile the exact production one-shot PUT helper against a mock transport."""
from pathlib import Path
import subprocess
import tempfile

component = Path(__file__).resolve().parents[1]
source = (component / "athom_cloud_client.c").read_text(encoding="utf-8")
signature = "static esp_err_t patch037_homey_put_once("
start = source.find(signature)
if start < 0:
    raise SystemExit("production patch037_homey_put_once missing")
brace = source.find("{", start)
depth = 0
end = None
for index in range(brace, len(source)):
    if source[index] == "{":
        depth += 1
    elif source[index] == "}":
        depth -= 1
        if depth == 0:
            end = index + 1
            break
if end is None:
    raise SystemExit("production one-shot helper body is incomplete")
production = source[start:end]

classify_signature = "static athom_homey_awning_write_result_t patch085_classify_awning_write("
classify_start = source.find(classify_signature)
if classify_start < 0:
    raise SystemExit("production awning result classifier missing")
classify_brace = source.find("{", classify_start)
classify_depth = 0
classify_end = None
for index in range(classify_brace, len(source)):
    if source[index] == "{":
        classify_depth += 1
    elif source[index] == "}":
        classify_depth -= 1
        if classify_depth == 0:
            classify_end = index + 1
            break
if classify_end is None:
    raise SystemExit("production awning result classifier body is incomplete")
classifier = source[classify_start:classify_end]

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG -2
#define ESP_ERR_INVALID_STATE -3
#define ESP_ERR_NO_MEM -4
#define ATHOM_HOMEY_AWNING_WRITE_ACCEPTED 0
#define ATHOM_HOMEY_AWNING_WRITE_INVALID_ARGUMENT 1
#define ATHOM_HOMEY_AWNING_WRITE_NOT_READY 2
#define ATHOM_HOMEY_AWNING_WRITE_TARGET_INVALID 3
#define ATHOM_HOMEY_AWNING_WRITE_UNAUTHORIZED 4
#define ATHOM_HOMEY_AWNING_WRITE_REJECTED 5
#define ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS 6
#define ATHOM_HOMEY_AWNING_WRITE_INTERNAL_ERROR 7
typedef int athom_homey_awning_write_result_t;
#define HOMEY_REMOTE_HTTP_TIMEOUT_MS 8000
#define HTTP_METHOD_PUT 2

typedef struct {
    char *data;
    size_t length, capacity, maximum;
    bool overflow, connected_event_seen, error_event_seen;
    bool disconnected_event_seen, fresh_status_received;
    int fresh_http_status;
} response_buffer_t;

typedef struct { int unused; } esp_http_client_event_t;
typedef esp_err_t (*event_handler_t)(esp_http_client_event_t *);
typedef struct {
    const char *url;
    event_handler_t event_handler;
    void *user_data;
    int timeout_ms;
    void (*crt_bundle_attach)(void);
    int buffer_size, buffer_size_tx;
    bool disable_auto_redirect;
    int max_authorization_retries;
} esp_http_client_config_t;
typedef struct mock_client *esp_http_client_handle_t;

typedef struct {
    bool write_attempted;
    bool fresh_response_received;
    bool response_complete;
    int fresh_http_status;
    esp_err_t perform_error;
} patch037_homey_put_once_result_t;

struct mock_client {
    esp_http_client_config_t config;
    int method;
    const char *authorization;
    const char *content_type;
    const char *body;
};

static unsigned perform_calls, cleanup_calls, preflight_calls;
static int mock_status;
static bool mock_response;
static bool mock_complete;
static esp_err_t mock_perform_error;
static bool mock_preflight_result;
static esp_err_t mock_setup_error;
static bool mock_awning_armed;
static bool athom_cloud_awning_write_is_armed(size_t widget)
{ return mock_awning_armed && (widget == 1U || widget == 2U); }

static void esp_crt_bundle_attach(void) {}
static esp_err_t event_handler(esp_http_client_event_t *event)
{ (void)event; return ESP_OK; }
static void zero_secure(void *p, size_t n) { if (p != NULL) memset(p, 0, n); }

static esp_http_client_handle_t esp_http_client_init(
    const esp_http_client_config_t *config)
{
    struct mock_client *client = calloc(1U, sizeof(*client));
    if (client != NULL) client->config = *config;
    return client;
}
static esp_err_t esp_http_client_set_method(esp_http_client_handle_t c, int method)
{ c->method = method; return mock_setup_error; }
static esp_err_t esp_http_client_set_header(
    esp_http_client_handle_t c, const char *name, const char *value)
{
    if (strcmp(name, "Authorization") == 0) c->authorization = value;
    if (strcmp(name, "Content-Type") == 0) c->content_type = value;
    return mock_setup_error;
}
static esp_err_t esp_http_client_set_post_field(
    esp_http_client_handle_t c, const char *body, int length)
{ assert(length == (int)strlen(body)); c->body = body; return mock_setup_error; }
static esp_err_t esp_http_client_perform(esp_http_client_handle_t c)
{
    perform_calls++;
    assert(c->method == HTTP_METHOD_PUT);
    assert(strcmp(c->authorization, "Bearer synthetic") == 0);
    assert(strcmp(c->content_type, "application/json") == 0);
    assert(strcmp(c->body, "{\"value\":0.75}") == 0);
    response_buffer_t *buffer = c->config.user_data;
    buffer->fresh_status_received = mock_response;
    buffer->fresh_http_status = mock_status;
    return mock_perform_error;
}
static esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c)
{ cleanup_calls++; free(c); return ESP_OK; }
static bool esp_http_client_is_complete_data_received(esp_http_client_handle_t c)
{ (void)c; return mock_complete; }
static bool preflight(void *context)
{ (void)context; preflight_calls++; return mock_preflight_result; }

'''

suffix = r'''
int main(void)
{
    patch037_homey_put_once_result_t result = {0};
    mock_preflight_result = true;
    mock_response = true;
    mock_complete = true;
    mock_status = 204;
    mock_perform_error = ESP_OK;
    mock_awning_armed = true;
    assert(patch037_homey_put_once("https://synthetic.invalid/fixed", "Bearer synthetic",
        "{\"value\":0.75}", preflight, NULL, false, 0U, &result) == ESP_OK);
    assert(result.write_attempted && result.fresh_response_received &&
           result.response_complete && result.fresh_http_status == 204);
    assert(perform_calls == 1U && preflight_calls == 1U && cleanup_calls == 1U);

    memset(&result, 0, sizeof(result));
    mock_response = false;
    mock_complete = false;
    mock_perform_error = -77;
    assert(patch037_homey_put_once("https://synthetic.invalid/fixed", "Bearer synthetic",
        "{\"value\":0.75}", preflight, NULL, true, 1U, &result) == -77);
    assert(result.write_attempted && !result.fresh_response_received &&
           !result.response_complete && result.fresh_http_status == 0);
    assert(perform_calls == 2U && preflight_calls == 2U && cleanup_calls == 2U);

    memset(&result, 0, sizeof(result));
    mock_response = true;
    mock_complete = true;
    mock_status = 429;
    mock_perform_error = ESP_OK;
    assert(patch037_homey_put_once("https://synthetic.invalid/fixed", "Bearer synthetic",
        "{\"value\":0.75}", preflight, NULL, true, 2U, &result) == ESP_OK);
    assert(result.write_attempted && result.fresh_response_received && result.fresh_http_status == 429);
    assert(perform_calls == 3U && preflight_calls == 3U && cleanup_calls == 3U);

    memset(&result, 0, sizeof(result));
    mock_preflight_result = false;
    assert(patch037_homey_put_once("https://synthetic.invalid/fixed", "Bearer synthetic",
        "{\"value\":0.75}", preflight, NULL, true, 1U, &result) == ESP_ERR_INVALID_STATE);
    assert(!result.write_attempted && perform_calls == 3U && preflight_calls == 4U && cleanup_calls == 4U);

    mock_preflight_result = true;
    mock_awning_armed = false;
    assert(patch037_homey_put_once("https://synthetic.invalid/fixed", "Bearer synthetic",
        "{\"value\":0.75}", preflight, NULL, true, 1U, &result) == ESP_ERR_INVALID_STATE);
    assert(!result.write_attempted && perform_calls == 3U);

    assert(patch085_classify_awning_write(true, true, 200, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_ACCEPTED);
    assert(patch085_classify_awning_write(true, true, 200, -77) ==
           ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch085_classify_awning_write(true, true, 403, -77) ==
           ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch085_classify_awning_write(true, true, 403, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_UNAUTHORIZED);
    assert(patch085_classify_awning_write(true, true, 500, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch085_classify_awning_write(true, false, 200, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch085_classify_awning_write(true, false, 403, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch085_classify_awning_write(false, true, 204, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_INTERNAL_ERROR);
    puts("PATCH085_AWNING_ONE_SHOT_TRANSPORT_TESTS PASS");
    return 0;
}
'''

with tempfile.TemporaryDirectory() as temporary:
    cfile = Path(temporary) / "patch085_transport_test.c"
    binary = Path(temporary) / "patch085_transport_test"
    cfile.write_text(prefix + production + "\n" + classifier + "\n" + suffix, encoding="utf-8")
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        str(cfile), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
