#include "athom_auth.h"
#include "athom_redaction.h"
#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_random.h"
#else
#include <stdlib.h>
#endif

static void random_hex(char *out, size_t out_size) {
    static const char hex[] = "0123456789abcdef";
    if (out_size < 3) return;
    size_t bytes = (out_size - 1) / 2;
    for (size_t i = 0; i < bytes; ++i) {
#ifdef ESP_PLATFORM
        unsigned value = esp_random();
#else
        unsigned value = (unsigned)rand();
#endif
        out[i * 2] = hex[(value >> 4) & 0xF];
        out[i * 2 + 1] = hex[value & 0xF];
    }
    out[bytes * 2] = '\0';
}

athom_status_t athom_auth_begin(
    athom_auth_context_t *ctx,
    const char *client_id,
    const char *client_secret,
    const char *redirect_uri,
    int64_t now_epoch_s,
    char *state_out,
    size_t state_out_size) {
    if (!ctx || !ctx->store || !ctx->transport || !client_id || !client_secret ||
        !redirect_uri || !state_out || state_out_size < 33) return ATHOM_ERR_ARGUMENT;

    athom_credentials_t credentials = {0};
    snprintf(credentials.client_id, sizeof(credentials.client_id), "%s", client_id);
    snprintf(credentials.client_secret, sizeof(credentials.client_secret), "%s", client_secret);
    snprintf(credentials.redirect_uri, sizeof(credentials.redirect_uri), "%s", redirect_uri);
    credentials.strategy = ATHOM_DISCOVERY_CLOUD;
    credentials.provisioned = false;

    athom_status_t status = ctx->store->vtable->save(ctx->store, &credentials);
    athom_secure_zero(&credentials, sizeof(credentials));
    if (status != ATHOM_OK) return status;

    random_hex(ctx->expected_state, sizeof(ctx->expected_state));
    snprintf(state_out, state_out_size, "%s", ctx->expected_state);
    ctx->state_expires_at_epoch_s = now_epoch_s + 600;
    ctx->state_pending = true;
    return ATHOM_OK;
}

athom_status_t athom_auth_import_code(
    athom_auth_context_t *ctx,
    const char *state,
    const char *authorization_code,
    int64_t now_epoch_s) {
    if (!ctx || !state || !authorization_code || authorization_code[0] == '\0')
        return ATHOM_ERR_ARGUMENT;
    if (!ctx->state_pending || strcmp(state, ctx->expected_state) != 0)
        return ATHOM_ERR_STATE_MISMATCH;
    if (now_epoch_s > ctx->state_expires_at_epoch_s)
        return ATHOM_ERR_STATE_EXPIRED;

    athom_credentials_t current = {0};
    athom_status_t status = ctx->store->vtable->load(ctx->store, &current);
    if (status != ATHOM_OK) return status;

    athom_credentials_t updated = current;
    status = ctx->transport->vtable->exchange_authorization_code(
        ctx->transport, &current, authorization_code, &updated);
    athom_secure_zero(&current, sizeof(current));
    if (status != ATHOM_OK) {
        athom_secure_zero(&updated, sizeof(updated));
        return status;
    }
    updated.provisioned = true;
    status = ctx->store->vtable->save(ctx->store, &updated);
    athom_secure_zero(&updated, sizeof(updated));
    athom_secure_zero(ctx->expected_state, sizeof(ctx->expected_state));
    ctx->state_pending = false;
    return status;
}

athom_status_t athom_auth_ensure_access_token(
    athom_auth_context_t *ctx,
    int64_t now_epoch_s,
    athom_credentials_t *out) {
    if (!ctx || !out) return ATHOM_ERR_ARGUMENT;
    athom_credentials_t current = {0};
    athom_status_t status = ctx->store->vtable->load(ctx->store, &current);
    if (status != ATHOM_OK) return status;
    if (!current.provisioned) {
        athom_secure_zero(&current, sizeof(current));
        return ATHOM_ERR_NOT_PROVISIONED;
    }
    if (current.expires_at_epoch_s > now_epoch_s + 60) {
        *out = current;
        athom_secure_zero(&current, sizeof(current));
        return ATHOM_OK;
    }
    athom_credentials_t refreshed = current;
    status = ctx->transport->vtable->refresh_access_token(ctx->transport, &current, &refreshed);
    athom_secure_zero(&current, sizeof(current));
    if (status != ATHOM_OK) {
        athom_secure_zero(&refreshed, sizeof(refreshed));
        return ATHOM_ERR_REFRESH_FAILED;
    }
    status = ctx->store->vtable->save(ctx->store, &refreshed);
    if (status == ATHOM_OK) *out = refreshed;
    athom_secure_zero(&refreshed, sizeof(refreshed));
    return status;
}

athom_status_t athom_auth_wipe(athom_auth_context_t *ctx) {
    if (!ctx || !ctx->store) return ATHOM_ERR_ARGUMENT;
    athom_secure_zero(ctx->expected_state, sizeof(ctx->expected_state));
    ctx->state_pending = false;
    ctx->state_expires_at_epoch_s = 0;
    return ctx->store->vtable->wipe(ctx->store);
}
