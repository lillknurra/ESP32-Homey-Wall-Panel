/* Patch116I2: read-only single-owner worker skeleton.
 * Default-off firmware has no worker task and no network symbols.
 */
#include "panel_update_worker.h"
#ifndef PANEL_UPDATE_READONLY_WORKER_ENABLED
#define PANEL_UPDATE_READONLY_WORKER_ENABLED 0
#endif
#if PANEL_UPDATE_READONLY_WORKER_ENABLED != 0 && PANEL_UPDATE_READONLY_WORKER_ENABLED != 1
#error "PANEL_UPDATE_READONLY_WORKER_ENABLED must be 0 or 1"
#endif

#if !PANEL_UPDATE_READONLY_WORKER_ENABLED
bool panel_update_worker_bootstrap(void) { return false; }
bool panel_update_worker_request(uint32_t generation, const char *channel,
    bool wifi_ready, bool normal_boot_settled)
{
    (void)generation; (void)channel; (void)wifi_ready; (void)normal_boot_settled;
    return false;
}
panel_update_poll_result_t panel_update_worker_poll(uint32_t *generation,
    char *body, size_t capacity, size_t *body_length)
{
    (void)body; (void)capacity;
    if (generation) *generation = 0U;
    if (body_length) *body_length = 0U;
    return PANEL_UPDATE_POLL_DISABLED;
}
#else
#if !defined(ESP_PLATFORM)
#error "Active worker requires ESP-IDF; host verifies separate pure-C policy"
#endif
#if !defined(PANEL_UPDATE_WORKER_AUDIT_ONLY) || PANEL_UPDATE_WORKER_AUDIT_ONLY != 1
#error "Native worker is compile-audit-only, NOT approved for runtime activation"
#endif
#if !defined(PANEL_UPDATE_MANIFEST_HTTPS_ENABLED) || PANEL_UPDATE_MANIFEST_HTTPS_ENABLED != 1
#error "Worker requires the separately audited read-only HTTPS fetch"
#endif
#include "panel_update_https_fetch.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* Build-only prototype. The 48KB internal heap floor is deliberately
 * conservative; runtime budgets and task stack margins are NOT established.
 */
#define WORKER_STACK_BYTES 8192U
#define WORKER_MIN_INTERNAL_FREE_BYTES (48U * 1024U)
#define WORKER_PRIORITY 2U

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t s_worker_task = NULL;
static panel_update_worker_policy_t s_policy;
static char s_channel[PANEL_UPDATE_CHANNEL_SIZE];
static char s_body[PANEL_UPDATE_MANIFEST_MAX_BYTES + 1U];
static size_t s_body_length;

static uint64_t monotonic_ms(void)
{
    int64_t us = esp_timer_get_time();
    return us < 0 ? UINT64_MAX : (uint64_t)us / 1000U;
}

static void worker_loop(void *context)
{
    (void)context;
    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t generation = 0U;
        char channel[PANEL_UPDATE_CHANNEL_SIZE] = {0};
        portENTER_CRITICAL(&s_mux);
        generation = s_policy.generation;
        bool claimed = panel_update_worker_policy_worker_begin(&s_policy, generation);
        if (claimed) memcpy(channel, s_channel, sizeof(channel));
        portEXIT_CRITICAL(&s_mux);
        if (!claimed) continue;
        size_t length = 0U;
        /* The worker uniquely owns HTTP until the fetch returns and cleans up.
         * Do not terminate it or free its context on caller-visible timeout. */
        const panel_update_fetch_result_t result = panel_update_https_fetch(
            channel, s_body, sizeof(s_body), &length);
        const uint64_t finished_at = monotonic_ms();
        portENTER_CRITICAL(&s_mux);
        const bool accepted = panel_update_worker_policy_worker_finish(
            &s_policy, generation, finished_at, result == PANEL_UPDATE_FETCH_OK);
        s_body_length = accepted && result == PANEL_UPDATE_FETCH_OK ? length : 0U;
        portEXIT_CRITICAL(&s_mux);
    }
}

bool panel_update_worker_bootstrap(void)
{
    /* Call once from the owning task after startup settled; never from Wi-Fi callback. */
    if (s_worker_task != NULL ||
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL) < WORKER_MIN_INTERNAL_FREE_BYTES) return false;
    panel_update_worker_policy_init(&s_policy);
    return xTaskCreate(worker_loop, "panel_meta_check", WORKER_STACK_BYTES,
                       NULL, WORKER_PRIORITY, &s_worker_task) == pdPASS;
}

bool panel_update_worker_request(uint32_t generation, const char *channel,
    bool wifi_ready, bool normal_boot_settled)
{
    if (!s_worker_task || !channel ||
        (strcmp(channel,"stable") != 0 && strcmp(channel,"beta") != 0)) return false;
    const bool heap_ok = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) >=
                         WORKER_MIN_INTERNAL_FREE_BYTES;
    const uint64_t now = monotonic_ms();
    portENTER_CRITICAL(&s_mux);
    bool queued = panel_update_worker_policy_queue(&s_policy, generation,
        now, wifi_ready, normal_boot_settled, heap_ok);
    if (queued) {
        memset(s_channel, 0, sizeof(s_channel));
        memcpy(s_channel, channel, strlen(channel));
        s_body_length = 0U;
    }
    portEXIT_CRITICAL(&s_mux);
    if (queued) xTaskNotifyGive(s_worker_task);
    return queued;
}

panel_update_poll_result_t panel_update_worker_poll(uint32_t *generation,
    char *body, size_t capacity, size_t *body_length)
{
    if (generation) *generation = 0U;
    if (body_length) *body_length = 0U;
    if (!s_worker_task || !generation || !body || !body_length ||
        capacity < sizeof(s_body)) return PANEL_UPDATE_POLL_DISABLED;
    bool success = false;
    uint32_t gen = 0U;
    size_t length = 0U;
    panel_update_worker_phase_t phase;
    const uint64_t now = monotonic_ms();
    portENTER_CRITICAL(&s_mux);
    (void)panel_update_worker_policy_tick(&s_policy, now);
    phase = s_policy.phase;
    gen = s_policy.generation;
    if (panel_update_worker_policy_take_begin(&s_policy, gen, now, &success)) {
        length = s_body_length;
        phase = PANEL_UPDATE_WORKER_READING;
    }
    portEXIT_CRITICAL(&s_mux);
    if (phase == PANEL_UPDATE_WORKER_READING) {
        /* A READING reservation prevents worker buffer reuse without holding
         * an interrupt-disabling critical section during the 2KB copy. */
        if (success && length > 0U && length < capacity) {
            memcpy(body, s_body, length);
            body[length] = '\0';
            *body_length = length;
        } else {
            success = false;
        }
        portENTER_CRITICAL(&s_mux);
        const bool ended = panel_update_worker_policy_take_end(&s_policy, gen);
        portEXIT_CRITICAL(&s_mux);
        if (!ended) return PANEL_UPDATE_POLL_QUARANTINED;
        *generation = gen;
        return success ? PANEL_UPDATE_POLL_RESULT_OK : PANEL_UPDATE_POLL_RESULT_ERROR;
    }
    if (phase == PANEL_UPDATE_WORKER_QUARANTINED) return PANEL_UPDATE_POLL_QUARANTINED;
    if (phase == PANEL_UPDATE_WORKER_IDLE) return PANEL_UPDATE_POLL_IDLE;
    return PANEL_UPDATE_POLL_BUSY;
}
#endif
