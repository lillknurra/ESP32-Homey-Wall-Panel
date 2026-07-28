#pragma once
#include "athom_auth_store.h"
#include "athom_cloud_model.h"
#ifdef ESP_PLATFORM
#include "esp_err.h"

typedef struct {
    athom_token_set_t tokens;
    athom_homey_list_t homeys;
    athom_homey_t selected_homey;
    char homey_session_token[ATHOM_TOKEN_MAX];
    uint64_t expires_at_s;
    size_t zone_count;
    size_t device_count;
} athom_cloud_state_t;

esp_err_t athom_cloud_exchange_code(
    const char *authorization_code,
    athom_cloud_state_t *state);

esp_err_t athom_cloud_refresh(athom_cloud_state_t *state);

esp_err_t athom_cloud_fetch_user_homeys(athom_cloud_state_t *state);

esp_err_t athom_cloud_select_and_connect(
    athom_cloud_state_t *state,
    const char *homey_id);

esp_err_t athom_cloud_fetch_inventory(athom_cloud_state_t *state);

const char *athom_cloud_diagnostic_stage(void);
esp_err_t athom_cloud_diagnostic_error(void);
int athom_cloud_diagnostic_http_status(void);

#endif
