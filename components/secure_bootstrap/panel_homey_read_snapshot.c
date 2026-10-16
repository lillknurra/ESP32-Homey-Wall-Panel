#include "panel_homey_read_snapshot.h"

#include "cJSON.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#define HOMEY_POSITION_CAPABILITY_ID "windowcoverings_set"
#define HOMEY_POSITION_MIN 0.0
#define HOMEY_POSITION_MAX 1.0

static void store_lock(const panel_homey_snapshot_store_t *store)
{
    if (store != NULL && store->lock != NULL) {
        store->lock(store->lock_context);
    }
}

static void store_unlock(const panel_homey_snapshot_store_t *store)
{
    if (store != NULL && store->unlock != NULL) {
        store->unlock(store->lock_context);
    }
}

static bool alias_valid(const char *value, size_t capacity)
{
    if (value == NULL || value[0] == '\0') {
        return false;
    }
    size_t length = 0U;
    while (length < capacity && value[length] != '\0') {
        length++;
    }
    if (length == 0U || length >= capacity) {
        return false;
    }
    for (size_t index = 0U; index < length; ++index) {
        const unsigned char ch = (unsigned char)value[index];
        if (!(islower(ch) || isdigit(ch) || ch == '_' || ch == '-')) {
            return false;
        }
    }
    return true;
}

static bool same_binding(
    const panel_homey_read_item_t *left,
    const char *device_alias,
    const char *capability_alias)
{
    return strcmp(left->device_alias, device_alias) == 0 &&
           strcmp(left->capability_alias, capability_alias) == 0;
}

static panel_homey_read_result_t append_item(
    panel_homey_read_snapshot_t *snapshot,
    const char *device_alias,
    const char *capability_alias,
    bool availability_known,
    bool available,
    bool position_binding_verified,
    const cJSON *value)
{
    if (!alias_valid(device_alias, PANEL_HOMEY_ALIAS_MAX) ||
        !alias_valid(capability_alias, PANEL_HOMEY_CAPABILITY_ALIAS_MAX)) {
        return PANEL_HOMEY_READ_INVALID;
    }
    for (size_t index = 0U; index < snapshot->item_count; ++index) {
        if (same_binding(&snapshot->items[index], device_alias, capability_alias)) {
            return PANEL_HOMEY_READ_DUPLICATE;
        }
    }
    if (snapshot->item_count >= PANEL_HOMEY_SNAPSHOT_MAX_ITEMS) {
        return PANEL_HOMEY_READ_OVERFLOW;
    }

    panel_homey_read_item_t *item = &snapshot->items[snapshot->item_count];
    memset(item, 0, sizeof(*item));
    memcpy(item->device_alias, device_alias, strlen(device_alias) + 1U);
    memcpy(item->capability_alias, capability_alias, strlen(capability_alias) + 1U);
    item->availability_known = availability_known;
    item->available = available;
    item->position_binding_verified = position_binding_verified;
    if (cJSON_IsBool(value)) {
        item->value_type = PANEL_HOMEY_VALUE_BOOL;
        item->bool_value = cJSON_IsTrue(value);
    } else if (cJSON_IsNumber(value) && isfinite(value->valuedouble) &&
               (!position_binding_verified ||
                (value->valuedouble >= HOMEY_POSITION_MIN &&
                 value->valuedouble <= HOMEY_POSITION_MAX))) {
        item->value_type = PANEL_HOMEY_VALUE_NUMBER;
        item->number_value = value->valuedouble;
    } else {
        item->value_type = PANEL_HOMEY_VALUE_NONE;
        item->bool_value = false;
        item->number_value = 0.0;
        item->position_binding_verified = false;
    }
    snapshot->item_count++;
    return PANEL_HOMEY_READ_OK;
}

static bool captured_position_binding_verified(
    const panel_homey_alias_snapshot_t *bindings,
    const char *raw_device_id,
    const char *raw_capability_id,
    const char *device_alias,
    const char *capability_alias)
{
    if (bindings == NULL || !bindings->configured || raw_device_id == NULL ||
        raw_capability_id == NULL || device_alias == NULL || capability_alias == NULL ||
        strcmp(raw_capability_id, HOMEY_POSITION_CAPABILITY_ID) != 0 ||
        strcmp(capability_alias, "status") != 0 ||
        (strcmp(device_alias, "awning_2") != 0 &&
         strcmp(device_alias, "awning_3") != 0)) {
        return false;
    }

    size_t role_matches = 0U;
    for (size_t index = 0U; index < bindings->entry_count; ++index) {
        const panel_homey_alias_snapshot_entry_t *entry = &bindings->entries[index];
        const uint8_t expected_role = strcmp(device_alias, "awning_2") == 0 ? 1U : 2U;
        if (strcmp(entry->device_alias, device_alias) == 0) {
            role_matches++;
            if (entry->dashboard_binding_index != expected_role ||
                strcmp(entry->raw_device_id, raw_device_id) != 0 ||
                strcmp(entry->raw_capability_id, raw_capability_id) != 0 ||
                strcmp(entry->capability_alias, capability_alias) != 0) {
                return false;
            }
        }
    }
    return role_matches == 1U;
}

