#pragma once
#include "athom_auth.h"
#include "athom_provisioning.h"
#include <stdbool.h>

typedef struct {
    athom_provisioning_t *provisioning;
    bool explicit_live_authorized;
    bool running;
} athom_provisioning_portal_t;

athom_status_t athom_provisioning_portal_start(athom_provisioning_portal_t *portal);
athom_status_t athom_provisioning_portal_stop(athom_provisioning_portal_t *portal);
athom_status_t athom_provisioning_portal_authorize_live(
    athom_provisioning_portal_t *portal, bool authorized);
