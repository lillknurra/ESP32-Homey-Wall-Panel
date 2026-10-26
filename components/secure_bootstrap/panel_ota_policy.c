#include "panel_ota_policy.h"

#define PANEL_OTA_POLICY_MIN_IMAGE_BYTES UINT32_C(4096)

void panel_ota_policy_init(panel_ota_policy_t *policy)
{
    if (policy == NULL) return;
    *policy = (panel_ota_policy_t){.state = PANEL_OTA_POLICY_IDLE};
}

static bool deny(panel_ota_policy_t *policy)
{
    if (policy != NULL) policy->state = PANEL_OTA_POLICY_DENIED;
    return false;
}

bool panel_ota_policy_begin(panel_ota_policy_t *policy,
                            const panel_ota_preflight_t *preflight)
{
    if (policy == NULL) return false;
    if (policy->state != PANEL_OTA_POLICY_IDLE) return deny(policy);
    if (preflight == NULL ||
        !preflight->authenticated_operator ||
        !preflight->explicit_update_confirmation ||
        !preflight->verified_release_source ||
        !preflight->signature_trust_anchor_ready ||
        !preflight->maintenance_mode_safe_for_flash ||
        !preflight->homey_command_queue_idle ||
        !preflight->running_app_confirmed ||
        preflight->declared_image_bytes < PANEL_OTA_POLICY_MIN_IMAGE_BYTES ||
        preflight->inactive_slot_bytes < PANEL_OTA_POLICY_MIN_IMAGE_BYTES ||
        preflight->declared_image_bytes > preflight->inactive_slot_bytes) {
        return deny(policy);
    }
    policy->state = PANEL_OTA_POLICY_RECEIVING;
    policy->expected_image_bytes = preflight->declared_image_bytes;
    policy->received_image_bytes = 0;
    return true;
}

bool panel_ota_policy_record_chunk(panel_ota_policy_t *policy, uint32_t count)
{
    if (policy == NULL) return false;
    if (policy->state != PANEL_OTA_POLICY_RECEIVING || count == 0 ||
        policy->received_image_bytes > policy->expected_image_bytes ||
        count > policy->expected_image_bytes - policy->received_image_bytes) {
        return deny(policy);
    }
    policy->received_image_bytes += count;
    return true;
}

bool panel_ota_policy_verify_complete(panel_ota_policy_t *policy,
                                      const panel_ota_verification_t *verification)
{
    if (policy == NULL) return false;
    if (policy->state != PANEL_OTA_POLICY_RECEIVING || verification == NULL ||
        policy->received_image_bytes != policy->expected_image_bytes ||
        !verification->complete_image_verified ||
        !verification->signature_verified ||
        !verification->release_digest_matches ||
        !verification->chip_and_project_match ||
        !verification->destination_partition_inactive) {
        return deny(policy);
    }
    policy->state = PANEL_OTA_POLICY_IMAGE_VERIFIED;
    return true;
}

bool panel_ota_policy_authorize_boot_switch(panel_ota_policy_t *policy,
                                             bool authenticated_reboot_confirmation)
{
    if (policy == NULL) return false;
    if (policy->state != PANEL_OTA_POLICY_IMAGE_VERIFIED ||
        !authenticated_reboot_confirmation) {
        return deny(policy);
    }
    policy->state = PANEL_OTA_POLICY_BOOT_SWITCH_ALLOWED;
    return true;
}

void panel_ota_policy_abort(panel_ota_policy_t *policy)
{
    if (policy != NULL) policy->state = PANEL_OTA_POLICY_DENIED;
}

bool panel_ota_policy_should_confirm_first_boot(
    bool running_partition_pending_verify,
    const panel_ota_local_boot_health_t *health)
{
    return running_partition_pending_verify && health != NULL &&
           health->core_services_healthy &&
           health->settings_nvs_readable &&
           health->display_initialized &&
           health->maintenance_barrier_allows_otadata_write;
}
