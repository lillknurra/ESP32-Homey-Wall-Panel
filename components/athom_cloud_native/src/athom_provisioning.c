#include "athom_provisioning.h"
#include "athom_homey_client.h"

athom_status_t athom_provisioning_start(
    athom_provisioning_t *portal,
    const char *client_id,
    const char *client_secret,
    const char *redirect_uri,
    int64_t now_epoch_s,
    char *state_out,
    size_t state_out_size) {
    if (!portal || !portal->auth) return ATHOM_ERR_ARGUMENT;
    athom_status_t status = athom_auth_begin(
        portal->auth, client_id, client_secret, redirect_uri,
        now_epoch_s, state_out, state_out_size);
    portal->state = status == ATHOM_OK
        ? ATHOM_PROVISIONING_WAITING_FOR_CODE
        : ATHOM_PROVISIONING_FAILED;
    return status;
}

athom_status_t athom_provisioning_submit_code(
    athom_provisioning_t *portal,
    const char *state,
    const char *authorization_code,
    int64_t now_epoch_s) {
    if (!portal || portal->state != ATHOM_PROVISIONING_WAITING_FOR_CODE)
        return ATHOM_ERR_ARGUMENT;
    athom_status_t status = athom_auth_import_code(
        portal->auth, state, authorization_code, now_epoch_s);
    portal->state = status == ATHOM_OK
        ? ATHOM_PROVISIONING_AUTHORIZED
        : ATHOM_PROVISIONING_FAILED;
    return status;
}

athom_status_t athom_provisioning_select_homey(
    athom_provisioning_t *portal,
    const char *homey_id,
    const athom_homey_list_t *available,
    athom_discovery_strategy_t strategy) {
    if (!portal || portal->state != ATHOM_PROVISIONING_AUTHORIZED)
        return ATHOM_ERR_ARGUMENT;
    athom_status_t status = athom_homey_select(
        portal->auth, homey_id, available, strategy);
    portal->state = status == ATHOM_OK
        ? ATHOM_PROVISIONING_COMPLETE
        : ATHOM_PROVISIONING_FAILED;
    return status;
}

athom_status_t athom_provisioning_wipe(athom_provisioning_t *portal) {
    if (!portal || !portal->auth) return ATHOM_ERR_ARGUMENT;
    athom_status_t status = athom_auth_wipe(portal->auth);
    portal->state = status == ATHOM_OK
        ? ATHOM_PROVISIONING_IDLE
        : ATHOM_PROVISIONING_FAILED;
    return status;
}
