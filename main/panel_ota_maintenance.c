#include "panel_ota_maintenance.h"

#include "panel_ota_first_boot_gate.h"
#include "panel_ota_handoff.h"
#include "panel_ota_supervisor.h"
#include "panel_ota_https_stage.h"
#include "panel_update_https_fetch.h"
#include "panel_update_manifest.h"
#include "sdkconfig.h"

#include <stdint.h>
#include <string.h>
#include <time.h>
#include "esp_app_desc.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define GOT_IP_BIT BIT0
#define OTA_WIFI_WAIT_MS 30000U
#define OTA_TIME_WAIT_MS 30000U
#define OTA_OWNER_STACK_BYTES 24576U

static const char *TAG = "ota_maintenance";
static EventGroupHandle_t s_events;
static bool s_headless_active;
static void maintenance_task(void *arg)
{
    (void)arg;
    panel_ota_maintenance_run();
}

void panel_ota_maintenance_start(void)
{
    if (xTaskCreate(maintenance_task, "ota_maintenance",
                    OTA_OWNER_STACK_BYTES, NULL, 5U, NULL) != pdPASS) {
        panel_ota_handoff_record_result(PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED);
        ESP_LOGE(TAG, "maintenance_owner_create=failed restart=normal_boot");
        esp_restart();
        for (;;) vTaskDelay(portMAX_DELAY);
    }
}

static bool stage_authorized(void *context)
{
    (void)context;
    return s_headless_active && panel_ota_supervisor_worker_authorized();
}

static void network_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP && s_events != NULL)
        xEventGroupSetBits(s_events, GOT_IP_BIT);
}

static bool start_saved_wifi(void)
{
    if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK)
        return false;
    esp_netif_t *sta = esp_netif_create_default_wifi_sta();
    if (sta == NULL) return false;
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init) != ESP_OK ||
        esp_wifi_set_storage(WIFI_STORAGE_FLASH) != ESP_OK) return false;
    wifi_config_t saved = {0};
    if (esp_wifi_get_config(WIFI_IF_STA, &saved) != ESP_OK || saved.sta.ssid[0] == 0U)
        return false;
    memset(&saved, 0, sizeof(saved));
    s_events = xEventGroupCreate();
    if (s_events == NULL) return false;
    if (esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                   network_event, NULL) != ESP_OK ||
        esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_start() != ESP_OK || esp_wifi_connect() != ESP_OK) return false;
    const EventBits_t bits = xEventGroupWaitBits(s_events, GOT_IP_BIT, pdFALSE,
        pdTRUE, pdMS_TO_TICKS(OTA_WIFI_WAIT_MS));
    return (bits & GOT_IP_BIT) != 0U;
}

static bool synchronize_time(void)
{
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    if (esp_netif_sntp_init(&cfg) != ESP_OK) return false;
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(OTA_TIME_WAIT_MS)) != ESP_OK) return false;
    const time_t now = time(NULL);
    return now >= (time_t)1760000000; /* Reject unset/pre-2025 wall clock. */
}

static bool fetch_release(panel_update_manifest_t *manifest)
{
    char body[PANEL_UPDATE_MANIFEST_MAX_BYTES + 1U] = {0};
    size_t length = 0U;
    if (panel_update_https_fetch("stable", body, sizeof(body), &length) !=
            PANEL_UPDATE_FETCH_OK || length == 0U || length > PANEL_UPDATE_MANIFEST_MAX_BYTES) {
        memset(body, 0, sizeof(body));
        return false;
    }
    const bool parsed = panel_update_manifest_parse(body, length, manifest) &&
        strcmp(manifest->project, PANEL_UPDATE_PROJECT_ID) == 0 &&
        strcmp(manifest->hardware_model, PANEL_UPDATE_MODEL_ID) == 0 &&
        strcmp(manifest->channel, "stable") == 0;
    memset(body, 0, sizeof(body));
    return parsed;
}

static bool release_is_newer(const panel_update_manifest_t *manifest)
{
    const esp_app_desc_t *current = esp_app_get_description();
    if (current == NULL) return false;
    const esp_partition_t *inactive = esp_ota_get_next_update_partition(NULL);
    if (inactive == NULL || inactive->size > PANEL_UPDATE_MAX_IMAGE_BYTES) return false;
    panel_update_local_t local = {
        .hardware_model = PANEL_UPDATE_MODEL_ID,
        .project = PANEL_UPDATE_PROJECT_ID,
        .channel = "stable",
        .installed_release_sequence = CONFIG_PANEL_OTA_RELEASE_SEQUENCE,
        .installed_version = current->version,
        .has_ota_partitions = true,
        .inactive_ota_capacity = inactive->size,
    };
    return panel_update_manifest_evaluate(manifest, &local) == PANEL_UPDATE_NEWER_AVAILABLE;
}

_Noreturn void panel_ota_maintenance_run(void)
{
    s_headless_active = true;
    if (!panel_ota_supervisor_start()) {
        panel_ota_handoff_record_result(PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED);
        esp_restart();
    }
    panel_update_manifest_t manifest = {0};
    panel_ota_stage_access_t access = {
        .authenticated_operator_confirmed = stage_authorized,
        .pre_display_maintenance_quiescent = stage_authorized,
    };
    panel_ota_handoff_result_t outcome = PANEL_OTA_HANDOFF_RESULT_NO_RELEASE;
    if (!panel_ota_https_stage_layout_preflight()) {
        outcome = PANEL_OTA_HANDOFF_RESULT_MANIFEST_REJECTED;
        goto finish;
    }
    if (!start_saved_wifi()) {
        outcome = PANEL_OTA_HANDOFF_RESULT_WIFI_FAILED;
        goto finish;
    }
    if (!synchronize_time()) {
        outcome = PANEL_OTA_HANDOFF_RESULT_TIME_FAILED;
        goto finish;
    }
    if (!fetch_release(&manifest) || !release_is_newer(&manifest)) {
        outcome = PANEL_OTA_HANDOFF_RESULT_MANIFEST_REJECTED;
        goto finish;
    }
    outcome = panel_ota_https_stage(&access, &manifest) == PANEL_OTA_STAGE_STAGED_NO_REBOOT
        ? PANEL_OTA_HANDOFF_RESULT_STAGED : PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED;
finish:
    memset(&manifest, 0, sizeof(manifest));
    if (panel_ota_supervisor_worker_complete(outcome)) {
        ESP_LOGW(TAG, "maintenance_result=%s restart=yes",
                 panel_ota_handoff_result_name(outcome));
        esp_restart();
    }
    for (;;) vTaskDelay(portMAX_DELAY);
}
