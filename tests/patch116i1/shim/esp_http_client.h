#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct fake_http_client *esp_http_client_handle_t;
typedef enum { HTTP_METHOD_GET = 0 } esp_http_client_method_t;
typedef struct {
    const char *url;
    esp_err_t (*crt_bundle_attach)(void *);
    bool disable_auto_redirect;
    bool keep_alive_enable;
    int timeout_ms;
    int buffer_size;
} esp_http_client_config_t;
#define ESP_ERR_HTTP_EAGAIN (0x7007)
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *cfg);
esp_err_t esp_http_client_set_method(esp_http_client_handle_t client, esp_http_client_method_t method);
esp_err_t esp_http_client_set_header(esp_http_client_handle_t client, const char *name, const char *value);
esp_err_t esp_http_client_open(esp_http_client_handle_t client, int write_len);
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t client);
int esp_http_client_get_status_code(esp_http_client_handle_t client);
bool esp_http_client_is_chunked_response(esp_http_client_handle_t client);
int esp_http_client_read(esp_http_client_handle_t client, char *buffer, int len);
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t client);
esp_err_t esp_http_client_close(esp_http_client_handle_t client);
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client);
