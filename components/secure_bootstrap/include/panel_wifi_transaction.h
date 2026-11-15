#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_WIFI_TXN_MARKER_VERSION 1U
#define PANEL_WIFI_TXN_MARKER_SIZE 16U

typedef enum {
    PANEL_WIFI_TXN_NONE = 0,
    PANEL_WIFI_TXN_CANDIDATE_WRITE_PENDING = 1,
    PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING = 2,
    PANEL_WIFI_TXN_ROLLBACK_PENDING = 3,
    PANEL_WIFI_TXN_RECOVERY_REQUIRED = 4,
} panel_wifi_txn_state_t;

typedef struct {
    panel_wifi_txn_state_t state;
    bool previous_config_present;
    uint32_t generation;
} panel_wifi_txn_marker_t;

typedef struct {
    bool quarantine;
    bool restore_previous_available;
    bool clear_first_install_available;
} panel_wifi_txn_boot_policy_t;

typedef enum {
    PANEL_WIFI_RECOVERY_OWNER_NONE = 0,
    PANEL_WIFI_RECOVERY_OWNER_PENDING,
    PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING,
    PANEL_WIFI_RECOVERY_OWNER_MANUAL_SERVICE,
    /* The slot is the single generation-scoped owner for commit, verify,
     * terminal work and timeout. Queue flags are not ownership. */
    PANEL_WIFI_RECOVERY_OWNER_COMMIT_PREPARING,
    PANEL_WIFI_RECOVERY_OWNER_CANDIDATE_DURABLE,
    PANEL_WIFI_RECOVERY_OWNER_VERIFY_WAITING,
    PANEL_WIFI_RECOVERY_OWNER_FINALIZE_QUEUED,
    PANEL_WIFI_RECOVERY_OWNER_FINALIZE_CLAIMED,
    PANEL_WIFI_RECOVERY_OWNER_FINALIZE_DURABLE,
    PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_DURABLE,
    PANEL_WIFI_RECOVERY_OWNER_TIMED_OUT,
} panel_wifi_recovery_owner_t;

typedef struct {
    uint32_t generation;
    uint32_t rollback_attempted_generation;
    panel_wifi_recovery_owner_t owner;
    uint64_t deadline_ms;
    uint64_t operation_started_ms;
} panel_wifi_recovery_slot_t;

typedef enum {
    PANEL_WIFI_TERMINAL_TIMEOUT_NONE = 0,
    PANEL_WIFI_TERMINAL_TIMEOUT_CLAIMED,
    PANEL_WIFI_TERMINAL_TIMEOUT_DURABLE_BLOCKED,
} panel_wifi_terminal_timeout_result_t;

bool panel_wifi_terminal_begin_prepare(
    panel_wifi_recovery_slot_t *slot, uint32_t generation,
    uint64_t now_ms, uint32_t timeout_ms);
bool panel_wifi_terminal_admit_candidate(
    panel_wifi_recovery_slot_t *slot, uint32_t generation, uint64_t now_ms);
bool panel_wifi_terminal_begin_verify(
    panel_wifi_recovery_slot_t *slot, uint32_t generation,
    uint64_t now_ms, uint32_t timeout_ms);
bool panel_wifi_terminal_queue_finalize(
    panel_wifi_recovery_slot_t *slot, uint32_t generation, uint64_t now_ms);
bool panel_wifi_terminal_claim_finalize(
    panel_wifi_recovery_slot_t *slot, uint32_t generation);
bool panel_wifi_terminal_claim_rollback(
    panel_wifi_recovery_slot_t *slot, uint32_t generation, uint64_t now_ms);
bool panel_wifi_terminal_admit_finalize(
    panel_wifi_recovery_slot_t *slot, uint32_t generation, uint64_t now_ms);
bool panel_wifi_terminal_admit_rollback(
    panel_wifi_recovery_slot_t *slot, uint32_t generation, uint64_t now_ms);
bool panel_wifi_terminal_publish_rollback(
    panel_wifi_recovery_slot_t *slot, uint32_t generation, uint64_t now_ms);
bool panel_wifi_terminal_observe_disconnect(
    const panel_wifi_recovery_slot_t *slot, uint32_t generation,
    bool *sta_link_online);
bool panel_wifi_terminal_fail_pre_durable(
    panel_wifi_recovery_slot_t *slot, uint32_t generation);
