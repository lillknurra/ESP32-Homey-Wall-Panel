#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED = 0,
    ATHOM_LIGHT_TOGGLE_DISPATCH_INVALID_WIDGET,
    ATHOM_LIGHT_TOGGLE_DISPATCH_NOT_READY,
    ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_NOT_FOUND,
    ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_INVALID,
    ATHOM_LIGHT_TOGGLE_DISPATCH_UNAUTHORIZED,
    ATHOM_LIGHT_TOGGLE_DISPATCH_REJECTED,
    ATHOM_LIGHT_TOGGLE_DISPATCH_TRANSPORT_AMBIGUOUS,
    ATHOM_LIGHT_TOGGLE_DISPATCH_INTERNAL_ERROR,
} athom_light_toggle_dispatch_result_t;

typedef enum {
    ATHOM_LIGHT_TOGGLE_QUEUE_QUEUED = 0,
    ATHOM_LIGHT_TOGGLE_QUEUE_INVALID_WIDGET,
    ATHOM_LIGHT_TOGGLE_QUEUE_NOT_READY,
    ATHOM_LIGHT_TOGGLE_QUEUE_BUSY,
    ATHOM_LIGHT_TOGGLE_QUEUE_FAILED,
} athom_light_toggle_queue_result_t;

#ifdef ESP_PLATFORM
#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t athom_oauth_runtime_register_handlers(httpd_handle_t server);
esp_err_t athom_oauth_runtime_on_wifi_online(void);

typedef enum {
    ATHOM_HOMEY_DATA_LOADING = 0,
    ATHOM_HOMEY_DATA_RETRYING,
    ATHOM_HOMEY_DATA_READY,
    ATHOM_HOMEY_DATA_ERROR,
} athom_homey_data_state_t;

athom_homey_data_state_t athom_oauth_runtime_homey_data_state(void);
const char *athom_oauth_runtime_homey_data_state_name(void);

esp_err_t athom_oauth_runtime_get_selected_homey_id(char *out, size_t capacity);

/*
 * Future UI-facing light dispatch surface. Only Favorites widgets 4 and 5
 * are accepted. The Patch036 execution-readiness gate is checked immediately
 * before the fixed Homey mutation primitive is called.
 */
athom_light_toggle_dispatch_result_t athom_oauth_runtime_dispatch_light_toggle(
    size_t widget_index,
    bool value);

/*
 * Patch038 UI-facing asynchronous surface. Queueing is bounded to Favorites
 * widgets 4/5 and never performs Homey I/O on the caller/LVGL task. The worker
 * rechecks Patch037 execution readiness immediately before the write.
 */
athom_light_toggle_queue_result_t athom_oauth_runtime_queue_light_toggle(
    size_t widget_index,
    bool value);
bool athom_oauth_runtime_light_toggle_pending(size_t widget_index);
uint32_t athom_oauth_runtime_light_toggle_completion_generation(void);

#endif
