#pragma once
#include "panel_homey_alias_provider.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_HOMEY_ALIAS_STORE_NAMESPACE "homey_alias_v1"
#define PANEL_HOMEY_ALIAS_STORE_SCHEMA_VERSION 1U
#define PANEL_HOMEY_ALIAS_STORE_MAX_ENTRIES 6U
#define PANEL_HOMEY_IDENTITY_DIGEST_SIZE 32U

typedef enum { PANEL_HOMEY_ALIAS_SLOT_NONE=0, PANEL_HOMEY_ALIAS_SLOT_A=1, PANEL_HOMEY_ALIAS_SLOT_B=2 } panel_homey_alias_slot_t;
typedef enum { PANEL_HOMEY_ALIAS_STORE_OK=0, PANEL_HOMEY_ALIAS_STORE_NOT_FOUND, PANEL_HOMEY_ALIAS_STORE_NOT_CONFIGURED, PANEL_HOMEY_ALIAS_STORE_INVALID, PANEL_HOMEY_ALIAS_STORE_IO_ERROR, PANEL_HOMEY_ALIAS_STORE_VERIFY_ERROR, PANEL_HOMEY_ALIAS_STORE_UNSTABLE } panel_homey_alias_store_result_t;
typedef enum { PANEL_HOMEY_ALIAS_DIAG_UNKNOWN=0, PANEL_HOMEY_ALIAS_DIAG_FALSE, PANEL_HOMEY_ALIAS_DIAG_TRUE } panel_homey_alias_diag_value_t;
typedef enum { PANEL_HOMEY_ALIAS_HINT_UNKNOWN=0, PANEL_HOMEY_ALIAS_HINT_NONE, PANEL_HOMEY_ALIAS_HINT_A, PANEL_HOMEY_ALIAS_HINT_B, PANEL_HOMEY_ALIAS_HINT_INVALID } panel_homey_alias_hint_t;
typedef enum { PANEL_HOMEY_ALIAS_SELECTION_UNKNOWN=0, PANEL_HOMEY_ALIAS_SELECTION_NONE, PANEL_HOMEY_ALIAS_SELECTION_ACTIVE_HINT, PANEL_HOMEY_ALIAS_SELECTION_ONLY_VALID_SLOT, PANEL_HOMEY_ALIAS_SELECTION_GENERATION } panel_homey_alias_selection_basis_t;
typedef struct { uint8_t dashboard_binding_index; char raw_device_id[PANEL_HOMEY_RAW_DEVICE_ID_MAX]; char raw_capability_id[PANEL_HOMEY_RAW_CAPABILITY_ID_MAX]; } panel_homey_alias_entry_t;
typedef struct { uint32_t generation; uint8_t homey_identity_digest[PANEL_HOMEY_IDENTITY_DIGEST_SIZE]; size_t entry_count; panel_homey_alias_entry_t entries[PANEL_HOMEY_ALIAS_STORE_MAX_ENTRIES]; } panel_homey_alias_record_t;
typedef struct { bool configured; panel_homey_alias_record_t record; } panel_homey_alias_runtime_t;
typedef struct {
    bool present;
    bool structurally_valid;
    bool generation_known;
    uint32_t generation;
    bool entry_count_known;
    uint8_t entry_count;
    panel_homey_alias_diag_value_t selected_homey_match;
    panel_homey_alias_diag_value_t awning_present[3];
} panel_homey_alias_slot_diagnostic_t;
typedef struct {
    panel_homey_alias_store_result_t result;
    panel_homey_alias_diag_value_t observation_stable;
    panel_homey_alias_hint_t active_slot_hint;
    panel_homey_alias_slot_t effective_selected_slot;
    panel_homey_alias_selection_basis_t selection_basis;
    panel_homey_alias_slot_diagnostic_t slots[2];
} panel_homey_alias_store_diagnostic_t;

bool panel_homey_alias_sha256(const char *text, uint8_t digest_out[32]);
uint32_t panel_homey_alias_next_generation(bool a_valid,uint32_t a_generation,bool b_valid,uint32_t b_generation);
bool panel_homey_alias_store_diagnose_records(
    const char *selected_homey_id,
    bool slot_a_present,
    bool slot_a_structurally_valid,
    const panel_homey_alias_record_t *slot_a,
    bool slot_b_present,
    bool slot_b_structurally_valid,
    const panel_homey_alias_record_t *slot_b,
    bool active_slot_value_present,
    bool active_slot_read_error,
    uint8_t active_slot_value,
    panel_homey_alias_store_diagnostic_t *diagnostic_out);
bool panel_homey_alias_store_observations_equal(
    const panel_homey_alias_store_diagnostic_t *first,
    const panel_homey_alias_store_diagnostic_t *second);
bool panel_homey_alias_record_encode(const panel_homey_alias_record_t*, uint8_t*, size_t);
bool panel_homey_alias_record_decode(const uint8_t*, size_t, panel_homey_alias_record_t*);
panel_homey_alias_slot_t panel_homey_alias_select_slot(bool,uint32_t,bool,uint32_t,panel_homey_alias_slot_t);
panel_homey_alias_store_result_t panel_homey_alias_runtime_activate(panel_homey_alias_runtime_t*,const panel_homey_alias_record_t*,const char*);
void panel_homey_alias_runtime_invalidate(panel_homey_alias_runtime_t*);
panel_homey_read_result_t panel_homey_alias_runtime_resolve(void*,const char*,const char*,char*,size_t,char*,size_t);
panel_homey_read_result_t panel_homey_alias_runtime_capture(
    void *context,
    panel_homey_alias_snapshot_t *snapshot_out);
#ifdef ESP_PLATFORM
panel_homey_alias_store_result_t panel_homey_alias_store_load(const char*,panel_homey_alias_record_t*,bool*);
panel_homey_alias_store_result_t panel_homey_alias_store_publish(const char*,const panel_homey_alias_record_t*);
panel_homey_alias_store_result_t panel_homey_alias_store_wipe(void);
panel_homey_alias_store_result_t panel_homey_alias_store_inspect(
    const char *selected_homey_id,
    panel_homey_alias_store_diagnostic_t *diagnostic_out);
#endif
