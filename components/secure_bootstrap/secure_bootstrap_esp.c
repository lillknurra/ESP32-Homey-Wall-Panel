#include "secure_bootstrap.h"
#include "phone_provisioning.h"
#include "panel_ui.h"
#include "homey_panel_font_22.h"
#ifdef ESP_PLATFORM
#include "bsp/esp32_s3_touch_lcd_4b.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lwip/sockets.h"
#include "nvs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AP_SSID "HomeyPanel-Setup"
#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define WIPE_POLL_INTERVAL_MS 100
#define ROTATION_POLL_INTERVAL_MS 250
#define MAX_FORM_BODY 512
#define QR_PAYLOAD_CAPACITY 96
#define DISPLAY_WIDTH 480
#define DISPLAY_MARGIN 18
#define TITLE_Y 12
#define DETAIL_Y 58
#define CODE_CAPTION_Y 134
#define CODE_Y 168
#define TOUCH_Y 214
#define QR_SIZE 180
#define QR_BOTTOM_MARGIN 8

static const char *TAG = "secure_bootstrap";
static const char PORTAL_SHARED_CSS[] =
    "body{font:18px system-ui;max-width:34rem;margin:2rem auto;padding:1rem;background:#f6f8fb;color:#16202a}"
    "main,.panel{background:#fff;border-radius:1rem;padding:1.4rem;box-shadow:0 .3rem 1.2rem rgba(0,0,0,.08)}"
    "h1,h2{margin-top:0}p{line-height:1.5}label{display:block;margin:.7rem 0}"
    "select,input,button{box-sizing:border-box;font:inherit;width:100%;padding:.7rem;margin:.4rem 0;border-radius:.55rem}"
    "button{white-space:normal;line-height:1.25;min-height:3.2rem;border:0;background:#1f5f99;color:#fff;font-weight:650}"
    "button:disabled{opacity:.45;cursor:not-allowed}.help{display:block;min-height:1.3em;color:#9b1c1c;font-size:.9rem}"
    ".check{display:flex;gap:.6rem;align-items:center}.check input{width:auto}.notice{padding:1rem;border-radius:.75rem;background:#eef5fb;margin:1rem 0}"
    ".testmode{padding:1rem;border-radius:.75rem;background:#fff3cd;border:1px solid #e4bd53;margin:1rem 0}"
    ".pass{color:#146c43;font-weight:700}.fail{color:#b02a37;font-weight:700}.danger{background:#a61b29}.muted{color:#5b6670;font-size:.92rem}";

static const char *portal_shared_css(void) { return PORTAL_SHARED_CSS; }

static secure_bootstrap_code_t s_code;
static secure_bootstrap_wifi_context_t s_wifi;
static httpd_handle_t s_server;
static bool s_homey_handlers_registered;
static esp_netif_t *s_ap_netif;
static esp_netif_t *s_sta_netif;
static lv_obj_t *s_title;
static lv_obj_t *s_detail;
static lv_obj_t *s_code_caption;
static lv_obj_t *s_code_label;
static lv_obj_t *s_qr;
static lv_obj_t *s_touch_button;
static lv_obj_t *s_provisioning_screen;
static panel_ui_model_t s_panel_model;
static panel_ui_t *s_panel_ui;
static wifi_config_t s_saved_config, s_candidate_config;
static secure_bootstrap_network_t s_networks[SECURE_BOOTSTRAP_MAX_NETWORKS];
static size_t s_network_count;
static bool s_scan_in_progress;
static bool s_saved_valid, s_candidate_valid, s_ip_obtained;
static bool s_wifi_nvs_enabled;
typedef enum {
    WIFI_PERSIST_STAGE_IDLE = 0,
    WIFI_PERSIST_STAGE_WRITING,
    WIFI_PERSIST_STAGE_VERIFY_CONNECT,
    WIFI_PERSIST_STAGE_ROLLBACK,
} wifi_persist_stage_t;
static bool s_persist_task_scheduled;
static bool s_persist_active;
static bool s_rollback_attempted;
static bool s_reconfigure_pending;
static wifi_persist_stage_t s_persist_stage;
static int64_t s_last_code_countdown_s = -1;

static int64_t now_s(void) { return esp_timer_get_time() / 1000000LL; }
static void secure_zero(void *buffer, size_t size) { volatile unsigned char *p = buffer; while (size--) *p++ = 0U; }
static bool has_saved_ssid(const wifi_config_t *config) { return config != NULL && config->sta.ssid[0] != 0U; }
static bool ssid_has_any_nonzero(const wifi_config_t *config)
{
    if (config == NULL) return false;
    for (size_t i = 0; i < sizeof(config->sta.ssid); ++i) {
        if (config->sta.ssid[i] != 0U) return true;
    }
    return false;
}

static void format_code_caption(char *caption, size_t caption_size, int64_t current_s)
{
    int64_t remaining_s =
        s_code.issued_at_s + SECURE_BOOTSTRAP_CODE_TTL_SECONDS - current_s;
    if (remaining_s < 0) {
        remaining_s = 0;
    }
    int64_t minutes = remaining_s / 60;
    int64_t seconds = remaining_s % 60;
    (void)snprintf(
        caption,
        caption_size,
        "Panelkod (%02lld:%02lld):",
        (long long)minutes,
        (long long)seconds);
}

static void refresh_code_countdown(void)
{
    if (s_wifi.state != SECURE_BOOTSTRAP_WIFI_PROVISIONING ||
        s_code.code[0] == '\0') {
        s_last_code_countdown_s = -1;
        return;
    }

    int64_t current_s = now_s();
    int64_t remaining_s =
        s_code.issued_at_s + SECURE_BOOTSTRAP_CODE_TTL_SECONDS - current_s;
    if (remaining_s < 0) {
        remaining_s = 0;
    }
    if (remaining_s == s_last_code_countdown_s) {
        return;
    }

    char caption[32];
    format_code_caption(caption, sizeof(caption), current_s);
    if (bsp_display_lock(1000)) {
        lv_label_set_text(s_code_caption, caption);
        bsp_display_unlock();
        s_last_code_countdown_s = remaining_s;
    }
    secure_zero(caption, sizeof(caption));
}

