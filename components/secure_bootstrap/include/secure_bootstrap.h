#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SECURE_BOOTSTRAP_CODE_LEN 12
#define SECURE_BOOTSTRAP_CODE_TTL_SECONDS 600
#define SECURE_BOOTSTRAP_WIPE_HOLD_MS 5000

typedef enum {
    SECURE_BOOTSTRAP_OK = 0,
    SECURE_BOOTSTRAP_ERR_ARGUMENT,
    SECURE_BOOTSTRAP_ERR_EXPIRED,
    SECURE_BOOTSTRAP_ERR_MISMATCH,
    SECURE_BOOTSTRAP_ERR_CONSUMED,
    SECURE_BOOTSTRAP_ERR_PLATFORM,
} secure_bootstrap_status_t;

typedef struct {
    char code[SECURE_BOOTSTRAP_CODE_LEN + 1];
    int64_t issued_at_s;
    bool consumed;
} secure_bootstrap_code_t;

typedef struct {
    int64_t pressed_since_ms;
    bool triggered;
} secure_bootstrap_wipe_tracker_t;

typedef struct {
    bool display_ready;
    bool touch_ready;
    bool softap_ready;
    bool wifi_configured;
    bool ip_obtained;
    bool oauth_locked;
} secure_bootstrap_status_snapshot_t;

secure_bootstrap_status_t secure_bootstrap_code_generate(
    secure_bootstrap_code_t *state,
    int64_t now_s,
    const uint8_t *random_bytes,
    size_t random_len);
secure_bootstrap_status_t secure_bootstrap_code_verify_and_consume(
    secure_bootstrap_code_t *state,
    const char *candidate,
    int64_t now_s);
bool secure_bootstrap_code_rotation_due(
    const secure_bootstrap_code_t *state,
    int64_t now_s);
void secure_bootstrap_code_wipe(secure_bootstrap_code_t *state);
bool secure_bootstrap_text_contains_code(
    const char *text,
    const secure_bootstrap_code_t *state);
void secure_bootstrap_wipe_tracker_reset(secure_bootstrap_wipe_tracker_t *tracker);
bool secure_bootstrap_wipe_tracker_update(
    secure_bootstrap_wipe_tracker_t *tracker,
    bool pressed,
    int64_t now_ms);

#ifdef ESP_PLATFORM
secure_bootstrap_status_t secure_bootstrap_start(void);
#endif
