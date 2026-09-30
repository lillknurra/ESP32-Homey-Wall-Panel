#include "panel_homey_alias_store.h"
#include "nvs.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_RECORD_SIZE 1232U
#define TEST_CRC_OFFSET 12U
#define TEST_ENTRY_COUNT_OFFSET 48U
#define TEST_RESERVED_0_OFFSET 49U
#define TEST_RESERVED_1_OFFSET 50U
#define TEST_RESERVED_2_OFFSET 51U

typedef enum {
    MOCK_EVENT_SET_BLOB,
    MOCK_EVENT_COMMIT_BLOB,
    MOCK_EVENT_READBACK,
    MOCK_EVENT_SET_ACTIVE,
    MOCK_EVENT_COMMIT_ACTIVE,
} mock_nvs_event_t;

typedef struct {
    bool slot_present[2];
    uint8_t slots[2][TEST_RECORD_SIZE];
    size_t slot_sizes[2];
    bool active_present;
    uint8_t active_slot;
    int open_mode;
    bool pending_blob;
    int pending_blob_slot;
    uint8_t pending_blob_data[TEST_RECORD_SIZE];
    size_t pending_blob_size;
    bool pending_active;
    uint8_t pending_active_slot;
    bool blob_committed;
    bool fail_readback;
    bool fail_allocation;
    size_t close_count;
    mock_nvs_event_t events[16];
    size_t event_count;
} mock_nvs_t;

static mock_nvs_t mock_nvs;

void *patch057_test_calloc(size_t count, size_t size)
{
    if (mock_nvs.fail_allocation ||
        (size != 0U && count > SIZE_MAX / size)) {
        return NULL;
    }
    void *memory = malloc(count * size);
    if (memory != NULL) {
        memset(memory, 0, count * size);
    }
    return memory;
}

static int mock_slot_index(const char *key)
{
    if (strcmp(key, "slot_a") == 0) {
        return 0;
    }
    if (strcmp(key, "slot_b") == 0) {
        return 1;
    }
    return -1;
}

static void mock_event(mock_nvs_event_t event)
{
    assert(mock_nvs.event_count <
           sizeof(mock_nvs.events) / sizeof(mock_nvs.events[0]));
    mock_nvs.events[mock_nvs.event_count++] = event;
}

esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle)
{
    assert(strcmp(name, PANEL_HOMEY_ALIAS_STORE_NAMESPACE) == 0);
    mock_nvs.open_mode = mode;
    *handle = 1U;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle)
{
    assert(handle == 1U);
    mock_nvs.close_count++;
}

esp_err_t nvs_get_blob(
    nvs_handle_t handle,
    const char *key,
    void *out_value,
    size_t *length)
{
    assert(handle == 1U && length != NULL);
    const int index = mock_slot_index(key);
    assert(index >= 0);
    if (!mock_nvs.slot_present[index]) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (mock_nvs.blob_committed) {
        mock_event(MOCK_EVENT_READBACK);
        if (mock_nvs.fail_readback) {
            return ESP_ERR_NVS_INVALID_LENGTH;
        }
    }
    if (out_value == NULL) {
        *length = mock_nvs.slot_sizes[index];
        return ESP_OK;
    }
    if (*length < mock_nvs.slot_sizes[index]) {
        *length = mock_nvs.slot_sizes[index];
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    if (out_value != NULL) {
        memcpy(out_value, mock_nvs.slots[index], mock_nvs.slot_sizes[index]);
    }
    *length = mock_nvs.slot_sizes[index];
    return ESP_OK;
}

esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out_value)
{
    assert(handle == 1U && strcmp(key, "active_slot") == 0);
    if (!mock_nvs.active_present) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    *out_value = mock_nvs.active_slot;
    return ESP_OK;
}