static void set_display_text(
    const char *title,
    const char *detail,
    const char *display_code)
{
    if (!bsp_display_lock(1000)) {
        return;
    }

    bool bootstrap_visible = display_code != NULL && display_code[0] != '\0';
    lv_label_set_text(s_title, title != NULL ? title : "");
    lv_label_set_text(s_detail, detail != NULL ? detail : "");
    if (bootstrap_visible) {
        char caption[32];
        format_code_caption(caption, sizeof(caption), now_s());
        lv_label_set_text(s_code_caption, caption);
        secure_zero(caption, sizeof(caption));
    } else {
        lv_label_set_text(s_code_caption, "");
        s_last_code_countdown_s = -1;
    }
    lv_label_set_text(s_code_label, display_code != NULL ? display_code : "");
    if (s_qr != NULL) {
        if (bootstrap_visible) {
            char payload[QR_PAYLOAD_CAPACITY];
            int written = snprintf(
                payload,
                sizeof(payload),
                "WIFI:T:WPA;S:%s;P:%s;;",
                AP_SSID,
                display_code);
            if (written > 0 && (size_t)written < sizeof(payload)) {
                lv_qrcode_update(s_qr, payload, (size_t)written);
                lv_obj_remove_flag(s_qr, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(s_qr, LV_OBJ_FLAG_HIDDEN);
            }
            secure_zero(payload, sizeof(payload));
        } else {
            lv_obj_add_flag(s_qr, LV_OBJ_FLAG_HIDDEN);
        }
    }

    bsp_display_unlock();
}

static esp_err_t apply_actions(uint32_t actions);
static void reconfigure_task(void *arg);
static void panel_brightness_request(void *context, uint8_t value)
{
    (void)context;
    (void)bsp_display_brightness_set((int)value);
}

static void panel_wifi_request(void *context)
{
    (void)context;
    if (s_reconfigure_pending || s_wifi.state != SECURE_BOOTSTRAP_WIFI_ONLINE) return;
    s_reconfigure_pending = true;
    if (xTaskCreate(reconfigure_task, "wifi_reconfigure", 4096, NULL, 5, NULL) != pdPASS) {
        s_reconfigure_pending = false;
    }
}

static void panel_choose_request(void *context)
{
    (void)context;
    ESP_LOGI(TAG, "PANEL_UI choose_homey_supported=false package=3");
}

static void panel_wipe_request(void *context)
{
    (void)context;
    ESP_LOGI(TAG, "PANEL_UI homey_wipe_supported=false package=3 mutation=false");
}

static void panel_account_request(void *context)
{
    (void)context;
    ESP_LOGI(TAG, "PANEL_UI change_athom_supported=false package=3 mutation=false");
}

static void panel_settings_request(void *context, const panel_ui_settings_t *settings)
{
    (void)context;
    (void)settings;
    ESP_LOGI(TAG, "PANEL_UI settings_changed=true persistence=false package=3");
}

static bool panel_show_dashboard(void)
{
    if (s_panel_ui == NULL || !bsp_display_lock(1000)) return false;
    panel_ui_connection_info_t connection = {
        .state = PANEL_UI_CONNECTION_CONNECTED,
        .display_name = "",
    };
    bool ok = panel_ui_activate(s_panel_ui);
    (void)panel_ui_set_connection(s_panel_ui, &connection);
    (void)panel_ui_set_time(s_panel_ui, NULL, false);
    (void)panel_ui_refresh(s_panel_ui);
    bsp_display_unlock();
    return ok;
}


static void set_reconfigure_button_visible(bool visible)
{
    if (s_touch_button == NULL || !bsp_display_lock(1000)) {
        return;
    }
    if (visible) {
        lv_obj_remove_state(s_touch_button, LV_STATE_DISABLED);
        lv_obj_remove_flag(s_touch_button, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_state(s_touch_button, LV_STATE_DISABLED);
        lv_obj_add_flag(s_touch_button, LV_OBJ_FLAG_HIDDEN);
    }
    bsp_display_unlock();
}

static void reconfigure_task(void *arg)
{
    (void)arg;
    uint32_t actions = secure_bootstrap_wifi_transition(
        &s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE);
    if ((actions & SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING) == 0U) {
        s_reconfigure_pending = false;
        if (s_wifi.state == SECURE_BOOTSTRAP_WIFI_ONLINE) {
            set_reconfigure_button_visible(true);
        }
        vTaskDelete(NULL);
        return;
    }

    esp_err_t result = apply_actions(actions);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "WIFI_RECONFIGURE open_result=%s", esp_err_to_name(result));
        uint32_t recovery = secure_bootstrap_wifi_transition(
            &s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_PROVISIONING_OPEN_FAILED);
        (void)apply_actions(recovery);
    } else {
        ESP_LOGI(TAG, "WIFI_RECONFIGURE requested=true");
    }
    vTaskDelete(NULL);
}

static void touch_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED ||
        s_reconfigure_pending ||
        s_wifi.state != SECURE_BOOTSTRAP_WIFI_ONLINE) {
        return;
    }

    s_reconfigure_pending = true;
    lv_obj_add_state(s_touch_button, LV_STATE_DISABLED);
    lv_obj_add_flag(s_touch_button, LV_OBJ_FLAG_HIDDEN);

    if (xTaskCreate(
            reconfigure_task,
            "wifi_reconfigure",
            4096,
            NULL,
            5,
            NULL) != pdPASS) {
        s_reconfigure_pending = false;
        lv_obj_remove_state(s_touch_button, LV_STATE_DISABLED);
        lv_obj_remove_flag(s_touch_button, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGE(TAG, "WIFI_RECONFIGURE task_create=false");
    }
}

static esp_err_t connect_current(void) { esp_err_t err = esp_wifi_connect(); if (err != ESP_OK) ESP_LOGW(TAG, "Wi-Fi connect scheduling failed: %s", esp_err_to_name(err)); return err; }
static esp_err_t server_start(void);
static esp_err_t ensure_homey_server(void);
static esp_err_t open_provisioning(void);

#define WIFI_BACKUP_NAMESPACE "hpanel_wifi"
#define WIFI_BACKUP_KEY "backup_v1"
#define WIFI_SENTINEL_NAMESPACE "p007diag"
#define WIFI_SENTINEL_KEY "commit_seen"

static esp_err_t backup_clear(void)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(WIFI_BACKUP_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_erase_key(handle, WIFI_BACKUP_KEY);
    if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG, "WIFI_BACKUP clear_result=%s", esp_err_to_name(err));
    return err;
}

static void legacy_sentinel_clear(void)
{
    nvs_handle_t handle;
    if (nvs_open(WIFI_SENTINEL_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return;
    esp_err_t err = nvs_erase_key(handle, WIFI_SENTINEL_KEY);
    if (err == ESP_OK) (void)nvs_commit(handle);
    nvs_close(handle);
}

static esp_err_t backup_write_saved(void)
{
    if (!s_saved_valid) return backup_clear();
    if (sizeof(wifi_config_t) > SECURE_BOOTSTRAP_WIFI_BACKUP_PAYLOAD_MAX) return ESP_ERR_INVALID_SIZE;
    secure_bootstrap_wifi_backup_blob_t blob;
    if (!secure_bootstrap_wifi_backup_encode(&blob, &s_saved_config, sizeof(s_saved_config))) return ESP_FAIL;
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(WIFI_BACKUP_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) err = nvs_set_blob(handle, WIFI_BACKUP_KEY, &blob, sizeof(blob));
    if (err == ESP_OK) err = nvs_commit(handle);
    if (err == ESP_OK) {
        size_t size = sizeof(blob);
        secure_bootstrap_wifi_backup_blob_t verify;
        err = nvs_get_blob(handle, WIFI_BACKUP_KEY, &verify, &size);
        if (err == ESP_OK && !secure_bootstrap_wifi_backup_decode(&verify, &s_saved_config, sizeof(s_saved_config))) err = ESP_ERR_INVALID_CRC;
        secure_zero(&verify, sizeof(verify));
    }
    if (handle != 0) nvs_close(handle);
    secure_zero(&blob, sizeof(blob));
    ESP_LOGI(TAG, "WIFI_BACKUP write_result=%s", esp_err_to_name(err));
    return err;
}

static esp_err_t backup_read(wifi_config_t *config, bool *present)
{
    if (config == NULL || present == NULL) return ESP_ERR_INVALID_ARG;
    *present = false;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_BACKUP_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK) return err;
    secure_bootstrap_wifi_backup_blob_t blob;
    size_t size = sizeof(blob);
    err = nvs_get_blob(handle, WIFI_BACKUP_KEY, &blob, &size);
    nvs_close(handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK || size != sizeof(blob) ||
        !secure_bootstrap_wifi_backup_decode(&blob, config, sizeof(*config)) ||
        !has_saved_ssid(config)) {
        secure_zero(&blob, sizeof(blob));
        secure_zero(config, sizeof(*config));
        return err == ESP_OK ? ESP_ERR_INVALID_CRC : err;
    }
    secure_zero(&blob, sizeof(blob));
    *present = true;
    return ESP_OK;
}

static esp_err_t driver_init_flash(void)
{
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init), TAG, "persist init");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "persist mode");
    return esp_wifi_set_storage(WIFI_STORAGE_FLASH);
}

