#include "secure_bootstrap.h"

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
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

#include <stdio.h>
#include <string.h>

#define AP_SSID "HomeyPanel-Setup"
#define WIFI_CONNECTED_BIT BIT0
#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define WIPE_POLL_INTERVAL_MS 100
#define ROTATION_POLL_INTERVAL_MS 250
#define MAX_FORM_BODY 512
#define QR_PAYLOAD_CAPACITY 96
#define STATUS_JSON_CAPACITY 224
#define DISPLAY_WIDTH 480
#define DISPLAY_MARGIN 18
#define TITLE_Y 12
#define DETAIL_Y 58
#define CODE_CAPTION_Y 134
#define CODE_Y 168
#define CODE_CAPTION_TO_CODE_GAP 12
#define TOUCH_Y 214
#define QR_SIZE 180
#define QR_BOTTOM_MARGIN 8
#define SUCCESS_BLOCK_Y 132

static const char *TAG = "secure_bootstrap";
static secure_bootstrap_code_t s_code;
static SemaphoreHandle_t s_code_mutex;
static EventGroupHandle_t s_wifi_events;
static lv_obj_t *s_title;
static lv_obj_t *s_detail;
static lv_obj_t *s_code_caption;
static lv_obj_t *s_code_label;
static lv_obj_t *s_qr;
static lv_obj_t *s_touch_button;
static httpd_handle_t s_server;
static bool s_wifi_configured;
static bool s_ip_obtained;
static bool s_bootstrap_closed;

static int64_t now_s(void)
{
    return esp_timer_get_time() / 1000000LL;
}

