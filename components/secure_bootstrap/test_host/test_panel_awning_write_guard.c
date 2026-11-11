#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "panel_awning_write_guard.h"
#include "nvs.h"

static uint8_t s_durable_blob[sizeof(panel_awning_guard_record_t)];
static bool s_durable_present;
static uint8_t s_staged_blob[sizeof(panel_awning_guard_record_t)];
static bool s_staged_present;
static int s_fault;
static bool s_corrupt_next_read;
static unsigned s_commit_count;

enum {
    FAULT_NONE = 0,
    FAULT_SET,
    FAULT_COMMIT_BEFORE_DURABLE,
    FAULT_COMMIT_AFTER_DURABLE,
    FAULT_READBACK_CORRUPT,
    FAULT_OPEN,
    FAULT_GET,
};

static uint32_t test_crc32(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0U; i < size; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0U; bit < 8U; ++bit)
            crc = (crc >> 1U) ^ ((0U - (crc & 1U)) & 0xedb88320U);
    }
    return ~crc;
}

esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle)
{
    assert(strcmp(name, "hpanel_awning") == 0);
    if (s_fault == FAULT_OPEN) return ESP_FAIL;
    *handle = (nvs_handle_t)mode;
    if (mode == NVS_READWRITE) {
        memcpy(s_staged_blob, s_durable_blob, sizeof(s_staged_blob));
        s_staged_present = s_durable_present;
    }
    return ESP_OK;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out, size_t *size)
{
    (void)handle;
    assert(strcmp(key, "guard_v1") == 0);
    if (s_fault == FAULT_GET) return ESP_FAIL;
    if (!s_durable_present) return ESP_ERR_NVS_NOT_FOUND;
    if (*size < sizeof(s_durable_blob)) return ESP_ERR_INVALID_SIZE;
    memcpy(out, s_durable_blob, sizeof(s_durable_blob));
    *size = sizeof(s_durable_blob);
    if (s_corrupt_next_read) {
        ((uint8_t *)out)[0] ^= 0x01U;
        s_corrupt_next_read = false;
    }
    return ESP_OK;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key,
                       const void *data, size_t size)
{
    (void)handle;
    assert(strcmp(key, "guard_v1") == 0);
    if (s_fault == FAULT_SET) return ESP_FAIL;
    assert(size == sizeof(s_staged_blob));
    memcpy(s_staged_blob, data, size);
    s_staged_present = true;
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    s_commit_count++;
    if (s_fault == FAULT_COMMIT_BEFORE_DURABLE) return ESP_FAIL;
    if (s_staged_present) {
        memcpy(s_durable_blob, s_staged_blob, sizeof(s_durable_blob));
        s_durable_present = true;
    }
    if (s_fault == FAULT_COMMIT_AFTER_DURABLE) return ESP_FAIL;
    if (s_fault == FAULT_READBACK_CORRUPT) s_corrupt_next_read = true;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) { (void)handle; }

static void seed_ready(uint32_t sequence)
{
    panel_awning_guard_record_t ready = {0};
    ready.magic = 0x41574731U;
    ready.version = 1U;
    ready.size = (uint16_t)sizeof(ready);
    ready.state = PANEL_AWNING_GUARD_READY;
    ready.sequence = sequence;
    memset(ready.homey_identity, 0xA7, sizeof(ready.homey_identity));
    ready.alias_generation = 9U;
    ready.crc32 = 0U;
    ready.crc32 = test_crc32(&ready, sizeof(ready));
    memcpy(s_durable_blob, &ready, sizeof(ready));
    s_durable_present = true;
}

static void test_absence_and_corruption_fail_closed(void)
{
    panel_awning_guard_record_t record;
    memset(&record, 0xAA, sizeof(record));
    s_durable_present = false;
    assert(panel_awning_write_guard_load(&record) == ESP_ERR_NOT_FOUND);
    assert(panel_awning_write_guard_begin(&record, &record) == ESP_ERR_INVALID_ARG);

    seed_ready(3U);
    ((panel_awning_guard_record_t *)s_durable_blob)->version = 99U;
    assert(panel_awning_write_guard_load(&record) == ESP_ERR_INVALID_CRC);
    seed_ready(3U);
    s_durable_blob[sizeof(s_durable_blob) - 1U] ^= 0x80U;
    assert(panel_awning_write_guard_load(&record) == ESP_ERR_INVALID_CRC);
    s_fault = FAULT_OPEN;
    assert(panel_awning_write_guard_load(&record) == ESP_FAIL);
    s_fault = FAULT_GET;
    assert(panel_awning_write_guard_load(&record) == ESP_FAIL);
    s_fault = FAULT_NONE;
}

