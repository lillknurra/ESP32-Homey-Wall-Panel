#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
} athom_pre_tls_diagnostic_t;

bool athom_pre_tls_diag_begin(bool cloud, uint32_t request_sequence);
void athom_pre_tls_diag_finish(bool acquired, athom_pre_tls_diagnostic_t *out);
bool athom_pre_tls_diag_json(const athom_pre_tls_diagnostic_t *d,
                            char *out, size_t capacity);
