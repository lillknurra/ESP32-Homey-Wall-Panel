#include "panel_homey_read_snapshot.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *raw_device_id;
    const char *raw_capability_id;
    const char *device_alias;
    const char *capability_alias;
    panel_homey_read_result_t result;
} binding_t;

typedef struct {
    unsigned lock_calls;
    unsigned unlock_calls;
    unsigned lock_depth;
    unsigned max_lock_depth;
    bool unlock_without_lock;
} lock_context_t;

typedef struct {
    const binding_t *bindings;
    size_t count;
    lock_context_t *lock;
    bool called_while_locked;
} provider_context_t;

static void test_lock(void *context)
{
    lock_context_t *lock = context;
    assert(lock != NULL);
    lock->lock_calls++;
    lock->lock_depth++;
    if (lock->lock_depth > lock->max_lock_depth) {
        lock->max_lock_depth = lock->lock_depth;
    }
}

static void test_unlock(void *context)
{
    lock_context_t *lock = context;
    assert(lock != NULL);
    lock->unlock_calls++;
    if (lock->lock_depth == 0U) {
        lock->unlock_without_lock = true;
        return;
    }
    lock->lock_depth--;
}

static void assert_lock_balanced(const lock_context_t *lock)
{
    assert(lock != NULL);
    assert(lock->lock_calls == lock->unlock_calls);
    assert(lock->lock_depth == 0U);
    assert(lock->max_lock_depth <= 1U);
    assert(!lock->unlock_without_lock);
}

static panel_homey_read_result_t resolve_binding(
    void *context,
    const char *raw_device_id,
    const char *raw_capability_id,
    char *device_alias_out,
    size_t device_alias_capacity,
    char *capability_alias_out,
    size_t capability_alias_capacity)
{
    provider_context_t *provider = context;
    if (provider == NULL) {
        return PANEL_HOMEY_READ_NOT_CONFIGURED;
    }
    if (provider->lock != NULL && provider->lock->lock_depth != 0U) {
        provider->called_while_locked = true;
    }
    for (size_t index = 0U; index < provider->count; ++index) {
        const binding_t *binding = &provider->bindings[index];
        if (strcmp(binding->raw_device_id, raw_device_id) != 0 ||
            strcmp(binding->raw_capability_id, raw_capability_id) != 0) {
            continue;
        }
        if (binding->result != PANEL_HOMEY_READ_OK) {
            return binding->result;
        }
        if (strlen(binding->device_alias) >= device_alias_capacity ||
            strlen(binding->capability_alias) >= capability_alias_capacity) {
            return PANEL_HOMEY_READ_OVERFLOW;
        }
        strcpy(device_alias_out, binding->device_alias);
        strcpy(capability_alias_out, binding->capability_alias);
        return PANEL_HOMEY_READ_OK;
    }
    return PANEL_HOMEY_READ_NOT_FOUND;
}

static bool memory_contains(
    const void *memory,
    size_t memory_size,
    const char *needle)
{
    const unsigned char *bytes = memory;
    const size_t needle_size = strlen(needle);
    if (needle_size == 0U || needle_size > memory_size) {
        return false;
    }
    for (size_t index = 0U; index + needle_size <= memory_size; ++index) {
        if (memcmp(bytes + index, needle, needle_size) == 0) {
            return true;
        }
    }
    return false;
}

static panel_homey_alias_provider_t make_provider(provider_context_t *context)
{
    const panel_homey_alias_provider_t provider = {
        .context = context,
        .resolve = resolve_binding,
    };
    return provider;
}

static void init_store(
    panel_homey_snapshot_store_t *store,
    lock_context_t *lock)
{
    memset(lock, 0, sizeof(*lock));
    panel_homey_snapshot_store_init(store, lock, test_lock, test_unlock);
}

static panel_homey_read_snapshot_t copy_ok(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms)
{
    panel_homey_read_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    assert(panel_homey_snapshot_copy(store, now_ms, &snapshot) ==
           PANEL_HOMEY_READ_OK);
    return snapshot;
}

