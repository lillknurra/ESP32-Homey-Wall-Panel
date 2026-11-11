#include "panel_first_boot_evidence.h"

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static char s_json[2048];
static atomic_bool s_done;

static void *reader_thread(void *unused)
{
    (void)unused;
    while (!atomic_load(&s_done)) {
        panel_first_boot_evidence_t snapshot;
        panel_first_boot_evidence_copy(&snapshot);
        assert(snapshot.runtime_id == 0x12345678U);
        if (snapshot.inventory_state == PANEL_FIRST_BOOT_INVENTORY_FAILED) {
            assert(snapshot.inventory.sequence == 7U);
            assert(snapshot.inventory.final_error == 28684);
            assert(snapshot.inventory.stage == 3U);
            assert(!snapshot.inventory.snapshot_published);
            assert(snapshot.inventory.favorites_transport.request_sequence == 77U);
            assert(snapshot.inventory.favorites_transport.perform_error == 28674);
            assert(snapshot.inventory.zones_transport.request_sequence == 78U);
            assert(snapshot.inventory.zones_transport.socket_errno == 104);
        }
    }
    return NULL;
}

static void test_first_boot_failure_stays_immutable_after_recovery(void)
{
    panel_first_boot_evidence_init(0x12345678U);
    panel_first_boot_evidence_auth_started(25U);
    panel_first_boot_evidence_auth_result(31U, 0, true, true);
    panel_first_boot_evidence_scheduler_started(32U);
    panel_first_boot_evidence_queue_result(2);
    panel_first_boot_evidence_queue_result(0);

    panel_first_boot_evidence_inventory_started(2U, 1U, 6U, 40U);
    panel_first_boot_evidence_inventory_started(0U, 2U, 7U, 41U);
    panel_first_boot_evidence_inventory_started(0U, 1U, 7U, 42U);

    panel_first_boot_attempt_result_t first = {
        .sequence = 7U,
        .attempt = 1U,
        .started_ms = 42U,
        .completed_ms = 260U,
        .final_error = 28684,
        .final_http_status = 200,
        .stage = 3U,
        .verified = false,
        .snapshot_published = false,
        .favorites_attempted = true,
        .favorites_error = 0,
        .favorites_http_status = 200,
        .favorites_elapsed_ms = 30U,
        .devices_available = true,
        .devices_error = 28684,
        .devices_http_status = 200,
        .devices_expected_bytes = 313766,
        .devices_received_bytes = 50666U,
        .devices_elapsed_ms = 21618U,
        .devices_body_complete = false,
        .favorites_transport = {
            .request_sequence = 77U, .perform_error = 28674,
            .request_count = 1U, .captured_request = 1U,
            .failed_request = 1U, .available = true,
            .available_fields = PANEL_FIRST_BOOT_TRANSPORT_TLS,
            .tls_query = 32769,
        },
        .zones_transport = {
            .request_sequence = 78U, .perform_error = 28679,
            .request_count = 1U, .captured_request = 1U,
            .failed_request = 1U, .available = true,
            .available_fields = PANEL_FIRST_BOOT_TRANSPORT_SOCKET_ERRNO,
            .socket_errno = 104,
        },
    };
    panel_first_boot_evidence_inventory_completed(&first);

    pthread_t reader;
    atomic_store(&s_done, false);
    assert(pthread_create(&reader, NULL, reader_thread, NULL) == 0);
    panel_first_boot_evidence_recovery_used(2U, 1U);
    panel_first_boot_evidence_inventory_started(2U, 1U, 8U, 1000U);
    panel_first_boot_attempt_result_t later_success = {
        .sequence = 8U, .attempt = 1U, .started_ms = 1000U,
        .completed_ms = 1200U, .verified = true,
        .snapshot_published = true,
    };
    panel_first_boot_evidence_inventory_completed(&later_success);
    panel_first_boot_evidence_ready(2U, 1U, 1201U);
    atomic_store(&s_done, true);
    assert(pthread_join(reader, NULL) == 0);

    panel_first_boot_evidence_t copy;
    panel_first_boot_evidence_copy(&copy);
    assert(copy.inventory_state == PANEL_FIRST_BOOT_INVENTORY_FAILED);
    assert(copy.inventory.sequence == 7U && copy.inventory.attempt == 1U);
    assert(copy.inventory.final_error == 28684 && copy.inventory.stage == 3U);
    assert(copy.inventory.devices_expected_bytes == 313766);
    assert(copy.inventory.devices_received_bytes == 50666U);
    assert(copy.inventory.favorites_transport.request_sequence == 77U);
    assert(copy.inventory.favorites_transport.tls_query == 32769);
    assert(copy.inventory.zones_transport.request_sequence == 78U);
    assert(copy.inventory.zones_transport.socket_errno == 104);
    assert(!copy.inventory.devices_body_complete);
    assert(!copy.inventory.snapshot_published);
    assert(copy.recovery_used_before_first_ready);
    assert(copy.first_ready_reached && copy.first_ready_origin == 2U);
    assert(copy.first_ready_elapsed_ms == 1201U);
    assert(copy.auth_state == 2U && copy.scheduler_started);
    assert(copy.queue_attempts == 2U && copy.queue_first_result == 2 &&
           copy.queue_last_result == 0);
}

