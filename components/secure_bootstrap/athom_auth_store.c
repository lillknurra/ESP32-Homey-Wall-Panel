#include "athom_auth_store.h"
#ifdef ESP_PLATFORM
#include "phone_provisioning.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define AUTH_NS "athom_auth_v2"
#define KEY_ACTIVE "active_slot"
#define KEY_A_META "a_meta"
#define KEY_A_ACCESS "a_access"
#define KEY_A_REFRESH "a_refresh"
#define KEY_A_SESSION "a_session"
#define KEY_B_META "b_meta"
#define KEY_B_ACCESS "b_access"
#define KEY_B_REFRESH "b_refresh"
#define KEY_B_SESSION "b_session"
#define AUTH_MAGIC 0x41555432U
#define AUTH_VERSION 2U

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t crc32;
    uint64_t expires_at_s;
    athom_homey_t selected_homey;
    uint32_t access_len;
    uint32_t refresh_len;
    uint32_t session_len;
    uint32_t zone_count;
    uint32_t device_count;
} auth_meta_t;

static void zero_secure(void *buffer, size_t size)
{
    volatile unsigned char *p = buffer;
    while (size--) *p++ = 0U;
}

static const char *meta_key(uint8_t slot){return slot == 0U ? KEY_A_META : KEY_B_META;}
static const char *access_key(uint8_t slot){return slot == 0U ? KEY_A_ACCESS : KEY_B_ACCESS;}
static const char *refresh_key(uint8_t slot){return slot == 0U ? KEY_A_REFRESH : KEY_B_REFRESH;}
static const char *session_key(uint8_t slot){return slot == 0U ? KEY_A_SESSION : KEY_B_SESSION;}

static bool meta_valid(const auth_meta_t *meta)
{
    if (meta == NULL || meta->magic != AUTH_MAGIC ||
        meta->version != AUTH_VERSION || meta->size != sizeof(*meta) ||
        meta->access_len == 0U || meta->access_len >= ATHOM_TOKEN_MAX ||
        meta->refresh_len == 0U || meta->refresh_len >= ATHOM_TOKEN_MAX ||
        meta->session_len >= ATHOM_TOKEN_MAX) {
        return false;
    }
    auth_meta_t copy = *meta;
    uint32_t expected = copy.crc32;
    copy.crc32 = 0U;
    return expected == phone_prov_crc32(&copy, sizeof(copy));
}