static const char *device_id(const cJSON *device)
{
    const cJSON *id = cJSON_GetObjectItemCaseSensitive((cJSON *)device, "_id");
    if (!cJSON_IsString(id) || id->valuestring == NULL || id->valuestring[0] == '\0') {
        id = cJSON_GetObjectItemCaseSensitive((cJSON *)device, "id");
    }
    return cJSON_IsString(id) && id->valuestring != NULL && id->valuestring[0] != '\0'
        ? id->valuestring
        : NULL;
}

static void load_awning_stages(
    const panel_homey_alias_snapshot_t *bindings,
    panel_homey_awning_match_stages_t stages[PANEL_HOMEY_AWNING_ROLE_COUNT])
{
    if (!bindings->configured) {
        return;
    }
    for (uint8_t role = 0U; role < PANEL_HOMEY_AWNING_ROLE_COUNT; ++role) {
        bool present = false;
        for (size_t index = 0U; index < bindings->entry_count; ++index) {
            if (bindings->entries[index].dashboard_binding_index == role) {
                present = true;
                break;
            }
        }
        stages[role].binding_entry_present = present
            ? PANEL_HOMEY_MATCH_VALUE_TRUE : PANEL_HOMEY_MATCH_VALUE_FALSE;
        stages[role].matched = PANEL_HOMEY_MATCH_VALUE_FALSE;
        if (present) {
            stages[role].device_present = PANEL_HOMEY_MATCH_VALUE_FALSE;
        }
    }
}

static void inspect_awning_device(
    const char *raw_device_id,
    const cJSON *capabilities,
    const panel_homey_alias_snapshot_t *bindings,
    panel_homey_awning_match_stages_t stages[PANEL_HOMEY_AWNING_ROLE_COUNT])
{
    if (!bindings->configured) {
        return;
    }
    for (size_t index = 0U; index < bindings->entry_count; ++index) {
        const panel_homey_alias_snapshot_entry_t *binding =
            &bindings->entries[index];
        const uint8_t role = binding->dashboard_binding_index;
        if (role >= PANEL_HOMEY_AWNING_ROLE_COUNT ||
            strcmp(raw_device_id, binding->raw_device_id) != 0) {
            continue;
        }
        stages[role].device_present = PANEL_HOMEY_MATCH_VALUE_TRUE;
        stages[role].capability_present = PANEL_HOMEY_MATCH_VALUE_FALSE;
        if (cJSON_IsObject(capabilities) &&
            cJSON_GetObjectItemCaseSensitive(
                (cJSON *)capabilities,
                binding->raw_capability_id) != NULL) {
            stages[role].capability_present = PANEL_HOMEY_MATCH_VALUE_TRUE;
            stages[role].matched = PANEL_HOMEY_MATCH_VALUE_TRUE;
        }
    }
}

static panel_homey_read_result_t resolve_captured_binding(
    const panel_homey_alias_snapshot_t *bindings,
    const char *raw_device_id,
    const char *raw_capability_id,
    char *device_alias_out,
    size_t device_alias_capacity,
    char *capability_alias_out,
    size_t capability_alias_capacity)
{
    if (!bindings->configured) {
        return PANEL_HOMEY_READ_NOT_CONFIGURED;
    }
    for (size_t index = 0U; index < bindings->entry_count; ++index) {
        const panel_homey_alias_snapshot_entry_t *binding =
            &bindings->entries[index];
        if (strcmp(raw_device_id, binding->raw_device_id) != 0 ||
            strcmp(raw_capability_id, binding->raw_capability_id) != 0) {
            continue;
        }
        const size_t device_length = strlen(binding->device_alias);
        const size_t capability_length = strlen(binding->capability_alias);
        if (device_length + 1U > device_alias_capacity ||
            capability_length + 1U > capability_alias_capacity) {
            return PANEL_HOMEY_READ_OVERFLOW;
        }
        memcpy(device_alias_out, binding->device_alias, device_length + 1U);
        memcpy(capability_alias_out, binding->capability_alias,
               capability_length + 1U);
        return PANEL_HOMEY_READ_OK;
    }
    return PANEL_HOMEY_READ_NOT_FOUND;
}

