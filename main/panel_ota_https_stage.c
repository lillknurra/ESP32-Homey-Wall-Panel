/* Patch120B: headless, operator-confirmed signed OTA staging. */
#include "panel_ota_https_stage.h"
#include "panel_update_manifest.h"
#include "sdkconfig.h"

#ifndef PANEL_OTA_HTTPS_STAGE_ENABLED
#define PANEL_OTA_HTTPS_STAGE_ENABLED 0
#endif
#if PANEL_OTA_HTTPS_STAGE_ENABLED != 0 && PANEL_OTA_HTTPS_STAGE_ENABLED != 1
#error "PANEL_OTA_HTTPS_STAGE_ENABLED must be 0 or 1"
#endif

#if PANEL_OTA_HTTPS_STAGE_ENABLED
#if !defined(CONFIG_IDF_TARGET_ESP32S3) || CONFIG_IDF_TARGET_ESP32S3 != 1 || \
    !defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) || CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE != 1 || \
    !defined(CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT) || CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT != 1 || \
    !defined(CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT) || CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT != 1 || \
    !defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE) || CONFIG_MBEDTLS_CERTIFICATE_BUNDLE != 1 || \
    !defined(PANEL_OTA_FIRST_BOOT_ENABLED) || (PANEL_OTA_FIRST_BOOT_ENABLED != 1)
#error "OTA transport needs verified ESP32-S3 signed+rollback release and first-boot gate"
#endif
#if defined(CONFIG_ESP_HTTPS_OTA_ALLOW_HTTP) && CONFIG_ESP_HTTPS_OTA_ALLOW_HTTP
#error "Insecure HTTP OTA must be disabled"
#endif
#if defined(CONFIG_SECURE_BOOT) || defined(CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK) || \
    defined(CONFIG_FLASH_ENCRYPTION_ENABLED)
#error "Hardware Secure Boot, eFuse anti-rollback or flash encryption are outside Patch116E"
#endif
#if !defined(PANEL_OTA_MAX_IMAGE_BYTES) || \
    !defined(PANEL_OTA_MAX_TRANSFER_MS)
#error "OTA image limit and transfer deadline are required"
#endif
#if PANEL_OTA_MAX_IMAGE_BYTES < 4096 || PANEL_OTA_MAX_IMAGE_BYTES > 0x400000
#error "Maximum image size must fit a 4-MiB OTA slot"
#endif
#if PANEL_OTA_MAX_TRANSFER_MS < 10000 || PANEL_OTA_MAX_TRANSFER_MS > 600000
#error "OTA deadline must be between 10 seconds and 10 minutes"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_app_format.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "psa/crypto.h"
#include "panel_ota_supervisor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char PROJECT_NAME[] = "esp32_homey_wall_panel";

static bool access_still_authorized(const panel_ota_stage_access_t *access)
{
    return access != NULL &&
        access->authenticated_operator_confirmed != NULL &&
        access->pre_display_maintenance_quiescent != NULL &&
        access->authenticated_operator_confirmed(access->context) &&
        access->pre_display_maintenance_quiescent(access->context);
}

typedef struct {
    esp_partition_type_t type;
    esp_partition_subtype_t subtype;
    const char *label;
    uint32_t offset;
    uint32_t size;
} partition_expected_t;

