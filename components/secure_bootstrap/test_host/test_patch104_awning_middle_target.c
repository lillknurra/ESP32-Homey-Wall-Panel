#include "panel_roller_model.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ESP_OK 0
#define ATHOM_HOMEY_AWNING_WRITE_ACCEPTED 0
#define ATHOM_HOMEY_AWNING_WRITE_INVALID_ARGUMENT 1
#define ATHOM_HOMEY_AWNING_WRITE_NOT_READY 2
#define ATHOM_HOMEY_AWNING_WRITE_TARGET_INVALID 3
#define ATHOM_HOMEY_AWNING_WRITE_UNAUTHORIZED 4
#define ATHOM_HOMEY_AWNING_WRITE_REJECTED 5
#define ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS 6
#define ATHOM_HOMEY_AWNING_WRITE_INTERNAL_ERROR 7
#define PANEL_HOMEY_IDENTITY_DIGEST_SIZE 32U
#define PANEL_HOMEY_RAW_DEVICE_ID_MAX 128U
#define ATHOM_HOMEY_URL_MAX 320U
#define ATHOM_TOKEN_MAX 128U
#define PATCH085_WRITE_PATH_MAX 512U
#define PATCH085_WRITE_BODY_MAX 32U
#define PATCH085_CAPABILITY_ID "windowcoverings_set"

typedef int esp_err_t;
typedef int athom_homey_awning_write_result_t;
typedef struct { char id[64]; char remote_url[ATHOM_HOMEY_URL_MAX]; } selected_homey_t;
typedef struct {
    selected_homey_t selected_homey;
    char homey_session_token[ATHOM_TOKEN_MAX];
} athom_cloud_state_t;
typedef struct {
    uint8_t selected_homey_digest[PANEL_HOMEY_IDENTITY_DIGEST_SIZE];
    uint32_t alias_generation;
    uint32_t snapshot_generation;
    double current_position;
    double minimum;
    double maximum;
    double step;
    bool step_present;
} athom_cloud_awning_command_context_t;
typedef struct {
    char raw_device_id[PANEL_HOMEY_RAW_DEVICE_ID_MAX];
    char raw_capability_id[64];
} panel_homey_alias_entry_t;
typedef struct { int unused; } persistent_http_client_t;
typedef struct {
    bool write_attempted;
    bool fresh_response_received;
    bool response_complete;
    int fresh_http_status;
    esp_err_t perform_error;
} patch037_homey_put_once_result_t;
typedef struct {
    const athom_cloud_state_t *state;
    size_t widget_index;
    const athom_cloud_awning_command_context_t *expected;
} patch085_dispatch_preflight_t;

static persistent_http_client_t s_homey_http;
static athom_cloud_awning_command_context_t s_current;
static bool s_armed = true;
static bool s_alias_valid = true;
static unsigned s_write_calls;
static char s_written_body[PATCH085_WRITE_BODY_MAX];

static bool patch085_awning_target_supported(size_t widget)
{ return widget == 1U || widget == 2U; }
static bool athom_cloud_awning_write_is_armed(size_t widget)
{ return s_armed && patch085_awning_target_supported(widget); }
static bool athom_cloud_get_awning_command_context(
    const athom_cloud_state_t *state, size_t widget,
    athom_cloud_awning_command_context_t *out)
{
    if (state == NULL || out == NULL || !patch085_awning_target_supported(widget)) return false;
    *out = s_current;
    return true;
}
static bool patch085_alias_target(
    const athom_cloud_state_t *state, size_t widget, panel_homey_alias_entry_t *entry,
    uint8_t digest[PANEL_HOMEY_IDENTITY_DIGEST_SIZE], uint32_t *generation)
{
    if (state == NULL || entry == NULL || digest == NULL || generation == NULL ||
        !patch085_awning_target_supported(widget) || !s_alias_valid) return false;
    strcpy(entry->raw_device_id, "synthetic-device");
    strcpy(entry->raw_capability_id, PATCH085_CAPABILITY_ID);
    memset(digest, 0x5A, PANEL_HOMEY_IDENTITY_DIGEST_SIZE);
    *generation = s_current.alias_generation;
    return true;
}
static bool patch037_encode_path_segment(const char *input, char *output, size_t capacity)
{
    if (input == NULL || output == NULL || strlen(input) >= capacity) return false;
    strcpy(output, input);
    return true;
}
static esp_err_t bearer_authorization(const char *token, char *output, size_t capacity)
{
    if (token == NULL || output == NULL) return -1;
    const int written = snprintf(output, capacity, "Bearer %s", token);
    return written > 0 && (size_t)written < capacity ? ESP_OK : -1;
}
static void transport_cleanup_one(persistent_http_client_t *client) { (void)client; }
static void diagnostic_set(const char *stage, esp_err_t error) { (void)stage; (void)error; }
static void diagnostic_set_http(const char *stage, esp_err_t error, int status)
{ (void)stage; (void)error; (void)status; }
static bool patch085_dispatch_preflight(void *opaque)
{
    patch085_dispatch_preflight_t *preflight = opaque;
    return preflight != NULL && preflight->state != NULL && preflight->expected != NULL;
}
static esp_err_t patch037_homey_put_once(
    const char *url, const char *authorization, const char *body,
    bool (*preflight)(void *), void *preflight_context, bool awning_write,
    size_t widget, patch037_homey_put_once_result_t *out)
{
    assert(url != NULL && authorization != NULL && body != NULL && out != NULL);
    assert(strcmp(authorization, "Bearer synthetic-session") == 0);
    assert(awning_write && patch085_awning_target_supported(widget));
    assert(preflight(preflight_context));
    s_write_calls++;
    snprintf(s_written_body, sizeof(s_written_body), "%s", body);
    *out = (patch037_homey_put_once_result_t){
        .write_attempted = true, .fresh_response_received = true,
        .response_complete = true, .fresh_http_status = 204, .perform_error = ESP_OK,
    };
    return ESP_OK;
}
static athom_homey_awning_write_result_t patch085_classify_awning_write(
    bool attempted, bool complete, int status, esp_err_t error)
{
    if (attempted && complete && status >= 200 && status < 300 && error == ESP_OK)
        return ATHOM_HOMEY_AWNING_WRITE_ACCEPTED;
    return ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS;
}

