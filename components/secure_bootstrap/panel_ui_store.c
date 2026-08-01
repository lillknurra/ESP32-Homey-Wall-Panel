#include "panel_ui_store.h"

#include <limits.h>
#include <string.h>

#define STORE_HEADER_AND_PAYLOAD_SIZE 28U
#define STORE_FLAG_WAKE_ON_TOUCH UINT8_C(0x01)
#define STORE_KNOWN_FLAGS STORE_FLAG_WAKE_ON_TOUCH
#define STORE_TIMEZONE_EUROPE_STOCKHOLM UINT8_C(0)

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void write_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & UINT16_C(0x00ff));
    p[1] = (uint8_t)((value >> 8) & UINT16_C(0x00ff));
}

static void write_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & UINT32_C(0x000000ff));
    p[1] = (uint8_t)((value >> 8) & UINT32_C(0x000000ff));
    p[2] = (uint8_t)((value >> 16) & UINT32_C(0x000000ff));
    p[3] = (uint8_t)((value >> 24) & UINT32_C(0x000000ff));
}

bool panel_ui_store_crc32(const uint8_t *data, size_t data_size, uint32_t *crc_out)
{
    if (crc_out == NULL || (data == NULL && data_size != 0U)) {
        return false;
    }

    uint32_t crc = UINT32_C(0xffffffff);
    for (size_t index = 0U; index < data_size; ++index) {
        crc ^= data[index];
        for (unsigned bit = 0U; bit < 8U; ++bit) {
            const uint32_t mask = (uint32_t)(-(int32_t)(crc & UINT32_C(1)));
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & mask);
        }
    }
    *crc_out = crc ^ UINT32_C(0xffffffff);
    return true;
}

bool panel_ui_store_encode(const panel_ui_settings_t *settings,
                           uint32_t generation,
                           uint8_t *record,
                           size_t record_size)
{
    if (settings == NULL || record == NULL || record_size != PANEL_UI_STORE_RECORD_SIZE || generation == 0U) {
        return false;
    }

    panel_ui_settings_t normalized = *settings;
    panel_ui_settings_normalize(&normalized);

    memset(record, 0, record_size);
    write_le32(&record[0], PANEL_UI_STORE_MAGIC);
    write_le16(&record[4], PANEL_UI_STORE_SCHEMA_VERSION);
    write_le16(&record[6], PANEL_UI_STORE_RECORD_SIZE);
    write_le32(&record[8], generation);
    record[12] = normalized.normal_brightness;
    record[13] = normalized.dimmed_brightness;
    record[14] = normalized.wake_on_touch ? STORE_FLAG_WAKE_ON_TOUCH : UINT8_C(0);
    record[15] = (uint8_t)normalized.background_mode;
    write_le32(&record[16], normalized.dim_after_seconds);
    write_le32(&record[20], normalized.off_after_seconds);
    record[24] = STORE_TIMEZONE_EUROPE_STOCKHOLM;

    uint32_t crc = 0U;
    if (!panel_ui_store_crc32(record, STORE_HEADER_AND_PAYLOAD_SIZE, &crc)) {
        return false;
    }
    write_le32(&record[28], crc);
    return true;
}

bool panel_ui_store_decode(const uint8_t *record,
                           size_t record_size,
                           panel_ui_settings_t *settings_out,
                           uint32_t *generation_out)
{
    if (record == NULL || settings_out == NULL || generation_out == NULL || record_size != PANEL_UI_STORE_RECORD_SIZE) {
        return false;
    }
    if (read_le32(&record[0]) != PANEL_UI_STORE_MAGIC ||
        read_le16(&record[4]) != PANEL_UI_STORE_SCHEMA_VERSION ||
        read_le16(&record[6]) != PANEL_UI_STORE_RECORD_SIZE) {
        return false;
    }

    const uint32_t generation = read_le32(&record[8]);
    if (generation == 0U || (record[14] & (uint8_t)~STORE_KNOWN_FLAGS) != 0U ||
        record[15] > (uint8_t)PANEL_BACKGROUND_BUILT_IN ||
        record[24] != STORE_TIMEZONE_EUROPE_STOCKHOLM ||
        record[25] != 0U || record[26] != 0U || record[27] != 0U) {
        return false;
    }

    uint32_t expected_crc = 0U;
    if (!panel_ui_store_crc32(record, STORE_HEADER_AND_PAYLOAD_SIZE, &expected_crc) ||
        expected_crc != read_le32(&record[28])) {
        return false;
    }

    panel_ui_settings_t decoded;
    panel_ui_settings_defaults(&decoded);
    decoded.normal_brightness = record[12];
    decoded.dimmed_brightness = record[13];
    decoded.wake_on_touch = (record[14] & STORE_FLAG_WAKE_ON_TOUCH) != 0U;
    decoded.background_mode = (panel_background_mode_t)record[15];
    decoded.dim_after_seconds = read_le32(&record[16]);
    decoded.off_after_seconds = read_le32(&record[20]);
    panel_ui_settings_normalize(&decoded);

    *settings_out = decoded;
    *generation_out = generation;
    return true;
}

