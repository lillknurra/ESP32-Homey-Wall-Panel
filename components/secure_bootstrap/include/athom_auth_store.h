#pragma once
#include "athom_cloud_model.h"
#ifdef ESP_PLATFORM
#include "esp_err.h"

typedef struct {
    athom_token_set_t tokens;
    athom_homey_t selected_homey;
    char homey_session_token[ATHOM_TOKEN_MAX];
    uint64_t expires_at_s;
    size_t zone_count;
    size_t device_count;
} athom_auth_record_t;

esp_err_t athom_auth_store_load(athom_auth_record_t *record, bool *present);
esp_err_t athom_auth_store_publish(const athom_auth_record_t *record);
esp_err_t athom_auth_store_wipe(void);
#endif
