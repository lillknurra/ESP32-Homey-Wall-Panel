#include "panel_render_diag.h"
#include "secure_bootstrap.h"
#ifndef PANEL_UPDATE_I3_OBSERVER_ENABLED
#define PANEL_UPDATE_I3_OBSERVER_ENABLED 0
#endif
#if PANEL_UPDATE_I3_OBSERVER_ENABLED
#include "panel_update_i3_probe.h"
#endif
#include "phone_provisioning.h"
#include "panel_ui.h"
#include "panel_clock.h"
#include "panel_homey_dashboard_binding.h"
#include "panel_homey_favorites.h"
#include "athom_cloud_client.h"
#include "athom_oauth_runtime.h"
#include "athom_auth_store.h"
#include "runtime_diag_journal.h"
#include "panel_homey_alias_store.h"
#include "panel_ui_store.h"
#include "panel_startup_status.h"
#include "panel_startup_logo.h"
#include "homey_panel_font_22.h"
#include "homey_panel_font_27.h"
#if defined(PATCH107_STARTUP_TITLE_DIAG)
#include "panel_startup_title_diag.h"
#endif
#ifdef ESP_PLATFORM
#include "bsp/esp32_s3_touch_lcd_4b.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lwip/sockets.h"
#include "nvs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define AP_SSID "HomeyPanel-Setup"
#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define WIPE_POLL_INTERVAL_MS 100
#define ROTATION_POLL_INTERVAL_MS 250
#define HOMEY_DASHBOARD_POLL_INTERVAL_MS 1000
#define MAX_FORM_BODY 512
#define QR_PAYLOAD_CAPACITY 96
#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 480
#define DISPLAY_MARGIN 18
#define TITLE_Y 12
#define DETAIL_Y 58
#define CODE_CAPTION_Y 134
#define CODE_Y 168
#define TOUCH_Y 214
#define TOUCH_HEIGHT 52
#define STARTUP_LOGO_Y_OFFSET (-127)
#define STARTUP_STATUS_BOTTOM_MARGIN 24
#define STARTUP_WIFI_BUTTON_GAP 16
#define STARTUP_STATUS_TEXT_CAPACITY 192U
#define QR_SIZE 180
#define QR_BOTTOM_MARGIN 8
#define PATCH022_SCROLL_LIMIT 4U
#define PATCH022_SCROLL_THROW 20U


static const char *TAG = "secure_bootstrap";
ESP_EVENT_DEFINE_BASE(SECURE_BOOTSTRAP_WIFI_FENCE_EVENT);
#define WIFI_FENCE_EVENT_DRAINED 1
#define WIFI_DISCONNECT_FENCE_TIMEOUT_MS 5000U
#define WIFI_EVENT_DRAIN_TIMEOUT_MS 2000U
#define WIFI_DISCONNECT_POLL_MS 25U
static const char PORTAL_SHARED_CSS[] =
    "body{font:22px system-ui;max-width:34rem;margin:2rem auto;padding:1rem;background:#f6f8fb;color:#16202a}"
    "main,.panel{background:#fff;border-radius:1rem;padding:1.4rem;box-shadow:0 .3rem 1.2rem rgba(0,0,0,.08)}"
    "h1,h2{margin-top:0}p{line-height:1.5}label{display:block;margin:.7rem 0}"
    "select,input,button{box-sizing:border-box;font:inherit;width:100%;padding:.7rem;margin:.4rem 0;border-radius:.55rem}"
    "button{white-space:normal;line-height:1.25;min-height:3.2rem;border:0;background:#1f5f99;color:#fff;font-weight:650}"
    "button:disabled{opacity:.45;cursor:not-allowed}.help{display:block;min-height:1.3em;color:#9b1c1c;font-size:22px}"
    ".check{display:flex;gap:.6rem;align-items:center}.check input{width:auto}.notice{padding:1rem;border-radius:.75rem;background:#eef5fb;margin:1rem 0}"
    ".testmode{padding:1rem;border-radius:.75rem;background:#fff3cd;border:1px solid #e4bd53;margin:1rem 0}"
    ".pass{color:#146c43;font-weight:700}.fail{color:#b02a37;font-weight:700}.danger{background:#a61b29}.muted{color:#5b6670;font-size:22px}";

static const char *portal_shared_css(void) { return PORTAL_SHARED_CSS; }

static secure_bootstrap_code_t s_code;
static secure_bootstrap_wifi_context_t s_wifi;
static secure_bootstrap_wifi_session_t s_wifi_session;
static portMUX_TYPE s_wifi_session_mux = portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t s_ap_config_mutex;
static SemaphoreHandle_t s_wifi_fence_sem;
static uint32_t s_wifi_fence_wait_generation;
static uint32_t s_wifi_fence_done_generation;
static uint32_t s_candidate_connect_generation;
static uint32_t s_persist_session_generation;
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
static panel_homey_dashboard_state_t s_homey_dashboard_state;
static panel_ui_connection_state_t s_dashboard_connection_state =
    PANEL_UI_CONNECTION_UNKNOWN;
static bool s_dashboard_homey_data_ready;
static uint64_t s_homey_dashboard_last_poll_ms;
static bool s_panel_dashboard_visible;
static bool s_startup_status_active = true;
static portMUX_TYPE s_startup_status_mux = portMUX_INITIALIZER_UNLOCKED;
#if defined(PATCH107_STARTUP_TITLE_DIAG)
static panel_startup_title_diag_log_gate_t s_startup_title_diag_gate;
static panel_startup_title_diag_id_t s_startup_title_diag_pending_id =
    PANEL_STARTUP_TITLE_DIAG_UNKNOWN;
static panel_startup_title_diag_rect_t s_startup_title_diag_visible_rect;
static uint64_t s_startup_title_diag_transition_us;
static bool s_startup_title_diag_waiting_refresh;
static bool s_startup_title_diag_refresh_seen;
static bool s_startup_title_diag_flush_intersects;
#endif
static bool s_homey_data_long_wait_rendered;
static panel_startup_wait_t s_homey_data_wait;
static panel_startup_status_t s_startup_status;
static bool s_startup_layout_active;
static lv_obj_t *s_startup_logo;
static uint32_t s_light_toggle_completion_generation_seen;
static bool s_sntp_init_attempted;
static bool s_sntp_time_received;
static bool s_sntp_start_pending;
static bool s_panel_timezone_ready;
static portMUX_TYPE s_sntp_time_mux = portMUX_INITIALIZER_UNLOCKED;
static panel_clock_render_state_t s_panel_clock_render_state;
static panel_ui_settings_t s_panel_settings_boot;
static panel_ui_settings_t s_panel_settings_persisted;
static bool s_panel_settings_persisted_valid;
static QueueHandle_t s_panel_settings_queue;
static portMUX_TYPE s_panel_settings_mux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t s_panel_settings_requested_generation;
static void startup_status_active_set(bool active);
static uint32_t s_panel_settings_completed_generation;
static uint32_t s_panel_settings_published_generation;
static bool s_panel_settings_last_save_ok;
/* Protected by s_panel_settings_mux; false until rotation publishes once. */
static bool s_panel_settings_publisher_ready;

typedef struct {
    panel_ui_settings_t settings;
    uint32_t generation;
} panel_ui_settings_save_request_t;

static bool panel_ui_settings_equal(
    const panel_ui_settings_t *left, const panel_ui_settings_t *right)
{
    return left != NULL && right != NULL &&
        left->normal_brightness == right->normal_brightness &&
        left->dimmed_brightness == right->dimmed_brightness &&
        left->dim_after_seconds == right->dim_after_seconds &&
        left->off_after_seconds == right->off_after_seconds &&
        left->wake_on_touch == right->wake_on_touch &&
        left->background_mode == right->background_mode &&
        strncmp(left->timezone_id, right->timezone_id,
                PANEL_UI_TIMEZONE_ID_MAX) == 0;
}

static panel_ui_settings_t panel_ui_settings_confirmed_snapshot(void)
{
    panel_ui_settings_t confirmed;
    portENTER_CRITICAL(&s_panel_settings_mux);
    confirmed = s_panel_settings_persisted_valid
        ? s_panel_settings_persisted : s_panel_settings_boot;
    portEXIT_CRITICAL(&s_panel_settings_mux);
    return confirmed;
}

static void panel_ui_settings_restore_confirmed(void)
{
    if (s_panel_ui == NULL) return;
    const panel_ui_settings_t confirmed = panel_ui_settings_confirmed_snapshot();
    (void)panel_ui_restore_settings(s_panel_ui, &confirmed);
}

static void panel_ui_settings_persist_worker(void *arg)
{
    (void)arg;
    for (;;) {
        panel_ui_settings_save_request_t request;
        if (xQueueReceive(s_panel_settings_queue, &request, portMAX_DELAY) != pdPASS) {
            continue;
        }

        /* A single-slot overwrite queue plus a short quiet period coalesces
         * touch bursts before any flash write begins. */
        panel_ui_settings_save_request_t latest;
        for (unsigned debounce_window = 0U; debounce_window < 4U;
             ++debounce_window) {
            if (xQueueReceive(s_panel_settings_queue, &latest,
                              pdMS_TO_TICKS(300)) != pdPASS) {
                break;
            }
            request = latest;
        }

        panel_ui_settings_normalize(&request.settings);
        panel_ui_store_result_t result = PANEL_UI_STORE_RESULT_OK;
        if (!s_panel_settings_persisted_valid ||
            !panel_ui_settings_equal(&request.settings,
                                     &s_panel_settings_persisted)) {
            result = panel_ui_store_save(&request.settings);
        }
        const bool save_ok = result == PANEL_UI_STORE_RESULT_OK;
        portENTER_CRITICAL(&s_panel_settings_mux);
        if (save_ok) {
            s_panel_settings_persisted = request.settings;
            s_panel_settings_persisted_valid = true;
        }
        s_panel_settings_completed_generation = request.generation;
        s_panel_settings_last_save_ok = save_ok;
        portEXIT_CRITICAL(&s_panel_settings_mux);
        ESP_LOGI(TAG, "PANEL_UI settings_persistence=%s generation=%lu",
                 save_ok ? "verified" : "failed",
                 (unsigned long)request.generation);
    }
}

static void panel_ui_settings_storage_start(void)
{
    panel_ui_settings_defaults(&s_panel_settings_boot);
    const panel_ui_store_result_t load_result =
        panel_ui_store_load(&s_panel_settings_boot);
    panel_ui_settings_normalize(&s_panel_settings_boot);
    s_panel_settings_persisted = s_panel_settings_boot;
    s_panel_settings_persisted_valid =
        load_result != PANEL_UI_STORE_RESULT_IO_ERROR &&
        load_result != PANEL_UI_STORE_RESULT_INVALID_ARGUMENT;
    ESP_LOGI(TAG, "PANEL_UI settings_load_result=%u defaults_applied=%s",
             (unsigned)load_result,
             load_result == PANEL_UI_STORE_RESULT_OK ? "false" : "true");

    s_panel_settings_queue = xQueueCreate(1U,
        sizeof(panel_ui_settings_save_request_t));
    if (s_panel_settings_queue == NULL) {
        ESP_LOGE(TAG, "PANEL_UI settings_worker_ready=false reason=queue_allocation");
        return;
    }
    if (xTaskCreate(panel_ui_settings_persist_worker,
                    "panel_ui_persist", 4096, NULL, 4, NULL) != pdPASS) {
        vQueueDelete(s_panel_settings_queue);
        s_panel_settings_queue = NULL;
        ESP_LOGE(TAG, "PANEL_UI settings_worker_ready=false reason=task_create");
        return;
    }
    ESP_LOGI(TAG, "PANEL_UI settings_worker_ready=true");
}

static void panel_ui_settings_persist_request(
    const panel_ui_settings_t *settings)
{
    if (settings == NULL) return;
    panel_ui_settings_save_request_t request = {0};
    request.settings = *settings;
    panel_ui_settings_normalize(&request.settings);
    portENTER_CRITICAL(&s_panel_settings_mux);
    if (!s_panel_settings_publisher_ready) {
        portEXIT_CRITICAL(&s_panel_settings_mux);
        ESP_LOGE(TAG, "PANEL_UI settings_request_rejected=publisher_not_ready");
        panel_ui_settings_restore_confirmed();
        (void)panel_ui_set_settings_save_status(s_panel_ui, PANEL_UI_SETTINGS_SAVE_FAILED);
        return;
    }
    s_panel_settings_requested_generation++;
    if (s_panel_settings_requested_generation == 0U) {
        s_panel_settings_requested_generation++;
    }
    request.generation = s_panel_settings_requested_generation;
    portEXIT_CRITICAL(&s_panel_settings_mux);

    if (s_panel_settings_queue == NULL ||
        xQueueOverwrite(s_panel_settings_queue, &request) != pdPASS) {
        portENTER_CRITICAL(&s_panel_settings_mux);
        s_panel_settings_completed_generation = request.generation;
        s_panel_settings_last_save_ok = false;
        portEXIT_CRITICAL(&s_panel_settings_mux);
        ESP_LOGE(TAG, "PANEL_UI settings_request_queued=false");
        panel_ui_settings_restore_confirmed();
        (void)panel_ui_set_settings_save_status(s_panel_ui, PANEL_UI_SETTINGS_SAVE_FAILED);
    } else {
        /* Presentation only; existing queue/worker/generation policy is unchanged.
         * This callback runs under the existing display lock. */
        (void)panel_ui_set_settings_save_status(s_panel_ui, PANEL_UI_SETTINGS_SAVE_SAVING);
    }
}

static void panel_ui_settings_publish_result(void)
{
    if (s_panel_ui == NULL) return;
    uint32_t requested;
    uint32_t completed;
    uint32_t published;
    bool save_ok;
    panel_ui_settings_t confirmed_settings;
    bool restore_confirmed = false;
    portENTER_CRITICAL(&s_panel_settings_mux);
    /* The sole caller is rotation_task under the display lock. Reaching this
     * publisher proves it has run, rather than only being created. */
    s_panel_settings_publisher_ready = true;
    requested = s_panel_settings_requested_generation;
    completed = s_panel_settings_completed_generation;
    published = s_panel_settings_published_generation;
    save_ok = s_panel_settings_last_save_ok;
    if (requested != 0U && completed == requested && published != completed) {
        s_panel_settings_published_generation = completed;
        if (!save_ok) {
            confirmed_settings = s_panel_settings_persisted_valid
                ? s_panel_settings_persisted : s_panel_settings_boot;
            restore_confirmed = true;
        }
    }
    portEXIT_CRITICAL(&s_panel_settings_mux);
    if (requested != 0U && completed == requested && published != completed) {
        if (restore_confirmed)
            (void)panel_ui_restore_settings(s_panel_ui, &confirmed_settings);
        (void)panel_ui_set_settings_save_status(
            s_panel_ui, save_ok ? PANEL_UI_SETTINGS_SAVE_SAVED
                                : PANEL_UI_SETTINGS_SAVE_FAILED);
    }
}

static void sntp_time_sync_callback(struct timeval *tv)
{
    if (tv == NULL || !panel_clock_epoch_is_valid(tv->tv_sec)) {
        return;
    }
    portENTER_CRITICAL(&s_sntp_time_mux);
    s_sntp_time_received = true;
    portEXIT_CRITICAL(&s_sntp_time_mux);
}

static bool sntp_time_is_available(void)
{
    bool available;
    portENTER_CRITICAL(&s_sntp_time_mux);
    available = s_sntp_time_received;
    portEXIT_CRITICAL(&s_sntp_time_mux);
    return available;
}

static void request_sntp_start(void)
{
    portENTER_CRITICAL(&s_sntp_time_mux);
    s_sntp_start_pending = true;
    portEXIT_CRITICAL(&s_sntp_time_mux);
}

static void cancel_sntp_start_request(void)
{
    portENTER_CRITICAL(&s_sntp_time_mux);
    s_sntp_start_pending = false;
    portEXIT_CRITICAL(&s_sntp_time_mux);
}

static bool take_sntp_start_request(void)
{
    bool pending;
    portENTER_CRITICAL(&s_sntp_time_mux);
    pending = s_sntp_start_pending;
    s_sntp_start_pending = false;
    portEXIT_CRITICAL(&s_sntp_time_mux);
    return pending;
}

static void start_network_time_sync_once(void)
{
    if (s_sntp_init_attempted) {
        return;
    }
    s_sntp_init_attempted = true;

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.wait_for_sync = false;
    config.sync_cb = sntp_time_sync_callback;
    const esp_err_t result = esp_netif_sntp_init(&config);
    ESP_LOGI(TAG, "TIME_SYNC sntp_init=%s", esp_err_to_name(result));
}

