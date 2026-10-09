#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RUNTIME_DIAG_JOURNAL_RECORD_SIZE 64U
#define RUNTIME_DIAG_JOURNAL_CAPACITY 16384U
#define RUNTIME_DIAG_JOURNAL_PAGE_MAX 256U
#define RUNTIME_DIAG_JOURNAL_PAGE_DEFAULT 64U

typedef enum {
    RUNTIME_DIAG_EVENT_BOOT_START = 1,
    RUNTIME_DIAG_EVENT_WIFI_START,
    RUNTIME_DIAG_EVENT_WIFI_GOT_IP,
    RUNTIME_DIAG_EVENT_WIFI_ONLINE,
    RUNTIME_DIAG_EVENT_HTTP_SERVER_START_BEGIN,
    RUNTIME_DIAG_EVENT_HTTP_SERVER_START_RESULT,
    RUNTIME_DIAG_EVENT_AUTH_RESTORE_BEGIN,
    RUNTIME_DIAG_EVENT_AUTH_RESTORE_RESULT,
    RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_BEGIN,
    RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_ATTEMPT_RESULT,
    RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_RETRY,
    RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_RESULT,
    RUNTIME_DIAG_EVENT_BOOT_AUTO_SCHEDULER_BEGIN,
    RUNTIME_DIAG_EVENT_BOOT_AUTO_QUEUE_RESULT,
    RUNTIME_DIAG_EVENT_HOMEY_REFRESH_BEGIN,
    RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_BEGIN,
    RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_FAILURE,
    RUNTIME_DIAG_EVENT_HOMEY_RETRY_SCHEDULED,
    RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_SUCCESS,
    RUNTIME_DIAG_EVENT_SNAPSHOT_PUBLISH_SUCCESS,
    RUNTIME_DIAG_EVENT_SNAPSHOT_PUBLISH_FAILURE,
    RUNTIME_DIAG_EVENT_HOMEY_DATA_STATE_CHANGE,
    RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_QUEUE_RESULT,
    RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_RESULT,
} runtime_diag_event_type_t;

typedef enum {
    RUNTIME_DIAG_RESET_UNKNOWN = 0,
    RUNTIME_DIAG_RESET_POWER_ON,
    RUNTIME_DIAG_RESET_EXTERNAL,
    RUNTIME_DIAG_RESET_SOFTWARE,
    RUNTIME_DIAG_RESET_PANIC,
    RUNTIME_DIAG_RESET_INT_WATCHDOG,
    RUNTIME_DIAG_RESET_TASK_WATCHDOG,
    RUNTIME_DIAG_RESET_WATCHDOG,
    RUNTIME_DIAG_RESET_DEEP_SLEEP,
    RUNTIME_DIAG_RESET_BROWNOUT,
    RUNTIME_DIAG_RESET_SDIO,
    RUNTIME_DIAG_RESET_USB,
    RUNTIME_DIAG_RESET_JTAG,
    RUNTIME_DIAG_RESET_EFUSE,
    RUNTIME_DIAG_RESET_POWER_GLITCH,
    RUNTIME_DIAG_RESET_CPU_LOCKUP,
} runtime_diag_reset_reason_t;

typedef struct {
    uint16_t event_type;
    uint8_t source;
    uint8_t origin;
    uint16_t attempt;
    uint16_t result;
    int32_t error_code;
    uint16_t http_status;
    uint8_t transport;
    uint8_t stage;
    uint32_t retry_delay_ms;
    uint32_t snapshot_generation;
    uint16_t reset_reason;
    uint64_t monotonic_ms;
} runtime_diag_event_t;

typedef struct {
    uint64_t sequence;
    uint32_t boot_sequence;
    uint64_t monotonic_ms;
    uint16_t event_type;
    uint8_t source;
    uint8_t origin;
    uint16_t attempt;
    uint16_t result;
    int32_t error_code;
    uint16_t http_status;
    uint8_t transport;
    uint8_t stage;
    uint32_t retry_delay_ms;
    uint32_t snapshot_generation;
    uint16_t reset_reason;
    uint16_t flags;
    uint32_t drop_count;
} runtime_diag_record_t;

typedef struct {
    bool available;
    bool recovering;
    bool journal_full;
    uint32_t capacity;
    uint32_t free_slots;
    uint32_t valid_record_count;
    uint64_t oldest_sequence;
    uint64_t newest_sequence;
    uint32_t volatile_queue_drop_count;
} runtime_diag_journal_info_t;

#ifdef ESP_PLATFORM
#include "esp_err.h"
esp_err_t runtime_diag_journal_start(runtime_diag_reset_reason_t reset_reason);
bool runtime_diag_journal_record(const runtime_diag_event_t *event);
esp_err_t runtime_diag_journal_get_page(
    uint32_t limit,
    uint64_t before_sequence,
    runtime_diag_record_t *records,
    size_t record_capacity,
    size_t *record_count,
    runtime_diag_journal_info_t *info);
runtime_diag_reset_reason_t runtime_diag_map_reset_reason(int reason);
#else
/* Test-only adapters bind an in-memory 0xff flash image. */
bool runtime_diag_test_recover(const uint8_t *media, size_t media_size,
                               uint32_t *next_slot, uint64_t *next_sequence,
                               uint32_t *boot_sequence, uint32_t *valid_count);
void runtime_diag_test_encode(const runtime_diag_record_t *record,
                              uint8_t bytes[RUNTIME_DIAG_JOURNAL_RECORD_SIZE]);
bool runtime_diag_test_decode(const uint8_t bytes[RUNTIME_DIAG_JOURNAL_RECORD_SIZE],
                              runtime_diag_record_t *record);
bool runtime_diag_test_append(uint8_t *media, size_t media_size,
                              uint32_t slot, uint64_t sequence,
                              const runtime_diag_record_t *record,
                              size_t interrupt_after_bytes);
bool runtime_diag_test_is_erased(const uint8_t *slot);
#endif

const char *runtime_diag_event_name(uint16_t event_type);
const char *runtime_diag_reset_reason_name(uint16_t reason);
bool runtime_diag_journal_parse_query(
    const char *query,
    size_t query_length,
    uint32_t *limit_out,
    uint64_t *before_sequence_out);
