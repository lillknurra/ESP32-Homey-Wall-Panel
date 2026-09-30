#include "panel_homey_alias_store.h"
#include "panel_homey_dashboard_binding.h"
#include <string.h>
#ifdef ESP_PLATFORM
#include "psa/crypto.h"
#include <stdlib.h>
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

static void diagnose_slot(
    panel_homey_alias_slot_diagnostic_t *out,
    bool present,
    bool structurally_valid,
    const panel_homey_alias_record_t *record,
    const uint8_t *selected_homey_digest,
    bool selected_homey_digest_valid)
{
    memset(out, 0, sizeof(*out));
    out->present = present;
    out->structurally_valid = present && structurally_valid &&
        record != NULL && record_ok(record);
    out->selected_homey_match = PANEL_HOMEY_ALIAS_DIAG_UNKNOWN;
    for (size_t i = 0U; i < 3U; ++i) {
        out->awning_present[i] = PANEL_HOMEY_ALIAS_DIAG_UNKNOWN;
    }
    if (!out->structurally_valid) {
        return;
    }

    out->generation_known = true;
    out->generation = record->generation;
    out->entry_count_known = true;
    out->entry_count = (uint8_t)record->entry_count;
    if (selected_homey_digest_valid) {
        out->selected_homey_match = memcmp(
            record->homey_identity_digest,
            selected_homey_digest,
            PANEL_HOMEY_IDENTITY_DIGEST_SIZE) == 0
                ? PANEL_HOMEY_ALIAS_DIAG_TRUE
                : PANEL_HOMEY_ALIAS_DIAG_FALSE;
    }

    for (size_t role = 0U; role < 3U; ++role) {
        bool present_for_role = false;
        for (size_t i = 0U; i < record->entry_count; ++i) {
            if (record->entries[i].dashboard_binding_index == role) {
                present_for_role = true;
                break;
            }
        }
        out->awning_present[role] = present_for_role
            ? PANEL_HOMEY_ALIAS_DIAG_TRUE
            : PANEL_HOMEY_ALIAS_DIAG_FALSE;
    }
}