static void assert_snapshot_equal(
    const panel_homey_read_snapshot_t *left,
    const panel_homey_read_snapshot_t *right)
{
    assert(memcmp(left, right, sizeof(*left)) == 0);
}

static void fill_alias(char *buffer, size_t length, char value)
{
    memset(buffer, (unsigned char)value, length);
    buffer[length] = '\0';
}

static char *make_inventory(size_t capability_count)
{
    const size_t capacity = 256U + capability_count * 96U;
    char *json = calloc(capacity, 1U);
    assert(json != NULL);
    size_t used = (size_t)snprintf(
        json,
        capacity,
        "{\"result\":{\"device\":{\"_id\":\"dev-many\","
        "\"available\":true,\"capabilitiesObj\":{");
    for (size_t index = 0U; index < capability_count; ++index) {
        used += (size_t)snprintf(
            json + used,
            capacity - used,
            "%s\"cap-%02zu\":{\"value\":%s}",
            index == 0U ? "" : ",",
            index,
            (index % 2U) == 0U ? "true" : "false");
        assert(used < capacity);
    }
    used += (size_t)snprintf(json + used, capacity - used, "}}}}}");
    assert(used < capacity);
    return json;
}

static void make_many_bindings(
    binding_t *bindings,
    char raw_ids[][16],
    char device_aliases[][PANEL_HOMEY_ALIAS_MAX],
    char capability_aliases[][PANEL_HOMEY_CAPABILITY_ALIAS_MAX],
    size_t count)
{
    for (size_t index = 0U; index < count; ++index) {
        snprintf(raw_ids[index], sizeof(raw_ids[index]), "cap-%02u", (unsigned)index);
        snprintf(device_aliases[index], sizeof(device_aliases[index]), "device_%02u", (unsigned)index);
        snprintf(capability_aliases[index], sizeof(capability_aliases[index]), "value_%02u", (unsigned)index);
        bindings[index] = (binding_t){
            .raw_device_id = "dev-many",
            .raw_capability_id = raw_ids[index],
            .device_alias = device_aliases[index],
            .capability_alias = capability_aliases[index],
            .result = PANEL_HOMEY_READ_OK,
        };
    }
}

static void test_null_arguments(void)
{
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    const panel_homey_alias_provider_t provider = {
        .context = NULL,
        .resolve = panel_homey_alias_provider_not_configured,
    };
    panel_homey_read_snapshot_t snapshot;
    panel_homey_read_item_t item;

    panel_homey_snapshot_store_init(NULL, NULL, NULL, NULL);
    assert(panel_homey_snapshot_publish_json(NULL, "{}", &provider, 0U) ==
           PANEL_HOMEY_READ_INVALID);
    assert(panel_homey_snapshot_publish_json(&store, NULL, &provider, 0U) ==
           PANEL_HOMEY_READ_INVALID);
    assert(panel_homey_snapshot_publish_json(&store, "{}", NULL, 0U) ==
           PANEL_HOMEY_READ_INVALID);
    const panel_homey_alias_provider_t missing_resolve = {0};
    assert(panel_homey_snapshot_publish_json(
               &store, "{}", &missing_resolve, 0U) ==
           PANEL_HOMEY_READ_INVALID);
    assert(panel_homey_snapshot_copy(NULL, 0U, &snapshot) ==
           PANEL_HOMEY_READ_INVALID);
    assert(panel_homey_snapshot_copy(&store, 0U, NULL) ==
           PANEL_HOMEY_READ_INVALID);
    assert(panel_homey_snapshot_find(NULL, "a", "b", 0U, &item) ==
           PANEL_HOMEY_READ_INVALID);
    assert(panel_homey_snapshot_find(&store, NULL, "b", 0U, &item) ==
           PANEL_HOMEY_READ_INVALID);
    assert(panel_homey_snapshot_find(&store, "a", NULL, 0U, &item) ==
           PANEL_HOMEY_READ_INVALID);
    assert(panel_homey_snapshot_find(&store, "a", "b", 0U, NULL) ==
           PANEL_HOMEY_READ_INVALID);
    assert_lock_balanced(&lock);
}

