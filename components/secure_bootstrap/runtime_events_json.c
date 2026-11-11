#include "runtime_events_json.h"

#include <inttypes.h>
#include <stdio.h>

static runtime_events_json_result_t write_formatted_chunk(
    char *chunk,
    size_t chunk_capacity,
    int written,
    runtime_events_json_write_fn write_chunk,
    void *write_context)
{
    if (written <= 0 || (size_t)written >= chunk_capacity) {
        return RUNTIME_EVENTS_JSON_FORMAT_ERROR;
    }
    return write_chunk(write_context, chunk, (size_t)written)
        ? RUNTIME_EVENTS_JSON_OK : RUNTIME_EVENTS_JSON_WRITE_ERROR;
}

static const char *close_event_kind_name(uint8_t kind)
{
    switch (kind) {
    case LWIP_CLOSE_DIAG_SOCKET_CLOSE: return "socket_close";
    case LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_BEGIN: return "transport_close_begin";
    case LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_END: return "transport_close_end";
    case LWIP_CLOSE_DIAG_TLS_CONNECT_FAILURE_CLEANUP: return "tls_connect_failure_cleanup";
    case LWIP_CLOSE_DIAG_TLS_CONNECT_FAILURE_CLEANUP_END: return "tls_connect_failure_cleanup_end";
    case LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_BEGIN: return "tls_connect_attempt_begin";
    case LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_END: return "tls_connect_attempt_end";
    default: return NULL;
    }
}