static void test_first_success_survives_later_failure(void)
{
    panel_first_boot_evidence_init(99U);
    panel_first_boot_evidence_inventory_started(0U, 1U, 3U, 10U);
    const panel_first_boot_attempt_result_t success = {
        .sequence = 3U, .attempt = 1U, .started_ms = 10U,
        .completed_ms = 20U, .verified = true,
        .snapshot_published = true, .stage = 4U,
    };
    panel_first_boot_evidence_inventory_completed(&success);
    panel_first_boot_evidence_ready(0U, 1U, 21U);
    panel_first_boot_evidence_inventory_started(2U, 1U, 4U, 100U);
    const panel_first_boot_attempt_result_t later_failure = {
        .sequence = 4U, .attempt = 1U, .started_ms = 100U,
        .completed_ms = 110U, .final_error = 28679,
        .stage = 2U, .verified = false,
    };
    panel_first_boot_evidence_inventory_completed(&later_failure);
    panel_first_boot_evidence_recovery_used(2U, 1U);
    panel_first_boot_evidence_t copy;
    panel_first_boot_evidence_copy(&copy);
    assert(copy.inventory_state == PANEL_FIRST_BOOT_INVENTORY_SUCCESS);
    assert(copy.inventory.sequence == 3U && copy.inventory.snapshot_published);
    assert(copy.first_ready_reached && copy.first_ready_origin == 0U);
    assert(!copy.recovery_used_before_first_ready);
}