esp_err_t nvs_set_blob(
    nvs_handle_t handle,
    const char *key,
    const void *value,
    size_t length)
{
    assert(handle == 1U && mock_nvs.open_mode == NVS_READWRITE);
    const int index = mock_slot_index(key);
    assert(index >= 0 && length == TEST_RECORD_SIZE);
    mock_nvs.pending_blob = true;
    mock_nvs.pending_blob_slot = index;
    mock_nvs.pending_blob_size = length;
    memcpy(mock_nvs.pending_blob_data, value, length);
    mock_event(MOCK_EVENT_SET_BLOB);
    return ESP_OK;
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value)
{
    assert(handle == 1U && mock_nvs.open_mode == NVS_READWRITE);
    assert(strcmp(key, "active_slot") == 0);
    mock_nvs.pending_active = true;
    mock_nvs.pending_active_slot = value;
    mock_event(MOCK_EVENT_SET_ACTIVE);
    return ESP_OK;
}

esp_err_t nvs_erase_all(nvs_handle_t handle)
{
    assert(handle == 1U && mock_nvs.open_mode == NVS_READWRITE);
    memset(mock_nvs.slot_present, 0, sizeof(mock_nvs.slot_present));
    mock_nvs.active_present = false;
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    assert(handle == 1U && mock_nvs.open_mode == NVS_READWRITE);
    if (mock_nvs.pending_blob) {
        const int index = mock_nvs.pending_blob_slot;
        memcpy(mock_nvs.slots[index],
               mock_nvs.pending_blob_data,
               mock_nvs.pending_blob_size);
        mock_nvs.slot_sizes[index] = mock_nvs.pending_blob_size;
        mock_nvs.slot_present[index] = true;
        mock_nvs.pending_blob = false;
        mock_nvs.blob_committed = true;
        mock_event(MOCK_EVENT_COMMIT_BLOB);
        return ESP_OK;
    }
    if (mock_nvs.pending_active) {
        mock_nvs.active_slot = mock_nvs.pending_active_slot;
        mock_nvs.active_present = true;
        mock_nvs.pending_active = false;
        mock_event(MOCK_EVENT_COMMIT_ACTIVE);
    }
    return ESP_OK;
}

static uint32_t test_crc32(const uint8_t *data, size_t size)
{
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            crc = (crc >> 1) ^
                (0xedb88320U &
                 ((uint32_t)-(int32_t)(crc & 1U)));
        }
    }
    return crc ^ 0xffffffffU;
}

static void test_put32(uint8_t *output, uint32_t value)
{
    for (unsigned i = 0; i < 4U; ++i) {
        output[i] = (uint8_t)(value >> (8U * i));
    }
}

static void refresh_blob_crc(uint8_t blob[TEST_RECORD_SIZE])
{
    test_put32(blob + TEST_CRC_OFFSET, 0U);
    test_put32(blob + TEST_CRC_OFFSET,
               test_crc32(blob, TEST_RECORD_SIZE));
}

static panel_homey_alias_record_t make_record(size_t count)
{
    panel_homey_alias_record_t record = {0};
    record.generation = 1U;
    record.entry_count = count;
    assert(panel_homey_alias_sha256(
        "selected-homey-test",
        record.homey_identity_digest));

    for (size_t i = 0U; i < count && i < 6U; ++i) {
        record.entries[i].dashboard_binding_index = (uint8_t)i;
        snprintf(record.entries[i].raw_device_id,
                 sizeof(record.entries[i].raw_device_id),
                 "test-device-%u",
                 (unsigned)i);
        snprintf(record.entries[i].raw_capability_id,
                 sizeof(record.entries[i].raw_capability_id),
                 "test-capability-%u",
                 (unsigned)i);
    }
    return record;
}

static void mock_store_reset(void)
{
    memset(&mock_nvs, 0, sizeof(mock_nvs));
}