static void test_copy_before_publication(void)
{
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    panel_homey_read_snapshot_t snapshot;
    panel_homey_read_item_t item;
    assert(panel_homey_snapshot_copy(&store, 0U, &snapshot) ==
           PANEL_HOMEY_READ_NOT_FOUND);
    assert(panel_homey_snapshot_find(&store, "device", "capability", 0U, &item) ==
           PANEL_HOMEY_READ_NOT_FOUND);
    assert_lock_balanced(&lock);
}

static void test_not_configured(void)
{
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    const panel_homey_alias_provider_t provider = {
        .context = NULL,
        .resolve = panel_homey_alias_provider_not_configured,
    };
    const char *json =
        "{\"result\":{\"x\":{\"_id\":\"dev-alpha\","
        "\"available\":true,\"capabilitiesObj\":{"
        "\"cap-switch\":{\"value\":true}}}}}";
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 100U) ==
           PANEL_HOMEY_READ_NOT_CONFIGURED);
    panel_homey_read_snapshot_t snapshot;
    assert(panel_homey_snapshot_copy(&store, 100U, &snapshot) ==
           PANEL_HOMEY_READ_NOT_FOUND);
    assert_lock_balanced(&lock);
}

static void test_empty_inventory(void)
{
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    provider_context_t context = {0};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    assert(panel_homey_snapshot_publish_json(&store, "{}", &provider, 10U) ==
           PANEL_HOMEY_READ_OK);
    panel_homey_read_snapshot_t first = copy_ok(&store, 10U);
    assert(first.generation == 1U);
    assert(first.item_count == 0U);
    assert(panel_homey_snapshot_publish_json(
               &store, "{\"result\":{}}", &provider, 20U) ==
           PANEL_HOMEY_READ_OK);
    panel_homey_read_snapshot_t second = copy_ok(&store, 20U);
    assert(second.generation == 2U);
    assert(second.item_count == 0U);
    assert_lock_balanced(&lock);
}

static void test_invalid_result(void)
{
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    provider_context_t context = {0};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    const char *cases[] = {
        "{\"result\":null}",
        "{\"result\":\"bad\"}",
        "{\"result\":42}",
        "{\"result\":true}",
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        assert(panel_homey_snapshot_publish_json(
                   &store, cases[index], &provider, 1U) ==
               PANEL_HOMEY_READ_INVALID);
    }
    assert_lock_balanced(&lock);
}

static void test_invalid_device_entry(void)
{
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    provider_context_t context = {0};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    const char *cases[] = {
        "[\"bad\"]",
        "[42]",
        "[true]",
        "[null]",
        "[[]]",
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        assert(panel_homey_snapshot_publish_json(
                   &store, cases[index], &provider, 1U) ==
               PANEL_HOMEY_READ_INVALID);
    }
    assert_lock_balanced(&lock);
}

static void test_missing_id(void)
{
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    provider_context_t context = {0};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    const char *cases[] = {
        "[{\"available\":true,\"capabilitiesObj\":{}}]",
        "[{\"_id\":\"\",\"id\":\"\",\"capabilitiesObj\":{}}]",
        "[{\"_id\":42,\"capabilitiesObj\":{}}]",
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        assert(panel_homey_snapshot_publish_json(
                   &store, cases[index], &provider, 1U) ==
               PANEL_HOMEY_READ_INVALID);
    }
    assert_lock_balanced(&lock);
}

