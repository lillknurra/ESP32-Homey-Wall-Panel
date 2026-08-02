#pragma once

#include "panel_homey_alias_provider.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_HOMEY_SNAPSHOT_MAX_ITEMS 16U
#define PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS 120000ULL

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

typedef void (*panel_homey_snapshot_lock_fn)(void *context);

typedef struct {
    uint32_t generation;
    uint64_t captured_at_ms;
    size_t item_count;
    panel_homey_read_item_t items[PANEL_HOMEY_SNAPSHOT_MAX_ITEMS];
} panel_homey_read_snapshot_t;

typedef struct {
    panel_homey_read_snapshot_t buffers[2];
    uint8_t active_index;
    bool active_valid;
    void *lock_context;
    panel_homey_snapshot_lock_fn lock;
    panel_homey_snapshot_lock_fn unlock;
} panel_homey_snapshot_store_t;

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

panel_homey_read_result_t panel_homey_snapshot_find(
    const panel_homey_snapshot_store_t *store,
    const char *device_alias,
    const char *capability_alias,
    uint64_t now_ms,
    panel_homey_read_item_t *out);
