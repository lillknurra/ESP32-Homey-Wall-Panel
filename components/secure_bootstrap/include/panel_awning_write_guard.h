#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define PANEL_AWNING_GUARD_IDENTITY_SIZE 32U

typedef enum {
    PANEL_AWNING_GUARD_UNINITIALIZED = 0,
    PANEL_AWNING_GUARD_READY,
    PANEL_AWNING_GUARD_WRITE_PENDING,
    PANEL_AWNING_GUARD_QUARANTINED,
} panel_awning_guard_state_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint8_t state;
    uint8_t reserved[3];
    uint32_t sequence;
    uint8_t homey_identity[PANEL_AWNING_GUARD_IDENTITY_SIZE];
    uint32_t alias_generation;
    uint32_t crc32;
} panel_awning_guard_record_t;

/* Missing storage is reported as ESP_ERR_NOT_FOUND. It never means READY. */
esp_err_t panel_awning_write_guard_load(panel_awning_guard_record_t *out);

/* First commissioning only: create READY sequence 1 when and only when the
 * dedicated guard record is genuinely absent. This never replaces an existing,
 * corrupt, unsupported, or unreadable record. Caller must serialize service
 * authorization and revalidate the Homey binding around this operation. */
esp_err_t panel_awning_write_guard_initialize_absent(
    const uint8_t homey_identity[PANEL_AWNING_GUARD_IDENTITY_SIZE],
    uint32_t alias_generation,
    panel_awning_guard_record_t *ready_out);

/* Persist and read back WRITE_PENDING, requiring the exact current READY record. */
esp_err_t panel_awning_write_guard_begin(
    const panel_awning_guard_record_t *expected_ready,
    panel_awning_guard_record_t *pending_out);

/* Persist and read back READY, requiring the exact current pending record. */
esp_err_t panel_awning_write_guard_resolve(
    const panel_awning_guard_record_t *expected_pending,
    panel_awning_guard_record_t *ready_out);

bool panel_awning_write_guard_record_valid(
    const panel_awning_guard_record_t *record);
