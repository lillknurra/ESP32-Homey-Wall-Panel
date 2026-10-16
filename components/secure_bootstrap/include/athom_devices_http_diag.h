#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Volatile last-success/first-failure Devices capture. No payload/pointer storage.
 * Note functions are called only by the task that acquired begin(). Readers
 * obtain only the immutable completed record through append_json(). */
typedef struct {
    uint64_t started_us, finished_us, last_positive_body_read_us;
    uint64_t last_read_end_us, terminal_read_us, read_max_us;
    uint64_t parser_max_us, callback_max_us, callback_total_us;
    uint64_t body_bytes, wire_bytes;
    int64_t content_length;
    uint32_t request_sequence, response_blocks, read_calls, connect_calls;
    uint32_t app_bytes, app_capacity;
    uint32_t internal_free_before, internal_largest_before, internal_min_before;
    uint32_t internal_free_after, internal_largest_after, internal_min_after;
    int32_t http_status, perform_error, application_result;
    int32_t terminal_result, terminal_errno, transport_errno_first, transport_errno_last;
    int32_t parser_before_terminal, parser_final, parser_first_positive_error;
    int32_t last_callback_error, connect_result, tls_query, tls_error, tls_flags;
    bool valid, headers_complete, chunked, body_complete, failure_latched;
    bool terminal_seen, terminal_in_body, callback_failed, application_overflow;
    bool client_reused, connection_reuse_known, connection_reused;
} athom_devices_http_diag_t;

bool athom_devices_http_diag_begin(uint32_t sequence, bool client_reused);
bool athom_devices_http_diag_owned(void);
void athom_devices_http_diag_connect(int result);
void athom_devices_http_diag_write(void);
void athom_devices_http_diag_read(int result, int saved_errno,
                                 uint64_t started_us, uint64_t finished_us);
void athom_devices_http_diag_parser(int before, int after, size_t input_bytes,
                                   uint64_t elapsed_us);
void athom_devices_http_diag_transport_errno(int value);
void athom_devices_http_diag_headers_sent(void);
void athom_devices_http_diag_status(int value);
void athom_devices_http_diag_headers(int64_t length, bool chunked);
void athom_devices_http_diag_data(size_t incoming, int callback_result,
                                 uint64_t elapsed_us);
void athom_devices_http_diag_finish(bool acquired, int perform_error,
                                   bool body_complete, size_t app_bytes,
                                   size_t app_capacity, bool overflow,
                                   int tls_query, int tls_error, int tls_flags);
bool athom_devices_http_diag_append_json(char *body, size_t capacity,
                                        size_t *length);