static void update_panel_clock_if_due(void)
{
    if (s_panel_ui == NULL) {
        return;
    }

    const time_t now = time(NULL);
    const bool synchronized = s_panel_timezone_ready &&
        sntp_time_is_available() && panel_clock_epoch_is_valid(now);
    if (!panel_clock_render_is_due(
            &s_panel_clock_render_state, now, synchronized)) {
        return;
    }

    struct tm local_time;
    const struct tm *display_time = NULL;
    if (synchronized && localtime_r(&now, &local_time) != NULL) {
        display_time = &local_time;
    } else if (synchronized) {
        return;
    }

    (void)panel_ui_set_time(s_panel_ui, display_time, display_time != NULL);
    panel_clock_render_committed(
        &s_panel_clock_render_state, now, display_time != NULL);
}

typedef struct {
    uint64_t refresh_start_us;
    uint64_t flush_start_us;
    uint64_t window_start_us;
    uint64_t refresh_total_us;
    uint64_t refresh_max_us;
    uint64_t flush_total_us;
    uint64_t flush_max_us;
    uint32_t refresh_count;
    uint32_t flush_count;
    uint64_t lock_wait_total_us;
    uint64_t lock_wait_max_us;
    uint64_t lock_hold_total_us;
    uint64_t lock_hold_max_us;
    uint32_t lock_count;
    uint32_t lock_fail_count;
    uint32_t homey_lock_count;
    uint32_t rotation_lock_count;
    uint32_t homey_refresh_requests;
    uint32_t homey_model_changed_count;
    uint32_t homey_favorites_changed_count;
} panel_display_perf_t;

static panel_display_perf_t s_panel_perf;

static bool panel_display_lock_traced(
    uint32_t timeout_ms,
    bool homey_path,
    uint64_t *acquired_us)
{
    const uint64_t start_us = (uint64_t)esp_timer_get_time();
    const bool locked = bsp_display_lock(timeout_ms);
    const uint64_t end_us = (uint64_t)esp_timer_get_time();
    const uint64_t wait_us = end_us - start_us;
    if (!locked) {
        s_panel_perf.lock_fail_count++;
        return false;
    }

    s_panel_perf.lock_count++;
    s_panel_perf.lock_wait_total_us += wait_us;
    if (wait_us > s_panel_perf.lock_wait_max_us) {
        s_panel_perf.lock_wait_max_us = wait_us;
    }
    if (homey_path) s_panel_perf.homey_lock_count++;
    else s_panel_perf.rotation_lock_count++;
    if (acquired_us != NULL) *acquired_us = end_us;
    return true;
}

static void panel_display_unlock_traced(uint64_t acquired_us)
{
    const uint64_t hold_us = (uint64_t)esp_timer_get_time() - acquired_us;
    s_panel_perf.lock_hold_total_us += hold_us;
    if (hold_us > s_panel_perf.lock_hold_max_us) {
        s_panel_perf.lock_hold_max_us = hold_us;
    }
    bsp_display_unlock();
}

#if defined(PATCH107_STARTUP_TITLE_DIAG)
typedef struct {
    panel_startup_title_diag_id_t id;
    bool active_screen;
    bool hidden;
    bool visible;
    const char *font_id;
    size_t text_len_bytes;
    uint32_t text_color;
    uint8_t text_opa;
    uint8_t effective_opa;
    lv_area_t object_area;
    panel_startup_title_diag_rect_t visible_area;
    bool visible_area_valid;
} panel_startup_title_diag_capture_t;

static bool startup_title_diag_capture_locked(
    const char *title,
    const char *detail,
    panel_startup_title_diag_capture_t *capture)
{
    if (capture == NULL || s_title == NULL) return false;
    memset(capture, 0, sizeof(*capture));
    capture->id = panel_startup_title_diag_classify(title, detail);
    if (!panel_startup_title_diag_log_transition(&s_startup_title_diag_gate, capture->id)) {
        return false;
    }

    capture->active_screen = lv_obj_get_screen(s_title) == lv_screen_active();
    capture->hidden = lv_obj_is_hidden(s_title);
    capture->visible = lv_obj_is_visible(s_title);
    const lv_font_t *font = lv_obj_get_style_text_font(s_title, LV_PART_MAIN);
    capture->font_id = font == &homey_panel_font_27 ? "homey_panel_font_27" :
                       font == &homey_panel_font_22 ? "homey_panel_font_22" : "other";
    const char *rendered_text = lv_label_get_text(s_title);
    capture->text_len_bytes = rendered_text != NULL ? strlen(rendered_text) : 0U;
    capture->text_color = lv_color_to_u32(
        lv_obj_get_style_text_color(s_title, LV_PART_MAIN));
    capture->text_opa = lv_obj_get_style_text_opa(s_title, LV_PART_MAIN);
    const lv_opa_t ancestor_opa = lv_obj_get_style_opa_recursive(s_title, LV_PART_MAIN);
    capture->effective_opa = LV_OPA_MIX2(capture->text_opa, ancestor_opa);
    lv_obj_get_coords(s_title, &capture->object_area);

    const int32_t width = (int32_t)capture->object_area.x2 - capture->object_area.x1 + 1;
    const int32_t height = (int32_t)capture->object_area.y2 - capture->object_area.y1 + 1;
    const lv_display_t *display = lv_display_get_default();
    const int32_t screen_width = display != NULL ? lv_display_get_horizontal_resolution(display) : 0;
    const int32_t screen_height = display != NULL ? lv_display_get_vertical_resolution(display) : 0;
    if (capture->active_screen && capture->visible && !capture->hidden) {
        const panel_startup_title_diag_rect_t object_rect = {
            .x = capture->object_area.x1,
            .y = capture->object_area.y1,
            .width = width,
            .height = height,
        };
        capture->visible_area_valid = panel_startup_title_diag_clip_rect(
            &object_rect, screen_width, screen_height, &capture->visible_area);
    }

    s_startup_title_diag_pending_id = capture->id;
    s_startup_title_diag_visible_rect = capture->visible_area;
    s_startup_title_diag_transition_us = (uint64_t)esp_timer_get_time();
    s_startup_title_diag_waiting_refresh = true;
    s_startup_title_diag_refresh_seen = false;
    s_startup_title_diag_flush_intersects = false;
    return true;
}

static void startup_title_diag_log_capture(const panel_startup_title_diag_capture_t *capture)
{
    if (capture == NULL) return;
    ESP_LOGI(TAG,
        "PATCH107_STARTUP_TITLE_DIAG id=%s active_screen=%s hidden=%s visible=%s font=%s text_len_bytes=%u color=0x%06lx text_opa=%u effective_opa=%u x=%d y=%d w=%d h=%d visible_area=%s:%d,%d,%d,%d flush_intersection=UNKNOWN privacy=text_omitted",
        panel_startup_title_diag_id_name(capture->id),
        capture->active_screen ? "yes" : "no",
        capture->hidden ? "yes" : "no",
        capture->visible ? "yes" : "no",
        capture->font_id,
        (unsigned)capture->text_len_bytes,
        (unsigned long)capture->text_color,
        (unsigned)capture->text_opa,
        (unsigned)capture->effective_opa,
        (int)capture->object_area.x1,
        (int)capture->object_area.y1,
        (int)((int32_t)capture->object_area.x2 - capture->object_area.x1 + 1),
        (int)((int32_t)capture->object_area.y2 - capture->object_area.y1 + 1),
        capture->visible_area_valid ? "yes" : "no",
        (int)capture->visible_area.x,
        (int)capture->visible_area.y,
        (int)capture->visible_area.width,
        (int)capture->visible_area.height);
}
#endif

static void panel_display_perf_event(lv_event_t *event)
{
    const uint64_t now = (uint64_t)esp_timer_get_time();
    switch (lv_event_get_code(event)) {
    case LV_EVENT_REFR_START:
        s_panel_perf.refresh_start_us = now;
#if defined(PATCH107_STARTUP_TITLE_DIAG)
        if (s_startup_title_diag_waiting_refresh) {
            if (now >= s_startup_title_diag_transition_us &&
                now - s_startup_title_diag_transition_us <= 250000ULL) {
                s_startup_title_diag_refresh_seen = true;
                s_startup_title_diag_flush_intersects = false;
            } else {
                s_startup_title_diag_waiting_refresh = false;
            }
        }
#endif
        break;
    case LV_EVENT_REFR_READY:
        if (s_panel_perf.refresh_start_us != 0U) {
            uint64_t elapsed = now - s_panel_perf.refresh_start_us;
            s_panel_perf.refresh_total_us += elapsed;
            if (elapsed > s_panel_perf.refresh_max_us) s_panel_perf.refresh_max_us = elapsed;
            s_panel_perf.refresh_count++;
            s_panel_perf.refresh_start_us = 0U;
        }
#if defined(PATCH107_STARTUP_TITLE_DIAG)
        if (s_startup_title_diag_waiting_refresh && s_startup_title_diag_refresh_seen) {
            ESP_LOGI(TAG,
                "PATCH107_STARTUP_TITLE_FLUSH id=%s refresh_observed=yes flush_intersection=%s area_source=LVGL_FLUSH_EVENT",
                panel_startup_title_diag_id_name(s_startup_title_diag_pending_id),
                s_startup_title_diag_flush_intersects ? "yes" : "no");
            s_startup_title_diag_waiting_refresh = false;
        }
#endif
        break;
    case LV_EVENT_FLUSH_START:
        s_panel_perf.flush_start_us = now;
#if defined(PATCH107_STARTUP_TITLE_DIAG)
        if (s_startup_title_diag_waiting_refresh && s_startup_title_diag_refresh_seen) {
            const lv_area_t *flush_area = lv_event_get_param(event);
            if (flush_area != NULL && panel_startup_title_diag_intersects_flush(
                    &s_startup_title_diag_visible_rect,
                    flush_area->x1, flush_area->y1, flush_area->x2, flush_area->y2)) {
                s_startup_title_diag_flush_intersects = true;
            }
        }
#endif
        break;
    case LV_EVENT_FLUSH_FINISH:
        if (s_panel_perf.flush_start_us != 0U) {
            uint64_t elapsed = now - s_panel_perf.flush_start_us;
            s_panel_perf.flush_total_us += elapsed;
            if (elapsed > s_panel_perf.flush_max_us) s_panel_perf.flush_max_us = elapsed;
            s_panel_perf.flush_count++;
            s_panel_perf.flush_start_us = 0U;
        }
        break;
    default:
        break;
    }
}

static void panel_display_perf_log_if_due(void)
{
    const uint64_t now = (uint64_t)esp_timer_get_time();
    if (s_panel_perf.window_start_us == 0U) s_panel_perf.window_start_us = now;
    uint64_t window_us = now - s_panel_perf.window_start_us;
    if (window_us < 5000000ULL) return;
    uint64_t refresh_avg = s_panel_perf.refresh_count == 0U ? 0U :
        s_panel_perf.refresh_total_us / s_panel_perf.refresh_count;
    uint64_t flush_avg = s_panel_perf.flush_count == 0U ? 0U :
        s_panel_perf.flush_total_us / s_panel_perf.flush_count;
    uint64_t lock_wait_avg = s_panel_perf.lock_count == 0U ? 0U :
        s_panel_perf.lock_wait_total_us / s_panel_perf.lock_count;
    uint64_t lock_hold_avg = s_panel_perf.lock_count == 0U ? 0U :
        s_panel_perf.lock_hold_total_us / s_panel_perf.lock_count;
    ESP_LOGI(TAG, "PANEL_PERF window_ms=%llu refresh_count=%lu refresh_avg_us=%llu refresh_max_us=%llu flush_count=%lu flush_avg_us=%llu flush_max_us=%llu",
        (unsigned long long)(window_us / 1000ULL),
        (unsigned long)s_panel_perf.refresh_count,
        (unsigned long long)refresh_avg,
        (unsigned long long)s_panel_perf.refresh_max_us,
        (unsigned long)s_panel_perf.flush_count,
        (unsigned long long)flush_avg,
        (unsigned long long)s_panel_perf.flush_max_us);
    ESP_LOGI(TAG, "PATCH021_DISPLAY_PERF window_ms=%llu refresh_count=%lu refresh_avg_us=%llu refresh_max_us=%llu flush_count=%lu flush_avg_us=%llu flush_max_us=%llu rotation_poll_ms=%u privacy=sanitized",
        (unsigned long long)(window_us / 1000ULL),
        (unsigned long)s_panel_perf.refresh_count,
        (unsigned long long)refresh_avg,
        (unsigned long long)s_panel_perf.refresh_max_us,
        (unsigned long)s_panel_perf.flush_count,
        (unsigned long long)flush_avg,
        (unsigned long long)s_panel_perf.flush_max_us,
        (unsigned)ROTATION_POLL_INTERVAL_MS);
    ESP_LOGI(TAG, "PATCH024_RENDER_PATH window_ms=%llu lock_count=%lu lock_fail_count=%lu lock_wait_avg_us=%llu lock_wait_max_us=%llu lock_hold_avg_us=%llu lock_hold_max_us=%llu homey_lock_count=%lu rotation_lock_count=%lu homey_refresh_requests=%lu homey_model_changed=%lu homey_favorites_changed=%lu privacy=sanitized",
        (unsigned long long)(window_us / 1000ULL),
        (unsigned long)s_panel_perf.lock_count,
        (unsigned long)s_panel_perf.lock_fail_count,
        (unsigned long long)lock_wait_avg,
        (unsigned long long)s_panel_perf.lock_wait_max_us,
        (unsigned long long)lock_hold_avg,
        (unsigned long long)s_panel_perf.lock_hold_max_us,
        (unsigned long)s_panel_perf.homey_lock_count,
        (unsigned long)s_panel_perf.rotation_lock_count,
        (unsigned long)s_panel_perf.homey_refresh_requests,
        (unsigned long)s_panel_perf.homey_model_changed_count,
        (unsigned long)s_panel_perf.homey_favorites_changed_count);
    s_panel_perf.window_start_us = now;
    s_panel_perf.refresh_total_us = 0U;
    s_panel_perf.refresh_max_us = 0U;
    s_panel_perf.flush_total_us = 0U;
    s_panel_perf.flush_max_us = 0U;
    s_panel_perf.refresh_count = 0U;
    s_panel_perf.flush_count = 0U;
    s_panel_perf.lock_wait_total_us = 0U;
    s_panel_perf.lock_wait_max_us = 0U;
    s_panel_perf.lock_hold_total_us = 0U;
    s_panel_perf.lock_hold_max_us = 0U;
    s_panel_perf.lock_count = 0U;
    s_panel_perf.lock_fail_count = 0U;
    s_panel_perf.homey_lock_count = 0U;
    s_panel_perf.rotation_lock_count = 0U;
    s_panel_perf.homey_refresh_requests = 0U;
    s_panel_perf.homey_model_changed_count = 0U;
    s_panel_perf.homey_favorites_changed_count = 0U;
}
static wifi_config_t s_saved_config, s_candidate_config;
static secure_bootstrap_network_snapshot_t s_network_snapshot;
static secure_bootstrap_scan_state_t s_scan_state;
static SemaphoreHandle_t s_networks_mutex;
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
static bool s_cancel_task_scheduled;
static wifi_persist_stage_t s_persist_stage;
static int64_t s_last_code_countdown_s = -1;

static bool wifi_session_snapshot(
    uint32_t *generation, secure_bootstrap_session_phase_t *phase)
{
    bool present;
    portENTER_CRITICAL(&s_wifi_session_mux);
    present = secure_bootstrap_wifi_session_current(
        &s_wifi_session, generation, phase);
    portEXIT_CRITICAL(&s_wifi_session_mux);
    return present;
}

static bool wifi_session_is_live(uint32_t generation)
{
    bool live;
    portENTER_CRITICAL(&s_wifi_session_mux);
    live = secure_bootstrap_wifi_session_is_live(&s_wifi_session, generation);
    portEXIT_CRITICAL(&s_wifi_session_mux);
    return live;
}

static uint32_t wifi_transition_serialized(secure_bootstrap_wifi_event_t event)
{
    uint32_t actions;
    portENTER_CRITICAL(&s_wifi_session_mux);
    actions = secure_bootstrap_wifi_transition(&s_wifi, event);
    portEXIT_CRITICAL(&s_wifi_session_mux);
    return actions;
}

static secure_bootstrap_wifi_state_t wifi_state_snapshot(void)
{
    secure_bootstrap_wifi_state_t state;
    portENTER_CRITICAL(&s_wifi_session_mux);
    state = s_wifi.state;
    portEXIT_CRITICAL(&s_wifi_session_mux);
    return state;
}

static void wifi_persist_state_snapshot(
    bool *active, wifi_persist_stage_t *stage)
{
    portENTER_CRITICAL(&s_wifi_session_mux);
    if (active != NULL) *active = s_persist_active;
    if (stage != NULL) *stage = s_persist_stage;
    portEXIT_CRITICAL(&s_wifi_session_mux);
}

