#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    ATHOM_RESTORE_POLICY_LOGIN_REQUIRED = 0,
    ATHOM_RESTORE_POLICY_COMPLETE_SELECTED,
    ATHOM_RESTORE_POLICY_FETCH_HOMEYS,
    ATHOM_RESTORE_POLICY_REFRESH_AUTH,
    ATHOM_RESTORE_POLICY_RETRY_TRANSIENT,
    ATHOM_RESTORE_POLICY_SELECTION_REQUIRED,
    ATHOM_RESTORE_POLICY_CONNECTION_ERROR,
} athom_restore_policy_action_t;

athom_restore_policy_action_t athom_restore_policy_after_store(
    int store_error, bool present, bool selected_present);

athom_restore_policy_action_t athom_restore_policy_after_discovery(
    int discovery_error, int http_status, size_t candidate_count,
    bool refreshed, bool transient, bool can_retry);

athom_restore_policy_action_t athom_restore_policy_after_refresh(int refresh_error);

bool athom_restore_policy_should_start_preselection(
    bool wifi_online,
    bool restore_running,
    bool pending,
    bool worker_running);
