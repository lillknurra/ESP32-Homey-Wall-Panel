/* Patch116I2: default-disabled, single persistent FreeRTOS read-only worker.
 * No app startup or Wi-Fi call site. Not an installer, no OTA writes.
 */
#pragma once
#include "panel_update_worker_policy.h"
#include "panel_update_manifest.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PANEL_UPDATE_POLL_DISABLED = 0,
    PANEL_UPDATE_POLL_IDLE,
    PANEL_UPDATE_POLL_BUSY,
    PANEL_UPDATE_POLL_QUARANTINED,
    PANEL_UPDATE_POLL_RESULT_OK,
    PANEL_UPDATE_POLL_RESULT_ERROR,
} panel_update_poll_result_t;

/* All functions except worker internals must be invoked on one owning task.
 * bootstrap creates ONE persistent low-priority task, never a task per request.
 * No task is force-deleted if ESP-IDF DNS/TLS/HTTP hangs indefinitely.
 */
bool panel_update_worker_bootstrap(void);
bool panel_update_worker_request(uint32_t generation, const char *channel,
    bool wifi_ready, bool normal_boot_settled);
/* Copies only a complete, timely result. No firmware installation permission. */
panel_update_poll_result_t panel_update_worker_poll(uint32_t *generation,
    char *body, size_t capacity, size_t *body_length);
