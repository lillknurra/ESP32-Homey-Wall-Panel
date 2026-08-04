#include "panel_homey_alias_store.h"
#include "panel_homey_dashboard_binding.h"
#include <string.h>
#ifdef ESP_PLATFORM
#include "psa/crypto.h"
#else
#include <CommonCrypto/CommonDigest.h>
#endif

#define MAGIC 0x48414c31U
#define RECORD_SIZE 1232U
#define HDR 52U
#define ENTRY_SIZE 196U
#define DIGEST_OFFSET 16U
#define DIGEST_SIZE 32U
#define ENTRY_COUNT_OFFSET 48U
#define RESERVED_0_OFFSET 49U
#define RESERVED_1_OFFSET 50U
#define RESERVED_2_OFFSET 51U

static uint32_t crc32(const uint8_t *p, size_t n)
{
    uint32_t c = 0xffffffffU;
    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (unsigned b = 0; b < 8; b++) {
            c = (c >> 1) ^
                (0xedb88320U & ((uint32_t)-(int32_t)(c & 1U)));
        }
    }
    return c ^ 0xffffffffU;
}

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++) {
        p[i] = (uint8_t)(v >> (8 * i));
    }
}

static uint16_t get16(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static bool text_ok(const char *s, size_t cap)
{
    size_t n = strnlen(s, cap);
    if (n == 0 || n >= cap) {
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        if ((unsigned char)s[i] < 0x20U) {
            return false;
        }
    }
    return true;
}

static bool record_ok(const panel_homey_alias_record_t *r)
{
    if (!r || r->generation == 0 ||
        r->entry_count > PANEL_HOMEY_ALIAS_STORE_MAX_ENTRIES) {
        return false;
    }

    bool used[6] = {0};
    for (size_t i = 0; i < r->entry_count; i++) {
        const panel_homey_alias_entry_t *e = &r->entries[i];
        if (e->dashboard_binding_index >= 6 ||
            used[e->dashboard_binding_index] ||
            !text_ok(e->raw_device_id, sizeof(e->raw_device_id)) ||
            !text_ok(e->raw_capability_id, sizeof(e->raw_capability_id))) {
            return false;
        }
        used[e->dashboard_binding_index] = true;
        for (size_t j = 0; j < i; j++) {
            if (strcmp(e->raw_device_id,
                       r->entries[j].raw_device_id) == 0 &&
                strcmp(e->raw_capability_id,
                       r->entries[j].raw_capability_id) == 0) {
                return false;
            }
        }
    }
    return true;
}

bool panel_homey_alias_record_encode(
    const panel_homey_alias_record_t *r,
    uint8_t *out,
    size_t n)
{
    if (n != RECORD_SIZE || !record_ok(r)) {
        return false;
    }

    memset(out, 0, n);
    put32(out, MAGIC);
    put16(out + 4U, PANEL_HOMEY_ALIAS_STORE_SCHEMA_VERSION);
    put16(out + 6U, RECORD_SIZE);
    put32(out + 8U, r->generation);
    memcpy(out + DIGEST_OFFSET,
           r->homey_identity_digest,
           DIGEST_SIZE);
    out[ENTRY_COUNT_OFFSET] = (uint8_t)r->entry_count;
    out[RESERVED_0_OFFSET] = 0U;
    out[RESERVED_1_OFFSET] = 0U;
    out[RESERVED_2_OFFSET] = 0U;

    size_t offset = HDR;
    for (size_t i = 0; i < r->entry_count; i++) {
        const panel_homey_alias_entry_t *e = &r->entries[i];
        out[offset] = e->dashboard_binding_index;
        memcpy(out + offset + 4U, e->raw_device_id, 128U);
        memcpy(out + offset + 132U, e->raw_capability_id, 64U);
        offset += ENTRY_SIZE;
    }

    put32(out + 12U, 0U);
    put32(out + 12U, crc32(out, n));
    return true;
}

bool panel_homey_alias_record_decode(
    const uint8_t *in,
    size_t n,
    panel_homey_alias_record_t *r)
{
    if (!in || !r || n != RECORD_SIZE ||
        get32(in) != MAGIC ||
        get16(in + 4U) != PANEL_HOMEY_ALIAS_STORE_SCHEMA_VERSION ||
        get16(in + 6U) != RECORD_SIZE) {
        return false;
    }

    uint8_t tmp[RECORD_SIZE];
    memcpy(tmp, in, n);
    uint32_t got = get32(tmp + 12U);
    put32(tmp + 12U, 0U);
    if (got != crc32(tmp, n)) {
        return false;
    }

    if (in[RESERVED_0_OFFSET] != 0U ||
        in[RESERVED_1_OFFSET] != 0U ||
        in[RESERVED_2_OFFSET] != 0U) {
        return false;
    }

    memset(r, 0, sizeof(*r));
    r->generation = get32(in + 8U);
    memcpy(r->homey_identity_digest,
           in + DIGEST_OFFSET,
           DIGEST_SIZE);
    r->entry_count = in[ENTRY_COUNT_OFFSET];

    size_t offset = HDR;
    for (size_t i = 0;
         i < r->entry_count &&
         i < PANEL_HOMEY_ALIAS_STORE_MAX_ENTRIES;
         i++) {
        r->entries[i].dashboard_binding_index = in[offset];
        memcpy(r->entries[i].raw_device_id,
               in + offset + 4U,
               128U);
        memcpy(r->entries[i].raw_capability_id,
               in + offset + 132U,
               64U);
        offset += ENTRY_SIZE;
    }

    return record_ok(r);
}

panel_homey_alias_slot_t panel_homey_alias_select_slot(
    bool av,
    uint32_t ag,
    bool bv,
    uint32_t bg,
    panel_homey_alias_slot_t hint)
{
    if (hint == PANEL_HOMEY_ALIAS_SLOT_A && av) {
        return hint;
    }
    if (hint == PANEL_HOMEY_ALIAS_SLOT_B && bv) {
        return hint;
    }
    if (av && !bv) {
        return PANEL_HOMEY_ALIAS_SLOT_A;
    }
    if (bv && !av) {
        return PANEL_HOMEY_ALIAS_SLOT_B;
    }
    if (!av && !bv) {
        return PANEL_HOMEY_ALIAS_SLOT_NONE;
    }
    return (int32_t)(ag - bg) >= 0
        ? PANEL_HOMEY_ALIAS_SLOT_A
        : PANEL_HOMEY_ALIAS_SLOT_B;
}

bool panel_homey_alias_sha256(const char *text, uint8_t out[32])
{
    if (text == NULL || out == NULL || text[0] == '\0') {
        return false;
    }
#ifdef ESP_PLATFORM
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return false;
    }

    size_t digest_length = 0U;
    status = psa_hash_compute(
        PSA_ALG_SHA_256,
        (const uint8_t *)text,
        strlen(text),
        out,
        32U,
        &digest_length);
    return status == PSA_SUCCESS && digest_length == 32U;
#else
    return CC_SHA256(
        text,
        (CC_LONG)strlen(text),
        out) != NULL;
#endif
}

