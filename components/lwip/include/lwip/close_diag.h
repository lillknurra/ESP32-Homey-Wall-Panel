/* Bounded, read-only diagnostic trace for socket close ownership. */
#pragma once

#include <stdint.h>

#define LWIP_CLOSE_DIAG_CAPACITY 16U
#define LWIP_CLOSE_DIAG_SCOPE_CAPACITY 8U

typedef enum {
    LWIP_CLOSE_DIAG_SOCKET_CLOSE = 1,
    LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_BEGIN = 2,
    LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_END = 3,
    LWIP_CLOSE_DIAG_TLS_CONNECT_FAILURE_CLEANUP = 4,
    LWIP_CLOSE_DIAG_TLS_CONNECT_FAILURE_CLEANUP_END = 5,
    LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_BEGIN = 6,
    LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_END = 7,
} lwip_close_diag_event_kind_t;

typedef enum {
    LWIP_CLOSE_DIAG_RELEASE_CONFIRMED = 1,
    LWIP_CLOSE_DIAG_RELEASE_FAILED = 2,
    LWIP_CLOSE_DIAG_NO_SOCKET_ENTRY = 3,
    LWIP_CLOSE_DIAG_NO_CLOSE_ATTEMPT = 4,
    LWIP_CLOSE_DIAG_CLOSE_RESULT = 5,
    LWIP_CLOSE_DIAG_CLOSE_ATTEMPT_EXPECTED = 6,
    LWIP_CLOSE_DIAG_OPERATION_STARTED = 7,
    LWIP_CLOSE_DIAG_OPERATION_SUCCEEDED = 8,
    LWIP_CLOSE_DIAG_OPERATION_FAILED = 9,
} lwip_close_diag_outcome_t;

enum {
    LWIP_CLOSE_DIAG_FLAG_SOCKET_ENTRY_FOUND = 1U << 0,
    LWIP_CLOSE_DIAG_FLAG_SOCKET_SLOT_RELEASED = 1U << 1,
    LWIP_CLOSE_DIAG_FLAG_TLS_INITIALIZED = 1U << 2,
    LWIP_CLOSE_DIAG_FLAG_TLS_OBJECT_PRESENT = 1U << 3,
    LWIP_CLOSE_DIAG_FLAG_TLS_SOCKET_VALID = 1U << 4,
};

typedef struct {
    uint32_t sequence;
    uint32_t monotonic_ms;
    uint32_t correlation_id;
    int32_t result;
    int32_t error_number;
    uint8_t kind;
    uint8_t outcome;
    uint8_t flags;
    uint8_t reserved;
} lwip_close_diag_event_t;

typedef struct {
    uint32_t newest_sequence;
    uint32_t overwritten_count;
    uint32_t scope_overflow_count;
    uint32_t event_count;
    lwip_close_diag_event_t events[LWIP_CLOSE_DIAG_CAPACITY];
} lwip_close_diag_snapshot_t;

/* Copy a bounded snapshot. This function never clears or consumes the trace. */
void lwip_close_diag_snapshot(lwip_close_diag_snapshot_t *snapshot);

/* Mark a synchronous transport cleanup scope on the current task. Zero means
 * uncorrelated (for example scope-table exhaustion or unsupported nesting).
 * The task handle is internal only and is never included in a snapshot. */
uint32_t lwip_close_diag_scope_begin(void);
void lwip_close_diag_scope_end(uint32_t correlation_id);

/* Internal hook called at lwip_close's actual socket-table release boundary. */
void lwip_close_diag_socket_event(
    uint8_t outcome,
    uint8_t flags,
    int32_t result,
    int32_t error_number);

/* Internal cross-component marker used around the existing TLS close call. */
void lwip_close_diag_transport_event(
    uint8_t kind,
    uint8_t outcome,
    uint8_t flags,
    int32_t result,
    int32_t error_number);
