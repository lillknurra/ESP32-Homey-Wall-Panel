#include "panel_ota_policy.h"

#include <assert.h>
#include <stdio.h>

static panel_ota_preflight_t valid_preflight(void)
{
    return (panel_ota_preflight_t){
        .authenticated_operator = true,
        .explicit_update_confirmation = true,
        .verified_release_source = true,
        .signature_trust_anchor_ready = true,
        .maintenance_mode_safe_for_flash = true,
        .homey_command_queue_idle = true,
        .running_app_confirmed = true,
        .declared_image_bytes = 1749184U,
        .inactive_slot_bytes = 4194304U,
    };
}

static panel_ota_verification_t valid_verification(void)
{
    return (panel_ota_verification_t){
        .complete_image_verified = true,
        .signature_verified = true,
        .release_digest_matches = true,
        .chip_and_project_match = true,
        .destination_partition_inactive = true,
    };
}

static void run_positive(void)
{
    panel_ota_policy_t policy;
    panel_ota_policy_init(&policy);
    panel_ota_preflight_t gate = valid_preflight();
    assert(panel_ota_policy_begin(&policy, &gate));
    assert(policy.state == PANEL_OTA_POLICY_RECEIVING);
    assert(panel_ota_policy_record_chunk(&policy, 16066U));
    assert(panel_ota_policy_record_chunk(&policy, gate.declared_image_bytes - 16066U));
    panel_ota_verification_t verification = valid_verification();
    assert(panel_ota_policy_verify_complete(&policy, &verification));
    assert(panel_ota_policy_authorize_boot_switch(&policy, true));
    assert(policy.state == PANEL_OTA_POLICY_BOOT_SWITCH_ALLOWED);
}

static void run_preflight_negative(void)
{
    panel_ota_preflight_t gate = valid_preflight();
    panel_ota_policy_t policy;
#define REJECT_WITH(field) do {                                           \
    panel_ota_policy_init(&policy);                                      \
    gate = valid_preflight(); gate.field = false;                         \
    assert(!panel_ota_policy_begin(&policy, &gate));                      \
    assert(policy.state == PANEL_OTA_POLICY_DENIED);                      \
    assert(!panel_ota_policy_record_chunk(&policy, 4096));               \
} while (0)
    REJECT_WITH(authenticated_operator);
    REJECT_WITH(explicit_update_confirmation);
    REJECT_WITH(verified_release_source);
    REJECT_WITH(signature_trust_anchor_ready);
    REJECT_WITH(maintenance_mode_safe_for_flash);
    REJECT_WITH(homey_command_queue_idle);
    REJECT_WITH(running_app_confirmed);
#undef REJECT_WITH

    panel_ota_policy_init(&policy);
    gate = valid_preflight(); gate.declared_image_bytes = 0;
    assert(!panel_ota_policy_begin(&policy, &gate));
    panel_ota_policy_init(&policy);
    gate = valid_preflight(); gate.declared_image_bytes = 4095;
    assert(!panel_ota_policy_begin(&policy, &gate));
    panel_ota_policy_init(&policy);
    gate = valid_preflight(); gate.declared_image_bytes = gate.inactive_slot_bytes + 1;
    assert(!panel_ota_policy_begin(&policy, &gate));
    panel_ota_policy_init(&policy);
    gate = valid_preflight(); gate.inactive_slot_bytes = 4095;
    assert(!panel_ota_policy_begin(&policy, &gate));
    panel_ota_policy_init(&policy);
    assert(!panel_ota_policy_begin(&policy, NULL));
    assert(!panel_ota_policy_begin(NULL, &gate));
}

