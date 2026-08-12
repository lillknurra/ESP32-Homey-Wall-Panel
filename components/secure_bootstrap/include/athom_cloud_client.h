#pragma once
#include "athom_auth_store.h"
#include "athom_cloud_model.h"
#include "panel_homey_read_snapshot.h"
#include "panel_homey_alias_store.h"
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

panel_homey_alias_store_result_t athom_cloud_alias_activate(const char *selected_homey_id);
void athom_cloud_alias_invalidate(void);

panel_homey_read_result_t athom_cloud_copy_device_snapshot(
    uint64_t now_ms,
    panel_homey_read_snapshot_t *out);

const char *athom_cloud_diagnostic_stage(void);
esp_err_t athom_cloud_diagnostic_error(void);
int athom_cloud_diagnostic_http_status(void);

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
    uint32_t last_request_elapsed_ms;
    athom_transport_class_t last_classification;
    int last_http_status;
    int last_tls_error;
    int last_tls_flags;
} athom_transport_metrics_t;

const char *athom_cloud_transport_class_name(athom_transport_class_t value);
void athom_cloud_transport_metrics_copy(athom_transport_metrics_t *out);
void athom_cloud_transport_reset(void);

#endif
