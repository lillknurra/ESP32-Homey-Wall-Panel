#include "panel_awning_write_guard.h"

#include <stddef.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "nvs.h"
#endif

#define GUARD_MAGIC 0x41574731U /* AWG1 */
#define GUARD_VERSION 1U
#define GUARD_NAMESPACE "hpanel_awning"
#define GUARD_KEY "guard_v1"

static uint32_t guard_crc32(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0U; i < size; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0U; bit < 8U; ++bit) {
            crc = (crc >> 1U) ^ ((0U - (crc & 1U)) & 0xedb88320U);
        }
    }
    return ~crc;
}

static uint32_t record_crc(const panel_awning_guard_record_t *record)
{
    panel_awning_guard_record_t copy = *record;
    copy.crc32 = 0U;
    return guard_crc32(&copy, sizeof(copy));
}

bool panel_awning_write_guard_record_valid(
    const panel_awning_guard_record_t *record)
{
    if (record == NULL || record->magic != GUARD_MAGIC ||
        record->version != GUARD_VERSION || record->size != sizeof(*record) ||
        (record->state != PANEL_AWNING_GUARD_READY &&
         record->state != PANEL_AWNING_GUARD_WRITE_PENDING) ||
        record->sequence == 0U || record->alias_generation == 0U ||
        record->reserved[0] != 0U || record->reserved[1] != 0U ||
        record->reserved[2] != 0U || record->crc32 != record_crc(record)) {
        return false;
    }
    uint8_t identity_or = 0U;
    for (size_t i = 0U; i < sizeof(record->homey_identity); ++i)
        identity_or |= record->homey_identity[i];
    return identity_or != 0U;
}

static void seal_record(panel_awning_guard_record_t *record)
{
    record->magic = GUARD_MAGIC;
    record->version = GUARD_VERSION;
    record->size = (uint16_t)sizeof(*record);
    record->reserved[0] = 0U;
    record->reserved[1] = 0U;
    record->reserved[2] = 0U;
    record->crc32 = 0U;
    record->crc32 = record_crc(record);
}

