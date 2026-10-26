/* Patch116I7: offline model for Homey-first, exclusive network admission.
 * No FreeRTOS locks, TLS, HTTP, Homey calls or OTA install are implemented.
 * The caller MUST serialize every transition on a single shared owner/lock.
 * This policy must not be wired into running firmware before a separate audit.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PANEL_UPDATE_I7_MIN_INTERNAL_FREE_BYTES (64U * 1024U)
#define PANEL_UPDATE_I7_MIN_INTERNAL_LARGEST_BYTES (24U * 1024U)
#define PANEL_UPDATE_I7_MIN_OWNER_STACK_MARGIN_BYTES 3072U

typedef enum {
    PANEL_UPDATE_I7_IDLE = 0,
    PANEL_UPDATE_I7_HOMEY_OWNED,
    PANEL_UPDATE_I7_OTA_RESERVED,
    PANEL_UPDATE_I7_OTA_ACTIVE,
    PANEL_UPDATE_I7_OTA_CANCEL_PENDING,
} panel_update_i7_phase_t;

typedef struct {
    /* The host model never grants permission to perform a live trial.
     * All gates must be independently attested by a future runtime adapter. */
    bool trial_separately_authorized;
    bool wifi_stable;
    bool normal_boot_settled;
    bool homey_ready;
    bool homey_network_phase_idle;
    bool homey_command_queue_empty;
    bool homey_tls_closed;
    bool cloud_tls_closed;
    bool other_outbound_tls_quiescent;
    uint32_t internal_free_bytes;
    uint32_t internal_largest_bytes;
    uint32_t owner_stack_margin_bytes;
} panel_update_i7_admission_t;

typedef struct {
    panel_update_i7_phase_t phase;
    uint32_t generation;
    uint32_t last_generation;
    bool homey_pending;
    bool ota_transport_outstanding;
} panel_update_i7_arbiter_t;

/* Pure-C model. A future adapter must use the SAME exclusive lock for
 * Homey phases and OTA; checking an unlocked snapshot is NOT sufficient. */
void panel_update_i7_init(panel_update_i7_arbiter_t *arbiter);

/* If OTA has the owner, record Homey priority and request cooperative OTA
 * cancellation; return false. Homey MUST retry after OTA cleanup/release.
 * No function can preempt a synchronous blocking DNS/TLS/HTTP operation. */
bool panel_update_i7_homey_try_acquire(panel_update_i7_arbiter_t *arbiter);
bool panel_update_i7_homey_release(panel_update_i7_arbiter_t *arbiter);
/* Only call if every pending Homey request has actually been withdrawn. */
bool panel_update_i7_homey_withdraw_pending(panel_update_i7_arbiter_t *arbiter);

/* Reserve OTA only after verified idle/quiescent Homey, sufficient measured
 * resources and separately approved experiment; no network call takes place. */
bool panel_update_i7_ota_try_reserve(panel_update_i7_arbiter_t *arbiter,
                                     uint32_t generation,
                                     const panel_update_i7_admission_t *evidence);
/* Re-attest immediately before first network operation. */
bool panel_update_i7_ota_begin(panel_update_i7_arbiter_t *arbiter,
                                uint32_t generation,
                                const panel_update_i7_admission_t *evidence);
/* Checkpoint may only deny further OTA I/O. It cannot interrupt blocking I/O. */
bool panel_update_i7_ota_checkpoint(panel_update_i7_arbiter_t *arbiter,
                                     uint32_t generation,
                                     const panel_update_i7_admission_t *evidence);
/* MUST happen BEFORE entering any DNS/TLS/HTTP operation that may block.
 * Treat the transport as held until all I/O has returned AND cleanup is done. */
bool panel_update_i7_ota_claim_transport(panel_update_i7_arbiter_t *arbiter,
                                          uint32_t generation);
/* The caller may attest cleanup only after transport operations returned
 * and all owned socket/HTTP/TLS resources are fully closed/cleaned. */
bool panel_update_i7_ota_ack_cleanup(panel_update_i7_arbiter_t *arbiter,
                                      uint32_t generation,
                                      bool cleanup_verified);
/* A logical timeout requests cancellation, never forcibly deletes a task. */
bool panel_update_i7_ota_request_cancel(panel_update_i7_arbiter_t *arbiter,
                                         uint32_t generation);
/* Never releases a potentially live/blocked transport or a stale generation. */
bool panel_update_i7_ota_release(panel_update_i7_arbiter_t *arbiter,
                                  uint32_t generation);
