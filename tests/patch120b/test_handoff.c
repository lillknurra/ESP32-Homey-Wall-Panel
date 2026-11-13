/* Production-linked RTC handoff tests; host shims only define IDF attributes/constants. */
#include <assert.h>
#include <stdio.h>

#include "../../components/secure_bootstrap/panel_ota_handoff.c"

int main(void)
{
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_NONE);
    assert(panel_ota_handoff_arm_stable_update());
    assert(!panel_ota_handoff_consume_for_software_reset(ESP_RST_SW + 1));
    /* Wrong reset reason consumes the intent so it cannot replay later. */
    assert(!panel_ota_handoff_consume_for_software_reset(ESP_RST_SW));

    assert(panel_ota_handoff_arm_stable_update());
    assert(panel_ota_handoff_consume_for_software_reset(ESP_RST_SW));
    assert(!panel_ota_handoff_consume_for_software_reset(ESP_RST_SW));

    assert(panel_ota_handoff_arm_stable_update());
    s_handoff.crc32 ^= 1U;
    assert(!panel_ota_handoff_consume_for_software_reset(ESP_RST_SW));

    panel_ota_handoff_record_result(PANEL_OTA_HANDOFF_RESULT_WIFI_FAILED);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_WIFI_FAILED);
    assert(panel_ota_handoff_last_runtime_result() == PANEL_OTA_HANDOFF_RESULT_WIFI_FAILED);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_NONE);
    assert(panel_ota_handoff_last_runtime_result() == PANEL_OTA_HANDOFF_RESULT_NONE);
    panel_ota_handoff_record_result(PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT);
    panel_ota_handoff_record_result(PANEL_OTA_HANDOFF_RESULT_BOOT_CONFIRMED);
    assert(panel_ota_handoff_last_runtime_result() == PANEL_OTA_HANDOFF_RESULT_BOOT_CONFIRMED);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_BOOT_CONFIRMED);
    panel_ota_handoff_record_result(PANEL_OTA_HANDOFF_RESULT_ROLLBACK_REQUESTED);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_ROLLBACK_REQUESTED);
    panel_ota_handoff_record_result((panel_ota_handoff_result_t)UINT32_MAX);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_NONE);
    puts("PATCH120B_PRODUCTION_HANDOFF=PASS");
    return 0;
}