static esp_err_t read_string(
    nvs_handle_t handle,
    const char *key,
    char *out,
    size_t capacity,
    uint32_t expected_len)
{
    size_t length = capacity;
    esp_err_t err = nvs_get_str(handle, key, out, &length);
    if (err != ESP_OK) return err;
    if (length == 0U || length - 1U != expected_len || length > capacity) {
        zero_secure(out, capacity);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t athom_auth_store_load(athom_auth_record_t *record, bool *present)
{
    if (record == NULL || present == NULL) return ESP_ERR_INVALID_ARG;
    *present = false;
    zero_secure(record, sizeof(*record));

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(AUTH_NS, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK || handle == 0) {
        if (handle != 0) nvs_close(handle);
        return err == ESP_OK ? ESP_ERR_INVALID_STATE : err;
    }

    uint8_t slot = 0U;
    err = nvs_get_u8(handle, KEY_ACTIVE, &slot);
    if (err == ESP_ERR_NVS_NOT_FOUND) { nvs_close(handle); return ESP_OK; }
    if (err != ESP_OK || slot > 1U) { nvs_close(handle); return ESP_ERR_INVALID_STATE; }

    auth_meta_t meta = {0};
    size_t meta_size = sizeof(meta);
    err = nvs_get_blob(handle, meta_key(slot), &meta, &meta_size);
    if (err != ESP_OK || meta_size != sizeof(meta) || !meta_valid(&meta)) {
        nvs_close(handle); zero_secure(&meta, sizeof(meta));
        return err == ESP_OK ? ESP_ERR_INVALID_CRC : err;
    }

    err = read_string(handle, access_key(slot), record->tokens.access_token,
                      sizeof(record->tokens.access_token), meta.access_len);
    if (err == ESP_OK) err = read_string(handle, refresh_key(slot), record->tokens.refresh_token,
                                         sizeof(record->tokens.refresh_token), meta.refresh_len);
    if (err == ESP_OK && meta.session_len > 0U) err = read_string(handle, session_key(slot), record->homey_session_token,
                                                                 sizeof(record->homey_session_token), meta.session_len);
    nvs_close(handle);
    if (err != ESP_OK) { zero_secure(record, sizeof(*record)); zero_secure(&meta, sizeof(meta)); return err; }

    record->tokens.expires_in_s = 0U;
    record->expires_at_s = meta.expires_at_s;
    record->selected_homey = meta.selected_homey;
    record->zone_count = meta.zone_count;
    record->device_count = meta.device_count;
    zero_secure(&meta, sizeof(meta));
    *present = true;
    return ESP_OK;
}

esp_err_t athom_auth_store_publish(const athom_auth_record_t *record)
{
    if (record == NULL || record->tokens.access_token[0] == '\0' ||
        record->tokens.refresh_token[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    size_t access_len = strlen(record->tokens.access_token);
    size_t refresh_len = strlen(record->tokens.refresh_token);
    size_t session_len = strlen(record->homey_session_token);
    if (access_len >= ATHOM_TOKEN_MAX || refresh_len >= ATHOM_TOKEN_MAX ||
        session_len >= ATHOM_TOKEN_MAX) {
        return ESP_ERR_INVALID_SIZE;
    }

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(AUTH_NS, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    uint8_t active = 1U;
    if (nvs_get_u8(handle, KEY_ACTIVE, &active) != ESP_OK || active > 1U) {
        active = 1U;
    }
    uint8_t target = active == 0U ? 1U : 0U;

    auth_meta_t meta = {
        .magic = AUTH_MAGIC,
        .version = AUTH_VERSION,
        .size = sizeof(meta),
        .expires_at_s = record->expires_at_s,
        .selected_homey = record->selected_homey,
        .access_len = (uint32_t)access_len,
        .refresh_len = (uint32_t)refresh_len,
        .session_len = (uint32_t)session_len,
        .zone_count = (uint32_t)record->zone_count,
        .device_count = (uint32_t)record->device_count,
    };
    meta.crc32 = 0U;
    meta.crc32 = phone_prov_crc32(&meta, sizeof(meta));

    err = nvs_set_str(handle, access_key(target), record->tokens.access_token);
    if (err == ESP_OK) err = nvs_set_str(handle, refresh_key(target), record->tokens.refresh_token);
    if (err == ESP_OK) {
        if (session_len > 0U) {
            err = nvs_set_str(handle, session_key(target), record->homey_session_token);
        } else {
            err = nvs_erase_key(handle, session_key(target));
            if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
        }
    }
    if (err == ESP_OK) err = nvs_set_blob(handle, meta_key(target), &meta, sizeof(meta));
    if (err == ESP_OK) err = nvs_commit(handle);

    if (err == ESP_OK) {
        auth_meta_t verify;
        size_t verify_size = sizeof(verify);
        err = nvs_get_blob(handle, meta_key(target), &verify, &verify_size);
        if (err == ESP_OK &&
            (verify_size != sizeof(verify) || !meta_valid(&verify))) {
            err = ESP_ERR_INVALID_CRC;
        }
        zero_secure(&verify, sizeof(verify));
    }

    if (err == ESP_OK) err = nvs_set_u8(handle, KEY_ACTIVE, target);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    zero_secure(&meta, sizeof(meta));
    return err;
}

esp_err_t athom_auth_store_wipe(void)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(AUTH_NS, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK) return err;
    err = nvs_erase_all(handle);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}
#endif
