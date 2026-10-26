/* Patch116I9: offline policy for an HTTPS endpoint built from a resolved IPv4.
 * This header does not resolve names or perform DNS, TLS, HTTP, or OTA I/O.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#define PANEL_UPDATE_I9_HOSTNAME "lillknurra.github.io"
#define PANEL_UPDATE_I9_URL_CAPACITY 192U
#define PANEL_UPDATE_I9_HOST_CAPACITY 64U

typedef struct {
    char url[PANEL_UPDATE_I9_URL_CAPACITY];
    char tls_common_name[PANEL_UPDATE_I9_HOST_CAPACITY];
    char http_host_header[PANEL_UPDATE_I9_HOST_CAPACITY];
    char manifest_path[PANEL_UPDATE_I9_URL_CAPACITY];
    /* These are required settings for a future adapter, not an adapter or
     * proof that ESP-IDF enforces them. The caller must apply and verify them. */
    bool disable_auto_redirect;
    bool skip_cert_common_name_check;
    bool require_certificate_bundle;
    bool require_async_http_mode;
} panel_update_i9_endpoint_t;

/* Accepts only canonical dotted-decimal, public-unicast IPv4 and exact
 * "stable"/"beta" channel names. On failure, a sufficiently sized output is
 * zeroed. out_size must be at least sizeof(*out). No allocation or I/O occurs.
 */
bool panel_update_i9_build_resolved_ipv4_endpoint(
    const char *channel,
    const char *resolved_ipv4,
    panel_update_i9_endpoint_t *out,
    size_t out_size);
