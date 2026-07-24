#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    ATHOM_ENDPOINT_AUTHORIZE = 0,
    ATHOM_ENDPOINT_TOKEN,
    ATHOM_ENDPOINT_USER_ME,
    ATHOM_ENDPOINT_DELEGATION_TOKEN,
    ATHOM_ENDPOINT_HOMEY_LOGIN,
    ATHOM_ENDPOINT_SYSTEM_INFO,
    ATHOM_ENDPOINT_ZONES,
    ATHOM_ENDPOINT_DEVICES,
    ATHOM_ENDPOINT_FLOWS,
    ATHOM_ENDPOINT_ADVANCED_FLOWS,
    ATHOM_ENDPOINT_MOODS,
} athom_endpoint_id_t;

typedef struct {
    athom_endpoint_id_t id;
    const char *method;
    const char *path;
    const char *scope;
    bool cloud_api;
    bool read_only;
} athom_endpoint_definition_t;

const athom_endpoint_definition_t *athom_endpoint_get(athom_endpoint_id_t id);
bool athom_endpoint_is_allowed(const char *method, const char *path);
bool athom_endpoint_is_mutation(const char *method, const char *path);