static bool generation_is_newer(uint32_t a, uint32_t b)
{
    return (int32_t)(a - b) > 0;
}

panel_ui_store_slot_t panel_ui_store_select_slot(const panel_ui_store_candidate_t *slot_a,
                                                 const panel_ui_store_candidate_t *slot_b,
                                                 panel_ui_store_slot_t active_hint)
{
    const bool a_valid = slot_a != NULL && slot_a->valid && slot_a->slot == PANEL_UI_STORE_SLOT_A && slot_a->generation != 0U;
    const bool b_valid = slot_b != NULL && slot_b->valid && slot_b->slot == PANEL_UI_STORE_SLOT_B && slot_b->generation != 0U;

    if (active_hint == PANEL_UI_STORE_SLOT_A && a_valid) {
        return PANEL_UI_STORE_SLOT_A;
    }
    if (active_hint == PANEL_UI_STORE_SLOT_B && b_valid) {
        return PANEL_UI_STORE_SLOT_B;
    }
    if (a_valid && !b_valid) {
        return PANEL_UI_STORE_SLOT_A;
    }
    if (b_valid && !a_valid) {
        return PANEL_UI_STORE_SLOT_B;
    }
    if (!a_valid && !b_valid) {
        return PANEL_UI_STORE_SLOT_NONE;
    }
    if (slot_a->generation == slot_b->generation) {
        return PANEL_UI_STORE_SLOT_A;
    }
    return generation_is_newer(slot_a->generation, slot_b->generation)
               ? PANEL_UI_STORE_SLOT_A
               : PANEL_UI_STORE_SLOT_B;
}

#ifdef ESP_PLATFORM
#include "nvs.h"

static bool settings_equal(const panel_ui_settings_t *a, const panel_ui_settings_t *b)
{
    return a != NULL && b != NULL &&
           a->normal_brightness == b->normal_brightness &&
           a->dimmed_brightness == b->dimmed_brightness &&
           a->dim_after_seconds == b->dim_after_seconds &&
           a->off_after_seconds == b->off_after_seconds &&
           a->wake_on_touch == b->wake_on_touch &&
           a->background_mode == b->background_mode &&
           strcmp(a->timezone_id, b->timezone_id) == 0;
}

static const char *slot_key(panel_ui_store_slot_t slot)
{
    return slot == PANEL_UI_STORE_SLOT_A ? PANEL_UI_STORE_KEY_SLOT_A : PANEL_UI_STORE_KEY_SLOT_B;
}

static bool read_candidate(nvs_handle_t handle, panel_ui_store_slot_t slot, panel_ui_store_candidate_t *candidate, bool *found)
{
    if (candidate == NULL || found == NULL) {
        return false;
    }

    memset(candidate, 0, sizeof(*candidate));
    candidate->slot = slot;
    *found = false;

    size_t size = 0U;
    esp_err_t error = nvs_get_blob(handle, slot_key(slot), NULL, &size);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (error != ESP_OK) {
        return false;
    }

    *found = true;
    if (size != PANEL_UI_STORE_RECORD_SIZE) {
        return true;
    }

    uint8_t record[PANEL_UI_STORE_RECORD_SIZE];
    error = nvs_get_blob(handle, slot_key(slot), record, &size);
    if (error != ESP_OK) {
        return false;
    }

    candidate->valid = panel_ui_store_decode(
        record,
        size,
        &candidate->settings,
        &candidate->generation);
    return true;
}

panel_ui_store_result_t panel_ui_store_load(panel_ui_settings_t *settings_out)
{
    if (settings_out == NULL) {
        return PANEL_UI_STORE_RESULT_INVALID_ARGUMENT;
    }
    panel_ui_settings_defaults(settings_out);

    nvs_handle_t handle;
    esp_err_t error = nvs_open(PANEL_UI_STORE_NAMESPACE, NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        return PANEL_UI_STORE_RESULT_DEFAULTED_NOT_FOUND;
    }
    if (error != ESP_OK) {
        return PANEL_UI_STORE_RESULT_IO_ERROR;
    }

    panel_ui_store_candidate_t a;
    panel_ui_store_candidate_t b;
    bool found_a = false;
    bool found_b = false;
    if (!read_candidate(handle, PANEL_UI_STORE_SLOT_A, &a, &found_a) ||
        !read_candidate(handle, PANEL_UI_STORE_SLOT_B, &b, &found_b)) {
        nvs_close(handle);
        return PANEL_UI_STORE_RESULT_IO_ERROR;
    }

    uint8_t active_raw = (uint8_t)PANEL_UI_STORE_SLOT_NONE;
    error = nvs_get_u8(handle, PANEL_UI_STORE_KEY_ACTIVE, &active_raw);
    if (error != ESP_OK && error != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return PANEL_UI_STORE_RESULT_IO_ERROR;
    }
    nvs_close(handle);

    panel_ui_store_slot_t active = active_raw <= (uint8_t)PANEL_UI_STORE_SLOT_B
                                       ? (panel_ui_store_slot_t)active_raw
                                       : PANEL_UI_STORE_SLOT_NONE;
    const panel_ui_store_slot_t selected = panel_ui_store_select_slot(&a, &b, active);
    if (selected == PANEL_UI_STORE_SLOT_A) {
        *settings_out = a.settings;
        return PANEL_UI_STORE_RESULT_OK;
    }
    if (selected == PANEL_UI_STORE_SLOT_B) {
        *settings_out = b.settings;
        return PANEL_UI_STORE_RESULT_OK;
    }
    return (!found_a && !found_b) ? PANEL_UI_STORE_RESULT_DEFAULTED_NOT_FOUND
                                  : PANEL_UI_STORE_RESULT_DEFAULTED_INVALID;
}

