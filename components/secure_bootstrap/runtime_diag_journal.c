#include "runtime_diag_journal.h"

#include <string.h>

#define DIAGLOG_SIZE_BYTES (1024U * 1024U)
#define DIAGLOG_MAGIC 0x4a444950U
#define DIAGLOG_FORMAT_VERSION 1U
#define DIAGLOG_CRC_OFFSET 56U
#define DIAGLOG_COMMIT_OFFSET 60U
#define DIAGLOG_COMMIT_WORD 0x434f4d54U
#define DIAGLOG_ERASED_BYTE 0xffU
#define DIAGLOG_SECTOR_SIZE 4096U
#define DIAGLOG_RECORDS_PER_SECTOR (DIAGLOG_SECTOR_SIZE / RUNTIME_DIAG_JOURNAL_RECORD_SIZE)

_Static_assert(RUNTIME_DIAG_JOURNAL_RECORD_SIZE == 64U, "journal record format size");
_Static_assert(DIAGLOG_CRC_OFFSET + sizeof(uint32_t) == DIAGLOG_COMMIT_OFFSET,
               "journal CRC offset");
_Static_assert(DIAGLOG_COMMIT_OFFSET + sizeof(uint32_t) == RUNTIME_DIAG_JOURNAL_RECORD_SIZE,
               "journal commit offset");
_Static_assert(RUNTIME_DIAG_JOURNAL_CAPACITY ==
                   (DIAGLOG_SIZE_BYTES / RUNTIME_DIAG_JOURNAL_RECORD_SIZE),
               "journal capacity");
_Static_assert(DIAGLOG_RECORDS_PER_SECTOR == 64U, "journal records per sector");
_Static_assert((DIAGLOG_SIZE_BYTES % DIAGLOG_SECTOR_SIZE) == 0U, "journal sector alignment");

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8U);
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) |
           ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
}

static uint64_t get_u64(const uint8_t *p)
{
    return (uint64_t)get_u32(p) | ((uint64_t)get_u32(p + 4U) << 32U);
}

static void put_u16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8U);
}

static void put_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8U);
    p[2] = (uint8_t)(value >> 16U);
    p[3] = (uint8_t)(value >> 24U);
}

static void put_u64(uint8_t *p, uint64_t value)
{
    put_u32(p, (uint32_t)value);
    put_u32(p + 4U, (uint32_t)(value >> 32U));
}

static uint32_t journal_crc32(const uint8_t *data, size_t size)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0U; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0U; bit < 8U; ++bit) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1U);
            crc = (crc >> 1U) ^ (0xedb88320U & mask);
        }
    }
    return crc ^ UINT32_MAX;
}

static bool slot_erased(const uint8_t *bytes)
{
    for (size_t i = 0U; i < RUNTIME_DIAG_JOURNAL_RECORD_SIZE; ++i) {
        if (bytes[i] != DIAGLOG_ERASED_BYTE) return false;
    }
    return true;
}

static void encode_record(
    const runtime_diag_record_t *record,
    uint8_t bytes[RUNTIME_DIAG_JOURNAL_RECORD_SIZE],
    bool committed)
{
    memset(bytes, 0xff, RUNTIME_DIAG_JOURNAL_RECORD_SIZE);
    put_u32(bytes + 0U, DIAGLOG_MAGIC);
    put_u16(bytes + 4U, DIAGLOG_FORMAT_VERSION);
    put_u16(bytes + 6U, record->event_type);
    put_u64(bytes + 8U, record->sequence);
    put_u32(bytes + 16U, record->boot_sequence);
    put_u64(bytes + 20U, record->monotonic_ms);
    put_u32(bytes + 28U, (uint32_t)record->error_code);
    put_u32(bytes + 32U, record->retry_delay_ms);
    put_u32(bytes + 36U, record->snapshot_generation);
    put_u32(bytes + 40U, record->drop_count);
    put_u16(bytes + 44U, record->attempt);
    put_u16(bytes + 46U, record->http_status);
    put_u16(bytes + 48U, record->result);
    put_u16(bytes + 50U, record->reset_reason);
    bytes[52] = record->source;
    bytes[53] = record->origin;
    bytes[54] = record->transport;
    bytes[55] = record->stage;
    put_u32(bytes + DIAGLOG_CRC_OFFSET, journal_crc32(bytes, DIAGLOG_CRC_OFFSET));
    if (committed) put_u32(bytes + DIAGLOG_COMMIT_OFFSET, DIAGLOG_COMMIT_WORD);
}