static esp_err_t persist_write_and_reopen(wifi_config_t *config, wifi_config_t *readback)
{
    esp_err_t err = esp_wifi_stop();
    ESP_LOGI(TAG, "WIFI_PERSIST stop_result=%s", esp_err_to_name(err));
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_STARTED) return err;
    err = esp_wifi_deinit();
    ESP_LOGI(TAG, "WIFI_PERSIST deinit_result=%s", esp_err_to_name(err));
    if (err != ESP_OK) return err;
    err = driver_init_flash();
    ESP_LOGI(TAG, "WIFI_PERSIST init_result=%s", esp_err_to_name(err));
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "WIFI_PERSIST storage_result=%s", esp_err_to_name(ESP_OK));
    err = esp_wifi_set_config(WIFI_IF_STA, config);
    ESP_LOGI(TAG, "WIFI_PERSIST config_result=%s", esp_err_to_name(err));
    if (err != ESP_OK) return err;
    err = esp_wifi_deinit();
    ESP_LOGI(TAG, "WIFI_PERSIST verify_deinit_result=%s", esp_err_to_name(err));
    if (err != ESP_OK) return err;
    err = driver_init_flash();
    ESP_LOGI(TAG, "WIFI_PERSIST verify_init_result=%s", esp_err_to_name(err));
    if (err != ESP_OK) return err;
    memset(readback, 0, sizeof(*readback));
    err = esp_wifi_get_config(WIFI_IF_STA, readback);
    bool valid = err == ESP_OK && has_saved_ssid(readback);
    ESP_LOGI(TAG, "WIFI_PERSIST verify_get_result=%s", esp_err_to_name(err));
    ESP_LOGI(TAG, "WIFI_PERSIST verify_valid=%s", valid ? "true" : "false");
    return err != ESP_OK ? err : (valid ? ESP_OK : ESP_ERR_INVALID_STATE);
}

static void persist_fail(const char *step, esp_err_t err)
{
    ESP_LOGE(TAG, "WIFI_PERSIST failed step=%s result=%s", step, esp_err_to_name(err));
}

static esp_err_t rollback_once(void)
{
    if (s_rollback_attempted) return ESP_ERR_INVALID_STATE;
    s_rollback_attempted = true;
    s_persist_stage = WIFI_PERSIST_STAGE_ROLLBACK;
    ESP_LOGI(TAG, "WIFI_ROLLBACK begin");
    wifi_config_t backup = {0};
    wifi_config_t readback = {0};
    bool present = false;
    esp_err_t err = backup_read(&backup, &present);
    ESP_LOGI(TAG, "WIFI_ROLLBACK backup_present=%s", present ? "true" : "false");
    if (err != ESP_OK || !present) {
        secure_zero(&backup, sizeof(backup));
        return err != ESP_OK ? err : ESP_ERR_NOT_FOUND;
    }
    err = persist_write_and_reopen(&backup, &readback);
    ESP_LOGI(TAG, "WIFI_ROLLBACK restore_result=%s", esp_err_to_name(err));
    if (err == ESP_OK) {
        s_saved_config = readback;
        s_saved_valid = true;
        err = esp_wifi_start();
        if (err == ESP_OK) err = esp_wifi_connect();
        ESP_LOGI(TAG, "WIFI_ROLLBACK connect_result=%s", esp_err_to_name(err));
    }
    secure_zero(&backup, sizeof(backup));
    secure_zero(&readback, sizeof(readback));
    if (err == ESP_OK) ESP_LOGI(TAG, "WIFI_ROLLBACK complete");
    else ESP_LOGE(TAG, "WIFI_ROLLBACK failed step=restore result=%s", esp_err_to_name(err));
    return err;
}

static void persistent_commit_task(void *arg)
{
    (void)arg;
    s_persist_active = true;
    s_persist_stage = WIFI_PERSIST_STAGE_WRITING;
    ESP_LOGI(TAG, "WIFI_PERSIST begin");
    if (s_server != NULL) { (void)httpd_stop(s_server); s_server = NULL; }
    esp_err_t err = backup_write_saved();
    ESP_LOGI(TAG, "WIFI_PERSIST backup_result=%s", esp_err_to_name(err));
    wifi_config_t readback = {0};
    if (err == ESP_OK) err = persist_write_and_reopen(&s_candidate_config, &readback);
    if (err == ESP_OK) {
        s_saved_config = readback;
        s_saved_valid = true;
        uint32_t actions = secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_OK);
        (void)actions;
        s_persist_stage = WIFI_PERSIST_STAGE_VERIFY_CONNECT;
        err = esp_wifi_start();
        ESP_LOGI(TAG, "WIFI_PERSIST start_result=%s", esp_err_to_name(err));
        if (err == ESP_OK) err = esp_wifi_connect();
        ESP_LOGI(TAG, "WIFI_PERSIST connect_result=%s", esp_err_to_name(err));
        if (err == ESP_OK) { secure_zero(&readback, sizeof(readback)); vTaskDelete(NULL); return; }
    }
    persist_fail("commit", err);
    (void)secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_FAILED);
    esp_err_t rollback_err = rollback_once();
    if (rollback_err == ESP_OK) {
        (void)secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_OK);
        s_persist_active = false;
    } else {
        (void)secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED);
        s_persist_active = false;
        if (esp_wifi_start() == ESP_OK) (void)open_provisioning();
    }
    secure_zero(&readback, sizeof(readback));
    vTaskDelete(NULL);
}

static esp_err_t schedule_persistent_commit(void)
{
    if (s_persist_task_scheduled || s_persist_active) return ESP_ERR_INVALID_STATE;
    s_persist_task_scheduled = true;
    if (xTaskCreate(persistent_commit_task, "wifi_persist", 6144, NULL, 5, NULL) != pdPASS) {
        s_persist_task_scheduled = false;
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "WIFI_PERSIST scheduled=true");
    return ESP_OK;
}

static esp_err_t configure_access_point(const char *password)
{
    wifi_config_t ap = {0};
    if (!password || strlen(password) != SECURE_BOOTSTRAP_CODE_LEN) return ESP_ERR_INVALID_ARG;
    snprintf((char *)ap.ap.ssid, sizeof(ap.ap.ssid), "%s", AP_SSID);
    snprintf((char *)ap.ap.password, sizeof(ap.ap.password), "%s", password);
    ap.ap.ssid_len = strlen(AP_SSID); ap.ap.channel = 1; ap.ap.max_connection = 2; ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &ap); secure_zero(&ap, sizeof(ap)); return err;
}

static esp_err_t generate_code(void)
{
    uint8_t random_bytes[SECURE_BOOTSTRAP_CODE_LEN]; esp_fill_random(random_bytes, sizeof(random_bytes));
    secure_bootstrap_status_t r = secure_bootstrap_code_generate(&s_code, now_s(), random_bytes, sizeof(random_bytes));
    secure_zero(random_bytes, sizeof(random_bytes)); return r == SECURE_BOOTSTRAP_OK ? ESP_OK : ESP_FAIL;
}

