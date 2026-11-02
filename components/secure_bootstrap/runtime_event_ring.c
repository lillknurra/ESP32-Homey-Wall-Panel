#include "runtime_event_ring.h"

#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
static portMUX_TYPE s_ring_mux = portMUX_INITIALIZER_UNLOCKED;
#define RING_LOCK() portENTER_CRITICAL(&s_ring_mux)
#define RING_UNLOCK() portEXIT_CRITICAL(&s_ring_mux)
#else
#include <pthread.h>
static pthread_mutex_t s_ring_mux = PTHREAD_MUTEX_INITIALIZER;
#define RING_LOCK() ((void)pthread_mutex_lock(&s_ring_mux))
#define RING_UNLOCK() ((void)pthread_mutex_unlock(&s_ring_mux))
#endif

_Static_assert(sizeof(runtime_event_record_t) == 32U, "runtime event record budget");

static runtime_event_record_t s_records[RUNTIME_EVENT_RING_CAPACITY];
static uint32_t s_next_sequence = 1U;
static uint32_t s_count;
static uint32_t s_overwritten_count;
static uint32_t s_dropped_count;
static uint32_t s_runtime_id;
static uint32_t s_inventory_sequence;
static uint8_t s_origin;

void runtime_event_ring_set_context(uint32_t runtime_id,
                                    uint32_t inventory_sequence,
                                    uint8_t origin)
{
    RING_LOCK();
    s_runtime_id = runtime_id;
    s_inventory_sequence = inventory_sequence;
    s_origin = origin;
    RING_UNLOCK();
}

bool runtime_event_ring_record_at(uint32_t monotonic_ms,
                                  uint16_t event_id,
                                  uint8_t stage,
                                  int32_t result,
                                  uint32_t value_a,
                                  uint32_t value_b)
{
    if (event_id == 0U) {
        RING_LOCK();
        if (s_dropped_count != UINT32_MAX) s_dropped_count++;
        RING_UNLOCK();
        return false;
    }

    RING_LOCK();
    const uint32_t sequence = s_next_sequence;
    const size_t slot = (size_t)((sequence - 1U) % RUNTIME_EVENT_RING_CAPACITY);
    if (s_count == RUNTIME_EVENT_RING_CAPACITY) {
        if (s_overwritten_count != UINT32_MAX) s_overwritten_count++;
    } else {
        s_count++;
    }
    s_records[slot] = (runtime_event_record_t){
        .monotonic_ms = monotonic_ms,
        .sequence = sequence,
        .runtime_id = s_runtime_id,
        .inventory_sequence = s_inventory_sequence,
        .value_a = value_a,
        .value_b = value_b,
        .result = result,
        .event_id = event_id,
        .origin = s_origin,
        .stage = stage,
    };
    s_next_sequence = sequence == UINT32_MAX ? 1U : sequence + 1U;
    RING_UNLOCK();
    return true;
}

size_t runtime_event_ring_copy_page(runtime_event_record_t *records,
                                    size_t record_capacity,
                                    uint32_t before_sequence,
                                    runtime_event_ring_info_t *info)
{
    if (info == NULL || (record_capacity > 0U && records == NULL)) return 0U;
    RING_LOCK();
    const uint32_t oldest_sequence = s_next_sequence > s_count
        ? s_next_sequence - s_count : 1U;
    const uint32_t newest_sequence = s_next_sequence - 1U;
    uint32_t page_end = newest_sequence;
    if (before_sequence != 0U && before_sequence - 1U < page_end) {
        page_end = before_sequence - 1U;
    }
    const uint32_t available = page_end >= oldest_sequence
        ? page_end - oldest_sequence + 1U : 0U;
    const size_t copied = available < record_capacity ? available : record_capacity;
    const uint32_t page_first = copied == 0U ? 0U : page_end - (uint32_t)copied + 1U;
    for (size_t i = 0U; i < copied; ++i) {
        const uint32_t sequence = page_first + (uint32_t)i;
        records[i] = s_records[(size_t)((sequence - 1U) % RUNTIME_EVENT_RING_CAPACITY)];
    }
    *info = (runtime_event_ring_info_t){
        .count = s_count,
        .next_sequence = s_next_sequence,
        .overwritten_count = s_overwritten_count,
        .dropped_count = s_dropped_count,
        .runtime_id = s_runtime_id,
        .inventory_sequence = s_inventory_sequence,
        .oldest_sequence = s_count == 0U ? 0U : oldest_sequence,
        .newest_sequence = s_count == 0U ? 0U : newest_sequence,
        .page_first_sequence = page_first,
        .page_last_sequence = copied == 0U ? 0U : page_end,
        .origin = s_origin,
    };
    RING_UNLOCK();
    return copied;
}

size_t runtime_event_ring_copy_latest(runtime_event_record_t *records,
                                      size_t record_capacity,
                                      runtime_event_ring_info_t *info)
{
    return runtime_event_ring_copy_page(records, record_capacity, 0U, info);
}