static panel_homey_read_result_t parse_device(
    panel_homey_read_snapshot_t *snapshot,
    const cJSON *device,
    const panel_homey_alias_provider_t *provider,
    const panel_homey_alias_snapshot_t *bindings,
    bool captured_bindings)
{
    if (!cJSON_IsObject(device)) {
        return PANEL_HOMEY_READ_INVALID;
    }
    const char *raw_device_id = device_id(device);
    if (raw_device_id == NULL) {
        return PANEL_HOMEY_READ_INVALID;
    }

    const cJSON *available_json =
        cJSON_GetObjectItemCaseSensitive((cJSON *)device, "available");
    const bool availability_known = cJSON_IsBool(available_json);
    const bool available = availability_known && cJSON_IsTrue(available_json);
    const cJSON *capabilities =
        cJSON_GetObjectItemCaseSensitive((cJSON *)device, "capabilitiesObj");
    inspect_awning_device(raw_device_id, capabilities, bindings,
                          snapshot->awning_match_stages);
    if (!cJSON_IsObject(capabilities)) {
        return PANEL_HOMEY_READ_OK;
    }

    const cJSON *capability = NULL;
    cJSON_ArrayForEach(capability, capabilities) {
        if (capability->string == NULL || capability->string[0] == '\0') {
            return PANEL_HOMEY_READ_INVALID;
        }
        char device_alias[PANEL_HOMEY_ALIAS_MAX] = {0};
        char capability_alias[PANEL_HOMEY_CAPABILITY_ALIAS_MAX] = {0};
        panel_homey_read_result_t resolved = captured_bindings
            ? resolve_captured_binding(
                  bindings, raw_device_id, capability->string,
                  device_alias, sizeof(device_alias),
                  capability_alias, sizeof(capability_alias))
            : provider->resolve(
                  provider->context, raw_device_id, capability->string,
                  device_alias, sizeof(device_alias),
                  capability_alias, sizeof(capability_alias));
        if (resolved == PANEL_HOMEY_READ_NOT_FOUND) {
            continue;
        }
        if (resolved != PANEL_HOMEY_READ_OK) {
            return resolved;
        }
        const bool position_binding_verified = captured_bindings &&
            captured_position_binding_verified(
                bindings, raw_device_id, capability->string,
                device_alias, capability_alias);
        const cJSON *value = cJSON_IsObject(capability)
            ? cJSON_GetObjectItemCaseSensitive((cJSON *)capability, "value")
            : NULL;
        panel_homey_read_result_t appended = append_item(
            snapshot,
            device_alias,
            capability_alias,
            availability_known,
            available,
            position_binding_verified,
            value);
        if (appended != PANEL_HOMEY_READ_OK) {
            return appended;
        }
        if (position_binding_verified && cJSON_IsObject(capability)) {
            panel_homey_read_item_t *item =
                &snapshot->items[snapshot->item_count - 1U];
            const cJSON *names = cJSON_GetObjectItemCaseSensitive(
                (cJSON *)device, "capabilities");
            size_t id_occurrences = 0U;
            const cJSON *name = NULL;
            cJSON_ArrayForEach(name, names) {
                if (cJSON_IsString(name) && name->valuestring != NULL &&
                    strcmp(name->valuestring, HOMEY_POSITION_CAPABILITY_ID) == 0) {
                    id_occurrences++;
                }
            }
            const cJSON *type = cJSON_GetObjectItemCaseSensitive(
                (cJSON *)capability, "type");
            const cJSON *getable = cJSON_GetObjectItemCaseSensitive(
                (cJSON *)capability, "getable");
            const cJSON *setable = cJSON_GetObjectItemCaseSensitive(
                (cJSON *)capability, "setable");
            const cJSON *minimum = cJSON_GetObjectItemCaseSensitive(
                (cJSON *)capability, "min");
            const cJSON *maximum = cJSON_GetObjectItemCaseSensitive(
                (cJSON *)capability, "max");
            const cJSON *step = cJSON_GetObjectItemCaseSensitive(
                (cJSON *)capability, "step");
            const bool step_valid = step == NULL ||
                (cJSON_IsNumber(step) && isfinite(step->valuedouble) &&
                 step->valuedouble > 0.0);
            const bool contract_valid = id_occurrences == 1U &&
                cJSON_IsString(type) && type->valuestring != NULL &&
                strcmp(type->valuestring, "number") == 0 &&
                cJSON_IsBool(getable) && cJSON_IsTrue(getable) &&
                cJSON_IsBool(setable) && cJSON_IsTrue(setable) &&
                cJSON_IsNumber(minimum) && isfinite(minimum->valuedouble) &&
                cJSON_IsNumber(maximum) && isfinite(maximum->valuedouble) &&
                minimum->valuedouble <= maximum->valuedouble && step_valid;
            if (contract_valid) {
                item->position_contract_verified = true;
                item->position_getable = true;
                item->position_setable = true;
                item->position_min = minimum->valuedouble;
                item->position_max = maximum->valuedouble;
                item->position_step_present = step != NULL;
                item->position_step = step != NULL ? step->valuedouble : 0.0;
            }
        }
    }
    return PANEL_HOMEY_READ_OK;
}

