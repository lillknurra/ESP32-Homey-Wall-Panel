#include "athom_credential_store.h"
#ifdef ESP_PLATFORM
#include "nvs.h"
#include "nvs_flash.h"
#include <stdlib.h>

typedef struct {
    const char *namespace_name;
} nvs_store_context_t;

static athom_status_t nvs_load(athom_credential_store_t *store, athom_credentials_t *out) {
    nvs_store_context_t *ctx = store->context;
    nvs_handle_t handle;
    if (nvs_open(ctx->namespace_name, NVS_READONLY, &handle) != ESP_OK) return ATHOM_ERR_STORAGE;
    size_t size = sizeof(*out);
    esp_err_t err = nvs_get_blob(handle, "credentials", out, &size);
    nvs_close(handle);
    return err == ESP_OK && size == sizeof(*out) ? ATHOM_OK : ATHOM_ERR_STORAGE;
}

static athom_status_t nvs_save(athom_credential_store_t *store, const athom_credentials_t *value) {
    nvs_store_context_t *ctx = store->context;
    nvs_handle_t handle;
    if (nvs_open(ctx->namespace_name, NVS_READWRITE, &handle) != ESP_OK) return ATHOM_ERR_STORAGE;
    esp_err_t err = nvs_set_blob(handle, "credentials", value, sizeof(*value));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK ? ATHOM_OK : ATHOM_ERR_STORAGE;
}

static athom_status_t nvs_wipe(athom_credential_store_t *store) {
    nvs_store_context_t *ctx = store->context;
    nvs_handle_t handle;
    if (nvs_open(ctx->namespace_name, NVS_READWRITE, &handle) != ESP_OK) return ATHOM_ERR_STORAGE;
    esp_err_t err = nvs_erase_key(handle, "credentials");
    if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK ? ATHOM_OK : ATHOM_ERR_STORAGE;
}

static const athom_credential_store_vtable_t VTABLE = {
    .load = nvs_load, .save = nvs_save, .wipe = nvs_wipe
};

athom_status_t athom_nvs_store_init(athom_credential_store_t *store) {
    if (!store) return ATHOM_ERR_ARGUMENT;
    static nvs_store_context_t context = { .namespace_name = "athom_auth" };
    store->vtable = &VTABLE;
    store->context = &context;
    return ATHOM_OK;
}
#else
athom_status_t athom_nvs_store_init(athom_credential_store_t *store) {
    (void)store;
    return ATHOM_ERR_UNSUPPORTED;
}
#endif
