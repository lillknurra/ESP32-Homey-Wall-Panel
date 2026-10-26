#pragma once

/* Patch116: pure decision model for OFFLINE HOST VALIDATION ONLY.
 * Not added to the ESP-IDF component's CMakeLists.txt and not connected to HTTP,
 * ESP-IDF OTA APIs, NVS, or flash. Do not treat boolean caller-supplied evidence
 * as a security boundary: production sources must be independently verified.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PANEL_OTA_POLICY_IDLE = 0,
    PANEL_OTA_POLICY_RECEIVING,
    PANEL_OTA_POLICY_IMAGE_VERIFIED,
    PANEL_OTA_POLICY_BOOT_SWITCH_ALLOWED,
    PANEL_OTA_POLICY_DENIED
} panel_ota_policy_state_t;

typedef struct {
    bool authenticated_operator;         /* authenticated admin, not simply LAN reachability */
    bool explicit_update_confirmation;   /* separate operator consent per update */
    bool verified_release_source;        /* authenticated TLS and release authorization */
    bool signature_trust_anchor_ready;   /* provisioned signed-app root, not caller-owned metadata */
    bool maintenance_mode_safe_for_flash;/* RGB DMA/flash policy owned by board integration */
    bool homey_command_queue_idle;       /* no overlapping device writes */
    bool running_app_confirmed;          /* not PENDING_VERIFY */
    uint32_t declared_image_bytes;       /* signed size, including signature block */
    uint32_t inactive_slot_bytes;        /* from esp_ota_get_next_update_partition() */
} panel_ota_preflight_t;

typedef struct {
    bool complete_image_verified;     /* ESP-IDF esp_ota_end()/image verifier result */
    bool signature_verified;          /* trusted cryptographic verifier result */
    bool release_digest_matches;      /* trusted, authenticated release metadata */
    bool chip_and_project_match;
    bool destination_partition_inactive;
} panel_ota_verification_t;

typedef struct {
    panel_ota_policy_state_t state;
    uint32_t expected_image_bytes;
    uint32_t received_image_bytes;
} panel_ota_policy_t;

typedef struct {
    bool core_services_healthy;
    bool settings_nvs_readable;
    bool display_initialized;
    bool maintenance_barrier_allows_otadata_write;
} panel_ota_local_boot_health_t;

void panel_ota_policy_init(panel_ota_policy_t *policy);
bool panel_ota_policy_begin(panel_ota_policy_t *policy,
                            const panel_ota_preflight_t *preflight);
bool panel_ota_policy_record_chunk(panel_ota_policy_t *policy, uint32_t count);
bool panel_ota_policy_verify_complete(panel_ota_policy_t *policy,
                                       const panel_ota_verification_t *verification);
bool panel_ota_policy_authorize_boot_switch(panel_ota_policy_t *policy,
                                             bool authenticated_reboot_confirmation);
void panel_ota_policy_abort(panel_ota_policy_t *policy);

/* Pure first-boot decision only. Caller must check actual OTA image state and use
 * esp_ota_mark_app_valid_cancel_rollback() or rollback APIs, with flash safety.
 * Homey READY is intentionally NOT an input; connectivity can be transient.
 */
bool panel_ota_policy_should_confirm_first_boot(
    bool running_partition_pending_verify,
    const panel_ota_local_boot_health_t *health);