static void test_nvs_workspace_allocation_failure_is_fail_closed(void)
{
    mock_store_reset();
    mock_nvs.fail_allocation = true;
    panel_homey_alias_record_t loaded = {0};
    bool present = false;
    assert(panel_homey_alias_store_load(
        "selected-homey-test", &loaded, &present) ==
        PANEL_HOMEY_ALIAS_STORE_IO_ERROR);
    assert(!present && mock_nvs.open_mode == NVS_READONLY);
    assert(mock_nvs.close_count == 1U && mock_nvs.event_count == 0U);

    mock_nvs.close_count = 0U;
    mock_nvs.open_mode = -1;
    panel_homey_alias_record_t input = make_record(3U);
    assert(panel_homey_alias_store_publish(
        "selected-homey-test", &input) == PANEL_HOMEY_ALIAS_STORE_IO_ERROR);
    assert(mock_nvs.open_mode == NVS_READWRITE);
    assert(mock_nvs.close_count == 1U && mock_nvs.event_count == 0U);
    assert(!mock_nvs.slot_present[0] && !mock_nvs.slot_present[1]);
}

static void mock_store_seed_slot(size_t index,
                                 const panel_homey_alias_record_t *record)
{
    assert(index < 2U);
    assert(panel_homey_alias_record_encode(
        record,
        mock_nvs.slots[index],
        sizeof(mock_nvs.slots[index])));
    mock_nvs.slot_sizes[index] = TEST_RECORD_SIZE;
    mock_nvs.slot_present[index] = true;
}

static void test_nvs_load_publish_order_and_generation(void)
{
    mock_store_reset();
    panel_homey_alias_record_t existing = make_record(1U);
    existing.generation = 5U;
    mock_store_seed_slot(0U, &existing);
    mock_nvs.active_present = true;
    mock_nvs.active_slot = PANEL_HOMEY_ALIAS_SLOT_A;

    panel_homey_alias_record_t loaded = {0};
    bool present = false;
    assert(panel_homey_alias_store_load(
        "selected-homey-test", &loaded, &present) ==
        PANEL_HOMEY_ALIAS_STORE_OK);
    assert(present && loaded.generation == 5U);
    assert(mock_nvs.open_mode == NVS_READONLY);

    panel_homey_alias_record_t input = make_record(3U);
    assert(panel_homey_alias_store_publish(
        "selected-homey-test", &input) == PANEL_HOMEY_ALIAS_STORE_OK);
    assert(mock_nvs.open_mode == NVS_READWRITE);
    assert(mock_nvs.event_count == 5U);
    assert(mock_nvs.events[0] == MOCK_EVENT_SET_BLOB);
    assert(mock_nvs.events[1] == MOCK_EVENT_COMMIT_BLOB);
    assert(mock_nvs.events[2] == MOCK_EVENT_READBACK);
    assert(mock_nvs.events[3] == MOCK_EVENT_SET_ACTIVE);
    assert(mock_nvs.events[4] == MOCK_EVENT_COMMIT_ACTIVE);
    assert(mock_nvs.active_slot == PANEL_HOMEY_ALIAS_SLOT_B);

    panel_homey_alias_record_t published = {0};
    assert(panel_homey_alias_record_decode(
        mock_nvs.slots[1], mock_nvs.slot_sizes[1], &published));
    assert(published.generation == 6U);
    assert(published.entry_count == 3U);
    assert(panel_homey_alias_runtime_activate(
        &(panel_homey_alias_runtime_t){0},
        &published,
        "selected-homey-test") == PANEL_HOMEY_ALIAS_STORE_OK);
}

static void test_nvs_readback_failure_keeps_old_active_hint(void)
{
    mock_store_reset();
    panel_homey_alias_record_t existing = make_record(1U);
    existing.generation = 8U;
    mock_store_seed_slot(0U, &existing);
    mock_nvs.active_present = true;
    mock_nvs.active_slot = PANEL_HOMEY_ALIAS_SLOT_A;
    mock_nvs.fail_readback = true;

    panel_homey_alias_record_t input = make_record(3U);
    assert(panel_homey_alias_store_publish(
        "selected-homey-test", &input) ==
        PANEL_HOMEY_ALIAS_STORE_VERIFY_ERROR);
    assert(mock_nvs.event_count == 3U);
    assert(mock_nvs.events[0] == MOCK_EVENT_SET_BLOB);
    assert(mock_nvs.events[1] == MOCK_EVENT_COMMIT_BLOB);
    assert(mock_nvs.events[2] == MOCK_EVENT_READBACK);
    assert(mock_nvs.slot_present[1]);
    assert(mock_nvs.active_slot == PANEL_HOMEY_ALIAS_SLOT_A);

    panel_homey_alias_record_t inactive = {0};
    assert(panel_homey_alias_record_decode(
        mock_nvs.slots[1], mock_nvs.slot_sizes[1], &inactive));
    assert(inactive.generation == 9U);
}