uint32_t panel_homey_alias_next_generation(
    bool av,
    uint32_t ag,
    bool bv,
    uint32_t bg)
{
    uint32_t newest = 0U;
    if (av) {
        newest = ag;
    }
    if (bv &&
        (newest == 0U || (int32_t)(bg - newest) > 0)) {
        newest = bg;
    }
    return newest == UINT32_MAX ? 1U : newest + 1U;
}

panel_homey_alias_store_result_t panel_homey_alias_runtime_activate(
    panel_homey_alias_runtime_t *rt,
    const panel_homey_alias_record_t *r,
    const char *homey)
{
    if (!rt || !r || !homey || !homey[0] || !record_ok(r)) {
        return PANEL_HOMEY_ALIAS_STORE_INVALID;
    }

    uint8_t digest[32];
    if (!panel_homey_alias_sha256(homey, digest)) {
        memset(rt, 0, sizeof(*rt));
        return PANEL_HOMEY_ALIAS_STORE_INVALID;
    }
    if (memcmp(digest,
               r->homey_identity_digest,
               sizeof(digest)) != 0) {
        memset(rt, 0, sizeof(*rt));
        return PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED;
    }

    rt->record = *r;
    rt->configured = true;
    return PANEL_HOMEY_ALIAS_STORE_OK;
}

