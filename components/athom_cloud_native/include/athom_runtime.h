#pragma once
#include "athom_cloud_types.h"
#include <stdbool.h>

typedef struct {
    bool wifi_started;
    bool wifi_connected;
    bool got_ip;
    bool provisioning_portal_started;
    bool live_gate_enabled;
} athom_runtime_status_t;

athom_status_t athom_runtime_start(athom_runtime_status_t *status);
athom_status_t athom_runtime_set_live_gate(athom_runtime_status_t *status, bool enabled);