const char *runtime_event_name(uint16_t event_id)
{
    switch ((runtime_event_id_t)event_id) {
    case RUNTIME_EVENT_HTTP_HANDLER_ENTER: return "http_handler_enter";
    case RUNTIME_EVENT_SNAPSHOT_COPY_BEGIN: return "snapshot_copy_begin";
    case RUNTIME_EVENT_SNAPSHOT_COPY_END: return "snapshot_copy_end";
    case RUNTIME_EVENT_DIAGNOSTICS_COPY_BEGIN: return "diagnostics_copy_begin";
    case RUNTIME_EVENT_DIAGNOSTICS_COPY_END: return "diagnostics_copy_end";
    case RUNTIME_EVENT_NVS_INSPECTION_BEGIN: return "nvs_inspection_begin";
    case RUNTIME_EVENT_NVS_INSPECTION_END: return "nvs_inspection_end";
    case RUNTIME_EVENT_JSON_BUILD_BEGIN: return "json_build_begin";
    case RUNTIME_EVENT_JSON_BUILD_END: return "json_build_end";
    case RUNTIME_EVENT_HTTP_SEND_BEGIN: return "http_send_begin";
    case RUNTIME_EVENT_HTTP_SEND_END: return "http_send_end";
    case RUNTIME_EVENT_HTTP_HANDLER_EXIT: return "http_handler_exit";
    case RUNTIME_EVENT_INVENTORY_BEGIN: return "inventory_begin";
    case RUNTIME_EVENT_INVENTORY_END: return "inventory_end";
    case RUNTIME_EVENT_INVENTORY_SUCCESS: return "inventory_success";
    case RUNTIME_EVENT_INVENTORY_FAILURE: return "inventory_failure";
    case RUNTIME_EVENT_SNAPSHOT_PUBLISH_BEGIN: return "snapshot_publish_begin";
    case RUNTIME_EVENT_SNAPSHOT_PUBLISH_SUCCESS: return "snapshot_publish_success";
    case RUNTIME_EVENT_SNAPSHOT_PUBLISH_REJECTED: return "snapshot_publish_rejected";
    case RUNTIME_EVENT_STAGE_BEGIN: return "stage_begin";
    case RUNTIME_EVENT_STAGE_END: return "stage_end";
    case RUNTIME_EVENT_HTTP_PERFORM_BEGIN: return "http_perform_begin";
    case RUNTIME_EVENT_HTTP_PERFORM_END: return "http_perform_end";
    case RUNTIME_EVENT_HTTP_CONNECTED: return "http_connected";
    case RUNTIME_EVENT_HTTP_HEADERS: return "http_headers";
    case RUNTIME_EVENT_HTTP_BODY_PROGRESS: return "http_body_progress";
    case RUNTIME_EVENT_HTTP_TRANSPORT_CLOSE: return "http_transport_close";
    case RUNTIME_EVENT_RESOURCE_SAMPLE: return "resource_sample";
    case RUNTIME_EVENT_WIFI_TRANSITION: return "wifi_transition";
    case RUNTIME_EVENT_HOMEY_STATE_CHANGE: return "homey_state_change";
    case RUNTIME_EVENT_HTTP_CONNECTION_REUSE: return "http_connection_reuse";
    case RUNTIME_EVENT_HTTP_HANDLE_CLEANUP: return "http_handle_cleanup";
    case RUNTIME_EVENT_HTTP_CONNECT_BEGIN: return "http_connect_begin";
    case RUNTIME_EVENT_HTTP_CONNECT_END: return "http_connect_end";
    case RUNTIME_EVENT_HTTP_READ_TERMINAL: return "http_read_terminal";
    case RUNTIME_EVENT_HTTP_CLIENT_ERROR: return "http_client_error";
    case RUNTIME_EVENT_HTTP_TLS_RESULT: return "http_tls_result";
    default: return "unknown";
    }
}

#ifdef ESP_PLATFORM
void runtime_event_ring_record_now(uint16_t event_id,
                                   uint8_t stage,
                                   int32_t result,
                                   uint32_t value_a,
                                   uint32_t value_b)
{
    const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000LL);
    (void)runtime_event_ring_record_at(now_ms, event_id, stage, result,
                                       value_a, value_b);
}

void runtime_event_ring_record_resources(uint16_t event_id, uint8_t stage)
{
    const uint32_t free_bytes = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const uint32_t largest_bytes = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const uint32_t stack_bytes = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    runtime_event_ring_record_now(event_id, stage, (int32_t)stack_bytes,
                                  free_bytes, largest_bytes);
}
#else
void runtime_event_ring_test_reset(void)
{
    RING_LOCK();
    memset(s_records, 0, sizeof(s_records));
    s_next_sequence = 1U;
    s_count = 0U;
    s_overwritten_count = 0U;
    s_dropped_count = 0U;
    s_runtime_id = 0U;
    s_inventory_sequence = 0U;
    s_origin = 0U;
    RING_UNLOCK();
}
#endif
