#include "panel_ota_pre_display_policy.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

static panel_ota_preboot_evidence_t full_evidence(void)
{
    return (panel_ota_preboot_evidence_t){
        .previously_authenticated_local_operator = true,
        .separately_confirmed_update_and_restart = true,
        .reset_reason_is_expected_software_restart = true,
        .volatile_intent_present_and_crc_valid = true,
        .volatile_intent_bounded_and_not_replayed = true,
        .release_identifier_is_allowlisted = true,
        .lvgl_not_started_this_boot = true,
        .rgb_controller_not_started_this_boot = true,
        .no_rgb_dma_activity = true,
        .homey_write_ingress_blocked = true,
        .homey_command_queue_empty = true,
        .all_other_flash_writers_quiesced = true,
        .signed_running_image_trust_anchor_valid = true,
        .rollback_bootloader_enabled = true,
        .otadata_partition_valid = true,
        .inactive_slot_exists_and_fits_image = true,
        .target_chip_project_and_version_policy_verified = true,
        .wifi_connected = true,
        .https_origin_and_certificate_authenticated = true,
        .release_metadata_authenticated = true,
    };
}

#define REJECT_MISSING(FIELD) do {                                \
    panel_ota_preboot_evidence_t e = full_evidence();             \
    e.FIELD = false;                                             \
    assert(panel_ota_preboot_decide(&e) !=                      \
        PANEL_OTA_PREBOOT_ALLOW_APP_STAGING);                    \
    assert(!panel_ota_pre_display_flash_gate(&e));               \
    ++cases;                                                     \
} while (0)

int main(void)
{
    unsigned cases = 0;
    panel_ota_preboot_evidence_t e = full_evidence();
    assert(panel_ota_preboot_decide(NULL) == PANEL_OTA_PREBOOT_DENY);
    assert(!panel_ota_pre_display_flash_gate(NULL));
    ++cases;
    assert(panel_ota_preboot_decide(&e) == PANEL_OTA_PREBOOT_ALLOW_APP_STAGING);
    assert(panel_ota_pre_display_flash_gate(&e));
    ++cases;

    REJECT_MISSING(previously_authenticated_local_operator);
    REJECT_MISSING(separately_confirmed_update_and_restart);
    REJECT_MISSING(reset_reason_is_expected_software_restart);
    REJECT_MISSING(volatile_intent_present_and_crc_valid);
    REJECT_MISSING(volatile_intent_bounded_and_not_replayed);
    REJECT_MISSING(release_identifier_is_allowlisted);
    REJECT_MISSING(lvgl_not_started_this_boot);
    REJECT_MISSING(rgb_controller_not_started_this_boot);
    REJECT_MISSING(no_rgb_dma_activity);
    REJECT_MISSING(homey_write_ingress_blocked);
    REJECT_MISSING(homey_command_queue_empty);
    REJECT_MISSING(all_other_flash_writers_quiesced);
    REJECT_MISSING(signed_running_image_trust_anchor_valid);
    REJECT_MISSING(rollback_bootloader_enabled);
    REJECT_MISSING(otadata_partition_valid);
    REJECT_MISSING(inactive_slot_exists_and_fits_image);
    REJECT_MISSING(target_chip_project_and_version_policy_verified);

    /* Awaiting Wi-Fi/TLS/release metadata is not permission to write. */
    e = full_evidence();
    e.wifi_connected = false;
    assert(panel_ota_preboot_decide(&e) == PANEL_OTA_PREBOOT_WAIT_FOR_HTTPS);
    assert(!panel_ota_pre_display_flash_gate(&e));
    ++cases;
    e = full_evidence();
    e.https_origin_and_certificate_authenticated = false;
    assert(panel_ota_preboot_decide(&e) == PANEL_OTA_PREBOOT_WAIT_FOR_HTTPS);
    assert(!panel_ota_pre_display_flash_gate(&e));
    ++cases;
    e = full_evidence();
    e.release_metadata_authenticated = false;
    assert(panel_ota_preboot_decide(&e) == PANEL_OTA_PREBOOT_WAIT_FOR_HTTPS);
    assert(!panel_ota_pre_display_flash_gate(&e));
    ++cases;

    /* Regression: a dark screen is NOT equivalent to stopped RGB DMA. */
    e = full_evidence();
    e.no_rgb_dma_activity = false;
    assert(!panel_ota_pre_display_flash_gate(&e));
    ++cases;

    /* Regression: a stale intent with a valid CRC is NOT sufficient. */
    e = full_evidence();
    e.volatile_intent_bounded_and_not_replayed = false;
    assert(!panel_ota_pre_display_flash_gate(&e));
    ++cases;

    printf("PATCH116B_PREDISPLAY_POLICY_HOST_TESTS=PASS cases=%u\n", cases);
    return 0;
}