/* PATCH104_PRODUCTION_FUNCTION */

static void setup(athom_cloud_state_t *state, double current, bool step_present, double step)
{
    memset(state, 0, sizeof(*state));
    strcpy(state->selected_homey.id, "synthetic-homey");
    strcpy(state->selected_homey.remote_url, "https://synthetic.invalid");
    strcpy(state->homey_session_token, "synthetic-session");
    s_current = (athom_cloud_awning_command_context_t){
        .alias_generation = 7U, .snapshot_generation = 11U,
        .current_position = current, .minimum = 0.0, .maximum = 1.0,
        .step = step, .step_present = step_present,
    };
    memset(s_current.selected_homey_digest, 0x5A, sizeof(s_current.selected_homey_digest));
}

int main(void)
{
    athom_cloud_state_t state;
    athom_cloud_awning_command_context_t expected;
    setup(&state, 0.25, false, 0.0);
    expected = s_current;
    assert(athom_cloud_set_awning_position(&state, 1U, 0.53, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_ACCEPTED);
    assert(strcmp(s_written_body, "{\"value\":0.53}") == 0 && s_write_calls == 1U);

    setup(&state, 0.75, false, 0.0);
    expected = s_current;
    assert(athom_cloud_set_awning_position(&state, 2U, 0.53, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_ACCEPTED);
    assert(strcmp(s_written_body, "{\"value\":0.53}") == 0 && s_write_calls == 2U);

    setup(&state, 0.25, true, 0.25);
    expected = s_current;
    assert(athom_cloud_set_awning_position(&state, 1U, 0.53, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_NOT_READY);
    assert(s_write_calls == 2U);

    setup(&state, 0.25, true, 0.0);
    expected = s_current;
    assert(athom_cloud_set_awning_position(&state, 1U, 0.53, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_NOT_READY);
    assert(s_write_calls == 2U);

    setup(&state, 0.75, false, 0.0);
    s_current.minimum = 0.54;
    expected = s_current;
    assert(athom_cloud_set_awning_position(&state, 2U, 0.53, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_NOT_READY);
    assert(s_write_calls == 2U);

    setup(&state, 0.75, false, 0.0);
    s_current.minimum = 0.8;
    s_current.maximum = 0.2;
    expected = s_current;
    assert(athom_cloud_set_awning_position(&state, 2U, 0.53, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_NOT_READY);
    assert(s_write_calls == 2U);

    setup(&state, 0.25, true, 0.01);
    expected = s_current;
    assert(athom_cloud_set_awning_position(&state, 1U, 0.53, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_ACCEPTED);
    assert(s_write_calls == 3U);

    setup(&state, 0.25, false, 0.0);
    expected = s_current;
    assert(athom_cloud_set_awning_position(&state, 1U, 0.50, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_INVALID_ARGUMENT);
    assert(s_write_calls == 3U);

    s_armed = false;
    assert(athom_cloud_set_awning_position(&state, 1U, 0.53, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_UNAUTHORIZED);
    assert(s_write_calls == 3U);
    assert(athom_cloud_set_awning_position(&state, 0U, 0.53, &expected) ==
           ATHOM_HOMEY_AWNING_WRITE_INVALID_ARGUMENT);
    puts("PATCH104_AWNING_MIDDLE_TARGET_TESTS PASS");
    return 0;
}