static void test_stage_transport_is_correlated_and_immutable(void)
{
    panel_first_boot_evidence_init(123U);
    panel_first_boot_evidence_inventory_started(0U, 1U, 51U, 100U);
    panel_first_boot_transport_t favorites = {
        .request_sequence = 401U, .perform_error = 28674,
        .http_status = -1, .tls_query = 32769, .tls_error = 0,
        .tls_flags = 0, .socket_errno = 11, .connect_result = -1,
        .elapsed_ms = 8181U,
        .available_fields = PANEL_FIRST_BOOT_TRANSPORT_TLS |
            PANEL_FIRST_BOOT_TRANSPORT_SOCKET_ERRNO |
            PANEL_FIRST_BOOT_TRANSPORT_CONNECT_RESULT |
            PANEL_FIRST_BOOT_TRANSPORT_ELAPSED,
        .classification = 2U, .request_count = 2U,
        .captured_request = 1U, .failed_request = 1U, .available = true,
    };
    panel_first_boot_transport_t zones = {
        .request_sequence = 402U, .perform_error = 28679,
        .http_status = -1, .tls_query = 0, .tls_error = 0,
        .tls_flags = 0, .socket_errno = 104, .elapsed_ms = 392U,
        .available_fields = PANEL_FIRST_BOOT_TRANSPORT_TLS |
            PANEL_FIRST_BOOT_TRANSPORT_SOCKET_ERRNO |
            PANEL_FIRST_BOOT_TRANSPORT_ELAPSED,
        .classification = 2U, .request_count = 1U,
        .captured_request = 1U, .failed_request = 1U, .available = true,
    };
    const panel_first_boot_attempt_result_t first = {
        .sequence = 51U, .origin = 0U, .attempt = 1U,
        .completed_ms = 500U, .final_error = 28674,
        .final_http_status = -1, .stage = 2U, .verified = false,
        .favorites_attempted = true, .favorites_transport = favorites,
        .zones_transport = zones,
    };
    panel_first_boot_evidence_inventory_completed(&first);

    panel_first_boot_evidence_recovery_used(2U, 2U);
    panel_first_boot_evidence_inventory_started(2U, 2U, 53U, 700U);
    const panel_first_boot_attempt_result_t recovery = {
        .sequence = 53U, .origin = 2U, .attempt = 2U,
        .completed_ms = 900U, .verified = true,
        .snapshot_published = true,
    };
    panel_first_boot_evidence_inventory_completed(&recovery);

    panel_first_boot_evidence_t copy;
    panel_first_boot_evidence_copy(&copy);
    assert(copy.inventory_state == PANEL_FIRST_BOOT_INVENTORY_FAILED);
    assert(copy.inventory.sequence == 51U);
    assert(copy.inventory.favorites_transport.request_sequence == 401U);
    assert(copy.inventory.favorites_transport.request_count == 2U);
    assert(copy.inventory.favorites_transport.captured_request == 1U);
    assert(copy.inventory.favorites_transport.perform_error == 28674);
    assert(copy.inventory.favorites_transport.tls_query == 32769);
    assert(copy.inventory.zones_transport.request_sequence == 402U);
    assert(copy.inventory.zones_transport.perform_error == 28679);
    assert(copy.inventory.zones_transport.socket_errno == 104);
    assert(!copy.inventory.snapshot_published);

    char output[PANEL_FIRST_BOOT_EVIDENCE_JSON_MAX] = "{}";
    size_t length = 2U;
    assert(panel_first_boot_evidence_append_json(output, sizeof(output), &length));
    output[length] = '\0';
    printf("PATCH117M_TRANSPORT_JSON=%s\n", output);
}

static void test_attempt_identity_mismatch_is_rejected(void)
{
    panel_first_boot_evidence_init(125U);
    panel_first_boot_evidence_inventory_started(0U, 1U, 71U, 1U);
    const panel_first_boot_attempt_result_t mismatch = {
        .sequence = 71U, .origin = 2U, .attempt = 1U,
        .completed_ms = 2U, .final_error = 28674, .stage = 2U,
        .verified = false,
    };
    panel_first_boot_evidence_inventory_completed(&mismatch);
    panel_first_boot_evidence_t copy;
    panel_first_boot_evidence_copy(&copy);
    assert(copy.inventory_state == PANEL_FIRST_BOOT_INVENTORY_IN_PROGRESS);
    assert(copy.inventory.sequence == 71U);
    const panel_first_boot_attempt_result_t correct = {
        .sequence = 71U, .origin = 0U, .attempt = 1U,
        .completed_ms = 3U, .final_error = 28674, .stage = 2U,
        .verified = false,
    };
    panel_first_boot_evidence_inventory_completed(&correct);
    panel_first_boot_evidence_copy(&copy);
    assert(copy.inventory_state == PANEL_FIRST_BOOT_INVENTORY_FAILED);
}

