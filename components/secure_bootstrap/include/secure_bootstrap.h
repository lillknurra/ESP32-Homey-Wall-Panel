#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SECURE_BOOTSTRAP_CODE_LEN 12
#define SECURE_BOOTSTRAP_CODE_TTL_SECONDS 600
#define SECURE_BOOTSTRAP_WIPE_HOLD_MS 5000
#define SECURE_BOOTSTRAP_WIFI_MAX_RETRIES 5U
#define SECURE_BOOTSTRAP_WIFI_RETRY_WINDOW_MS 30000
#define SECURE_BOOTSTRAP_MAX_NETWORKS 16U
#define SECURE_BOOTSTRAP_SSID_MAX_LEN 32U
#define SECURE_BOOTSTRAP_WIFI_BACKUP_MAGIC 0x48505742U
#define SECURE_BOOTSTRAP_WIFI_BACKUP_VERSION 1U
#define SECURE_BOOTSTRAP_WIFI_BACKUP_PAYLOAD_MAX 256U

typedef enum {
    SECURE_BOOTSTRAP_OK = 0,
    SECURE_BOOTSTRAP_ERR_ARGUMENT,
    SECURE_BOOTSTRAP_ERR_EXPIRED,
    SECURE_BOOTSTRAP_ERR_MISMATCH,
    SECURE_BOOTSTRAP_ERR_CONSUMED,
    SECURE_BOOTSTRAP_ERR_PLATFORM,
} secure_bootstrap_status_t;

typedef enum {
    SECURE_BOOTSTRAP_WIFI_BOOT = 0,
    SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED,
    SECURE_BOOTSTRAP_WIFI_ONLINE,
    SECURE_BOOTSTRAP_WIFI_SAVED_CONNECTION_FAILED,
    SECURE_BOOTSTRAP_WIFI_PROVISIONING,
    SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE,
    SECURE_BOOTSTRAP_WIFI_CANDIDATE_CONNECTION_FAILED,
    SECURE_BOOTSTRAP_WIFI_PERSISTING,
    SECURE_BOOTSTRAP_WIFI_PERSIST_VERIFY_CONNECTING,
    SECURE_BOOTSTRAP_WIFI_ROLLING_BACK,
} secure_bootstrap_wifi_state_t;

typedef enum {
    SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED = 0,
    SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED,
    SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED,
    SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP,
    SECURE_BOOTSTRAP_WIFI_EVENT_USER_RETRY,
    SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE,
    SECURE_BOOTSTRAP_WIFI_EVENT_PROVISIONING_OPEN_FAILED,
    SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED,
    SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_ABORTED,
    SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP,
    SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_OK,
    SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_FAILED,
    SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_GOT_IP,
    SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_FAILED,
    SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_OK,
    SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED,
} secure_bootstrap_wifi_event_t;

typedef enum {
    SECURE_BOOTSTRAP_WIFI_ACTION_NONE = 0,
    SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT = 1U << 0,
    SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING = 1U << 1,
    SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING = 1U << 2,
    SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST = 1U << 3,
    SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED = 1U << 4,
    SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST_VERIFY = 1U << 5,
    SECURE_BOOTSTRAP_WIFI_ACTION_START_ROLLBACK = 1U << 6,
    SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE = 1U << 7,
} secure_bootstrap_wifi_action_t;

typedef enum {
    SECURE_BOOTSTRAP_WIFI_SECURITY_OPEN = 0,
    SECURE_BOOTSTRAP_WIFI_SECURITY_WEP,
    SECURE_BOOTSTRAP_WIFI_SECURITY_WPA,
    SECURE_BOOTSTRAP_WIFI_SECURITY_WPA2,
    SECURE_BOOTSTRAP_WIFI_SECURITY_WPA3,
    SECURE_BOOTSTRAP_WIFI_SECURITY_ENTERPRISE,
    SECURE_BOOTSTRAP_WIFI_SECURITY_UNKNOWN,
} secure_bootstrap_wifi_security_t;

typedef struct {
    secure_bootstrap_wifi_state_t state;
    unsigned retry_count;
    bool saved_config_present;
    bool candidate_active;
} secure_bootstrap_wifi_context_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t payload_size;
    uint32_t payload_crc32;
    uint8_t payload[SECURE_BOOTSTRAP_WIFI_BACKUP_PAYLOAD_MAX];
} secure_bootstrap_wifi_backup_blob_t;

typedef struct {
    char ssid[SECURE_BOOTSTRAP_SSID_MAX_LEN + 1U];
    int8_t rssi;
    secure_bootstrap_wifi_security_t security;
} secure_bootstrap_network_t;

typedef struct {
    char code[SECURE_BOOTSTRAP_CODE_LEN + 1];
    int64_t issued_at_s;
    bool consumed;
} secure_bootstrap_code_t;

typedef struct {
    int64_t pressed_since_ms;
    bool triggered;
} secure_bootstrap_wipe_tracker_t;

typedef struct {
    bool display_ready;
    bool touch_ready;
    bool softap_ready;
    bool wifi_configured;
    bool ip_obtained;
    bool oauth_locked;
} secure_bootstrap_status_snapshot_t;

secure_bootstrap_status_t secure_bootstrap_code_generate(secure_bootstrap_code_t *state, int64_t now_s, const uint8_t *random_bytes, size_t random_len);
secure_bootstrap_status_t secure_bootstrap_code_verify_and_consume(secure_bootstrap_code_t *state, const char *candidate, int64_t now_s);
bool secure_bootstrap_code_rotation_due(const secure_bootstrap_code_t *state, int64_t now_s);
void secure_bootstrap_code_wipe(secure_bootstrap_code_t *state);
bool secure_bootstrap_text_contains_code(const char *text, const secure_bootstrap_code_t *state);
void secure_bootstrap_wipe_tracker_reset(secure_bootstrap_wipe_tracker_t *tracker);
bool secure_bootstrap_wipe_tracker_update(secure_bootstrap_wipe_tracker_t *tracker, bool pressed, int64_t now_ms);

void secure_bootstrap_wifi_context_init(secure_bootstrap_wifi_context_t *context);
uint32_t secure_bootstrap_wifi_transition(secure_bootstrap_wifi_context_t *context, secure_bootstrap_wifi_event_t event);
size_t secure_bootstrap_network_insert(secure_bootstrap_network_t *networks, size_t count, size_t capacity, const char *ssid, int8_t rssi, secure_bootstrap_wifi_security_t security);
const char *secure_bootstrap_signal_label(int8_t rssi);
const char *secure_bootstrap_security_label(secure_bootstrap_wifi_security_t security);
bool secure_bootstrap_html_escape(const char *source, char *destination, size_t destination_size);
uint32_t secure_bootstrap_crc32(const void *data, size_t size);
bool secure_bootstrap_wifi_backup_encode(secure_bootstrap_wifi_backup_blob_t *blob, const void *payload, size_t payload_size);
bool secure_bootstrap_wifi_backup_decode(const secure_bootstrap_wifi_backup_blob_t *blob, void *payload, size_t payload_capacity);

#ifdef ESP_PLATFORM
secure_bootstrap_status_t secure_bootstrap_start(void);
#endif