static bool decode_record(
    const uint8_t bytes[RUNTIME_DIAG_JOURNAL_RECORD_SIZE],
    runtime_diag_record_t *record)
{
    if (record == NULL || get_u32(bytes + 0U) != DIAGLOG_MAGIC ||
        get_u16(bytes + 4U) != DIAGLOG_FORMAT_VERSION ||
        get_u32(bytes + DIAGLOG_COMMIT_OFFSET) != DIAGLOG_COMMIT_WORD ||
        get_u32(bytes + DIAGLOG_CRC_OFFSET) != journal_crc32(bytes, DIAGLOG_CRC_OFFSET)) {
        return false;
    }
    memset(record, 0, sizeof(*record));
    record->event_type = get_u16(bytes + 6U);
    record->sequence = get_u64(bytes + 8U);
    record->boot_sequence = get_u32(bytes + 16U);
    record->monotonic_ms = get_u64(bytes + 20U);
    record->error_code = (int32_t)get_u32(bytes + 28U);
    record->retry_delay_ms = get_u32(bytes + 32U);
    record->snapshot_generation = get_u32(bytes + 36U);
    record->drop_count = get_u32(bytes + 40U);
    record->attempt = get_u16(bytes + 44U);
    record->http_status = get_u16(bytes + 46U);
    record->result = get_u16(bytes + 48U);
    record->reset_reason = get_u16(bytes + 50U);
    record->source = bytes[52];
    record->origin = bytes[53];
    record->transport = bytes[54];
    record->stage = bytes[55];
    return record->sequence != 0U && record->event_type != 0U;
}

typedef enum {
    SLOT_ERASED,
    SLOT_VALID_COMMITTED,
    SLOT_DIRTY_OR_TORN,
} slot_class_t;

static slot_class_t classify_slot(const uint8_t *bytes, runtime_diag_record_t *record)
{
    runtime_diag_record_t decoded;
    if (slot_erased(bytes)) return SLOT_ERASED;
    if (decode_record(bytes, &decoded)) {
        if (record != NULL) *record = decoded;
        return SLOT_VALID_COMMITTED;
    }
    return SLOT_DIRTY_OR_TORN;
}

#ifdef RUNTIME_DIAG_JOURNAL_HOST_TEST
int runtime_diag_test_classify_slot(const uint8_t *bytes)
{
    return (int)classify_slot(bytes, NULL);
}
#endif

typedef bool (*journal_read_fn)(void *ctx, uint32_t offset, void *dst, size_t size);

typedef struct {
    uint32_t next_slot;
    uint64_t next_sequence;
    uint32_t boot_sequence;
    uint32_t valid_count;
    uint32_t free_count;
    uint32_t dirty_count;
    uint32_t newest_slot;
    uint64_t newest_sequence;
    uint32_t newest_boot;
    bool sequence_exhausted;
} recovery_result_t;

static bool recover_core(journal_read_fn read_fn, void *ctx,
                         uint8_t scratch[DIAGLOG_SECTOR_SIZE], recovery_result_t *out)
{
    if (read_fn == NULL || scratch == NULL || out == NULL) return false;
    memset(out, 0, sizeof(*out));
    bool have_newest = false;
    for (uint32_t sector = 0U; sector < DIAGLOG_SIZE_BYTES / DIAGLOG_SECTOR_SIZE; ++sector) {
        const uint32_t offset = sector * DIAGLOG_SECTOR_SIZE;
        if (!read_fn(ctx, offset, scratch, DIAGLOG_SECTOR_SIZE)) return false;
        for (uint32_t within = 0U; within < DIAGLOG_SECTOR_SIZE;
             within += RUNTIME_DIAG_JOURNAL_RECORD_SIZE) {
            runtime_diag_record_t record;
            const slot_class_t cls = classify_slot(scratch + within, &record);
            const uint32_t slot = sector * DIAGLOG_RECORDS_PER_SECTOR +
                within / RUNTIME_DIAG_JOURNAL_RECORD_SIZE;
            if (cls == SLOT_ERASED) out->free_count++;
            else if (cls == SLOT_DIRTY_OR_TORN) out->dirty_count++;
            else {
                out->valid_count++;
                if (!have_newest || record.sequence > out->newest_sequence) {
                    have_newest = true;
                    out->newest_sequence = record.sequence;
                    out->newest_slot = slot;
                }
                if (record.boot_sequence > out->newest_boot) out->newest_boot = record.boot_sequence;
            }
        }
    }
    uint32_t cursor = have_newest ? out->newest_slot + 1U : 0U;
    while (cursor < RUNTIME_DIAG_JOURNAL_CAPACITY) {
        const uint32_t sector = cursor / DIAGLOG_RECORDS_PER_SECTOR;
        if (!read_fn(ctx, sector * DIAGLOG_SECTOR_SIZE, scratch, DIAGLOG_SECTOR_SIZE)) return false;
        const uint32_t start = (cursor % DIAGLOG_RECORDS_PER_SECTOR) * RUNTIME_DIAG_JOURNAL_RECORD_SIZE;
        bool found = false;
        for (uint32_t within = start; within < DIAGLOG_SECTOR_SIZE;
             within += RUNTIME_DIAG_JOURNAL_RECORD_SIZE) {
            if (classify_slot(scratch + within, NULL) == SLOT_ERASED) {
                cursor = sector * DIAGLOG_RECORDS_PER_SECTOR + within / RUNTIME_DIAG_JOURNAL_RECORD_SIZE;
                found = true;
                break;
            }
        }
        if (found) break;
        cursor = (sector + 1U) * DIAGLOG_RECORDS_PER_SECTOR;
    }
    out->next_slot = cursor;
    if ((have_newest && out->newest_sequence == UINT64_MAX) || out->newest_boot == UINT32_MAX) {
        out->sequence_exhausted = true;
        return false;
    }
    out->next_sequence = have_newest ? out->newest_sequence + 1U : 1U;
    out->boot_sequence = out->newest_boot + 1U;
    if (out->boot_sequence == 0U) out->boot_sequence = 1U;
    return true;
}