static void test_transport_metadata_unavailable_is_explicit(void)
{
    panel_first_boot_evidence_init(124U);
    panel_first_boot_evidence_inventory_started(0U, 1U, 61U, 1U);
    const panel_first_boot_attempt_result_t first = {
        .sequence = 61U, .origin = 0U, .attempt = 1U,
        .completed_ms = 2U, .final_error = 28674, .stage = 2U,
        .verified = false,
        .favorites_transport = {
            .request_sequence = 501U, .perform_error = 28674,
            .request_count = 1U, .captured_request = 1U,
            .failed_request = 1U, .available = true,
            .available_fields = PANEL_FIRST_BOOT_TRANSPORT_SOCKET_ERRNO,
            .socket_errno = 11,
        },
    };
    panel_first_boot_evidence_inventory_completed(&first);
    panel_first_boot_evidence_t copy;
    panel_first_boot_evidence_copy(&copy);
    assert(copy.inventory.favorites_transport.available);
    assert((copy.inventory.favorites_transport.available_fields &
            PANEL_FIRST_BOOT_TRANSPORT_TLS) == 0U);
    assert(copy.inventory.zones_transport.available == false);
    char output[PANEL_FIRST_BOOT_EVIDENCE_JSON_MAX] = "{}";
    size_t length = 2U;
    assert(panel_first_boot_evidence_append_json(output, sizeof(output), &length));
    output[length] = '\0';
    printf("PATCH117M_UNAVAILABLE_JSON=%s\n", output);

    panel_first_boot_evidence_init(126U);
    panel_first_boot_evidence_inventory_started(0U, 1U, 62U, 1U);
    panel_first_boot_attempt_result_t bad_correlation = first;
    bad_correlation.sequence = 62U;
    bad_correlation.favorites_transport.request_sequence = 0U;
    panel_first_boot_evidence_inventory_completed(&bad_correlation);
    panel_first_boot_evidence_copy(&copy);
    assert(!copy.inventory.favorites_transport.available);
}

static void test_first_failure_stages_remain_distinguishable(void)
{
    const struct {
        uint32_t sequence;
        uint8_t stage;
        int32_t error;
        int32_t http_status;
        bool favorites_attempted;
        uint32_t devices_bytes;
    } cases[] = {
        {11U, 1U, 28674, -1, true, 0U},  /* Favorites transport/connect failure. */
        {12U, 2U, 28679, 200, true, 0U}, /* Zones EAGAIN after response. */
        {13U, 3U, 28684, 200, true, 50666U}, /* Partial Devices body. */
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        panel_first_boot_evidence_init((uint32_t)(100U + i));
        panel_first_boot_evidence_inventory_started(0U, 1U,
            cases[i].sequence, 1000U);
        panel_first_boot_attempt_result_t result = {
            .sequence = cases[i].sequence,
            .attempt = 1U,
            .started_ms = 1000U,
            .completed_ms = 1200U,
            .final_error = cases[i].error,
            .final_http_status = cases[i].http_status,
            .stage = cases[i].stage,
            .verified = false,
            .favorites_attempted = cases[i].favorites_attempted,
            .favorites_error = cases[i].stage == 1U ? cases[i].error : 0,
            .favorites_http_status = cases[i].stage == 1U ? cases[i].http_status : -1,
            .devices_available = cases[i].stage == 3U,
            .devices_error = cases[i].stage == 3U ? cases[i].error : 0,
            .devices_http_status = cases[i].stage == 3U ? 200 : -1,
            .devices_expected_bytes = cases[i].stage == 3U ? 313766 : -1,
            .devices_received_bytes = cases[i].devices_bytes,
            .devices_body_complete = false,
        };
        panel_first_boot_evidence_inventory_completed(&result);
        panel_first_boot_evidence_t copy;
        panel_first_boot_evidence_copy(&copy);
        assert(copy.inventory_state == PANEL_FIRST_BOOT_INVENTORY_FAILED);
        assert(copy.inventory.stage == cases[i].stage);
        assert(copy.inventory.final_error == cases[i].error);
        assert(copy.inventory.favorites_attempted);
        if (cases[i].stage == 3U) {
            assert(copy.inventory.devices_expected_bytes == 313766);
            assert(copy.inventory.devices_received_bytes == 50666U);
            assert(!copy.inventory.devices_body_complete);
        }
    }
}

