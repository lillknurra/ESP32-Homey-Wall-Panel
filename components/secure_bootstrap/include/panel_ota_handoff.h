#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PANEL_OTA_HANDOFF_RESULT_NONE = 0,
    PANEL_OTA_HANDOFF_RESULT_NO_RELEASE,
    PANEL_OTA_HANDOFF_RESULT_WIFI_FAILED,
    PANEL_OTA_HANDOFF_RESULT_TIME_FAILED,
    PANEL_OTA_HANDOFF_RESULT_MANIFEST_REJECTED,
    PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED,
    PANEL_OTA_HANDOFF_RESULT_STAGED,
    PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT,
    PANEL_OTA_HANDOFF_RESULT_BOOT_CONFIRMED,
    PANEL_OTA_HANDOFF_RESULT_ROLLBACK_REQUESTED,
} panel_ota_handoff_result_t;

/* RTC memory provides a one-boot hint only. CRC detects accidental corruption;
 * it is not authentication and is never accepted without a software reset. */
bool panel_ota_handoff_arm_stable_update(void);
bool panel_ota_handoff_consume_for_software_reset(int reset_reason);
void panel_ota_handoff_record_result(panel_ota_handoff_result_t result);
panel_ota_handoff_result_t panel_ota_handoff_take_result(void);
panel_ota_handoff_result_t panel_ota_handoff_last_runtime_result(void);
const char *panel_ota_handoff_result_name(panel_ota_handoff_result_t result);