static const partition_expected_t EXPECTED[] = {
    {ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "nvs", 0x9000, 0x6000},
    {ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_PHY, "phy_init", 0xf000, 0x1000},
    {ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory", 0x10000, 0x600000},
    {ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "diaglog", 0x610000, 0x100000},
    {ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, "otadata", 0x710000, 0x2000},
    {ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, "ota_0", 0x720000, 0x400000},
    {ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "ota_1", 0xb20000, 0x400000},
};

static bool layout_valid(void)
{
    for (size_t i = 0; i < sizeof(EXPECTED) / sizeof(EXPECTED[0]); ++i) {
        const partition_expected_t *p = &EXPECTED[i];
        const esp_partition_t *actual = esp_partition_find_first(p->type, p->subtype, p->label);
        if (actual == NULL || actual->address != p->offset || actual->size != p->size ||
            actual->type != p->type || actual->subtype != p->subtype) return false;
    }
    return true;
}

static bool equal_terminated(const char *left, const char *right, size_t len);

bool panel_ota_https_stage_layout_preflight(void)
{
#if !PANEL_OTA_HTTPS_STAGE_ENABLED
    return false;
#else
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t desc = {0};
    return layout_valid() && running != NULL && running->type == ESP_PARTITION_TYPE_APP &&
        esp_ota_get_partition_description(running, &desc) == ESP_OK &&
        equal_terminated(desc.project_name, PROJECT_NAME, sizeof(desc.project_name));
#endif
}

static bool is_expected_ota_slot(const esp_partition_t *p)
{
    return p != NULL && p->type == ESP_PARTITION_TYPE_APP && p->size == 0x400000U &&
        ((p->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0 && p->address == 0x720000U) ||
         (p->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1 && p->address == 0xb20000U));
}

static bool equal_terminated(const char *left, const char *right, size_t len)
{
    return memchr(left, '\0', len) != NULL && memchr(right, '\0', len) != NULL &&
           strncmp(left, right, len) == 0;
}

static bool preflight(const panel_ota_stage_access_t *access,
                      const panel_update_manifest_t *manifest,
                      esp_app_desc_t *current_desc, const esp_partition_t **target)
{
    if (!access_still_authorized(access) || !layout_valid() || manifest == NULL ||
        !panel_update_manifest_metadata_valid(manifest) ||
        strcmp(manifest->hardware_model, PANEL_UPDATE_MODEL_ID) != 0 ||
        strcmp(manifest->project, PROJECT_NAME) != 0 ||
        strcmp(manifest->channel, "stable") != 0 ||
        manifest->release_sequence <= CONFIG_PANEL_OTA_RELEASE_SEQUENCE) return false;
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    if (running == NULL || !is_expected_ota_slot(next) || next == running ||
        running->type != ESP_PARTITION_TYPE_APP) return false;
    if (running->subtype != ESP_PARTITION_SUBTYPE_APP_FACTORY) {
        if (!is_expected_ota_slot(running)) return false;
        esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
        if (esp_ota_get_state_partition(running, &state) != ESP_OK ||
            state != ESP_OTA_IMG_VALID) return false;
    } else if (running->address != 0x10000U || running->size != 0x600000U) {
        return false;
    }
    const esp_app_desc_t *app = esp_app_get_description();
    if (app == NULL ||
        esp_ota_get_partition_description(running, current_desc) != ESP_OK ||
        !equal_terminated(app->project_name, PROJECT_NAME, sizeof(app->project_name)) ||
        !equal_terminated(current_desc->project_name, PROJECT_NAME,
                          sizeof(current_desc->project_name)) ||
        !equal_terminated(current_desc->version, app->version, sizeof(app->version))) return false;
    *target = next;
    return true;
}

static bool candidate_acceptable(const esp_app_desc_t *current,
                                 const esp_app_desc_t *candidate,
                                 const panel_update_manifest_t *manifest)
{
    int version_order = 0;
    uint32_t installed_sequence = 0U;
    uint32_t candidate_sequence = 0U;
    if (current == NULL || candidate == NULL || manifest == NULL ||
        memchr(current->version, '\0', sizeof(current->version)) == NULL ||
        memchr(candidate->version, '\0', sizeof(candidate->version)) == NULL ||
        !equal_terminated(candidate->project_name, PROJECT_NAME,
                          sizeof(candidate->project_name)) ||
        !panel_update_release_sequence_for_version(current->version,
                                                   &installed_sequence) ||
        installed_sequence != CONFIG_PANEL_OTA_RELEASE_SEQUENCE ||
        !panel_update_release_sequence_for_version(candidate->version,
                                                   &candidate_sequence) ||
        candidate_sequence != manifest->release_sequence ||
        candidate->secure_version != current->secure_version ||
        candidate->version[0] == '\0' ||
        strncmp(candidate->version, current->version,
                sizeof(candidate->version)) == 0 ||
        strncmp(candidate->version, manifest->version,
                sizeof(candidate->version)) != 0 ||
        !panel_update_semver_compare(candidate->version, current->version,
                                     &version_order) || version_order <= 0) return false;
    /* Sequence and semantic version both move strictly forward. Sequence is
     * software release policy, not eFuse anti-rollback. */
    return manifest->release_sequence > CONFIG_PANEL_OTA_RELEASE_SEQUENCE;
}

static bool transfer_time_remaining(int64_t started)
{
    const int64_t now = esp_timer_get_time();
    if (started < 0 || now < started) return false;
    return (uint64_t)(now - started) <=
        (uint64_t)PANEL_OTA_MAX_TRANSFER_MS * 1000ULL;
}
#endif /* ENABLED */

/* The IDF 6.0.1 esp_https_ota_get_img_desc() helper loops without a total
 * EAGAIN deadline inside read_header(). Do NOT call it, including indirectly
 * through esp_https_ota_perform(). Instead use one owned HTTP client for both
 * descriptor preflight and the OTA write stream. The first flash write occurs
 * only after the complete verified image preamble and signed release policy.
 * A normal 2-second I/O timeout is not a hard guarantee on DNS/TLS internals;
 * no implementation can force-close a synchronous IDF call from this task.
 */
#if PANEL_OTA_HTTPS_STAGE_ENABLED
#define PANEL_OTA_IO_TIMEOUT_MS 2000
#define PANEL_OTA_READ_BUFFER_SIZE 2048U
#define PANEL_OTA_HEADER_MAX_READ_CALLS 512U
#define PANEL_OTA_MAX_EAGAIN_STREAK 8U
#define PANEL_OTA_PREAMBLE_SIZE \
    (sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))

