#pragma once

#include "panel_homey_alias_provider.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_HOMEY_SNAPSHOT_MAX_ITEMS 16U
#define PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS 120000ULL
#define PANEL_HOMEY_AWNING_ROLE_COUNT 3U

typedef enum {
    PANEL_HOMEY_VALUE_NONE = 0,
    PANEL_HOMEY_VALUE_BOOL,
} panel_homey_value_type_t;

typedef struct {
    char device_alias[PANEL_HOMEY_ALIAS_MAX];
    char capability_alias[PANEL_HOMEY_CAPABILITY_ALIAS_MAX];
    bool available;
    panel_homey_value_type_t value_type;
    bool bool_value;
} panel_homey_read_item_t;

typedef enum {
    PANEL_HOMEY_MATCH_VALUE_UNKNOWN = 0,
    PANEL_HOMEY_MATCH_VALUE_FALSE,
    PANEL_HOMEY_MATCH_VALUE_TRUE,
} panel_homey_match_value_t;

typedef struct {
    panel_homey_match_value_t binding_entry_present;
    panel_homey_match_value_t device_present;
    panel_homey_match_value_t capability_present;
    panel_homey_match_value_t matched;
} panel_homey_awning_match_stages_t;

typedef void (*panel_homey_snapshot_lock_fn)(void *context);

typedef struct {
    uint32_t generation;
    uint64_t captured_at_ms;
    size_t item_count;
    panel_homey_read_item_t items[PANEL_HOMEY_SNAPSHOT_MAX_ITEMS];
    panel_homey_awning_match_stages_t awning_match_stages[PANEL_HOMEY_AWNING_ROLE_COUNT];
} panel_homey_read_snapshot_t;

typedef struct {
    bool attempted;
    panel_homey_read_result_t result;
    uint64_t attempted_at_ms;
} panel_homey_snapshot_publish_state_t;

typedef struct {
    bool attempted;
    panel_homey_read_result_t result;
    bool age_valid;
    uint64_t age_ms;
} panel_homey_snapshot_publish_inspection_t;

typedef struct {
    panel_homey_read_snapshot_t buffers[2];
    uint8_t active_index;
    bool active_valid;
    panel_homey_snapshot_publish_state_t last_publish;
    void *lock_context;
    panel_homey_snapshot_lock_fn lock;
    panel_homey_snapshot_lock_fn unlock;
} panel_homey_snapshot_store_t;

typedef struct {
    panel_homey_read_result_t result;
    bool present;
    bool fresh;
    uint64_t age_ms;
    panel_homey_read_snapshot_t snapshot;
} panel_homey_snapshot_inspection_t;

void panel_homey_snapshot_store_init(
    panel_homey_snapshot_store_t *store,
    void *lock_context,
    panel_homey_snapshot_lock_fn lock,
    panel_homey_snapshot_lock_fn unlock);

panel_homey_read_result_t panel_homey_snapshot_publish_json(
    panel_homey_snapshot_store_t *store,
    const char *device_json,
    const panel_homey_alias_provider_t *provider,
    uint64_t now_ms);

panel_homey_read_result_t panel_homey_snapshot_copy(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms,
    panel_homey_read_snapshot_t *out);

/* Passive diagnostic copy. Unlike snapshot_copy, this preserves stale data
 * and its metadata for inspection; it never changes the store. */
panel_homey_read_result_t panel_homey_snapshot_inspect(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms,
    panel_homey_snapshot_inspection_t *out);

/* Atomically copies the active snapshot and its latest publish-attempt state. */
panel_homey_read_result_t panel_homey_snapshot_inspect_with_publish(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms,
    panel_homey_snapshot_inspection_t *snapshot_out,
    panel_homey_snapshot_publish_inspection_t *publish_out);

/* Passive copy of the most recent publication attempt and its age. */
void panel_homey_snapshot_publish_inspect(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms,
    panel_homey_snapshot_publish_inspection_t *out);

panel_homey_read_result_t panel_homey_snapshot_find(
    const panel_homey_snapshot_store_t *store,
    const char *device_alias,
    const char *capability_alias,
    uint64_t now_ms,
    panel_homey_read_item_t *out);