static bool reconfigure_pending_get(void)
{
    bool pending;
    portENTER_CRITICAL(&s_wifi_session_mux);
    pending = s_reconfigure_pending;
    portEXIT_CRITICAL(&s_wifi_session_mux);
    return pending;
}

static bool reconfigure_pending_set_if_clear(void)
{
    bool accepted = false;
    portENTER_CRITICAL(&s_wifi_session_mux);
    if (!s_reconfigure_pending) {
        s_reconfigure_pending = true;
        accepted = true;
    }
    portEXIT_CRITICAL(&s_wifi_session_mux);
    return accepted;
}

static void reconfigure_pending_set(bool pending)
{
    portENTER_CRITICAL(&s_wifi_session_mux);
    s_reconfigure_pending = pending;
    portEXIT_CRITICAL(&s_wifi_session_mux);
}

/* Caller holds s_ap_config_mutex so the AP password and its displayed QR/text
 * are one consistent session snapshot. */
static void panel_wifi_present_session_locked(void)
{
    uint32_t generation = 0U;
    secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    if (!wifi_session_snapshot(&generation, &phase) ||
        (phase != SECURE_BOOTSTRAP_SESSION_ACTIVE &&
         phase != SECURE_BOOTSTRAP_SESSION_CANDIDATE &&
         phase != SECURE_BOOTSTRAP_SESSION_COMMIT_RESERVED &&
         phase != SECURE_BOOTSTRAP_SESSION_COMMITTING)) return;
    if (s_panel_ui == NULL || !bsp_display_lock(1000)) return;
    const bool commit_reserved =
        phase == SECURE_BOOTSTRAP_SESSION_COMMIT_RESERVED ||
        phase == SECURE_BOOTSTRAP_SESSION_COMMITTING;
    const bool published = panel_ui_update_wifi_provisioning(
        s_panel_ui, s_qr, commit_reserved ? "" : AP_SSID,
        commit_reserved ? "" : s_code.code,
        !commit_reserved);
    if (!published) {
        (void)panel_ui_set_wifi_reconfigure_result(
            s_panel_ui, PANEL_UI_WIFI_RECONFIGURE_FAILED);
        bsp_display_unlock();
        return;
    }
    (void)panel_ui_present_wifi_provisioning(s_panel_ui, s_qr, commit_reserved);
    bsp_display_unlock();
}

static void panel_wifi_present_session(void)
{
    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) return;
    panel_wifi_present_session_locked();
    (void)xSemaphoreGive(s_ap_config_mutex);
}

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
    if (wifi_state_snapshot() != SECURE_BOOTSTRAP_WIFI_PROVISIONING ||
        s_code.code[0] == '\0') {
        s_last_code_countdown_s = -1;
        return;
    }

    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) return;
    int64_t current_s = now_s();
    int64_t remaining_s =
        s_code.issued_at_s + SECURE_BOOTSTRAP_CODE_TTL_SECONDS - current_s;
    if (remaining_s < 0) {
        remaining_s = 0;
    }
    if (remaining_s == s_last_code_countdown_s) {
        (void)xSemaphoreGive(s_ap_config_mutex);
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
    (void)xSemaphoreGive(s_ap_config_mutex);
}

