#pragma once

#include <stddef.h>

#define PANEL_HOMEY_ALIAS_MAX 48U
#define PANEL_HOMEY_CAPABILITY_ALIAS_MAX 32U

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

typedef struct {
    void *context;
    panel_homey_alias_resolve_fn resolve;
} panel_homey_alias_provider_t;

panel_homey_read_result_t panel_homey_alias_provider_not_configured(
    void *context,
    const char *raw_device_id,
    const char *raw_capability_id,
    char *device_alias_out,
    size_t device_alias_capacity,
    char *capability_alias_out,
    size_t capability_alias_capacity);
