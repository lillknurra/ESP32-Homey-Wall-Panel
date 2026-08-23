#include "athom_restore_policy.h"

athom_restore_policy_action_t athom_restore_policy_after_store(
    int store_error, bool present, bool selected_present)
{
    if (store_error != 0 || !present) return ATHOM_RESTORE_POLICY_LOGIN_REQUIRED;
    if (selected_present) return ATHOM_RESTORE_POLICY_COMPLETE_SELECTED;
    return ATHOM_RESTORE_POLICY_FETCH_HOMEYS;
}

athom_restore_policy_action_t athom_restore_policy_after_discovery(
    int discovery_error, int http_status, size_t candidate_count,
    bool refreshed, bool transient, bool can_retry)
{
    (void)candidate_count;
    if (discovery_error == 0) return ATHOM_RESTORE_POLICY_SELECTION_REQUIRED;
    if (http_status == 401 || http_status == 403) {
        return refreshed ? ATHOM_RESTORE_POLICY_LOGIN_REQUIRED
                         : ATHOM_RESTORE_POLICY_REFRESH_AUTH;
    }
    if (transient && can_retry) return ATHOM_RESTORE_POLICY_RETRY_TRANSIENT;
    return ATHOM_RESTORE_POLICY_CONNECTION_ERROR;
}

athom_restore_policy_action_t athom_restore_policy_after_refresh(int refresh_error)
{
    return refresh_error == 0 ? ATHOM_RESTORE_POLICY_FETCH_HOMEYS
                              : ATHOM_RESTORE_POLICY_LOGIN_REQUIRED;
}

bool athom_restore_policy_should_start_preselection(
    bool wifi_online,
    bool restore_running,
    bool pending,
    bool worker_running)
{
    return wifi_online && !restore_running && pending && !worker_running;
}