static void set_display_text(
    const char *title,
    const char *detail,
    const char *display_code)
{
    if (!bsp_display_lock(1000)) {
        return;
    }

    if (s_startup_layout_active) {
        if (s_startup_logo != NULL) lv_obj_set_hidden(s_startup_logo, true);
        lv_obj_set_style_pad_top(s_detail, 0, 0);
        lv_obj_set_height(s_title, LV_SIZE_CONTENT);
        lv_obj_set_height(s_detail, LV_SIZE_CONTENT);
        lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, TITLE_Y);
        lv_obj_align(s_detail, LV_ALIGN_TOP_MID, 0, DETAIL_Y);
        lv_obj_set_style_text_font(s_title, &homey_panel_font_22, 0);
        lv_obj_remove_flag(s_title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_hidden(s_detail, false);
        s_startup_layout_active = false;
    }

    bool bootstrap_visible = display_code != NULL && display_code[0] != '\0';
    uint32_t session_generation = 0U;
    secure_bootstrap_session_phase_t session_phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    const bool preserve_wifi_session = wifi_session_snapshot(
        &session_generation, &session_phase) &&
        session_phase != SECURE_BOOTSTRAP_SESSION_CLOSED &&
        session_phase != SECURE_BOOTSTRAP_SESSION_OPENING;
    lv_label_set_text(s_title, title != NULL ? title : "");
    lv_label_set_text(s_detail, detail != NULL ? detail : "");
    if (bootstrap_visible) {
        char caption[32];
        format_code_caption(caption, sizeof(caption), now_s());
        lv_label_set_text(s_code_caption, caption);
        secure_zero(caption, sizeof(caption));
    } else if (!preserve_wifi_session) {
        lv_label_set_text(s_code_caption, "");
        s_last_code_countdown_s = -1;
    }
    if (display_code != NULL || !preserve_wifi_session) {
        lv_label_set_text(s_code_label, display_code != NULL ? display_code : "");
    }
    if (s_qr != NULL && (display_code != NULL || !preserve_wifi_session)) {
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

    if (bootstrap_visible && s_panel_ui != NULL) {
        uint32_t generation = 0U;
        secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
        if (wifi_session_snapshot(&generation, &phase) &&
            (phase == SECURE_BOOTSTRAP_SESSION_ACTIVE ||
             phase == SECURE_BOOTSTRAP_SESSION_CANDIDATE ||
             phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED)) {
            const bool published = panel_ui_update_wifi_provisioning(
                s_panel_ui, s_qr, AP_SSID, display_code,
                phase != SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED);
            if (!published) {
                (void)panel_ui_set_wifi_reconfigure_result(
                    s_panel_ui, PANEL_UI_WIFI_RECONFIGURE_FAILED);
            }
        }
    }

    bsp_display_unlock();
}

/* Startup messages reuse the existing labels; their y positions are recomputed
 * from LVGL's wrapped content heights, without blocking network work. */
static bool set_startup_display_text(
    const char *title,
    const char *detail,
    bool error)
{
    if (s_title == NULL || s_detail == NULL) return false;
    char visible_status_text[STARTUP_STATUS_TEXT_CAPACITY];
    if (!panel_startup_compose_visible_text(
            visible_status_text, sizeof(visible_status_text), title, detail)) {
        return false;
    }
    if (!bsp_display_lock(0)) return false;
#if defined(PATCH107_STARTUP_TITLE_DIAG)
    panel_startup_title_diag_capture_t title_diag_capture;
    bool title_diag_log = false;
#endif

    bool layout_changed = false;
    if (!s_startup_layout_active) {
        if (s_startup_logo != NULL) lv_obj_set_hidden(s_startup_logo, false);
        lv_obj_set_width(s_title, DISPLAY_WIDTH - (2 * DISPLAY_MARGIN));
        lv_obj_set_width(s_detail, DISPLAY_WIDTH - (2 * DISPLAY_MARGIN));
        lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_align(s_detail, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(s_detail, &homey_panel_font_22, 0);
        lv_label_set_text(s_title, "");
        lv_obj_add_flag(s_title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_detail, LV_OBJ_FLAG_HIDDEN);
        s_startup_layout_active = true;
        layout_changed = true;
    }
    (void)error;
    if (!lv_obj_has_flag(s_title, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(s_title, LV_OBJ_FLAG_HIDDEN);
        layout_changed = true;
    }
    const char *current_visible_text = lv_label_get_text(s_detail);
    if (current_visible_text == NULL ||
        strcmp(current_visible_text, visible_status_text) != 0) {
        lv_label_set_text(s_detail, visible_status_text);
        layout_changed = true;
    }
    if (lv_obj_has_flag(s_detail, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_remove_flag(s_detail, LV_OBJ_FLAG_HIDDEN);
        layout_changed = true;
    }

    if (!layout_changed) {
#if defined(PATCH107_STARTUP_TITLE_DIAG)
        title_diag_log = startup_title_diag_capture_locked(title, detail, &title_diag_capture);
#endif
        bsp_display_unlock();
#if defined(PATCH107_STARTUP_TITLE_DIAG)
        if (title_diag_log) startup_title_diag_log_capture(&title_diag_capture);
#endif
        return true;
    }
    lv_obj_set_style_pad_top(s_detail, 0, 0);
    lv_obj_set_height(s_detail, LV_SIZE_CONTENT);
    lv_obj_update_layout(s_detail);
    const uint16_t text_height = (uint16_t)lv_obj_get_height(s_detail);
    panel_startup_layout_t layout;
    bool layout_ok = panel_startup_layout_compute(
        DISPLAY_HEIGHT,
        TOUCH_Y + TOUCH_HEIGHT,
        STARTUP_WIFI_BUTTON_GAP,
        STARTUP_STATUS_BOTTOM_MARGIN,
        0U,
        text_height,
        0U,
        &layout);
    const uint16_t status_area_top = TOUCH_Y + TOUCH_HEIGHT + STARTUP_WIFI_BUTTON_GAP;
    const uint16_t status_area_height = DISPLAY_HEIGHT -
        STARTUP_STATUS_BOTTOM_MARGIN - status_area_top;
    if (!layout_ok || text_height > status_area_height) {
        bsp_display_unlock();
        return false;
    }
    lv_obj_set_style_pad_top(
        s_detail, (lv_coord_t)(status_area_height - text_height), 0);
    lv_obj_set_height(s_detail, (lv_coord_t)status_area_height);
    lv_obj_align(s_detail, LV_ALIGN_TOP_MID, 0, (lv_coord_t)status_area_top);

#if defined(PATCH107_STARTUP_TITLE_DIAG)
    title_diag_log = startup_title_diag_capture_locked(title, detail, &title_diag_capture);
#endif
    bsp_display_unlock();
#if defined(PATCH107_STARTUP_TITLE_DIAG)
    if (title_diag_log) startup_title_diag_log_capture(&title_diag_capture);
#endif
    return true;
}

static bool startup_status_active_get(void)
{
    bool active;
    portENTER_CRITICAL(&s_startup_status_mux);
    active = s_startup_status_active;
    portEXIT_CRITICAL(&s_startup_status_mux);
    return active;
}

static void startup_status_active_set(bool active)
{
    portENTER_CRITICAL(&s_startup_status_mux);
    s_startup_status_active = active;
    portEXIT_CRITICAL(&s_startup_status_mux);
}

/* Restore/live-ready callbacks must not replace the timed startup sequence.
 * Explicit provisioning regains display ownership after its touch action. */
static void panel_provisioning_display_update(
    const char *title,
    const char *detail,
    const char *display_code)
{
    if (startup_status_active_get()) {
        ESP_LOGD(TAG, "STARTUP_UI provisioning_display_suppressed=true");
        return;
    }
    if (detail != NULL &&
        (strcmp(detail, "Status: Ansluten") == 0 ||
         strcmp(detail, "Status: READY") == 0)) {
        ESP_LOGD(TAG, "STARTUP_UI runtime_homey_label_suppressed=true");
        return;
    }
    set_display_text(title, detail, display_code);
}

static esp_err_t apply_actions(uint32_t actions);
static void reconfigure_task(void *arg);
static void panel_interaction_trace(void *context, panel_ui_trace_event_t event, bool callback_present)
{
    (void)context;
    const char *name = event == PANEL_UI_TRACE_WIFI_CLICK ? "wifi_click" : "choose_homey_click";
    ESP_LOGI(TAG, "PANEL_UI interaction=%s callback_present=%s",
             name, callback_present ? "true" : "false");
}

static void panel_wifi_result_update(panel_ui_wifi_reconfigure_result_t result)
{
    if (s_panel_ui == NULL) return;
    if (!bsp_display_lock(50)) {
        ESP_LOGW(TAG, "PANEL_UI wifi_result_lock=false result=%u", (unsigned)result);
        return;
    }
    bool updated = panel_ui_set_wifi_reconfigure_result(s_panel_ui, result);
    bsp_display_unlock();
    ESP_LOGI(TAG, "PANEL_UI wifi_result=%u updated=%s",
             (unsigned)result, updated ? "true" : "false");
}

static void panel_brightness_request(void *context, uint8_t value)
{
    (void)context;
    const char *operation = value == 0U ? "off" : "set";
    ESP_LOGI(TAG, "PANEL_UI brightness_request=%u operation=%s",
             (unsigned)value, operation);
    const esp_err_t err = bsp_display_brightness_set((int)value);
    ESP_LOGI(TAG, "PANEL_UI backlight_driver=bsp-pwm");
    ESP_LOGI(TAG, "PANEL_UI brightness_result=%s", esp_err_to_name(err));
}

static void wifi_reopen_task(void *arg)
{
    (void)arg;
    panel_wifi_present_session();
    vTaskDelete(NULL);
}

static void panel_wifi_request(void *context)
{
    (void)context;
    uint32_t session_generation = 0U;
    secure_bootstrap_session_phase_t session_phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    if (wifi_session_snapshot(&session_generation, &session_phase) &&
        (session_phase == SECURE_BOOTSTRAP_SESSION_ACTIVE ||
         session_phase == SECURE_BOOTSTRAP_SESSION_CANDIDATE)) {
        if (xTaskCreate(wifi_reopen_task, "wifi_reopen", 3072, NULL, 5, NULL) != pdPASS) {
            (void)panel_ui_set_wifi_reconfigure_result(
                (panel_ui_t *)context, PANEL_UI_WIFI_RECONFIGURE_FAILED);
        }
        return;
    }
    if (session_phase == SECURE_BOOTSTRAP_SESSION_COMMIT_RESERVED ||
        session_phase == SECURE_BOOTSTRAP_SESSION_COMMITTING) {
        (void)xTaskCreate(wifi_reopen_task, "wifi_reopen", 3072, NULL, 5, NULL);
        return;
    }
    const secure_bootstrap_wifi_state_t wifi_state = wifi_state_snapshot();
    ESP_LOGI(TAG, "PANEL_UI wifi_reconfigure_callback=true state=%u pending=%s",
             (unsigned)wifi_state, reconfigure_pending_get() ? "true" : "false");
    if (reconfigure_pending_get()) {
        ESP_LOGW(TAG, "PANEL_UI wifi_reconfigure_blocked=pending");
        (void)panel_ui_set_wifi_reconfigure_result(
            (panel_ui_t *)context, PANEL_UI_WIFI_RECONFIGURE_BLOCKED);
        return;
    }
    if (wifi_state != SECURE_BOOTSTRAP_WIFI_ONLINE) {
        ESP_LOGW(TAG, "PANEL_UI wifi_reconfigure_blocked=state state=%u", (unsigned)wifi_state);
        (void)panel_ui_set_wifi_reconfigure_result(
            (panel_ui_t *)context, PANEL_UI_WIFI_RECONFIGURE_BLOCKED);
        return;
    }
    if (!reconfigure_pending_set_if_clear()) return;
    if (xTaskCreate(reconfigure_task, "wifi_reconfigure", 4096, NULL, 5, NULL) != pdPASS) {
        reconfigure_pending_set(false);
        ESP_LOGE(TAG, "PANEL_UI wifi_reconfigure_task_create=false");
        (void)panel_ui_set_wifi_reconfigure_result(
            (panel_ui_t *)context, PANEL_UI_WIFI_RECONFIGURE_FAILED);
    } else {
        ESP_LOGI(TAG, "PANEL_UI wifi_reconfigure_task_create=true");
    }
}

static void wifi_cancel_task(void *arg);

static void panel_wifi_cancel_request(void *context)
{
    (void)context;
    uint32_t generation = 0U;
    secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    bool reserved = false;
    portENTER_CRITICAL(&s_wifi_session_mux);
    if (!s_cancel_task_scheduled &&
        secure_bootstrap_wifi_session_current(&s_wifi_session, &generation, &phase) &&
        secure_bootstrap_wifi_session_reserve_cancel(&s_wifi_session, generation)) {
        s_cancel_task_scheduled = true;
        reserved = true;
    }
    portEXIT_CRITICAL(&s_wifi_session_mux);

    if (!reserved) {
        return;
    }
    if (xTaskCreate(wifi_cancel_task, "wifi_cancel", 4096,
                    (void *)(uintptr_t)generation, 5, NULL) != pdPASS) {
        portENTER_CRITICAL(&s_wifi_session_mux);
        (void)secure_bootstrap_wifi_session_cancel_start_failed(&s_wifi_session, generation);
        s_cancel_task_scheduled = false;
        portEXIT_CRITICAL(&s_wifi_session_mux);
        (void)panel_ui_set_wifi_cancel_pending(
            (panel_ui_t *)context, false, false);
        return;
    }
}

/* Written/read only while holding the existing BSP display lock. The normal
 * dashboard poll remains its sole producer; no extra snapshot/Homey request. */
static panel_information_snapshot_t s_information_snapshot;

static bool panel_read_information(void *context, panel_information_kind_t kind,
                                   panel_information_t *out)
{
    (void)context;
    if (out == NULL || (unsigned)kind >= PANEL_INFORMATION_COUNT) return false;
    memset(out, 0, sizeof(*out));
    out->now_ms = (uint64_t)(esp_timer_get_time() / 1000LL);
    if (kind == PANEL_INFORMATION_WIFI || kind == PANEL_INFORMATION_HOMEY) {
        out->wifi_known = true;
        out->wifi_online = phone_provisioning_wifi_online();
    }
    if (kind == PANEL_INFORMATION_WIFI) {
        uint32_t generation = 0U;
        secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
        out->provisioning_known = true;
        out->provisioning_active = wifi_session_snapshot(&generation, &phase) &&
            phase != SECURE_BOOTSTRAP_SESSION_CLOSED;
    }
    if (kind == PANEL_INFORMATION_HOMEY || kind == PANEL_INFORMATION_SYSTEM) {
        out->homey_known = true;
        out->homey_state = (unsigned)athom_oauth_runtime_homey_data_state();
        out->snapshot = s_information_snapshot;
    }
    if (kind == PANEL_INFORMATION_FIRMWARE) {
        const esp_app_desc_t *app = esp_app_get_description();
        if (app != NULL) {
            out->firmware_known = true;
            memcpy(out->version, app->version, sizeof(out->version));
            memcpy(out->build_date, app->date, sizeof(app->date));
            memcpy(out->build_time, app->time, sizeof(app->time));
            memcpy(out->idf_version, app->idf_ver, sizeof(out->idf_version));
        }
    }
    if (kind == PANEL_INFORMATION_TIME) {
        out->time_known = true;
        out->swedish_timezone = s_panel_timezone_ready;
        out->time_synchronized = sntp_time_is_available() && panel_clock_epoch_is_valid(time(NULL));
    }
    if (kind == PANEL_INFORMATION_SYSTEM) {
        const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
        out->memory_known = true;
        out->internal_free = (uint32_t)heap_caps_get_free_size(caps);
        out->internal_minimum = (uint32_t)heap_caps_get_minimum_free_size(caps);
        out->psram_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
        /* panel_read_information is called from the LVGL information events
         * while the existing BSP display lock is held. LV_USE_OS=0 means the
         * allocator monitor has no separate internal mutex. */
        lv_mem_monitor_t lvgl_memory = {0};
        lv_mem_monitor(&lvgl_memory);
        if (lvgl_memory.total_size > 0U &&
            lvgl_memory.free_size <= lvgl_memory.total_size) {
            const size_t lvgl_current_used =
                lvgl_memory.total_size - lvgl_memory.free_size;
            if (lvgl_current_used <= LV_MEM_SIZE &&
                lvgl_memory.max_used <= LV_MEM_SIZE) {
                out->lvgl_memory_known = true;
                out->lvgl_pool_size = (uint32_t)LV_MEM_SIZE;
                out->lvgl_current_used = (uint32_t)lvgl_current_used;
                out->lvgl_max_used = (uint32_t)lvgl_memory.max_used;
            }
        }
#endif
    }
    return true;
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
    panel_ui_settings_persist_request(settings);
}

static bool panel_light_toggle_request(
    void *context, size_t widget_index, bool value)
{
    (void)context;
    const athom_light_toggle_queue_result_t result =
        athom_oauth_runtime_queue_light_toggle(widget_index, value);
    ESP_LOGI(TAG,
             "PATCH038_LIGHT_UI_QUEUE widget=%u result=%u privacy=sanitized",
             (unsigned)widget_index, (unsigned)result);
    return result == ATHOM_LIGHT_TOGGLE_QUEUE_QUEUED;
}

static bool panel_roller_step_request(
    void *context, size_t widget_index, int8_t direction)
{
    (void)context;
    const athom_awning_queue_result_t result =
        athom_oauth_runtime_queue_awning_step(widget_index, direction);
    ESP_LOGI(TAG,
             "PATCH085_AWNING_UI_QUEUE widget=%u direction=%s result=%u "
             "touch_feedback_only=yes privacy=sanitized",
             (unsigned)widget_index, direction < 0 ? "up" : "down",
             (unsigned)result);
    return result == ATHOM_AWNING_QUEUE_QUEUED;
}

static bool panel_show_dashboard(void)
{
    if (s_panel_ui == NULL || !bsp_display_lock(1000)) return false;
    const bool snapshot_fresh = s_homey_dashboard_state.generation_valid &&
        !s_homey_dashboard_state.stale;
    const bool presentation_ready = phone_provisioning_wifi_online() &&
        athom_oauth_runtime_homey_data_state() == ATHOM_HOMEY_DATA_READY &&
        snapshot_fresh;
    if (!presentation_ready) {
        bsp_display_unlock();
        ESP_LOGW(TAG, "HOMEY_DATA_UI phase=dashboard_show blocked=readiness_changed");
        return false;
    }
    if (s_startup_logo != NULL) lv_obj_set_hidden(s_startup_logo, true);
    const panel_ui_connection_state_t connection_state =
        !phone_provisioning_wifi_online()
            ? PANEL_UI_CONNECTION_OFFLINE
            : (presentation_ready ? PANEL_UI_CONNECTION_CONNECTED
                                  : PANEL_UI_CONNECTION_CONNECTING);
    panel_ui_connection_info_t connection = {
        .state = connection_state,
        .display_name = "",
    };
    (void)panel_ui_set_homey_data_ready(s_panel_ui, presentation_ready);
    (void)panel_homey_favorites_apply_ui_model(&s_panel_model);
    bool ok = panel_ui_activate(s_panel_ui);
    (void)panel_ui_set_connection(s_panel_ui, &connection);
    (void)panel_ui_refresh(s_panel_ui);
    bsp_display_unlock();
    if (ok) {
        s_panel_dashboard_visible = true;
        startup_status_active_set(false);
        s_dashboard_connection_state = connection_state;
        s_dashboard_homey_data_ready = presentation_ready;
    }
    return ok;
}

static void update_homey_data_boot_view(uint64_t now_ms)
{
    if (s_panel_dashboard_visible) return;
    const bool wifi_online = wifi_state_snapshot() == SECURE_BOOTSTRAP_WIFI_ONLINE;
    const athom_homey_data_state_t state = athom_oauth_runtime_homey_data_state();
    const bool snapshot_fresh = s_homey_dashboard_state.generation_valid &&
        !s_homey_dashboard_state.stale;
    const bool verified_ready = wifi_online &&
        state == ATHOM_HOMEY_DATA_READY && snapshot_fresh;
    panel_startup_wait_update(
        &s_homey_data_wait, wifi_online, verified_ready, now_ms);
    const bool long_wait = panel_startup_wait_expired(
        &s_homey_data_wait,
        state == ATHOM_HOMEY_DATA_ERROR,
        now_ms);
    const panel_startup_homey_state_t homey_state =
        verified_ready ? PANEL_STARTUP_HOMEY_READY :
        state == ATHOM_HOMEY_DATA_ERROR ? PANEL_STARTUP_HOMEY_ERROR :
        PANEL_STARTUP_HOMEY_LOADING;
    const panel_startup_action_t action = panel_startup_status_tick(
        &s_startup_status, wifi_online, homey_state, now_ms);

    switch (action) {
    case PANEL_STARTUP_ACTION_WIFI_CONNECTED:
        if (set_startup_display_text("Wi-Fi ansluten", "", false)) {
            s_homey_data_long_wait_rendered = false;
            ESP_LOGI(TAG, "STARTUP_UI status=wifi_connected minimum_ms=2000");
        } else {
            panel_startup_status_render_failed(&s_startup_status, action);
        }
        break;
    case PANEL_STARTUP_ACTION_WIFI_DISCONNECTED:
        s_homey_data_long_wait_rendered = false;
        if (!set_startup_display_text(
                "Ansluter till Wi-Fi...", "Återansluter till sparat nätverk", false)) {
            panel_startup_status_render_failed(&s_startup_status, action);
        }
        ESP_LOGI(TAG, "STARTUP_UI status=wifi_disconnected immediate=true");
        break;
    case PANEL_STARTUP_ACTION_HOMEY_LOADING:
    {
        bool rendered = false;
        if (long_wait) {
            rendered = set_startup_display_text(
                "Hämtningen tar längre tid än väntat", "Försöker igen...", false);
            if (rendered) s_homey_data_long_wait_rendered = true;
        } else {
            rendered = set_startup_display_text(
                "Hämtar Homey-data...", "Väntar på verifierad live-data", false);
            if (rendered) s_homey_data_long_wait_rendered = false;
        }
        if (!rendered) panel_startup_status_render_failed(&s_startup_status, action);
        ESP_LOGI(TAG, "STARTUP_UI status=homey_loading");
        break;
    }
    case PANEL_STARTUP_ACTION_HOMEY_READY:
        if (set_startup_display_text("Homey READY", "", false)) {
            ESP_LOGI(TAG, "STARTUP_UI status=homey_ready minimum_ms=2000");
        } else {
            panel_startup_status_render_failed(&s_startup_status, action);
        }
        break;
    case PANEL_STARTUP_ACTION_HOMEY_ERROR:
        if (!set_startup_display_text(
            "Homey-data kunde inte hämtas",
            "Kontrollera anslutningen eller Homey-inloggningen",
            true)) {
            panel_startup_status_render_failed(&s_startup_status, action);
        }
        s_homey_data_long_wait_rendered = false;
        ESP_LOGW(TAG, "STARTUP_UI status=homey_error immediate=true");
        break;
    case PANEL_STARTUP_ACTION_SHOW_DASHBOARD:
        ESP_LOGI(TAG, "HOMEY_DATA_UI phase=dashboard_show verified=true");
        if (!panel_show_dashboard()) {
            panel_startup_status_render_failed(
                &s_startup_status, PANEL_STARTUP_ACTION_SHOW_DASHBOARD);
        }
        break;
    case PANEL_STARTUP_ACTION_NONE:
    default:
        if (wifi_online && !verified_ready &&
            state != ATHOM_HOMEY_DATA_ERROR && long_wait &&
            !s_homey_data_long_wait_rendered &&
            s_startup_status.wifi_status_shown &&
            now_ms >= s_startup_status.wifi_status_shown_at_ms &&
            now_ms - s_startup_status.wifi_status_shown_at_ms >=
                PANEL_STARTUP_STATUS_MIN_MS) {
            if (set_startup_display_text(
                    "Hämtningen tar längre tid än väntat", "Försöker igen...", false)) {
                s_homey_data_long_wait_rendered = true;
            }
        }
        break;
    }
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
    uint32_t actions = wifi_transition_serialized(
        SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE);
    ESP_LOGI(TAG, "WIFI_RECONFIGURE transition_actions=0x%08lx", (unsigned long)actions);
    if ((actions & SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING) == 0U) {
        ESP_LOGW(TAG, "WIFI_RECONFIGURE open_provisioning=false actions=0x%08lx", (unsigned long)actions);
        reconfigure_pending_set(false);
        if (wifi_state_snapshot() == SECURE_BOOTSTRAP_WIFI_ONLINE) {
            set_reconfigure_button_visible(true);
        }
        panel_wifi_result_update(PANEL_UI_WIFI_RECONFIGURE_BLOCKED);
        vTaskDelete(NULL);
        return;
    }

    esp_err_t result = apply_actions(actions);
    ESP_LOGI(TAG, "WIFI_RECONFIGURE apply_result=%s", esp_err_to_name(result));
    panel_wifi_result_update(result == ESP_OK
        ? PANEL_UI_WIFI_RECONFIGURE_OPENED
        : PANEL_UI_WIFI_RECONFIGURE_FAILED);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "WIFI_RECONFIGURE open_result=%s", esp_err_to_name(result));
        uint32_t recovery = wifi_transition_serialized(
            SECURE_BOOTSTRAP_WIFI_EVENT_PROVISIONING_OPEN_FAILED);
        (void)apply_actions(recovery);
    } else {
        ESP_LOGI(TAG, "WIFI_RECONFIGURE requested=true");
    }
    vTaskDelete(NULL);
}

static void touch_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED ||
        reconfigure_pending_get() ||
        wifi_state_snapshot() != SECURE_BOOTSTRAP_WIFI_ONLINE) {
        return;
    }

    if (!reconfigure_pending_set_if_clear()) return;
    startup_status_active_set(false);
    lv_obj_add_state(s_touch_button, LV_STATE_DISABLED);
    lv_obj_add_flag(s_touch_button, LV_OBJ_FLAG_HIDDEN);

    if (xTaskCreate(
            reconfigure_task,
            "wifi_reconfigure",
            4096,
            NULL,
            5,
            NULL) != pdPASS) {
        reconfigure_pending_set(false);
        lv_obj_remove_state(s_touch_button, LV_STATE_DISABLED);
        lv_obj_remove_flag(s_touch_button, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGE(TAG, "WIFI_RECONFIGURE task_create=false");
    }
}

static esp_err_t connect_current(void) { esp_err_t err = esp_wifi_connect(); if (err != ESP_OK) ESP_LOGW(TAG, "Wi-Fi connect scheduling failed: %s", esp_err_to_name(err)); return err; }

/* Driver events carry no provisioning generation. Establish a real disconnect,
 * then post a sentinel onto the same default event loop. FIFO dispatch drains
 * events already queued for A before B is armed. */
static bool candidate_disconnect_and_drain_events(uint32_t generation)
{
    if (generation == 0U || s_wifi_fence_sem == NULL) return false;
    esp_err_t err = esp_wifi_disconnect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_CONNECT) return false;

    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(WIFI_DISCONNECT_FENCE_TIMEOUT_MS);
    bool disconnected = false;
    while ((xTaskGetTickCount() - start) < timeout) {
        wifi_ap_record_t ap = {0};
        err = esp_wifi_sta_get_ap_info(&ap);
        secure_zero(&ap, sizeof(ap));
        if (err == ESP_ERR_WIFI_NOT_CONNECT) {
            disconnected = true;
            break;
        }
        if (err != ESP_OK) return false;
        vTaskDelay(pdMS_TO_TICKS(WIFI_DISCONNECT_POLL_MS));
    }
    if (!disconnected) return false;

    /* Stop the old DHCP transaction synchronously in the lwIP task. This
     * prevents a delayed lease callback from posting a fresh GOT_IP after the
     * event-loop fence below. esp_netif_down leaves DHCP in INIT, so the
     * default STA-connected action can start DHCP for the new association. */
    if (s_sta_netif == NULL) return false;
    esp_netif_action_disconnected(
        s_sta_netif, WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, NULL);
    esp_netif_ip_info_t cleared_ip = {0};
    if (esp_netif_get_ip_info(s_sta_netif, &cleared_ip) != ESP_OK ||
        cleared_ip.ip.addr != 0U) return false;
    s_ip_obtained = secure_bootstrap_sta_link_apply_event(
        s_ip_obtained, SECURE_BOOTSTRAP_STA_LINK_EVENT_DISCONNECTED);
    phone_provisioning_on_wifi_offline();
    cancel_sntp_start_request();

    while (xSemaphoreTake(s_wifi_fence_sem, 0) == pdTRUE) { }
    portENTER_CRITICAL(&s_wifi_session_mux);
    s_wifi_fence_wait_generation = generation;
    s_wifi_fence_done_generation = 0U;
    portEXIT_CRITICAL(&s_wifi_session_mux);
    err = esp_event_post(SECURE_BOOTSTRAP_WIFI_FENCE_EVENT,
        WIFI_FENCE_EVENT_DRAINED, &generation, sizeof(generation),
        pdMS_TO_TICKS(WIFI_EVENT_DRAIN_TIMEOUT_MS));
    if (err != ESP_OK ||
        xSemaphoreTake(s_wifi_fence_sem,
            pdMS_TO_TICKS(WIFI_EVENT_DRAIN_TIMEOUT_MS)) != pdTRUE) {
        portENTER_CRITICAL(&s_wifi_session_mux);
        if (s_wifi_fence_wait_generation == generation)
            s_wifi_fence_wait_generation = 0U;
        portEXIT_CRITICAL(&s_wifi_session_mux);
        return false;
    }
    portENTER_CRITICAL(&s_wifi_session_mux);
    const bool drained = s_wifi_fence_done_generation == generation &&
        s_wifi_fence_wait_generation == generation;
    if (s_wifi_fence_wait_generation == generation)
        s_wifi_fence_wait_generation = 0U;
    portEXIT_CRITICAL(&s_wifi_session_mux);
    return drained;
}

