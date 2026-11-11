#include "runtime_events_json.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OUTPUT_CAPACITY 16384U

typedef struct {
    char data[OUTPUT_CAPACITY];
    size_t length;
    size_t maximum_chunk;
    size_t fail_after;
} output_t;

static bool append_chunk(void *context, const char *data, size_t length)
{
    output_t *output = context;
    assert(output != NULL && data != NULL && length > 0U);
    if (length > output->maximum_chunk) output->maximum_chunk = length;
    if (output->length + length >= sizeof(output->data) ||
        output->length + length > output->fail_after) return false;
    memcpy(output->data + output->length, data, length);
    output->length += length;
    output->data[output->length] = '\0';
    return true;
}

static const char *event_name(uint16_t event_id)
{
    switch (event_id) {
    case 1U: return "http_handler_enter";
    case 15U: return "inventory_success";
    default: return "other_event";
    }
}

static void emit_case(const char *name, uint32_t count, bool boot_complete)
{
    runtime_event_record_t records[RUNTIME_EVENT_RING_PAGE_MAX] = {0};
    runtime_event_ring_info_t info = {
        .count = count,
        .overwritten_count = 2U,
        .dropped_count = 1U,
        .runtime_id = 1234U,
        .inventory_sequence = count == 0U ? 0U : 77U,
        .oldest_sequence = count == 0U ? 0U : 50U,
        .newest_sequence = count == 0U ? 0U : 49U + count,
        .page_first_sequence = count == 0U ? 0U : 50U,
        .page_last_sequence = count == 0U ? 0U : 49U + count,
        .origin = RUNTIME_EVENT_ORIGIN_PERIODIC,
    };
    for (uint32_t i = 0U; i < count; ++i) {
        records[i] = (runtime_event_record_t) {
            .monotonic_ms = 1000U + i,
            .sequence = 50U + i,
            .runtime_id = 1234U,
            .inventory_sequence = 77U,
            .value_a = 3U + i,
            .value_b = 4U + i,
            .result = 0,
            .event_id = i == 0U ? 1U : 15U,
            .origin = RUNTIME_EVENT_ORIGIN_PERIODIC,
            .stage = RUNTIME_EVENT_STAGE_OTHER,
        };
    }
    runtime_events_json_boot_progress_t boot = {0};
    lwip_close_diag_snapshot_t close_diag = {0};
    if (count != 0U) {
        close_diag.event_count = strcmp(name, "page_max") == 0
            ? LWIP_CLOSE_DIAG_CAPACITY
            : (count > LWIP_CLOSE_DIAG_CAPACITY ? LWIP_CLOSE_DIAG_CAPACITY : count);
        close_diag.newest_sequence = close_diag.event_count;
        close_diag.overwritten_count = count > close_diag.event_count ? 1U : 0U;
        close_diag.scope_overflow_count = strcmp(name, "page_max") == 0 ? 2U : 0U;
        for (uint32_t i = 0U; i < close_diag.event_count; ++i) {
            close_diag.events[i] = (lwip_close_diag_event_t) {
                .sequence = i + 1U,
                .monotonic_ms = 2000U + i,
                .correlation_id = strcmp(name, "page_max") == 0 && i < 4U ? 77U : 0U,
                .result = i == 1U ? -1 : 0,
                .error_number = i == 1U ? 11 : 0,
                .kind = i == 0U ? LWIP_CLOSE_DIAG_SOCKET_CLOSE :
                    (i == 1U ? LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_END :
                        (i == 2U ? LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_BEGIN :
                            (i == 3U ? LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_END :
                                LWIP_CLOSE_DIAG_TLS_CONNECT_FAILURE_CLEANUP))),
                .outcome = i == 0U ? LWIP_CLOSE_DIAG_RELEASE_CONFIRMED :
                    (i == 1U ? LWIP_CLOSE_DIAG_NO_CLOSE_ATTEMPT :
                        (i == 2U ? LWIP_CLOSE_DIAG_OPERATION_STARTED :
                            (i == 3U ? LWIP_CLOSE_DIAG_OPERATION_FAILED :
                                LWIP_CLOSE_DIAG_CLOSE_RESULT))),
                .flags = i == 0U
                    ? LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND |
                        LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED
                    : LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED,
            };
        }
    }
    if (boot_complete) {
        boot = (runtime_events_json_boot_progress_t) {
            .phase = 8U,
            .completed_phase = 7U,
            .failed_phase = 0U,
            .init_error = 0,
            .handler_count = 20U,
            .handler_capacity = 24U,
            .auth_restore_started = true,
            .command_worker_created = true,
            .periodic_scheduler_started = true,
            .inventory_queued = false,
            .inventory_in_flight = false,
            .recovery_pending = false,
            .retry_budget_remaining = 2U,
            .last_progress_ms = 9000U,
        };
    }
    output_t output = {.fail_after = SIZE_MAX};
    const runtime_events_json_result_t result = runtime_events_json_write(
        &info, count, 48U, 32U, 24000U, 16000U, 3000U, 512U,
        &boot, &close_diag, records, event_name, append_chunk, &output);
    assert(result == RUNTIME_EVENTS_JSON_OK);
    assert(output.maximum_chunk <= RUNTIME_EVENTS_JSON_CHUNK_SIZE - 1U);
    printf("CASE\t%s\t%s\n", name, output.data);
}

int main(void)
{
    emit_case("empty", 0U, false);
    emit_case("single", 1U, true);
    emit_case("multiple", 3U, true);
    emit_case("page_max", RUNTIME_EVENT_RING_PAGE_MAX, true);
    emit_case("page_boundary", 1U, false);

    runtime_event_ring_info_t info = {0};
    runtime_events_json_boot_progress_t boot = {0};
    lwip_close_diag_snapshot_t close_diag = {0};
    output_t short_output = {.fail_after = 16U};
    assert(runtime_events_json_write(
        &info, 0U, 48U, 32U, 1U, 1U, 1U, 1U, &boot, &close_diag,
        NULL, NULL, append_chunk, &short_output) == RUNTIME_EVENTS_JSON_WRITE_ERROR);
    assert(short_output.length <= short_output.fail_after);
    puts("CONTROL\ttruncated_writer\tPASS");
    assert(runtime_events_json_write(
        &info, 1U, 48U, 32U, 1U, 1U, 1U, 1U, &boot, &close_diag,
        NULL, NULL, append_chunk, &short_output) == RUNTIME_EVENTS_JSON_INVALID_ARGUMENT);
    puts("CONTROL\tinvalid_arguments\tPASS");
    return 0;
}