void panel_homey_alias_runtime_invalidate(
    panel_homey_alias_runtime_t *rt)
{
    if (rt) {
        memset(rt, 0, sizeof(*rt));
    }
}

panel_homey_read_result_t panel_homey_alias_runtime_resolve(
    void *ctx,
    const char *dev,
    const char *cap,
    char *device_alias,
    size_t device_alias_capacity,
    char *capability_alias,
    size_t capability_alias_capacity)
{
    if (device_alias && device_alias_capacity) {
        device_alias[0] = 0;
    }
    if (capability_alias && capability_alias_capacity) {
        capability_alias[0] = 0;
    }

    panel_homey_alias_runtime_t *rt = ctx;
    if (!rt || !rt->configured) {
        return PANEL_HOMEY_READ_NOT_CONFIGURED;
    }

    for (size_t i = 0; i < rt->record.entry_count; i++) {
        panel_homey_alias_entry_t *e = &rt->record.entries[i];
        if (strcmp(dev, e->raw_device_id) == 0 &&
            strcmp(cap, e->raw_capability_id) == 0) {
            const char *alias =
                panel_homey_dashboard_device_alias(
                    e->dashboard_binding_index);
            const char *capability =
                panel_homey_dashboard_capability_alias(
                    e->dashboard_binding_index);
            if (!alias || !capability ||
                strlen(alias) + 1U > device_alias_capacity ||
                strlen(capability) + 1U > capability_alias_capacity) {
                return PANEL_HOMEY_READ_OVERFLOW;
            }
            strcpy(device_alias, alias);
            strcpy(capability_alias, capability);
            return PANEL_HOMEY_READ_OK;
        }
    }

    return PANEL_HOMEY_READ_NOT_FOUND;
}

#ifdef ESP_PLATFORM
#include "nvs.h"

#define KA "slot_a"
#define KB "slot_b"
#define KACTIVE "active_slot"

static const char *key(panel_homey_alias_slot_t slot)
{
    return slot == PANEL_HOMEY_ALIAS_SLOT_A ? KA : KB;
}

static bool readslot(
    nvs_handle_t handle,
    panel_homey_alias_slot_t slot,
    panel_homey_alias_record_t *record,
    bool *valid)
{
    uint8_t blob[RECORD_SIZE];
    size_t size = sizeof(blob);
    esp_err_t error = nvs_get_blob(
        handle,
        key(slot),
        blob,
        &size);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        *valid = false;
        return true;
    }
    if (error != ESP_OK) {
        *valid = false;
        return false;
    }
    *valid = panel_homey_alias_record_decode(
        blob,
        size,
        record);
    return true;
}

panel_homey_alias_store_result_t panel_homey_alias_store_load(
    const char *homey,
    panel_homey_alias_record_t *out,
    bool *present)
{
    if (!homey || !out || !present) {
        return PANEL_HOMEY_ALIAS_STORE_INVALID;
    }

    *present = false;
    nvs_handle_t handle;
    esp_err_t error = nvs_open(
        PANEL_HOMEY_ALIAS_STORE_NAMESPACE,
        NVS_READONLY,
        &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        return PANEL_HOMEY_ALIAS_STORE_NOT_FOUND;
    }
    if (error != ESP_OK) {
        return PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
    }

    panel_homey_alias_record_t a;
    panel_homey_alias_record_t b;
    bool av = false;
    bool bv = false;
    if (!readslot(handle,
                  PANEL_HOMEY_ALIAS_SLOT_A,
                  &a,
                  &av) ||
        !readslot(handle,
                  PANEL_HOMEY_ALIAS_SLOT_B,
                  &b,
                  &bv)) {
        nvs_close(handle);
        return PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
    }

    uint8_t active = 0;
    nvs_get_u8(handle, KACTIVE, &active);
    nvs_close(handle);

    panel_homey_alias_slot_t slot =
        panel_homey_alias_select_slot(
            av,
            a.generation,
            bv,
            b.generation,
            (panel_homey_alias_slot_t)active);
    if (slot == PANEL_HOMEY_ALIAS_SLOT_NONE) {
        return PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED;
    }

    *out = slot == PANEL_HOMEY_ALIAS_SLOT_A ? a : b;
    panel_homey_alias_runtime_t runtime = {0};
    panel_homey_alias_store_result_t result =
        panel_homey_alias_runtime_activate(
            &runtime,
            out,
            homey);
    if (result != PANEL_HOMEY_ALIAS_STORE_OK) {
        return result;
    }

    *present = true;
    return PANEL_HOMEY_ALIAS_STORE_OK;
}

