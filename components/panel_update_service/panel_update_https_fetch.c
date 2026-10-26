/* Patch116I1: read-only fixed-origin HTTPS manifest fetch.
 * Never follows redirects, downloads firmware, mutates flash, or issues Homey calls.
 * Default-disabled. A logical post-call deadline is NOT an absolute wall-clock
 * bound for esp_http_client_open/fetch_headers/read, DNS, or TLS.
 */
#include "panel_update_https_fetch.h"
#include <stdbool.h>
#include <string.h>

#ifndef PANEL_UPDATE_MANIFEST_HTTPS_ENABLED
#define PANEL_UPDATE_MANIFEST_HTTPS_ENABLED 0
#endif
#if PANEL_UPDATE_MANIFEST_HTTPS_ENABLED != 0 && PANEL_UPDATE_MANIFEST_HTTPS_ENABLED != 1
#error "PANEL_UPDATE_MANIFEST_HTTPS_ENABLED must be 0 or 1"
#endif

#define PANEL_UPDATE_STABLE_URL \
    "https://lillknurra.github.io/ESP32-Homey-Wall-Panel/updates/stable/waveshare-esp32-s3-touch-lcd-4b-n16r8.json"
#define PANEL_UPDATE_BETA_URL \
    "https://lillknurra.github.io/ESP32-Homey-Wall-Panel/updates/beta/waveshare-esp32-s3-touch-lcd-4b-n16r8.json"

#if PANEL_UPDATE_MANIFEST_HTTPS_ENABLED && !defined(ESP_PLATFORM)
#error "Active manifest network requires ESP-IDF; host tests use ESP_PLATFORM API shims"
#endif

#if PANEL_UPDATE_MANIFEST_HTTPS_ENABLED
#if !defined(PANEL_UPDATE_HTTPS_AUDIT_ONLY) || PANEL_UPDATE_HTTPS_AUDIT_ONLY != 1
#error "HTTP metadata network remains research-only: never activate until worker lifetime and bounded I/O are reviewed"
#endif
#if !defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE) || !CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#error "Manifest HTTPS requires verified X509 certificate bundle"
#endif
#if !defined(CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS) || !CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS
#error "Manifest network requires ESP-IDF HTTPS client enabled"
#endif
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_timer.h"

/* These parameters reduce ordinary stalls; they do NOT make the underlying
 * synchronous functions preemptible. Run only on an isolated, single owner.
 */
#define PANEL_UPDATE_HTTP_OPERATION_TIMEOUT_MS 1500
#define PANEL_UPDATE_HTTP_MAX_NO_PROGRESS 4U
#define PANEL_UPDATE_HTTP_READ_CHUNK 256U

static int64_t current_ms(void)
{
    const int64_t us = esp_timer_get_time();
    return us < 0 ? -1 : us / 1000;
}

static bool before_deadline(int64_t start_ms)
{
    const int64_t now = current_ms();
    return start_ms >= 0 && now >= start_ms &&
           (uint64_t)(now - start_ms) < PANEL_UPDATE_CHECK_DEADLINE_MS;
}

static const char *manifest_url(const char *channel)
{
    if (!channel) return NULL;
    if (strcmp(channel, "stable") == 0) return PANEL_UPDATE_STABLE_URL;
    if (strcmp(channel, "beta") == 0) return PANEL_UPDATE_BETA_URL;
    return NULL;
}
#endif