bool panel_homey_alias_store_diagnose_records(
    const char *selected_homey_id,
    bool slot_a_present,
    bool slot_a_structurally_valid,
    const panel_homey_alias_record_t *slot_a,
    bool slot_b_present,
    bool slot_b_structurally_valid,
    const panel_homey_alias_record_t *slot_b,
    bool active_slot_value_present,
    bool active_slot_read_error,
    uint8_t active_slot_value,
    panel_homey_alias_store_diagnostic_t *diagnostic_out)
{
    if (diagnostic_out == NULL ||
        (slot_a_structurally_valid && (!slot_a_present || slot_a == NULL)) ||
        (slot_b_structurally_valid && (!slot_b_present || slot_b == NULL)) ||
        (active_slot_value_present && active_slot_read_error)) {
        return false;
    }

    memset(diagnostic_out, 0, sizeof(*diagnostic_out));
    diagnostic_out->result = active_slot_read_error
        ? PANEL_HOMEY_ALIAS_STORE_IO_ERROR
        : PANEL_HOMEY_ALIAS_STORE_OK;
    diagnostic_out->active_slot_hint = PANEL_HOMEY_ALIAS_HINT_UNKNOWN;
    diagnostic_out->effective_selected_slot = PANEL_HOMEY_ALIAS_SLOT_NONE;
    diagnostic_out->selection_basis = PANEL_HOMEY_ALIAS_SELECTION_UNKNOWN;

    uint8_t selected_homey_digest[PANEL_HOMEY_IDENTITY_DIGEST_SIZE] = {0};
    const bool selected_homey_digest_valid =
        selected_homey_id != NULL && selected_homey_id[0] != '\0' &&
        panel_homey_alias_sha256(
            selected_homey_id,
            selected_homey_digest);

    diagnose_slot(
        &diagnostic_out->slots[0],
        slot_a_present,
        slot_a_structurally_valid,
        slot_a,
        selected_homey_digest,
        selected_homey_digest_valid);
    diagnose_slot(
        &diagnostic_out->slots[1],
        slot_b_present,
        slot_b_structurally_valid,
        slot_b,
        selected_homey_digest,
        selected_homey_digest_valid);
    memset(selected_homey_digest, 0, sizeof(selected_homey_digest));

    if (active_slot_read_error) {
        return true;
    }

    if (!active_slot_value_present || active_slot_value == 0U) {
        diagnostic_out->active_slot_hint = PANEL_HOMEY_ALIAS_HINT_NONE;
    } else if (active_slot_value == PANEL_HOMEY_ALIAS_SLOT_A) {
        diagnostic_out->active_slot_hint = PANEL_HOMEY_ALIAS_HINT_A;
    } else if (active_slot_value == PANEL_HOMEY_ALIAS_SLOT_B) {
        diagnostic_out->active_slot_hint = PANEL_HOMEY_ALIAS_HINT_B;
    } else {
        diagnostic_out->active_slot_hint = PANEL_HOMEY_ALIAS_HINT_INVALID;
    }

    const bool a_valid = diagnostic_out->slots[0].structurally_valid;
    const bool b_valid = diagnostic_out->slots[1].structurally_valid;
    diagnostic_out->effective_selected_slot = panel_homey_alias_select_slot(
        a_valid,
        a_valid ? slot_a->generation : 0U,
        b_valid,
        b_valid ? slot_b->generation : 0U,
        diagnostic_out->active_slot_hint == PANEL_HOMEY_ALIAS_HINT_A
            ? PANEL_HOMEY_ALIAS_SLOT_A
            : diagnostic_out->active_slot_hint == PANEL_HOMEY_ALIAS_HINT_B
                ? PANEL_HOMEY_ALIAS_SLOT_B
                : PANEL_HOMEY_ALIAS_SLOT_NONE);

    if (diagnostic_out->effective_selected_slot == PANEL_HOMEY_ALIAS_SLOT_NONE) {
        diagnostic_out->selection_basis = PANEL_HOMEY_ALIAS_SELECTION_NONE;
    } else if (
        (diagnostic_out->active_slot_hint == PANEL_HOMEY_ALIAS_HINT_A && a_valid) ||
        (diagnostic_out->active_slot_hint == PANEL_HOMEY_ALIAS_HINT_B && b_valid)) {
        diagnostic_out->selection_basis = PANEL_HOMEY_ALIAS_SELECTION_ACTIVE_HINT;
    } else if (a_valid != b_valid) {
        diagnostic_out->selection_basis = PANEL_HOMEY_ALIAS_SELECTION_ONLY_VALID_SLOT;
    } else {
        diagnostic_out->selection_basis = PANEL_HOMEY_ALIAS_SELECTION_GENERATION;
    }
    return true;
}

