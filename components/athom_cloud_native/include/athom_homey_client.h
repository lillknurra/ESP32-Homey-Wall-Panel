#pragma once
#include "athom_auth.h"

typedef enum {
    ATHOM_READ_PLATFORM = 0,
    ATHOM_READ_ZONES,
    ATHOM_READ_DEVICES,
    ATHOM_READ_FLOWS,
    ATHOM_READ_ADVANCED_FLOWS,
    ATHOM_READ_MOODS,
} athom_read_operation_t;

athom_status_t athom_homey_list(
    athom_auth_context_t *ctx,
    int64_t now_epoch_s,
    athom_homey_list_t *out);

athom_status_t athom_homey_select(
    athom_auth_context_t *ctx,
    const char *homey_id,
    const athom_homey_list_t *available,
    athom_discovery_strategy_t strategy);

athom_status_t athom_homey_read_inventory(
    athom_auth_context_t *ctx,
    int64_t now_epoch_s,
    athom_inventory_summary_t *out);

bool athom_mutation_allowed(const char *operation_name);