static void test_nvs_load_fails_closed_for_homey_mismatch_and_missing_slots(void)
{
    mock_store_reset();
    panel_homey_alias_record_t mismatched = make_record(1U);
    assert(panel_homey_alias_sha256(
        "different-homey", mismatched.homey_identity_digest));
    mock_store_seed_slot(0U, &mismatched);
    mock_nvs.active_present = true;
    mock_nvs.active_slot = PANEL_HOMEY_ALIAS_SLOT_A;

    panel_homey_alias_record_t loaded = {0};
    bool present = true;
    assert(panel_homey_alias_store_load(
        "selected-homey-test", &loaded, &present) ==
        PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED);
    assert(!present);

    mock_store_reset();
    present = true;
    assert(panel_homey_alias_store_load(
        "selected-homey-test", &loaded, &present) ==
        PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED);
    assert(!present);
    assert(mock_nvs.open_mode == NVS_READONLY);
}

static void test_store_diagnostics(void)
{
    panel_homey_alias_store_diagnostic_t diagnostic = {0};
    panel_homey_alias_record_t slot_a = make_record(0U);
    panel_homey_alias_record_t slot_b = make_record(3U);
    slot_a.generation = 1U;
    slot_b.generation = 2U;

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        false, false, NULL,
        false, false, NULL,
        false, false, 0U,
        &diagnostic));
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_NONE);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_NONE);
    assert(!diagnostic.slots[0].present && !diagnostic.slots[1].present);

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &slot_a,
        false, false, NULL,
        true, false, PANEL_HOMEY_ALIAS_SLOT_A,
        &diagnostic));
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_A);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_ACTIVE_HINT);
    assert(diagnostic.slots[0].structurally_valid);
    assert(diagnostic.slots[0].generation == 1U);
    assert(diagnostic.slots[0].entry_count == 0U);
    assert(diagnostic.slots[0].selected_homey_match == PANEL_HOMEY_ALIAS_DIAG_TRUE);
    for (size_t i = 0U; i < 3U; ++i) {
        assert(diagnostic.slots[0].awning_present[i] == PANEL_HOMEY_ALIAS_DIAG_FALSE);
    }

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        false, false, NULL,
        true, true, &slot_b,
        false, false, 0U,
        &diagnostic));
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_B);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_ONLY_VALID_SLOT);

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        false, false, NULL,
        true, true, &slot_b,
        true, false, PANEL_HOMEY_ALIAS_SLOT_B,
        &diagnostic));
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_B);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_ACTIVE_HINT);

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &slot_a,
        true, true, &slot_b,
        true, false, PANEL_HOMEY_ALIAS_SLOT_A,
        &diagnostic));
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_A);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_ACTIVE_HINT);
    assert(diagnostic.slots[0].generation < diagnostic.slots[1].generation);

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &slot_a,
        true, true, &slot_b,
        true, false, PANEL_HOMEY_ALIAS_SLOT_B,
        &diagnostic));
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_B);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_ACTIVE_HINT);

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &slot_a,
        true, true, &slot_b,
        false, false, 0U,
        &diagnostic));
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_B);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_GENERATION);

    panel_homey_alias_record_t tied_slot_a = slot_a;
    panel_homey_alias_record_t tied_slot_b = slot_b;
    tied_slot_a.generation = 7U;
    tied_slot_b.generation = 7U;
    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &tied_slot_a,
        true, true, &tied_slot_b,
        false, false, 0U,
        &diagnostic));
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_A);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_GENERATION);

    panel_homey_alias_store_diagnostic_t first_observation = diagnostic;
    panel_homey_alias_store_diagnostic_t second_observation = diagnostic;
    assert(panel_homey_alias_store_observations_equal(
        &first_observation, &second_observation));
    second_observation.active_slot_hint = PANEL_HOMEY_ALIAS_HINT_A;
    assert(!panel_homey_alias_store_observations_equal(
        &first_observation, &second_observation));
    second_observation = first_observation;
    second_observation.slots[1].generation++;
    assert(!panel_homey_alias_store_observations_equal(
        &first_observation, &second_observation));

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &slot_a,
        true, true, &slot_b,
        true, false, 99U,
        &diagnostic));
    assert(diagnostic.active_slot_hint == PANEL_HOMEY_ALIAS_HINT_INVALID);
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_B);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_GENERATION);

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &slot_a,
        true, false, NULL,
        false, false, 0U,
        &diagnostic));
    assert(diagnostic.slots[1].present);
    assert(!diagnostic.slots[1].structurally_valid);
    assert(diagnostic.effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_A);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_ONLY_VALID_SLOT);

    panel_homey_alias_record_t mismatched = make_record(1U);
    assert(panel_homey_alias_sha256(
        "different-selected-homey",
        mismatched.homey_identity_digest));
    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &mismatched,
        false, false, NULL,
        false, false, 0U,
        &diagnostic));
    assert(diagnostic.slots[0].selected_homey_match == PANEL_HOMEY_ALIAS_DIAG_FALSE);

    panel_homey_alias_record_t exactly_one = make_record(1U);
    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &exactly_one,
        false, false, NULL,
        false, false, 0U,
        &diagnostic));
    assert(diagnostic.slots[0].entry_count == 1U);
    assert(diagnostic.slots[0].awning_present[0] == PANEL_HOMEY_ALIAS_DIAG_TRUE);
    assert(diagnostic.slots[0].awning_present[1] == PANEL_HOMEY_ALIAS_DIAG_FALSE);

    panel_homey_alias_record_t all_six = make_record(6U);
    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &all_six,
        false, false, NULL,
        false, false, 0U,
        &diagnostic));
    assert(diagnostic.slots[0].entry_count == 6U);
    for (size_t i = 0U; i < 3U; ++i) {
        assert(diagnostic.slots[0].awning_present[i] == PANEL_HOMEY_ALIAS_DIAG_TRUE);
    }
    assert(sizeof(diagnostic) < sizeof(all_six));

    assert(panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        true, true, &slot_a,
        true, true, &slot_b,
        false, true, 0U,
        &diagnostic));
    assert(diagnostic.result == PANEL_HOMEY_ALIAS_STORE_IO_ERROR);
    assert(diagnostic.selection_basis == PANEL_HOMEY_ALIAS_SELECTION_UNKNOWN);
    assert(diagnostic.slots[0].structurally_valid &&
           diagnostic.slots[1].structurally_valid);

    assert(!panel_homey_alias_store_diagnose_records(
        "selected-homey-test",
        false, true, &slot_a,
        false, false, NULL,
        false, false, 0U,
        &diagnostic));
}

