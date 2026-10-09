#pragma once
#include "athom_auth_store.h"
#include "athom_pre_tls_diag.h"
#include "athom_cloud_model.h"
#include "panel_homey_read_snapshot.h"
#include "panel_homey_alias_store.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    ATHOM_HOMEY_LIGHT_WRITE_ACCEPTED = 0,
    ATHOM_HOMEY_LIGHT_WRITE_INVALID_ARGUMENT,
    ATHOM_HOMEY_LIGHT_WRITE_NOT_READY,
    ATHOM_HOMEY_LIGHT_WRITE_TARGET_NOT_FOUND,
    ATHOM_HOMEY_LIGHT_WRITE_TARGET_INVALID,
    ATHOM_HOMEY_LIGHT_WRITE_UNAUTHORIZED,
    ATHOM_HOMEY_LIGHT_WRITE_REJECTED,
    ATHOM_HOMEY_LIGHT_WRITE_TRANSPORT_AMBIGUOUS,
    ATHOM_HOMEY_LIGHT_WRITE_INTERNAL_ERROR,
} athom_homey_light_write_result_t;

typedef struct {
    bool attempted;
    panel_homey_alias_store_result_t result;
} athom_cloud_alias_activation_status_t;

#ifdef ESP_PLATFORM
#include "esp_err.h"

typedef struct {
    athom_token_set_t tokens;
    athom_homey_list_t homeys;
    athom_homey_t selected_homey;
    char homey_session_token[ATHOM_TOKEN_MAX];
    uint64_t expires_at_s;
    size_t zone_count;
    size_t device_count;
} athom_cloud_state_t;

esp_err_t athom_cloud_exchange_code(
    const char *authorization_code,
    athom_cloud_state_t *state);

esp_err_t athom_cloud_refresh(athom_cloud_state_t *state);

esp_err_t athom_cloud_fetch_user_homeys(athom_cloud_state_t *state);

esp_err_t athom_cloud_select_and_connect(
    athom_cloud_state_t *state,
    const char *homey_id);

esp_err_t athom_cloud_fetch_inventory(athom_cloud_state_t *state);

/* Read-only inventory through the exact previously selected remote session.
 * Fails closed if the in-memory selection, cached discovery entry, session or
 * selected-Homey alias binding do not agree. */
esp_err_t athom_cloud_fetch_inventory_from_cached_session(
    athom_cloud_state_t *state,
    const char *expected_homey_id);

/*
 * Fixed, bounded Homey light mutation primitive for Favorites widgets 4/5.
 * The caller supplies only the sanitized widget index and desired boolean.
 * Raw device identifiers remain inside the private alias runtime.
 */
athom_homey_light_write_result_t athom_cloud_set_favorite_light_onoff(
    athom_cloud_state_t *state,
    size_t widget_index,
    bool value);

panel_homey_alias_store_result_t athom_cloud_alias_activate(const char *selected_homey_id);
void athom_cloud_alias_invalidate(void);

panel_homey_read_result_t athom_cloud_copy_device_snapshot(
    uint64_t now_ms,
    panel_homey_read_snapshot_t *out);

panel_homey_read_result_t athom_cloud_inspect_device_snapshot(
    uint64_t now_ms,
    panel_homey_snapshot_inspection_t *out);
panel_homey_read_result_t athom_cloud_inspect_device_snapshot_with_publish(
    uint64_t now_ms,
    panel_homey_snapshot_inspection_t *snapshot_out,
    panel_homey_snapshot_publish_inspection_t *out);
athom_cloud_alias_activation_status_t athom_cloud_alias_activation_status(void);

const char *athom_cloud_diagnostic_stage(void);
esp_err_t athom_cloud_diagnostic_error(void);
int athom_cloud_diagnostic_http_status(void);
uint32_t athom_cloud_diagnostic_revision(void);

typedef enum {
    ATHOM_TRANSPORT_OK = 0,
    ATHOM_TRANSPORT_DNS_FAIL,
    ATHOM_TRANSPORT_TCP_CONNECT_FAIL,
    ATHOM_TRANSPORT_TLS_FAIL,
    ATHOM_TRANSPORT_HTTP_TIMEOUT,
    ATHOM_TRANSPORT_HTTP_401,
    ATHOM_TRANSPORT_HTTP_403,
    ATHOM_TRANSPORT_HTTP_408,
    ATHOM_TRANSPORT_HTTP_429,
    ATHOM_TRANSPORT_HTTP_5XX,
    ATHOM_TRANSPORT_HOMEY_SESSION_FAIL,
    ATHOM_TRANSPORT_FAVORITES_FAIL,
    ATHOM_TRANSPORT_ZONES_FAIL,
    ATHOM_TRANSPORT_DEVICES_FAIL,
    ATHOM_TRANSPORT_PARSE_FAIL,
    ATHOM_TRANSPORT_NO_VALID_ENDPOINT,
} athom_transport_class_t;