static esp_err_t start_network_scan(void);
static size_t consume_network_scan_results(void);

static esp_err_t open_provisioning(void)
{
    ESP_LOGI(TAG, "WIFI_TRACE portal_begin");
    if (s_code.code[0] == '\0' || s_code.consumed) {
        if (generate_code() != ESP_OK) {
            return ESP_FAIL;
        }
    }

    set_display_text(
        "Säker Wi-Fi-installation",
        "Anslut till HomeyPanel-Setup\nÖppna 192.168.4.1",
        s_code.code);
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "APSTA mode");
    ESP_RETURN_ON_ERROR(configure_access_point(s_code.code), TAG, "AP config");
    ESP_RETURN_ON_ERROR(ensure_homey_server(), TAG, "Homey provisioning server");
    (void)start_network_scan();
    ESP_LOGI(
        TAG,
        "WIFI_PROVISIONING_OPEN networks=%u sensitive_values_logged=false",
        (unsigned)s_network_count);
    ESP_LOGI(TAG, "WIFI_TRACE provisioning_ready");
    return ESP_OK;
}

static void close_provisioning(void)
{
    if (s_server) { httpd_stop(s_server); s_server = NULL; }
    s_homey_handlers_registered = false;
    (void)esp_wifi_set_mode(WIFI_MODE_STA);
    secure_bootstrap_code_wipe(&s_code);
}

static esp_err_t restore_saved_ram(void)
{
    if (!s_saved_valid) return ESP_ERR_NOT_FOUND;
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "RAM storage");
    return esp_wifi_set_config(WIFI_IF_STA, &s_saved_config);
}

static esp_err_t apply_actions(uint32_t actions)
{
    esp_err_t err = ESP_OK;
    if (actions & SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED) {
        err = restore_saved_ram();
        if (err != ESP_OK) return err;
        s_candidate_valid = false;
        secure_zero(&s_candidate_config, sizeof(s_candidate_config));
    }
    if (actions & SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING) {
        set_reconfigure_button_visible(false);
        err = open_provisioning();
        if (err != ESP_OK) return err;
    }
    if (actions & SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT) {
        err = connect_current();
        if (err != ESP_OK) return err;
    }
    if (actions & SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST) {
        err = schedule_persistent_commit();
        if (err != ESP_OK) return err;
    }
    if (actions & SECURE_BOOTSTRAP_WIFI_ACTION_CLOSE_PROVISIONING) close_provisioning();
    if (actions & SECURE_BOOTSTRAP_WIFI_ACTION_SHOW_ONLINE) {
        err = ensure_homey_server();
        if (err != ESP_OK) return err;
        ESP_LOGI(TAG, "PHONE_PROV lan_portal_ready=true");
        s_reconfigure_pending = false;
        phone_provisioning_on_wifi_online();
        set_reconfigure_button_visible(true);
        (void)panel_show_dashboard();
    }
    return ESP_OK;
}


