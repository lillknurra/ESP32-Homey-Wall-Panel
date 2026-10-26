#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "esp_app_desc.h"
#include "esp_http_client.h"
#include "esp_partition.h"
typedef struct ota_handle_struct *esp_https_ota_handle_t;
typedef struct {
 const esp_http_client_config_t *http_config;
 struct {const esp_partition_t *staging;const esp_partition_t *final;bool finalize_with_copy;} partition;
} esp_https_ota_config_t;
esp_err_t esp_https_ota_begin(const esp_https_ota_config_t *,esp_https_ota_handle_t *);
esp_err_t esp_https_ota_get_img_desc(esp_https_ota_handle_t,esp_app_desc_t *);
int esp_https_ota_get_image_size(esp_https_ota_handle_t);
esp_err_t esp_https_ota_perform(esp_https_ota_handle_t);
int esp_https_ota_get_image_len_read(esp_https_ota_handle_t);
bool esp_https_ota_is_complete_data_received(esp_https_ota_handle_t);
esp_err_t esp_https_ota_finish(esp_https_ota_handle_t);
esp_err_t esp_https_ota_abort(esp_https_ota_handle_t);
