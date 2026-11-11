#include "panel_first_failure_diag.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static void *copy_stress(void *unused)
{
    (void)unused;
    for (unsigned i = 0U; i < 10000U; ++i) {
        panel_first_failure_diag_t snapshot;
        panel_first_failure_diag_copy(&snapshot);
        if (snapshot.valid) {
            assert(snapshot.inventory_sequence == 2U);
            assert(snapshot.runtime_id == 42U);
            assert(snapshot.final_error == -28684);
        }
    }
    return NULL;
}

static panel_first_failure_diag_t failure(uint32_t runtime, uint32_t seq,
                                          uint8_t stage, int error)
{
    return (panel_first_failure_diag_t){
        .valid = true, .runtime_id = runtime, .inventory_sequence = seq,
        .attempt = 1U, .origin = 2U, .stage = stage,
        .started_us = 1000U + seq, .completed_us = 2000U + seq,
        .final_error = error, .http_status = 200,
        .request_available = true, .request_sequence = seq + 10U,
        .request_error = error, .request_http_status = 200,
        .device_body_available = stage == 3U,
        .headers_complete = stage == 3U, .body_complete = false,
        .content_length = stage == 3U ? 314680 : -1,
        .received_bytes = stage == 3U ? 108767U : 0U,
        .last_positive_read_us = stage == 3U ? 42000000U : 0U,
        .terminal_available = stage == 3U,
        .terminal_result = 0, .terminal_errno = 11,
        .terminal_read_us = stage == 3U ? 8000000U : 0U,
        .memory_available = true, .internal_free = 19995U,
        .internal_largest = 16384U, .internal_minimum = 2523U,
        .psram_free = 7600000U, .psram_largest = 7400000U,
        .rgb_available = true, .rgb_shortfall_delta = 2U,
        .rgb_restart_decision_delta = 2U, .rgb_restart_path_delta = 1U,
        .close_correlation_complete = false,
    };
}

int main(void)
{
    panel_first_failure_diag_t out;
    panel_first_failure_diag_init(42U);

    /* Successful inventory is not a failure and cannot occupy the latch. */
    panel_first_failure_diag_t success = failure(42U, 1U, 3U, 0);
    panel_first_failure_diag_capture(&success);
    panel_first_failure_diag_copy(&out);
    assert(!out.valid);

    /* First failure can be any stage and is immutable across later outcomes. */
    panel_first_failure_diag_t first = failure(42U, 2U, 3U, -28684);
    panel_first_failure_diag_capture(&first);
    panel_first_failure_diag_t later = failure(42U, 9U, 2U, -1);
    panel_first_failure_diag_capture(&later);
    pthread_t reader;
    assert(pthread_create(&reader, NULL, copy_stress, NULL) == 0);
    panel_first_failure_diag_capture(&later);
    assert(pthread_join(reader, NULL) == 0);
    panel_first_failure_diag_copy(&out);
    assert(out.valid && out.inventory_sequence == 2U && out.stage == 3U);
    assert(out.received_bytes == 108767U && out.terminal_errno == 11);
    assert(!out.close_correlation_complete);

    char json[1024] = "{}";
    size_t length = 2U;
    assert(panel_first_failure_diag_append_json(json, sizeof(json), &length));
    assert(length == strlen(json) && json[length] == '\0');
    puts(json);

    /* New runtime resets RAM snapshot; same-runtime init does not. */
    panel_first_failure_diag_init(42U);
    panel_first_failure_diag_copy(&out);
    assert(out.inventory_sequence == 2U);
    panel_first_failure_diag_init(43U);
    panel_first_failure_diag_copy(&out);
    assert(!out.valid);

    /* Missing close correlation cannot be promoted by a later event/result. */
    panel_first_failure_diag_t no_close = failure(43U, 3U, 1U, -28674);
    no_close.close_correlation_complete = false;
    panel_first_failure_diag_capture(&no_close);
    panel_first_failure_diag_copy(&out);
    assert(out.valid && !out.close_correlation_complete);

    /* Favorites and Zones failures are retained even without Devices fields. */
    panel_first_failure_diag_init(44U);
    panel_first_failure_diag_t favorites = failure(44U, 4U, 1U, -28674);
    favorites.device_body_available = false;
    favorites.connect_result_available = true;
    favorites.connect_result = -1;
    panel_first_failure_diag_capture(&favorites);
    panel_first_failure_diag_t later_devices = failure(44U, 5U, 3U, -28679);
    panel_first_failure_diag_capture(&later_devices);
    panel_first_failure_diag_copy(&out);
    assert(out.inventory_sequence == 4U && out.stage == 1U);
    assert(out.connect_result_available && out.connect_result == -1);
    assert(!out.device_body_available);

    panel_first_failure_diag_init(45U);
    panel_first_failure_diag_t zones = failure(45U, 6U, 2U, -28679);
    zones.http_status = 0;
    zones.request_http_status = 0;
    zones.device_body_available = false;
    zones.connect_result_available = false;
    panel_first_failure_diag_capture(&zones);
    panel_first_failure_diag_copy(&out);
    assert(out.stage == 2U && out.final_error == -28679);
    assert(out.http_status == 0 && !out.connect_result_available);

    char tiny[8] = "{}";
    size_t tiny_length = 2U;
    assert(!panel_first_failure_diag_append_json(tiny, sizeof(tiny),
                                                  &tiny_length));
    return 0;
}
