#include "panel_wifi_transaction.h"

#include <string.h>

#define MARKER_MAGIC 0x32545750U /* PWT2 */
#define MARKER_HEADER_SIZE 12U

static uint32_t marker_crc32(const uint8_t *data, size_t size)
{
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0U; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0U; bit < 8U; ++bit) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1U);
            crc = (crc >> 1U) ^ (0xedb88320U & mask);
        }
    }
    return ~crc;
}

static void put_u32_le(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8U);
    out[2] = (uint8_t)(value >> 16U);
    out[3] = (uint8_t)(value >> 24U);
}

static uint32_t get_u32_le(const uint8_t *in)
{
    return (uint32_t)in[0] |
        ((uint32_t)in[1] << 8U) |
        ((uint32_t)in[2] << 16U) |
        ((uint32_t)in[3] << 24U);
}

bool panel_wifi_txn_marker_encode(
    const panel_wifi_txn_marker_t *marker,
    uint8_t out[PANEL_WIFI_TXN_MARKER_SIZE])
{
    if (marker == NULL || out == NULL || marker->generation == 0U ||
        marker->state < PANEL_WIFI_TXN_CANDIDATE_WRITE_PENDING ||
        marker->state > PANEL_WIFI_TXN_RECOVERY_REQUIRED) return false;
    memset(out, 0, PANEL_WIFI_TXN_MARKER_SIZE);
    put_u32_le(out, MARKER_MAGIC);
    out[4] = PANEL_WIFI_TXN_MARKER_VERSION;
    out[5] = (uint8_t)marker->state;
    out[6] = marker->previous_config_present ? 1U : 0U;
    put_u32_le(&out[8], marker->generation);
    put_u32_le(&out[12], marker_crc32(out, MARKER_HEADER_SIZE));
    return true;
}

bool panel_wifi_txn_marker_decode(
    const uint8_t *data,
    size_t size,
    panel_wifi_txn_marker_t *marker)
{
    if (data == NULL || marker == NULL || size != PANEL_WIFI_TXN_MARKER_SIZE ||
        get_u32_le(data) != MARKER_MAGIC ||
        data[4] != PANEL_WIFI_TXN_MARKER_VERSION ||
        data[5] < PANEL_WIFI_TXN_CANDIDATE_WRITE_PENDING ||
        data[5] > PANEL_WIFI_TXN_RECOVERY_REQUIRED ||
        data[6] > 1U || data[7] != 0U ||
        get_u32_le(&data[8]) == 0U ||
        get_u32_le(&data[12]) != marker_crc32(data, MARKER_HEADER_SIZE)) return false;
    marker->state = (panel_wifi_txn_state_t)data[5];
    marker->previous_config_present = data[6] != 0U;
    marker->generation = get_u32_le(&data[8]);
    return true;
}

bool panel_wifi_txn_marker_is_unresolved(const panel_wifi_txn_marker_t *marker)
{
    return marker != NULL && marker->state >= PANEL_WIFI_TXN_CANDIDATE_WRITE_PENDING &&
        marker->state <= PANEL_WIFI_TXN_RECOVERY_REQUIRED;
}

panel_wifi_txn_boot_policy_t panel_wifi_txn_classify_boot(
    bool marker_storage_ok,
    bool marker_present,
    bool marker_valid,
    const panel_wifi_txn_marker_t *marker,
    bool backup_storage_ok,
    bool backup_present,
    bool backup_valid)
{
    panel_wifi_txn_boot_policy_t result = {0};
    if (marker_storage_ok && !marker_present && backup_storage_ok && !backup_present)
        return result;
    result.quarantine = true;
    if (marker_storage_ok && marker_present && marker_valid &&
        panel_wifi_txn_marker_is_unresolved(marker)) {
        if (marker->previous_config_present) {
            result.restore_previous_available = backup_storage_ok &&
                backup_present && backup_valid;
        } else {
            result.clear_first_install_available = backup_storage_ok &&
                !backup_present;
        }
    } else {
        /* A corrupt/unreadable marker never proves first-install. A valid
         * backup may still support an explicit, confirmed restore. */
        result.restore_previous_available = backup_storage_ok &&
            backup_present && backup_valid;
    }
    return result;
}

static bool ops_valid(const panel_wifi_txn_ops_t *ops)
{
    return ops != NULL && ops->context != NULL &&
        ops->backup_prepare_verified != NULL &&
        ops->marker_write_verified != NULL &&
        ops->marker_clear_verified != NULL && ops->backup_clear != NULL &&
        ops->executor_available != NULL &&
        ops->candidate_persist_verified != NULL &&
        ops->restore_previous_verified != NULL &&
        ops->clear_first_install_verified != NULL;
}

panel_wifi_txn_step_result_t panel_wifi_txn_begin_candidate(
    const panel_wifi_txn_ops_t *ops,
    bool previous_config_present,
    uint32_t generation)
{
    if (!ops_valid(ops) || generation == 0U) return PANEL_WIFI_TXN_STEP_INVALID;
    if (!ops->executor_available(ops->context, generation))
        return PANEL_WIFI_TXN_STEP_EXECUTOR_UNAVAILABLE;
    if (ops->backup_prepare_verified(ops->context, previous_config_present) != 0)
        return PANEL_WIFI_TXN_STEP_BACKUP_FAILED;

    panel_wifi_txn_marker_t marker = {
        .state = PANEL_WIFI_TXN_CANDIDATE_WRITE_PENDING,
        .previous_config_present = previous_config_present,
        .generation = generation,
    };
    if (ops->marker_write_verified(ops->context, &marker) != 0)
        return PANEL_WIFI_TXN_STEP_MARKER_FAILED;
    if (!ops->executor_available(ops->context, generation))
        return PANEL_WIFI_TXN_STEP_EXECUTOR_UNAVAILABLE;
    if (ops->candidate_persist_verified(ops->context, generation) != 0)
        return PANEL_WIFI_TXN_STEP_CANDIDATE_FAILED;

    marker.state = PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING;
    if (ops->marker_write_verified(ops->context, &marker) != 0)
        return PANEL_WIFI_TXN_STEP_MARKER_FAILED;
    return PANEL_WIFI_TXN_STEP_OK;
}