static void secure_zero(void *buffer, size_t size)
{
    volatile unsigned char *p = (volatile unsigned char *)buffer;
    while (size > 0U) {
        *p++ = 0U;
        --size;
    }
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
    lv_label_set_text(s_code_caption, bootstrap_visible ? "Code:" : "");
    lv_label_set_text(s_code_label, display_code != NULL ? display_code : "");
    if (s_touch_button != NULL) {
        if (bootstrap_visible) {
            lv_obj_remove_flag(s_touch_button, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_touch_button, LV_OBJ_FLAG_HIDDEN);
        }
    }

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

static void touch_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_PRESSED) {
        ESP_LOGI(TAG, "BOOTSTRAP_TOUCH_EVENT detected=true");
    }
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
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x071018), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(screen, LV_OPA_COVER, 0);

    s_title = lv_label_create(screen);
    lv_obj_set_width(s_title, DISPLAY_WIDTH - (2 * DISPLAY_MARGIN));
    lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, TITLE_Y);
    lv_label_set_long_mode(s_title, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(s_title, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_28, 0);

    s_detail = lv_label_create(screen);
    lv_obj_set_width(s_detail, DISPLAY_WIDTH - (2 * DISPLAY_MARGIN));
    lv_obj_align(s_detail, LV_ALIGN_TOP_MID, 0, DETAIL_Y);
    lv_label_set_long_mode(s_detail, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_detail, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_detail, lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_text_opa(s_detail, LV_OPA_COVER, 0);
    lv_obj_set_style_text_line_space(s_detail, 5, 0);
    lv_obj_set_style_text_font(s_detail, &lv_font_montserrat_22, 0);

    s_code_caption = lv_label_create(screen);
    lv_obj_set_width(s_code_caption, DISPLAY_WIDTH - (2 * DISPLAY_MARGIN));
    lv_obj_align(s_code_caption, LV_ALIGN_TOP_MID, 0, CODE_CAPTION_Y);
    lv_obj_set_style_text_align(s_code_caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_code_caption, lv_color_hex(0xF4F7FA), 0);
    lv_obj_set_style_text_opa(s_code_caption, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_code_caption, &lv_font_montserrat_22, 0);

    s_code_label = lv_label_create(screen);
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
    lv_obj_add_event_cb(s_touch_button, touch_event, LV_EVENT_PRESSED, NULL);
    lv_obj_t *touch_label = lv_label_create(s_touch_button);
    lv_label_set_text(touch_label, "Touch test");
    lv_obj_set_style_text_color(touch_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(touch_label, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(touch_label, &lv_font_montserrat_22, 0);
    lv_obj_center(touch_label);

    s_qr = lv_qrcode_create(screen);
    if (s_qr != NULL) {
        lv_qrcode_set_size(s_qr, QR_SIZE);
        lv_obj_align(s_qr, LV_ALIGN_BOTTOM_MID, 0, -QR_BOTTOM_MARGIN);
    }

    bsp_display_unlock();
    return ESP_OK;
}

static esp_err_t configure_access_point(const char *password)
{
    if (password == NULL || strlen(password) != SECURE_BOOTSTRAP_CODE_LEN) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t access_point = {0};
    int ssid_written = snprintf(
        (char *)access_point.ap.ssid,
        sizeof(access_point.ap.ssid),
        "%s",
        AP_SSID);
    int password_written = snprintf(
        (char *)access_point.ap.password,
        sizeof(access_point.ap.password),
        "%s",
        password);

    if (ssid_written < 0 || (size_t)ssid_written >= sizeof(access_point.ap.ssid) ||
        password_written < 0 ||
        (size_t)password_written >= sizeof(access_point.ap.password)) {
        secure_zero(&access_point, sizeof(access_point));
        return ESP_ERR_INVALID_SIZE;
    }

    access_point.ap.ssid_len = strlen(AP_SSID);
    access_point.ap.channel = 1;
    access_point.ap.max_connection = 2;
    access_point.ap.authmode = WIFI_AUTH_WPA2_PSK;

    esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &access_point);
    secure_zero(&access_point, sizeof(access_point));
    return err;
}

static esp_err_t generate_code(secure_bootstrap_code_t *destination)
{
    uint8_t random_bytes[SECURE_BOOTSTRAP_CODE_LEN];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    secure_bootstrap_status_t result = secure_bootstrap_code_generate(
        destination,
        now_s(),
        random_bytes,
        sizeof(random_bytes));
    secure_zero(random_bytes, sizeof(random_bytes));
    return result == SECURE_BOOTSTRAP_OK ? ESP_OK : ESP_FAIL;
}

static void bootstrap_close_task(void *arg)
{
    (void)arg;

    bool http_closed = s_server == NULL;
    if (s_server != NULL) {
        httpd_handle_t server = s_server;
        esp_err_t stop_err = httpd_stop(server);
        if (stop_err == ESP_OK) {
            s_server = NULL;
            http_closed = true;
        } else {
            ESP_LOGE(TAG, "Bootstrap HTTP stop failed: %s", esp_err_to_name(stop_err));
        }
    }

    esp_err_t mode_err = esp_wifi_set_mode(WIFI_MODE_STA);
    bool softap_closed = mode_err == ESP_OK;
    if (!softap_closed) {
        ESP_LOGE(TAG, "SoftAP close failed: %s", esp_err_to_name(mode_err));
    }

    bool code_wiped = false;
    if (xSemaphoreTake(s_code_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        secure_bootstrap_code_wipe(&s_code);
        code_wiped = s_code.code[0] == '\0';
        xSemaphoreGive(s_code_mutex);
    } else {
        ESP_LOGE(TAG, "Provisioning code wipe mutex timeout");
    }

    bool closed = http_closed && softap_closed && code_wiped && s_ip_obtained;
    s_bootstrap_closed = closed;
    if (closed) {
        set_display_text(
            "Wi-Fi connected",
            "Setup complete\nBootstrap closed\nAthom OAuth remains locked",
            NULL);
        ESP_LOGI(
            TAG,
            "BOOTSTRAP_CLOSED softap=false http=false code_wiped=true physical_reopen_required=true ip_obtained=true");
    } else {
        set_display_text(
            "Setup incomplete",
            "Bootstrap could not close safely.\nHold BOOT for five seconds to retry.",
            NULL);
        ESP_LOGE(
            TAG,
            "BOOTSTRAP_CLOSE_FAILED http_closed=%s softap_closed=%s code_wiped=%s ip_obtained=%s",
            http_closed ? "true" : "false",
            softap_closed ? "true" : "false",
            code_wiped ? "true" : "false",
            s_ip_obtained ? "true" : "false");
    }
    vTaskDelete(NULL);
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_ip_obtained = true;
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
        ESP_LOGI(
            TAG,
            "BOOTSTRAP_STATUS wifi_configured=true ip_obtained=true oauth_locked=true");
        if (!s_bootstrap_closed) {
            BaseType_t created = xTaskCreate(
                bootstrap_close_task,
                "bootstrap_close",
                4096,
                NULL,
                6,
                NULL);
            if (created != pdPASS) {
                ESP_LOGE(TAG, "Bootstrap close task creation failed");
            }
        }
    } else if (
        base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED && s_wifi_configured) {
        s_ip_obtained = false;
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Wi-Fi reconnect scheduling failed: %s", esp_err_to_name(err));
        }
    }
}

static bool is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

static unsigned char hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return (unsigned char)(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return (unsigned char)(c - 'a' + 10);
    }
    return (unsigned char)(c - 'A' + 10);
}

