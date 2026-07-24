#include "secure_bootstrap.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "homey_wall_panel";

void app_main(void)
{
    esp_err_t nvs_status = nvs_flash_init();
    if (nvs_status == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(nvs_status);
    }

    ESP_LOGI(TAG, "Patch 005H.1 secure local hardware bootstrap");
    ESP_LOGI(TAG, "Athom OAuth and Athom credential portal are disabled");

    if (secure_bootstrap_start() != SECURE_BOOTSTRAP_OK) {
        ESP_LOGE(TAG, "Secure bootstrap failed");
    }
}