static bool image_preamble_acceptable(const uint8_t preamble[PANEL_OTA_PREAMBLE_SIZE],
                                     int64_t length, const esp_app_desc_t *current,
                                     const panel_update_manifest_t *manifest)
{
    esp_image_header_t image = {0};
    esp_image_segment_header_t segment = {0};
    esp_app_desc_t desc = {0};
    memcpy(&image, preamble, sizeof(image));
    memcpy(&segment, preamble + sizeof(image), sizeof(segment));
    memcpy(&desc, preamble + sizeof(image) + sizeof(segment), sizeof(desc));
    return image.magic == ESP_IMAGE_HEADER_MAGIC &&
        image.chip_id == ESP_CHIP_ID_ESP32S3 &&
        image.segment_count > 0U && image.segment_count <= ESP_IMAGE_MAX_SEGMENTS &&
        desc.magic_word == ESP_APP_DESC_MAGIC_WORD &&
        segment.data_len >= sizeof(desc) &&
        length >= (int64_t)PANEL_OTA_PREAMBLE_SIZE &&
        (uint64_t)segment.data_len <= (uint64_t)length -
            (sizeof(image) + sizeof(segment)) &&
        candidate_acceptable(current, &desc, manifest);
}

/* No read/parse loop may retry forever, even with a frozen test clock. */
static bool read_bounded_preamble(
    esp_http_client_handle_t client, uint8_t out[PANEL_OTA_PREAMBLE_SIZE],
    int64_t started, const panel_ota_stage_access_t *access,
    panel_ota_stage_result_t *failure)
{
    size_t received = 0U;
    unsigned eagain_streak = 0U;
    for (unsigned calls = 0U; received < PANEL_OTA_PREAMBLE_SIZE &&
                              calls < PANEL_OTA_HEADER_MAX_READ_CALLS; ++calls) {
        if (!transfer_time_remaining(started)) {
            *failure = PANEL_OTA_STAGE_INCOMPLETE;
            return false;
        }
        if (!access_still_authorized(access)) {
            *failure = PANEL_OTA_STAGE_DENIED;
            return false;
        }
        /* esp_http_client_read() can itself loop internally until requested
         * bytes are received. Request just ONE preamble byte per call so the
         * aggregate deadline is checked between every byte, even if a peer
         * trickles data without ever letting an IDF read time out. */
        const int got = esp_http_client_read(client, (char *)out + received, 1);
        if (got == -ESP_ERR_HTTP_EAGAIN) {
            if (++eagain_streak > PANEL_OTA_MAX_EAGAIN_STREAK) {
                *failure = PANEL_OTA_STAGE_INCOMPLETE;
                return false;
            }
            continue;
        }
        if (got <= 0) {
            *failure = got == 0 ? PANEL_OTA_STAGE_INCOMPLETE :
                PANEL_OTA_STAGE_TRANSPORT_ERROR;
            return false;
        }
        if (got != 1) {
            *failure = PANEL_OTA_STAGE_TRANSPORT_ERROR;
            return false;
        }
        received++;
        eagain_streak = 0U;
    }
    if (received != PANEL_OTA_PREAMBLE_SIZE || !transfer_time_remaining(started)) {
        *failure = PANEL_OTA_STAGE_INCOMPLETE;
        return false;
    }
    return true;
}