bool panel_wifi_terminal_timeout(
    panel_wifi_recovery_slot_t *slot, uint32_t generation, uint64_t now_ms,
    uint32_t durable_stall_ms, panel_wifi_terminal_timeout_result_t *result);
bool panel_wifi_terminal_settle_failed_operation(
    panel_wifi_recovery_slot_t *slot, uint32_t generation,
    panel_wifi_recovery_owner_t durable_owner);

typedef int32_t panel_wifi_txn_result_t;

typedef struct {
    void *context;
    panel_wifi_txn_result_t (*backup_prepare_verified)(void *context, bool previous_config_present);
    panel_wifi_txn_result_t (*marker_write_verified)(void *context, const panel_wifi_txn_marker_t *marker);
    panel_wifi_txn_result_t (*marker_clear_verified)(void *context, uint32_t generation);
    panel_wifi_txn_result_t (*backup_clear)(void *context);
    bool (*executor_available)(void *context, uint32_t generation);
    panel_wifi_txn_result_t (*candidate_persist_verified)(void *context, uint32_t generation);
    panel_wifi_txn_result_t (*restore_previous_verified)(void *context, uint32_t generation);
    panel_wifi_txn_result_t (*clear_first_install_verified)(void *context, uint32_t generation);
} panel_wifi_txn_ops_t;

typedef enum {
    PANEL_WIFI_TXN_STEP_OK = 0,
    PANEL_WIFI_TXN_STEP_INVALID,
    PANEL_WIFI_TXN_STEP_BACKUP_FAILED,
    PANEL_WIFI_TXN_STEP_MARKER_FAILED,
    PANEL_WIFI_TXN_STEP_EXECUTOR_UNAVAILABLE,
    PANEL_WIFI_TXN_STEP_CANDIDATE_FAILED,
    PANEL_WIFI_TXN_STEP_MARKER_CLEAR_FAILED,
    PANEL_WIFI_TXN_STEP_BACKUP_CLEANUP_FAILED,
    PANEL_WIFI_TXN_STEP_RESTORE_FAILED,
    PANEL_WIFI_TXN_STEP_FIRST_INSTALL_CLEAR_FAILED,
} panel_wifi_txn_step_result_t;

bool panel_wifi_txn_marker_encode(
    const panel_wifi_txn_marker_t *marker,
    uint8_t out[PANEL_WIFI_TXN_MARKER_SIZE]);
bool panel_wifi_txn_marker_decode(
    const uint8_t *data,
    size_t size,
    panel_wifi_txn_marker_t *marker);
bool panel_wifi_txn_marker_is_unresolved(const panel_wifi_txn_marker_t *marker);
panel_wifi_txn_boot_policy_t panel_wifi_txn_classify_boot(
    bool marker_storage_ok,
    bool marker_present,
    bool marker_valid,
    const panel_wifi_txn_marker_t *marker,
    bool backup_storage_ok,
    bool backup_present,
    bool backup_valid);

panel_wifi_txn_step_result_t panel_wifi_txn_begin_candidate(
    const panel_wifi_txn_ops_t *ops,
    bool previous_config_present,
    uint32_t generation);
panel_wifi_txn_step_result_t panel_wifi_txn_finish_candidate(
    const panel_wifi_txn_ops_t *ops,
    uint32_t generation);
panel_wifi_txn_step_result_t panel_wifi_txn_restore_previous(
    const panel_wifi_txn_ops_t *ops,
    const panel_wifi_txn_marker_t *marker);
panel_wifi_txn_step_result_t panel_wifi_txn_clear_first_install(
    const panel_wifi_txn_ops_t *ops,
    const panel_wifi_txn_marker_t *marker,
    bool backup_absent);

void panel_wifi_recovery_slot_init(panel_wifi_recovery_slot_t *slot);
bool panel_wifi_recovery_publish_rollback(
    panel_wifi_recovery_slot_t *slot,
    uint32_t generation);
bool panel_wifi_recovery_claim_rollback(
    panel_wifi_recovery_slot_t *slot,
    uint32_t generation);
bool panel_wifi_recovery_claim_manual(
    panel_wifi_recovery_slot_t *slot,
    uint32_t generation);
bool panel_wifi_recovery_finish(
    panel_wifi_recovery_slot_t *slot,
    uint32_t generation,
    panel_wifi_recovery_owner_t owner);
bool panel_wifi_recovery_rollback_already_attempted(
    const panel_wifi_recovery_slot_t *slot,
    uint32_t generation);
