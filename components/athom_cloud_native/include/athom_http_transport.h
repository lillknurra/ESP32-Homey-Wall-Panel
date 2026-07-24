#pragma once
#include "athom_cloud_types.h"

typedef struct athom_http_transport athom_http_transport_t;

typedef struct {
    athom_status_t (*exchange_authorization_code)(
        athom_http_transport_t *transport,
        const athom_credentials_t *client,
        const char *authorization_code,
        athom_credentials_t *updated);
    athom_status_t (*refresh_access_token)(
        athom_http_transport_t *transport,
        const athom_credentials_t *current,
        athom_credentials_t *updated);
    athom_status_t (*list_homeys)(
        athom_http_transport_t *transport,
        const char *access_token,
        athom_homey_list_t *out);
    athom_status_t (*read_inventory)(
        athom_http_transport_t *transport,
        const char *access_token,
        const char *homey_id,
        athom_discovery_strategy_t strategy,
        athom_inventory_summary_t *out);
} athom_http_transport_vtable_t;

struct athom_http_transport {
    const athom_http_transport_vtable_t *vtable;
    void *context;
};
