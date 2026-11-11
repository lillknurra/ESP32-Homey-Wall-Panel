#include "panel_first_failure_diag.h"
#include "athom_devices_http_diag.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int32_t esp_err_t;
#define ESP_OK 0
typedef enum { ATHOM_REFRESH_ORIGIN_BOOT_AUTO = 0,
    ATHOM_REFRESH_ORIGIN_MANUAL, ATHOM_REFRESH_ORIGIN_PERIODIC,
    ATHOM_REFRESH_ORIGIN_LIGHT_RECONCILIATION,
    ATHOM_REFRESH_ORIGIN_PRESELECTION_RESTORE } athom_refresh_origin_t;
typedef enum { ATHOM_INVENTORY_STAGE_UNKNOWN = 0,
    ATHOM_INVENTORY_STAGE_FAVORITES = 6, ATHOM_INVENTORY_STAGE_ZONES = 7,
    ATHOM_INVENTORY_STAGE_DEVICES = 8 } athom_inventory_attempt_stage_t;
typedef struct { uint32_t perform_count; } athom_transport_metrics_t;
typedef struct { bool valid; uint32_t connect_calls; int32_t connect_result; }
    athom_favorites_fetch_diagnostic_t;
typedef struct { bool attempted; athom_favorites_fetch_diagnostic_t fetch; }
    athom_favorites_read_diagnostic_t;
typedef struct {
    bool valid; uint32_t sequence; athom_refresh_origin_t origin; uint32_t attempt;
    athom_inventory_attempt_stage_t stage; bool raw_transport_observed;
    uint32_t raw_request_sequence, raw_request_elapsed_ms;
    int32_t raw_perform_error, raw_http_status, raw_tls_query, raw_tls_error,
        raw_socket_errno;
    athom_favorites_read_diagnostic_t favorites_read;
} athom_inventory_attempt_diagnostic_t;

#define MALLOC_CAP_INTERNAL 1U
#define MALLOC_CAP_8BIT 2U
#define MALLOC_CAP_SPIRAM 4U

static uint32_t s_runtime_id;
static athom_inventory_attempt_diagnostic_t s_mock_diagnostic;
static athom_transport_metrics_t s_mock_after;
static athom_devices_http_diag_summary_t s_mock_devices;
static uint64_t s_now_us;

static void athom_inventory_attempt_diagnostic_copy(
    athom_inventory_attempt_diagnostic_t *out) { *out = s_mock_diagnostic; }
static void athom_cloud_transport_metrics_copy(athom_transport_metrics_t *out)
    { *out = s_mock_after; }
bool athom_devices_http_diag_copy_summary(
    athom_devices_http_diag_summary_t *out) { *out = s_mock_devices; return out->valid; }
static uint32_t heap_caps_get_free_size(uint32_t caps)
    { return caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) ? 18000U : 7000000U; }
static uint32_t heap_caps_get_largest_free_block(uint32_t caps)
    { return caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) ? 15000U : 6800000U; }
static uint32_t heap_caps_get_minimum_free_size(uint32_t caps)
    { (void)caps; return 2500U; }
static uint64_t esp_timer_get_time(void) { return s_now_us; }

/* PATCH119P4N12_PRODUCTION_FUNCTIONS */

int main(void)
{
    panel_first_failure_diag_init(99U);
    s_runtime_id = 99U;
    s_now_us = 50000000U;
    s_mock_diagnostic = (athom_inventory_attempt_diagnostic_t){
        .valid = true, .sequence = 12U, .origin = ATHOM_REFRESH_ORIGIN_PERIODIC,
        .attempt = 1U, .stage = ATHOM_INVENTORY_STAGE_DEVICES,
        .raw_transport_observed = true, .raw_request_sequence = 45U,
        .raw_request_elapsed_ms = 37326U, .raw_perform_error = 28684,
        .raw_http_status = 200, .raw_socket_errno = 11,
    };
    s_mock_after.perform_count = 46U;
    s_mock_devices = (athom_devices_http_diag_summary_t){
        .valid = true, .request_sequence = 45U, .connect_calls = 1U,
        .started_us = 1000000U, .finished_us = 40000000U,
        .last_positive_body_read_us = 42192000U, .terminal_read_us = 8000000U,
        .content_length = 314680, .body_bytes = 108767U, .http_status = 200,
        .perform_error = 28684, .terminal_result = 0, .terminal_errno = 11,
        .connect_result = 0, .headers_complete = true, .terminal_seen = true,
        .body_complete = false,
    };
    athom_transport_metrics_t before = {.perform_count = 40U};
    panel_first_failure_rgb_t rgb = {0};
    first_failure_capture_attempt(ATHOM_REFRESH_ORIGIN_PERIODIC, 1U, 12U,
        900000U, 28684, 200, "inventory_devices", &before, &rgb, false);
    panel_first_failure_diag_t captured;
    panel_first_failure_diag_copy(&captured);
    assert(captured.valid && captured.runtime_id == 99U);
    assert(captured.inventory_sequence == 12U && captured.stage == 3U);
    assert(captured.request_sequence == 45U && captured.request_error == 28684);
    assert(captured.http_status == 200 && captured.connect_result == 0);
    assert(captured.content_length == 314680 && captured.received_bytes == 108767U);
    assert(captured.last_positive_read_us == 42192000U);
    assert(captured.terminal_available && captured.terminal_result == 0 &&
           captured.terminal_errno == 11 && captured.terminal_read_us == 8000000U);
    assert(captured.memory_available && captured.internal_minimum == 2500U);
    assert(!captured.close_correlation_complete && !captured.rgb_available);

    /* Later periodic failures and overwriteable close events do not replace it. */
    s_mock_diagnostic.sequence = 13U;
    s_mock_diagnostic.stage = ATHOM_INVENTORY_STAGE_ZONES;
    s_mock_diagnostic.raw_request_sequence = 50U;
    s_mock_diagnostic.raw_perform_error = 28679;
    s_mock_devices.request_sequence = 0U;
    first_failure_capture_attempt(ATHOM_REFRESH_ORIGIN_PERIODIC, 2U, 13U,
        60000000U, 28679, 0, "inventory_zones", &before, &rgb, false);
    panel_first_failure_diag_copy(&captured);
    assert(captured.inventory_sequence == 12U && captured.stage == 3U);
    puts("PRODUCTION_FIRST_FAILURE_CAPTURE=PASS");
    return 0;
}
