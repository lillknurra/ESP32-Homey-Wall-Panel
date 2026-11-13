#include "panel_ota_handoff.h"

#include <stddef.h>
#include <string.h>
#include "esp_attr.h"
#include "esp_system.h"

#define HANDOFF_MAGIC UINT32_C(0x50313230)
#define RESULT_MAGIC UINT32_C(0x50313252)
#define HANDOFF_SCHEMA 1U
#define CHANNEL_STABLE 1U

typedef struct {
    uint32_t magic;
    uint16_t schema;
    uint8_t channel;
    uint8_t state;
    uint32_t expected_reset_reason;
    uint32_t crc32;
} handoff_record_t;

typedef struct {
    uint32_t magic;
    uint32_t result;
    uint32_t crc32;
} result_record_t;

RTC_NOINIT_ATTR static handoff_record_t s_handoff;
RTC_NOINIT_ATTR static result_record_t s_result;
static panel_ota_handoff_result_t s_runtime_result;

static bool result_valid(panel_ota_handoff_result_t result)
{
    return result >= PANEL_OTA_HANDOFF_RESULT_NONE &&
        result <= PANEL_OTA_HANDOFF_RESULT_ROLLBACK_REQUESTED;
}

static uint32_t crc32_bytes(const void *data, size_t length)
{
    const uint8_t *p = data;
    uint32_t crc = UINT32_C(0xffffffff);
    for (size_t i = 0; i < length; ++i) {
        crc ^= p[i];
        for (unsigned bit = 0; bit < 8U; ++bit)
            crc = (crc >> 1U) ^ (UINT32_C(0xedb88320) & (0U - (crc & 1U)));
    }
    return ~crc;
}

bool panel_ota_handoff_arm_stable_update(void)
{
    memset(&s_handoff, 0, sizeof(s_handoff));
    s_handoff.magic = HANDOFF_MAGIC;
    s_handoff.schema = HANDOFF_SCHEMA;
    s_handoff.channel = CHANNEL_STABLE;
    s_handoff.state = 1U;
    s_handoff.expected_reset_reason = (uint32_t)ESP_RST_SW;
    s_handoff.crc32 = crc32_bytes(&s_handoff, offsetof(handoff_record_t, crc32));
    return true;
}

bool panel_ota_handoff_consume_for_software_reset(int reset_reason)
{
    handoff_record_t copy = s_handoff;
    const bool valid = reset_reason == (int)ESP_RST_SW &&
        copy.magic == HANDOFF_MAGIC && copy.schema == HANDOFF_SCHEMA &&
        copy.channel == CHANNEL_STABLE && copy.state == 1U &&
        copy.expected_reset_reason == (uint32_t)reset_reason &&
        copy.crc32 == crc32_bytes(&copy, offsetof(handoff_record_t, crc32));
    /* Clear first. The intent cannot survive a second boot or network failure. */
    memset(&s_handoff, 0, sizeof(s_handoff));
    return valid;
}

void panel_ota_handoff_record_result(panel_ota_handoff_result_t result)
{
    if (!result_valid(result)) {
        memset(&s_result, 0, sizeof(s_result));
        s_runtime_result = PANEL_OTA_HANDOFF_RESULT_NONE;
        return;
    }
    memset(&s_result, 0, sizeof(s_result));
    s_result.magic = RESULT_MAGIC;
    s_result.result = (uint32_t)result;
    s_result.crc32 = crc32_bytes(&s_result, offsetof(result_record_t, crc32));
    s_runtime_result = result;
}

panel_ota_handoff_result_t panel_ota_handoff_take_result(void)
{
    const result_record_t copy = s_result;
    memset(&s_result, 0, sizeof(s_result));
    if (copy.magic != RESULT_MAGIC ||
        copy.crc32 != crc32_bytes(&copy, offsetof(result_record_t, crc32)) ||
        copy.result > PANEL_OTA_HANDOFF_RESULT_ROLLBACK_REQUESTED) {
        s_runtime_result = PANEL_OTA_HANDOFF_RESULT_NONE;
    } else {
        s_runtime_result = (panel_ota_handoff_result_t)copy.result;
    }
    return s_runtime_result;
}

panel_ota_handoff_result_t panel_ota_handoff_last_runtime_result(void)
{
    return s_runtime_result;
}

const char *panel_ota_handoff_result_name(panel_ota_handoff_result_t result)
{
    switch (result) {
    case PANEL_OTA_HANDOFF_RESULT_NO_RELEASE: return "no_release";
    case PANEL_OTA_HANDOFF_RESULT_WIFI_FAILED: return "wifi_failed";
    case PANEL_OTA_HANDOFF_RESULT_TIME_FAILED: return "time_failed";
    case PANEL_OTA_HANDOFF_RESULT_MANIFEST_REJECTED: return "manifest_rejected";
    case PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED: return "stage_failed";
    case PANEL_OTA_HANDOFF_RESULT_STAGED: return "staged";
    case PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT: return "supervisor_timeout";
    case PANEL_OTA_HANDOFF_RESULT_BOOT_CONFIRMED: return "boot_confirmed";
    case PANEL_OTA_HANDOFF_RESULT_ROLLBACK_REQUESTED: return "rollback_requested";
    case PANEL_OTA_HANDOFF_RESULT_NONE:
    default: return "none";
    }
}
