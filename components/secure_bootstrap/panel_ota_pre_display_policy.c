#include "panel_ota_pre_display_policy.h"

static bool authorization_and_exclusivity_ok(const panel_ota_preboot_evidence_t *e)
{
    return e != NULL &&
        e->previously_authenticated_local_operator &&
        e->separately_confirmed_update_and_restart &&
        e->reset_reason_is_expected_software_restart &&
        e->volatile_intent_present_and_crc_valid &&
        e->volatile_intent_bounded_and_not_replayed &&
        e->release_identifier_is_allowlisted &&
        e->lvgl_not_started_this_boot &&
        e->rgb_controller_not_started_this_boot &&
        e->no_rgb_dma_activity &&
        e->homey_write_ingress_blocked &&
        e->homey_command_queue_empty &&
        e->all_other_flash_writers_quiesced &&
        e->signed_running_image_trust_anchor_valid &&
        e->rollback_bootloader_enabled &&
        e->otadata_partition_valid &&
        e->inactive_slot_exists_and_fits_image &&
        e->target_chip_project_and_version_policy_verified;
}

panel_ota_preboot_decision_t panel_ota_preboot_decide(
    const panel_ota_preboot_evidence_t *e)
{
    if (!authorization_and_exclusivity_ok(e)) {
        return PANEL_OTA_PREBOOT_DENY;
    }
    if (!e->wifi_connected ||
        !e->https_origin_and_certificate_authenticated ||
        !e->release_metadata_authenticated) {
        return PANEL_OTA_PREBOOT_WAIT_FOR_HTTPS;
    }
    return PANEL_OTA_PREBOOT_ALLOW_APP_STAGING;
}

bool panel_ota_pre_display_flash_gate(
    const panel_ota_preboot_evidence_t *e)
{
    /* Called again just before any flash-changing native call. A native
     * integration must prevent races between this check and the actual write. */
    return panel_ota_preboot_decide(e) == PANEL_OTA_PREBOOT_ALLOW_APP_STAGING;
}
