#include "secure_bootstrap.h"
#include "runtime_diag_journal.h"
#include "nvs_startup_gate.h"
#include "panel_ota_first_boot_gate.h"
#include "panel_ota_handoff.h"
#include "panel_ota_maintenance.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "homey_wall_panel";

void app_main(void)
{
    const int reset_reason_raw = (int)esp_reset_reason();
    const bool maintenance = panel_ota_handoff_consume_for_software_reset(reset_reason_raw);
    if (maintenance) {
        if (!panel_nvs_startup_gate()) {
            panel_ota_handoff_record_result(PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED);
            esp_restart();
        }
        panel_ota_maintenance_start();
        /* The dedicated maintenance task owns all network/TLS/flash calls.
         * Release app_main's smaller stack while it runs. */
        vTaskDelete(NULL);
    }

    const runtime_diag_reset_reason_t reset_reason =
        runtime_diag_map_reset_reason(reset_reason_raw);
    (void)runtime_diag_journal_start(reset_reason);
    const runtime_diag_event_t boot_event = {
        .event_type = RUNTIME_DIAG_EVENT_BOOT_START,
        .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
        .source = 1U,
        .reset_reason = (uint16_t)reset_reason,
    };
    (void)runtime_diag_journal_record(&boot_event);
    const panel_ota_handoff_result_t ota_result = panel_ota_handoff_take_result();
    if (ota_result != PANEL_OTA_HANDOFF_RESULT_NONE) {
        const runtime_diag_event_t result_event = {
            .event_type = RUNTIME_DIAG_EVENT_OTA_MAINTENANCE_RESULT,
            .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
            .source = 1U,
            .result = (uint16_t)ota_result,
        };
        (void)runtime_diag_journal_record(&result_event);
        ESP_LOGW(TAG, "OTA previous_attempt=%s", panel_ota_handoff_result_name(ota_result));
    }

    if (!panel_nvs_startup_gate()) {
        return;
    }

    /* OTA first-boot acceptance MUST precede bsp_display_start() and Homey. */
    if (!panel_ota_first_boot_gate_after_nvs()) {
        ESP_LOGE(TAG, "OTA first-boot gate stopped normal startup");
        return;
    }

    ESP_LOGI(TAG, "Patch 005H.1 secure local hardware bootstrap");
    ESP_LOGI(TAG, "Real Athom OAuth disabled; synthetic phone provisioning foundation enabled");

    if (secure_bootstrap_start() != SECURE_BOOTSTRAP_OK) {
        ESP_LOGE(TAG, "Secure bootstrap failed");
    }
}
