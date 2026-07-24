#include "athom_homey_client.h"
#include "athom_redaction.h"
#include <stdio.h>
#include <string.h>

athom_status_t athom_homey_list(
    athom_auth_context_t *ctx,
    int64_t now_epoch_s,
    athom_homey_list_t *out) {
    if (!ctx || !out) return ATHOM_ERR_ARGUMENT;
    athom_credentials_t credentials = {0};
    athom_status_t status = athom_auth_ensure_access_token(ctx, now_epoch_s, &credentials);
    if (status != ATHOM_OK) return status;
    status = ctx->transport->vtable->list_homeys(ctx->transport, credentials.access_token, out);
    athom_secure_zero(&credentials, sizeof(credentials));
    return status;
}

athom_status_t athom_homey_select(
    athom_auth_context_t *ctx,
    const char *homey_id,
    const athom_homey_list_t *available,
    athom_discovery_strategy_t strategy) {
    if (!ctx || !homey_id || !available) return ATHOM_ERR_ARGUMENT;
    bool found = false;
    for (size_t i = 0; i < available->count; ++i) {
        if (strcmp(homey_id, available->items[i].id) == 0) {
            found = true;
            break;
        }
    }
    if (!found) return ATHOM_ERR_HOMEY_NOT_FOUND;

    athom_credentials_t credentials = {0};
    athom_status_t status = ctx->store->vtable->load(ctx->store, &credentials);
    if (status != ATHOM_OK) return status;
    snprintf(credentials.selected_homey_id, sizeof(credentials.selected_homey_id), "%s", homey_id);
    credentials.strategy = strategy;
    status = ctx->store->vtable->save(ctx->store, &credentials);
    athom_secure_zero(&credentials, sizeof(credentials));
    return status;
}

athom_status_t athom_homey_read_inventory(
    athom_auth_context_t *ctx,
    int64_t now_epoch_s,
    athom_inventory_summary_t *out) {
    if (!ctx || !out) return ATHOM_ERR_ARGUMENT;
    athom_credentials_t credentials = {0};
    athom_status_t status = athom_auth_ensure_access_token(ctx, now_epoch_s, &credentials);
    if (status != ATHOM_OK) return status;
    if (credentials.selected_homey_id[0] == '\0') {
        athom_secure_zero(&credentials, sizeof(credentials));
        return ATHOM_ERR_HOMEY_NOT_FOUND;
    }
    status = ctx->transport->vtable->read_inventory(
        ctx->transport,
        credentials.access_token,
        credentials.selected_homey_id,
        credentials.strategy,
        out);
    athom_secure_zero(&credentials, sizeof(credentials));
    return status;
}

bool athom_mutation_allowed(const char *operation_name) {
    (void)operation_name;
    return false;
}
