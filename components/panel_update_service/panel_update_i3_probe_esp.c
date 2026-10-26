#include "panel_update_i3_probe.h"
#include "panel_update_i3a_web_diag.h"
#if !PANEL_UPDATE_I3_OBSERVER_ENABLED
void panel_update_i3_owner_tick(const panel_update_i3_owner_input_t *input)
{
    (void)input;
}
bool panel_update_i3a_snapshot_copy(panel_update_i3a_snapshot_t *out)
{
    (void)out;
    return false;
}
#else
#if !defined(ESP_PLATFORM)
#error "Patch116I3 observer requires ESP-IDF"
#endif
#if !defined(PANEL_UPDATE_I3_AUDIT_ONLY) || PANEL_UPDATE_I3_AUDIT_ONLY != 1
#error "Patch116I3 observer is audit-only, not enabled for production firmware"
#endif
#include "panel_update_i3_probe_policy.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <limits.h>
#include <inttypes.h>
#include <stddef.h>

#if PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED
#if !defined(PANEL_UPDATE_READONLY_WORKER_ENABLED) || PANEL_UPDATE_READONLY_WORKER_ENABLED != 1 || \
    !defined(PANEL_UPDATE_MANIFEST_HTTPS_ENABLED) || PANEL_UPDATE_MANIFEST_HTTPS_ENABLED != 1 || \
    !defined(PANEL_UPDATE_WORKER_AUDIT_ONLY) || PANEL_UPDATE_WORKER_AUDIT_ONLY != 1 || \
    !defined(PANEL_UPDATE_HTTPS_AUDIT_ONLY) || PANEL_UPDATE_HTTPS_AUDIT_ONLY != 1
#error "I3 one-shot transport can only compile with both existing audit-only guards"
#endif
#include "panel_update_worker.h"
#include "panel_update_manifest.h"
#endif

static const char *TAG = "panel_update_observer";
static panel_update_i3_policy_t s_probe_policy; /* single owner */
static bool s_probe_policy_initialized;
/* Single writer, concurrent HTTP reader. Only a small numeric snapshot is
 * copied inside the spinlock; heap queries and logging occur outside. */
static portMUX_TYPE s_i3a_snapshot_mux = portMUX_INITIALIZER_UNLOCKED;
static panel_update_i3a_snapshot_t s_i3a_snapshot;
static bool s_i3a_snapshot_valid;

bool panel_update_i3a_snapshot_copy(panel_update_i3a_snapshot_t *out)
{
    if (!out) return false;
    portENTER_CRITICAL(&s_i3a_snapshot_mux);
    const bool valid = s_i3a_snapshot_valid;
    if (valid) *out = s_i3a_snapshot;
    portEXIT_CRITICAL(&s_i3a_snapshot_mux);
    return valid;
}

#if PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED
static bool s_trial_in_flight;
static uint64_t s_trial_started_ms;
static uint64_t s_last_trial_poll_ms;
static char s_trial_body[PANEL_UPDATE_MANIFEST_MAX_BYTES + 1U];
static bool s_trial_completed;
#endif

static uint32_t safely_narrow(size_t size)
{
    return size > UINT32_MAX ? UINT32_MAX : (uint32_t)size;
}

