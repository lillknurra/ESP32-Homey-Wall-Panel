#pragma once

#include "runtime_event_ring.h"
#include "lwip/close_diag.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RUNTIME_EVENTS_JSON_CHUNK_SIZE 320U

typedef struct {
    uint32_t phase;
    uint32_t completed_phase;
    uint32_t failed_phase;
    int32_t init_error;
    uint16_t handler_count;
    uint16_t handler_capacity;
    bool auth_restore_started;
    bool command_worker_created;
    bool periodic_scheduler_started;
    bool inventory_queued;
    bool inventory_in_flight;
    bool recovery_pending;
    uint8_t retry_budget_remaining;
    uint32_t last_progress_ms;
} runtime_events_json_boot_progress_t;

typedef bool (*runtime_events_json_write_fn)(
    void *context, const char *data, size_t length);
typedef const char *(*runtime_events_json_event_name_fn)(uint16_t event_id);

typedef enum {
    RUNTIME_EVENTS_JSON_OK = 0,
    RUNTIME_EVENTS_JSON_INVALID_ARGUMENT,
    RUNTIME_EVENTS_JSON_FORMAT_ERROR,
    RUNTIME_EVENTS_JSON_WRITE_ERROR,
} runtime_events_json_result_t;

runtime_events_json_result_t runtime_events_json_write(
    const runtime_event_ring_info_t *info,
    uint32_t returned_count,
    uint32_t ring_capacity,
    uint32_t record_size_bytes,
    uint32_t internal_heap_free,
    uint32_t internal_heap_largest,
    uint32_t internal_heap_minimum,
    uint32_t http_server_stack_highwater_bytes,
    const runtime_events_json_boot_progress_t *boot_progress,
    const lwip_close_diag_snapshot_t *close_diag,
    const runtime_event_record_t *records,
    runtime_events_json_event_name_fn event_name,
    runtime_events_json_write_fn write_chunk,
    void *write_context);