static void test_capabilities_missing_or_invalid(void)
{
    static const binding_t binding = {
        "dev-alpha", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK};
    provider_context_t context = {&binding, 1U, NULL, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    const char *cases[] = {
        "[{\"_id\":\"dev-alpha\",\"available\":true}]",
        "[{\"_id\":\"dev-alpha\",\"available\":true,\"capabilitiesObj\":null}]",
        "[{\"_id\":\"dev-alpha\",\"available\":true,\"capabilitiesObj\":\"bad\"}]",
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        panel_homey_snapshot_store_t store;
        lock_context_t lock;
        init_store(&store, &lock);
        assert(panel_homey_snapshot_publish_json(
                   &store, cases[index], &provider, 1U) ==
               PANEL_HOMEY_READ_OK);
        panel_homey_read_snapshot_t snapshot = copy_ok(&store, 1U);
        assert(snapshot.item_count == 0U);
        assert_lock_balanced(&lock);
    }
}

static void test_missing_capability(void)
{
    static const binding_t binding = {
        "dev-alpha", "cap-other", "lighting_1", "onoff", PANEL_HOMEY_READ_OK};
    provider_context_t context = {&binding, 1U, NULL, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    const char *json =
        "[{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":true}}}]";
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
           PANEL_HOMEY_READ_OK);
    panel_homey_read_snapshot_t snapshot = copy_ok(&store, 1U);
    assert(snapshot.item_count == 0U);
    assert_lock_balanced(&lock);
}

static void test_wrong_capability_object_type(void)
{
    static const binding_t binding = {
        "dev-alpha", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK};
    provider_context_t context = {&binding, 1U, NULL, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    const char *values[] = {"true", "\"bad\"", "42", "null"};
    for (size_t index = 0U; index < sizeof(values) / sizeof(values[0]); ++index) {
        char json[256];
        snprintf(
            json,
            sizeof(json),
            "[{\"_id\":\"dev-alpha\",\"available\":true,"
            "\"capabilitiesObj\":{\"cap-switch\":%s}}]",
            values[index]);
        panel_homey_snapshot_store_t store;
        lock_context_t lock;
        init_store(&store, &lock);
        assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
               PANEL_HOMEY_READ_OK);
        panel_homey_read_item_t item;
        assert(panel_homey_snapshot_find(
                   &store, "lighting_1", "onoff", 1U, &item) ==
               PANEL_HOMEY_READ_OK);
        assert(item.value_type == PANEL_HOMEY_VALUE_NONE);
        assert(!item.bool_value);
        assert_lock_balanced(&lock);
    }
}

static void test_invalid_alias_characters(void)
{
    const char *invalid_aliases[] = {
        "", " leading", "trailing ", "embedded space", "upperA", "dot.name",
        "slash/name", "back\\name", "line\nbreak"};
    const char *json =
        "[{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":true}}}]";
    for (size_t index = 0U;
         index < sizeof(invalid_aliases) / sizeof(invalid_aliases[0]);
         ++index) {
        binding_t binding = {
            "dev-alpha", "cap-switch", invalid_aliases[index], "onoff",
            PANEL_HOMEY_READ_OK};
        provider_context_t context = {&binding, 1U, NULL, false};
        const panel_homey_alias_provider_t provider = make_provider(&context);
        panel_homey_snapshot_store_t store;
        lock_context_t lock;
        init_store(&store, &lock);
        assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
               PANEL_HOMEY_READ_INVALID);
        assert_lock_balanced(&lock);
    }
}

