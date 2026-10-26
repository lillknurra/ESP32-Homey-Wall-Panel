/* Patch116G — transport-only bounded state model, no networking/flash calls.
 * Host-model evidence does not prove the ESP-IDF adapter is bounded.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PANEL_OTA_ASYNC_MAX_IMAGE_BYTES UINT32_C(0x400000)
#define PANEL_OTA_ASYNC_PREAMBLE_BYTES UINT32_C(288)
#define PANEL_OTA_ASYNC_MAX_TOTAL_MS UINT32_C(600000)
#define PANEL_OTA_ASYNC_MAX_EVENTS UINT32_C(200000)

typedef enum {
    PANEL_OTA_ASYNC_DNS = 0,
    PANEL_OTA_ASYNC_TLS,
    PANEL_OTA_ASYNC_REQUEST,
    PANEL_OTA_ASYNC_HEADERS,
    PANEL_OTA_ASYNC_PREAMBLE,
    PANEL_OTA_ASYNC_BODY,
    PANEL_OTA_ASYNC_VERIFY,
    PANEL_OTA_ASYNC_BOOT_SELECT,
    PANEL_OTA_ASYNC_DONE,
    PANEL_OTA_ASYNC_FAILED,
} panel_ota_async_phase_t;

typedef enum {
    PANEL_OTA_ASYNC_POLL = 0,
    PANEL_OTA_ASYNC_DNS_RESOLVED,
    PANEL_OTA_ASYNC_TLS_VERIFIED,
    PANEL_OTA_ASYNC_REQUEST_SENT,
    PANEL_OTA_ASYNC_HEADERS_ACCEPTED,
    PANEL_OTA_ASYNC_PREAMBLE_VERIFIED,
    PANEL_OTA_ASYNC_BODY_BYTES,
    PANEL_OTA_ASYNC_IMAGE_VALIDATED,
    PANEL_OTA_ASYNC_BOOT_SELECTED,
    PANEL_OTA_ASYNC_CANCEL,
    PANEL_OTA_ASYNC_IO_ERROR,
} panel_ota_async_event_type_t;

typedef enum {
    PANEL_OTA_ASYNC_CONTINUE = 0,
    PANEL_OTA_ASYNC_IGNORE_STALE,
    PANEL_OTA_ASYNC_FINISHED,
    PANEL_OTA_ASYNC_REJECTED,
} panel_ota_async_outcome_t;

typedef enum {
    PANEL_OTA_ASYNC_REASON_NONE = 0,
    PANEL_OTA_ASYNC_REASON_INVALID_INPUT,
    PANEL_OTA_ASYNC_REASON_CANCELLED,
    PANEL_OTA_ASYNC_REASON_AUTH_REVOKED,
    PANEL_OTA_ASYNC_REASON_TIMEOUT,
    PANEL_OTA_ASYNC_REASON_EVENT_LIMIT,
    PANEL_OTA_ASYNC_REASON_ORDER,
    PANEL_OTA_ASYNC_REASON_TRANSFER,
    PANEL_OTA_ASYNC_REASON_IO,
} panel_ota_async_reason_t;

typedef struct {
    uint32_t total_ms;
    uint32_t dns_ms;
    uint32_t tls_ms;
    uint32_t request_ms;
    uint32_t headers_ms;
    uint32_t preamble_ms;
    uint32_t body_ms;
    uint32_t verify_ms;
    uint32_t boot_select_ms;
} panel_ota_async_limits_t;

typedef struct {
    panel_ota_async_event_type_t kind;
    uint32_t generation;
    uint32_t length; /* HEADERS_ACCEPTED: total bytes; BODY_BYTES: increment */
} panel_ota_async_event_t;

typedef struct {
    panel_ota_async_phase_t phase;
    panel_ota_async_reason_t reason;
    uint32_t generation;
    uint32_t expected_image_length;
    uint32_t bytes_received;
    uint32_t event_count;
    uint32_t phase_events;
    uint64_t started_us;
    uint64_t phase_started_us;
    uint64_t last_us;
    panel_ota_async_limits_t limits;
} panel_ota_async_state_t;

/* Generation is a stale-event fence, NOT an authentication token.
 * 'authorized' means a separately verified authenticated operator action,
 * and 'pre_display' means RGB was never started in this boot.
 */
bool panel_ota_async_begin(panel_ota_async_state_t *out,
                           const panel_ota_async_limits_t *limits,
                           uint32_t generation, uint64_t now_us);
panel_ota_async_outcome_t panel_ota_async_step(
    panel_ota_async_state_t *state, const panel_ota_async_event_t *event,
    uint64_t now_us, bool authorized, bool pre_display);
bool panel_ota_async_can_begin_flash(const panel_ota_async_state_t *state,
                                      uint64_t now_us, bool authorized, bool pre_display);
bool panel_ota_async_can_select_boot(const panel_ota_async_state_t *state,
                                    uint64_t now_us, bool authorized, bool pre_display);
