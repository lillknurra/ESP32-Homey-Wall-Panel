#pragma once
#ifdef ESP_PLATFORM
#include <stddef.h>
#include "esp_err.h"
#include "esp_http_server.h"
esp_err_t athom_oauth_runtime_register_handlers(httpd_handle_t server);
esp_err_t athom_oauth_runtime_on_wifi_online(void);

esp_err_t athom_oauth_runtime_get_selected_homey_id(char *out, size_t capacity);
#endif
