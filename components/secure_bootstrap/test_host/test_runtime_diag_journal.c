#include "runtime_diag_journal.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEDIA_SIZE (1024U * 1024U)
#define SLOT_SIZE RUNTIME_DIAG_JOURNAL_RECORD_SIZE

extern int runtime_diag_test_classify_slot(const uint8_t *bytes);
extern bool runtime_diag_test_append_counted(uint8_t *media, size_t media_size, uint32_t slot,
                                             const runtime_diag_record_t *record,
                                             size_t *write_attempts, size_t *erase_attempts);

_Static_assert(sizeof(runtime_diag_event_t) == 40U, "bounded event RAM size");

static uint8_t *s_media;

static runtime_diag_record_t sample_record(uint64_t sequence, uint32_t boot)
{
    return (runtime_diag_record_t){
        .sequence = sequence,
        .boot_sequence = boot,
        .monotonic_ms = UINT64_C(0x1122334455667788),
        .event_type = RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_FAILURE,
        .source = 2U,
        .origin = 3U,
        .attempt = 4U,
        .result = 5U,
        .error_code = -6,
        .http_status = 503U,
        .transport = 7U,
        .stage = 8U,
        .retry_delay_ms = 30000U,
        .snapshot_generation = 33U,
        .reset_reason = RUNTIME_DIAG_RESET_TASK_WATCHDOG,
        .drop_count = 9U,
    };
}

static bool append(uint32_t slot, uint64_t sequence, uint32_t boot)
{
    const runtime_diag_record_t record = sample_record(sequence, boot);
    return runtime_diag_test_append(s_media, MEDIA_SIZE, slot, sequence, &record, SIZE_MAX);
}

static void test_explicit_record_round_trip(void)
{
    uint8_t bytes[SLOT_SIZE];
    runtime_diag_record_t decoded;
    const runtime_diag_record_t expected = sample_record(0x0102030405060708ULL, 11U);
    runtime_diag_test_encode(&expected, bytes);
    assert(runtime_diag_test_decode(bytes, &decoded));
    assert(decoded.sequence == expected.sequence);
    assert(decoded.boot_sequence == expected.boot_sequence);
    assert(decoded.monotonic_ms == expected.monotonic_ms);
    assert(decoded.event_type == expected.event_type);
    assert(decoded.source == expected.source);
    assert(decoded.origin == expected.origin);
    assert(decoded.attempt == expected.attempt);
    assert(decoded.result == expected.result);
    assert(decoded.error_code == expected.error_code);
    assert(decoded.http_status == expected.http_status);
    assert(decoded.transport == expected.transport);
    assert(decoded.stage == expected.stage);
    assert(decoded.retry_delay_ms == expected.retry_delay_ms);
    assert(decoded.snapshot_generation == expected.snapshot_generation);
    assert(decoded.reset_reason == expected.reset_reason);
    assert(decoded.drop_count == expected.drop_count);
    assert(bytes[60] == 0x54U && bytes[61] == 0x4dU &&
           bytes[62] == 0x4fU && bytes[63] == 0x43U);
}

static void test_crc_and_marker_rejection(void)
{
    uint8_t bytes[SLOT_SIZE];
    runtime_diag_record_t decoded;
    runtime_diag_test_encode(&(runtime_diag_record_t){
        .sequence = 1U,
        .boot_sequence = 1U,
        .event_type = RUNTIME_DIAG_EVENT_BOOT_START,
    }, bytes);
    assert(runtime_diag_test_decode(bytes, &decoded));
    bytes[40] ^= 1U;
    assert(!runtime_diag_test_decode(bytes, &decoded));
    runtime_diag_test_encode(&(runtime_diag_record_t){
        .sequence = 1U,
        .boot_sequence = 1U,
        .event_type = RUNTIME_DIAG_EVENT_BOOT_START,
    }, bytes);
    bytes[63] = 0xffU;
    assert(!runtime_diag_test_decode(bytes, &decoded));
}

static void test_blank_recovery_and_boot_sequence(void)
{
    memset(s_media, 0xff, MEDIA_SIZE);
    uint32_t next_slot = 99U, boot = 99U, valid = 99U;
    uint64_t next_sequence = 99U;
    assert(runtime_diag_test_recover(s_media, MEDIA_SIZE, &next_slot,
                                     &next_sequence, &boot, &valid));
    assert(next_slot == 0U && next_sequence == 1U && boot == 1U && valid == 0U);
    assert(append(0U, 1U, 4U));
    assert(append(1U, 2U, 4U));
    assert(runtime_diag_test_recover(s_media, MEDIA_SIZE, &next_slot,
                                     &next_sequence, &boot, &valid));
    assert(next_slot == 2U && next_sequence == 3U && boot == 5U && valid == 2U);
    assert(runtime_diag_test_is_erased(s_media + 2U * SLOT_SIZE));
}