void panel_update_i3_owner_active_tick(const panel_update_i3_owner_input_t *in)
{
    if (!in || in->monotonic_ms == UINT64_MAX) return;
    if (s_probe_policy.stopped) return;
    if (s_probe_policy_initialized && s_probe_policy.initialized &&
        in->monotonic_ms < s_probe_policy.previous_ms) {
        s_probe_policy.stopped = true; /* Fail closed on a monotonic regression. */
        return;
    }
    if (!s_probe_policy_initialized) {
        panel_update_i3_policy_init(&s_probe_policy);
        s_probe_policy_initialized = true;
    }
    /* Cheap early return: do not query heap or FreeRTOS every rotation tick. */
    const bool due = !s_probe_policy.initialized ||
        (in->monotonic_ms >= s_probe_policy.last_sample_ms &&
         in->monotonic_ms - s_probe_policy.last_sample_ms >= PANEL_UPDATE_I3_SAMPLE_MS);
    if (due) {
        const panel_update_i3_sample_t sample = {
            .now_ms = in->monotonic_ms,
            .internal_free_bytes = safely_narrow(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
            .internal_largest_bytes = safely_narrow(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)),
            .internal_minimum_free_bytes = safely_narrow(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)),
            .owner_stack_margin_bytes = (uint32_t)uxTaskGetStackHighWaterMark(NULL),
            .wifi_online = in->wifi_online,
            .ui_allocated = in->ui_allocated,
            .homey_ready = in->homey_ready,
            .operator_trial_approved = in->operator_trial_approved,
        };
        const panel_update_i3_decision_t decision = panel_update_i3_policy_step(
            &s_probe_policy, &sample, PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED == 1);
        if (decision.sampled) {
            const panel_update_i3a_snapshot_t published = {
                .monotonic_ms = sample.now_ms,
                .internal_free_bytes = sample.internal_free_bytes,
                .internal_largest_block_bytes = sample.internal_largest_bytes,
                .internal_minimum_free_bytes = sample.internal_minimum_free_bytes,
                .rotation_stack_margin_bytes = sample.owner_stack_margin_bytes,
                .reset_reason = (uint32_t)esp_reset_reason(),
                .wifi_online = sample.wifi_online,
                .ui_allocated = sample.ui_allocated,
                .homey_ready = sample.homey_ready,
            };
            portENTER_CRITICAL(&s_i3a_snapshot_mux);
            s_i3a_snapshot = published;
            s_i3a_snapshot_valid = true;
            portEXIT_CRITICAL(&s_i3a_snapshot_mux);
            /* Health flags are passive observations, not visible display tests.
             * Never log addresses, names, URLs, tokens or other private state. */
            ESP_LOGI(TAG,
                "resource_probe sample=%" PRIu32 " monotonic_ms=%" PRIu64
                " internal_free=%" PRIu32
                " internal_largest=%" PRIu32 " internal_min_free=%" PRIu32
                " rotation_stack_hwm=%" PRIu32 " wifi=%u ui_allocated=%u homey_ready=%u healthy=%u",
                s_probe_policy.count, sample.now_ms, sample.internal_free_bytes,
                sample.internal_largest_bytes, sample.internal_minimum_free_bytes,
                sample.owner_stack_margin_bytes, (unsigned)sample.wifi_online,
                (unsigned)sample.ui_allocated, (unsigned)sample.homey_ready,
                (unsigned)decision.sample_healthy);
        }
#if PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED
        if (decision.one_shot_trial_request && !s_trial_completed) {
            /* Exactly one try per boot; never repeat after bootstrap failure. */
            const bool created = panel_update_worker_bootstrap();
            const bool queued = created && panel_update_worker_request(
                1U, "stable", in->wifi_online, in->ui_allocated);
            s_trial_in_flight = queued;
            s_trial_completed = !queued;
            s_trial_started_ms = in->monotonic_ms;
            ESP_LOGI(TAG, "one_shot_trial queued=%u", (unsigned)queued);
        }
#endif
    }
#if PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED
    if (!s_trial_in_flight || in->monotonic_ms < s_last_trial_poll_ms ||
        in->monotonic_ms - s_last_trial_poll_ms < UINT64_C(1000)) return;
    s_last_trial_poll_ms = in->monotonic_ms;
    uint32_t generation = 0U;
    size_t size = 0U;
    const panel_update_poll_result_t result = panel_update_worker_poll(
        &generation, s_trial_body, sizeof(s_trial_body), &size);
    /* A logical timeout does not abort a stuck DNS/TLS operation. It only
     * quarantines the worker. This experiment never launches a replacement. */
    if (result == PANEL_UPDATE_POLL_QUARANTINED) {
        if (!s_trial_completed) {
            ESP_LOGW(TAG, "one_shot_trial quarantine=1 elapsed_ms=%llu",
                (unsigned long long)(in->monotonic_ms - s_trial_started_ms));
            s_trial_completed = true;
        }
        s_trial_in_flight = false;
    } else if (result == PANEL_UPDATE_POLL_RESULT_OK ||
               result == PANEL_UPDATE_POLL_RESULT_ERROR) {
        /* Do not publish raw server content: a separate checked origin and
         * manifest-evaluation interface is needed before showing updates. */
        ESP_LOGI(TAG, "one_shot_trial result=%u bytes=%u generation=%" PRIu32,
                 (unsigned)result, (unsigned)size, generation);
        s_trial_in_flight = false;
        s_trial_completed = true;
    }
#endif
}
#endif