static bool url_decode(char *dst, size_t dst_size, const char *src)
{
    if (dst == NULL || dst_size == 0U || src == NULL) {
        return false;
    }

    size_t di = 0U;
    for (size_t i = 0U; src[i] != '\0'; ++i) {
        if (di + 1U >= dst_size) {
            dst[0] = '\0';
            return false;
        }

        if (src[i] == '+') {
            dst[di++] = ' ';
        } else if (
            src[i] == '%' && src[i + 1U] != '\0' && src[i + 2U] != '\0' &&
            is_hex_digit(src[i + 1U]) && is_hex_digit(src[i + 2U])) {
            dst[di++] = (char)((hex_value(src[i + 1U]) << 4U) |
                               hex_value(src[i + 2U]));
            i += 2U;
        } else {
            dst[di++] = src[i];
        }
    }

    dst[di] = '\0';
    return true;
}

static bool form_value(
    const char *body,
    const char *key,
    char *out,
    size_t out_size)
{
    if (body == NULL || key == NULL || out == NULL || out_size == 0U) {
        return false;
    }

    size_t key_len = strlen(key);
    const char *p = body;

    while (*p != '\0') {
        if ((p == body || p[-1] == '&') && strncmp(p, key, key_len) == 0 &&
            p[key_len] == '=') {
            const char *value = p + key_len + 1U;
            const char *end = strchr(value, '&');
            size_t value_len = end != NULL ? (size_t)(end - value) : strlen(value);
            char encoded[160];

            if (value_len >= sizeof(encoded)) {
                return false;
            }

            memcpy(encoded, value, value_len);
            encoded[value_len] = '\0';
            bool decoded = url_decode(out, out_size, encoded);
            secure_zero(encoded, sizeof(encoded));
            return decoded;
        }

        p = strchr(p, '&');
        if (p == NULL) {
            break;
        }
        ++p;
    }

    return false;
}

