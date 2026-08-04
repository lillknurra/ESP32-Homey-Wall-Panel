#include "panel_homey_alias_store.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_RECORD_SIZE 1232U
#define TEST_CRC_OFFSET 12U
#define TEST_ENTRY_COUNT_OFFSET 48U
#define TEST_RESERVED_0_OFFSET 49U
#define TEST_RESERVED_1_OFFSET 50U
#define TEST_RESERVED_2_OFFSET 51U

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

    puts("PATCH_015_ALIAS_HOST_TESTS=PASS");
    return 0;
}