#ifdef RUNTIME_DIAG_JOURNAL_HOST_TEST
static bool media_read(void *ctx, uint32_t offset, void *dst, size_t size)
{
    const uint8_t *media = (const uint8_t *)ctx;
    if (media == NULL || offset > DIAGLOG_SIZE_BYTES || size > DIAGLOG_SIZE_BYTES - offset) return false;
    memcpy(dst, media + offset, size);
    return true;
}
#endif

typedef enum { APPEND_OK, APPEND_OUT_OF_RANGE, APPEND_READ_ERROR, APPEND_NOT_ERASED,
               APPEND_WRITE_ERROR } append_result_t;
typedef int (*journal_append_read_fn)(void *ctx, uint32_t offset, void *dst, size_t size);
typedef int (*journal_append_write_fn)(void *ctx, uint32_t offset, const void *src, size_t size);

static append_result_t append_core(journal_append_read_fn read_fn,
                                   journal_append_write_fn write_fn, void *ctx, uint32_t slot,
                                   int *io_error_out,
                                   const runtime_diag_record_t *record)
{
    if (io_error_out != NULL) *io_error_out = 0;
    if (slot >= RUNTIME_DIAG_JOURNAL_CAPACITY) return APPEND_OUT_OF_RANGE;
    _Alignas(4) uint8_t bytes[RUNTIME_DIAG_JOURNAL_RECORD_SIZE];
    const uint32_t offset = slot * RUNTIME_DIAG_JOURNAL_RECORD_SIZE;
    const int read_result = read_fn(ctx, offset, bytes, sizeof(bytes));
    if (read_result != 0) {
        if (io_error_out != NULL) *io_error_out = read_result;
        return APPEND_READ_ERROR;
    }
    if (classify_slot(bytes, NULL) != SLOT_ERASED) return APPEND_NOT_ERASED;
    encode_record(record, bytes, false);
    int write_result = write_fn(ctx, offset, bytes, DIAGLOG_COMMIT_OFFSET);
    if (write_result != 0) {
        if (io_error_out != NULL) *io_error_out = write_result;
        return APPEND_WRITE_ERROR;
    }
    put_u32(bytes + DIAGLOG_COMMIT_OFFSET, DIAGLOG_COMMIT_WORD);
    write_result = write_fn(ctx, offset + DIAGLOG_COMMIT_OFFSET,
                            bytes + DIAGLOG_COMMIT_OFFSET, sizeof(uint32_t));
    if (write_result != 0) {
        if (io_error_out != NULL) *io_error_out = write_result;
        return APPEND_WRITE_ERROR;
    }
    return APPEND_OK;
}

#ifdef RUNTIME_DIAG_JOURNAL_HOST_TEST
typedef struct { uint8_t *media; size_t interrupt_after; size_t written; size_t write_attempts; bool interrupted; } host_media_t;
static int host_read(void *ctx, uint32_t offset, void *dst, size_t size)
{
    return media_read(((host_media_t *)ctx)->media, offset, dst, size) ? 0 : -1;
}
static int host_write(void *ctx, uint32_t offset, const void *src, size_t size)
{
    host_media_t *host = (host_media_t *)ctx;
    host->write_attempts++;
    if (offset > DIAGLOG_SIZE_BYTES || size > DIAGLOG_SIZE_BYTES - offset) return -1;
    if (host->written >= host->interrupt_after) { host->interrupted = true; return -1; }
    size_t count = size;
    if (host->written + count > host->interrupt_after) count = host->interrupt_after - host->written;
    const uint8_t *input = (const uint8_t *)src;
    for (size_t i = 0U; i < count; ++i) host->media[offset + i] &= input[i];
    host->written += count;
    return count == size ? 0 : -1;
}
#endif

