#include "athom_runtime.h"
#ifdef ESP_PLATFORM
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

static const char *TAG = "athom_runtime";
static EventGroupHandle_t events;
static const EventBits_t GOT_IP = BIT0;

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)data;
    athom_runtime_status_t *status = arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        status->wifi_started = true;
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        status->wifi_connected = false;
        status->got_ip = false;
        xEventGroupClearBits(events, GOT_IP);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        status->wifi_connected = true;
        status->got_ip = true;
        xEventGroupSetBits(events, GOT_IP);
    }
}

athom_status_t athom_runtime_start(athom_runtime_status_t *status) {
    if (!status) return ATHOM_ERR_ARGUMENT;
    *status = (athom_runtime_status_t){0};
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if (nvs_flash_erase() != ESP_OK || nvs_flash_init() != ESP_OK) return ATHOM_ERR_STORAGE;
    } else if (err != ESP_OK) return ATHOM_ERR_STORAGE;
    if (esp_netif_init() != ESP_OK) return ATHOM_ERR_TRANSPORT;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return ATHOM_ERR_TRANSPORT;
    if (!esp_netif_create_default_wifi_sta()) return ATHOM_ERR_TRANSPORT;
    events = xEventGroupCreate();
    if (!events) return ATHOM_ERR_TRANSPORT;
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&config) != ESP_OK) return ATHOM_ERR_TRANSPORT;
    if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, status) != ESP_OK ||
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, status) != ESP_OK)
        return ATHOM_ERR_TRANSPORT;
    if (esp_wifi_set_storage(WIFI_STORAGE_FLASH) != ESP_OK ||
        esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK || esp_wifi_start() != ESP_OK)
        return ATHOM_ERR_TRANSPORT;
    EventBits_t bits = xEventGroupWaitBits(events, GOT_IP, pdFALSE, pdTRUE, pdMS_TO_TICKS(30000));
    if ((bits & GOT_IP) == 0) {
        ESP_LOGW(TAG, "Wi-Fi station started but no IP was obtained");
        return ATHOM_ERR_TRANSPORT;
    }
    ESP_LOGI(TAG, "Wi-Fi station connected; IP obtained; live gate disabled");
    return ATHOM_OK;
}

athom_status_t athom_runtime_set_live_gate(athom_runtime_status_t *status, bool enabled) {
    if (!status) return ATHOM_ERR_ARGUMENT;
    status->live_gate_enabled = enabled;
    ESP_LOGI(TAG, "Live OAuth gate: %s", enabled ? "enabled" : "disabled");
    return ATHOM_OK;
}
#else
athom_status_t athom_runtime_start(athom_runtime_status_t *status) {
    if (!status) return ATHOM_ERR_ARGUMENT;
    *status = (athom_runtime_status_t){0};
    return ATHOM_OK;
}
athom_status_t athom_runtime_set_live_gate(athom_runtime_status_t *status, bool enabled) {
    if (!status) return ATHOM_ERR_ARGUMENT;
    status->live_gate_enabled = enabled;
    return ATHOM_OK;
}
#endif