static void test_alias_boundaries(void)
{
    char device_alias_ok[PANEL_HOMEY_ALIAS_MAX];
    char device_alias_too_long[PANEL_HOMEY_ALIAS_MAX + 1U];
    char capability_alias_ok[PANEL_HOMEY_CAPABILITY_ALIAS_MAX];
    char capability_alias_too_long[PANEL_HOMEY_CAPABILITY_ALIAS_MAX + 1U];
    fill_alias(device_alias_ok, PANEL_HOMEY_ALIAS_MAX - 1U, 'a');
    fill_alias(device_alias_too_long, PANEL_HOMEY_ALIAS_MAX, 'a');
    fill_alias(capability_alias_ok, PANEL_HOMEY_CAPABILITY_ALIAS_MAX - 1U, 'b');
    fill_alias(capability_alias_too_long, PANEL_HOMEY_CAPABILITY_ALIAS_MAX, 'b');
    const char *json =
        "[{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":true}}}]";

    binding_t valid = {
        "dev-alpha", "cap-switch", device_alias_ok, capability_alias_ok,
        PANEL_HOMEY_READ_OK};
    provider_context_t valid_context = {&valid, 1U, NULL, false};
    panel_homey_alias_provider_t provider = make_provider(&valid_context);
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
           PANEL_HOMEY_READ_OK);
    panel_homey_read_item_t item;
    assert(panel_homey_snapshot_find(
               &store, device_alias_ok, capability_alias_ok, 1U, &item) ==
           PANEL_HOMEY_READ_OK);
    assert_lock_balanced(&lock);

    binding_t long_device = {
        "dev-alpha", "cap-switch", device_alias_too_long, "onoff",
        PANEL_HOMEY_READ_OK};
    provider_context_t long_device_context = {&long_device, 1U, NULL, false};
    provider = make_provider(&long_device_context);
    init_store(&store, &lock);
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
           PANEL_HOMEY_READ_OVERFLOW);
    assert_lock_balanced(&lock);

    binding_t long_capability = {
        "dev-alpha", "cap-switch", "lighting_1", capability_alias_too_long,
        PANEL_HOMEY_READ_OK};
    provider_context_t long_capability_context = {
        &long_capability, 1U, NULL, false};
    provider = make_provider(&long_capability_context);
    init_store(&store, &lock);
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
           PANEL_HOMEY_READ_OVERFLOW);
    assert_lock_balanced(&lock);
}

static void test_item_limit_16(void)
{
    binding_t bindings[PANEL_HOMEY_SNAPSHOT_MAX_ITEMS];
    char raw_ids[PANEL_HOMEY_SNAPSHOT_MAX_ITEMS][16];
    char device_aliases[PANEL_HOMEY_SNAPSHOT_MAX_ITEMS][PANEL_HOMEY_ALIAS_MAX];
    char capability_aliases[PANEL_HOMEY_SNAPSHOT_MAX_ITEMS]
                           [PANEL_HOMEY_CAPABILITY_ALIAS_MAX];
    make_many_bindings(
        bindings,
        raw_ids,
        device_aliases,
        capability_aliases,
        PANEL_HOMEY_SNAPSHOT_MAX_ITEMS);
    provider_context_t context = {
        bindings, PANEL_HOMEY_SNAPSHOT_MAX_ITEMS, NULL, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    char *json = make_inventory(PANEL_HOMEY_SNAPSHOT_MAX_ITEMS);
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 50U) ==
           PANEL_HOMEY_READ_OK);
    panel_homey_read_snapshot_t snapshot = copy_ok(&store, 50U);
    assert(snapshot.item_count == PANEL_HOMEY_SNAPSHOT_MAX_ITEMS);
    assert(snapshot.generation == 1U);
    assert_lock_balanced(&lock);
    free(json);
}

static void test_item_overflow_17(void)
{
    enum { ITEM_COUNT = PANEL_HOMEY_SNAPSHOT_MAX_ITEMS + 1U };
    binding_t bindings[ITEM_COUNT];
    char raw_ids[ITEM_COUNT][16];
    char device_aliases[ITEM_COUNT][PANEL_HOMEY_ALIAS_MAX];
    char capability_aliases[ITEM_COUNT][PANEL_HOMEY_CAPABILITY_ALIAS_MAX];
    make_many_bindings(
        bindings, raw_ids, device_aliases, capability_aliases, ITEM_COUNT);
    provider_context_t context = {bindings, ITEM_COUNT, NULL, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    char *json = make_inventory(ITEM_COUNT);
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 50U) ==
           PANEL_HOMEY_READ_OVERFLOW);
    panel_homey_read_snapshot_t snapshot;
    assert(panel_homey_snapshot_copy(&store, 50U, &snapshot) ==
           PANEL_HOMEY_READ_NOT_FOUND);
    assert_lock_balanced(&lock);
    free(json);
}

