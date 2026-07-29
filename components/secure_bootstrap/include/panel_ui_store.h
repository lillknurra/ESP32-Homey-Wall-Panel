#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "panel_ui_model.h"

#define PANEL_UI_STORE_NAMESPACE "hpanel_ui"
#define PANEL_UI_STORE_KEY_SLOT_A "cfg_a"
#define PANEL_UI_STORE_KEY_SLOT_B "cfg_b"
#define PANEL_UI_STORE_KEY_ACTIVE "active"
#define PANEL_UI_STORE_MAGIC UINT32_C(0x48505549)
#define PANEL_UI_STORE_SCHEMA_VERSION UINT16_C(1)
#define PANEL_UI_STORE_RECORD_SIZE 32U

typedef enum {
    PANEL_UI_STORE_SLOT_A = 0,
    PANEL_UI_STORE_SLOT_B = 1,
    PANEL_UI_STORE_SLOT_NONE = 255,
} panel_ui_store_slot_t;

typedef enum {
    PANEL_UI_STORE_RESULT_OK = 0,
    PANEL_UI_STORE_RESULT_DEFAULTED_NOT_FOUND,
    PANEL_UI_STORE_RESULT_DEFAULTED_INVALID,
    PANEL_UI_STORE_RESULT_INVALID_ARGUMENT,
    PANEL_UI_STORE_RESULT_IO_ERROR,
    PANEL_UI_STORE_RESULT_VERIFY_ERROR,
} panel_ui_store_result_t;

typedef struct {
    bool valid;
    panel_ui_store_slot_t slot;
    uint32_t generation;
    panel_ui_settings_t settings;
} panel_ui_store_candidate_t;

bool panel_ui_store_crc32(const uint8_t *data, size_t data_size, uint32_t *crc_out);
bool panel_ui_store_encode(const panel_ui_settings_t *settings,
                           uint32_t generation,
                           uint8_t *record,
                           size_t record_size);
bool panel_ui_store_decode(const uint8_t *record,
                           size_t record_size,
                           panel_ui_settings_t *settings_out,
                           uint32_t *generation_out);
panel_ui_store_slot_t panel_ui_store_select_slot(const panel_ui_store_candidate_t *slot_a,
                                                 const panel_ui_store_candidate_t *slot_b,
                                                 panel_ui_store_slot_t active_hint);

#ifdef ESP_PLATFORM
panel_ui_store_result_t panel_ui_store_load(panel_ui_settings_t *settings_out);
panel_ui_store_result_t panel_ui_store_save(const panel_ui_settings_t *settings);
#endif