panel_homey_read_result_t panel_homey_alias_provider_not_configured(
    void *context,
    const char *raw_device_id,
    const char *raw_capability_id,
    char *device_alias_out,
    size_t device_alias_capacity,
    char *capability_alias_out,
    size_t capability_alias_capacity)
{
    (void)context;
    (void)raw_device_id;
    (void)raw_capability_id;
    if (device_alias_out != NULL && device_alias_capacity > 0U) {
        device_alias_out[0] = '\0';
    }
    if (capability_alias_out != NULL && capability_alias_capacity > 0U) {
        capability_alias_out[0] = '\0';
    }
    return PANEL_HOMEY_READ_NOT_CONFIGURED;
}

void panel_homey_snapshot_store_init(
    panel_homey_snapshot_store_t *store,
    void *lock_context,
    panel_homey_snapshot_lock_fn lock,
    panel_homey_snapshot_lock_fn unlock)
{
    if (store == NULL) {
        return;
    }
    memset(store, 0, sizeof(*store));
    store->lock_context = lock_context;
    store->lock = lock;
    store->unlock = unlock;
}

static panel_homey_read_result_t snapshot_publish_json_impl(
    panel_homey_snapshot_store_t *store,
    const char *device_json,
    const panel_homey_alias_provider_t *provider,
    uint64_t now_ms)
{
    if (store == NULL || device_json == NULL || provider == NULL ||
        (provider->capture == NULL && provider->resolve == NULL)) {
        return PANEL_HOMEY_READ_INVALID;
    }

    cJSON *root = cJSON_Parse(device_json);
    if (root == NULL) {
        return PANEL_HOMEY_READ_INVALID;
    }
    cJSON *devices = cJSON_GetObjectItemCaseSensitive(root, "result");
    if (devices == NULL) {
        devices = root;
    }
    if (!cJSON_IsObject(devices) && !cJSON_IsArray(devices)) {
        cJSON_Delete(root);
        return PANEL_HOMEY_READ_INVALID;
    }

    panel_homey_read_snapshot_t candidate;
    memset(&candidate, 0, sizeof(candidate));
    candidate.captured_at_ms = now_ms;
    panel_homey_alias_snapshot_t alias_snapshot = {0};
    const bool captured_bindings = provider->capture != NULL;
    if (captured_bindings) {
        const panel_homey_read_result_t capture_result = provider->capture(
            provider->context, &alias_snapshot);
        if (capture_result != PANEL_HOMEY_READ_OK &&
            capture_result != PANEL_HOMEY_READ_NOT_CONFIGURED) {
            cJSON_Delete(root);
            memset(&alias_snapshot, 0, sizeof(alias_snapshot));
            return capture_result;
        }
        if (capture_result == PANEL_HOMEY_READ_NOT_CONFIGURED) {
            memset(&alias_snapshot, 0, sizeof(alias_snapshot));
        }
        load_awning_stages(&alias_snapshot, candidate.awning_match_stages);
        if (alias_snapshot.configured) {
            candidate.alias_generation = alias_snapshot.generation;
            candidate.selected_homey_digest_valid =
                alias_snapshot.homey_identity_digest_valid;
            if (candidate.selected_homey_digest_valid) {
                memcpy(candidate.selected_homey_digest,
                       alias_snapshot.homey_identity_digest,
                       sizeof(candidate.selected_homey_digest));
            }
        }
    }

    panel_homey_read_result_t result = PANEL_HOMEY_READ_OK;
    const cJSON *device = NULL;
    cJSON_ArrayForEach(device, devices) {
        result = parse_device(
            &candidate, device, provider, &alias_snapshot, captured_bindings);
        if (result != PANEL_HOMEY_READ_OK) {
            break;
        }
    }
    cJSON_Delete(root);
    memset(&alias_snapshot, 0, sizeof(alias_snapshot));
    if (result != PANEL_HOMEY_READ_OK) {
        return result;
    }

    store_lock(store);
    const uint8_t inactive = store->active_valid ? (uint8_t)(1U - store->active_index) : 0U;
    const uint32_t previous_generation = store->active_valid
        ? store->buffers[store->active_index].generation
        : 0U;
    candidate.generation = previous_generation + 1U;
    store->buffers[inactive] = candidate;
    store->active_index = inactive;
    store->active_valid = true;
    store->last_publish.attempted = true;
    store->last_publish.result = PANEL_HOMEY_READ_OK;
    store->last_publish.attempted_at_ms = now_ms;
    store_unlock(store);
    return PANEL_HOMEY_READ_OK;
}