static void test_torn_writes_preserve_committed_history(void)
{
    for (size_t interrupted = 0U; interrupted < SLOT_SIZE; ++interrupted) {
        memset(s_media, 0xff, MEDIA_SIZE);
        assert(append(0U, 1U, 1U));
        const runtime_diag_record_t candidate = sample_record(2U, 1U);
        assert(!runtime_diag_test_append(s_media, MEDIA_SIZE, 1U, 2U,
                                         &candidate, interrupted));
        uint32_t next_slot = 0U, boot = 0U, valid = 0U;
        uint64_t next_sequence = 0U;
        assert(runtime_diag_test_recover(s_media, MEDIA_SIZE, &next_slot,
                                         &next_sequence, &boot, &valid));
        assert(valid == 1U && next_sequence == 2U && boot == 2U);
        assert(next_slot == (interrupted == 0U ? 1U : 2U));
        runtime_diag_record_t decoded;
        assert(runtime_diag_test_decode(s_media, &decoded));
        assert(decoded.sequence == 1U);
    }
    memset(s_media, 0xff, MEDIA_SIZE);
    assert(append(0U, 1U, 1U));
    const runtime_diag_record_t candidate = sample_record(2U, 1U);
    assert(runtime_diag_test_append(s_media, MEDIA_SIZE, 1U, 2U,
                                    &candidate, SLOT_SIZE));
    uint32_t next_slot = 0U, boot = 0U, valid = 0U;
    uint64_t next_sequence = 0U;
    assert(runtime_diag_test_recover(s_media, MEDIA_SIZE, &next_slot,
                                     &next_sequence, &boot, &valid));
    assert(valid == 2U && next_slot == 2U && next_sequence == 3U);
}

static void test_corrupt_slot_skipped_and_sector_boundary(void)
{
    memset(s_media, 0xff, MEDIA_SIZE);
    assert(append(62U, 41U, 7U));
    assert(append(63U, 42U, 7U));
    s_media[64U * SLOT_SIZE] = 0x7fU;
    uint8_t dirty_before[SLOT_SIZE];
    memcpy(dirty_before, s_media + 64U * SLOT_SIZE, SLOT_SIZE);
    assert(runtime_diag_test_classify_slot(s_media + 62U * SLOT_SIZE) == 1);
    assert(runtime_diag_test_classify_slot(s_media + 64U * SLOT_SIZE) == 2);
    assert(runtime_diag_test_classify_slot(s_media + 65U * SLOT_SIZE) == 0);
    uint32_t next_slot = 0U, boot = 0U, valid = 0U;
    uint64_t next_sequence = 0U;
    assert(runtime_diag_test_recover(s_media, MEDIA_SIZE, &next_slot,
                                     &next_sequence, &boot, &valid));
    assert(valid == 2U && next_slot == 65U && next_sequence == 43U && boot == 8U);
    runtime_diag_record_t decoded;
    assert(runtime_diag_test_decode(s_media + 62U * SLOT_SIZE, &decoded) &&
           decoded.sequence == 41U && decoded.boot_sequence == 7U);
    assert(runtime_diag_test_decode(s_media + 63U * SLOT_SIZE, &decoded) &&
           decoded.sequence == 42U && decoded.boot_sequence == 7U);
    const runtime_diag_record_t forbidden = sample_record(43U, 8U);
    assert(!runtime_diag_test_append(s_media, MEDIA_SIZE, 64U, 43U, &forbidden, SIZE_MAX));
    assert(memcmp(dirty_before, s_media + 64U * SLOT_SIZE, SLOT_SIZE) == 0);
    assert(append(next_slot, next_sequence, boot));
    assert(memcmp(dirty_before, s_media + 64U * SLOT_SIZE, SLOT_SIZE) == 0);
    assert(runtime_diag_test_classify_slot(s_media + 65U * SLOT_SIZE) == 1);
    assert(runtime_diag_test_decode(s_media + 65U * SLOT_SIZE, &decoded) &&
           decoded.sequence == 43U && decoded.boot_sequence == 8U);
    assert(runtime_diag_test_recover(s_media, MEDIA_SIZE, &next_slot,
                                     &next_sequence, &boot, &valid));
    assert(valid == 3U && next_slot == 66U && next_sequence == 44U && boot == 9U);
    assert(runtime_diag_test_classify_slot(s_media + 64U * SLOT_SIZE) == 2);
    assert(runtime_diag_test_classify_slot(s_media + 62U * SLOT_SIZE) == 1);
    assert(runtime_diag_test_classify_slot(s_media + 63U * SLOT_SIZE) == 1);
}