panel_homey_alias_store_result_t panel_homey_alias_store_publish(
    const char *homey,
    const panel_homey_alias_record_t *input)
{
    if (!homey || !input) {
        return PANEL_HOMEY_ALIAS_STORE_INVALID;
    }

    nvs_handle_t handle;
    if (nvs_open(PANEL_HOMEY_ALIAS_STORE_NAMESPACE,
                 NVS_READWRITE,
                 &handle) != ESP_OK) {
        return PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
    }

    panel_homey_alias_record_t a = {0};
    panel_homey_alias_record_t b = {0};
    bool av = false;
    bool bv = false;
    if (!readslot(handle,
                  PANEL_HOMEY_ALIAS_SLOT_A,
                  &a,
                  &av) ||
        !readslot(handle,
                  PANEL_HOMEY_ALIAS_SLOT_B,
                  &b,
                  &bv)) {
        nvs_close(handle);
        return PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
    }

    uint8_t active = 0;
    if (nvs_get_u8(handle, KACTIVE, &active) != ESP_OK ||
        active > 2U) {
        active = 0;
    }

    panel_homey_alias_slot_t current =
        panel_homey_alias_select_slot(
            av,
            a.generation,
            bv,
            b.generation,
            (panel_homey_alias_slot_t)active);
    panel_homey_alias_slot_t target =
        current == PANEL_HOMEY_ALIAS_SLOT_A
            ? PANEL_HOMEY_ALIAS_SLOT_B
            : PANEL_HOMEY_ALIAS_SLOT_A;

    panel_homey_alias_record_t record = *input;
    record.generation = panel_homey_alias_next_generation(
        av,
        a.generation,
        bv,
        b.generation);
    if (!panel_homey_alias_sha256(
            homey,
            record.homey_identity_digest)) {
        nvs_close(handle);
        return PANEL_HOMEY_ALIAS_STORE_INVALID;
    }

    uint8_t blob[RECORD_SIZE];
    uint8_t verify[RECORD_SIZE];
    if (!panel_homey_alias_record_encode(
            &record,
            blob,
            sizeof(blob)) ||
        nvs_set_blob(handle,
                     key(target),
                     blob,
                     sizeof(blob)) != ESP_OK ||
        nvs_commit(handle) != ESP_OK) {
        nvs_close(handle);
        return PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
    }

    size_t size = sizeof(verify);
    if (nvs_get_blob(handle,
                     key(target),
                     verify,
                     &size) != ESP_OK ||
        size != sizeof(verify) ||
        memcmp(blob, verify, size) != 0) {
        nvs_close(handle);
        return PANEL_HOMEY_ALIAS_STORE_VERIFY_ERROR;
    }

    if (nvs_set_u8(handle,
                   KACTIVE,
                   (uint8_t)target) != ESP_OK ||
        nvs_commit(handle) != ESP_OK) {
        nvs_close(handle);
        return PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
    }

    nvs_close(handle);
    return PANEL_HOMEY_ALIAS_STORE_OK;
}

panel_homey_alias_store_result_t panel_homey_alias_store_wipe(void)
{
    nvs_handle_t handle;
    esp_err_t error = nvs_open(
        PANEL_HOMEY_ALIAS_STORE_NAMESPACE,
        NVS_READWRITE,
        &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        return PANEL_HOMEY_ALIAS_STORE_OK;
    }
    if (error != ESP_OK) {
        return PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
    }

    error = nvs_erase_all(handle);
    if (error == ESP_OK) {
        error = nvs_commit(handle);
    }
    nvs_close(handle);
    return error == ESP_OK
        ? PANEL_HOMEY_ALIAS_STORE_OK
        : PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
}
#endif
