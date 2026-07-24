#pragma once
#include "athom_auth.h"

typedef enum {
    ATHOM_PROVISIONING_IDLE = 0,
    ATHOM_PROVISIONING_WAITING_FOR_CODE,
    ATHOM_PROVISIONING_AUTHORIZED,
    ATHOM_PROVISIONING_HOMEY_SELECTED,
    ATHOM_PROVISIONING_COMPLETE,
    ATHOM_PROVISIONING_FAILED,
} athom_provisioning_state_t;

typedef struct {
    athom_auth_context_t *auth;
    athom_provisioning_state_t state;
} athom_provisioning_t;

athom_status_t athom_provisioning_start(
    athom_provisioning_t *portal,
    const char *client_id,
    const char *client_secret,
    const char *redirect_uri,
    int64_t now_epoch_s,
    char *state_out,
    size_t state_out_size);

athom_status_t athom_provisioning_submit_code(
    athom_provisioning_t *portal,
    const char *state,
    const char *authorization_code,
    int64_t now_epoch_s);

athom_status_t athom_provisioning_select_homey(
    athom_provisioning_t *portal,
    const char *homey_id,
    const athom_homey_list_t *available,
    athom_discovery_strategy_t strategy);

athom_status_t athom_provisioning_wipe(athom_provisioning_t *portal);