panel_update_fetch_result_t panel_update_https_fetch(
    const char *channel, char *body, size_t body_capacity, size_t *body_length)
{
    if (body_length) *body_length = 0U;
    if (!body || !body_length || body_capacity < PANEL_UPDATE_MANIFEST_MAX_BYTES + 1U ||
        !channel || (strcmp(channel, "stable") != 0 && strcmp(channel, "beta") != 0)) {
        return PANEL_UPDATE_FETCH_REJECTED;
    }
    body[0] = '\0';
#if !PANEL_UPDATE_MANIFEST_HTTPS_ENABLED
    return PANEL_UPDATE_FETCH_DISABLED;
#else
    const char *url = manifest_url(channel);
    const int64_t start_ms = current_ms();
    if (!url || start_ms < 0) return PANEL_UPDATE_FETCH_REJECTED;
    esp_http_client_config_t cfg = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .disable_auto_redirect = true,
        .keep_alive_enable = false,
        .timeout_ms = PANEL_UPDATE_HTTP_OPERATION_TIMEOUT_MS,
        .buffer_size = 512,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return PANEL_UPDATE_FETCH_NETWORK_ERROR;
    panel_update_fetch_result_t result = PANEL_UPDATE_FETCH_NETWORK_ERROR;
    size_t total = 0U;
    bool opened = false;
    do {
        if (!before_deadline(start_ms)) { result = PANEL_UPDATE_FETCH_TIMEOUT; break; }
        if (esp_http_client_set_method(client, HTTP_METHOD_GET) != ESP_OK ||
            esp_http_client_set_header(client, "Accept", "application/json") != ESP_OK ||
            esp_http_client_set_header(client, "Accept-Encoding", "identity") != ESP_OK ||
            esp_http_client_set_header(client, "Cache-Control", "no-cache") != ESP_OK) break;
        if (!before_deadline(start_ms)) { result = PANEL_UPDATE_FETCH_TIMEOUT; break; }
        if (esp_http_client_open(client, 0) != ESP_OK) break;
        opened = true;
        if (!before_deadline(start_ms)) { result = PANEL_UPDATE_FETCH_TIMEOUT; break; }
        const int64_t content_length = esp_http_client_fetch_headers(client);
        if (!before_deadline(start_ms)) { result = PANEL_UPDATE_FETCH_TIMEOUT; break; }
        if (esp_http_client_get_status_code(client) != 200 ||
            esp_http_client_is_chunked_response(client) || content_length < 1 ||
            content_length > PANEL_UPDATE_MANIFEST_MAX_BYTES) {
            result = PANEL_UPDATE_FETCH_REJECTED;
            break;
        }
        const size_t expected = (size_t)content_length;
        unsigned no_progress = 0U;
        while (total < expected) {
            if (!before_deadline(start_ms)) { result = PANEL_UPDATE_FETCH_TIMEOUT; break; }
            const size_t remaining = expected - total;
            const int want = (int)(remaining < PANEL_UPDATE_HTTP_READ_CHUNK
                                    ? remaining : PANEL_UPDATE_HTTP_READ_CHUNK);
            const int count = esp_http_client_read(client, body + total, want);
            if (!before_deadline(start_ms)) { result = PANEL_UPDATE_FETCH_TIMEOUT; break; }
            if (count == -ESP_ERR_HTTP_EAGAIN) {
                if (++no_progress >= PANEL_UPDATE_HTTP_MAX_NO_PROGRESS) {
                    result = PANEL_UPDATE_FETCH_TIMEOUT;
                    break;
                }
                continue;
            }
            if (count <= 0 || count > want) {
                result = PANEL_UPDATE_FETCH_INCOMPLETE;
                break;
            }
            total += (size_t)count;
            no_progress = 0U;
        }
        if (result == PANEL_UPDATE_FETCH_TIMEOUT || result == PANEL_UPDATE_FETCH_INCOMPLETE) break;
        if (total != expected || !esp_http_client_is_complete_data_received(client)) {
            result = PANEL_UPDATE_FETCH_INCOMPLETE;
            break;
        }
        if (!before_deadline(start_ms)) { result = PANEL_UPDATE_FETCH_TIMEOUT; break; }
        body[total] = '\0';
        *body_length = total;
        result = PANEL_UPDATE_FETCH_OK;
    } while (false);
    if (opened) (void)esp_http_client_close(client);
    (void)esp_http_client_cleanup(client);
    if (result != PANEL_UPDATE_FETCH_OK) {
        memset(body, 0, total < body_capacity ? total + 1U : body_capacity);
        *body_length = 0U;
    }
    return result;
#endif
}

panel_update_fetch_result_t panel_update_https_fetch_and_complete(
    panel_update_check_t *check, uint32_t generation,
    const panel_update_local_t *local)
{
    if (!check || !local || !local->channel || !check->in_flight ||
        generation == 0U || generation != check->generation) {
        return PANEL_UPDATE_FETCH_REJECTED;
    }
    char body[PANEL_UPDATE_MANIFEST_MAX_BYTES + 1U];
    size_t size = 0U;
    const panel_update_fetch_result_t result = panel_update_https_fetch(
        local->channel, body, sizeof(body), &size);
    /* Actual completion must use the current monotonic time, not the stale
     * start time supplied by the caller. Disabled builds have no clock hook;
     * no completion may be published in that case.
     */
#if PANEL_UPDATE_MANIFEST_HTTPS_ENABLED
    const int64_t completion_ms = current_ms();
    uint64_t checked_now = completion_ms < 0 ? UINT64_MAX : (uint64_t)completion_ms;
#else
    return result;
#endif
#if PANEL_UPDATE_MANIFEST_HTTPS_ENABLED
    (void)panel_update_check_complete(check, generation, checked_now,
                                      result == PANEL_UPDATE_FETCH_OK,
                                      result == PANEL_UPDATE_FETCH_OK ? body : NULL,
                                      size, local);
#endif
    return result;
}