#ifdef RUNTIME_DIAG_JOURNAL_HOST_TEST
static bool media_recover(
    const uint8_t *media,
    size_t media_size,
    uint32_t *next_slot,
    uint64_t *next_sequence,
    uint32_t *boot_sequence,
    uint32_t *valid_count,
    uint32_t *free_count)
{
    if (media_size != DIAGLOG_SIZE_BYTES || next_slot == NULL || next_sequence == NULL ||
        boot_sequence == NULL || valid_count == NULL) return false;
    uint8_t scratch[DIAGLOG_SECTOR_SIZE];
    recovery_result_t result;
    if (!recover_core(media_read, (void *)media, scratch, &result)) return false;
    *next_slot = result.next_slot; *next_sequence = result.next_sequence;
    *boot_sequence = result.boot_sequence; *valid_count = result.valid_count;
    if (free_count != NULL) *free_count = result.free_count;
    return true;
}
#endif

#ifdef RUNTIME_DIAG_JOURNAL_HOST_TEST
bool runtime_diag_test_recover(const uint8_t *media, size_t media_size,
                               uint32_t *next_slot, uint64_t *next_sequence,
                               uint32_t *boot_sequence, uint32_t *valid_count)
{
    return media_recover(media, media_size, next_slot, next_sequence,
                         boot_sequence, valid_count, NULL);
}

void runtime_diag_test_encode(const runtime_diag_record_t *record,
                              uint8_t bytes[RUNTIME_DIAG_JOURNAL_RECORD_SIZE])
{
    encode_record(record, bytes, true);
}

bool runtime_diag_test_decode(const uint8_t bytes[RUNTIME_DIAG_JOURNAL_RECORD_SIZE],
                              runtime_diag_record_t *record)
{
    return decode_record(bytes, record);
}

bool runtime_diag_test_is_erased(const uint8_t *slot)
{
    return slot_erased(slot);
}

bool runtime_diag_test_append(uint8_t *media, size_t media_size,
                              uint32_t slot, uint64_t sequence,
                              const runtime_diag_record_t *record,
                              size_t interrupt_after_bytes)
{
    if (media == NULL || record == NULL || media_size != DIAGLOG_SIZE_BYTES) return false;
    runtime_diag_record_t candidate = *record;
    candidate.sequence = sequence;
    host_media_t host = { .media = media, .interrupt_after = interrupt_after_bytes };
    return append_core(host_read, host_write, &host, slot, NULL, &candidate) == APPEND_OK;
}

bool runtime_diag_test_append_counted(uint8_t *media, size_t media_size, uint32_t slot,
                                      const runtime_diag_record_t *record,
                                      size_t *write_attempts, size_t *erase_attempts)
{
    if (media == NULL || record == NULL || media_size != DIAGLOG_SIZE_BYTES) return false;
    host_media_t host = { .media = media, .interrupt_after = SIZE_MAX };
    const bool ok = append_core(host_read, host_write, &host, slot, NULL, record) == APPEND_OK;
    if (write_attempts != NULL) *write_attempts = host.write_attempts;
    if (erase_attempts != NULL) *erase_attempts = 0U;
    return ok;
}
#endif

const char *runtime_diag_event_name(uint16_t event_type)
{
    switch (event_type) {
    case RUNTIME_DIAG_EVENT_BOOT_START: return "boot_start";
    case RUNTIME_DIAG_EVENT_WIFI_START: return "wifi_start";
    case RUNTIME_DIAG_EVENT_WIFI_GOT_IP: return "wifi_got_ip";
    case RUNTIME_DIAG_EVENT_WIFI_ONLINE: return "wifi_online";
    case RUNTIME_DIAG_EVENT_HTTP_SERVER_START_BEGIN: return "http_server_start_begin";
    case RUNTIME_DIAG_EVENT_HTTP_SERVER_START_RESULT: return "http_server_start_result";
    case RUNTIME_DIAG_EVENT_AUTH_RESTORE_BEGIN: return "auth_restore_begin";
    case RUNTIME_DIAG_EVENT_AUTH_RESTORE_RESULT: return "auth_restore_result";
    case RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_BEGIN: return "preselection_restore_begin";
    case RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_ATTEMPT_RESULT: return "preselection_restore_attempt_result";
    case RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_RETRY: return "preselection_restore_retry";
    case RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_RESULT: return "preselection_restore_result";
    case RUNTIME_DIAG_EVENT_BOOT_AUTO_SCHEDULER_BEGIN: return "boot_auto_scheduler_begin";
    case RUNTIME_DIAG_EVENT_BOOT_AUTO_QUEUE_RESULT: return "boot_auto_queue_result";
    case RUNTIME_DIAG_EVENT_HOMEY_REFRESH_BEGIN: return "homey_refresh_begin";
    case RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_BEGIN: return "homey_attempt_begin";
    case RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_FAILURE: return "homey_attempt_failure";
    case RUNTIME_DIAG_EVENT_HOMEY_RETRY_SCHEDULED: return "homey_retry_scheduled";
    case RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_SUCCESS: return "homey_attempt_success";
    case RUNTIME_DIAG_EVENT_SNAPSHOT_PUBLISH_SUCCESS: return "snapshot_publish_success";
    case RUNTIME_DIAG_EVENT_SNAPSHOT_PUBLISH_FAILURE: return "snapshot_publish_failure";
    case RUNTIME_DIAG_EVENT_HOMEY_DATA_STATE_CHANGE: return "homey_data_state_change";
    case RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_QUEUE_RESULT: return "periodic_refresh_queue_result";
    case RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_RESULT: return "periodic_refresh_result";
    default: return "unknown";
    }
}