/* fetch_headers() itself returns on EAGAIN (IDF 6.0.1). We supply a single
 * deadline and a finite retry budget over its repeated attempts. */
static bool fetch_bounded_headers(
    esp_http_client_handle_t client, int64_t started,
    const panel_ota_stage_access_t *access, int64_t *content_length,
    panel_ota_stage_result_t *failure)
{
    for (unsigned attempt = 0U; attempt < PANEL_OTA_MAX_EAGAIN_STREAK; ++attempt) {
        if (!transfer_time_remaining(started)) {
            *failure = PANEL_OTA_STAGE_INCOMPLETE;
            return false;
        }
        if (!access_still_authorized(access)) {
            *failure = PANEL_OTA_STAGE_DENIED;
            return false;
        }
        const int64_t length = esp_http_client_fetch_headers(client);
        if (length == -(int64_t)ESP_ERR_HTTP_EAGAIN) continue;
        if (length < 0) {
            *failure = PANEL_OTA_STAGE_TRANSPORT_ERROR;
            return false;
        }
        if (esp_http_client_get_status_code(client) != 200 ||
            esp_http_client_is_chunked_response(client) ||
            length < (int64_t)PANEL_OTA_PREAMBLE_SIZE ||
            (uint64_t)length > PANEL_OTA_MAX_IMAGE_BYTES ||
            (uint64_t)length > 0x400000U) {
            *failure = PANEL_OTA_STAGE_RELEASE_REJECTED;
            return false;
        }
        if (!transfer_time_remaining(started)) {
            *failure = PANEL_OTA_STAGE_INCOMPLETE;
            return false;
        }
        *content_length = length;
        return true;
    }
    *failure = PANEL_OTA_STAGE_INCOMPLETE;
    return false;
}
#endif