panel_homey_read_result_t panel_homey_snapshot_publish_json(
    panel_homey_snapshot_store_t *store,
    const char *device_json,
    const panel_homey_alias_provider_t *provider,
    uint64_t now_ms)
{
    if (store == NULL) {
        return PANEL_HOMEY_READ_INVALID;
    }

    const panel_homey_read_result_t result = snapshot_publish_json_impl(
        store, device_json, provider, now_ms);
    if (result != PANEL_HOMEY_READ_OK) {
        store_lock(store);
        store->last_publish.attempted = true;
        store->last_publish.result = result;
        store->last_publish.attempted_at_ms = now_ms;
        store_unlock(store);
    }
    return result;
}

panel_homey_read_result_t panel_homey_snapshot_copy(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms,
    panel_homey_read_snapshot_t *out)
{
    if (store == NULL || out == NULL) {
        return PANEL_HOMEY_READ_INVALID;
    }
    store_lock(store);
    if (!store->active_valid) {
        store_unlock(store);
        return PANEL_HOMEY_READ_NOT_FOUND;
    }
    const panel_homey_read_snapshot_t snapshot = store->buffers[store->active_index];
    store_unlock(store);

    if (now_ms < snapshot.captured_at_ms ||
        now_ms - snapshot.captured_at_ms > PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS) {
        return PANEL_HOMEY_READ_STALE;
    }
    *out = snapshot;
    return PANEL_HOMEY_READ_OK;
}

panel_homey_read_result_t panel_homey_snapshot_inspect(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms,
    panel_homey_snapshot_inspection_t *out)
{
    panel_homey_snapshot_publish_inspection_t ignored_publish;
    return panel_homey_snapshot_inspect_with_publish(
        store, now_ms, out, &ignored_publish);
}

panel_homey_read_result_t panel_homey_snapshot_read_awning(
    const panel_homey_snapshot_store_t *store,
    const char *device_alias,
    const char *capability_alias,
    uint64_t now_ms,
    panel_homey_awning_snapshot_t *out)
{
    if (store == NULL || device_alias == NULL || capability_alias == NULL ||
        out == NULL || device_alias[0] == '\0' || capability_alias[0] == '\0') {
        return PANEL_HOMEY_READ_INVALID;
    }
    memset(out, 0, sizeof(*out));
    store_lock(store);
    if (!store->active_valid) {
        store_unlock(store);
        return PANEL_HOMEY_READ_NOT_FOUND;
    }
    const panel_homey_read_snapshot_t *snapshot =
        &store->buffers[store->active_index];
    out->present = true;
    out->generation = snapshot->generation;
    out->alias_generation = snapshot->alias_generation;
    out->selected_homey_digest_valid = snapshot->selected_homey_digest_valid;
    memcpy(out->selected_homey_digest, snapshot->selected_homey_digest,
           sizeof(out->selected_homey_digest));
    if (now_ms < snapshot->captured_at_ms) {
        out->fresh = false;
        store_unlock(store);
        return PANEL_HOMEY_READ_INVALID;
    }
    out->age_ms = now_ms - snapshot->captured_at_ms;
    out->fresh = out->age_ms <= PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS;
    for (size_t index = 0U; index < snapshot->item_count; ++index) {
        const panel_homey_read_item_t *item = &snapshot->items[index];
        if (strcmp(item->device_alias, device_alias) == 0 &&
            strcmp(item->capability_alias, capability_alias) == 0) {
            out->item_matches++;
            if (out->item_matches == 1U) out->item = *item;
        }
    }
    store_unlock(store);
    return out->fresh ? PANEL_HOMEY_READ_OK : PANEL_HOMEY_READ_STALE;
}

