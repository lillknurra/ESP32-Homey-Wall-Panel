#pragma once
#include "athom_cloud_types.h"
#include "athom_credential_store.h"
#include "athom_http_transport.h"

typedef struct {
    athom_credential_store_t *store;
    athom_http_transport_t *transport;
    char expected_state[ATHOM_MAX_OAUTH_STATE];
    int64_t state_expires_at_epoch_s;
    bool state_pending;
} athom_auth_context_t;

athom_status_t athom_auth_begin(
    athom_auth_context_t *ctx,
    const char *client_id,
    const char *client_secret,
    const char *redirect_uri,
    int64_t now_epoch_s,
    char *state_out,
    size_t state_out_size);

athom_status_t athom_auth_import_code(
    athom_auth_context_t *ctx,
    const char *state,
    const char *authorization_code,
    int64_t now_epoch_s);

athom_status_t athom_auth_ensure_access_token(
    athom_auth_context_t *ctx,
    int64_t now_epoch_s,
    athom_credentials_t *out);

athom_status_t athom_auth_wipe(athom_auth_context_t *ctx);