bool panel_homey_alias_store_observations_equal(
    const panel_homey_alias_store_diagnostic_t *first,
    const panel_homey_alias_store_diagnostic_t *second)
{
    if (first == NULL || second == NULL ||
        first->result != second->result ||
        first->active_slot_hint != second->active_slot_hint ||
        first->effective_selected_slot != second->effective_selected_slot ||
        first->selection_basis != second->selection_basis) {
        return false;
    }

    for (size_t slot = 0U; slot < 2U; ++slot) {
        const panel_homey_alias_slot_diagnostic_t *a = &first->slots[slot];
        const panel_homey_alias_slot_diagnostic_t *b = &second->slots[slot];
        if (a->present != b->present ||
            a->structurally_valid != b->structurally_valid ||
            a->generation_known != b->generation_known ||
            a->generation != b->generation ||
            a->entry_count_known != b->entry_count_known ||
            a->entry_count != b->entry_count ||
            a->selected_homey_match != b->selected_homey_match) {
            return false;
        }
        for (size_t role = 0U; role < 3U; ++role) {
            if (a->awning_present[role] != b->awning_present[role]) {
                return false;
            }
        }
    }
    return true;
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

panel_homey_read_result_t panel_homey_alias_runtime_capture(
    void *context,
    panel_homey_alias_snapshot_t *snapshot_out)
{
    if (snapshot_out == NULL) {
        return PANEL_HOMEY_READ_INVALID;
    }
    memset(snapshot_out, 0, sizeof(*snapshot_out));

    const panel_homey_alias_runtime_t *runtime = context;
    if (runtime == NULL || !runtime->configured) {
        return PANEL_HOMEY_READ_NOT_CONFIGURED;
    }
    if (runtime->record.entry_count > PANEL_HOMEY_ALIAS_SNAPSHOT_MAX_ENTRIES) {
        return PANEL_HOMEY_READ_OVERFLOW;
    }

    snapshot_out->generation = runtime->record.generation;
    for (size_t index = 0U; index < runtime->record.entry_count; ++index) {
        const panel_homey_alias_entry_t *source = &runtime->record.entries[index];
        panel_homey_alias_snapshot_entry_t *target =
            &snapshot_out->entries[index];
        const size_t device_length = strnlen(
            source->raw_device_id, sizeof(source->raw_device_id));
        const size_t capability_length = strnlen(
            source->raw_capability_id, sizeof(source->raw_capability_id));
        const char *device_alias = panel_homey_dashboard_device_alias(
            source->dashboard_binding_index);
        const char *capability_alias = panel_homey_dashboard_capability_alias(
            source->dashboard_binding_index);
        if (device_length == 0U || device_length >= sizeof(source->raw_device_id) ||
            capability_length == 0U ||
            capability_length >= sizeof(source->raw_capability_id) ||
            device_alias == NULL || capability_alias == NULL) {
            memset(snapshot_out, 0, sizeof(*snapshot_out));
            return PANEL_HOMEY_READ_INVALID;
        }
        target->dashboard_binding_index = source->dashboard_binding_index;
        memcpy(target->raw_device_id, source->raw_device_id, device_length + 1U);
        memcpy(target->raw_capability_id, source->raw_capability_id,
               capability_length + 1U);
        memcpy(target->device_alias, device_alias, strlen(device_alias) + 1U);
        memcpy(target->capability_alias, capability_alias,
               strlen(capability_alias) + 1U);
    }
    snapshot_out->entry_count = runtime->record.entry_count;
    snapshot_out->configured = true;
    return PANEL_HOMEY_READ_OK;
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

static bool inspect_persisted_slot(
    nvs_handle_t handle,
    panel_homey_alias_slot_t slot,
    panel_homey_alias_record_t *record,
    uint8_t blob[RECORD_SIZE],
    bool *present,
    bool *structurally_valid)
{
    if (record == NULL || present == NULL || structurally_valid == NULL) {
        return false;
    }
    memset(record, 0, sizeof(*record));
    *present = false;
    *structurally_valid = false;

    size_t stored_size = 0U;
    esp_err_t error = nvs_get_blob(
        handle,
        key(slot),
        NULL,
        &stored_size);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (error != ESP_OK) {
        return false;
    }
    *present = true;
    if (stored_size != RECORD_SIZE) {
        return true;
    }

    memset(blob, 0, RECORD_SIZE);
    size_t read_size = RECORD_SIZE;
    error = nvs_get_blob(
        handle,
        key(slot),
        blob,
        &read_size);
    if (error != ESP_OK || read_size != RECORD_SIZE) {
        memset(blob, 0, RECORD_SIZE);
        return false;
    }
    *structurally_valid = panel_homey_alias_record_decode(
        blob,
        RECORD_SIZE,
        record);
    memset(blob, 0, RECORD_SIZE);
    if (!*structurally_valid) {
        memset(record, 0, sizeof(*record));
    }
    return true;
}

static panel_homey_alias_store_result_t inspect_store_once(
    const char *selected_homey_id,
    panel_homey_alias_store_diagnostic_t *diagnostic_out)
{
    if (diagnostic_out == NULL) {
        return PANEL_HOMEY_ALIAS_STORE_INVALID;
    }
    memset(diagnostic_out, 0, sizeof(*diagnostic_out));

    nvs_handle_t handle;
    esp_err_t error = nvs_open(
        PANEL_HOMEY_ALIAS_STORE_NAMESPACE,
        NVS_READONLY,
        &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        (void)panel_homey_alias_store_diagnose_records(
            selected_homey_id,
            false, false, NULL,
            false, false, NULL,
            false, false, 0U,
            diagnostic_out);
        diagnostic_out->result = PANEL_HOMEY_ALIAS_STORE_NOT_FOUND;
        return diagnostic_out->result;
    }
    if (error != ESP_OK) {
        diagnostic_out->result = PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
        return diagnostic_out->result;
    }

    const size_t record_bytes = sizeof(panel_homey_alias_record_t) * 2U;
    uint8_t *scratch = calloc(1U, record_bytes + RECORD_SIZE);
    if (scratch == NULL) {
        nvs_close(handle);
        diagnostic_out->result = PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
        return diagnostic_out->result;
    }
    panel_homey_alias_record_t *slot_a =
        (panel_homey_alias_record_t *)scratch;
    panel_homey_alias_record_t *slot_b = slot_a + 1;
    uint8_t *blob = scratch + record_bytes;
    bool slot_a_present = false;
    bool slot_a_valid = false;
    bool slot_b_present = false;
    bool slot_b_valid = false;
    const bool slots_read =
        inspect_persisted_slot(
            handle,
            PANEL_HOMEY_ALIAS_SLOT_A,
            slot_a,
            blob,
            &slot_a_present,
            &slot_a_valid) &&
        inspect_persisted_slot(
            handle,
            PANEL_HOMEY_ALIAS_SLOT_B,
            slot_b,
            blob,
            &slot_b_present,
            &slot_b_valid);

    uint8_t active_slot_value = 0U;
    error = nvs_get_u8(handle, KACTIVE, &active_slot_value);
    const bool active_slot_present = error == ESP_OK;
    const bool active_slot_read_error =
        error != ESP_OK && error != ESP_ERR_NVS_NOT_FOUND;
    nvs_close(handle);

    if (!slots_read) {
        memset(scratch, 0, record_bytes + RECORD_SIZE);
        free(scratch);
        diagnostic_out->result = PANEL_HOMEY_ALIAS_STORE_IO_ERROR;
        return diagnostic_out->result;
    }

    if (!panel_homey_alias_store_diagnose_records(
            selected_homey_id,
            slot_a_present,
            slot_a_valid,
            slot_a,
            slot_b_present,
            slot_b_valid,
            slot_b,
            active_slot_present,
            active_slot_read_error,
            active_slot_value,
            diagnostic_out)) {
        diagnostic_out->result = PANEL_HOMEY_ALIAS_STORE_INVALID;
    }
    memset(scratch, 0, record_bytes + RECORD_SIZE);
    free(scratch);
    return diagnostic_out->result;
}

panel_homey_alias_store_result_t panel_homey_alias_store_inspect(
    const char *selected_homey_id,
    panel_homey_alias_store_diagnostic_t *diagnostic_out)
{
    if (diagnostic_out == NULL) {
        return PANEL_HOMEY_ALIAS_STORE_INVALID;
    }

    panel_homey_alias_store_diagnostic_t first = {0};
    panel_homey_alias_store_diagnostic_t second = {0};
    const panel_homey_alias_store_result_t first_result = inspect_store_once(
        selected_homey_id, &first);
    const panel_homey_alias_store_result_t second_result = inspect_store_once(
        selected_homey_id, &second);

    memset(diagnostic_out, 0, sizeof(*diagnostic_out));
    if (first_result == PANEL_HOMEY_ALIAS_STORE_IO_ERROR ||
        second_result == PANEL_HOMEY_ALIAS_STORE_IO_ERROR ||
        first_result == PANEL_HOMEY_ALIAS_STORE_INVALID ||
        second_result == PANEL_HOMEY_ALIAS_STORE_INVALID) {
        diagnostic_out->result =
            first_result == PANEL_HOMEY_ALIAS_STORE_IO_ERROR ||
            second_result == PANEL_HOMEY_ALIAS_STORE_IO_ERROR
                ? PANEL_HOMEY_ALIAS_STORE_IO_ERROR
                : PANEL_HOMEY_ALIAS_STORE_INVALID;
        memset(&first, 0, sizeof(first));
        memset(&second, 0, sizeof(second));
        return diagnostic_out->result;
    }

    if (first_result != second_result ||
        !panel_homey_alias_store_observations_equal(&first, &second)) {
        diagnostic_out->result = PANEL_HOMEY_ALIAS_STORE_UNSTABLE;
        diagnostic_out->observation_stable = PANEL_HOMEY_ALIAS_DIAG_FALSE;
        memset(&first, 0, sizeof(first));
        memset(&second, 0, sizeof(second));
        return diagnostic_out->result;
    }

    *diagnostic_out = second;
    diagnostic_out->observation_stable = PANEL_HOMEY_ALIAS_DIAG_TRUE;
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    return diagnostic_out->result;
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