static void test_provider_errors(void)
{
    const panel_homey_read_result_t errors[] = {
        PANEL_HOMEY_READ_NOT_CONFIGURED,
        PANEL_HOMEY_READ_DUPLICATE,
        PANEL_HOMEY_READ_INVALID,
        PANEL_HOMEY_READ_OVERFLOW,
    };
    const char *json =
        "[{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":true}}}]";
    for (size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); ++index) {
        binding_t binding = {
            "dev-alpha", "cap-switch", "lighting_1", "onoff", errors[index]};
        provider_context_t context = {&binding, 1U, NULL, false};
        const panel_homey_alias_provider_t provider = make_provider(&context);
        panel_homey_snapshot_store_t store;
        lock_context_t lock;
        init_store(&store, &lock);
        assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
               errors[index]);
        panel_homey_read_snapshot_t snapshot;
        assert(panel_homey_snapshot_copy(&store, 1U, &snapshot) ==
               PANEL_HOMEY_READ_NOT_FOUND);
        assert_lock_balanced(&lock);
    }
}

static void test_publication_preserves_previous(void)
{
    static const binding_t valid_binding = {
        "dev-alpha", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK};
    provider_context_t valid_context = {&valid_binding, 1U, NULL, false};
    panel_homey_alias_provider_t provider = make_provider(&valid_context);
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    const char *valid_json =
        "[{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":false}}}]";
    assert(panel_homey_snapshot_publish_json(
               &store, valid_json, &provider, 10U) ==
           PANEL_HOMEY_READ_OK);
    const panel_homey_read_snapshot_t before = copy_ok(&store, 10U);

    assert(panel_homey_snapshot_publish_json(&store, "{", &provider, 20U) ==
           PANEL_HOMEY_READ_INVALID);
    panel_homey_read_snapshot_t after = copy_ok(&store, 20U);
    assert_snapshot_equal(&before, &after);

    assert(panel_homey_snapshot_publish_json(
               &store, "{\"result\":null}", &provider, 20U) ==
           PANEL_HOMEY_READ_INVALID);
    after = copy_ok(&store, 20U);
    assert_snapshot_equal(&before, &after);

    binding_t invalid_binding = {
        "dev-alpha", "cap-switch", "BadAlias", "onoff", PANEL_HOMEY_READ_OK};
    provider_context_t invalid_context = {&invalid_binding, 1U, NULL, false};
    provider = make_provider(&invalid_context);
    assert(panel_homey_snapshot_publish_json(
               &store, valid_json, &provider, 20U) ==
           PANEL_HOMEY_READ_INVALID);
    after = copy_ok(&store, 20U);
    assert_snapshot_equal(&before, &after);

    binding_t error_binding = {
        "dev-alpha", "cap-switch", "lighting_1", "onoff",
        PANEL_HOMEY_READ_OVERFLOW};
    provider_context_t error_context = {&error_binding, 1U, NULL, false};
    provider = make_provider(&error_context);
    assert(panel_homey_snapshot_publish_json(
               &store, valid_json, &provider, 20U) ==
           PANEL_HOMEY_READ_OVERFLOW);
    after = copy_ok(&store, 20U);
    assert_snapshot_equal(&before, &after);
    assert_lock_balanced(&lock);
}

static void test_lookup_not_found(void)
{
    static const binding_t binding = {
        "dev-alpha", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK};
    provider_context_t context = {&binding, 1U, NULL, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    const char *json =
        "[{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":true}}}]";
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
           PANEL_HOMEY_READ_OK);
    panel_homey_read_item_t item;
    assert(panel_homey_snapshot_find(
               &store, "missing", "onoff", 1U, &item) ==
           PANEL_HOMEY_READ_NOT_FOUND);
    assert(panel_homey_snapshot_find(
               &store, "lighting_1", "missing", 1U, &item) ==
           PANEL_HOMEY_READ_NOT_FOUND);
    assert(panel_homey_snapshot_find(
               &store, "LIGHTING_1", "onoff", 1U, &item) ==
           PANEL_HOMEY_READ_NOT_FOUND);
    assert_lock_balanced(&lock);
}

