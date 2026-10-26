/* Patch116I8: OFFLINE-ONLY, split-phase DNS/HTTPS handoff coordinator.
 * This is a state machine, NOT a DNS resolver or an ESP-IDF transport driver.
 * Every call including Homey arrival MUST use the same serialized owner lock
 * as the I7 arbiter and all future Homey network-phase reservations.
 * No runtime network operation is authorized by this header.
 */
#pragma once

#include "panel_update_i7_admission_policy.h"
#include <stdbool.h>
#include <stdint.h>

/* Provisional offline contracts, NOT measured ESP32 latency guarantees. */
#define PANEL_UPDATE_I8_MAX_ATTESTED_CALL_MS 25U
#define PANEL_UPDATE_I8_MAX_ATTESTED_CLEANUP_MS 100U
#define PANEL_UPDATE_I8_MAX_LEASE_MS 15000U

typedef struct {
    /* ALL booleans must be proven by a separate native-adapter review before
     * any runtime use; setting them true does not create these capabilities. */
    bool shared_lock_covers_all_homey_phases;
    bool dns_callback_nonblocking;
    bool dns_abort_drains_or_tombstones_callbacks;
    bool tls_progress_nonblocking;
    bool http_progress_nonblocking;
    bool abort_is_cooperative;
    bool abort_never_reconnects;
    bool cleanup_destroys_all_io_resources;
    uint32_t worst_single_call_ms;
    uint32_t worst_cleanup_ack_ms;
} panel_update_i8_capabilities_t;

typedef enum {
    PANEL_UPDATE_I8_IDLE = 0,
    PANEL_UPDATE_I8_NEED_DNS,
    PANEL_UPDATE_I8_WAIT_DNS,
    PANEL_UPDATE_I8_NEED_TLS,
    PANEL_UPDATE_I8_WAIT_TLS,
    PANEL_UPDATE_I8_NEED_HTTP,
    PANEL_UPDATE_I8_WAIT_HTTP,
    PANEL_UPDATE_I8_NEED_CLOSE,
    PANEL_UPDATE_I8_WAIT_CLOSE,
    PANEL_UPDATE_I8_NEED_ABORT,
    PANEL_UPDATE_I8_WAIT_ABORT,
    PANEL_UPDATE_I8_QUARANTINED,
    PANEL_UPDATE_I8_FINISHED,
    PANEL_UPDATE_I8_CANCELLED,
} panel_update_i8_phase_t;

typedef enum {
    PANEL_UPDATE_I8_NO_ACTION = 0,
    PANEL_UPDATE_I8_START_ASYNC_DNS,
    PANEL_UPDATE_I8_START_ASYNC_TLS,
    PANEL_UPDATE_I8_START_ASYNC_HTTP_GET,
    PANEL_UPDATE_I8_CLOSE_TRANSPORT,
    PANEL_UPDATE_I8_ABORT_AND_CLOSE,
} panel_update_i8_action_t;

typedef enum {
    PANEL_UPDATE_I8_DNS_RESOLVED = 0,
    PANEL_UPDATE_I8_TLS_VERIFIED,
    PANEL_UPDATE_I8_HTTP_COMPLETE,
    PANEL_UPDATE_I8_IO_FAILED,
    /* Only after all I/O returned, socket/HTTP/TLS cleaned, AND any DNS
     * callback context is either drained or held as an immutable tombstone.
     * Caller must independently attest cleanup_verified=true. */
    PANEL_UPDATE_I8_CLEANUP_FINISHED,
} panel_update_i8_event_t;

typedef enum {
    PANEL_UPDATE_I8_HOMEY_ACQUIRED = 0,
    PANEL_UPDATE_I8_HOMEY_DEFERRED_RETRY_REQUIRED,
    PANEL_UPDATE_I8_HOMEY_BUSY_OTHER,
} panel_update_i8_homey_result_t;

typedef struct {
    panel_update_i8_phase_t phase;
    uint32_t generation;
    uint64_t started_ms;
    uint64_t last_ms;
    uint64_t total_deadline_ms;
    uint64_t cleanup_requested_ms;
    uint32_t cleanup_bound_ms;
    bool cancelled;
    bool http_completed;
} panel_update_i8_session_t;

/* The adapter must be genuinely split-phase. The returned action is a one-shot
 * *request*, never proof that actual DNS/TLS/HTTP can be cancelled. The adapter
 * must execute bounded operations and send events back under the shared lock.
 */
/* Sessions must be zero-initialized before first use (e.g. = {0}).
 * A live session MUST NOT be reset: it may own outstanding transport.
 * This operation refuses to clear a live/quarantined session. */
bool panel_update_i8_reset_terminal(panel_update_i8_session_t *s);
bool panel_update_i8_begin(panel_update_i8_session_t *s,
                           panel_update_i7_arbiter_t *arbiter,
                           uint32_t generation, uint64_t now_ms,
                           uint32_t total_budget_ms,
                           const panel_update_i7_admission_t *admission,
                           const panel_update_i8_capabilities_t *capabilities);

/* Homey must RETAIN its requested action, then retry acquisition after cleanup.
 * This helper does not enqueue/execute an awning command. */
panel_update_i8_homey_result_t panel_update_i8_homey_arrived(
    panel_update_i8_session_t *s, panel_update_i7_arbiter_t *arbiter);

/* Drives only state transitions. No I/O, allocations, tasks or HTTP here. */
panel_update_i8_action_t panel_update_i8_next(
    panel_update_i8_session_t *s, panel_update_i7_arbiter_t *arbiter,
    uint64_t now_ms, const panel_update_i7_admission_t *admission);

/* Events require the precise active generation. CLEANUP_FINISHED only releases
 * I7 on explicit verified cleanup; stale/duplicate events cannot unlock it. */
bool panel_update_i8_event(
    panel_update_i8_session_t *s, panel_update_i7_arbiter_t *arbiter,
    uint32_t generation, panel_update_i8_event_t event,
    uint64_t now_ms, bool cleanup_verified);

bool panel_update_i8_request_cancel(panel_update_i8_session_t *s,
                                    panel_update_i7_arbiter_t *arbiter);