static const char *close_outcome_name(uint8_t outcome)
{
    switch (outcome) {
    case LWIP_CLOSE_DIAG_RELEASE_CONFIRMED: return "release_confirmed";
    case LWIP_CLOSE_DIAG_RELEASE_FAILED: return "release_failed";
    case LWIP_CLOSE_DIAG_NO_SOCKET_ENTRY: return "no_socket_entry";
    case LWIP_CLOSE_DIAG_NO_CLOSE_ATTEMPT: return "no_close_attempt";
    case LWIP_CLOSE_DIAG_CLOSE_RESULT: return "wrapper_result_only";
    case LWIP_CLOSE_DIAG_CLOSE_ATTEMPT_EXPECTED: return "close_attempt_expected";
    case LWIP_CLOSE_DIAG_OPERATION_STARTED: return "operation_started";
    case LWIP_CLOSE_DIAG_OPERATION_SUCCEEDED: return "operation_succeeded";
    case LWIP_CLOSE_DIAG_OPERATION_FAILED: return "operation_failed";
    default: return NULL;
    }
}

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
    void *write_context)
{
    if (info == NULL || boot_progress == NULL || close_diag == NULL ||
        (returned_count != 0U && (records == NULL || event_name == NULL)) ||
        write_chunk == NULL || returned_count > RUNTIME_EVENT_RING_PAGE_MAX ||
        close_diag->event_count > LWIP_CLOSE_DIAG_CAPACITY) {
        return RUNTIME_EVENTS_JSON_INVALID_ARGUMENT;
    }

    char chunk[RUNTIME_EVENTS_JSON_CHUNK_SIZE];
    int written = snprintf(chunk, sizeof(chunk),
        "{\"format_version\":1,\"capacity\":%u,\"record_size_bytes\":%u,"
        "\"count\":%" PRIu32 ",\"returned_count\":%u,\"overwritten_count\":%" PRIu32 ","
        "\"dropped_count\":%" PRIu32 ",\"oldest_sequence\":%" PRIu32 ",\"newest_sequence\":%" PRIu32 ",",
        (unsigned)ring_capacity, (unsigned)record_size_bytes,
        info->count, (unsigned)returned_count, info->overwritten_count,
        info->dropped_count, info->oldest_sequence, info->newest_sequence);
    runtime_events_json_result_t result = write_formatted_chunk(
        chunk, sizeof(chunk), written, write_chunk, write_context);
    if (result != RUNTIME_EVENTS_JSON_OK) return result;

    written = snprintf(chunk, sizeof(chunk),
        "\"page_first_sequence\":%" PRIu32 ",\"page_last_sequence\":%" PRIu32 ","
        "\"runtime_id\":%" PRIu32 ",\"inventory_sequence\":%" PRIu32 ","
        "\"origin\":%u,\"internal_heap_free\":%u,\"internal_heap_largest\":%u,"
        "\"internal_heap_minimum\":%u,\"http_server_stack_highwater_bytes\":%u,",
        info->page_first_sequence, info->page_last_sequence,
        info->runtime_id, info->inventory_sequence, (unsigned)info->origin,
        (unsigned)internal_heap_free, (unsigned)internal_heap_largest,
        (unsigned)internal_heap_minimum,
        (unsigned)http_server_stack_highwater_bytes);
    result = write_formatted_chunk(
        chunk, sizeof(chunk), written, write_chunk, write_context);
    if (result != RUNTIME_EVENTS_JSON_OK) return result;

    written = snprintf(chunk, sizeof(chunk),
        "\"boot_progress\":{\"phase\":%u,\"completed_phase\":%u,"
        "\"failed_phase\":%u,\"init_error\":%" PRId32 ",\"handler_count\":%u,"
        "\"handler_capacity\":%u,\"auth_restore_started\":%s,",
        (unsigned)boot_progress->phase,
        (unsigned)boot_progress->completed_phase,
        (unsigned)boot_progress->failed_phase,
        boot_progress->init_error,
        (unsigned)boot_progress->handler_count,
        (unsigned)boot_progress->handler_capacity,
        boot_progress->auth_restore_started ? "true" : "false");
    result = write_formatted_chunk(
        chunk, sizeof(chunk), written, write_chunk, write_context);
    if (result != RUNTIME_EVENTS_JSON_OK) return result;

    written = snprintf(chunk, sizeof(chunk),
        "\"command_worker_created\":%s,\"periodic_scheduler_started\":%s,"
        "\"inventory_queued\":%s,\"inventory_in_flight\":%s,"
        "\"recovery_pending\":%s,\"retry_budget_remaining\":%u,"
        "\"last_progress_ms\":%" PRIu32 "},",
        boot_progress->command_worker_created ? "true" : "false",
        boot_progress->periodic_scheduler_started ? "true" : "false",
        boot_progress->inventory_queued ? "true" : "false",
        boot_progress->inventory_in_flight ? "true" : "false",
        boot_progress->recovery_pending ? "true" : "false",
        (unsigned)boot_progress->retry_budget_remaining,
        boot_progress->last_progress_ms);
    result = write_formatted_chunk(
        chunk, sizeof(chunk), written, write_chunk, write_context);
    if (result != RUNTIME_EVENTS_JSON_OK) return result;

    written = snprintf(chunk, sizeof(chunk),
        "\"socket_close_diag\":{\"capacity\":%u,\"newest_sequence\":%" PRIu32 ","
        "\"overwritten_count\":%" PRIu32 ",\"scope_overflow_count\":%" PRIu32 ","
        "\"event_count\":%" PRIu32 ",\"events\":[",
        (unsigned)LWIP_CLOSE_DIAG_CAPACITY, close_diag->newest_sequence,
        close_diag->overwritten_count, close_diag->scope_overflow_count,
        close_diag->event_count);
    result = write_formatted_chunk(
        chunk, sizeof(chunk), written, write_chunk, write_context);
    if (result != RUNTIME_EVENTS_JSON_OK) return result;
    for (uint32_t i = 0U; i < close_diag->event_count; ++i) {
        const lwip_close_diag_event_t *event = &close_diag->events[i];
        const char *kind = close_event_kind_name(event->kind);
        const char *outcome = close_outcome_name(event->outcome);
        if (kind == NULL || outcome == NULL) return RUNTIME_EVENTS_JSON_FORMAT_ERROR;
        written = snprintf(chunk, sizeof(chunk),
            "%s{\"sequence\":%" PRIu32 ",\"monotonic_ms\":%" PRIu32 ","
            "\"correlation_id\":%" PRIu32 ","
            "\"kind\":\"%s\",\"outcome\":\"%s\",\"result\":%" PRId32 ","
            "\"error_number\":%" PRId32 ",\"socket_entry_found\":%s,"
            "\"socket_slot_released\":%s,\"tls_initialized\":%s,"
            "\"tls_object_present\":%s,\"tls_socket_valid\":%s}",
            i == 0U ? "" : ",", event->sequence, event->monotonic_ms,
            event->correlation_id,
            kind, outcome, event->result, event->error_number,
            (event->flags & LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND) != 0U ? "true" : "false",
            (event->flags & LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED) != 0U ? "true" : "false",
            (event->flags & LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED) != 0U ? "true" : "false",
            (event->flags & LWIP_CLOSE_DIAG_FLAG_TLS_OBJECT_PRESENT) != 0U ? "true" : "false",
            (event->flags & LWIP_CLOSE_DIAG_FLAG_TLS_SOCKET_VALID) != 0U ? "true" : "false");
        result = write_formatted_chunk(
            chunk, sizeof(chunk), written, write_chunk, write_context);
        if (result != RUNTIME_EVENTS_JSON_OK) return result;
    }
    result = write_chunk(write_context, "]},", 3U)
        ? RUNTIME_EVENTS_JSON_OK : RUNTIME_EVENTS_JSON_WRITE_ERROR;
    if (result != RUNTIME_EVENTS_JSON_OK) return result;

    result = write_chunk(write_context, "\"records\":[", 11U)
        ? RUNTIME_EVENTS_JSON_OK : RUNTIME_EVENTS_JSON_WRITE_ERROR;
    if (result != RUNTIME_EVENTS_JSON_OK) return result;

    for (uint32_t i = 0U; i < returned_count; ++i) {
        const runtime_event_record_t *record = &records[i];
        const char *name = event_name(record->event_id);
        if (name == NULL) return RUNTIME_EVENTS_JSON_FORMAT_ERROR;
        written = snprintf(chunk, sizeof(chunk),
            "%s{\"sequence\":%" PRIu32 ",\"monotonic_ms\":%" PRIu32 ",\"runtime_id\":%" PRIu32 ","
            "\"inventory_sequence\":%" PRIu32 ",\"event_id\":%u,\"event\":\"%s\","
            "\"origin\":%u,\"stage\":%u,\"result\":%" PRId32 ",\"value_a\":%" PRIu32 ",\"value_b\":%" PRIu32 "}",
            i == 0U ? "" : ",", record->sequence, record->monotonic_ms,
            record->runtime_id, record->inventory_sequence,
            (unsigned)record->event_id, name, (unsigned)record->origin,
            (unsigned)record->stage, record->result, record->value_a,
            record->value_b);
        result = write_formatted_chunk(
            chunk, sizeof(chunk), written, write_chunk, write_context);
        if (result != RUNTIME_EVENTS_JSON_OK) return result;
    }

    return write_chunk(write_context, "]}", 2U)
        ? RUNTIME_EVENTS_JSON_OK : RUNTIME_EVENTS_JSON_WRITE_ERROR;
}