esp_err_t panel_awning_write_guard_load(panel_awning_guard_record_t *out)
{
    if (out == NULL) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
#ifdef ESP_PLATFORM
    nvs_handle_t handle;
    esp_err_t err = nvs_open(GUARD_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND || err == ESP_ERR_NOT_FOUND)
        return ESP_ERR_NOT_FOUND;
    if (err != ESP_OK) return err;
    size_t size = sizeof(*out);
    err = nvs_get_blob(handle, GUARD_KEY, out, &size);
    nvs_close(handle);
    if (err != ESP_OK) {
        memset(out, 0, sizeof(*out));
        return (err == ESP_ERR_NVS_NOT_FOUND || err == ESP_ERR_NOT_FOUND)
            ? ESP_ERR_NOT_FOUND : err;
    }
    if (size != sizeof(*out) || !panel_awning_write_guard_record_valid(out)) {
        memset(out, 0, sizeof(*out));
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t panel_awning_write_guard_initialize_absent(
    const uint8_t homey_identity[PANEL_AWNING_GUARD_IDENTITY_SIZE],
    uint32_t alias_generation,
    panel_awning_guard_record_t *ready_out)
{
    if (homey_identity == NULL || alias_generation == 0U || ready_out == NULL)
        return ESP_ERR_INVALID_ARG;
    uint8_t identity_or = 0U;
    for (size_t i = 0U; i < PANEL_AWNING_GUARD_IDENTITY_SIZE; ++i)
        identity_or |= homey_identity[i];
    if (identity_or == 0U) return ESP_ERR_INVALID_ARG;
#ifdef ESP_PLATFORM
    panel_awning_guard_record_t current = {0};
    esp_err_t err = panel_awning_write_guard_load(&current);
    if (err != ESP_ERR_NOT_FOUND) {
        memset(&current, 0, sizeof(current));
        return err == ESP_OK ? ESP_ERR_INVALID_STATE : err;
    }

    panel_awning_guard_record_t ready = {0};
    ready.state = PANEL_AWNING_GUARD_READY;
    ready.sequence = 1U;
    memcpy(ready.homey_identity, homey_identity, sizeof(ready.homey_identity));
    ready.alias_generation = alias_generation;
    seal_record(&ready);

    nvs_handle_t handle;
    err = nvs_open(GUARD_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        /* Recheck absence through the writable handle immediately before set.
         * The runtime's service reservation serializes all guard writers. */
        panel_awning_guard_record_t existing = {0};
        size_t existing_size = sizeof(existing);
        const esp_err_t existing_err = nvs_get_blob(
            handle, GUARD_KEY, &existing, &existing_size);
        memset(&existing, 0, sizeof(existing));
        if (existing_err != ESP_ERR_NVS_NOT_FOUND &&
            existing_err != ESP_ERR_NOT_FOUND) {
            err = existing_err == ESP_OK ? ESP_ERR_INVALID_STATE : existing_err;
        } else {
            err = nvs_set_blob(handle, GUARD_KEY, &ready, sizeof(ready));
            if (err == ESP_OK) err = nvs_commit(handle);
        }
        nvs_close(handle);
    }
    if (err == ESP_OK) {
        panel_awning_guard_record_t readback = {0};
        err = panel_awning_write_guard_load(&readback);
        if (err == ESP_OK && memcmp(&readback, &ready, sizeof(ready)) != 0)
            err = ESP_ERR_INVALID_CRC;
        if (err == ESP_OK) *ready_out = readback;
        memset(&readback, 0, sizeof(readback));
    }
    memset(&ready, 0, sizeof(ready));
    return err;
#else
    (void)ready_out;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t write_and_verify(
    const panel_awning_guard_record_t *expected,
    const panel_awning_guard_record_t *replacement,
    panel_awning_guard_record_t *verified_out)
{
#ifdef ESP_PLATFORM
    panel_awning_guard_record_t current;
    esp_err_t err = panel_awning_write_guard_load(&current);
    if (err != ESP_OK) return err;
    if (memcmp(&current, expected, sizeof(current)) != 0) {
        memset(&current, 0, sizeof(current));
        return ESP_ERR_INVALID_STATE;
    }
    memset(&current, 0, sizeof(current));

    nvs_handle_t handle;
    err = nvs_open(GUARD_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(handle, GUARD_KEY, replacement, sizeof(*replacement));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err != ESP_OK) return err;

    panel_awning_guard_record_t readback;
    err = panel_awning_write_guard_load(&readback);
    if (err == ESP_OK && memcmp(&readback, replacement, sizeof(readback)) != 0)
        err = ESP_ERR_INVALID_CRC;
    if (err == ESP_OK && verified_out != NULL) *verified_out = readback;
    memset(&readback, 0, sizeof(readback));
    return err;
#else
    (void)expected;
    (void)replacement;
    (void)verified_out;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t panel_awning_write_guard_begin(
    const panel_awning_guard_record_t *expected_ready,
    panel_awning_guard_record_t *pending_out)
{
    if (expected_ready == NULL || pending_out == NULL ||
        !panel_awning_write_guard_record_valid(expected_ready) ||
        expected_ready->state != PANEL_AWNING_GUARD_READY) {
        return ESP_ERR_INVALID_ARG;
    }
    if (expected_ready->sequence == UINT32_MAX) return ESP_ERR_INVALID_STATE;
    panel_awning_guard_record_t pending = *expected_ready;
    pending.state = PANEL_AWNING_GUARD_WRITE_PENDING;
    pending.sequence++;
    seal_record(&pending);
    const esp_err_t err = write_and_verify(expected_ready, &pending, pending_out);
    memset(&pending, 0, sizeof(pending));
    return err;
}

esp_err_t panel_awning_write_guard_resolve(
    const panel_awning_guard_record_t *expected_pending,
    panel_awning_guard_record_t *ready_out)
{
    if (expected_pending == NULL || ready_out == NULL ||
        !panel_awning_write_guard_record_valid(expected_pending) ||
        expected_pending->state != PANEL_AWNING_GUARD_WRITE_PENDING) {
        return ESP_ERR_INVALID_ARG;
    }
    panel_awning_guard_record_t ready = *expected_pending;
    ready.state = PANEL_AWNING_GUARD_READY;
    seal_record(&ready);
    const esp_err_t err = write_and_verify(expected_pending, &ready, ready_out);
    memset(&ready, 0, sizeof(ready));
    return err;
}