static panel_wifi_txn_step_result_t marker_clear_then_backup(
    const panel_wifi_txn_ops_t *ops,
    uint32_t generation)
{
    if (ops->marker_clear_verified(ops->context, generation) != 0)
        return PANEL_WIFI_TXN_STEP_MARKER_CLEAR_FAILED;
    if (ops->backup_clear(ops->context) != 0)
        return PANEL_WIFI_TXN_STEP_BACKUP_CLEANUP_FAILED;
    return PANEL_WIFI_TXN_STEP_OK;
}

panel_wifi_txn_step_result_t panel_wifi_txn_finish_candidate(
    const panel_wifi_txn_ops_t *ops,
    uint32_t generation)
{
    if (!ops_valid(ops) || generation == 0U) return PANEL_WIFI_TXN_STEP_INVALID;
    return marker_clear_then_backup(ops, generation);
}

panel_wifi_txn_step_result_t panel_wifi_txn_restore_previous(
    const panel_wifi_txn_ops_t *ops,
    const panel_wifi_txn_marker_t *marker)
{
    if (!ops_valid(ops) || !panel_wifi_txn_marker_is_unresolved(marker) ||
        !marker->previous_config_present) return PANEL_WIFI_TXN_STEP_INVALID;
    panel_wifi_txn_marker_t pending = *marker;
    pending.state = PANEL_WIFI_TXN_ROLLBACK_PENDING;
    if (ops->marker_write_verified(ops->context, &pending) != 0)
        return PANEL_WIFI_TXN_STEP_MARKER_FAILED;
    if (ops->restore_previous_verified(ops->context, marker->generation) != 0)
        return PANEL_WIFI_TXN_STEP_RESTORE_FAILED;
    return marker_clear_then_backup(ops, marker->generation);
}

panel_wifi_txn_step_result_t panel_wifi_txn_clear_first_install(
    const panel_wifi_txn_ops_t *ops,
    const panel_wifi_txn_marker_t *marker,
    bool backup_absent)
{
    if (!ops_valid(ops) || !panel_wifi_txn_marker_is_unresolved(marker) ||
        marker->previous_config_present || !backup_absent) return PANEL_WIFI_TXN_STEP_INVALID;
    if (ops->clear_first_install_verified(ops->context, marker->generation) != 0)
        return PANEL_WIFI_TXN_STEP_FIRST_INSTALL_CLEAR_FAILED;
    if (ops->marker_clear_verified(ops->context, marker->generation) != 0)
        return PANEL_WIFI_TXN_STEP_MARKER_CLEAR_FAILED;
    return PANEL_WIFI_TXN_STEP_OK;
}

void panel_wifi_recovery_slot_init(panel_wifi_recovery_slot_t *slot)
{
    if (slot != NULL) memset(slot, 0, sizeof(*slot));
}

bool panel_wifi_recovery_publish_rollback(
    panel_wifi_recovery_slot_t *slot,
    uint32_t generation)
{
    if (slot == NULL || generation == 0U ||
        slot->rollback_attempted_generation == generation) return false;
    if (slot->owner == PANEL_WIFI_RECOVERY_OWNER_PENDING &&
        slot->generation == generation) return true;
    if (slot->owner != PANEL_WIFI_RECOVERY_OWNER_NONE) return false;
    slot->generation = generation;
    slot->owner = PANEL_WIFI_RECOVERY_OWNER_PENDING;
    return true;
}

bool panel_wifi_recovery_claim_rollback(
    panel_wifi_recovery_slot_t *slot,
    uint32_t generation)
{
    if (slot == NULL || generation == 0U || slot->generation != generation ||
        slot->owner != PANEL_WIFI_RECOVERY_OWNER_PENDING ||
        slot->rollback_attempted_generation == generation) return false;
    slot->owner = PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING;
    slot->rollback_attempted_generation = generation;
    return true;
}

bool panel_wifi_recovery_claim_manual(
    panel_wifi_recovery_slot_t *slot,
    uint32_t generation)
{
    if (slot == NULL || generation == 0U ||
        (slot->owner != PANEL_WIFI_RECOVERY_OWNER_NONE &&
         !(slot->owner == PANEL_WIFI_RECOVERY_OWNER_PENDING &&
           slot->generation == generation))) return false;
    slot->generation = generation;
    slot->owner = PANEL_WIFI_RECOVERY_OWNER_MANUAL_SERVICE;
    return true;
}

bool panel_wifi_recovery_finish(
    panel_wifi_recovery_slot_t *slot,
    uint32_t generation,
    panel_wifi_recovery_owner_t owner)
{
    if (slot == NULL || generation == 0U || slot->generation != generation ||
        slot->owner != owner || owner == PANEL_WIFI_RECOVERY_OWNER_NONE) return false;
    if (owner == PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING)
        slot->rollback_attempted_generation = generation;
    slot->owner = PANEL_WIFI_RECOVERY_OWNER_NONE;
    return true;
}

bool panel_wifi_recovery_rollback_already_attempted(
    const panel_wifi_recovery_slot_t *slot,
    uint32_t generation)
{
    return slot != NULL && generation != 0U &&
        slot->rollback_attempted_generation == generation;
}