static esp_err_t root_handler(httpd_req_t *req)
{
    static const char html[] =
        "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>Homey Panel Wi-Fi Setup</title>"
        "<style>body{font:18px system-ui;max-width:34rem;margin:2rem auto;padding:1rem}"
        "input,button{font:inherit;width:100%;padding:.7rem;margin:.4rem 0}</style>"
        "<h1>Secure Wi-Fi bootstrap</h1>"
        "<p>Enter the code shown on the panel and your Wi-Fi credentials. "
        "Athom OAuth is locked in this phase.</p>"
        "<form method=post action=/wifi>"
        "<label>Panel code<input name=code type=password required></label>"
        "<label>Wi-Fi SSID<input name=ssid maxlength=32 required></label>"
        "<label>Wi-Fi password<input name=password type=password maxlength=64 required></label>"
        "<button>Connect</button></form>";

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t wifi_handler(httpd_req_t *req)
{
    if (s_bootstrap_closed) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "bootstrap closed");
    }
    if (req->content_len <= 0 || req->content_len >= MAX_FORM_BODY) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid form");
    }

    char body[MAX_FORM_BODY];
    int received = httpd_req_recv(req, body, req->content_len);
    if (received <= 0) {
        secure_zero(body, sizeof(body));
        return ESP_FAIL;
    }
    body[received] = '\0';

    char code[32] = {0};
    char ssid[33] = {0};
    char password[65] = {0};

    bool parsed =
        form_value(body, "code", code, sizeof(code)) &&
        form_value(body, "ssid", ssid, sizeof(ssid)) &&
        form_value(body, "password", password, sizeof(password));

    secure_zero(body, sizeof(body));

    if (!parsed) {
        secure_zero(code, sizeof(code));
        secure_zero(ssid, sizeof(ssid));
        secure_zero(password, sizeof(password));
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing field");
    }

    secure_bootstrap_status_t verified = SECURE_BOOTSTRAP_ERR_PLATFORM;
    if (xSemaphoreTake(s_code_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        verified = secure_bootstrap_code_verify_and_consume(&s_code, code, now_s());
        xSemaphoreGive(s_code_mutex);
    }
    secure_zero(code, sizeof(code));

    if (verified != SECURE_BOOTSTRAP_OK) {
        secure_zero(ssid, sizeof(ssid));
        secure_zero(password, sizeof(password));
        return httpd_resp_send_err(
            req,
            HTTPD_403_FORBIDDEN,
            "invalid or expired panel code");
    }

    wifi_config_t station = {0};
    int ssid_written = snprintf(
        (char *)station.sta.ssid,
        sizeof(station.sta.ssid),
        "%s",
        ssid);
    int password_written = snprintf(
        (char *)station.sta.password,
        sizeof(station.sta.password),
        "%s",
        password);

    secure_zero(ssid, sizeof(ssid));
    secure_zero(password, sizeof(password));

    if (ssid_written < 0 || (size_t)ssid_written >= sizeof(station.sta.ssid) ||
        password_written < 0 ||
        (size_t)password_written >= sizeof(station.sta.password)) {
        secure_zero(&station, sizeof(station));
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid credentials");
    }

    station.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &station);
    secure_zero(&station, sizeof(station));

    if (err == ESP_OK) {
        err = esp_wifi_connect();
    }
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "connect failed");
    }

    s_wifi_configured = true;
    set_display_text(
        "Connecting to Wi-Fi",
        "Credentials accepted. Athom OAuth remains locked.",
        NULL);
    ESP_LOGI(
        TAG,
        "BOOTSTRAP_STATUS wifi_configured=true ip_obtained=false oauth_locked=true");
    return httpd_resp_sendstr(req, "Wi-Fi credentials accepted. Return to the panel.");
}

static esp_err_t status_handler(httpd_req_t *req)
{
    char json[STATUS_JSON_CAPACITY];
    int written = snprintf(
        json,
        sizeof(json),
        "{\"display_ready\":true,\"touch_ready\":true,\"softap_ready\":%s,"
        "\"wifi_configured\":%s,\"ip_obtained\":%s,\"bootstrap_closed\":%s,"
        "\"oauth_locked\":true}",
        s_bootstrap_closed ? "false" : "true",
        s_wifi_configured ? "true" : "false",
        s_ip_obtained ? "true" : "false",
        s_bootstrap_closed ? "true" : "false");

    if (written < 0 || (size_t)written >= sizeof(json)) {
        return httpd_resp_send_err(
            req,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "status overflow");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, written);
}

static esp_err_t server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 4;

    ESP_RETURN_ON_ERROR(httpd_start(&s_server, &config), TAG, "HTTP server start");

    const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL,
    };
    const httpd_uri_t wifi = {
        .uri = "/wifi",
        .method = HTTP_POST,
        .handler = wifi_handler,
        .user_ctx = NULL,
    };
    const httpd_uri_t status = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL,
    };

    esp_err_t err = httpd_register_uri_handler(s_server, &root);
    if (err == ESP_OK) {
        err = httpd_register_uri_handler(s_server, &wifi);
    }
    if (err == ESP_OK) {
        err = httpd_register_uri_handler(s_server, &status);
    }

    if (err != ESP_OK) {
        httpd_stop(s_server);
        s_server = NULL;
    }
    return err;
}