static void test_generation_progression(void)
{
    static const binding_t binding = {
        "dev-alpha", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK};
    provider_context_t context = {&binding, 1U, NULL, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    const char *json =
        "[{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":true}}}]";
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 10U) ==
           PANEL_HOMEY_READ_OK);
    assert(copy_ok(&store, 10U).generation == 1U);
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 20U) ==
           PANEL_HOMEY_READ_OK);
    assert(copy_ok(&store, 20U).generation == 2U);
    assert(panel_homey_snapshot_publish_json(&store, "{", &provider, 30U) ==
           PANEL_HOMEY_READ_INVALID);
    assert(copy_ok(&store, 30U).generation == 2U);
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 40U) ==
           PANEL_HOMEY_READ_OK);
    assert(copy_ok(&store, 40U).generation == 3U);
    assert_lock_balanced(&lock);
}

static void test_publish_copy_find_stale_and_privacy(void)
{
    static const binding_t bindings[] = {
        {"dev-alpha", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK},
        {"dev-beta", "cap-switch", "lighting_2", "onoff", PANEL_HOMEY_READ_OK},
    };
    lock_context_t lock;
    panel_homey_snapshot_store_t store;
    init_store(&store, &lock);
    provider_context_t context = {
        bindings, sizeof(bindings) / sizeof(bindings[0]), &lock, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    const char *json =
        "{\"result\":{"
        "\"a\":{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":true}}},"
        "\"b\":{\"id\":\"dev-beta\",\"available\":false,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":\"unknown\"}}}}}";
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 100U) ==
           PANEL_HOMEY_READ_OK);
    assert(!context.called_while_locked);
    panel_homey_read_snapshot_t snapshot = copy_ok(&store, 100U);
    assert(snapshot.generation == 1U);
    assert(snapshot.item_count == 2U);
    assert(!memory_contains(&snapshot, sizeof(snapshot), "dev-alpha"));
    assert(!memory_contains(&snapshot, sizeof(snapshot), "dev-beta"));
    assert(!memory_contains(&snapshot, sizeof(snapshot), "cap-switch"));
    panel_homey_read_item_t item;
    assert(panel_homey_snapshot_find(
               &store, "lighting_1", "onoff", 100U, &item) ==
           PANEL_HOMEY_READ_OK);
    assert(item.available);
    assert(item.value_type == PANEL_HOMEY_VALUE_BOOL);
    assert(item.bool_value);
    assert(panel_homey_snapshot_find(
               &store, "lighting_2", "onoff", 100U, &item) ==
           PANEL_HOMEY_READ_OK);
    assert(!item.available);
    assert(item.value_type == PANEL_HOMEY_VALUE_NONE);
    assert(!item.bool_value);
    assert(panel_homey_snapshot_copy(
               &store, 100U + PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS, &snapshot) ==
           PANEL_HOMEY_READ_OK);
    assert(panel_homey_snapshot_copy(
               &store,
               101U + PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS,
               &snapshot) == PANEL_HOMEY_READ_STALE);
    assert(panel_homey_snapshot_copy(&store, 99U, &snapshot) ==
           PANEL_HOMEY_READ_STALE);
    assert_lock_balanced(&lock);
}

static void test_duplicate_alias_rejected(void)
{
    static const binding_t bindings[] = {
        {"dev-alpha", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK},
        {"dev-beta", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK},
    };
    provider_context_t context = {
        bindings, sizeof(bindings) / sizeof(bindings[0]), NULL, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    const char *json =
        "{\"result\":{"
        "\"a\":{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":true}}},"
        "\"b\":{\"_id\":\"dev-beta\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":false}}}}}";
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
           PANEL_HOMEY_READ_DUPLICATE);
    assert_lock_balanced(&lock);
}

