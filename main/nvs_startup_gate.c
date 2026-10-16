#include "nvs_startup_gate.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "homey_wall_panel";

bool panel_nvs_startup_gate(void)
{
    const esp_err_t status = nvs_flash_init();
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed; normal startup halted (0x%x)",
                 (unsigned int)status);
        return false;
    }
    return true;
}