static void rotation_task(void *arg)
{
    (void)arg;

    for (;;) {
        if (!s_bootstrap_closed && !s_wifi_configured &&
            xSemaphoreTake(s_code_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            if (secure_bootstrap_code_rotation_due(&s_code, now_s())) {
                secure_bootstrap_code_t next = {0};
                esp_err_t generated = generate_code(&next);
                esp_err_t configured = generated == ESP_OK
                                           ? configure_access_point(next.code)
                                           : generated;
                if (configured == ESP_OK) {
                    esp_err_t deauth = esp_wifi_deauth_sta(0);
                    if (deauth != ESP_OK) {
                        ESP_LOGW(TAG, "SoftAP client deauth failed: %s", esp_err_to_name(deauth));
                    }
                    secure_bootstrap_code_wipe(&s_code);
                    s_code = next;
                    set_display_text(
                        "Secure Wi-Fi setup",
                        "Code rotated\nConnect to HomeyPanel-Setup\nOpen 192.168.4.1",
                        s_code.code);
                    ESP_LOGI(
                        TAG,
                        "BOOTSTRAP_ROTATED ttl_seconds=600 old_code_invalid=true");
                } else {
                    secure_bootstrap_code_wipe(&s_code);
                    set_display_text(
                        "Bootstrap unavailable",
                        "Code rotation failed. Hold BOOT for five seconds.",
                        NULL);
                    ESP_LOGE(TAG, "Provisioning code rotation failed closed");
                }
                secure_zero(&next, sizeof(next));
            }
            xSemaphoreGive(s_code_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(ROTATION_POLL_INTERVAL_MS));
    }
}

static void wipe_task(void *arg)
{
    (void)arg;

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BOOT_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));

    secure_bootstrap_wipe_tracker_t tracker = {0};
    secure_bootstrap_wipe_tracker_reset(&tracker);
    for (;;) {
        int64_t now_ms = esp_timer_get_time() / 1000LL;
        bool pressed = gpio_get_level(BOOT_BUTTON_GPIO) == 0;
        if (secure_bootstrap_wipe_tracker_update(&tracker, pressed, now_ms)) {
            ESP_LOGW(TAG, "BOOTSTRAP_PHYSICAL_REPROVISION hold_ms=5000 accepted=true");
            if (xSemaphoreTake(s_code_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                secure_bootstrap_code_wipe(&s_code);
                xSemaphoreGive(s_code_mutex);
            }
            esp_err_t restore_err = esp_wifi_restore();
            if (restore_err != ESP_OK) {
                ESP_LOGE(TAG, "Physical reprovision Wi-Fi restore failed: %s", esp_err_to_name(restore_err));
                secure_bootstrap_wipe_tracker_reset(&tracker);
                vTaskDelay(pdMS_TO_TICKS(WIPE_POLL_INTERVAL_MS));
                continue;
            }
            ESP_LOGW(TAG, "BOOTSTRAP_RESTARTING reason=physical_reprovision");
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_restart();
        }
        vTaskDelay(pdMS_TO_TICKS(WIPE_POLL_INTERVAL_MS));
    }
}

secure_bootstrap_status_t secure_bootstrap_start(void)
{
    s_code_mutex = xSemaphoreCreateMutex();
    if (s_code_mutex == NULL) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }
    if (generate_code(&s_code) != ESP_OK) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }

    if (display_init() != ESP_OK) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }

    set_display_text(
        "Secure Wi-Fi setup",
        "Connect to HomeyPanel-Setup\nOpen 192.168.4.1",
        s_code.code);

    s_wifi_events = xEventGroupCreate();
    if (s_wifi_events == NULL) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (esp_netif_create_default_wifi_ap() == NULL ||
        esp_netif_create_default_wifi_sta() == NULL) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi_event,
        NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        wifi_event,
        NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(configure_access_point(s_code.code));
    ESP_ERROR_CHECK(esp_wifi_start());

    if (server_start() != ESP_OK) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }

    BaseType_t wipe_created = xTaskCreate(
        wipe_task,
        "physical_wipe",
        3072,
        NULL,
        5,
        NULL);
    if (wipe_created != pdPASS) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }

    BaseType_t rotation_created = xTaskCreate(
        rotation_task,
        "bootstrap_rotation",
        4096,
        NULL,
        5,
        NULL);
    if (rotation_created != pdPASS) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }

    ESP_LOGI(
        TAG,
        "BOOTSTRAP_READY display=true touch=true softap=true oauth_locked=true runtime_rotation=true");
    ESP_LOGI(TAG, "Provisioning code is displayed locally and is never logged");
    return SECURE_BOOTSTRAP_OK;
}

#endif
