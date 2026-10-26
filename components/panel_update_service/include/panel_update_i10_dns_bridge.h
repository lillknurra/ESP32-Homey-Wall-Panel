/* Patch116I10: bounded-lifetime, IPv4-only async DNS bridge.
 * Offline/audit source only; it is not registered in native firmware CMake.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PANEL_UPDATE_I10_DNS_HOST "lillknurra.github.io"
#define PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY 16U
#define PANEL_UPDATE_I10_REQUEST_SLOT_COUNT 2U
#define PANEL_UPDATE_I10_SCHEDULER_TICKET_COUNT 32U
#define PANEL_UPDATE_I10_DNS_TICKET_COUNT 16U

typedef struct {
    uint64_t generation;
    uint8_t slot;
} panel_update_i10_handle_t;

typedef enum {
    PANEL_UPDATE_I10_START_ACCEPTED = 0,
    PANEL_UPDATE_I10_START_INVALID,
    PANEL_UPDATE_I10_START_NO_REQUEST_SLOT,
    PANEL_UPDATE_I10_START_NO_SCHEDULER_TICKET,
    PANEL_UPDATE_I10_START_SCHEDULING_FAILED,
    PANEL_UPDATE_I10_START_GENERATION_EXHAUSTED,
} panel_update_i10_start_result_t;

typedef enum {
    PANEL_UPDATE_I10_CANCELLED_OWNER_RELEASE_SAFE = 0,
    PANEL_UPDATE_I10_CANCEL_ALREADY_STALE,
} panel_update_i10_cancel_result_t;

typedef enum {
    PANEL_UPDATE_I10_RESULT_PENDING = 0,
    PANEL_UPDATE_I10_RESULT_IPV4,
    PANEL_UPDATE_I10_RESULT_DNS_FAILED,
    PANEL_UPDATE_I10_RESULT_STALE,
} panel_update_i10_result_t;

/* Schedules the fixed GitHub Pages hostname from the caller/worker thread with
 * tcpip_try_callback(). The result is polled with take_result(); no caller
 * callback or queue operation runs on the TCP/IP thread. No caller memory is
 * retained. No retry is performed on scheduling failure.
 */
panel_update_i10_start_result_t panel_update_i10_dns_start(
    panel_update_i10_handle_t *out_handle);

/* Cancelling DNS immediately permits the caller to release its network owner:
 * this adapter owns no socket/TLS/HTTP resource. A scheduled/DNS callback may
 * still arrive later; immutable ticket generations fence it from later slots.
 */
panel_update_i10_cancel_result_t panel_update_i10_dns_cancel(
    panel_update_i10_handle_t handle);

/* Nonblocking worker-side poll/take. Success copies an IPv4 literal and
 * consumes the request slot. DNS failure also consumes it. A stale or canceled
 * generation can never produce an address suitable for starting HTTPS.
 */
panel_update_i10_result_t panel_update_i10_dns_take_result(
    panel_update_i10_handle_t handle,
    char ipv4_out[PANEL_UPDATE_I10_IPV4_TEXT_CAPACITY]);