static void test_persistent_round_trip(void)
{
    panel_homey_alias_record_t record = make_record(6U);
    panel_homey_alias_record_t decoded = {0};
    uint8_t blob[TEST_RECORD_SIZE];

    assert(panel_homey_alias_record_encode(
        &record,
        blob,
        sizeof(blob)));
    assert(blob[TEST_ENTRY_COUNT_OFFSET] == 6U);
    assert(blob[TEST_RESERVED_0_OFFSET] == 0U);
    assert(blob[TEST_RESERVED_1_OFFSET] == 0U);
    assert(blob[TEST_RESERVED_2_OFFSET] == 0U);

    assert(panel_homey_alias_record_decode(
        blob,
        sizeof(blob),
        &decoded));
    assert(decoded.entry_count == 6U);
    assert(memcmp(record.homey_identity_digest,
                  decoded.homey_identity_digest,
                  sizeof(record.homey_identity_digest)) == 0);

    panel_homey_alias_runtime_t runtime = {0};
    assert(panel_homey_alias_runtime_activate(
        &runtime,
        &decoded,
        "selected-homey-test") ==
        PANEL_HOMEY_ALIAS_STORE_OK);
    assert(runtime.configured);

    panel_homey_alias_runtime_invalidate(&runtime);
    assert(panel_homey_alias_runtime_activate(
        &runtime,
        &decoded,
        "wrong-homey") ==
        PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED);
    assert(!runtime.configured);
}

