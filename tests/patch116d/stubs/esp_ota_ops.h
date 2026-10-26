#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "esp_partition.h"
#include "esp_app_desc.h"
typedef enum { ESP_OTA_IMG_NEW=0, ESP_OTA_IMG_PENDING_VERIFY=1,
 ESP_OTA_IMG_VALID=2, ESP_OTA_IMG_INVALID=3, ESP_OTA_IMG_ABORTED=4,
 ESP_OTA_IMG_UNDEFINED=5 } esp_ota_img_states_t;
const esp_partition_t *esp_ota_get_running_partition(void);
esp_err_t esp_ota_get_state_partition(const esp_partition_t *partition,
                                       esp_ota_img_states_t *state);
esp_err_t esp_ota_get_partition_description(const esp_partition_t *partition,
                                             esp_app_desc_t *desc);
bool esp_ota_check_rollback_is_possible(void);
esp_err_t esp_ota_mark_app_valid_cancel_rollback(void);
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot(void);
