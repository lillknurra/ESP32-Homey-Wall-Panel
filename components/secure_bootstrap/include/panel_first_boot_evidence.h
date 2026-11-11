#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_FIRST_BOOT_EVIDENCE_JSON_MAX 1920U

enum {
    PANEL_FIRST_BOOT_TRANSPORT_HTTP_STATUS = 1U << 0,
    PANEL_FIRST_BOOT_TRANSPORT_TLS = 1U << 1,
    PANEL_FIRST_BOOT_TRANSPORT_SOCKET_ERRNO = 1U << 2,
    PANEL_FIRST_BOOT_TRANSPORT_CONNECT_RESULT = 1U << 3,
    PANEL_FIRST_BOOT_TRANSPORT_ELAPSED = 1U << 4,
    PANEL_FIRST_BOOT_TRANSPORT_CLASSIFICATION = 1U << 5,
};

/* Sanitized scalars from one request in the immutable first BOOT_AUTO attempt.
 * Values without their availability bit are exposed as JSON null. */
typedef struct {
    uint32_t request_sequence;
    int32_t perform_error;
    int32_t http_status;
    int32_t tls_query;
    int32_t tls_error;
    int32_t tls_flags;
    int32_t socket_errno;
    int32_t connect_result;
    uint32_t elapsed_ms;
    uint8_t available_fields;
    uint8_t classification;
    uint8_t request_count;
    uint8_t captured_request;
    uint8_t failed_request;
    bool available;
} panel_first_boot_transport_t;

typedef enum {
    PANEL_FIRST_BOOT_INVENTORY_NOT_STARTED = 0,
    PANEL_FIRST_BOOT_INVENTORY_IN_PROGRESS = 1,
    PANEL_FIRST_BOOT_INVENTORY_FAILED = 2,
    PANEL_FIRST_BOOT_INVENTORY_SUCCESS = 3,
} panel_first_boot_inventory_state_t;

typedef struct {
    uint32_t sequence;
    uint32_t attempt;
    uint32_t started_ms;
    uint32_t completed_ms;
    int32_t final_error;
    int32_t final_http_status; /* -1 means not available. */
    uint8_t stage;             /* Existing inventory-stage enum value. */
    bool snapshot_published;
    bool favorites_attempted;
    int32_t favorites_error;
    int32_t favorites_http_status;
    uint32_t favorites_elapsed_ms;
    uint8_t zones_result;      /* 0 unknown, 1 complete, 2 failed. */
    int32_t zones_http_status; /* -1 means not available. */
    bool devices_available;
    int32_t devices_error;
    int32_t devices_http_status;
    int32_t devices_expected_bytes; /* -1 means not available/chunked. */
    uint32_t devices_received_bytes;
    uint32_t devices_elapsed_ms;
    bool devices_body_complete;
    panel_first_boot_transport_t favorites_transport;
    panel_first_boot_transport_t zones_transport;
} panel_first_boot_inventory_t;

typedef struct {
    uint32_t runtime_id;
    uint32_t auth_started_ms;
    int32_t auth_error;
    uint8_t auth_state; /* 0 not started, 1 in progress, 2 restored, 3 absent, 4 error. */
    bool selected_homey_present;
    uint32_t scheduler_started_ms;
    bool scheduler_started;
    uint8_t queue_attempts;
    int8_t queue_first_result; /* -1 means no queue result. */
    int8_t queue_last_result;
    uint8_t scheduler_skip_reason; /* 0 none, 1 already ready, 2 queue wait exhausted, 3 create failed. */
    panel_first_boot_inventory_state_t inventory_state;
    panel_first_boot_inventory_t inventory;
    bool recovery_used_before_first_ready;
    bool first_ready_reached;
    uint8_t first_ready_origin;
    uint32_t first_ready_attempt;
    uint32_t first_ready_elapsed_ms;
} panel_first_boot_evidence_t;

typedef struct {
    uint32_t sequence;
    uint8_t origin;
    uint32_t attempt;
    uint32_t started_ms;
    uint32_t completed_ms;
    int32_t final_error;
    int32_t final_http_status;
    uint8_t stage;
    bool verified;
    bool snapshot_published;
    bool favorites_attempted;
    int32_t favorites_error;
    int32_t favorites_http_status;
    uint32_t favorites_elapsed_ms;
    bool devices_available;
    int32_t devices_error;
    int32_t devices_http_status;
    int32_t devices_expected_bytes;
    uint32_t devices_received_bytes;
    uint32_t devices_elapsed_ms;
    bool devices_body_complete;
    panel_first_boot_transport_t favorites_transport;
    panel_first_boot_transport_t zones_transport;
} panel_first_boot_attempt_result_t;

void panel_first_boot_evidence_init(uint32_t runtime_id);
void panel_first_boot_evidence_auth_started(uint32_t now_ms);
void panel_first_boot_evidence_auth_result(uint32_t now_ms, int32_t error,
                                           bool record_present,
                                           bool selected_homey_present);
void panel_first_boot_evidence_scheduler_started(uint32_t now_ms);
void panel_first_boot_evidence_scheduler_failed(uint8_t reason);
void panel_first_boot_evidence_scheduler_skipped(uint8_t reason);
void panel_first_boot_evidence_queue_result(int8_t result);
void panel_first_boot_evidence_inventory_started(uint8_t origin,
                                                  uint32_t attempt,
                                                  uint32_t sequence,
                                                  uint32_t now_ms);
void panel_first_boot_evidence_inventory_completed(
    const panel_first_boot_attempt_result_t *result);
void panel_first_boot_evidence_recovery_used(uint8_t origin,
                                             uint32_t attempt);
void panel_first_boot_evidence_ready(uint8_t origin, uint32_t attempt,
                                     uint32_t now_ms);
void panel_first_boot_evidence_copy(panel_first_boot_evidence_t *out);
bool panel_first_boot_evidence_append_json(char *body, size_t capacity,
                                            size_t *length);