static bool wifi_got_ip_matches_active_sta(const ip_event_got_ip_t *event)
{
    if (event == NULL || s_sta_netif == NULL || event->esp_netif != s_sta_netif ||
        event->ip_info.ip.addr == 0U) return false;
    esp_netif_ip_info_t current_ip = {0};
    if (esp_netif_get_ip_info(s_sta_netif, &current_ip) != ESP_OK ||
        current_ip.ip.addr == 0U || current_ip.ip.addr != event->ip_info.ip.addr ||
        current_ip.netmask.addr != event->ip_info.netmask.addr ||
        current_ip.gw.addr != event->ip_info.gw.addr) return false;
    wifi_ap_record_t ap = {0};
    const esp_err_t ap_result = esp_wifi_sta_get_ap_info(&ap);
    secure_zero(&ap, sizeof(ap));
    return ap_result == ESP_OK;
}

static bool wifi_got_ip_matches_candidate(const ip_event_got_ip_t *event)
{
    wifi_config_t candidate = {0};
    wifi_config_t active = {0};
    bool candidate_valid = false;
    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    candidate_valid = s_candidate_valid;
    if (candidate_valid) candidate = s_candidate_config;
    (void)xSemaphoreGive(s_ap_config_mutex);
    if (!candidate_valid ||
        esp_wifi_get_config(WIFI_IF_STA, &active) != ESP_OK ||
        memcmp(candidate.sta.password, active.sta.password, sizeof(candidate.sta.password)) != 0 ||
        !wifi_got_ip_matches_active_sta(event)) {
        secure_zero(&candidate, sizeof(candidate));
        secure_zero(&active, sizeof(active));
        return false;
    }
    wifi_ap_record_t ap = {0};
    const esp_err_t ap_result = esp_wifi_sta_get_ap_info(&ap);
    const bool ap_matches = ap_result == ESP_OK &&
        secure_bootstrap_wifi_ssid_association_matches(
            ap.ssid, active.sta.ssid, candidate.sta.ssid);
    secure_zero(&ap, sizeof(ap));
    secure_zero(&candidate, sizeof(candidate));
    secure_zero(&active, sizeof(active));
    return ap_matches;
}

static bool wifi_got_ip_matches_saved(const ip_event_got_ip_t *event)
{
    wifi_config_t saved = {0};
    wifi_config_t active = {0};
    bool saved_valid = false;
    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    saved_valid = s_saved_valid && has_saved_ssid(&s_saved_config);
    if (saved_valid) saved = s_saved_config;
    (void)xSemaphoreGive(s_ap_config_mutex);
    const bool matches = saved_valid &&
        esp_wifi_get_config(WIFI_IF_STA, &active) == ESP_OK &&
        memcmp(saved.sta.ssid, active.sta.ssid, sizeof(saved.sta.ssid)) == 0 &&
        memcmp(saved.sta.password, active.sta.password, sizeof(saved.sta.password)) == 0 &&
        wifi_got_ip_matches_active_sta(event);
    secure_zero(&saved, sizeof(saved));
    secure_zero(&active, sizeof(active));
    return matches;
}

static esp_err_t server_start(void);
static esp_err_t ensure_homey_server(void);
static esp_err_t open_provisioning(void);
static esp_err_t restore_saved_ram(void);

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
    portENTER_CRITICAL(&s_wifi_session_mux);
    s_persist_stage = WIFI_PERSIST_STAGE_ROLLBACK;
    portEXIT_CRITICAL(&s_wifi_session_mux);
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
    portENTER_CRITICAL(&s_wifi_session_mux);
    const bool owns_commit = secure_bootstrap_wifi_session_commit_started(
        &s_wifi_session, s_persist_session_generation);
    if (owns_commit) {
        s_persist_active = true;
        s_persist_stage = WIFI_PERSIST_STAGE_WRITING;
    } else {
        s_persist_task_scheduled = false;
    }
    portEXIT_CRITICAL(&s_wifi_session_mux);
    if (!owns_commit) {
        ESP_LOGE(TAG, "WIFI_PERSIST start_rejected=true reason=session_generation");
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "WIFI_PERSIST begin");
    if (s_server != NULL) { (void)httpd_stop(s_server); s_server = NULL; }
    esp_err_t err = backup_write_saved();
    ESP_LOGI(TAG, "WIFI_PERSIST backup_result=%s", esp_err_to_name(err));
    wifi_config_t readback = {0};
    if (err == ESP_OK) err = persist_write_and_reopen(&s_candidate_config, &readback);
    if (err == ESP_OK) {
        s_saved_config = readback;
        s_saved_valid = true;
        uint32_t actions = wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_OK);
        (void)actions;
        portENTER_CRITICAL(&s_wifi_session_mux);
        s_persist_stage = WIFI_PERSIST_STAGE_VERIFY_CONNECT;
        portEXIT_CRITICAL(&s_wifi_session_mux);
        err = esp_wifi_start();
        ESP_LOGI(TAG, "WIFI_PERSIST start_result=%s", esp_err_to_name(err));
        if (err == ESP_OK) err = esp_wifi_connect();
        ESP_LOGI(TAG, "WIFI_PERSIST connect_result=%s", esp_err_to_name(err));
        if (err == ESP_OK) { secure_zero(&readback, sizeof(readback)); vTaskDelete(NULL); return; }
    }
    persist_fail("commit", err);
    (void)wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_COMMIT_FAILED);
    esp_err_t rollback_err = rollback_once();
    if (rollback_err == ESP_OK) {
        (void)wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_OK);
    } else {
        (void)wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED);
    }
    portENTER_CRITICAL(&s_wifi_session_mux);
    s_persist_active = false;
    (void)secure_bootstrap_wifi_session_finish_commit(
        &s_wifi_session, s_persist_session_generation);
    portEXIT_CRITICAL(&s_wifi_session_mux);
    if (rollback_err != ESP_OK && esp_wifi_start() == ESP_OK) (void)open_provisioning();
    secure_zero(&readback, sizeof(readback));
    vTaskDelete(NULL);
}