bool panel_homey_snapshot_matches_identity(
    const panel_homey_snapshot_store_t *store,
    const uint8_t selected_homey_digest[32],
    uint32_t *snapshot_generation_out,
    uint32_t *alias_generation_out)
{
    if (store == NULL || selected_homey_digest == NULL ||
        snapshot_generation_out == NULL || alias_generation_out == NULL) {
        return false;
    }
    *snapshot_generation_out = 0U;
    *alias_generation_out = 0U;
    store_lock(store);
    if (!store->active_valid) {
        store_unlock(store);
        return false;
    }
    const panel_homey_read_snapshot_t *snapshot =
        &store->buffers[store->active_index];
    const bool matches = snapshot->generation != 0U &&
        snapshot->alias_generation != 0U &&
        snapshot->selected_homey_digest_valid &&
        memcmp(snapshot->selected_homey_digest, selected_homey_digest, 32U) == 0;
    if (matches) {
        *snapshot_generation_out = snapshot->generation;
        *alias_generation_out = snapshot->alias_generation;
    }
    store_unlock(store);
    return matches;
}

panel_homey_read_result_t panel_homey_snapshot_inspect_with_publish(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms,
    panel_homey_snapshot_inspection_t *snapshot_out,
    panel_homey_snapshot_publish_inspection_t *publish_out)
{
    if (snapshot_out == NULL || publish_out == NULL) {
        return PANEL_HOMEY_READ_INVALID;
    }
    memset(snapshot_out, 0, sizeof(*snapshot_out));
    memset(publish_out, 0, sizeof(*publish_out));
    snapshot_out->result = PANEL_HOMEY_READ_INVALID;
    publish_out->result = PANEL_HOMEY_READ_INVALID;
    if (store == NULL) {
        return PANEL_HOMEY_READ_INVALID;
    }

    store_lock(store);
    const bool present = store->active_valid;
    if (present) {
        snapshot_out->snapshot = store->buffers[store->active_index];
    }
    const panel_homey_snapshot_publish_state_t state = store->last_publish;
    store_unlock(store);

    if (present) {
        snapshot_out->present = true;
        if (now_ms < snapshot_out->snapshot.captured_at_ms) {
            snapshot_out->result = PANEL_HOMEY_READ_INVALID;
        } else {
            snapshot_out->age_ms = now_ms - snapshot_out->snapshot.captured_at_ms;
            snapshot_out->fresh =
                snapshot_out->age_ms <= PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS;
            snapshot_out->result = snapshot_out->fresh
                ? PANEL_HOMEY_READ_OK : PANEL_HOMEY_READ_STALE;
        }
    } else {
        snapshot_out->result = PANEL_HOMEY_READ_NOT_FOUND;
    }

    if (state.attempted) {
        publish_out->attempted = true;
        publish_out->result = state.result;
        if (now_ms >= state.attempted_at_ms) {
            publish_out->age_valid = true;
            publish_out->age_ms = now_ms - state.attempted_at_ms;
        }
    }
    return snapshot_out->result;
}

void panel_homey_snapshot_publish_inspect(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms,
    panel_homey_snapshot_publish_inspection_t *out)
{
    panel_homey_snapshot_inspection_t ignored_snapshot;
    (void)panel_homey_snapshot_inspect_with_publish(
        store, now_ms, &ignored_snapshot, out);
}

panel_homey_read_result_t panel_homey_snapshot_find(
    const panel_homey_snapshot_store_t *store,
    const char *device_alias,
    const char *capability_alias,
    uint64_t now_ms,
    panel_homey_read_item_t *out)
{
    if (device_alias == NULL || capability_alias == NULL || out == NULL) {
        return PANEL_HOMEY_READ_INVALID;
    }
    panel_homey_read_snapshot_t snapshot;
    panel_homey_read_result_t result = panel_homey_snapshot_copy(store, now_ms, &snapshot);
    if (result != PANEL_HOMEY_READ_OK) {
        return result;
    }
    for (size_t index = 0U; index < snapshot.item_count; ++index) {
        if (same_binding(&snapshot.items[index], device_alias, capability_alias)) {
            *out = snapshot.items[index];
            return PANEL_HOMEY_READ_OK;
        }
    }
    return PANEL_HOMEY_READ_NOT_FOUND;
}
