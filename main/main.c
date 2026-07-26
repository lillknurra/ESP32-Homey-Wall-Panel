#include "secure_bootstrap.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "homey_wall_panel";

void app_main(void)
{
    esp_err_t nvs_status = nvs_flash_init();
    ESP_LOGI(TAG, "NVS_BOOT init_result=%s", esp_err_to_name(nvs_status));
    const bool recovery_erase_requested =
        nvs_status == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_status == ESP_ERR_NVS_NEW_VERSION_FOUND;
    ESP_LOGI(TAG, "NVS_BOOT recovery_erase_requested=%s",
             recovery_erase_requested ? "true" : "false");

    if (recovery_erase_requested) {
        esp_err_t erase_status = nvs_flash_erase();
        ESP_LOGI(TAG, "NVS_BOOT erase_result=%s", esp_err_to_name(erase_status));
        ESP_ERROR_CHECK(erase_status);

        esp_err_t reinit_status = nvs_flash_init();
        ESP_LOGI(TAG, "NVS_BOOT reinit_result=%s", esp_err_to_name(reinit_status));
        ESP_ERROR_CHECK(reinit_status);
    } else {
        ESP_ERROR_CHECK(nvs_status);
    }

    ESP_LOGI(TAG, "Patch 005H.1 secure local hardware bootstrap");
    ESP_LOGI(TAG, "Athom OAuth and Athom credential portal are disabled");

    if (secure_bootstrap_start() != SECURE_BOOTSTRAP_OK) {
        ESP_LOGE(TAG, "Secure bootstrap failed");
    }
}