static void test_reserved_header_bytes(void)
{
    panel_homey_alias_record_t record = make_record(1U);
    panel_homey_alias_record_t decoded = {0};
    uint8_t blob[TEST_RECORD_SIZE];

    assert(panel_homey_alias_record_encode(
        &record,
        blob,
        sizeof(blob)));

    const size_t reserved_offsets[] = {
        TEST_RESERVED_0_OFFSET,
        TEST_RESERVED_1_OFFSET,
        TEST_RESERVED_2_OFFSET,
    };

    for (size_t i = 0;
         i < sizeof(reserved_offsets) / sizeof(reserved_offsets[0]);
         ++i) {
        uint8_t modified[TEST_RECORD_SIZE];
        memcpy(modified, blob, sizeof(modified));
        modified[reserved_offsets[i]] = 1U;
        refresh_blob_crc(modified);
        assert(!panel_homey_alias_record_decode(
            modified,
            sizeof(modified),
            &decoded));
    }
}

int main(void)
{
    static const uint8_t abc_sha256[32] = {
        0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,
        0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,
        0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
    };

    uint8_t digest[32];
    assert(panel_homey_alias_sha256("abc", digest));
    assert(memcmp(digest, abc_sha256, sizeof(digest)) == 0);
    assert(!panel_homey_alias_sha256("", digest));

    test_persistent_round_trip();
    test_reserved_header_bytes();
    test_store_diagnostics();
    test_nvs_load_publish_order_and_generation();
    test_nvs_readback_failure_keeps_old_active_hint();
    test_nvs_load_fails_closed_for_homey_mismatch_and_missing_slots();
    test_nvs_workspace_allocation_failure_is_fail_closed();
    puts("PATCH057_NVS_PUBLICATION_HOST_TESTS=PASS");

    panel_homey_alias_record_t record = make_record(6U);
    panel_homey_alias_record_t decoded = {0};
    uint8_t blob[TEST_RECORD_SIZE];
    assert(panel_homey_alias_record_encode(
        &record,
        blob,
        sizeof(blob)));
    blob[100] ^= 1U;
    assert(!panel_homey_alias_record_decode(
        blob,
        sizeof(blob),
        &decoded));

    record = make_record(2U);
    record.entries[1].dashboard_binding_index =
        record.entries[0].dashboard_binding_index;
    assert(!panel_homey_alias_record_encode(
        &record,
        blob,
        sizeof(blob)));

    record = make_record(2U);
    strcpy(record.entries[1].raw_device_id,
           record.entries[0].raw_device_id);
    strcpy(record.entries[1].raw_capability_id,
           record.entries[0].raw_capability_id);
    assert(!panel_homey_alias_record_encode(
        &record,
        blob,
        sizeof(blob)));

    record = make_record(1U);
    memset(record.entries[0].raw_device_id,
           'x',
           sizeof(record.entries[0].raw_device_id));
    assert(!panel_homey_alias_record_encode(
        &record,
        blob,
        sizeof(blob)));

    record = make_record(1U);
    record.entry_count = 7U;
    assert(!panel_homey_alias_record_encode(
        &record,
        blob,
        sizeof(blob)));

    assert(panel_homey_alias_select_slot(
        false, 0, false, 0,
        PANEL_HOMEY_ALIAS_SLOT_NONE) ==
        PANEL_HOMEY_ALIAS_SLOT_NONE);
    assert(panel_homey_alias_select_slot(
        true, 3, false, 0,
        PANEL_HOMEY_ALIAS_SLOT_NONE) ==
        PANEL_HOMEY_ALIAS_SLOT_A);
    assert(panel_homey_alias_select_slot(
        false, 0, true, 4,
        PANEL_HOMEY_ALIAS_SLOT_NONE) ==
        PANEL_HOMEY_ALIAS_SLOT_B);
    assert(panel_homey_alias_select_slot(
        true, 3, true, 4,
        PANEL_HOMEY_ALIAS_SLOT_NONE) ==
        PANEL_HOMEY_ALIAS_SLOT_B);
    assert(panel_homey_alias_select_slot(
        true, 3, true, 4,
        PANEL_HOMEY_ALIAS_SLOT_A) ==
        PANEL_HOMEY_ALIAS_SLOT_A);

    assert(panel_homey_alias_next_generation(
        false, 0, false, 0) == 1U);
    assert(panel_homey_alias_next_generation(
        true, 7, true, 9) == 10U);
    assert(panel_homey_alias_next_generation(
        true, UINT32_MAX, false, 0) == 1U);

    record = make_record(1U);
    panel_homey_alias_runtime_t runtime = {0};
    char device_alias[48];
    char capability_alias[32];

    assert(panel_homey_alias_runtime_resolve(
        &runtime,
        "x",
        "y",
        device_alias,
        sizeof(device_alias),
        capability_alias,
        sizeof(capability_alias)) ==
        PANEL_HOMEY_READ_NOT_CONFIGURED);

    assert(panel_homey_alias_runtime_activate(
        &runtime,
        &record,
        "wrong-homey") ==
        PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED);
    assert(!runtime.configured);

    assert(panel_homey_alias_runtime_activate(
        &runtime,
        &record,
        "selected-homey-test") ==
        PANEL_HOMEY_ALIAS_STORE_OK);

    assert(panel_homey_alias_runtime_resolve(
        &runtime,
        "test-device-0",
        "test-capability-0",
        device_alias,
        sizeof(device_alias),
        capability_alias,
        sizeof(capability_alias)) ==
        PANEL_HOMEY_READ_OK);
    assert(strcmp(device_alias, "awning_1") == 0);
    assert(strcmp(capability_alias, "status") == 0);

    panel_homey_alias_snapshot_t captured = {0};
    assert(panel_homey_alias_runtime_capture(&runtime, &captured) ==
           PANEL_HOMEY_READ_OK);
    assert(captured.configured && captured.generation == record.generation);
    assert(captured.entry_count == 1U);
    assert(strcmp(captured.entries[0].raw_device_id, "test-device-0") == 0);
    assert(strcmp(captured.entries[0].raw_capability_id,
                  "test-capability-0") == 0);
    assert(strcmp(captured.entries[0].device_alias, "awning_1") == 0);
    assert(strcmp(captured.entries[0].capability_alias, "status") == 0);

    assert(panel_homey_alias_runtime_resolve(
        &runtime,
        "missing",
        "test-capability-0",
        device_alias,
        sizeof(device_alias),
        capability_alias,
        sizeof(capability_alias)) ==
        PANEL_HOMEY_READ_NOT_FOUND);

    assert(panel_homey_alias_runtime_resolve(
        &runtime,
        "test-device-0",
        "test-capability-0",
        device_alias,
        2U,
        capability_alias,
        sizeof(capability_alias)) ==
        PANEL_HOMEY_READ_OVERFLOW);

    panel_homey_alias_runtime_invalidate(&runtime);
    assert(!runtime.configured);
    assert(strcmp(captured.entries[0].raw_device_id, "test-device-0") == 0);
    assert(strcmp(captured.entries[0].raw_capability_id,
                  "test-capability-0") == 0);

    puts("PATCH_015_ALIAS_HOST_TESTS=PASS");
    return 0;
}