const char *runtime_diag_reset_reason_name(uint16_t reason)
{
    switch (reason) {
    case RUNTIME_DIAG_RESET_POWER_ON: return "power_on";
    case RUNTIME_DIAG_RESET_EXTERNAL: return "external";
    case RUNTIME_DIAG_RESET_SOFTWARE: return "software";
    case RUNTIME_DIAG_RESET_PANIC: return "panic";
    case RUNTIME_DIAG_RESET_INT_WATCHDOG: return "int_watchdog";
    case RUNTIME_DIAG_RESET_TASK_WATCHDOG: return "task_watchdog";
    case RUNTIME_DIAG_RESET_WATCHDOG: return "watchdog";
    case RUNTIME_DIAG_RESET_DEEP_SLEEP: return "deep_sleep";
    case RUNTIME_DIAG_RESET_BROWNOUT: return "brownout";
    case RUNTIME_DIAG_RESET_SDIO: return "sdio";
    case RUNTIME_DIAG_RESET_USB: return "usb";
    case RUNTIME_DIAG_RESET_JTAG: return "jtag";
    case RUNTIME_DIAG_RESET_EFUSE: return "efuse";
    case RUNTIME_DIAG_RESET_POWER_GLITCH: return "power_glitch";
    case RUNTIME_DIAG_RESET_CPU_LOCKUP: return "cpu_lockup";
    default: return "unknown";
    }
}

static bool parse_decimal_u64(const char *text, size_t length, uint64_t *value_out)
{
    if (text == NULL || length == 0U || value_out == NULL) return false;
    uint64_t value = 0U;
    for (size_t i = 0U; i < length; ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        const uint8_t digit = (uint8_t)(text[i] - '0');
        if (value > (UINT64_MAX - digit) / 10U) return false;
        value = (value * 10U) + digit;
    }
    *value_out = value;
    return true;
}

bool runtime_diag_journal_parse_query(
    const char *query,
    size_t query_length,
    uint32_t *limit_out,
    uint64_t *before_sequence_out)
{
    if (limit_out == NULL || before_sequence_out == NULL ||
        (query_length != 0U && query == NULL) || query_length >= 96U) return false;
    *limit_out = RUNTIME_DIAG_JOURNAL_PAGE_DEFAULT;
    *before_sequence_out = 0U;
    if (query_length == 0U) return true;
    if (query[query_length - 1U] == '&') return false;
    bool have_limit = false;
    bool have_before = false;
    size_t start = 0U;
    while (start < query_length) {
        size_t end = start;
        while (end < query_length && query[end] != '&') end++;
        size_t equal = start;
        while (equal < end && query[equal] != '=') equal++;
        if (equal == end || equal == start || equal + 1U == end) return false;
        const size_t key_length = equal - start;
        uint64_t parsed = 0U;
        if (key_length == 5U && memcmp(query + start, "limit", 5U) == 0) {
            if (have_limit || !parse_decimal_u64(query + equal + 1U,
                                                  end - equal - 1U, &parsed) ||
                parsed == 0U || parsed > RUNTIME_DIAG_JOURNAL_PAGE_MAX) return false;
            have_limit = true;
            *limit_out = (uint32_t)parsed;
        } else if (key_length == 10U &&
                   memcmp(query + start, "before_seq", 10U) == 0) {
            if (have_before || !parse_decimal_u64(query + equal + 1U,
                                                   end - equal - 1U, &parsed) ||
                parsed == 0U) return false;
            have_before = true;
            *before_sequence_out = parsed;
        } else {
            return false;
        }
        start = end + 1U;
    }
    return true;
}

#ifdef ESP_PLATFORM
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdalign.h>
#include <string.h>

#define DIAGLOG_QUEUE_DEPTH 32U
#define DIAGLOG_WRITER_STACK_BYTES 4096U

_Static_assert(sizeof(runtime_diag_event_t) <= 40U, "event queue item must remain bounded");

static StaticQueue_t s_queue_control;
static alignas(8) uint8_t s_queue_storage[DIAGLOG_QUEUE_DEPTH * sizeof(runtime_diag_event_t)];
static QueueHandle_t s_event_queue;
static SemaphoreHandle_t s_media_mutex;
static const esp_partition_t *s_partition;
static TaskHandle_t s_writer_task;
static portMUX_TYPE s_state_mux = portMUX_INITIALIZER_UNLOCKED;
static alignas(4) uint32_t s_scan_buffer[DIAGLOG_SECTOR_SIZE / sizeof(uint32_t)];
static bool s_available;
static bool s_recovering;
static bool s_full;
static bool s_append_fault;
static uint32_t s_next_slot;
static uint64_t s_next_sequence;
static uint32_t s_boot_sequence;
static uint32_t s_valid_count;
static uint32_t s_free_slots;
static uint32_t s_volatile_drops;