static void test_missing_available_is_fail_closed(void)
{
    static const binding_t binding = {
        "dev-alpha", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK};
    provider_context_t context = {&binding, 1U, NULL, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    panel_homey_snapshot_store_t store;
    lock_context_t lock;
    init_store(&store, &lock);
    const char *json =
        "[{\"_id\":\"dev-alpha\",\"capabilitiesObj\":{"
        "\"cap-switch\":{\"value\":true}}}]";
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
           PANEL_HOMEY_READ_OK);
    panel_homey_read_item_t item;
    assert(panel_homey_snapshot_find(
               &store, "lighting_1", "onoff", 1U, &item) ==
           PANEL_HOMEY_READ_OK);
    assert(!item.available);
    assert_lock_balanced(&lock);
}

static void test_lock_balance(void)
{
    static const binding_t binding = {
        "dev-alpha", "cap-switch", "lighting_1", "onoff", PANEL_HOMEY_READ_OK};
    lock_context_t lock;
    panel_homey_snapshot_store_t store;
    init_store(&store, &lock);
    provider_context_t context = {&binding, 1U, &lock, false};
    const panel_homey_alias_provider_t provider = make_provider(&context);
    const char *json =
        "[{\"_id\":\"dev-alpha\",\"available\":true,"
        "\"capabilitiesObj\":{\"cap-switch\":{\"value\":true}}}]";
    assert(panel_homey_snapshot_publish_json(&store, json, &provider, 1U) ==
           PANEL_HOMEY_READ_OK);
    assert(!context.called_while_locked);
    panel_homey_read_snapshot_t snapshot;
    assert(panel_homey_snapshot_copy(&store, 1U, &snapshot) ==
           PANEL_HOMEY_READ_OK);
    panel_homey_read_item_t item;
    assert(panel_homey_snapshot_find(
               &store, "lighting_1", "onoff", 1U, &item) ==
           PANEL_HOMEY_READ_OK);
    const unsigned lock_calls_before_failure = lock.lock_calls;
    assert(panel_homey_snapshot_publish_json(&store, "{", &provider, 2U) ==
           PANEL_HOMEY_READ_INVALID);
    assert(lock.lock_calls == lock_calls_before_failure);
    assert_lock_balanced(&lock);
    assert(lock.max_lock_depth == 1U);
}

#define RUN_TEST(test_name) \
    do { \
        test_name(); \
        puts(#test_name " PASS"); \
    } while (0)

int main(void)
{
    RUN_TEST(test_null_arguments);
    RUN_TEST(test_copy_before_publication);
    RUN_TEST(test_not_configured);
    RUN_TEST(test_empty_inventory);
    RUN_TEST(test_invalid_result);
    RUN_TEST(test_invalid_device_entry);
    RUN_TEST(test_missing_id);
    RUN_TEST(test_capabilities_missing_or_invalid);
    RUN_TEST(test_missing_capability);
    RUN_TEST(test_wrong_capability_object_type);
    RUN_TEST(test_invalid_alias_characters);
    RUN_TEST(test_alias_boundaries);
    RUN_TEST(test_item_limit_16);
    RUN_TEST(test_item_overflow_17);
    RUN_TEST(test_provider_errors);
    RUN_TEST(test_publication_preserves_previous);
    RUN_TEST(test_lookup_not_found);
    RUN_TEST(test_generation_progression);
    RUN_TEST(test_publish_copy_find_stale_and_privacy);
    RUN_TEST(test_duplicate_alias_rejected);
    RUN_TEST(test_missing_available_is_fail_closed);
    RUN_TEST(test_lock_balance);
    puts("PANEL_HOMEY_READ_SNAPSHOT_HOST_TEST PASS");
    return 0;
}