static void run_chunk_negative(void)
{
    panel_ota_policy_t policy;
    panel_ota_preflight_t gate = valid_preflight();
    panel_ota_verification_t verify = valid_verification();
    panel_ota_policy_init(&policy);
    assert(panel_ota_policy_begin(&policy, &gate));
    assert(!panel_ota_policy_verify_complete(&policy, &verify)); /* incomplete */
    assert(policy.state == PANEL_OTA_POLICY_DENIED);

    panel_ota_policy_init(&policy);
    assert(panel_ota_policy_begin(&policy, &gate));
    assert(!panel_ota_policy_record_chunk(&policy, 0));
    panel_ota_policy_init(&policy);
    assert(panel_ota_policy_begin(&policy, &gate));
    assert(!panel_ota_policy_record_chunk(&policy, gate.declared_image_bytes + 1U));
    panel_ota_policy_init(&policy);
    assert(panel_ota_policy_begin(&policy, &gate));
    assert(panel_ota_policy_record_chunk(&policy, gate.declared_image_bytes));
    assert(!panel_ota_policy_record_chunk(&policy, 1));
    panel_ota_policy_init(&policy);
    assert(panel_ota_policy_begin(&policy, &gate));
    assert(!panel_ota_policy_begin(&policy, &gate)); /* reentrant */
    panel_ota_policy_init(&policy);
    assert(panel_ota_policy_begin(&policy, &gate));
    panel_ota_policy_abort(&policy);
    assert(!panel_ota_policy_record_chunk(&policy, 4096));
    panel_ota_policy_abort(NULL);
    assert(!panel_ota_policy_record_chunk(NULL, 4096));
    panel_ota_policy_init(&policy);
    assert(!panel_ota_policy_record_chunk(&policy, 4096)); /* before begin */
    panel_ota_policy_init(&policy);
    assert(panel_ota_policy_begin(&policy, &gate));
    policy.received_image_bytes = policy.expected_image_bytes + 1U;
    assert(!panel_ota_policy_record_chunk(&policy, 4096)); /* corrupt state */
}

static void run_verify_negative(void)
{
    panel_ota_policy_t policy;
    panel_ota_preflight_t gate = valid_preflight();
    panel_ota_verification_t verification;
#define REJECT_VERIFY(field) do {                                     \
    panel_ota_policy_init(&policy);                                  \
    assert(panel_ota_policy_begin(&policy, &gate));                   \
    assert(panel_ota_policy_record_chunk(&policy, gate.declared_image_bytes)); \
    verification = valid_verification();                             \
    verification.field = false;                                      \
    assert(!panel_ota_policy_verify_complete(&policy, &verification)); \
    assert(policy.state == PANEL_OTA_POLICY_DENIED);                  \
} while (0)
    REJECT_VERIFY(complete_image_verified);
    REJECT_VERIFY(signature_verified);
    REJECT_VERIFY(release_digest_matches);
    REJECT_VERIFY(chip_and_project_match);
    REJECT_VERIFY(destination_partition_inactive);
#undef REJECT_VERIFY
    panel_ota_policy_init(&policy);
    assert(panel_ota_policy_begin(&policy, &gate));
    assert(panel_ota_policy_record_chunk(&policy, gate.declared_image_bytes));
    assert(!panel_ota_policy_verify_complete(&policy, NULL));
    panel_ota_policy_init(&policy);
    assert(panel_ota_policy_begin(&policy, &gate));
    assert(panel_ota_policy_record_chunk(&policy, gate.declared_image_bytes));
    verification = valid_verification();
    assert(panel_ota_policy_verify_complete(&policy, &verification));
    assert(!panel_ota_policy_authorize_boot_switch(&policy, false));
    assert(policy.state == PANEL_OTA_POLICY_DENIED);
    assert(!panel_ota_policy_authorize_boot_switch(&policy, true));
}

static void run_boot_health(void)
{
    panel_ota_local_boot_health_t health = {
        .core_services_healthy = true,
        .settings_nvs_readable = true,
        .display_initialized = true,
        .maintenance_barrier_allows_otadata_write = true,
    };
    assert(panel_ota_policy_should_confirm_first_boot(true, &health));
    assert(!panel_ota_policy_should_confirm_first_boot(false, &health));
    assert(!panel_ota_policy_should_confirm_first_boot(true, NULL));
    health.settings_nvs_readable = false;
    assert(!panel_ota_policy_should_confirm_first_boot(true, &health));
    health.settings_nvs_readable = true; health.display_initialized = false;
    assert(!panel_ota_policy_should_confirm_first_boot(true, &health));
    health.display_initialized = true; health.core_services_healthy = false;
    assert(!panel_ota_policy_should_confirm_first_boot(true, &health));
    health.core_services_healthy = true; health.maintenance_barrier_allows_otadata_write = false;
    assert(!panel_ota_policy_should_confirm_first_boot(true, &health));
}

int main(void)
{
    run_positive();
    run_preflight_negative();
    run_chunk_negative();
    run_verify_negative();
    run_boot_health();
    puts("PATCH116_OTA_POLICY_TESTS=PASS (pure host-only model; no firmware integration)");
    return 0;
}
