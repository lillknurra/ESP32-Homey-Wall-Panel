#pragma once
#include "athom_cloud_types.h"

typedef struct athom_credential_store athom_credential_store_t;

typedef struct {
    athom_status_t (*load)(athom_credential_store_t *store, athom_credentials_t *out);
    athom_status_t (*save)(athom_credential_store_t *store, const athom_credentials_t *value);
    athom_status_t (*wipe)(athom_credential_store_t *store);
} athom_credential_store_vtable_t;

struct athom_credential_store {
    const athom_credential_store_vtable_t *vtable;
    void *context;
};

athom_status_t athom_nvs_store_init(athom_credential_store_t *store);
