#include "athom_homey_client.h"
#include "athom_redaction.h"
#include <stdio.h>
#include <string.h>

athom_status_t athom_homey_list(athom_auth_context_t *ctx, int64_t now_epoch_s, athom_homey_list_t *out) {
    if (!ctx || !out || !ctx->transport || !ctx->transport->vtable) return ATHOM_ERR_ARGUMENT;
    athom_credentials_t credentials = {0};
    athom_status_t status = athom_auth_ensure_access_token(ctx, now_epoch_s, &credentials);
    if (status == ATHOM_OK) status = ctx->transport->vtable->list_homeys(ctx->transport, credentials.access_token, out);
    athom_secure_zero(&credentials, sizeof(credentials));
    return status;
}

athom_status_t athom_homey_select(
    athom_auth_context_t *ctx, const char *homey_id,
    const athom_homey_list_t *available, athom_discovery_strategy_t strategy) {
    if (!ctx || !ctx->store || !ctx->store->vtable || !homey_id || !homey_id[0] || !available)
        return ATHOM_ERR_ARGUMENT;
    if (available->count == 0) return ATHOM_ERR_HOMEY_LIST_EMPTY;
    if (available->count > ATHOM_MAX_HOMEYS) return ATHOM_ERR_VALUE_TOO_LARGE;
    if (strategy != ATHOM_DISCOVERY_CLOUD && strategy != ATHOM_DISCOVERY_REMOTE_FORWARDED)
        return ATHOM_ERR_ARGUMENT;
    size_t match_count = 0;
    for (size_t i = 0; i < available->count; ++i) {
        if (!available->items[i].id[0]) return ATHOM_ERR_RESPONSE;
        if (strcmp(homey_id, available->items[i].id) == 0) ++match_count;
        for (size_t j = 0; j < i; ++j)
            if (strcmp(available->items[j].id, available->items[i].id) == 0)
                return ATHOM_ERR_HOMEY_DUPLICATE;
    }
    if (match_count == 0) return ATHOM_ERR_HOMEY_SELECTION_STALE;
    if (match_count != 1) return ATHOM_ERR_HOMEY_DUPLICATE;

    athom_credentials_t current = {0};
    athom_credentials_t candidate = {0};
    athom_status_t status = ctx->store->vtable->load(ctx->store, &current);
    if (status != ATHOM_OK) goto cleanup;
    candidate = current;
    bool changed = strcmp(current.selected_homey_id, homey_id) != 0 || current.strategy != strategy;
    snprintf(candidate.selected_homey_id, sizeof(candidate.selected_homey_id), "%s", homey_id);
    candidate.strategy = strategy;
    status = ctx->store->vtable->save(ctx->store, &candidate);
    if (status == ATHOM_OK && changed && ctx->transport && ctx->transport->vtable &&
        ctx->transport->vtable->invalidate_session)
        ctx->transport->vtable->invalidate_session(ctx->transport);
cleanup:
    athom_secure_zero(&candidate, sizeof(candidate));
    athom_secure_zero(&current, sizeof(current));
    return status;
}

athom_status_t athom_homey_read_inventory(athom_auth_context_t *ctx, int64_t now_epoch_s, athom_inventory_summary_t *out) {
    if (!ctx || !out) return ATHOM_ERR_ARGUMENT;
    athom_credentials_t credentials = {0};
    athom_status_t status = athom_auth_ensure_access_token(ctx, now_epoch_s, &credentials);
    if (status != ATHOM_OK) return status;
    if (!credentials.selected_homey_id[0]) status = ATHOM_ERR_HOMEY_SELECTION_REQUIRED;
    else status = ctx->transport->vtable->read_inventory(ctx->transport, credentials.access_token,
        credentials.selected_homey_id, credentials.strategy, out);
    athom_secure_zero(&credentials, sizeof(credentials));
    return status;
}

bool athom_mutation_allowed(const char *operation_name) { (void)operation_name; return false; }
