/* Patch116D -- integrated, DEFAULT-DISABLED ESP-IDF first-boot OTA gate.
 * This is NOT an OTA updater. There is no download/HTTP/RTC/flash writer here.
 * When deliberately enabled with signed-app/rollback configs, this component
 * may write ONLY otadata using ESP-IDF's supported first-boot APIs, strictly
 * before the LVGL/RGB display starts. A later release must separately approve
 * key management, full firmware updates, rescue procedure and activation.
 */
#include "panel_ota_first_boot_gate.h"
#include "sdkconfig.h"

#ifndef PANEL_OTA_FIRST_BOOT_ENABLED
#define PANEL_OTA_FIRST_BOOT_ENABLED 0
#endif

#if (PANEL_OTA_FIRST_BOOT_ENABLED != 0) && (PANEL_OTA_FIRST_BOOT_ENABLED != 1)
#error "PANEL_OTA_FIRST_BOOT_ENABLED must be 0 or 1"
#endif

#if PANEL_OTA_FIRST_BOOT_ENABLED
#if !defined(CONFIG_IDF_TARGET_ESP32S3) || \
    !defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) || \
    !defined(CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT) || \
    !defined(CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT)
#error "OTA first-boot gate requires ESP32-S3, rollback and signed OTA verification"
#endif

/* These values MUST be selected using measured native ESP-IDF and panel
 * startup evidence, not guessed by the offline policy or host tests. */
#if !defined(PANEL_OTA_MIN_INTERNAL_BLOCK_BYTES) || \
    (PANEL_OTA_MIN_INTERNAL_BLOCK_BYTES <= 0)
#error "Select measured PANEL_OTA_MIN_INTERNAL_BLOCK_BYTES before enabling OTA"
#endif
#if !defined(PANEL_OTA_MIN_PSRAM_BLOCK_BYTES) || \
    (PANEL_OTA_MIN_PSRAM_BLOCK_BYTES <= 0)
#error "Select measured PANEL_OTA_MIN_PSRAM_BLOCK_BYTES before enabling OTA"
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_psram.h"
#include "nvs.h"

static const char *const TAG = "ota_first_boot";
static const char PROJECT_NAME[] = "esp32_homey_wall_panel";

/* Exact Patch116 candidate offsets. This code is *not* enabled until the
 * verified OTA partition table and a compatible factory image are installed. */
typedef struct {
    esp_partition_type_t type;
    esp_partition_subtype_t subtype;
    const char *label;
    uint32_t offset;
    uint32_t size;
} expected_partition_t;

