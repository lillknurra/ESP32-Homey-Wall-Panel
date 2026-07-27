#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PHONE_PROV_STATE_BYTES 32U
#define PHONE_PROV_TOKEN_MAX 96U
#define PHONE_PROV_HOMEY_ID_MAX 64U
#define PHONE_PROV_HOMEY_NAME_MAX 48U
#define PHONE_PROV_MAX_HOMEYS 4U
#define PHONE_PROV_SESSION_TTL_SECONDS 600
#define PHONE_PROV_RECORD_MAGIC 0x48504155U
#define PHONE_PROV_RECORD_VERSION 1U

typedef enum {
    PHONE_PROV_UNPROVISIONED = 0,
    PHONE_PROV_PORTAL_ACTIVE,
    PHONE_PROV_AUTH_PENDING,
    PHONE_PROV_HOMEY_SELECTION_REQUIRED,
    PHONE_PROV_READY,
    PHONE_PROV_REPROVISION_REQUIRED,
} phone_prov_state_t;

typedef enum {
    PHONE_PROV_OK = 0,
    PHONE_PROV_ERR_ARGUMENT,
    PHONE_PROV_ERR_STATE,
    PHONE_PROV_ERR_EXPIRED,
    PHONE_PROV_ERR_MISMATCH,
    PHONE_PROV_ERR_CONSUMED,
    PHONE_PROV_ERR_DUPLICATE,
    PHONE_PROV_ERR_STALE,
    PHONE_PROV_ERR_STORAGE,
} phone_prov_result_t;

typedef struct {
    char id[PHONE_PROV_HOMEY_ID_MAX];
    char name[PHONE_PROV_HOMEY_NAME_MAX];
} phone_prov_homey_t;

typedef struct {
    char access_token[PHONE_PROV_TOKEN_MAX];
    char refresh_token[PHONE_PROV_TOKEN_MAX];
    int64_t expires_at;
    phone_prov_homey_t homeys[PHONE_PROV_MAX_HOMEYS];
    size_t homey_count;
} phone_prov_oauth_result_t;

typedef struct {
    phone_prov_state_t state;
    uint8_t oauth_state[PHONE_PROV_STATE_BYTES];
    int64_t issued_at_s;
    bool consumed;
    phone_prov_oauth_result_t result;
} phone_prov_context_t;

typedef struct {
    uint32_t magic;
    uint16_t schema_version;
    uint16_t payload_size;
    uint32_t generation;
    uint32_t payload_crc32;
    int64_t expires_at;
    uint8_t discovery_strategy;
    char access_token[PHONE_PROV_TOKEN_MAX];
    char refresh_token[PHONE_PROV_TOKEN_MAX];
    char homey_id[PHONE_PROV_HOMEY_ID_MAX];
    char homey_name[PHONE_PROV_HOMEY_NAME_MAX];
} phone_prov_record_v1_t;

typedef struct {
    phone_prov_result_t (*complete)(phone_prov_oauth_result_t *out);
    void (*wipe)(phone_prov_oauth_result_t *out);
} phone_prov_provider_t;

void phone_prov_init(phone_prov_context_t *ctx);
phone_prov_result_t phone_prov_open(phone_prov_context_t *ctx);
phone_prov_result_t phone_prov_begin(phone_prov_context_t *ctx, int64_t now_s, const uint8_t random_state[PHONE_PROV_STATE_BYTES]);
phone_prov_result_t phone_prov_complete(phone_prov_context_t *ctx, int64_t now_s, const uint8_t candidate[PHONE_PROV_STATE_BYTES], const phone_prov_provider_t *provider);
phone_prov_result_t phone_prov_select(phone_prov_context_t *ctx, const char *homey_id, phone_prov_record_v1_t *record, uint32_t generation);
phone_prov_result_t phone_prov_change(phone_prov_context_t *ctx);
phone_prov_result_t phone_prov_change_refresh(phone_prov_context_t *ctx,const phone_prov_provider_t *provider);
void phone_prov_wipe_context(phone_prov_context_t *ctx);
bool phone_prov_record_valid(const phone_prov_record_v1_t *record);
bool phone_prov_publication_readback_valid(const phone_prov_record_v1_t *expected, const phone_prov_record_v1_t *actual, bool present);
uint32_t phone_prov_crc32(const void *data, size_t size);
const phone_prov_provider_t *phone_prov_mock_provider(void);

#ifdef ESP_PLATFORM
#include "esp_err.h"
#include "esp_http_server.h"
typedef void (*phone_prov_display_cb_t)(const char *title, const char *detail, const char *code);
void phone_provisioning_set_display_callback(phone_prov_display_cb_t cb);
void phone_provisioning_on_wifi_online(void);
bool phone_provisioning_display_ready_rendered(void);
void phone_provisioning_set_portal_css(const char *css);
esp_err_t phone_provisioning_register_handlers(httpd_handle_t server);
esp_err_t phone_provisioning_boot_restore(void);
#endif