static uint32_t saturating_increment(uint32_t value)
{
    return value == UINT32_MAX ? UINT32_MAX : value + 1U;
}

static void record_drop(void)
{
    portENTER_CRITICAL(&s_state_mux);
    s_volatile_drops = saturating_increment(s_volatile_drops);
    portEXIT_CRITICAL(&s_state_mux);
}

static bool read_flash(uint32_t offset, void *destination, size_t size)
{
    return esp_partition_read(s_partition, offset, destination, size) == ESP_OK;
}

static bool partition_read(void *ctx, uint32_t offset, void *destination, size_t size)
{
    return esp_partition_read((const esp_partition_t *)ctx, offset, destination, size) == ESP_OK;
}

static int partition_append_read(void *ctx, uint32_t offset, void *destination, size_t size)
{
    return (int)esp_partition_read((const esp_partition_t *)ctx, offset, destination, size);
}

static int partition_append_write(void *ctx, uint32_t offset, const void *source, size_t size)
{
    return (int)esp_partition_write((const esp_partition_t *)ctx, offset, source, size);
}

static esp_err_t recover_partition(void)
{
    if (xSemaphoreTake(s_media_mutex, portMAX_DELAY) != pdTRUE) return ESP_FAIL;
    uint8_t *chunk = (uint8_t *)s_scan_buffer;
    recovery_result_t result;
    if (!recover_core(partition_read, (void *)s_partition, chunk, &result)) {
        xSemaphoreGive(s_media_mutex);
        if (result.sequence_exhausted) {
            portENTER_CRITICAL(&s_state_mux);
            s_recovering = false;
            portEXIT_CRITICAL(&s_state_mux);
            return ESP_ERR_INVALID_STATE;
        }
        return ESP_FAIL;
    }
    portENTER_CRITICAL(&s_state_mux);
    s_next_slot = result.next_slot;
    s_next_sequence = result.next_sequence;
    s_boot_sequence = result.boot_sequence;
    s_valid_count = result.valid_count;
    s_free_slots = result.free_count;
    s_full = result.next_slot >= RUNTIME_DIAG_JOURNAL_CAPACITY;
    s_recovering = false;
    portEXIT_CRITICAL(&s_state_mux);
    xSemaphoreGive(s_media_mutex);
    return ESP_OK;
}

static esp_err_t append_event(const runtime_diag_event_t *event)
{
    portENTER_CRITICAL(&s_state_mux);
    if (!s_available || s_full || s_append_fault) {
        s_volatile_drops = saturating_increment(s_volatile_drops);
        portEXIT_CRITICAL(&s_state_mux);
        return ESP_ERR_INVALID_STATE;
    }
    const uint32_t slot = s_next_slot;
    const uint64_t sequence = s_next_sequence;
    const uint32_t boot_sequence = s_boot_sequence;
    const uint32_t drops = s_volatile_drops;
    portEXIT_CRITICAL(&s_state_mux);

    if (slot >= RUNTIME_DIAG_JOURNAL_CAPACITY ||
        xSemaphoreTake(s_media_mutex, portMAX_DELAY) != pdTRUE) return ESP_FAIL;
    const uint32_t offset = slot * RUNTIME_DIAG_JOURNAL_RECORD_SIZE;
    runtime_diag_record_t record = {
            .sequence = sequence,
            .boot_sequence = boot_sequence,
            .monotonic_ms = event->monotonic_ms != 0U ? event->monotonic_ms :
                (uint64_t)(esp_timer_get_time() / 1000LL),
            .event_type = event->event_type,
            .source = event->source,
            .origin = event->origin,
            .attempt = event->attempt,
            .result = event->result,
            .error_code = event->error_code,
            .http_status = event->http_status,
            .transport = event->transport,
            .stage = event->stage,
            .retry_delay_ms = event->retry_delay_ms,
            .snapshot_generation = event->snapshot_generation,
            .reset_reason = event->reset_reason,
            .drop_count = drops,
        };
    int io_error = 0;
    const append_result_t append_result = append_core(partition_append_read,
        partition_append_write, (void *)s_partition, slot, &io_error, &record);
    const esp_err_t err = append_result == APPEND_OK ? ESP_OK :
        (append_result == APPEND_NOT_ERASED ? ESP_ERR_INVALID_STATE :
         ((append_result == APPEND_READ_ERROR || append_result == APPEND_WRITE_ERROR)
            ? (esp_err_t)io_error : ESP_FAIL));
    xSemaphoreGive(s_media_mutex);

    portENTER_CRITICAL(&s_state_mux);
    if (err == ESP_OK) {
        s_next_slot = slot + 1U;
        s_next_sequence = sequence == UINT64_MAX ? UINT64_MAX : sequence + 1U;
        s_valid_count = saturating_increment(s_valid_count);
        if (s_free_slots > 0U) s_free_slots--;
        s_full = s_next_slot >= RUNTIME_DIAG_JOURNAL_CAPACITY;
        if (sequence == UINT64_MAX) s_append_fault = true;
    } else {
        /* A failed flash write may have consumed bits. Never retry the same slot. */
        s_next_slot = slot + 1U;
        s_full = s_next_slot >= RUNTIME_DIAG_JOURNAL_CAPACITY;
        s_append_fault = true;
        s_volatile_drops = saturating_increment(s_volatile_drops);
    }
    portEXIT_CRITICAL(&s_state_mux);
    return err;
}

