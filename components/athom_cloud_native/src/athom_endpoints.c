#include "athom_endpoints.h"
#include <string.h>

static const athom_endpoint_definition_t ENDPOINTS[] = {
    {ATHOM_ENDPOINT_AUTHORIZE, "GET", "https://api.athom.com/oauth2/authorise", "-", true, true},
    {ATHOM_ENDPOINT_TOKEN, "POST", "https://api.athom.com/oauth2/token", "-", true, true},
    {ATHOM_ENDPOINT_USER_ME, "GET", "https://api.athom.com/user/me", "-", true, true},
    {ATHOM_ENDPOINT_DELEGATION_TOKEN, "POST", "https://api.athom.com/delegation/token?audience=homey", "-", true, true},
    {ATHOM_ENDPOINT_HOMEY_LOGIN, "POST", "/api/manager/users/login", "-", false, true},
    {ATHOM_ENDPOINT_SYSTEM_INFO, "GET", "/api/manager/system/", "homey.system.readonly", false, true},
    {ATHOM_ENDPOINT_ZONES, "GET", "/api/manager/zones/zone", "homey.zone.readonly", false, true},
    {ATHOM_ENDPOINT_DEVICES, "GET", "/api/manager/devices/device", "homey.device.readonly", false, true},
    {ATHOM_ENDPOINT_FLOWS, "GET", "/api/manager/flow/flow", "homey.flow.readonly", false, true},
    {ATHOM_ENDPOINT_ADVANCED_FLOWS, "GET", "/api/manager/flow/advancedflow", "homey.flow.readonly", false, true},
    {ATHOM_ENDPOINT_MOODS, "GET", "/api/manager/moods/mood", "homey.mood.readonly", false, true},
};

const athom_endpoint_definition_t *athom_endpoint_get(athom_endpoint_id_t id) {
    for (size_t i = 0; i < sizeof(ENDPOINTS) / sizeof(ENDPOINTS[0]); ++i) {
        if (ENDPOINTS[i].id == id) return &ENDPOINTS[i];
    }
    return NULL;
}

bool athom_endpoint_is_allowed(const char *method, const char *path) {
    if (!method || !path) return false;
    for (size_t i = 0; i < sizeof(ENDPOINTS) / sizeof(ENDPOINTS[0]); ++i) {
        if (strcmp(method, ENDPOINTS[i].method) == 0 &&
            strcmp(path, ENDPOINTS[i].path) == 0) return true;
    }
    return false;
}

bool athom_endpoint_is_mutation(const char *method, const char *path) {
    return !athom_endpoint_is_allowed(method, path);
}
