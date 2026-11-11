#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RUNTIME_EVENT_RING_CAPACITY 48U
#define RUNTIME_EVENT_RING_PAGE_MAX 12U

typedef enum {
    RUNTIME_EVENT_HTTP_HANDLER_ENTER = 1,
    RUNTIME_EVENT_SNAPSHOT_COPY_BEGIN,
    RUNTIME_EVENT_SNAPSHOT_COPY_END,
    RUNTIME_EVENT_DIAGNOSTICS_COPY_BEGIN,
    RUNTIME_EVENT_DIAGNOSTICS_COPY_END,
    RUNTIME_EVENT_NVS_INSPECTION_BEGIN,
    RUNTIME_EVENT_NVS_INSPECTION_END,
    RUNTIME_EVENT_JSON_BUILD_BEGIN,
    RUNTIME_EVENT_JSON_BUILD_END,
    RUNTIME_EVENT_HTTP_SEND_BEGIN,
    RUNTIME_EVENT_HTTP_SEND_END,
    RUNTIME_EVENT_HTTP_HANDLER_EXIT,
    RUNTIME_EVENT_INVENTORY_BEGIN,
    RUNTIME_EVENT_INVENTORY_END,
    RUNTIME_EVENT_INVENTORY_SUCCESS,
    RUNTIME_EVENT_INVENTORY_FAILURE,
    RUNTIME_EVENT_SNAPSHOT_PUBLISH_BEGIN,
    RUNTIME_EVENT_SNAPSHOT_PUBLISH_SUCCESS,
    RUNTIME_EVENT_SNAPSHOT_PUBLISH_REJECTED,
    RUNTIME_EVENT_STAGE_BEGIN,
    RUNTIME_EVENT_STAGE_END,
    RUNTIME_EVENT_HTTP_PERFORM_BEGIN,
    RUNTIME_EVENT_HTTP_PERFORM_END,
    RUNTIME_EVENT_HTTP_CONNECTED,
    RUNTIME_EVENT_HTTP_HEADERS,
    RUNTIME_EVENT_HTTP_BODY_PROGRESS,
    RUNTIME_EVENT_HTTP_TRANSPORT_CLOSE,
    RUNTIME_EVENT_RESOURCE_SAMPLE,
    RUNTIME_EVENT_WIFI_TRANSITION,
    RUNTIME_EVENT_HOMEY_STATE_CHANGE,
    RUNTIME_EVENT_HTTP_CONNECTION_REUSE,
    RUNTIME_EVENT_HTTP_HANDLE_CLEANUP,
    RUNTIME_EVENT_HTTP_CONNECT_BEGIN,
    RUNTIME_EVENT_HTTP_CONNECT_END,
    RUNTIME_EVENT_HTTP_READ_TERMINAL,
    RUNTIME_EVENT_HTTP_CLIENT_ERROR,
    RUNTIME_EVENT_HTTP_TLS_RESULT,
} runtime_event_id_t;

typedef enum {
    RUNTIME_EVENT_ORIGIN_NONE = 0,
    RUNTIME_EVENT_ORIGIN_BOOT_AUTO = 1,
    RUNTIME_EVENT_ORIGIN_PERIODIC = 2,
    RUNTIME_EVENT_ORIGIN_MANUAL = 3,
    RUNTIME_EVENT_ORIGIN_OTHER = 4,
} runtime_event_origin_t;

typedef enum {
    RUNTIME_EVENT_STAGE_NONE = 0,
    RUNTIME_EVENT_STAGE_FAVORITES = 1,
    RUNTIME_EVENT_STAGE_ZONES = 2,
    RUNTIME_EVENT_STAGE_DEVICES = 3,
    RUNTIME_EVENT_STAGE_HTTP_HANDLER = 4,
    RUNTIME_EVENT_STAGE_WIFI = 5,
    RUNTIME_EVENT_STAGE_OTHER = 255,
} runtime_event_stage_t;

/* Exactly 32 bytes. Values are event-specific; no payload or identity is stored. */
typedef struct {
    uint32_t monotonic_ms;
    uint32_t sequence;
    uint32_t runtime_id;
    uint32_t inventory_sequence;
    uint32_t value_a;
    uint32_t value_b;
    int32_t result;
    uint16_t event_id;
    uint8_t origin;
    uint8_t stage;
} runtime_event_record_t;

typedef struct {
    uint32_t count;
    uint32_t next_sequence;
    uint32_t overwritten_count;
    uint32_t dropped_count;
    uint32_t runtime_id;
    uint32_t inventory_sequence;
    uint32_t oldest_sequence;
    uint32_t newest_sequence;
    uint32_t page_first_sequence;
    uint32_t page_last_sequence;
    uint8_t origin;
} runtime_event_ring_info_t;

void runtime_event_ring_set_context(uint32_t runtime_id,
                                    uint32_t inventory_sequence,
                                    uint8_t origin);
bool runtime_event_ring_record_at(uint32_t monotonic_ms,
                                  uint16_t event_id,
                                  uint8_t stage,
                                  int32_t result,
                                  uint32_t value_a,
                                  uint32_t value_b);
size_t runtime_event_ring_copy_latest(runtime_event_record_t *records,
                                      size_t record_capacity,
                                      runtime_event_ring_info_t *info);
size_t runtime_event_ring_copy_page(runtime_event_record_t *records,
                                    size_t record_capacity,
                                    uint32_t before_sequence,
                                    runtime_event_ring_info_t *info);
const char *runtime_event_name(uint16_t event_id);

#ifdef ESP_PLATFORM
void runtime_event_ring_record_now(uint16_t event_id,
                                   uint8_t stage,
                                   int32_t result,
                                   uint32_t value_a,
                                   uint32_t value_b);
void runtime_event_ring_record_resources(uint16_t event_id, uint8_t stage);
#else
void runtime_event_ring_test_reset(void);
#endif