static esp_err_t display_init(void)
{
    lv_display_t *display = bsp_display_start();
    if (display == NULL) {
        return ESP_FAIL;
    }

    ESP_RETURN_ON_ERROR(bsp_display_brightness_set(85), TAG, "display brightness");

    if (!bsp_display_lock(0)) {
        return ESP_FAIL;
    }

    lv_obj_t *screen = lv_screen_active();
    s_provisioning_screen = screen;
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x071018), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(screen, LV_OPA_COVER, 0);

    s_title = lv_label_create(screen);
    lv_label_set_text(s_title, "");
    lv_obj_set_width(s_title, DISPLAY_WIDTH - (2 * DISPLAY_MARGIN));
    lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, TITLE_Y);
    lv_label_set_long_mode(s_title, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(s_title, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_title, &homey_panel_font_22, 0);

    s_detail = lv_label_create(screen);
    lv_label_set_text(s_detail, "");
    lv_obj_set_width(s_detail, DISPLAY_WIDTH - (2 * DISPLAY_MARGIN));
    lv_obj_align(s_detail, LV_ALIGN_TOP_MID, 0, DETAIL_Y);
    lv_label_set_long_mode(s_detail, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_detail, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_detail, lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_text_opa(s_detail, LV_OPA_COVER, 0);
    lv_obj_set_style_text_line_space(s_detail, 5, 0);
    lv_obj_set_style_text_font(s_detail, &homey_panel_font_22, 0);

    s_code_caption = lv_label_create(screen);
    lv_label_set_text(s_code_caption, "");
    lv_obj_set_width(s_code_caption, DISPLAY_WIDTH - (2 * DISPLAY_MARGIN));
    lv_obj_align(s_code_caption, LV_ALIGN_TOP_MID, 0, CODE_CAPTION_Y);
    lv_obj_set_style_text_align(s_code_caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_code_caption, lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_text_opa(s_code_caption, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_code_caption, &homey_panel_font_22, 0);

    s_code_label = lv_label_create(screen);
    lv_label_set_text(s_code_label, "");
    lv_obj_set_width(s_code_label, DISPLAY_WIDTH - (2 * DISPLAY_MARGIN));
    lv_obj_align(s_code_label, LV_ALIGN_TOP_MID, 0, CODE_Y);
    lv_obj_set_style_text_align(s_code_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_code_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(s_code_label, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_code_label, &lv_font_montserrat_28, 0);

    s_touch_button = lv_button_create(screen);
    lv_obj_set_size(s_touch_button, 190, 52);
    lv_obj_align(s_touch_button, LV_ALIGN_TOP_MID, 0, TOUCH_Y);
    lv_obj_set_style_bg_color(s_touch_button, lv_color_hex(0x1976D2), 0);
    lv_obj_set_style_bg_opa(s_touch_button, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(s_touch_button, touch_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *touch_label = lv_label_create(s_touch_button);
    lv_label_set_text(touch_label, "Ändra Wi-Fi");
    lv_obj_set_style_text_color(touch_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(touch_label, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(touch_label, &homey_panel_font_22, 0);
    lv_obj_center(touch_label);
    lv_obj_add_flag(s_touch_button, LV_OBJ_FLAG_HIDDEN);

    s_qr = lv_qrcode_create(screen);
    if (s_qr != NULL) {
        lv_qrcode_set_size(s_qr, QR_SIZE);
        lv_obj_align(s_qr, LV_ALIGN_BOTTOM_MID, 0, -QR_BOTTOM_MARGIN);
        lv_obj_add_flag(s_qr, LV_OBJ_FLAG_HIDDEN);
    }

    panel_ui_model_init(&s_panel_model, (uint64_t)(esp_timer_get_time() / 1000LL));
    panel_ui_config_t panel_config = {
        .model = &s_panel_model,
        .callbacks = {
            .context = NULL,
            .request_brightness = panel_brightness_request,
            .request_wifi_reconfigure = panel_wifi_request,
            .request_choose_homey = panel_choose_request,
            .request_homey_wipe = panel_wipe_request,
            .request_change_athom_account = panel_account_request,
            .settings_changed = panel_settings_request,
        },
    };
    if (!panel_ui_create(&s_panel_ui, &panel_config)) {
        bsp_display_unlock();
        return ESP_FAIL;
    }

    bsp_display_unlock();
    return ESP_OK;
}

static secure_bootstrap_wifi_security_t map_auth(wifi_auth_mode_t mode)
{
    if (mode == WIFI_AUTH_OPEN) return SECURE_BOOTSTRAP_WIFI_SECURITY_OPEN;
    if (mode == WIFI_AUTH_WEP) return SECURE_BOOTSTRAP_WIFI_SECURITY_WEP;
    if (mode == WIFI_AUTH_WPA_PSK) return SECURE_BOOTSTRAP_WIFI_SECURITY_WPA;
    if (mode == WIFI_AUTH_WPA2_PSK || mode == WIFI_AUTH_WPA_WPA2_PSK) return SECURE_BOOTSTRAP_WIFI_SECURITY_WPA2;
    if (mode >= WIFI_AUTH_WPA3_PSK) return SECURE_BOOTSTRAP_WIFI_SECURITY_WPA3;
    return SECURE_BOOTSTRAP_WIFI_SECURITY_UNKNOWN;
}

static esp_err_t start_network_scan(void)
{
    if (s_scan_in_progress) {
        return ESP_OK;
    }

    wifi_scan_config_t config = {
        .show_hidden = false,
    };
    s_scan_in_progress = true;
    ESP_LOGI(TAG, "WIFI_TRACE scan_start_begin");
    esp_err_t err = esp_wifi_scan_start(&config, false);
    ESP_LOGI(TAG, "WIFI_TRACE scan_start_end result=%s", esp_err_to_name(err));
    if (err != ESP_OK) {
        s_scan_in_progress = false;
        ESP_LOGW(TAG, "Wi-Fi scan start failed: %s", esp_err_to_name(err));
    }
    return err;
}

static size_t consume_network_scan_results(void)
{
    s_scan_in_progress = false;

    uint16_t total = 0U;
    esp_err_t err = esp_wifi_scan_get_ap_num(&total);
    if (err != ESP_OK || total == 0U) {
        memset(s_networks, 0, sizeof(s_networks));
        s_network_count = 0U;
        ESP_LOGI(TAG, "WIFI_TRACE scan_results_consumed count=0 result=%s", esp_err_to_name(err));
        return 0U;
    }
    if (total > 64U) {
        total = 64U;
    }

    wifi_ap_record_t *records = calloc(total, sizeof(*records));
    if (records == NULL) {
        ESP_LOGE(TAG, "Wi-Fi scan result allocation failed");
        return s_network_count;
    }

    uint16_t fetched = total;
    err = esp_wifi_scan_get_ap_records(&fetched, records);
    if (err != ESP_OK) {
        secure_zero(records, total * sizeof(*records));
        free(records);
        ESP_LOGW(TAG, "Wi-Fi scan result read failed: %s", esp_err_to_name(err));
        return s_network_count;
    }

    secure_bootstrap_network_t updated[SECURE_BOOTSTRAP_MAX_NETWORKS] = {0};
    size_t updated_count = 0U;
    for (uint16_t index = 0U; index < fetched; ++index) {
        updated_count = secure_bootstrap_network_insert(
            updated,
            updated_count,
            SECURE_BOOTSTRAP_MAX_NETWORKS,
            (const char *)records[index].ssid,
            records[index].rssi,
            map_auth(records[index].authmode));
    }

    memcpy(s_networks, updated, sizeof(s_networks));
    s_network_count = updated_count;
    secure_zero(updated, sizeof(updated));
    secure_zero(records, total * sizeof(*records));
    free(records);
    ESP_LOGI(TAG, "WIFI_TRACE scan_results_consumed count=%u", (unsigned)s_network_count);
    return s_network_count;
}

static const secure_bootstrap_network_t *find_scanned_network(const char *ssid)
{
    if (ssid == NULL || ssid[0] == '\0') return NULL;
    for (size_t index = 0U; index < s_network_count; ++index) {
        if (strcmp(s_networks[index].ssid, ssid) == 0) return &s_networks[index];
    }
    return NULL;
}

static bool request_uses_setup_ap(httpd_req_t *req)
{
    const bool request_valid = req != NULL;
    const bool ap_netif_valid = s_ap_netif != NULL;
    if (!request_valid || !ap_netif_valid) {
        ESP_LOGI(TAG, "PORTAL_ROUTE request_valid=%s ap_netif_valid=%s",
                 request_valid ? "true" : "false",
                 ap_netif_valid ? "true" : "false");
        return false;
    }

    int sockfd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "PORTAL_ROUTE sockfd_valid=%s", sockfd >= 0 ? "true" : "false");
    if (sockfd < 0) return false;

    struct sockaddr_storage local = {0};
    socklen_t local_len = sizeof(local);
    int socket_result = getsockname(sockfd, (struct sockaddr *)&local, &local_len);
    ESP_LOGI(TAG, "PORTAL_ROUTE getsockname_result=%d", socket_result);
    if (socket_result != 0) return false;

    ESP_LOGI(TAG, "PORTAL_ROUTE local_family=%d", (int)local.ss_family);

    esp_netif_ip_info_t ap_ip = {0};
    esp_err_t ap_ip_result = esp_netif_get_ip_info(s_ap_netif, &ap_ip);
    ESP_LOGI(TAG, "PORTAL_ROUTE ap_ip_result=%s", esp_err_to_name(ap_ip_result));
    if (ap_ip_result != ESP_OK) return false;

    uint32_t local_ipv4 = 0U;
    bool comparable_ipv4 = false;

    if (local.ss_family == AF_INET) {
        const struct sockaddr_in *local_v4 = (const struct sockaddr_in *)&local;
        local_ipv4 = local_v4->sin_addr.s_addr;
        comparable_ipv4 = true;
    } else if (local.ss_family == AF_INET6) {
        const struct sockaddr_in6 *local_v6 = (const struct sockaddr_in6 *)&local;
        const uint8_t *address = local_v6->sin6_addr.s6_addr;
        const bool ipv4_mapped =
            address[0] == 0U && address[1] == 0U &&
            address[2] == 0U && address[3] == 0U &&
            address[4] == 0U && address[5] == 0U &&
            address[6] == 0U && address[7] == 0U &&
            address[8] == 0U && address[9] == 0U &&
            address[10] == 0xffU && address[11] == 0xffU;
        ESP_LOGI(TAG, "PORTAL_ROUTE ipv4_mapped=%s", ipv4_mapped ? "true" : "false");
        if (ipv4_mapped) {
            memcpy(&local_ipv4, &address[12], sizeof(local_ipv4));
            comparable_ipv4 = true;
        }
    }

    if (!comparable_ipv4) {
        ESP_LOGI(TAG, "PORTAL_ROUTE setup_ap_match=false");
        return false;
    }

    const bool setup_ap_match = local_ipv4 == ap_ip.ip.addr;
    ESP_LOGI(TAG, "PORTAL_ROUTE setup_ap_match=%s", setup_ap_match ? "true" : "false");
    return setup_ap_match;
}

static const char *security_data_value(secure_bootstrap_wifi_security_t security)
{
    return security == SECURE_BOOTSTRAP_WIFI_SECURITY_OPEN ? "open" : "secured";
}

static esp_err_t send_network_options(httpd_req_t *req)
{
    esp_err_t result = httpd_resp_sendstr_chunk(
        req, "<option value=\"\" selected disabled>Välj ett nätverk</option>");
    if (result != ESP_OK) return result;

    for (size_t index = 0U; index < s_network_count; ++index) {
        char escaped[192];
        if (!secure_bootstrap_html_escape(s_networks[index].ssid, escaped, sizeof(escaped))) continue;
        char option[352];
        int written = snprintf(
            option,
            sizeof(option),
            "<option value=\"%s\" data-security=\"%s\">%s - %s signal - %s</option>",
            escaped,
            security_data_value(s_networks[index].security),
            escaped,
            secure_bootstrap_signal_label(s_networks[index].rssi),
            secure_bootstrap_security_label(s_networks[index].security));
        if (written > 0 && (size_t)written < sizeof(option)) {
            result = httpd_resp_sendstr_chunk(req, option);
            if (result != ESP_OK) return result;
        }
    }

    return httpd_resp_sendstr_chunk(
        req,
        "<option value=\"__manual__\" data-security=\"manual\">Annat nätverk...</option>");
}

static esp_err_t networks_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    ESP_RETURN_ON_ERROR(send_network_options(req), TAG, "network options");
    return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t root_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "PORTAL_ROOT begin");
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    char escaped_code[64] = {0};
    const bool setup_ap_request = request_uses_setup_ap(req);
    ESP_LOGI(TAG, "PORTAL_ROOT setup_ap_request=%s",
             setup_ap_request ? "true" : "false");
    if (setup_ap_request) {
        if (!secure_bootstrap_html_escape(s_code.code, escaped_code, sizeof(escaped_code))) {
            escaped_code[0] = '\0';
        }
    }
    esp_err_t result = httpd_resp_sendstr_chunk(req,
        "<!doctype html><html lang=\"sv\"><head><meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Wi-Fi</title><style>");
    if (result != ESP_OK) return result;
    result = httpd_resp_sendstr_chunk(req, portal_shared_css());
    if (result != ESP_OK) return result;
    result = httpd_resp_sendstr_chunk(req,
        "</style></head><body><main>"
        "<h1>Homey Panel Wi-Fi</h1><form id=wifi-form method=post action=/wifi novalidate>"
        "<label>Panelkod<input id=code name=code type=password required autocomplete=off value=\"");
    ESP_LOGI(TAG, "PORTAL_ROOT chunk=prefix result=%s", esp_err_to_name(result));
    if (result != ESP_OK) return result;

    const bool code_present = escaped_code[0] != '\0';
    ESP_LOGI(TAG, "PORTAL_ROOT code_present=%s", code_present ? "true" : "false");
    if (code_present) {
        result = httpd_resp_sendstr_chunk(req, escaped_code);
        ESP_LOGI(TAG, "PORTAL_ROOT chunk=code result=%s", esp_err_to_name(result));
        secure_zero(escaped_code, sizeof(escaped_code));
        if (result != ESP_OK) return result;
    } else {
        ESP_LOGI(TAG, "PORTAL_ROOT chunk=code skipped=true");
    }

    result = httpd_resp_sendstr_chunk(req,
        "\"><span id=code-help class=help></span></label>"
        "<label>Nätverk<select name=ssid id=ssid required>");
    ESP_LOGI(TAG, "PORTAL_ROOT chunk=after_code result=%s", esp_err_to_name(result));
    if (result != ESP_OK) return result;

    result = send_network_options(req);
    ESP_LOGI(TAG, "PORTAL_ROOT chunk=network_options result=%s", esp_err_to_name(result));
    if (result != ESP_OK) return result;

    result = httpd_resp_sendstr_chunk(req,
        "</select><span id=network-help class=help></span></label>"
        "<div id=manual hidden><label>SSID<input id=manual-ssid name=manual_ssid maxlength=32 autocomplete=off>"
        "<span id=manual-help class=help></span></label>"
        "<label class=check><input id=manual-open name=manual_open type=checkbox value=1>Detta är ett öppet nätverk</label></div>"
        "<label>Lösenord<input id=password name=password type=password maxlength=63 autocomplete=new-password>"
        "<span id=password-help class=help></span></label>"
        "<button id=connect type=submit disabled>Anslut panelen till valt nätverk</button></form>"
        "<script>const f=document.getElementById('wifi-form'),c=document.getElementById('code'),s=document.getElementById('ssid'),"
        "m=document.getElementById('manual'),ms=document.getElementById('manual-ssid'),mo=document.getElementById('manual-open'),"
        "p=document.getElementById('password'),b=document.getElementById('connect'),ch=document.getElementById('code-help'),"
        "nh=document.getElementById('network-help'),mh=document.getElementById('manual-help'),ph=document.getElementById('password-help');"
        "const bytes=v=>new TextEncoder().encode(v).length;function validate(){const o=s.options[s.selectedIndex],manual=s.value==='__manual__',"
        "open=manual?mo.checked:(o&&o.dataset.security==='open'),codeOk=c.value.trim().length>0,networkOk=s.value!==''&&(!manual||bytes(ms.value)>0&&bytes(ms.value)<=32);"
        "m.hidden=!manual;mo.disabled=!manual;p.readOnly=open;if(open&&p.value){p.value=''}const n=bytes(p.value),passOk=open?n===0:n>=8&&n<=63;"
        "ch.textContent=codeOk?'':'Ange panelkoden';nh.textContent=s.value?'':'Välj ett nätverk';"
        "mh.textContent=manual&&!networkOk?'Ange nätverksnamnet':'';ph.textContent=open?'Öppet nätverk - inget lösenord krävs':"
        "(n===0?'Ange ett lösenord med 8-63 tecken':(n<8?'Lösenordet är för kort':(n>63?'Lösenordet får innehålla högst 63 tecken':'')));"
        "b.disabled=!(codeOk&&networkOk&&passOk)}[c,s,ms,mo,p].forEach(e=>{e.addEventListener('input',validate);e.addEventListener('change',validate)});validate();</script>"
        "</main></body></html>");
    ESP_LOGI(TAG, "PORTAL_ROOT chunk=body result=%s", esp_err_to_name(result));
    if (result != ESP_OK) return result;

    result = httpd_resp_sendstr_chunk(req, NULL);
    ESP_LOGI(TAG, "PORTAL_ROOT chunk=finish result=%s", esp_err_to_name(result));
    if (result == ESP_OK) ESP_LOGI(TAG, "PORTAL_ROOT complete");
    return result;
}

static bool is_hex(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static unsigned hexval(char c) { if (c <= '9') return (unsigned)(c - '0'); if (c <= 'F') return (unsigned)(c - 'A' + 10); return (unsigned)(c - 'a' + 10); }
static bool decode(char *dst, size_t cap, const char *src) { size_t d=0; for(size_t i=0; src[i]; ++i){ if(d+1>=cap)return false; if(src[i]=='+')dst[d++]=' '; else if(src[i]=='%'&&is_hex(src[i+1])&&is_hex(src[i+2])){dst[d++]=(char)((hexval(src[i+1])<<4)|hexval(src[i+2]));i+=2;} else dst[d++]=src[i]; } dst[d]=0; return true; }
static bool form_value(const char *body,const char *key,char *out,size_t cap){ size_t k=strlen(key); const char *p=body; while(p&&*p){ if((p==body||p[-1]=='&')&&strncmp(p,key,k)==0&&p[k]=='='){const char *v=p+k+1,*e=strchr(v,'&');size_t n=e?(size_t)(e-v):strlen(v);char enc[192];if(n>=sizeof(enc))return false;memcpy(enc,v,n);enc[n]=0;bool ok=decode(out,cap,enc);secure_zero(enc,sizeof(enc));return ok;} p=strchr(p,'&');if(p)++p;} return false; }

static esp_err_t send_html_error(
    httpd_req_t *req,
    httpd_err_code_t status,
    const char *message)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send_err(req, status, message);
}

static esp_err_t send_candidate_status_page(
    httpd_req_t *req,
    bool saved_config_present)
{
    static const char page_with_restore[] =
        "<!doctype html><html lang=\"sv\"><head><meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Nätverket testas</title>"
        "<style>body{font:18px system-ui;max-width:34rem;margin:2rem auto;padding:1rem;"
        "background:#f6f8fb;color:#16202a}main{background:#fff;border-radius:1rem;"
        "padding:1.4rem;box-shadow:0 .3rem 1.2rem rgba(0,0,0,.08)}h1{margin-top:0}"
        "p{line-height:1.5}</style></head><body><main><h1>Nätverket testas</h1>"
        "<p>Panelen försöker ansluta med de nya inställningarna.</p>"
        "<p>Du kan nu återgå till panelen. Om anslutningen misslyckas återställs "
        "den tidigare Wi-Fi-konfigurationen automatiskt.</p></main></body></html>";
    static const char page_without_restore[] =
        "<!doctype html><html lang=\"sv\"><head><meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Nätverket testas</title>"
        "<style>body{font:18px system-ui;max-width:34rem;margin:2rem auto;padding:1rem;"
        "background:#f6f8fb;color:#16202a}main{background:#fff;border-radius:1rem;"
        "padding:1.4rem;box-shadow:0 .3rem 1.2rem rgba(0,0,0,.08)}h1{margin-top:0}"
        "p{line-height:1.5}</style></head><body><main><h1>Nätverket testas</h1>"
        "<p>Panelen försöker ansluta med de nya inställningarna.</p>"
        "<p>Du kan nu återgå till panelen. Om anslutningen misslyckas kan du "
        "försöka igen från panelens installationsläge.</p></main></body></html>";

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_sendstr(
        req,
        saved_config_present ? page_with_restore : page_without_restore);
}

static esp_err_t wifi_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len >= MAX_FORM_BODY) {
        return send_html_error(req, HTTPD_400_BAD_REQUEST, "Ogiltigt formulär");
    }
    char body[MAX_FORM_BODY] = {0};
    int got = httpd_req_recv(req, body, req->content_len);
    if (got <= 0) return ESP_FAIL;
    body[got] = 0;

    char code[32] = {0};
    char selected[33] = {0};
    char manual[33] = {0};
    char password[65] = {0};
    char manual_open_value[4] = {0};
    bool parsed = form_value(body, "code", code, sizeof(code)) &&
                  form_value(body, "ssid", selected, sizeof(selected)) &&
                  form_value(body, "password", password, sizeof(password));
    (void)form_value(body, "manual_ssid", manual, sizeof(manual));
    bool manual_open = form_value(body, "manual_open", manual_open_value, sizeof(manual_open_value)) &&
                       strcmp(manual_open_value, "1") == 0;
    secure_zero(body, sizeof(body));
    secure_zero(manual_open_value, sizeof(manual_open_value));
    if (!parsed || code[0] == '\0') goto invalid;

    bool is_manual = strcmp(selected, "__manual__") == 0;
    const secure_bootstrap_network_t *scanned = is_manual ? NULL : find_scanned_network(selected);
    const char *ssid = is_manual ? manual : selected;
    size_t ssid_len = strlen(ssid);
    if (ssid_len == 0U || ssid_len > 32U || (!is_manual && scanned == NULL)) goto invalid;

    bool open_network = is_manual
        ? manual_open
        : scanned->security == SECURE_BOOTSTRAP_WIFI_SECURITY_OPEN;
    size_t password_len = strlen(password);
    if ((open_network && password_len != 0U) ||
        (!open_network && (password_len < 8U || password_len > 63U))) goto invalid;

    secure_bootstrap_status_t verified = secure_bootstrap_code_verify_and_consume(&s_code, code, now_s());
    secure_zero(code, sizeof(code));
    if (verified != SECURE_BOOTSTRAP_OK) goto forbidden;

    memset(&s_candidate_config, 0, sizeof(s_candidate_config));
    int ssid_written = snprintf(
        (char *)s_candidate_config.sta.ssid,
        sizeof(s_candidate_config.sta.ssid),
        "%s",
        ssid);
    int password_written = snprintf(
        (char *)s_candidate_config.sta.password,
        sizeof(s_candidate_config.sta.password),
        "%s",
        password);
    if (ssid_written < 0 || (size_t)ssid_written >= sizeof(s_candidate_config.sta.ssid) ||
        password_written < 0 || (size_t)password_written >= sizeof(s_candidate_config.sta.password)) goto invalid;
    s_candidate_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    secure_zero(selected, sizeof(selected));
    secure_zero(manual, sizeof(manual));
    secure_zero(password, sizeof(password));
    esp_err_t disconnect_err = esp_wifi_disconnect();
    if (disconnect_err != ESP_OK && disconnect_err != ESP_ERR_WIFI_NOT_CONNECT) {
        ESP_LOGW(TAG, "Candidate disconnect failed: %s", esp_err_to_name(disconnect_err));
        return send_html_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Kunde inte förbereda nätverkstestet");
    }
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "candidate RAM");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &s_candidate_config), TAG, "candidate config");
    s_candidate_valid = true;
    (void)apply_actions(secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED));
    return send_candidate_status_page(req, s_wifi.saved_config_present);