static void test_never_started_and_abandoned_attempt_are_visible(void)
{
    panel_first_boot_evidence_init(1U);
    panel_first_boot_evidence_scheduler_skipped(4U);
    panel_first_boot_evidence_t copy;
    panel_first_boot_evidence_copy(&copy);
    assert(copy.inventory_state == PANEL_FIRST_BOOT_INVENTORY_NOT_STARTED);
    assert(copy.scheduler_skip_reason == 4U);
    assert(!copy.scheduler_started && !copy.first_ready_reached);

    panel_first_boot_evidence_init(2U);
    panel_first_boot_evidence_inventory_started(0U, 1U, 9U, 500U);
    panel_first_boot_evidence_copy(&copy);
    assert(copy.inventory_state == PANEL_FIRST_BOOT_INVENTORY_IN_PROGRESS);
    assert(copy.inventory.sequence == 9U);
}

static void test_same_runtime_init_is_idempotent(void)
{
    panel_first_boot_evidence_init(77U);
    panel_first_boot_evidence_scheduler_started(15U);
    panel_first_boot_evidence_queue_result(0);
    panel_first_boot_evidence_init(77U);
    panel_first_boot_evidence_t copy;
    panel_first_boot_evidence_copy(&copy);
    assert(copy.runtime_id == 77U);
    assert(copy.scheduler_started && copy.scheduler_started_ms == 15U);
    assert(copy.queue_attempts == 1U && copy.queue_first_result == 0);
    panel_first_boot_evidence_init(78U);
    panel_first_boot_evidence_copy(&copy);
    assert(copy.runtime_id == 78U);
    assert(!copy.scheduler_started && copy.queue_attempts == 0U);
}

static void test_json_is_additive_bounded_and_secret_free(void)
{
    panel_first_boot_evidence_init(0xAABBCCDDU);
    char empty_object[PANEL_FIRST_BOOT_EVIDENCE_JSON_MAX] = "{}";
    size_t empty_length = 2U;
    assert(panel_first_boot_evidence_append_json(
        empty_object, sizeof(empty_object), &empty_length));
    empty_object[empty_length] = '\0';
    printf("PATCH117L_EMPTY_JSON=%s\n", empty_object);

    strcpy(s_json, "{\"state\":\"error\"}");
    size_t length = strlen(s_json);
    assert(panel_first_boot_evidence_append_json(s_json, sizeof(s_json), &length));
    s_json[length] = '\0';
    assert(strstr(s_json, "\"state\":\"error\"") != NULL);
    assert(strstr(s_json, "\"first_boot_evidence\"") != NULL);
    assert(strstr(s_json, "token") == NULL);
    assert(strstr(s_json, "remote_url") == NULL);
    assert(strstr(s_json, "favorites_transport") != NULL);
    assert(strstr(s_json, "zones_transport") != NULL);
    printf("PATCH117L_EXISTING_JSON=%s\n", s_json);
    char tiny[16] = "{}";
    size_t tiny_length = 2U;
    assert(!panel_first_boot_evidence_append_json(tiny, sizeof(tiny), &tiny_length));
    puts(s_json);
}

