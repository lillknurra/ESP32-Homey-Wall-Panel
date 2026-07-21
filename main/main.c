#include "esp_log.h"
#include "esp_system.h"
#include "esp_idf_version.h"

static const char *TAG = "homey_wall_panel";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Homey Wall Panel bootstrap");
    ESP_LOGI(TAG, "ESP-IDF: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
}