panel_ota_stage_result_t panel_ota_https_stage(
    const panel_ota_stage_access_t *access,
    const panel_update_manifest_t *manifest)
{
#if !PANEL_OTA_HTTPS_STAGE_ENABLED
    (void)access;
    (void)manifest;
    return PANEL_OTA_STAGE_DISABLED;
#else
    esp_app_desc_t running_desc = {0};
    const esp_partition_t *next = NULL;
    if (!preflight(access, manifest, &running_desc, &next)) return PANEL_OTA_STAGE_DENIED;
    const int64_t started = esp_timer_get_time();
    if (started < 0) return PANEL_OTA_STAGE_INCOMPLETE;

    const esp_http_client_config_t http = {
        .url = manifest->image_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = PANEL_OTA_IO_TIMEOUT_MS,
        .disable_auto_redirect = true,
        .keep_alive_enable = false,
        .max_authorization_retries = -1,
        .buffer_size = PANEL_OTA_READ_BUFFER_SIZE,
    };
    esp_http_client_handle_t client = esp_http_client_init(&http);
    if (client == NULL) return PANEL_OTA_STAGE_TRANSPORT_ERROR;

    panel_ota_stage_result_t result = PANEL_OTA_STAGE_TRANSPORT_ERROR;
    esp_ota_handle_t ota = 0;
    bool ota_live = false;
    bool flash_session_live = false;
    uint8_t *buffer = NULL;
    uint8_t preamble[PANEL_OTA_PREAMBLE_SIZE] = {0};
    uint8_t image_sha256[32] = {0};
    psa_hash_operation_t sha_op = PSA_HASH_OPERATION_INIT;
    bool sha_live = false;
    int64_t total = 0;
    int64_t received = 0;
    unsigned eagain_streak = 0U;

    if (esp_http_client_set_method(client, HTTP_METHOD_GET) != ESP_OK ||
        esp_http_client_open(client, 0) != ESP_OK ||
        !transfer_time_remaining(started)) goto done;
    if (!fetch_bounded_headers(client, started, access, &total, &result)) goto done;
    if ((uint64_t)total != manifest->image_size_bytes) {
        result = PANEL_OTA_STAGE_RELEASE_REJECTED;
        goto done;
    }
    if (!read_bounded_preamble(client, preamble, started, access, &result)) goto done;
    if (!image_preamble_acceptable(preamble, total, &running_desc, manifest)) {
        result = PANEL_OTA_STAGE_RELEASE_REJECTED;
        goto done;
    }
    if (!access_still_authorized(access)) {
        result = PANEL_OTA_STAGE_DENIED;
        goto done;
    }
    if (!transfer_time_remaining(started)) {
        result = PANEL_OTA_STAGE_INCOMPLETE;
        goto done;
    }
    buffer = malloc(PANEL_OTA_READ_BUFFER_SIZE);
    if (buffer == NULL) {
        result = PANEL_OTA_STAGE_TRANSPORT_ERROR;
        goto done;
    }

    result = PANEL_OTA_STAGE_TRANSPORT_ERROR;
    /* First flash mutation. Never attempt this until single-connection
     * descriptor check, fixed size, TLS and all access checks have passed. */
    if (!panel_ota_supervisor_flash_session_begin()) {
        result = PANEL_OTA_STAGE_DENIED;
        goto done;
    }
    flash_session_live = true;
    if (!panel_ota_supervisor_flash_operation_begin()) {
        result = PANEL_OTA_STAGE_DENIED;
        goto done;
    }
    const esp_err_t begin_err = esp_ota_begin(next, (size_t)total, &ota);
    panel_ota_supervisor_native_operation_end();
    if (begin_err != ESP_OK) {
        result = PANEL_OTA_STAGE_TRANSPORT_ERROR;
        goto done;
    }
    ota_live = true;
    if (!transfer_time_remaining(started)) {
        result = PANEL_OTA_STAGE_INCOMPLETE;
        goto done;
    }
    if (!access_still_authorized(access)) {
        result = PANEL_OTA_STAGE_DENIED;
        goto done;
    }
    if (!panel_ota_supervisor_flash_operation_begin()) {
        result = PANEL_OTA_STAGE_DENIED;
        goto done;
    }
    const esp_err_t preamble_write_err = esp_ota_write(ota, preamble, sizeof(preamble));
    panel_ota_supervisor_native_operation_end();
    if (preamble_write_err != ESP_OK) goto done;
    sha_op = psa_hash_operation_init();
    sha_live = true;
    if (psa_crypto_init() != PSA_SUCCESS ||
        psa_hash_setup(&sha_op, PSA_ALG_SHA_256) != PSA_SUCCESS ||
        psa_hash_update(&sha_op, preamble, sizeof(preamble)) != PSA_SUCCESS) goto done;
    received = (int64_t)sizeof(preamble);

    while (received < total) {
        if (!transfer_time_remaining(started)) {
            result = PANEL_OTA_STAGE_INCOMPLETE;
            goto done;
        }
        if (!access_still_authorized(access)) {
            result = PANEL_OTA_STAGE_DENIED;
            goto done;
        }
        const int want = (int)((total - received) < PANEL_OTA_READ_BUFFER_SIZE
            ? (total - received) : PANEL_OTA_READ_BUFFER_SIZE);
        const int got = esp_http_client_read(client, (char *)buffer, want);
        if (got == -ESP_ERR_HTTP_EAGAIN) {
            if (++eagain_streak > PANEL_OTA_MAX_EAGAIN_STREAK) {
                result = PANEL_OTA_STAGE_INCOMPLETE;
                goto done;
            }
            continue;
        }
        if (got < 0) goto done;
        if (got == 0) {
            result = PANEL_OTA_STAGE_INCOMPLETE;
            goto done;
        }
        if (got > want) goto done;
        if (!transfer_time_remaining(started)) {
            result = PANEL_OTA_STAGE_INCOMPLETE;
            goto done;
        }
        if (!panel_ota_supervisor_flash_operation_begin()) {
            result = PANEL_OTA_STAGE_DENIED;
            goto done;
        }
        const esp_err_t write_err = esp_ota_write(ota, buffer, (size_t)got);
        panel_ota_supervisor_native_operation_end();
        if (write_err != ESP_OK) goto done;
        if (psa_hash_update(&sha_op, buffer, (size_t)got) != PSA_SUCCESS) goto done;
        received += got;
        eagain_streak = 0U;
    }
    if (!esp_http_client_is_complete_data_received(client) ||
        received != total || !transfer_time_remaining(started)) {
        result = PANEL_OTA_STAGE_INCOMPLETE;
        goto done;
    }
    if (!access_still_authorized(access)) {
        result = PANEL_OTA_STAGE_DENIED;
        goto done;
    }
    size_t digest_len = 0U;
    if (psa_hash_finish(&sha_op, image_sha256, sizeof(image_sha256), &digest_len) != PSA_SUCCESS ||
        digest_len != sizeof(image_sha256) ||
        memcmp(image_sha256, manifest->image_sha256, sizeof(image_sha256)) != 0) {
        result = PANEL_OTA_STAGE_RELEASE_REJECTED;
        goto done;
    }
    /* esp_ota_end consumes the handle, including on validation failure. It
     * performs image + signed-app verification under the required config. */
    if (!panel_ota_supervisor_flash_operation_begin()) {
        result = PANEL_OTA_STAGE_DENIED;
        goto done;
    }
    const esp_err_t finish_err = esp_ota_end(ota);
    panel_ota_supervisor_native_operation_end();
    ota_live = false;
    if (finish_err != ESP_OK) {
        result = PANEL_OTA_STAGE_FINALIZE_FAILED;
        goto done;
    }
    if (!transfer_time_remaining(started)) {
        result = PANEL_OTA_STAGE_INCOMPLETE;
        goto done;
    }
    if (!access_still_authorized(access)) {
        result = PANEL_OTA_STAGE_DENIED;
        goto done;
    }
    /* Only after full image, native signature verification, and final access
     * check do we mutate otadata to select the next OTA image. No reboot. */
    if (!panel_ota_supervisor_boot_selection_begin()) {
        result = PANEL_OTA_STAGE_DENIED;
        goto done;
    }
    const esp_err_t select_err = esp_ota_set_boot_partition(next);
    panel_ota_supervisor_boot_selection_complete(select_err == ESP_OK);
    flash_session_live = false;
    result = select_err == ESP_OK
        ? PANEL_OTA_STAGE_STAGED_NO_REBOOT : PANEL_OTA_STAGE_FINALIZE_FAILED;
    if (select_err == ESP_OK) {
        /* Native boot selection has returned and STAGED is recorded. Restart
         * now, before TLS teardown: cleanup is not allowed to delay the
         * already-required maintenance reboot. No OTA flash API is active. */
        esp_restart();
        for (;;) vTaskDelay(portMAX_DELAY);
    }

done:
    if (ota_live && panel_ota_supervisor_worker_authorized() &&
        panel_ota_supervisor_flash_cleanup_begin()) {
        (void)esp_ota_abort(ota);
        panel_ota_supervisor_native_operation_end();
    }
    if (flash_session_live) panel_ota_supervisor_flash_session_end();
    if (sha_live) (void)psa_hash_abort(&sha_op);
    memset(image_sha256, 0, sizeof(image_sha256));
    free(buffer);
    (void)esp_http_client_cleanup(client); /* closes TLS and frees client */
    return result;
#endif
}
