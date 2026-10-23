#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PANEL_STARTUP_TITLE_DIAG_MAX_TRANSITIONS 12U

typedef enum {
    PANEL_STARTUP_TITLE_DIAG_UNKNOWN = 0,
    PANEL_STARTUP_TITLE_DIAG_WIFI_CONNECTING,
    PANEL_STARTUP_TITLE_DIAG_WIFI_DISCONNECTED,
    PANEL_STARTUP_TITLE_DIAG_WIFI_CONNECTED,
    PANEL_STARTUP_TITLE_DIAG_HOMEY_LOADING,
    PANEL_STARTUP_TITLE_DIAG_HOMEY_LONG_WAIT,
    PANEL_STARTUP_TITLE_DIAG_HOMEY_READY,
    PANEL_STARTUP_TITLE_DIAG_HOMEY_ERROR,
} panel_startup_title_diag_id_t;

typedef struct {
    uint8_t transition_count;
    panel_startup_title_diag_id_t last_id;
    bool has_last_id;
} panel_startup_title_diag_log_gate_t;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} panel_startup_title_diag_rect_t;

panel_startup_title_diag_id_t panel_startup_title_diag_classify(
    const char *title,
    const char *detail);
const char *panel_startup_title_diag_id_name(panel_startup_title_diag_id_t id);
bool panel_startup_title_diag_log_transition(
    panel_startup_title_diag_log_gate_t *gate,
    panel_startup_title_diag_id_t id);
bool panel_startup_title_diag_clip_rect(
    const panel_startup_title_diag_rect_t *input,
    int32_t screen_width,
    int32_t screen_height,
    panel_startup_title_diag_rect_t *output);
bool panel_startup_title_diag_intersects_flush(
    const panel_startup_title_diag_rect_t *rect,
    int32_t x1,
    int32_t y1,
    int32_t x2,
    int32_t y2);