panel_ui_store_result_t panel_ui_store_save(const panel_ui_settings_t *settings)
{
    if (settings == NULL) {
        return PANEL_UI_STORE_RESULT_INVALID_ARGUMENT;
    }

    panel_ui_settings_t normalized = *settings;
    panel_ui_settings_normalize(&normalized);

    nvs_handle_t handle;
    if (nvs_open(PANEL_UI_STORE_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        return PANEL_UI_STORE_RESULT_IO_ERROR;
    }

    panel_ui_store_candidate_t a;
    panel_ui_store_candidate_t b;
    bool found_a = false;
    bool found_b = false;
    if (!read_candidate(handle, PANEL_UI_STORE_SLOT_A, &a, &found_a) ||
        !read_candidate(handle, PANEL_UI_STORE_SLOT_B, &b, &found_b)) {
        nvs_close(handle);
        return PANEL_UI_STORE_RESULT_IO_ERROR;
    }

    uint8_t active_raw = (uint8_t)PANEL_UI_STORE_SLOT_NONE;
    esp_err_t error = nvs_get_u8(handle, PANEL_UI_STORE_KEY_ACTIVE, &active_raw);
    if (error != ESP_OK && error != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return PANEL_UI_STORE_RESULT_IO_ERROR;
    }
    panel_ui_store_slot_t active = active_raw <= (uint8_t)PANEL_UI_STORE_SLOT_B
                                       ? (panel_ui_store_slot_t)active_raw
                                       : PANEL_UI_STORE_SLOT_NONE;
    const panel_ui_store_slot_t current = panel_ui_store_select_slot(&a, &b, active);
    const panel_ui_store_slot_t target = current == PANEL_UI_STORE_SLOT_A ? PANEL_UI_STORE_SLOT_B : PANEL_UI_STORE_SLOT_A;

    uint32_t generation = 1U;
    if (current == PANEL_UI_STORE_SLOT_A) {
        generation = a.generation == UINT32_MAX ? 1U : a.generation + 1U;
    } else if (current == PANEL_UI_STORE_SLOT_B) {
        generation = b.generation == UINT32_MAX ? 1U : b.generation + 1U;
    }

    uint8_t record[PANEL_UI_STORE_RECORD_SIZE];
    if (!panel_ui_store_encode(&normalized, generation, record, sizeof(record)) ||
        nvs_set_blob(handle, slot_key(target), record, sizeof(record)) != ESP_OK ||
        nvs_commit(handle) != ESP_OK) {
        nvs_close(handle);
        return PANEL_UI_STORE_RESULT_IO_ERROR;
    }

    uint8_t readback[PANEL_UI_STORE_RECORD_SIZE];
    size_t readback_size = sizeof(readback);
    if (nvs_get_blob(handle, slot_key(target), readback, &readback_size) != ESP_OK ||
        readback_size != sizeof(readback) || memcmp(record, readback, sizeof(record)) != 0) {
        nvs_close(handle);
        return PANEL_UI_STORE_RESULT_VERIFY_ERROR;
    }

    panel_ui_settings_t decoded;
    uint32_t decoded_generation = 0U;
    if (!panel_ui_store_decode(readback, readback_size, &decoded, &decoded_generation) ||
        decoded_generation != generation || !settings_equal(&normalized, &decoded)) {
        nvs_close(handle);
        return PANEL_UI_STORE_RESULT_VERIFY_ERROR;
    }

    if (nvs_set_u8(handle, PANEL_UI_STORE_KEY_ACTIVE, (uint8_t)target) != ESP_OK ||
        nvs_commit(handle) != ESP_OK) {
        nvs_close(handle);
        return PANEL_UI_STORE_RESULT_IO_ERROR;
    }

    uint8_t active_readback = (uint8_t)PANEL_UI_STORE_SLOT_NONE;
    if (nvs_get_u8(handle, PANEL_UI_STORE_KEY_ACTIVE, &active_readback) != ESP_OK ||
        active_readback != (uint8_t)target) {
        nvs_close(handle);
        return PANEL_UI_STORE_RESULT_VERIFY_ERROR;
    }

    nvs_close(handle);
    return PANEL_UI_STORE_RESULT_OK;
}
#endif