static void writer_task(void *arg)
{
    (void)arg;
    if (recover_partition() != ESP_OK) {
        portENTER_CRITICAL(&s_state_mux);
        s_available = false;
        s_recovering = false;
        portEXIT_CRITICAL(&s_state_mux);
        vTaskDelete(NULL);
        return;
    }
    runtime_diag_event_t event;
    for (;;) {
        if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) != pdTRUE) continue;
        if (append_event(&event) != ESP_OK) {
            portENTER_CRITICAL(&s_state_mux);
            const bool stop = s_full || s_append_fault || !s_available;
            portEXIT_CRITICAL(&s_state_mux);
            if (stop) {
                while (xQueueReceive(s_event_queue, &event, 0U) == pdTRUE) record_drop();
            }
        }
    }
}

esp_err_t runtime_diag_journal_start(runtime_diag_reset_reason_t reset_reason)
{
    if (s_writer_task != NULL) return ESP_OK;
    (void)reset_reason;
    s_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "diaglog");
    if (s_partition == NULL || s_partition->type != ESP_PARTITION_TYPE_DATA ||
        s_partition->subtype != ESP_PARTITION_SUBTYPE_DATA_UNDEFINED ||
        s_partition->size != DIAGLOG_SIZE_BYTES || s_partition->address != 0x610000U ||
        strcmp(s_partition->label, "diaglog") != 0) return ESP_ERR_NOT_FOUND;

    s_event_queue = xQueueCreateStatic(DIAGLOG_QUEUE_DEPTH, sizeof(runtime_diag_event_t),
                                       s_queue_storage, &s_queue_control);
    s_media_mutex = xSemaphoreCreateMutex();
    if (s_event_queue == NULL || s_media_mutex == NULL) return ESP_ERR_NO_MEM;
    s_available = true;
    s_recovering = true;
    if (xTaskCreate(writer_task, "diaglog_writer", DIAGLOG_WRITER_STACK_BYTES,
                    NULL, 0, &s_writer_task) != pdPASS) {
        s_available = false;
        s_recovering = false;
        s_writer_task = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

bool runtime_diag_journal_record(const runtime_diag_event_t *event)
{
    if (event == NULL || event->event_type < RUNTIME_DIAG_EVENT_BOOT_START ||
        event->event_type > RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_RESULT) return false;
    portENTER_CRITICAL(&s_state_mux);
    if (!s_available || s_full || s_append_fault || s_event_queue == NULL) {
        s_volatile_drops = saturating_increment(s_volatile_drops);
        portEXIT_CRITICAL(&s_state_mux);
        return false;
    }
    portEXIT_CRITICAL(&s_state_mux);
    if (xQueueSend(s_event_queue, event, 0U) != pdTRUE) {
        record_drop();
        return false;
    }
    return true;
}

static bool scan_page_locked(
    uint32_t limit,
    uint64_t before_sequence,
    runtime_diag_record_t *records,
    size_t record_capacity,
    size_t *record_count,
    runtime_diag_journal_info_t *info)
{
    uint8_t *chunk = (uint8_t *)s_scan_buffer;
    runtime_diag_record_t decoded;
    bool have_sequence = false;
    uint64_t oldest = UINT64_MAX;
    uint64_t newest = 0U;
    uint32_t valid = 0U;
    uint32_t free_slots = 0U;

    for (uint32_t sector = 0U; sector < (DIAGLOG_SIZE_BYTES / DIAGLOG_SECTOR_SIZE); ++sector) {
        const uint32_t offset = sector * DIAGLOG_SECTOR_SIZE;
        if (!read_flash(offset, chunk, DIAGLOG_SECTOR_SIZE)) return false;
        for (uint32_t within = 0U; within < DIAGLOG_SECTOR_SIZE;
             within += RUNTIME_DIAG_JOURNAL_RECORD_SIZE) {
            const uint8_t *slot = chunk + within;
            if (slot_erased(slot)) {
                free_slots++;
            } else if (decode_record(slot, &decoded)) {
                valid++;
                if (decoded.sequence < oldest) oldest = decoded.sequence;
                if (decoded.sequence > newest) newest = decoded.sequence;
            }
        }
    }
    const uint64_t cutoff = before_sequence == 0U
        ? newest : (before_sequence == 0U ? 0U : before_sequence - 1U);
    *record_count = 0U;
    if (limit > record_capacity) return false;
    for (uint32_t sector = 0U; sector < (DIAGLOG_SIZE_BYTES / DIAGLOG_SECTOR_SIZE); ++sector) {
        const uint32_t offset = sector * DIAGLOG_SECTOR_SIZE;
        if (!read_flash(offset, chunk, DIAGLOG_SECTOR_SIZE)) return false;
        for (uint32_t within = 0U; within < DIAGLOG_SECTOR_SIZE;
             within += RUNTIME_DIAG_JOURNAL_RECORD_SIZE) {
            if (!decode_record(chunk + within, &decoded) || decoded.sequence > cutoff) continue;
            if (*record_count == limit) {
                if (decoded.sequence <= records[0].sequence) continue;
                memmove(records, records + 1U, (limit - 1U) * sizeof(*records));
                (*record_count)--;
            }
            size_t pos = 0U;
            while (pos < *record_count && records[pos].sequence < decoded.sequence) pos++;
            if (pos < *record_count && records[pos].sequence == decoded.sequence) continue;
            memmove(&records[pos + 1U], &records[pos],
                    (*record_count - pos) * sizeof(*records));
            records[pos] = decoded;
            (*record_count)++;
        }
    }

    info->capacity = RUNTIME_DIAG_JOURNAL_CAPACITY;
    info->free_slots = free_slots;
    info->valid_record_count = valid;
    info->oldest_sequence = valid == 0U ? 0U : oldest;
    info->newest_sequence = newest;
    portENTER_CRITICAL(&s_state_mux);
    info->available = s_available;
    info->recovering = s_recovering;
    info->journal_full = s_full;
    info->volatile_queue_drop_count = s_volatile_drops;
    portEXIT_CRITICAL(&s_state_mux);
    return true;
}

esp_err_t runtime_diag_journal_get_page(
    uint32_t limit,
    uint64_t before_sequence,
    runtime_diag_record_t *records,
    size_t record_capacity,
    size_t *record_count,
    runtime_diag_journal_info_t *info)
{
    if (records == NULL || record_count == NULL || info == NULL || limit == 0U ||
        limit > RUNTIME_DIAG_JOURNAL_PAGE_MAX || record_capacity < limit) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&s_state_mux);
    const bool available = s_available;
    portEXIT_CRITICAL(&s_state_mux);
    if (!available || s_media_mutex == NULL) return ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s_media_mutex, pdMS_TO_TICKS(250)) != pdTRUE) return ESP_ERR_TIMEOUT;
    const bool ok = scan_page_locked(limit, before_sequence, records, record_capacity,
                                     record_count, info);
    xSemaphoreGive(s_media_mutex);
    return ok ? ESP_OK : ESP_FAIL;
}