invalid:
    secure_zero(code, sizeof(code));
    secure_zero(selected, sizeof(selected));
    secure_zero(manual, sizeof(manual));
    secure_zero(password, sizeof(password));
    return send_html_error(req, HTTPD_400_BAD_REQUEST, "Ogiltiga formulärfält");
forbidden:
    secure_zero(selected, sizeof(selected));
    secure_zero(manual, sizeof(manual));
    secure_zero(password, sizeof(password));
    return send_html_error(req, HTTPD_403_FORBIDDEN, "Felaktig eller utgången panelkod");
}

static esp_err_t status_handler(httpd_req_t *req) { char json[192]; int n=snprintf(json,sizeof(json),"{\"state\":%u,\"ip_obtained\":%s,\"oauth_locked\":true}",(unsigned)s_wifi.state,s_ip_obtained?"true":"false"); httpd_resp_set_type(req,"application/json"); return httpd_resp_send(req,json,n); }
static esp_err_t server_start(void)
{
    httpd_config_t cfg=HTTPD_DEFAULT_CONFIG(); cfg.max_uri_handlers=20;
    ESP_LOGI(TAG, "WIFI_TRACE http_start_begin");
    esp_err_t http_start_err = httpd_start(&s_server, &cfg);
    ESP_LOGI(TAG, "WIFI_TRACE http_start_end result=%s", esp_err_to_name(http_start_err));
    ESP_RETURN_ON_ERROR(http_start_err, TAG, "server");
    const httpd_uri_t root={.uri="/",.method=HTTP_GET,.handler=root_handler}; const httpd_uri_t networks={.uri="/networks",.method=HTTP_GET,.handler=networks_handler}; const httpd_uri_t wifi={.uri="/wifi",.method=HTTP_POST,.handler=wifi_handler}; const httpd_uri_t status={.uri="/status",.method=HTTP_GET,.handler=status_handler};
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server,&root),TAG,"root"); ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server,&networks),TAG,"networks"); ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server,&wifi),TAG,"wifi"); return httpd_register_uri_handler(s_server,&status);
}