typedef enum {
    ATHOM_TRANSPORT_ROLE_NONE = 0,
    ATHOM_TRANSPORT_ROLE_CLOUD,
    ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE,
} athom_transport_role_t;

typedef struct {
    bool capture_attempted;
    bool hook_registered;
    uint32_t matching_failure_count;
    uint32_t first_requested_size;
    uint32_t last_requested_size;
    uint32_t max_requested_size;
    uint32_t failure_caps;
    bool all_heap_caps_calloc;
    uint32_t internal_8bit_free_before;
    uint32_t internal_8bit_largest_before;
    uint32_t internal_8bit_minimum_before;
    uint32_t internal_8bit_free_at_failure;
    uint32_t internal_8bit_largest_at_failure;
    uint32_t internal_8bit_minimum_at_failure;
    uint32_t internal_8bit_free_after;
    uint32_t internal_8bit_largest_after;
    uint32_t internal_8bit_minimum_after;
} athom_tls_memory_diagnostic_t;

/* PATCH069_FAVORITES_DIAGNOSTIC_BEGIN */
/* Volatile, sanitized result of the user/me GET, captured before inventory
 * requests replace the shared last-transport metrics. No identifiers. */
typedef struct {
    bool attempted;
    bool transport_observed;
    bool client_reused;
    bool data_verified;
    int32_t error;
    int32_t http_status;
    int32_t perform_error;
    int32_t tls_error;
    int32_t socket_errno;
    uint32_t elapsed_ms;
    bool response_received;
    bool body_complete;
    bool connected_event_seen;
    bool error_event_seen;
    bool disconnected_event_seen;
} athom_favorites_read_diagnostic_t;
/* PATCH069_FAVORITES_DIAGNOSTIC_END */

typedef struct {
    uint32_t cloud_client_init_count;
    uint32_t cloud_client_reuse_count;
    uint32_t cloud_client_cleanup_count;
    uint32_t homey_client_init_count;
    uint32_t homey_client_reuse_count;
    uint32_t homey_client_cleanup_count;
    uint32_t cloud_request_count;
    uint32_t homey_request_count;
    uint32_t homey_session_create_count;
    uint32_t remote_rebind_count;
    uint32_t perform_count;
    uint32_t inventory_read_count;
    bool inventory_snapshot_published;
    athom_favorites_read_diagnostic_t favorites_read;
    bool last_body_complete;
    uint32_t last_request_elapsed_ms;
    athom_transport_class_t last_classification;
    athom_transport_role_t last_perform_role;
    athom_transport_class_t last_perform_classification;
    int last_perform_http_status;
    bool last_connected_event_seen;
    bool last_error_event_seen;
    bool last_disconnected_event_seen;
    int last_http_status;
    int last_tls_error;
    int last_tls_flags;
    int last_socket_errno;
    esp_err_t last_perform_err;
    esp_err_t last_tls_query;
    athom_tls_memory_diagnostic_t last_tls_memory_diagnostic;
    athom_pre_tls_diagnostic_t last_pre_tls_diagnostic;
} athom_transport_metrics_t;

typedef struct {
    bool executed;
    esp_err_t perform_err;
    int fresh_http_status;
    bool transport_response_received;
    athom_transport_class_t classification;
    int tls_error;
    int socket_errno;
    uint32_t elapsed_ms;
    uint32_t cloud_client_init_count;
    uint32_t cloud_client_reuse_count;
    uint32_t cloud_client_cleanup_count;
} athom_cloud_debug_probe_result_t;

const char *athom_cloud_transport_class_name(athom_transport_class_t value);
void athom_cloud_transport_metrics_copy(athom_transport_metrics_t *out);
void athom_cloud_transport_reset(void);

esp_err_t athom_cloud_debug_probe_user_me(
    const athom_cloud_state_t *state,
    athom_cloud_debug_probe_result_t *out);

#endif