static const expected_partition_t EXPECTED_PARTITIONS[] = {
    { ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "nvs",       0x9000,   0x6000 },
    { ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_PHY, "phy_init",  0xF000,   0x1000 },
    { ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory", 0x10000,  0x600000 },
    { ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "diaglog", 0x610000, 0x100000 },
    { ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, "otadata",  0x710000, 0x2000 },
    { ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, "ota_0",    0x720000, 0x400000 },
    { ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "ota_1",    0xB20000, 0x400000 },
};

static bool partition_layout_matches(void)
{
    for (size_t index = 0; index < sizeof(EXPECTED_PARTITIONS) /
                                            sizeof(EXPECTED_PARTITIONS[0]); ++index) {
        const expected_partition_t *want = &EXPECTED_PARTITIONS[index];
        const esp_partition_t *actual = esp_partition_find_first(
            want->type, want->subtype, want->label);
        if (actual == NULL || actual->address != want->offset ||
            actual->size != want->size || actual->type != want->type ||
            actual->subtype != want->subtype) {
            ESP_LOGE(TAG, "OTA partition layout mismatch at index %u", (unsigned)index);
            return false;
        }
    }
    return true;
}

static bool local_health_passes(const esp_partition_t *running)
{
    if (!partition_layout_matches()) return false;
    if (running->subtype != ESP_PARTITION_SUBTYPE_APP_OTA_0 &&
        running->subtype != ESP_PARTITION_SUBTYPE_APP_OTA_1) return false;
    const uint32_t expected = running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0
        ? 0x720000U : 0xB20000U;
    if (running->type != ESP_PARTITION_TYPE_APP ||
        running->address != expected || running->size != 0x400000U) return false;

    const esp_app_desc_t *compiled_desc = esp_app_get_description();
    esp_app_desc_t flashed_desc = {0};
    if (compiled_desc == NULL ||
        esp_ota_get_partition_description(running, &flashed_desc) != ESP_OK ||
        strncmp(compiled_desc->project_name, PROJECT_NAME,
                sizeof(compiled_desc->project_name)) != 0 ||
        strncmp(flashed_desc.project_name, PROJECT_NAME,
                sizeof(flashed_desc.project_name)) != 0 ||
        strncmp(flashed_desc.version, compiled_desc->version,
                sizeof(flashed_desc.version)) != 0) {
        ESP_LOGE(TAG, "OTA app descriptor invalid or inconsistent");
        return false;
    }

    /* Verify the settings namespace can be accessed READ ONLY. Its absence is
     * acceptable on an as-yet-unconfigured device; IO errors are not. */
    nvs_handle_t handle = 0;
    esp_err_t nvs_result = nvs_open("hpanel_ui", NVS_READONLY, &handle);
    if (nvs_result == ESP_OK) {
        nvs_close(handle);
    } else if (nvs_result != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Read-only settings namespace probe failed");
        return false;
    }

    if (!esp_psram_is_initialized()) {
        ESP_LOGE(TAG, "PSRAM unavailable");
        return false;
    }
    const size_t largest_internal = heap_caps_get_largest_free_block(
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t largest_psram = heap_caps_get_largest_free_block(
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (largest_internal < PANEL_OTA_MIN_INTERNAL_BLOCK_BYTES ||
        largest_psram < PANEL_OTA_MIN_PSRAM_BLOCK_BYTES) {
        ESP_LOGE(TAG, "Required memory margin unavailable before display");
        return false;
    }

    /* The firmware's OTA signature trust is established by ESP-IDF update
     * verification, NOT by these descriptor/heap checks. */
    return esp_ota_check_rollback_is_possible();
}

static bool reject_pending_and_stop(void)
{
    /* Never proceed to the UI with an unconfirmed failed image. If a safe
     * fallback exists, use the official one-shot rollback API. It reboots on
     * success; if it returns, STOP rather than repeating or continuing. */
    if (esp_ota_check_rollback_is_possible()) {
        const esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();
        ESP_LOGE(TAG, "Rollback operation returned without reboot: 0x%x", (unsigned)err);
    }
    ESP_LOGE(TAG, "OTA first-boot validation denied; stopping before RGB");
    return false;
}
#endif /* PANEL_OTA_FIRST_BOOT_ENABLED */

bool panel_ota_first_boot_gate_after_nvs(void)
{
#if !PANEL_OTA_FIRST_BOOT_ENABLED
    /* No new behavior in the installed Patch113/default-off configuration. */
    return true;
#else
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running == NULL) {
        ESP_LOGE(TAG, "Running partition unavailable");
        return false;
    }
    if (running->type != ESP_PARTITION_TYPE_APP) return false;
    if (running->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) {
        /* Legacy/factory boot is not a PENDING_VERIFY OTA image. */
        return true;
    }
    if (running->subtype != ESP_PARTITION_SUBTYPE_APP_OTA_0 &&
        running->subtype != ESP_PARTITION_SUBTYPE_APP_OTA_1) {
        return false;
    }
    esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
    if (esp_ota_get_state_partition(running, &state) != ESP_OK) {
        ESP_LOGE(TAG, "OTA image state unavailable");
        return false;
    }
    if (state == ESP_OTA_IMG_VALID) return true;
    if (state != ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGE(TAG, "Unexpected OTA image state; stopping before display");
        return false;
    }

    if (!local_health_passes(running)) return reject_pending_and_stop();

    const esp_err_t confirm = esp_ota_mark_app_valid_cancel_rollback();
    if (confirm != ESP_OK) {
        ESP_LOGE(TAG, "OTA validation commit failed: 0x%x", (unsigned)confirm);
        return false;
    }
    ESP_LOGI(TAG, "First-boot local health confirmed before display start");
    return true;
#endif
}
