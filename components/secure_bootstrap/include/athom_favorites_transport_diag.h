#pragma once
#include <stdbool.h>
#include <stdint.h>

/* One existing Favorites request. Fixed scalars only: no content or handles. */
typedef struct {
    bool valid;
    bool client_reused;
    bool connection_reuse_known;
    bool connection_reused; /* First request write, not TLS session resumption. */
    bool request_headers_sent;
    bool complete_header_observed;
    bool timeout_observed;
    bool fin_reported;
    bool close_called;
    uint32_t request_header_blocks;
    uint32_t connect_calls;
    int32_t connect_result;
    uint32_t write_calls;
    int32_t last_write_result;
    uint32_t request_bytes_written;
    uint32_t header_read_calls;
    int32_t last_header_read_result;
    int32_t last_header_read_errno;
    uint32_t response_bytes_observed; /* Decrypted bytes before header completion. */
    uint32_t parser_calls;
    int32_t parser_error;
    uint32_t header_wait_elapsed_ms;
    uint32_t transport_errno_calls;
    int32_t transport_errno; /* First nonzero value from existing destructive getter. */
    int32_t close_result;
    int32_t request_result;
} athom_favorites_fetch_diagnostic_t;

typedef enum {
    ATHOM_FAVORITES_REQUEST_HEADERS_SENT,
    ATHOM_FAVORITES_RESPONSE_HEADERS_COMPLETE,
} athom_favorites_transport_event_t;

bool athom_favorites_transport_diag_begin(bool client_reused);
void athom_favorites_transport_diag_client_reused(void);
void athom_favorites_transport_diag_event(athom_favorites_transport_event_t event);
void athom_favorites_transport_diag_close_result(int32_t result);
void athom_favorites_transport_diag_finish(
    bool acquired, int32_t result, athom_favorites_fetch_diagnostic_t *out);
const char *athom_favorites_transport_diag_class(
    const athom_favorites_fetch_diagnostic_t *diagnostic);