static void test_append_only_full_partition_never_wraps(void)
{
    memset(s_media, 0xff, MEDIA_SIZE);
    for (uint32_t slot = 0U; slot < RUNTIME_DIAG_JOURNAL_CAPACITY; ++slot) {
        assert(append(slot, (uint64_t)slot + 1U, 1U));
    }
    uint32_t next_slot = 0U, boot = 0U, valid = 0U;
    uint64_t next_sequence = 0U;
    assert(runtime_diag_test_recover(s_media, MEDIA_SIZE, &next_slot,
                                     &next_sequence, &boot, &valid));
    assert(next_slot == RUNTIME_DIAG_JOURNAL_CAPACITY);
    assert(next_sequence == RUNTIME_DIAG_JOURNAL_CAPACITY + 1ULL);
    assert(valid == RUNTIME_DIAG_JOURNAL_CAPACITY);
    uint8_t first[SLOT_SIZE];
    memcpy(first, s_media, SLOT_SIZE);
    const runtime_diag_record_t extra = sample_record(next_sequence, boot);
    size_t writes = 99U, erases = 99U;
    assert(!runtime_diag_test_append_counted(s_media, MEDIA_SIZE, next_slot, &extra,
                                             &writes, &erases));
    assert(writes == 0U && erases == 0U);
    assert(memcmp(first, s_media, SLOT_SIZE) == 0);
}

static void test_query_defaults_limits_and_rejection(void)
{
    uint32_t limit = 0U;
    uint64_t before = UINT64_MAX;
    assert(runtime_diag_journal_parse_query(NULL, 0U, &limit, &before));
    assert(limit == RUNTIME_DIAG_JOURNAL_PAGE_DEFAULT && before == 0U);
    assert(runtime_diag_journal_parse_query("limit=256", 9U, &limit, &before));
    assert(limit == RUNTIME_DIAG_JOURNAL_PAGE_MAX);
    assert(runtime_diag_journal_parse_query(
        "limit=17&before_seq=9223372036854775807", 39U, &limit, &before));
    assert(limit == 17U && before == 9223372036854775807ULL);
    assert(!runtime_diag_journal_parse_query("limit=257", 9U, &limit, &before));
    assert(!runtime_diag_journal_parse_query("limit=0", 7U, &limit, &before));
    assert(!runtime_diag_journal_parse_query("limit=x", 7U, &limit, &before));
    assert(!runtime_diag_journal_parse_query("limit=2&limit=3", 15U, &limit, &before));
    assert(!runtime_diag_journal_parse_query("before_seq=0", 12U, &limit, &before));
    assert(!runtime_diag_journal_parse_query("unknown=1", 9U, &limit, &before));
    assert(!runtime_diag_journal_parse_query("limit=1&", 8U, &limit, &before));
    assert(!runtime_diag_journal_parse_query("limit=18446744073709551616", 26U,
                                             &limit, &before));
}

static void test_unknown_enum_names_are_fixed_fallbacks(void)
{
    assert(strcmp(runtime_diag_event_name(UINT16_MAX), "unknown") == 0);
    assert(strcmp(runtime_diag_reset_reason_name(UINT16_MAX), "unknown") == 0);
}

int main(void)
{
    s_media = malloc(MEDIA_SIZE);
    assert(s_media != NULL);
    test_explicit_record_round_trip();
    test_crc_and_marker_rejection();
    test_blank_recovery_and_boot_sequence();
    test_torn_writes_preserve_committed_history();
    test_corrupt_slot_skipped_and_sector_boundary();
    test_append_only_full_partition_never_wraps();
    test_query_defaults_limits_and_rejection();
    test_unknown_enum_names_are_fixed_fallbacks();
    free(s_media);
    puts("RUNTIME_DIAG_JOURNAL_HOST_TESTS PASS");
    return 0;
}
