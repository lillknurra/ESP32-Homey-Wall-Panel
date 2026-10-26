#include "secure_bootstrap.h"
#include "runtime_diag_journal.h"
#include "nvs_startup_gate.h"
#include "panel_ota_first_boot_gate.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

static const char *TAG = "homey_wall_panel";

void app_main(void)
{
    const runtime_diag_reset_reason_t reset_reason =
        runtime_diag_map_reset_reason((int)esp_reset_reason());
    (void)runtime_diag_journal_start(reset_reason);
    const runtime_diag_event_t boot_event = {
        .event_type = RUNTIME_DIAG_EVENT_BOOT_START,
        .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
        .source = 1U,
        .reset_reason = (uint16_t)reset_reason,
    };
    (void)runtime_diag_journal_record(&boot_event);

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
