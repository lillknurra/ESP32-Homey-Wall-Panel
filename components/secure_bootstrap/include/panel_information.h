#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "panel_homey_read_snapshot.h"

typedef enum {
    PANEL_INFORMATION_WIFI = 0,
    PANEL_INFORMATION_HOMEY,
    PANEL_INFORMATION_FIRMWARE,
    PANEL_INFORMATION_TIME,
    PANEL_INFORMATION_SYSTEM,
    PANEL_INFORMATION_RGB,
    PANEL_INFORMATION_COUNT
} panel_information_kind_t;

/* Allowlisted, volatile information only. No identifiers or credential fields. */
typedef struct {
    bool observed;
    panel_homey_read_result_t result;
    uint64_t captured_at_ms;
    uint32_t generation;
} panel_information_snapshot_t;

typedef struct {
    uint64_t observed_at_us;
    uint64_t last_deficit_us, last_restart_us;
    uint32_t eof, vsync, shortfall, startup_shortfall;
    uint32_t restart_decisions, restart_paths, last_actual, expected;
    uint32_t vsync_min_us, vsync_max_us, bounce_max_us;
    uint32_t alloc_failures, dropped_events;
} panel_information_rgb_t;

typedef struct {
    uint64_t now_ms;
    bool wifi_known, wifi_online;
    bool provisioning_known, provisioning_active;
    bool homey_known;
    unsigned homey_state; /* 0 loading, 1 retrying, 2 ready, 3 error */
    panel_information_snapshot_t snapshot;
    bool firmware_known;
    char version[32], build_date[16], build_time[16], idf_version[32];
    bool time_known, time_synchronized, swedish_timezone;
    bool memory_known;
    uint32_t internal_free, internal_minimum, psram_free;
    bool lvgl_memory_known;
    uint32_t lvgl_pool_size, lvgl_current_used, lvgl_max_used;
    bool rgb_known;
    panel_information_rgb_t rgb;
} panel_information_t;

/* Call under the existing display lock, after the normal dashboard poll. */
void panel_information_observe_snapshot(panel_information_snapshot_t *out,
    panel_homey_read_result_t result, uint64_t captured_at_ms, uint32_t generation);
const char *panel_information_title(panel_information_kind_t kind);
bool panel_information_format(panel_information_kind_t kind,
    const panel_information_t *info, char *out, size_t capacity);
