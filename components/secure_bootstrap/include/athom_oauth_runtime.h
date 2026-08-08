#pragma once
#ifdef ESP_PLATFORM
#include <stddef.h>
#include "esp_err.h"
#include "esp_http_server.h"
esp_err_t athom_oauth_runtime_register_handlers(httpd_handle_t server);
esp_err_t athom_oauth_runtime_on_wifi_online(void);

typedef enum {
    ATHOM_HOMEY_DATA_LOADING = 0,
    ATHOM_HOMEY_DATA_RETRYING,
    ATHOM_HOMEY_DATA_READY,
    ATHOM_HOMEY_DATA_ERROR,
} athom_homey_data_state_t;

athom_homey_data_state_t athom_oauth_runtime_homey_data_state(void);
const char *athom_oauth_runtime_homey_data_state_name(void);

esp_err_t athom_oauth_runtime_get_selected_homey_id(char *out, size_t capacity);
#endif
