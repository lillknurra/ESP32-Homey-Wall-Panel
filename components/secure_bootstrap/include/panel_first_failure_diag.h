#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Sanitized, fixed-size snapshot of the first failed inventory attempt in a
 * runtime. It contains no Homey identifiers, pointers or socket descriptors. */
typedef struct {
    uint32_t runtime_id, inventory_sequence, attempt;
    uint8_t origin, stage;
    uint64_t started_us, completed_us;
    int32_t final_error, http_status;
    uint32_t request_sequence, request_elapsed_ms;
    int32_t request_error, request_http_status;
    int32_t tls_query, tls_error, socket_errno, connect_result;
    int64_t content_length;
    uint32_t received_bytes;
    uint64_t last_positive_read_us, terminal_read_us;
    int32_t terminal_result, terminal_errno;
    uint32_t internal_free, internal_largest, internal_minimum;
    uint32_t psram_free, psram_largest;
    uint32_t rgb_shortfall_delta, rgb_restart_decision_delta, rgb_restart_path_delta;
    bool valid, request_available, connect_result_available;
    bool device_body_available, headers_complete, body_complete, terminal_available;
    bool memory_available, rgb_available;
    /* P4N9A close events lack request ownership; remain false/unknown. */
    bool close_correlation_complete;
} panel_first_failure_diag_t;

typedef struct {
    uint32_t shortfall, restart_decisions, restart_paths;
} panel_first_failure_rgb_t;

void panel_first_failure_diag_init(uint32_t runtime_id);
void panel_first_failure_diag_capture(const panel_first_failure_diag_t *failure);
void panel_first_failure_diag_copy(panel_first_failure_diag_t *out);
bool panel_first_failure_diag_append_json(char *body, size_t capacity,
                                           size_t *length);
bool panel_first_failure_diag_rgb_snapshot(panel_first_failure_rgb_t *out);