static void test_commit_barrier_and_resolution(void)
{
    panel_awning_guard_record_t ready = {0}, pending = {0}, resolved = {0};
    seed_ready(41U);
    assert(panel_awning_write_guard_load(&ready) == ESP_OK);
    assert(ready.state == PANEL_AWNING_GUARD_READY && ready.sequence == 41U);
    assert(panel_awning_write_guard_begin(&ready, &pending) == ESP_OK);
    assert(pending.state == PANEL_AWNING_GUARD_WRITE_PENDING &&
           pending.sequence == 42U);
    assert(s_commit_count == 1U);
    assert(panel_awning_write_guard_load(&resolved) == ESP_OK);
    assert(memcmp(&pending, &resolved, sizeof(pending)) == 0);

    panel_awning_guard_record_t stale = ready;
    stale.sequence++;
    stale.crc32 = 0U;
    stale.crc32 = test_crc32(&stale, sizeof(stale));
    assert(panel_awning_write_guard_begin(&stale, &resolved) == ESP_ERR_INVALID_STATE);
    assert(panel_awning_write_guard_resolve(&pending, &resolved) == ESP_OK);
    assert(resolved.state == PANEL_AWNING_GUARD_READY && resolved.sequence == 42U);
    assert(panel_awning_write_guard_load(&pending) == ESP_OK);
    assert(memcmp(&resolved, &pending, sizeof(resolved)) == 0);
}

static void test_ambiguous_nvs_failures(void)
{
    panel_awning_guard_record_t ready = {0}, result = {0};
    seed_ready(1U);
    assert(panel_awning_write_guard_load(&ready) == ESP_OK);

    s_fault = FAULT_SET;
    assert(panel_awning_write_guard_begin(&ready, &result) == ESP_FAIL);
    assert(panel_awning_write_guard_load(&result) == ESP_OK);
    assert(result.state == PANEL_AWNING_GUARD_READY);

    s_fault = FAULT_COMMIT_BEFORE_DURABLE;
    assert(panel_awning_write_guard_begin(&ready, &result) == ESP_FAIL);
    assert(panel_awning_write_guard_load(&result) == ESP_OK);
    assert(result.state == PANEL_AWNING_GUARD_READY);

    s_fault = FAULT_COMMIT_AFTER_DURABLE;
    assert(panel_awning_write_guard_begin(&ready, &result) == ESP_FAIL);
    s_fault = FAULT_NONE;
    assert(panel_awning_write_guard_load(&result) == ESP_OK);
    assert(result.state == PANEL_AWNING_GUARD_WRITE_PENDING);

    s_fault = FAULT_READBACK_CORRUPT;
    assert(panel_awning_write_guard_resolve(&result, &ready) == ESP_ERR_INVALID_CRC);
    s_fault = FAULT_NONE;
    assert(panel_awning_write_guard_load(&ready) == ESP_OK);
    /* The commit may have made READY durable despite a failed readback. This
     * is safe only because resolve is called after confirmed or unsent work. */
    assert(ready.state == PANEL_AWNING_GUARD_READY);
}

static void test_initialization_requires_genuine_absence(void)
{
    panel_awning_guard_record_t ready = {0};
    uint8_t identity[PANEL_AWNING_GUARD_IDENTITY_SIZE];
    memset(identity, 0x5C, sizeof(identity));
    s_durable_present = false;
    s_fault = FAULT_NONE;
    assert(panel_awning_write_guard_initialize_absent(
        identity, 17U, &ready) == ESP_OK);
    assert(ready.state == PANEL_AWNING_GUARD_READY && ready.sequence == 1U);
    assert(ready.alias_generation == 17U);
    assert(memcmp(ready.homey_identity, identity, sizeof(identity)) == 0);
    assert(panel_awning_write_guard_record_valid(&ready));

    panel_awning_guard_record_t other = {0};
    memset(identity, 0xA3, sizeof(identity));
    assert(panel_awning_write_guard_initialize_absent(
        identity, 18U, &other) == ESP_ERR_INVALID_STATE);
    assert(panel_awning_write_guard_load(&other) == ESP_OK);
    assert(memcmp(&other, &ready, sizeof(ready)) == 0);

    s_durable_present = false;
    s_fault = FAULT_SET;
    assert(panel_awning_write_guard_initialize_absent(
        identity, 18U, &other) == ESP_FAIL);
    assert(panel_awning_write_guard_load(&other) == ESP_ERR_NOT_FOUND);
    s_fault = FAULT_COMMIT_BEFORE_DURABLE;
    assert(panel_awning_write_guard_initialize_absent(
        identity, 18U, &other) == ESP_FAIL);
    assert(panel_awning_write_guard_load(&other) == ESP_ERR_NOT_FOUND);
    s_fault = FAULT_COMMIT_AFTER_DURABLE;
    assert(panel_awning_write_guard_initialize_absent(
        identity, 18U, &other) == ESP_FAIL);
    s_fault = FAULT_NONE;
    assert(panel_awning_write_guard_load(&other) == ESP_OK);
    assert(other.state == PANEL_AWNING_GUARD_READY && other.sequence == 1U);

    ((panel_awning_guard_record_t *)s_durable_blob)->version = 99U;
    assert(panel_awning_write_guard_initialize_absent(
        identity, 19U, &other) == ESP_ERR_INVALID_CRC);
    assert(panel_awning_write_guard_load(&other) == ESP_ERR_INVALID_CRC);
}

int main(void)
{
    test_absence_and_corruption_fail_closed();
    test_commit_barrier_and_resolution();
    test_ambiguous_nvs_failures();
    test_initialization_requires_genuine_absence();
    puts("PANEL_AWNING_WRITE_GUARD_ADAPTER_TESTS PASS");
    return 0;
}
