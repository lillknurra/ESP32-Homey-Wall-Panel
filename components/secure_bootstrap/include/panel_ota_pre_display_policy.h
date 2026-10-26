#pragma once

/* PATCH116B - OFFLINE-ONLY, PURE HOST-TESTABLE MODEL.
 * This file MUST NOT be used directly as an authorization source for hardware
 * flash writes. All evidence must be supplied by separately verified native
 * ESP-IDF/BSP code. No runtime adapter or flash writer is present here.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PANEL_OTA_PREBOOT_DENY = 0,
    PANEL_OTA_PREBOOT_WAIT_FOR_HTTPS,
    PANEL_OTA_PREBOOT_ALLOW_APP_STAGING
} panel_ota_preboot_decision_t;

typedef struct {
    /* The opaque request is an intent handoff, NOT an authentication token.
     * A CRC on RTC memory can only detect corruption, not hostile tampering. */
    bool previously_authenticated_local_operator;
    bool separately_confirmed_update_and_restart;
    bool reset_reason_is_expected_software_restart;
    bool volatile_intent_present_and_crc_valid;
    bool volatile_intent_bounded_and_not_replayed;
    bool release_identifier_is_allowlisted;

    /* Before starting OTA, display hardware MUST NEVER have been started in
     * this boot. Backlight-off and LVGL mutex alone are not sufficient. */
    bool lvgl_not_started_this_boot;
    bool rgb_controller_not_started_this_boot;
    bool no_rgb_dma_activity;

    /* A native arbiter must exclude Homey motion and all other flash writes. */
    bool homey_write_ingress_blocked;
    bool homey_command_queue_empty;
    bool all_other_flash_writers_quiesced;

    /* Secure update prerequisite. These booleans are placeholders for native
     * evidence, not a substitute for ESP-IDF image signature verification. */
    bool signed_running_image_trust_anchor_valid;
    bool rollback_bootloader_enabled;
    bool otadata_partition_valid;
    bool inactive_slot_exists_and_fits_image;
    bool target_chip_project_and_version_policy_verified;

    /* HTTPS may not yet be connected when early boot first examines intent. */
    bool wifi_connected;
    bool https_origin_and_certificate_authenticated;
    bool release_metadata_authenticated;
} panel_ota_preboot_evidence_t;

/* Zero-alloc, no I/O, and no side effects. This is a conservative precondition
 * classifier for future native integration, NOT an implementation of OTA. */
panel_ota_preboot_decision_t panel_ota_preboot_decide(
    const panel_ota_preboot_evidence_t *e);

/* A second check immediately before EVERY erase/write/final boot-selection
 * operation. Never accept proof based on 'screen dark' or 'LCD backlight off'. */
bool panel_ota_pre_display_flash_gate(
    const panel_ota_preboot_evidence_t *e);