static esp_err_t schedule_persistent_commit(void)
{
    uint32_t generation = 0U;
    secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    bool reserved = false;
    bool already_scheduled = false;
    bool rejected_busy = false;
    portENTER_CRITICAL(&s_wifi_session_mux);
    reserved = secure_bootstrap_wifi_session_current(
        &s_wifi_session, &generation, &phase) &&
        phase == SECURE_BOOTSTRAP_SESSION_COMMIT_RESERVED;
    if (reserved && (s_persist_task_scheduled || s_persist_active)) {
        already_scheduled = s_persist_task_scheduled &&
            s_persist_session_generation == generation;
        if (!already_scheduled) {
            (void)secure_bootstrap_wifi_session_commit_start_failed(
                &s_wifi_session, generation);
            rejected_busy = true;
        }
    } else if (reserved) {
        s_persist_task_scheduled = true;
        s_persist_session_generation = generation;
    }
    portEXIT_CRITICAL(&s_wifi_session_mux);
    if (!reserved) return ESP_ERR_INVALID_STATE;
    if (already_scheduled) return ESP_OK;
    if (rejected_busy) {
        (void)wifi_transition_serialized(
            SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_TASK_CREATE_FAILED);
        if (s_panel_ui != NULL && bsp_display_lock(1000)) {
            (void)panel_ui_set_wifi_cancel_pending(s_panel_ui, false, false);
            bsp_display_unlock();
        }
        return ESP_ERR_INVALID_STATE;
    }
    if (xTaskCreate(persistent_commit_task, "wifi_persist", 6144, NULL, 5, NULL) != pdPASS) {
        portENTER_CRITICAL(&s_wifi_session_mux);
        const bool still_reserved = s_persist_task_scheduled &&
            s_persist_session_generation == generation &&
            secure_bootstrap_wifi_session_commit_start_failed(&s_wifi_session, generation);
        if (still_reserved) s_persist_task_scheduled = false;
        portEXIT_CRITICAL(&s_wifi_session_mux);
        if (still_reserved) {
            (void)wifi_transition_serialized(
                SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_TASK_CREATE_FAILED);
            if (s_panel_ui != NULL && bsp_display_lock(1000)) {
                (void)panel_ui_set_wifi_cancel_pending(s_panel_ui, false, false);
                bsp_display_unlock();
            }
        }
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

static bool networks_snapshot_copy(
    secure_bootstrap_network_t *out, size_t capacity, size_t *out_count)
{
    if (out == NULL || out_count == NULL || capacity < SECURE_BOOTSTRAP_MAX_NETWORKS ||
        s_networks_mutex == NULL) return false;
    uint32_t current_generation = 0U;
    secure_bootstrap_session_phase_t current_phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    if (!wifi_session_snapshot(&current_generation, &current_phase) ||
        (current_phase != SECURE_BOOTSTRAP_SESSION_ACTIVE &&
         current_phase != SECURE_BOOTSTRAP_SESSION_CANDIDATE)) return false;
    if (xSemaphoreTake(s_networks_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    (void)secure_bootstrap_network_snapshot_copy(
        &s_network_snapshot, current_generation, out, capacity, out_count);
    (void)xSemaphoreGive(s_networks_mutex);
    return true;
}

static void networks_snapshot_clear(void)
{
    if (s_networks_mutex == NULL ||
        xSemaphoreTake(s_networks_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return;
    secure_zero(&s_network_snapshot, sizeof(s_network_snapshot));
    (void)xSemaphoreGive(s_networks_mutex);
}

static esp_err_t open_provisioning(void)
{
    ESP_LOGI(TAG, "WIFI_TRACE portal_begin");
    uint32_t session_generation;
    portENTER_CRITICAL(&s_wifi_session_mux);
    if (s_wifi_session.phase == SECURE_BOOTSTRAP_SESSION_CANDIDATE &&
        s_wifi.state == SECURE_BOOTSTRAP_WIFI_PROVISIONING) {
        /* Candidate retries exhausted: invalidate old asynchronous work before
         * creating a replacement provisioning generation. */
        s_wifi_session.generation++;
        if (s_wifi_session.generation == 0U) s_wifi_session.generation++;
        s_wifi_session.phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
        s_wifi_session.cancel_from = SECURE_BOOTSTRAP_SESSION_CLOSED;
    }
    session_generation = secure_bootstrap_wifi_session_begin(&s_wifi_session);
    portEXIT_CRITICAL(&s_wifi_session_mux);
    if (session_generation == 0U) return ESP_ERR_INVALID_STATE;

    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
        portENTER_CRITICAL(&s_wifi_session_mux);
        (void)secure_bootstrap_wifi_session_fail_open(&s_wifi_session, session_generation);
        portEXIT_CRITICAL(&s_wifi_session_mux);
        return ESP_ERR_TIMEOUT;
    }
    if (s_code.code[0] == '\0' || s_code.consumed) {
        if (generate_code() != ESP_OK) {
            (void)xSemaphoreGive(s_ap_config_mutex);
            portENTER_CRITICAL(&s_wifi_session_mux);
            (void)secure_bootstrap_wifi_session_fail_open(&s_wifi_session, session_generation);
            portEXIT_CRITICAL(&s_wifi_session_mux);
            return ESP_FAIL;
        }
    }

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err == ESP_OK) err = configure_access_point(s_code.code);
    if (err == ESP_OK) err = ensure_homey_server();
    if (err != ESP_OK) {
        (void)xSemaphoreGive(s_ap_config_mutex);
        portENTER_CRITICAL(&s_wifi_session_mux);
        (void)secure_bootstrap_wifi_session_fail_open(&s_wifi_session, session_generation);
        portEXIT_CRITICAL(&s_wifi_session_mux);
        ESP_LOGE(TAG, "WIFI_PROVISIONING open_result=%s", esp_err_to_name(err));
        return err;
    }
    portENTER_CRITICAL(&s_wifi_session_mux);
    bool activated = secure_bootstrap_wifi_session_activate(&s_wifi_session, session_generation);
    portEXIT_CRITICAL(&s_wifi_session_mux);
    if (!activated) {
        (void)xSemaphoreGive(s_ap_config_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    /* Do not publish QR/password until the AP configuration and portal are
     * both active. The underlying bootstrap QR and modal use this same code. */
    set_display_text(
        "Säker Wi-Fi-installation",
        "Anslut till HomeyPanel-Setup\nÖppna 192.168.4.1",
        s_code.code);
    panel_wifi_present_session_locked();
    (void)xSemaphoreGive(s_ap_config_mutex);
    (void)start_network_scan();
    size_t network_count = 0U;
    secure_bootstrap_network_t network_snapshot[SECURE_BOOTSTRAP_MAX_NETWORKS];
    if (!networks_snapshot_copy(network_snapshot, SECURE_BOOTSTRAP_MAX_NETWORKS, &network_count)) {
        network_count = 0U;
    }
    ESP_LOGI(
        TAG,
        "WIFI_PROVISIONING_OPEN networks=%u sensitive_values_logged=false",
        (unsigned)network_count);
    secure_zero(network_snapshot, sizeof(network_snapshot));
    ESP_LOGI(TAG, "WIFI_TRACE provisioning_ready");
    return ESP_OK;
}

static void close_provisioning(void)
{
    if (s_server) { httpd_stop(s_server); s_server = NULL; }
    s_homey_handlers_registered = false;
    (void)esp_wifi_set_mode(WIFI_MODE_STA);
    if (s_panel_ui != NULL && bsp_display_lock(1000)) {
        (void)panel_ui_clear_wifi_provisioning(s_panel_ui, s_qr);
        bsp_display_unlock();
    }
    secure_bootstrap_code_wipe(&s_code);
}

static void wifi_cancel_task(void *arg)
{
    const uint32_t generation = (uint32_t)(uintptr_t)arg;
    bool portal_stop_failed = false;
    if (s_networks_mutex != NULL &&
        xSemaphoreTake(s_networks_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        secure_bootstrap_scan_cancel(&s_scan_state);
        (void)xSemaphoreGive(s_networks_mutex);
    }
    (void)esp_wifi_scan_stop();
    /* httpd_stop waits for any current /wifi handler. The session reservation
     * already fences its later events, and no state lock is held here. */
    if (s_server != NULL) {
        if (httpd_stop(s_server) != ESP_OK) {
            portal_stop_failed = true;
            goto cancel_failed;
        }
        s_server = NULL;
    }
    s_homey_handlers_registered = false;
    (void)esp_wifi_disconnect();

    const bool saved_config_present = s_saved_valid && has_saved_ssid(&s_saved_config);
    if (saved_config_present) {
        portENTER_CRITICAL(&s_wifi_session_mux);
        const bool owns_cancel = s_wifi_session.phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED &&
            s_wifi_session.generation == generation;
        const uint32_t actions = owns_cancel
            ? secure_bootstrap_wifi_transition(&s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_USER_CANCEL)
            : SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
        portEXIT_CRITICAL(&s_wifi_session_mux);
        if (!owns_cancel || actions == SECURE_BOOTSTRAP_WIFI_ACTION_NONE) goto cancel_failed;
        if (s_ap_config_mutex == NULL ||
            xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(3000)) != pdTRUE) goto cancel_failed;
        if ((actions & SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED) == 0U ||
            restore_saved_ram() != ESP_OK) {
            (void)xSemaphoreGive(s_ap_config_mutex);
            goto cancel_failed;
        }
        s_candidate_valid = false;
        secure_zero(&s_candidate_config, sizeof(s_candidate_config));
        networks_snapshot_clear();
        close_provisioning();
        portENTER_CRITICAL(&s_wifi_session_mux);
        (void)secure_bootstrap_wifi_session_finish_cancel(
            &s_wifi_session, generation, false);
        s_cancel_task_scheduled = false;
        portEXIT_CRITICAL(&s_wifi_session_mux);
        (void)xSemaphoreGive(s_ap_config_mutex);
        const esp_err_t connect_result = connect_current();
        if (connect_result != ESP_OK) {
            ESP_LOGW(TAG, "WIFI_PROVISIONING_CANCEL saved_connect_scheduled=false retry_policy=existing");
        }
        ESP_LOGI(TAG, "WIFI_PROVISIONING_CANCEL result=restored_saved_sta credentials_preserved=true");
        vTaskDelete(NULL);
        return;
    }

    /* First-install cancellation retains a usable portal; it never writes the
     * candidate to flash and rotates the one-time AP credential. */
    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(3000)) != pdTRUE) goto cancel_failed;
    if (generate_code() != ESP_OK ||
        esp_wifi_set_mode(WIFI_MODE_APSTA) != ESP_OK ||
        configure_access_point(s_code.code) != ESP_OK ||
        ensure_homey_server() != ESP_OK) {
        (void)xSemaphoreGive(s_ap_config_mutex);
        goto cancel_failed;
    }
    portENTER_CRITICAL(&s_wifi_session_mux);
    const bool owns_cancel = s_wifi_session.phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED &&
        s_wifi_session.generation == generation;
    const bool kept = owns_cancel && secure_bootstrap_wifi_cancel_first_install(
        &s_wifi, &s_wifi_session, generation);
    s_cancel_task_scheduled = false;
    portEXIT_CRITICAL(&s_wifi_session_mux);
    if (!kept) {
        (void)xSemaphoreGive(s_ap_config_mutex);
        goto cancel_failed;
    }
    s_candidate_valid = false;
    secure_zero(&s_candidate_config, sizeof(s_candidate_config));
    networks_snapshot_clear();
    set_display_text(
        "Säker Wi-Fi-installation",
        "Anslut till HomeyPanel-Setup\nÖppna 192.168.4.1",
        s_code.code);
    panel_wifi_present_session_locked();
    (void)xSemaphoreGive(s_ap_config_mutex);
    (void)start_network_scan();
    ESP_LOGI(TAG, "WIFI_PROVISIONING_CANCEL result=portal_retained credentials_preserved=true");
    vTaskDelete(NULL);
    return;

cancel_failed:
    ;
    if (portal_stop_failed) {
        /* httpd_stop failed before any disconnect or persistence side effect.
         * Keep the already-running first-install portal usable, but roll to a
         * fresh generation so every request that began before Cancel is stale. */
        portENTER_CRITICAL(&s_wifi_session_mux);
        const bool owns_failed_stop =
            s_wifi_session.phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED &&
            s_wifi_session.generation == generation;
        bool resumed = false;
        if (owns_failed_stop) {
            s_wifi.state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            s_wifi.candidate_active = false;
            s_wifi.retry_count = 0U;
            resumed = secure_bootstrap_wifi_session_finish_cancel(
                &s_wifi_session, generation, true);
        }
        s_cancel_task_scheduled = false;
        portEXIT_CRITICAL(&s_wifi_session_mux);
        if (resumed) {
            s_candidate_valid = false;
            secure_zero(&s_candidate_config, sizeof(s_candidate_config));
            reconfigure_pending_set(false);
            esp_err_t restore_err = ESP_OK;
            if (s_saved_valid && has_saved_ssid(&s_saved_config)) {
                (void)esp_wifi_disconnect();
                restore_err = restore_saved_ram();
                if (restore_err == ESP_OK) restore_err = connect_current();
            }
            ESP_LOGW(TAG, "WIFI_PROVISIONING_CANCEL result=portal_kept_after_stop_error credentials_preserved=true sta_restore=%s",
                     esp_err_to_name(restore_err));
            panel_wifi_result_update(PANEL_UI_WIFI_CANCEL_FAILED);
            vTaskDelete(NULL);
            return;
        }
    }
    /* Restore a usable portal before reopening the session. This path never
     * writes the candidate credentials to persistent storage. */
    bool recovered = !portal_stop_failed;
    bool recovery_lock_held = false;
    if (recovered && s_server == NULL) {
        recovery_lock_held = s_ap_config_mutex != NULL &&
            xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(3000)) == pdTRUE;
        recovered = recovery_lock_held;
        if (recovered) recovered = generate_code() == ESP_OK;
        if (recovered && esp_wifi_set_mode(WIFI_MODE_APSTA) != ESP_OK) recovered = false;
        if (recovered && configure_access_point(s_code.code) != ESP_OK) recovered = false;
        if (recovered && ensure_homey_server() != ESP_OK) recovered = false;
        if (recovery_lock_held) (void)xSemaphoreGive(s_ap_config_mutex);
    } else if (recovered) {
        /* A non-null server here means it was not safely stopped. Do not
         * claim recovery or reactivate a session around a possibly-live
         * handler. */
        recovered = false;
    }
    portENTER_CRITICAL(&s_wifi_session_mux);
    if (s_wifi_session.phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED &&
        s_wifi_session.generation == generation) {
        if (recovered) {
            s_wifi.state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;
            s_wifi.candidate_active = false;
            s_wifi.retry_count = 0U;
            (void)secure_bootstrap_wifi_session_finish_cancel(
                &s_wifi_session, generation, true);
        }
    }
    s_cancel_task_scheduled = false;
    portEXIT_CRITICAL(&s_wifi_session_mux);
    if (recovered) {
        s_candidate_valid = false;
        secure_zero(&s_candidate_config, sizeof(s_candidate_config));
        set_display_text(
            "Säker Wi-Fi-installation",
            "Anslut till HomeyPanel-Setup\nÖppna 192.168.4.1",
            s_code.code);
        ESP_LOGW(TAG, "WIFI_PROVISIONING_CANCEL result=portal_recovered candidate_not_committed=true");
        panel_wifi_present_session();
    } else {
        if (s_panel_ui != NULL && bsp_display_lock(1000)) {
            (void)panel_ui_clear_wifi_provisioning(s_panel_ui, s_qr);
            (void)panel_ui_set_wifi_reconfigure_result(
                s_panel_ui, PANEL_UI_WIFI_CANCEL_FAILED);
            bsp_display_unlock();
        }
        ESP_LOGE(TAG, "WIFI_PROVISIONING_CANCEL result=failed session_remains_reserved=true");
    }
    vTaskDelete(NULL);
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
        reconfigure_pending_set(false);
        phone_provisioning_on_wifi_online();
        set_reconfigure_button_visible(true);
        update_homey_data_boot_view((uint64_t)(esp_timer_get_time() / 1000LL));
    }
    return ESP_OK;
}


static esp_err_t display_init(void)
{
    ESP_RETURN_ON_ERROR(runtime_diag_journal_pause_flash_writes(), TAG,
                        "pause diagnostic journal flash writes before RGB startup");
    panel_render_diag_start();
    lv_display_t *display = bsp_display_start();
    if (display == NULL) {
        return ESP_FAIL;
    }

    ESP_RETURN_ON_ERROR(bsp_display_brightness_set(80), TAG, "display brightness");

    memset(&s_panel_perf, 0, sizeof(s_panel_perf));
    s_panel_perf.window_start_us = (uint64_t)esp_timer_get_time();
    lv_display_add_event_cb(display, panel_display_perf_event, LV_EVENT_REFR_START, NULL);
    lv_display_add_event_cb(display, panel_display_perf_event, LV_EVENT_REFR_READY, NULL);
    lv_display_add_event_cb(display, panel_display_perf_event, LV_EVENT_FLUSH_START, NULL);
    lv_display_add_event_cb(display, panel_display_perf_event, LV_EVENT_FLUSH_FINISH, NULL);

    lv_indev_t *input_device = bsp_display_get_input_dev();
    if (input_device != NULL) {
        lv_indev_set_scroll_limit(input_device, PATCH022_SCROLL_LIMIT);
        lv_indev_set_scroll_throw(input_device, PATCH022_SCROLL_THROW);
        ESP_LOGI(TAG, "PANEL_UI indev_scroll_limit=%u indev_scroll_throw=%u",
                 PATCH022_SCROLL_LIMIT, PATCH022_SCROLL_THROW);
    } else {
        ESP_LOGW(TAG, "PANEL_UI input_device_available=false");
    }

    if (!bsp_display_lock(0)) {
        return ESP_FAIL;
    }

    lv_obj_t *screen = lv_screen_active();
    s_provisioning_screen = screen;
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x071018), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(screen, LV_OPA_COVER, 0);

    s_startup_logo = lv_image_create(screen);
    lv_image_set_src(s_startup_logo, &panel_startup_logo_image);
    lv_obj_set_style_image_recolor(s_startup_logo, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(s_startup_logo, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_align(s_startup_logo, LV_ALIGN_CENTER, 0, STARTUP_LOGO_Y_OFFSET);

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
    lv_obj_set_size(s_touch_button, 190, TOUCH_HEIGHT);
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
    s_panel_model.settings = s_panel_settings_boot;
    panel_homey_dashboard_state_init(&s_homey_dashboard_state);
    s_homey_dashboard_last_poll_ms = 0U;
    panel_ui_config_t panel_config = {
        .model = &s_panel_model,
        .callbacks = {
            .context = s_panel_ui,
            .request_brightness = panel_brightness_request,
            .request_wifi_reconfigure = panel_wifi_request,
            .request_wifi_cancel = panel_wifi_cancel_request,
            .request_choose_homey = panel_choose_request,
            .request_homey_wipe = panel_wipe_request,
            .request_change_athom_account = panel_account_request,
            .request_light_toggle = panel_light_toggle_request,
            .request_roller_step = panel_roller_step_request,
            .settings_changed = panel_settings_request,
            .interaction_trace = panel_interaction_trace,
            .read_information = panel_read_information,
        },
    };
    ESP_LOGI(TAG, "PANEL_UI callbacks wifi=%s choose=%s wipe=%s account=%s light=%s awning=%s trace=%s",
        panel_config.callbacks.request_wifi_reconfigure != NULL ? "true" : "false",
        panel_config.callbacks.request_choose_homey != NULL ? "true" : "false",
        panel_config.callbacks.request_homey_wipe != NULL ? "true" : "false",
        panel_config.callbacks.request_change_athom_account != NULL ? "true" : "false",
        panel_config.callbacks.request_light_toggle != NULL ? "true" : "false",
        panel_config.callbacks.request_roller_step != NULL ? "true" : "false",
        panel_config.callbacks.interaction_trace != NULL ? "true" : "false");
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
    if (s_networks_mutex == NULL) return ESP_ERR_INVALID_STATE;
    uint32_t session_generation = 0U;
    secure_bootstrap_session_phase_t session_phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    if (!wifi_session_snapshot(&session_generation, &session_phase) ||
        (session_phase != SECURE_BOOTSTRAP_SESSION_ACTIVE &&
         session_phase != SECURE_BOOTSTRAP_SESSION_CANDIDATE)) return ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(s_networks_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return ESP_ERR_TIMEOUT;
    if (s_scan_state.in_progress) {
        (void)xSemaphoreGive(s_networks_mutex);
        return ESP_OK;
    }
    if (!secure_bootstrap_scan_begin(&s_scan_state, session_generation)) {
        (void)xSemaphoreGive(s_networks_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    (void)xSemaphoreGive(s_networks_mutex);

    wifi_scan_config_t config = {
        .show_hidden = false,
    };
    ESP_LOGI(TAG, "WIFI_TRACE scan_start_begin");
    esp_err_t err = esp_wifi_scan_start(&config, false);
    ESP_LOGI(TAG, "WIFI_TRACE scan_start_end result=%s", esp_err_to_name(err));
    if (err != ESP_OK) {
        if (xSemaphoreTake(s_networks_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            (void)secure_bootstrap_scan_complete(
                &s_scan_state, session_generation, 0U, false);
            (void)xSemaphoreGive(s_networks_mutex);
        }
        ESP_LOGW(TAG, "Wi-Fi scan start failed: %s", esp_err_to_name(err));
    }
    return err;
}

static size_t consume_network_scan_results(void)
{
    if (s_networks_mutex == NULL) return 0U;
    uint32_t scan_generation = 0U;
    bool scan_was_active = false;
    bool discard = false;
    if (xSemaphoreTake(s_networks_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return 0U;
    scan_was_active = s_scan_state.in_progress;
    scan_generation = s_scan_state.session_generation;
    discard = s_scan_state.discard_result;
    (void)xSemaphoreGive(s_networks_mutex);
    if (!scan_was_active) {
        (void)esp_wifi_clear_ap_list();
        return 0U;
    }
    uint16_t total = 0U;
    esp_err_t err = esp_wifi_scan_get_ap_num(&total);
    if (err != ESP_OK) total = 0U;
    if (total > 64U) {
        total = 64U;
    }

    wifi_ap_record_t *records = total > 0U ? calloc(total, sizeof(*records)) : NULL;
    uint16_t fetched = total;
    if (total > 0U && records == NULL) {
        ESP_LOGE(TAG, "Wi-Fi scan result allocation failed");
        size_t count = 0U;
        if (xSemaphoreTake(s_networks_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            (void)secure_bootstrap_scan_complete(
                &s_scan_state, scan_generation, 0U, false);
            count = s_network_snapshot.count;
            (void)xSemaphoreGive(s_networks_mutex);
        }
        (void)esp_wifi_clear_ap_list();
        return count;
    }
    if (total > 0U) err = esp_wifi_scan_get_ap_records(&fetched, records);
    secure_bootstrap_network_t updated[SECURE_BOOTSTRAP_MAX_NETWORKS] = {0};
    size_t updated_count = 0U;
    if (err == ESP_OK) {
        for (uint16_t index = 0U; index < fetched; ++index) {
            updated_count = secure_bootstrap_network_insert(
                updated,
                updated_count,
                SECURE_BOOTSTRAP_MAX_NETWORKS,
                (const char *)records[index].ssid,
                records[index].rssi,
                map_auth(records[index].authmode));
        }
    }
    uint32_t current_generation = 0U;
    secure_bootstrap_session_phase_t current_phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    const bool current_session = wifi_session_snapshot(&current_generation, &current_phase) &&
        (current_phase == SECURE_BOOTSTRAP_SESSION_ACTIVE ||
         current_phase == SECURE_BOOTSTRAP_SESSION_CANDIDATE);
    size_t result_count = 0U;
    if (xSemaphoreTake(s_networks_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const bool publish = !discard && secure_bootstrap_scan_complete(
            &s_scan_state, scan_generation, current_generation, current_session);
        if (publish) {
            if (err == ESP_OK) {
                (void)secure_bootstrap_network_snapshot_publish(
                    &s_network_snapshot, updated, updated_count, scan_generation);
            } else {
                (void)secure_bootstrap_network_snapshot_publish(
                    &s_network_snapshot, NULL, 0U, scan_generation);
            }
        }
        result_count = s_network_snapshot.count;
        if (!publish) {
            /* Complete the single outstanding driver scan even when it was
             * cancelled/rotated, so the next session can safely start one. */
            (void)secure_bootstrap_scan_complete(
                &s_scan_state, scan_generation, current_generation, false);
        }
        (void)xSemaphoreGive(s_networks_mutex);
    }
    secure_zero(updated, sizeof(updated));
    secure_zero(records, total * sizeof(*records));
    free(records);
    ESP_LOGI(TAG, "WIFI_TRACE scan_results_consumed count=%u result=%s", (unsigned)result_count,
             esp_err_to_name(err));
    return result_count;
}

static bool find_scanned_network(const char *ssid, secure_bootstrap_network_t *out)
{
    if (ssid == NULL || ssid[0] == '\0' || out == NULL) return false;
    secure_bootstrap_network_t networks[SECURE_BOOTSTRAP_MAX_NETWORKS] = {0};
    size_t count = 0U;
    if (!networks_snapshot_copy(networks, SECURE_BOOTSTRAP_MAX_NETWORKS, &count)) return false;
    bool found = false;
    for (size_t index = 0U; index < count; ++index) {
        if (strcmp(networks[index].ssid, ssid) == 0) {
            *out = networks[index];
            found = true;
            break;
        }
    }
    secure_zero(networks, sizeof(networks));
    return found;
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
    secure_bootstrap_network_t networks[SECURE_BOOTSTRAP_MAX_NETWORKS] = {0};
    size_t network_count = 0U;
    if (!networks_snapshot_copy(networks, SECURE_BOOTSTRAP_MAX_NETWORKS, &network_count)) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t result = httpd_resp_sendstr_chunk(
        req, "<option value=\"\" selected disabled>Välj ett nätverk</option>");
    if (result != ESP_OK) goto done;

    for (size_t index = 0U; index < network_count; ++index) {
        char escaped[192];
        if (!secure_bootstrap_html_escape(networks[index].ssid, escaped, sizeof(escaped))) continue;
        char option[352];
        int written = snprintf(
            option,
            sizeof(option),
            "<option value=\"%s\" data-security=\"%s\">%s - %s signal - %s</option>",
            escaped,
            security_data_value(networks[index].security),
            escaped,
            secure_bootstrap_signal_label(networks[index].rssi),
            secure_bootstrap_security_label(networks[index].security));
        if (written > 0 && (size_t)written < sizeof(option)) {
            result = httpd_resp_sendstr_chunk(req, option);
            if (result != ESP_OK) goto done;
        }
    }

    result = httpd_resp_sendstr_chunk(
        req,
        "<option value=\"__manual__\" data-security=\"manual\">Annat nätverk...</option>");
done:
    secure_zero(networks, sizeof(networks));
    return result;
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
        char code_copy[SECURE_BOOTSTRAP_CODE_LEN + 1U] = {0};
        bool copied = s_ap_config_mutex != NULL &&
            xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(1000)) == pdTRUE;
        if (copied) {
            memcpy(code_copy, s_code.code, sizeof(code_copy));
            (void)xSemaphoreGive(s_ap_config_mutex);
        }
        if (!copied || !secure_bootstrap_html_escape(
                code_copy, escaped_code, sizeof(escaped_code))) {
            escaped_code[0] = '\0';
        }
        secure_zero(code_copy, sizeof(code_copy));
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
        "<style>body{font:22px system-ui;max-width:34rem;margin:2rem auto;padding:1rem;"
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
        "<style>body{font:22px system-ui;max-width:34rem;margin:2rem auto;padding:1rem;"
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
    uint32_t request_generation = 0U;
    secure_bootstrap_session_phase_t request_phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
    if (!wifi_session_snapshot(&request_generation, &request_phase) ||
        request_phase != SECURE_BOOTSTRAP_SESSION_ACTIVE) {
        return send_html_error(req, HTTPD_403_FORBIDDEN, "Installationen är inte aktiv");
    }
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
    secure_bootstrap_network_t scanned = {0};
    const bool scanned_found = !is_manual && find_scanned_network(selected, &scanned);
    const char *ssid = is_manual ? manual : selected;
    size_t ssid_len = strlen(ssid);
    if (ssid_len == 0U || ssid_len > 32U || (!is_manual && !scanned_found)) goto invalid;

    bool open_network = is_manual
        ? manual_open
        : scanned.security == SECURE_BOOTSTRAP_WIFI_SECURITY_OPEN;
    size_t password_len = strlen(password);
    if ((open_network && password_len != 0U) ||
        (!open_network && (password_len < 8U || password_len > 63U))) goto invalid;

    wifi_config_t candidate_config = {0};
    /* SSIDs are length-delimited byte arrays in the ESP-IDF STA config.
     * Copy the validated 1..32 bytes directly so a full 32-byte SSID is valid
     * and no terminator is written over the adjacent password field. */
    memcpy(candidate_config.sta.ssid, ssid, ssid_len);
    int password_written = snprintf(
        (char *)candidate_config.sta.password,
        sizeof(candidate_config.sta.password), "%s", password);
    if (password_written < 0 || (size_t)password_written >= sizeof(candidate_config.sta.password)) {
        secure_zero(&candidate_config, sizeof(candidate_config));
        goto invalid;
    }
    candidate_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    secure_bootstrap_status_t verified;
    uint32_t candidate_actions = SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
        secure_zero(&candidate_config, sizeof(candidate_config));
        secure_zero(code, sizeof(code));
        goto forbidden;
    }
    portENTER_CRITICAL(&s_wifi_session_mux);
    verified = secure_bootstrap_wifi_session_is_live(&s_wifi_session, request_generation)
        ? secure_bootstrap_code_verify_and_consume(&s_code, code, now_s())
        : SECURE_BOOTSTRAP_ERR_PLATFORM;
    if (verified == SECURE_BOOTSTRAP_OK) {
        candidate_actions = secure_bootstrap_wifi_transition(
            &s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_SUBMITTED);
        if ((candidate_actions & SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT) == 0U) {
            verified = SECURE_BOOTSTRAP_ERR_PLATFORM;
        } else if (secure_bootstrap_wifi_session_begin_candidate(&s_wifi_session, request_generation)) {
            s_candidate_config = candidate_config;
        } else {
            (void)secure_bootstrap_wifi_transition(
                &s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_CANDIDATE_ABORTED);
            verified = SECURE_BOOTSTRAP_ERR_PLATFORM;
        }
    } else if (verified == SECURE_BOOTSTRAP_OK) {
        verified = SECURE_BOOTSTRAP_ERR_PLATFORM;
    }
    portEXIT_CRITICAL(&s_wifi_session_mux);
    (void)xSemaphoreGive(s_ap_config_mutex);
    secure_zero(&candidate_config, sizeof(candidate_config));
    secure_zero(code, sizeof(code));
    secure_zero(&scanned, sizeof(scanned));
    if (verified != SECURE_BOOTSTRAP_OK) goto forbidden;
    ESP_LOGI(TAG, "WIFI_PROVISIONING code_consumed=true candidate_session_active=true");

    secure_zero(selected, sizeof(selected));
    secure_zero(manual, sizeof(manual));
    secure_zero(password, sizeof(password));
    if (!wifi_session_is_live(request_generation)) {
        return send_html_error(req, HTTPD_403_FORBIDDEN, "Installationen avbröts");
    }
    if (!candidate_disconnect_and_drain_events(request_generation)) {
        ESP_LOGW(TAG, "WIFI_CANDIDATE fence=failed generation=%lu", (unsigned long)request_generation);
        return send_html_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Kunde inte säkert förbereda nätverkstestet. Avbryt och försök igen.");
    }
    if (!wifi_session_is_live(request_generation)) {
        return send_html_error(req, HTTPD_403_FORBIDDEN, "Installationen avbröts");
    }
    if (s_ap_config_mutex == NULL ||
        xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
        return send_html_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Kunde inte förbereda nätverkstestet");
    }
    esp_err_t config_err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (config_err == ESP_OK) config_err = esp_wifi_set_config(WIFI_IF_STA, &s_candidate_config);
    if (config_err == ESP_OK) s_candidate_valid = true;
    (void)xSemaphoreGive(s_ap_config_mutex);
    if (config_err != ESP_OK) {
        ESP_LOGW(TAG, "WIFI_CANDIDATE config=failed result=%s", esp_err_to_name(config_err));
        return send_html_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Kunde inte förbereda nätverkstestet");
    }
    portENTER_CRITICAL(&s_wifi_session_mux);
    const bool armed = secure_bootstrap_wifi_candidate_arm_connect(
        &s_wifi, &s_wifi_session, request_generation, true, true);
    if (armed) s_candidate_connect_generation = request_generation;
    portEXIT_CRITICAL(&s_wifi_session_mux);
    if (!armed) {
        return send_html_error(req, HTTPD_403_FORBIDDEN, "Installationen avbröts");
    }
    const esp_err_t connect_result = apply_actions(candidate_actions);
    if (connect_result != ESP_OK) {
        portENTER_CRITICAL(&s_wifi_session_mux);
        if (s_wifi_session.generation == request_generation &&
            s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE) {
            s_wifi.candidate_connect_armed = false;
        }
        portEXIT_CRITICAL(&s_wifi_session_mux);
        ESP_LOGW(TAG, "WIFI_CANDIDATE connect=failed result=%s", esp_err_to_name(connect_result));
        return send_html_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Kunde inte starta nätverkstestet");
    }
    return send_candidate_status_page(req, s_wifi.saved_config_present);

invalid:
    secure_zero(code, sizeof(code));
    secure_zero(&scanned, sizeof(scanned));
    secure_zero(selected, sizeof(selected));
    secure_zero(manual, sizeof(manual));
    secure_zero(password, sizeof(password));
    return send_html_error(req, HTTPD_400_BAD_REQUEST, "Ogiltiga formulärfält");
forbidden:
    secure_zero(&scanned, sizeof(scanned));
    secure_zero(selected, sizeof(selected));
    secure_zero(manual, sizeof(manual));
    secure_zero(password, sizeof(password));
    return send_html_error(req, HTTPD_403_FORBIDDEN, "Felaktig eller utgången panelkod");
}

static esp_err_t status_handler(httpd_req_t *req) { char json[192]; int n=snprintf(json,sizeof(json),"{\"state\":%u,\"ip_obtained\":%s,\"oauth_locked\":true}",(unsigned)wifi_state_snapshot(),s_ip_obtained?"true":"false"); httpd_resp_set_type(req,"application/json"); return httpd_resp_send(req,json,n); }
static esp_err_t server_start(void)
{
    httpd_config_t cfg=HTTPD_DEFAULT_CONFIG(); cfg.max_uri_handlers=24;
    const runtime_diag_event_t begin_event = {
        .event_type = RUNTIME_DIAG_EVENT_HTTP_SERVER_START_BEGIN,
        .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
        .source = 3U,
    };
    (void)runtime_diag_journal_record(&begin_event);
    ESP_LOGI(TAG, "WIFI_TRACE http_start_begin");
    esp_err_t http_start_err = httpd_start(&s_server, &cfg);
    const runtime_diag_event_t result_event = {
        .event_type = RUNTIME_DIAG_EVENT_HTTP_SERVER_START_RESULT,
        .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
        .source = 3U,
        .result = http_start_err == ESP_OK ? 1U : 2U,
        .error_code = http_start_err,
    };
    (void)runtime_diag_journal_record(&result_event);
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
        phone_provisioning_set_display_callback(panel_provisioning_display_update);
        phone_provisioning_set_portal_css(portal_shared_css());
        ESP_RETURN_ON_ERROR(phone_provisioning_register_handlers(s_server), TAG, "Homey provisioning handlers");
        s_homey_handlers_registered = true;
    }
    return ESP_OK;
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == SECURE_BOOTSTRAP_WIFI_FENCE_EVENT &&
        id == WIFI_FENCE_EVENT_DRAINED && data != NULL &&
        s_wifi_fence_sem != NULL) {
        const uint32_t generation = *(const uint32_t *)data;
        bool matched = false;
        portENTER_CRITICAL(&s_wifi_session_mux);
        if (generation != 0U && s_wifi_fence_wait_generation == generation) {
            s_wifi_fence_done_generation = generation;
            matched = true;
        }
        portEXIT_CRITICAL(&s_wifi_session_mux);
        if (matched) (void)xSemaphoreGive(s_wifi_fence_sem);
        return;
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        const runtime_diag_event_t event = {
            .event_type = RUNTIME_DIAG_EVENT_WIFI_START,
            .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
            .source = 3U,
            .result = 1U,
        };
        (void)runtime_diag_journal_record(&event);
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "WIFI_TRACE scan_done_event");
        (void)consume_network_scan_results();
        return;
    }
    bool persist_active = false;
    wifi_persist_stage_t persist_stage = WIFI_PERSIST_STAGE_IDLE;
    wifi_persist_state_snapshot(&persist_active, &persist_stage);
    if (persist_active && persist_stage != WIFI_PERSIST_STAGE_VERIFY_CONNECT) {
        ESP_LOGI(TAG, "WIFI_PERSIST event_suppressed=%ld", (long)id);
        return;
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *got_ip = (const ip_event_got_ip_t *)data;
        const secure_bootstrap_wifi_state_t observed_state = wifi_state_snapshot();
        wifi_persist_state_snapshot(&persist_active, &persist_stage);
        const bool persist_verify = persist_active && persist_stage == WIFI_PERSIST_STAGE_VERIFY_CONNECT;
        const bool candidate_observation =
            observed_state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE;
        const bool association_verified = (candidate_observation || persist_verify)
            ? wifi_got_ip_matches_candidate(got_ip)
            : wifi_got_ip_matches_saved(got_ip);
        if (!association_verified) {
            ESP_LOGW(TAG, "WIFI_GOT_IP ignored=true reason=active_association_unverified");
            return;
        }
        const bool expected_state = observed_state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED ||
            observed_state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE;
        if (!persist_verify && !expected_state) {
            if (observed_state == SECURE_BOOTSTRAP_WIFI_PROVISIONING) {
                s_ip_obtained = secure_bootstrap_sta_link_apply_event(
                    s_ip_obtained, SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP);
                request_sntp_start();
                const runtime_diag_event_t got_ip_event = {
                    .event_type = RUNTIME_DIAG_EVENT_WIFI_GOT_IP,
                    .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
                    .source = 3U,
                    .result = 1U,
                };
                (void)runtime_diag_journal_record(&got_ip_event);
                phone_provisioning_on_wifi_online();
                ESP_LOGI(TAG,
                         "WIFI_GOT_IP state=provisioning sta_link=online ap_portal=active");
                return;
            }
            ESP_LOGW(TAG, "WIFI_GOT_IP ignored=true state=%u", (unsigned)observed_state);
            return;
        }
        if (persist_verify) {
            s_ip_obtained = secure_bootstrap_sta_link_apply_event(
                s_ip_obtained, SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP);
            request_sntp_start();
            const runtime_diag_event_t got_ip_event = {
                .event_type = RUNTIME_DIAG_EVENT_WIFI_GOT_IP,
                .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
                .source = 3U,
                .result = 1U,
            };
            (void)runtime_diag_journal_record(&got_ip_event);
            uint32_t actions = wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_GOT_IP);
            s_candidate_valid = false;
            secure_zero(&s_candidate_config, sizeof(s_candidate_config));
            (void)backup_clear();
            legacy_sentinel_clear();
            portENTER_CRITICAL(&s_wifi_session_mux);
            s_persist_active = false;
            s_persist_task_scheduled = false;
            s_persist_stage = WIFI_PERSIST_STAGE_IDLE;
            (void)secure_bootstrap_wifi_session_finish_commit(
                &s_wifi_session, s_persist_session_generation);
            portEXIT_CRITICAL(&s_wifi_session_mux);
            ESP_LOGI(TAG, "WIFI_PERSIST complete");
            ESP_LOGI(TAG, "WIFI_ONLINE persistent_saved_connect=true");
            const runtime_diag_event_t online_event = {
                .event_type = RUNTIME_DIAG_EVENT_WIFI_ONLINE,
                .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
                .source = 3U,
                .result = 1U,
            };
            (void)runtime_diag_journal_record(&online_event);
            (void)apply_actions(actions);
            return;
        }
        bool candidate = false;
        uint32_t actions = SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
        bool accepted = true;
        portENTER_CRITICAL(&s_wifi_session_mux);
        secure_bootstrap_session_phase_t phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
        uint32_t phase_generation = 0U;
        (void)secure_bootstrap_wifi_session_current(
            &s_wifi_session, &phase_generation, &phase);
        if (phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED) {
            accepted = false;
        } else {
            candidate = s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE;
            if (candidate) {
                if (phase != SECURE_BOOTSTRAP_SESSION_CANDIDATE ||
                    phase_generation != s_candidate_connect_generation) {
                    accepted = false;
                } else {
                    actions = secure_bootstrap_wifi_candidate_got_ip_verified(
                        &s_wifi, &s_wifi_session, phase_generation,
                        association_verified);
                    accepted = (actions & SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST) != 0U;
                }
            } else if (accepted) {
                actions = secure_bootstrap_wifi_transition(
                    &s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_GOT_IP);
            }
        }
        portEXIT_CRITICAL(&s_wifi_session_mux);
        if (!accepted) {
            ESP_LOGI(TAG, "WIFI_GOT_IP ignored=true reason=session_cancelled");
            return;
        }
        s_ip_obtained = secure_bootstrap_sta_link_apply_event(
            s_ip_obtained, SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP);
        request_sntp_start();
        const runtime_diag_event_t got_ip_event = {
            .event_type = RUNTIME_DIAG_EVENT_WIFI_GOT_IP,
            .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
            .source = 3U,
            .result = 1U,
        };
        (void)runtime_diag_journal_record(&got_ip_event);
        if (candidate && s_panel_ui != NULL && bsp_display_lock(1000)) {
            (void)panel_ui_set_wifi_cancel_pending(s_panel_ui, true, true);
            bsp_display_unlock();
        }
        ESP_LOGI(TAG, "WIFI_GOT_IP commit_requested=%s", candidate ? "true" : "false");
        const esp_err_t action_result = apply_actions(actions);
        if (candidate && action_result != ESP_OK) {
            panel_wifi_present_session();
        }
        if (!candidate) {
            ESP_LOGI(TAG, "WIFI_ONLINE persistent_saved_connect=false");
            const runtime_diag_event_t online_event = {
                .event_type = RUNTIME_DIAG_EVENT_WIFI_ONLINE,
                .monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000LL),
                .source = 3U,
                .result = 1U,
            };
            (void)runtime_diag_journal_record(&online_event);
        }
        return;
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_ip_obtained = secure_bootstrap_sta_link_apply_event(
            s_ip_obtained, SECURE_BOOTSTRAP_STA_LINK_EVENT_DISCONNECTED);
        phone_provisioning_on_wifi_offline();
        cancel_sntp_start_request();
        wifi_persist_state_snapshot(&persist_active, &persist_stage);
        if (persist_active && persist_stage == WIFI_PERSIST_STAGE_VERIFY_CONNECT) {
            (void)wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_PERSIST_VERIFY_FAILED);
            esp_err_t err = rollback_once();
            if (err == ESP_OK) (void)wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_OK);
            else {
                (void)wifi_transition_serialized(SECURE_BOOTSTRAP_WIFI_EVENT_ROLLBACK_FAILED);
            }
            portENTER_CRITICAL(&s_wifi_session_mux);
            (void)secure_bootstrap_wifi_session_finish_commit(
                &s_wifi_session, s_persist_session_generation);
            s_persist_active = false;
            s_persist_task_scheduled = false;
            s_persist_stage = WIFI_PERSIST_STAGE_IDLE;
            portEXIT_CRITICAL(&s_wifi_session_mux);
            if (err != ESP_OK && esp_wifi_start() == ESP_OK) (void)open_provisioning();
            return;
        }
        secure_bootstrap_session_phase_t session_phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
        uint32_t session_generation = 0U;
        uint32_t actions = SECURE_BOOTSTRAP_WIFI_ACTION_NONE;
        bool transition_expected = false;
        portENTER_CRITICAL(&s_wifi_session_mux);
        (void)secure_bootstrap_wifi_session_current(
            &s_wifi_session, &session_generation, &session_phase);
        const bool cancel_reserved =
            session_phase == SECURE_BOOTSTRAP_SESSION_CANCEL_RESERVED;
        transition_expected =
            s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_SAVED ||
            s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE ||
            s_wifi.state == SECURE_BOOTSTRAP_WIFI_ONLINE;
        /* A disconnect event during the candidate fence is part of the
         * deliberate teardown. Do not let the ordinary retry action reconnect
         * before old DHCP/GOT_IP work has been drained. */
        if (s_wifi.state == SECURE_BOOTSTRAP_WIFI_CONNECTING_CANDIDATE &&
            !s_wifi.candidate_connect_armed) {
            transition_expected = false;
        }
        if (!cancel_reserved && transition_expected) {
            actions = secure_bootstrap_wifi_transition(
                &s_wifi, SECURE_BOOTSTRAP_WIFI_EVENT_DISCONNECTED);
        }
        portEXIT_CRITICAL(&s_wifi_session_mux);
        (void)session_generation;
        if (cancel_reserved) {
            ESP_LOGI(TAG, "WIFI_DISCONNECTED deferred=true reason=session_cancelled");
            return;
        }
        if (transition_expected) {
            (void)apply_actions(actions);
            ESP_LOGW(TAG, "WIFI_DISCONNECTED state=%u display_unchanged=true", (unsigned)wifi_state_snapshot());
        }
    }
}

static bool dashboard_snapshot_poll_allowed(
    bool dashboard_visible,
    athom_homey_data_state_t runtime_state)
{
    return runtime_state == ATHOM_HOMEY_DATA_READY || dashboard_visible;
}

static bool dashboard_favorites_apply_allowed(
    athom_homey_data_state_t runtime_state)
{
    return runtime_state == ATHOM_HOMEY_DATA_READY;
}

static bool dashboard_poll_requires_refresh(
    bool model_changed,
    bool favorites_changed)
{
    return model_changed || favorites_changed;
}

static void poll_homey_dashboard_if_due(uint64_t now_ms)
{
    if (s_panel_ui == NULL) {
        return;
    }

    const athom_homey_data_state_t runtime_state =
        athom_oauth_runtime_homey_data_state();
    if (!dashboard_snapshot_poll_allowed(
            s_panel_dashboard_visible, runtime_state)) {
        return;
    }

    if (s_homey_dashboard_last_poll_ms != 0U &&
        now_ms - s_homey_dashboard_last_poll_ms <
            HOMEY_DASHBOARD_POLL_INTERVAL_MS) {
        return;
    }
    s_homey_dashboard_last_poll_ms = now_ms;

    /* Only rotation_task calls this function, from its single task instance.
     * Copy/apply are synchronous and retain no pointer to this workspace. */
    static panel_homey_read_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    panel_homey_read_result_t snapshot_result =
        athom_cloud_copy_device_snapshot(now_ms, &snapshot);
    athom_oauth_runtime_awning_snapshot_observed();

    uint64_t lock_acquired_us = 0U;
    if (!panel_display_lock_traced(50, true, &lock_acquired_us)) {
        s_homey_dashboard_last_poll_ms = 0U;
        return;
    }

    /* Commit snapshot state and its UI model under the same successful lock.
     * A lock timeout must leave the snapshot pending for the next poll. */
    const panel_homey_dashboard_apply_result_t apply_result =
        panel_homey_dashboard_apply_snapshot(
            snapshot_result,
            snapshot_result == PANEL_HOMEY_READ_OK ? &snapshot : NULL,
            now_ms,
            &s_homey_dashboard_state);

    panel_information_observe_snapshot(&s_information_snapshot, snapshot_result,
        snapshot.captured_at_ms, snapshot.generation);

    bool model_changed = false;
    if (apply_result == PANEL_HOMEY_DASHBOARD_APPLY_UPDATED) {
        model_changed = panel_ui_apply_homey_dashboard_state(
            &s_panel_model,
            &s_homey_dashboard_state);
    }
    const bool snapshot_fresh = snapshot_result == PANEL_HOMEY_READ_OK &&
        s_homey_dashboard_state.generation_valid &&
        !s_homey_dashboard_state.stale;
    const panel_ui_connection_state_t desired_connection =
        !phone_provisioning_wifi_online()
            ? PANEL_UI_CONNECTION_OFFLINE
            : (runtime_state == ATHOM_HOMEY_DATA_READY && snapshot_fresh
                   ? PANEL_UI_CONNECTION_CONNECTED
                   : PANEL_UI_CONNECTION_CONNECTING);
    const bool presentation_ready =
        desired_connection == PANEL_UI_CONNECTION_CONNECTED;
    const bool connection_changed =
        desired_connection != s_dashboard_connection_state;
    const bool readiness_changed =
        presentation_ready != s_dashboard_homey_data_ready;
    if (connection_changed || readiness_changed) {
        const panel_ui_connection_info_t connection = {
            .state = desired_connection,
            .display_name = "",
        };
        (void)panel_ui_set_connection(s_panel_ui, &connection);
        (void)panel_ui_set_homey_data_ready(s_panel_ui, presentation_ready);
        s_dashboard_connection_state = desired_connection;
        s_dashboard_homey_data_ready = presentation_ready;
    }
    const bool favorites_changed = dashboard_favorites_apply_allowed(runtime_state)
        ? panel_homey_favorites_apply_ui_model(&s_panel_model)
        : false;
    if (model_changed) s_panel_perf.homey_model_changed_count++;
    if (favorites_changed) s_panel_perf.homey_favorites_changed_count++;
    if (dashboard_poll_requires_refresh(
            model_changed || connection_changed || readiness_changed,
            favorites_changed)) {
        s_panel_perf.homey_refresh_requests++;
        (void)panel_ui_refresh(s_panel_ui);
    }

    panel_display_unlock_traced(lock_acquired_us);
}

static void rotation_task(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t session_generation = 0U;
        secure_bootstrap_session_phase_t session_phase = SECURE_BOOTSTRAP_SESSION_CLOSED;
        const bool active_session = wifi_session_snapshot(
            &session_generation, &session_phase) &&
            session_phase == SECURE_BOOTSTRAP_SESSION_ACTIVE;
        if (wifi_state_snapshot() == SECURE_BOOTSTRAP_WIFI_PROVISIONING && active_session) {
            if (s_ap_config_mutex != NULL &&
                xSemaphoreTake(s_ap_config_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
              if (wifi_session_is_live(session_generation) &&
                  secure_bootstrap_code_rotation_due(&s_code, now_s())) {
                secure_bootstrap_code_t next = {0};
                uint8_t random_bytes[SECURE_BOOTSTRAP_CODE_LEN];
                esp_fill_random(random_bytes, sizeof(random_bytes));
                secure_bootstrap_status_t generated = secure_bootstrap_code_generate(&next, now_s(), random_bytes, sizeof(random_bytes));
                secure_zero(random_bytes, sizeof(random_bytes));
                const bool ap_rotated = generated == SECURE_BOOTSTRAP_OK &&
                    configure_access_point(next.code) == ESP_OK;
                bool rotation_committed = false;
                if (ap_rotated) {
                    /* The cancel/commit winner shares this mux. If cancel
                     * reserved first, restore the still-active AP key before
                     * publishing anything from this stale rotation. */
                    portENTER_CRITICAL(&s_wifi_session_mux);
                    rotation_committed = secure_bootstrap_wifi_session_is_live(
                        &s_wifi_session, session_generation);
                    if (rotation_committed) {
                        secure_bootstrap_code_wipe(&s_code);
                        s_code = next;
                        memset(&next, 0, sizeof(next));
                    }
                    portEXIT_CRITICAL(&s_wifi_session_mux);
                    if (!rotation_committed) {
                        const esp_err_t restore_err = configure_access_point(s_code.code);
                        ESP_LOGW(TAG, "WIFI_PROVISIONING code_rotation_commit=false session_fenced=true ap_restore=%s",
                                 esp_err_to_name(restore_err));
                    }
                }
                if (rotation_committed) {
                    set_display_text(
                        "Säker Wi-Fi-installation",
                        "Anslut till HomeyPanel-Setup\nÖppna 192.168.4.1",
                        s_code.code);
                    ESP_LOGI(TAG, "WIFI_PROVISIONING code_rotation_commit=true");
                } else {
                    secure_bootstrap_code_wipe(&next);
                    if (!ap_rotated) {
                        ESP_LOGW(TAG, "WIFI_PROVISIONING code_rotation_commit=false session_fenced=true");
                    }
                }
              }
              (void)xSemaphoreGive(s_ap_config_mutex);
            }
            refresh_code_countdown();
        } else {
            s_last_code_countdown_s = -1;
        }
        const uint64_t panel_now_ms =
            (uint64_t)(esp_timer_get_time() / 1000LL);
        if (wifi_state_snapshot() == SECURE_BOOTSTRAP_WIFI_ONLINE &&
            take_sntp_start_request()) {
            start_network_time_sync_once();
        }
        const uint32_t light_completion_generation =
            athom_oauth_runtime_light_toggle_completion_generation();
        const bool light_completion_changed =
            light_completion_generation !=
                s_light_toggle_completion_generation_seen;
        if (light_completion_changed) {
            s_homey_dashboard_last_poll_ms = 0U;
        }
        poll_homey_dashboard_if_due(panel_now_ms);
        s_light_toggle_completion_generation_seen =
            light_completion_generation;
        update_homey_data_boot_view(panel_now_ms);
        uint64_t lock_acquired_us = 0U;
        if (s_panel_ui != NULL &&
            panel_display_lock_traced(50, false, &lock_acquired_us)) {
            if (light_completion_changed &&
                athom_oauth_runtime_homey_data_state() != ATHOM_HOMEY_DATA_READY) {
                (void)panel_ui_set_homey_data_ready(s_panel_ui, false);
            }
            (void)panel_ui_set_light_toggle_pending(
                s_panel_ui, 4U,
                athom_oauth_runtime_light_toggle_pending(4U));
            (void)panel_ui_set_light_toggle_pending(
                s_panel_ui, 5U,
                athom_oauth_runtime_light_toggle_pending(5U));
            update_panel_clock_if_due();
            panel_ui_settings_publish_result();
            const panel_power_state_t panel_power_before = s_panel_model.power_state;
            if (panel_ui_update_inactivity(s_panel_ui, panel_now_ms)) {
                const uint64_t idle_ms = panel_now_ms >= s_panel_model.last_activity_ms
                    ? panel_now_ms - s_panel_model.last_activity_ms : 0U;
                ESP_LOGI(TAG,
                    "PANEL_POWER transition=%u->%u idle_ms=%llu dim_threshold_ms=%llu off_threshold_ms=%llu",
                    (unsigned)panel_power_before, (unsigned)s_panel_model.power_state,
                    (unsigned long long)idle_ms,
                    (unsigned long long)s_panel_model.settings.dim_after_seconds * 1000ULL,
                    (unsigned long long)s_panel_model.settings.off_after_seconds * 1000ULL);
            }
            panel_display_perf_log_if_due();
            panel_display_unlock_traced(lock_acquired_us);
        }
#if PANEL_UPDATE_I3_OBSERVER_ENABLED
        /* One owner: the pre-existing rotation task; no new UI or Homey actions. */
        const panel_update_i3_owner_input_t observer_input = {
            .monotonic_ms = panel_now_ms,
            .wifi_online = wifi_state_snapshot() == SECURE_BOOTSTRAP_WIFI_ONLINE,
            .ui_allocated = s_panel_ui != NULL,
            .homey_ready = athom_oauth_runtime_homey_data_state() == ATHOM_HOMEY_DATA_READY,
            .operator_trial_approved = false, /* No operator consent source exists yet. */
        };
        panel_update_i3_owner_active_tick(&observer_input);
#endif
        vTaskDelay(pdMS_TO_TICKS(ROTATION_POLL_INTERVAL_MS));
    }
}
static esp_err_t wipe_homey_account_state(void)
{
    athom_cloud_alias_invalidate();

    esp_err_t first_error = ESP_OK;
    panel_homey_alias_store_result_t alias_result =
        panel_homey_alias_store_wipe();
    if (alias_result != PANEL_HOMEY_ALIAS_STORE_OK) {
        first_error = ESP_FAIL;
    }

    esp_err_t auth_result = athom_auth_store_wipe();
    if (auth_result != ESP_OK && first_error == ESP_OK) {
        first_error = auth_result;
    }

    return first_error;
}

static void wipe_task(void *arg)
{
    (void)arg; gpio_config_t io={.pin_bit_mask=1ULL<<BOOT_BUTTON_GPIO,.mode=GPIO_MODE_INPUT,.pull_up_en=GPIO_PULLUP_ENABLE}; ESP_ERROR_CHECK(gpio_config(&io)); secure_bootstrap_wipe_tracker_t t={0};
    for(;;){ if(secure_bootstrap_wipe_tracker_update(&t,gpio_get_level(BOOT_BUTTON_GPIO)==0,esp_timer_get_time()/1000LL)){ ESP_LOGW(TAG,"BOOTSTRAP_PHYSICAL_REPROVISION accepted=true"); esp_err_t homey_wipe_error = wipe_homey_account_state(); if (homey_wipe_error != ESP_OK) { ESP_LOGW(TAG, "HOMEY_ACCOUNT_WIPE result=%s", esp_err_to_name(homey_wipe_error)); } (void)backup_clear(); legacy_sentinel_clear(); ESP_ERROR_CHECK(esp_wifi_restore()); esp_restart(); } vTaskDelay(pdMS_TO_TICKS(WIPE_POLL_INTERVAL_MS)); }
}

secure_bootstrap_status_t secure_bootstrap_start(void)
{
    s_panel_timezone_ready = panel_clock_configure_timezone();
    if (!s_panel_timezone_ready) {
        ESP_LOGW(TAG, "TIME_SYNC timezone_configured=false; clock display remains unsynchronized");
    }
    /* app_main has completed NVS initialization before secure_bootstrap_start.
     * Load settings before display/UI creation and without the LVGL lock. */
    panel_ui_settings_storage_start();
    panel_startup_status_init(&s_startup_status);
    memset(&s_homey_data_wait, 0, sizeof(s_homey_data_wait));
    s_homey_data_long_wait_rendered = false;
    startup_status_active_set(true);
    secure_bootstrap_wifi_context_init(&s_wifi);
    secure_bootstrap_wifi_session_init(&s_wifi_session);
    s_ap_config_mutex = xSemaphoreCreateMutex();
    if (s_ap_config_mutex == NULL) return SECURE_BOOTSTRAP_ERR_PLATFORM;
    s_wifi_fence_sem = xSemaphoreCreateBinary();
    if (s_wifi_fence_sem == NULL) return SECURE_BOOTSTRAP_ERR_PLATFORM;
    s_networks_mutex = xSemaphoreCreateMutex();
    if (s_networks_mutex == NULL) return SECURE_BOOTSTRAP_ERR_PLATFORM;
    legacy_sentinel_clear();
    if(display_init()!=ESP_OK)return SECURE_BOOTSTRAP_ERR_PLATFORM;
    (void)set_startup_display_text(
        "Ansluter till Wi-Fi...", "Kontrollerar sparad konfiguration", false);
    ESP_ERROR_CHECK(esp_netif_init()); ESP_ERROR_CHECK(esp_event_loop_create_default()); s_ap_netif=esp_netif_create_default_wifi_ap(); s_sta_netif=esp_netif_create_default_wifi_sta(); if(!s_ap_netif||!s_sta_netif)return SECURE_BOOTSTRAP_ERR_PLATFORM;
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    s_wifi_nvs_enabled = init.nvs_enable != 0;
    ESP_LOGI(TAG, "WIFI_BOOT nvs_enabled=%s", s_wifi_nvs_enabled ? "true" : "false");
    ESP_ERROR_CHECK(esp_wifi_init(&init)); ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event,NULL)); ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event,NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SECURE_BOOTSTRAP_WIFI_FENCE_EVENT,
        WIFI_FENCE_EVENT_DRAINED, wifi_event, NULL));
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
    uint32_t actions=wifi_transition_serialized(s_saved_valid?SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITH_SAVED:SECURE_BOOTSTRAP_WIFI_EVENT_BOOT_WITHOUT_SAVED);
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
            8192,
            NULL,
            5,
            NULL) != pdPASS) {
        return SECURE_BOOTSTRAP_ERR_PLATFORM;
    }
    ESP_LOGI(TAG,"BOOTSTRAP_READY saved_config=%s softap=%s oauth_locked=true",s_saved_valid?"true":"false",s_saved_valid?"false":"true"); return SECURE_BOOTSTRAP_OK;
}
#endif