static esp_err_t ensure_homey_server(void)
{
    if (s_server == NULL) {
        ESP_RETURN_ON_ERROR(server_start(), TAG, "HTTP server");
        s_homey_handlers_registered = false;
    }
    if (!s_homey_handlers_registered) {
        phone_provisioning_set_display_callback(set_display_text);
        phone_provisioning_set_portal_css(portal_shared_css());
        ESP_RETURN_ON_ERROR(phone_provisioning_register_handlers(s_server), TAG, "Homey provisioning handlers");
        s_homey_handlers_registered = true;
    }
    return ESP_OK;
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "WIFI_TRACE scan_done_event");
        (void)consume_network_scan_results();
        return;
    }
    if (s_persist_active && s_persist_stage != WIFI_PERSIST_STAGE_VERIFY_CONNECT) {
        ESP_LOGI(TAG, "WIFI_PERSIST event_suppressed=%ld", (long)id);
        return;
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_ip_obtained = true;
        if (s_persist_active && s_persist_stage == WIFI_PERSIST_STAGE_VERIFY_CONNECT) {
            uint32_t actions = secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_GOT_IP);
            s_candidate_valid = false;
            secure_zero(&s_candidate_config, sizeof(s_candidate_config));
            (void)backup_clear();
            legacy_sentinel_clear();
            s_persist_active = false;
            s_persist_task_scheduled = false;
            s_persist_stage = WIFI_PERSIST_STAGE_IDLE;
            ESP_LOGI(TAG, "WIFI_PERSIST complete");
            ESP_LOGI(TAG, "WIFI_ONLINE persistent_saved_connect=true");
            (void)apply_actions(actions);
            return;
        }
        bool candidate = s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE;
        uint32_t actions = secure_bootstrap_wifi_transition(
            &s_wifi,
            candidate ? SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_GOT_IP : SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP);
        ESP_LOGI(TAG, "WIFI_GOT_IP commit_requested=%s", candidate ? "true" : "false");
        (void)apply_actions(actions);
        if (!candidate) ESP_LOGI(TAG, "WIFI_ONLINE persistent_saved_connect=false");
        return;
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_ip_obtained = false;
        if (s_persist_active && s_persist_stage == WIFI_PERSIST_STAGE_VERIFY_CONNECT) {
            (void)secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_FAILED);
            esp_err_t err = rollback_once();
            if (err == ESP_OK) (void)secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_OK);
            else {
                (void)secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED);
                if (esp_wifi_start() == ESP_OK) (void)open_provisioning();
            }
            s_persist_active = false;
            return;
        }
        if (s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED ||
            s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE ||
            s_wifi.state == SECURE_BOOTSTRAP_WIFI_ONLINE) {
            uint32_t actions = secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED);
            (void)apply_actions(actions);
            ESP_LOGW(TAG, "WIFI_DISCONNECTED state=%u display_unchanged=true", (unsigned)s_wifi.state);
        }
    }
}

