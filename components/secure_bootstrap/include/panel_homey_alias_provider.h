#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_HOMEY_ALIAS_MAX 48U
#define PANEL_HOMEY_CAPABILITY_ALIAS_MAX 32U
#define PANEL_HOMEY_ALIAS_SNAPSHOT_MAX_ENTRIES 6U
#define PANEL_HOMEY_RAW_DEVICE_ID_MAX 128U
#define PANEL_HOMEY_RAW_CAPABILITY_ID_MAX 64U

typedef enum {
    PANEL_HOMEY_READ_OK = 0,
    PANEL_HOMEY_READ_NOT_CONFIGURED,
    PANEL_HOMEY_READ_NOT_FOUND,
    PANEL_HOMEY_READ_DUPLICATE,
    PANEL_HOMEY_READ_INVALID,
    PANEL_HOMEY_READ_OVERFLOW,
    PANEL_HOMEY_READ_STALE,
} panel_homey_read_result_t;

typedef panel_homey_read_result_t (*panel_homey_alias_resolve_fn)(
    void *context,
    const char *raw_device_id,
    const char *raw_capability_id,
    char *device_alias_out,
    size_t device_alias_capacity,
    char *capability_alias_out,
    size_t capability_alias_capacity);

/* A private, bounded copy of one active alias-record epoch. Raw identifiers
 * are transient parser inputs and must never be published or logged. */
typedef struct {
    uint8_t dashboard_binding_index;
    char raw_device_id[PANEL_HOMEY_RAW_DEVICE_ID_MAX];
    char raw_capability_id[PANEL_HOMEY_RAW_CAPABILITY_ID_MAX];
    char device_alias[PANEL_HOMEY_ALIAS_MAX];
    char capability_alias[PANEL_HOMEY_CAPABILITY_ALIAS_MAX];
} panel_homey_alias_snapshot_entry_t;

typedef struct {
    bool configured;
    uint32_t generation;
    size_t entry_count;
    panel_homey_alias_snapshot_entry_t entries[
        PANEL_HOMEY_ALIAS_SNAPSHOT_MAX_ENTRIES];
} panel_homey_alias_snapshot_t;

typedef panel_homey_read_result_t (*panel_homey_alias_capture_fn)(
    void *context,
    panel_homey_alias_snapshot_t *snapshot_out);

typedef struct {
    void *context;
    panel_homey_alias_resolve_fn resolve;
    panel_homey_alias_capture_fn capture;
} panel_homey_alias_provider_t;

panel_homey_read_result_t panel_homey_alias_provider_not_configured(
    void *context,
    const char *raw_device_id,
    const char *raw_capability_id,
    char *device_alias_out,
    size_t device_alias_capacity,
    char *capability_alias_out,
    size_t capability_alias_capacity);