runtime_diag_reset_reason_t runtime_diag_map_reset_reason(int reason)
{
    switch ((esp_reset_reason_t)reason) {
    case ESP_RST_POWERON: return RUNTIME_DIAG_RESET_POWER_ON;
    case ESP_RST_EXT: return RUNTIME_DIAG_RESET_EXTERNAL;
    case ESP_RST_SW: return RUNTIME_DIAG_RESET_SOFTWARE;
    case ESP_RST_PANIC: return RUNTIME_DIAG_RESET_PANIC;
    case ESP_RST_INT_WDT: return RUNTIME_DIAG_RESET_INT_WATCHDOG;
    case ESP_RST_TASK_WDT: return RUNTIME_DIAG_RESET_TASK_WATCHDOG;
    case ESP_RST_WDT: return RUNTIME_DIAG_RESET_WATCHDOG;
    case ESP_RST_DEEPSLEEP: return RUNTIME_DIAG_RESET_DEEP_SLEEP;
    case ESP_RST_BROWNOUT: return RUNTIME_DIAG_RESET_BROWNOUT;
    case ESP_RST_SDIO: return RUNTIME_DIAG_RESET_SDIO;
    case ESP_RST_USB: return RUNTIME_DIAG_RESET_USB;
    case ESP_RST_JTAG: return RUNTIME_DIAG_RESET_JTAG;
    case ESP_RST_EFUSE: return RUNTIME_DIAG_RESET_EFUSE;
    case ESP_RST_PWR_GLITCH: return RUNTIME_DIAG_RESET_POWER_GLITCH;
    case ESP_RST_CPU_LOCKUP: return RUNTIME_DIAG_RESET_CPU_LOCKUP;
    case ESP_RST_UNKNOWN:
    default: return RUNTIME_DIAG_RESET_UNKNOWN;
    }
}
#endif
