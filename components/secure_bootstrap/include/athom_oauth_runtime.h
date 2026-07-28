#pragma once
#ifdef ESP_PLATFORM
#include "esp_err.h"
#include "esp_http_server.h"
esp_err_t athom_oauth_runtime_register_handlers(httpd_handle_t server);
esp_err_t athom_oauth_runtime_on_wifi_online(void);
#endif