static void rotation_task(void *arg)
{
    (void)arg;
    for (;;) {
        if (s_wifi.state == SECURE_BOOTSTRAP_WIFI_PROVISIONING) {
            if (secure_bootstrap_code_rotation_due(&s_code, now_s())) {
                secure_bootstrap_code_wipe(&s_code);
                if (generate_code() == ESP_OK) {
                    configure_access_point(s_code.code);
                    set_display_text(
                        "Säker Wi-Fi-installation",
                        "Anslut till HomeyPanel-Setup\nÖppna 192.168.4.1",
                        s_code.code);
                }
            }
            refresh_code_countdown();
        } else {
            s_last_code_countdown_s = -1;
        }
        if (s_panel_ui != NULL && bsp_display_lock(50)) {
            (void)panel_ui_tick(s_panel_ui, (uint64_t)(esp_timer_get_time() / 1000LL));
            bsp_display_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(ROTATION_POLL_INTERVAL_MS));
    }
}
static void wipe_task(void *arg)
{
    (void)arg; gpio_config_t io={.pin_bit_mask=1ULL<<BOOT_BUTTON_GPIO,.mode=GPIO_MODE_INPUT,.pull_up_en=GPIO_PULLUP_ENABLE}; ESP_ERROR_CHECK(gpio_config(&io)); secure_bootstrap_wipe_tracker_t t={0};
    for(;;){ if(secure_bootstrap_wipe_tracker_update(&t,gpio_get_level(BOOT_BUTTON_GPIO)==0,esp_timer_get_time()/1000LL)){ ESP_LOGW(TAG,"BOOTSTRAP_PHYSICAL_REPROVISION accepted=true"); (void)backup_clear(); legacy_sentinel_clear(); ESP_ERROR_CHECK(esp_wifi_restore()); esp_restart(); } vTaskDelay(pdMS_TO_TICKS(WIPE_POLL_INTERVAL_MS)); }
}

secure_bootstrap_status_t secure_bootstrap_start(void)
{
    secure_bootstrap_wifi_context_init(&s_wifi); if(display_init()!=ESP_OK)return SECURE_BOOTSTRAP_ERR_PLATFORM;
    set_display_text("Ansluter till Wi-Fi...", "Kontrollerar sparad konfiguration", NULL);
    ESP_ERROR_CHECK(esp_netif_init()); ESP_ERROR_CHECK(esp_event_loop_create_default()); s_ap_netif=esp_netif_create_default_wifi_ap(); s_sta_netif=esp_netif_create_default_wifi_sta(); if(!s_ap_netif||!s_sta_netif)return SECURE_BOOTSTRAP_ERR_PLATFORM;
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    s_wifi_nvs_enabled = init.nvs_enable != 0;
    ESP_LOGI(TAG, "WIFI_BOOT nvs_enabled=%s", s_wifi_nvs_enabled ? "true" : "false");
    legacy_sentinel_clear();
    ESP_ERROR_CHECK(esp_wifi_init(&init)); ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event,NULL)); ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event,NULL));
    esp_err_t storage_err = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    ESP_LOGI(TAG, "WIFI_BOOT storage_result=%s", esp_err_to_name(storage_err));
    ESP_ERROR_CHECK(storage_err);
    memset(&s_saved_config,0,sizeof(s_saved_config));
    esp_err_t get_config_err = esp_wifi_get_config(WIFI_IF_STA,&s_saved_config);
    ESP_LOGI(TAG, "WIFI_BOOT get_config_result=%s", esp_err_to_name(get_config_err));
    ESP_ERROR_CHECK(get_config_err);
    bool ssid_first_byte_zero = s_saved_config.sta.ssid[0] == 0U;
    bool ssid_any_nonzero = ssid_has_any_nonzero(&s_saved_config);
    ESP_LOGI(TAG, "WIFI_BOOT ssid_first_byte_zero=%s", ssid_first_byte_zero ? "true" : "false");
    ESP_LOGI(TAG, "WIFI_BOOT ssid_any_nonzero=%s", ssid_any_nonzero ? "true" : "false");
    s_saved_valid=has_saved_ssid(&s_saved_config);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    uint32_t actions=secure_bootstrap_wifi_transition(&s_wifi,s_saved_valid?SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED:SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED);
    ESP_LOGI(TAG, "WIFI_BOOT saved_config=%s", s_saved_valid ? "true" : "false");
    (void)apply_actions(actions);
    if (xTaskCreate(
            wipe_task,
            "physical_wipe",
            3072,
            NULL,
            5,
            NULL) != pdPASS) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }
    if (xTaskCreate(
            rotation_task,
            "bootstrap_rotation",
            4096,
            NULL,
            5,
            NULL) != pdPASS) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }
    ESP_LOGI(TAG,"BOOTSTRAP_READY saved_config=%s softap=%s oauth_locked=true",s_saved_valid?"true":"false",s_saved_valid?"false":"true"); return SECURE_BOOTSTRAP_OK;
}
#endif
