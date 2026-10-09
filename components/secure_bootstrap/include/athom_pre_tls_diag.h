#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Calls are outer mbedtls_ssl_handshake invocations, NOT internal steps.
 * Bytes are accepted/returned by BIO, not proof of server receipt/record parse.
 * Timings are relative to the first handshake invocation of this connection.
 * errno is the value observed immediately after BIO; it may be stale when the
 * underlying callback performs no syscall. Ret remains the primary signal. */
typedef struct {
    bool valid, completed, tx_observed, rx_observed, peer_closed;
    uint32_t call_count, want_read_count, want_write_count;
    uint32_t send_calls, send_bytes, recv_calls, recv_bytes;
    int last_ret, send_last_ret, recv_last_ret;
    int send_last_errno, recv_last_errno;
    uint32_t first_tx_ms, first_rx_ms, last_progress_ms;
    unsigned state; /* bounded project classification, never a pointer */
} athom_tls_handshake_diagnostic_t;

/* Volatile evidence only. No address, descriptor, hostname or private material. */
typedef struct {
    bool valid;
    uint32_t request_sequence;
    bool readiness_observed, default_route_present, netif_up, ip_assigned;
    bool dns_server_configured, wifi_associated;
    bool connection_called, connection_completed;
    uint32_t connection_count;
    bool dns_started, dns_ok;
    uint32_t dns_result_count;
    int dns_code;
    unsigned address_family; /* 4, 6 or 0 (other/not observed) */
    bool socket_attempted, socket_created;
    bool connect_started, connect_pending, tcp_connected;
    bool wait_observed, wait_timeout;
    bool so_error_observed, tls_setup_started;
    int socket_error, connect_error, wait_error, so_error, so_query_error;
    int connection_result;
    uint32_t connect_elapsed_ms;
    athom_tls_handshake_diagnostic_t handshake;
} athom_pre_tls_diagnostic_t;

bool athom_pre_tls_diag_begin(bool cloud, uint32_t request_sequence);
void athom_pre_tls_diag_finish(bool acquired, athom_pre_tls_diagnostic_t *out);
bool athom_pre_tls_diag_json(const athom_pre_tls_diagnostic_t *d,
                            char *out, size_t capacity);