static void test_worst_case_json_fits_endpoint_bound(void)
{
    panel_first_boot_evidence_init(UINT32_MAX);
    panel_first_boot_evidence_auth_started(UINT32_MAX);
    panel_first_boot_evidence_auth_result(UINT32_MAX, INT32_MIN, true, true);
    panel_first_boot_evidence_scheduler_started(UINT32_MAX);
    panel_first_boot_evidence_scheduler_failed(UINT8_MAX);
    panel_first_boot_evidence_queue_result(INT8_MIN);
    panel_first_boot_evidence_inventory_started(0U, 1U,
                                                UINT32_MAX, UINT32_MAX);
    const panel_first_boot_attempt_result_t result = {
        .sequence = UINT32_MAX, .origin = 0U, .attempt = 1U,
        .started_ms = UINT32_MAX, .completed_ms = UINT32_MAX,
        .final_error = INT32_MIN, .final_http_status = INT32_MAX,
        .stage = UINT8_MAX, .verified = false, .snapshot_published = true,
        .favorites_attempted = true, .favorites_error = INT32_MIN,
        .favorites_http_status = INT32_MAX, .favorites_elapsed_ms = UINT32_MAX,
        .devices_available = true, .devices_error = INT32_MIN,
        .devices_http_status = INT32_MAX, .devices_expected_bytes = INT32_MAX,
        .devices_received_bytes = UINT32_MAX, .devices_elapsed_ms = UINT32_MAX,
        .devices_body_complete = true,
        .favorites_transport = {
            .request_sequence = UINT32_MAX, .perform_error = INT32_MIN,
            .http_status = INT32_MAX, .tls_query = INT32_MIN,
            .tls_error = INT32_MIN, .tls_flags = INT32_MIN,
            .socket_errno = INT32_MIN, .connect_result = INT32_MIN,
            .elapsed_ms = UINT32_MAX, .available_fields = UINT8_MAX,
            .classification = UINT8_MAX, .request_count = UINT8_MAX,
            .captured_request = UINT8_MAX, .failed_request = UINT8_MAX,
            .available = true,
        },
        .zones_transport = {
            .request_sequence = UINT32_MAX, .perform_error = INT32_MIN,
            .http_status = INT32_MAX, .tls_query = INT32_MIN,
            .tls_error = INT32_MIN, .tls_flags = INT32_MIN,
            .socket_errno = INT32_MIN, .connect_result = INT32_MIN,
            .elapsed_ms = UINT32_MAX, .available_fields = UINT8_MAX,
            .classification = UINT8_MAX, .request_count = UINT8_MAX,
            .captured_request = UINT8_MAX, .failed_request = UINT8_MAX,
            .available = true,
        },
    };
    panel_first_boot_evidence_inventory_completed(&result);
    panel_first_boot_evidence_recovery_used(2U, UINT32_MAX);
    panel_first_boot_evidence_ready(2U, UINT32_MAX, UINT32_MAX);
    char bounded[PANEL_FIRST_BOOT_EVIDENCE_JSON_MAX] = "{}";
    size_t length = 2U;
    assert(panel_first_boot_evidence_append_json(bounded, sizeof(bounded), &length));
    bounded[length] = '\0';
    assert(length < sizeof(bounded));
    assert(sizeof(panel_first_boot_evidence_t) <= 256U);
    puts(bounded);
}

int main(void)
{
    printf("PATCH117M_STATIC_EVIDENCE_BYTES=%zu\n",
           sizeof(panel_first_boot_evidence_t));
    test_first_boot_failure_stays_immutable_after_recovery();
    test_first_success_survives_later_failure();
    test_stage_transport_is_correlated_and_immutable();
    test_attempt_identity_mismatch_is_rejected();
    test_transport_metadata_unavailable_is_explicit();
    test_first_failure_stages_remain_distinguishable();
    test_never_started_and_abandoned_attempt_are_visible();
    test_same_runtime_init_is_idempotent();
    test_json_is_additive_bounded_and_secret_free();
    test_worst_case_json_fits_endpoint_bound();
    puts("PANEL_FIRST_BOOT_EVIDENCE_HOST_TESTS PASS");
    return 0;
}
