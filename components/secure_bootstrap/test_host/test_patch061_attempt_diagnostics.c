#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdatomic.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_HTTP_CONNECT 0x7002

typedef enum {
    ATHOM_TRANSPORT_OK = 0,
    ATHOM_TRANSPORT_DNS_FAIL,
    ATHOM_TRANSPORT_TCP_CONNECT_FAIL,
    ATHOM_TRANSPORT_TLS_FAIL,
    ATHOM_TRANSPORT_HTTP_TIMEOUT,
    ATHOM_TRANSPORT_HTTP_401,
    ATHOM_TRANSPORT_HTTP_403,
    ATHOM_TRANSPORT_HTTP_408,
    ATHOM_TRANSPORT_HTTP_429,
    ATHOM_TRANSPORT_HTTP_5XX,
    ATHOM_TRANSPORT_HOMEY_SESSION_FAIL,
    ATHOM_TRANSPORT_FAVORITES_FAIL,
    ATHOM_TRANSPORT_ZONES_FAIL,
    ATHOM_TRANSPORT_DEVICES_FAIL,
    ATHOM_TRANSPORT_PARSE_FAIL,
    ATHOM_TRANSPORT_NO_VALID_ENDPOINT,
} athom_transport_class_t;

typedef enum {
    ATHOM_TRANSPORT_ROLE_NONE = 0,
    ATHOM_TRANSPORT_ROLE_CLOUD,
    ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE,
} athom_transport_role_t;

typedef struct {
    bool capture_attempted;
    bool hook_registered;
    uint32_t matching_failure_count;
    uint32_t first_requested_size;
    uint32_t last_requested_size;
    uint32_t max_requested_size;
    uint32_t failure_caps;
    bool all_heap_caps_calloc;
    uint32_t internal_8bit_free_before;
    uint32_t internal_8bit_largest_before;
    uint32_t internal_8bit_minimum_before;
    uint32_t internal_8bit_free_at_failure;
    uint32_t internal_8bit_largest_at_failure;
    uint32_t internal_8bit_minimum_at_failure;
    uint32_t internal_8bit_free_after;
    uint32_t internal_8bit_largest_after;
    uint32_t internal_8bit_minimum_after;
} athom_tls_memory_diagnostic_t;

typedef struct {
    uint32_t cloud_client_init_count;
    uint32_t cloud_client_reuse_count;
    uint32_t cloud_client_cleanup_count;
    uint32_t homey_client_init_count;
    uint32_t homey_client_reuse_count;
    uint32_t homey_client_cleanup_count;
    uint32_t cloud_request_count;
    uint32_t homey_request_count;
    uint32_t homey_session_create_count;
    uint32_t remote_rebind_count;
    uint32_t perform_count;
    uint32_t last_request_elapsed_ms;
    athom_transport_class_t last_classification;
    athom_transport_role_t last_perform_role;
    athom_transport_class_t last_perform_classification;
    int last_perform_http_status;
    bool last_connected_event_seen;
    bool last_error_event_seen;
    bool last_disconnected_event_seen;
    int last_http_status;
    int last_tls_error;
    int last_tls_flags;
    int last_socket_errno;
    esp_err_t last_perform_err;
    esp_err_t last_tls_query;
    athom_tls_memory_diagnostic_t last_tls_memory_diagnostic;
    athom_pre_tls_diagnostic_t last_pre_tls_diagnostic;
} athom_transport_metrics_t;

typedef enum {
    ATHOM_REFRESH_ORIGIN_BOOT_AUTO = 0,
    ATHOM_REFRESH_ORIGIN_MANUAL,
    ATHOM_REFRESH_ORIGIN_PERIODIC,
    ATHOM_REFRESH_ORIGIN_LIGHT_RECONCILIATION,
    ATHOM_REFRESH_ORIGIN_PRESELECTION_RESTORE,
} athom_refresh_origin_t;

typedef pthread_mutex_t portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define portENTER_CRITICAL(mux) assert(pthread_mutex_lock(mux) == 0)
#define portEXIT_CRITICAL(mux) assert(pthread_mutex_unlock(mux) == 0)

static const char *athom_cloud_transport_class_name(athom_transport_class_t value)
{
    switch (value) {
    case ATHOM_TRANSPORT_OK: return "OK";
    case ATHOM_TRANSPORT_DNS_FAIL: return "DNS_FAIL";
    case ATHOM_TRANSPORT_TCP_CONNECT_FAIL: return "TCP_CONNECT_FAIL";
    case ATHOM_TRANSPORT_TLS_FAIL: return "TLS_FAIL";
    case ATHOM_TRANSPORT_HTTP_TIMEOUT: return "HTTP_TIMEOUT";
    case ATHOM_TRANSPORT_HTTP_401: return "HTTP_401";
    case ATHOM_TRANSPORT_HTTP_403: return "HTTP_403";
    case ATHOM_TRANSPORT_HTTP_408: return "HTTP_408";
    case ATHOM_TRANSPORT_HTTP_429: return "HTTP_429";
    case ATHOM_TRANSPORT_HTTP_5XX: return "HTTP_5XX";
    case ATHOM_TRANSPORT_HOMEY_SESSION_FAIL: return "HOMEY_SESSION_FAIL";
    case ATHOM_TRANSPORT_FAVORITES_FAIL: return "FAVORITES_FAIL";
    case ATHOM_TRANSPORT_ZONES_FAIL: return "ZONES_FAIL";
    case ATHOM_TRANSPORT_DEVICES_FAIL: return "DEVICES_FAIL";
    case ATHOM_TRANSPORT_PARSE_FAIL: return "PARSE_FAIL";
    case ATHOM_TRANSPORT_NO_VALID_ENDPOINT: return "NO_VALID_ENDPOINT";
    default: return "UNKNOWN";
    }
}

static athom_transport_metrics_t s_live_metrics;
static uint32_t s_live_revision;
static int64_t s_test_time_us;

static void athom_cloud_transport_metrics_copy(athom_transport_metrics_t *out)
{
    if (out != NULL) *out = s_live_metrics;
}

static uint32_t athom_cloud_diagnostic_revision(void)
{
    return s_live_revision;
}

static int64_t esp_timer_get_time(void)
{
    return s_test_time_us;
}

static int s_cloud;
static bool s_fixture_refresh, s_fixture_perform = true;
static esp_err_t s_fixture_result = ESP_ERR_HTTP_CONNECT;
static int s_fixture_status;
static const char *s_fixture_stage = "oauth_user_me_request";
static int athom_cloud_diagnostic_http_status(void) { return s_fixture_status; }
static const char *athom_cloud_diagnostic_stage(void) { return s_fixture_stage; }
static esp_err_t fixture_transport(void *state, bool refresh)
{
    assert(state == &s_cloud && refresh == s_fixture_refresh);
    if (s_fixture_perform) {
        s_live_metrics.perform_count++;
        s_live_metrics.cloud_request_count++;
        s_live_metrics.last_perform_role = ATHOM_TRANSPORT_ROLE_CLOUD;
        s_live_metrics.last_perform_err = s_fixture_result;
        s_live_metrics.last_perform_classification = s_fixture_result == ESP_OK ? ATHOM_TRANSPORT_OK
            : (refresh ? ATHOM_TRANSPORT_TLS_FAIL : ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
        s_live_metrics.last_perform_http_status = s_fixture_status;
        s_live_metrics.last_connected_event_seen = refresh;
        s_live_metrics.last_error_event_seen = true;
        s_live_metrics.last_disconnected_event_seen = refresh;
        s_live_metrics.last_tls_memory_diagnostic = (athom_tls_memory_diagnostic_t){
            .capture_attempted = true,
            .hook_registered = true,
            .matching_failure_count = refresh ? 0U : 1U,
            .first_requested_size = refresh ? 0U : 2048U,
            .last_requested_size = refresh ? 0U : 2048U,
            .max_requested_size = refresh ? 0U : 2048U,
            .failure_caps = refresh ? 0U : 25U,
            .all_heap_caps_calloc = !refresh,
            .internal_8bit_free_before = 50000U,
            .internal_8bit_largest_before = 30000U,
            .internal_8bit_minimum_before = 40000U,
            .internal_8bit_free_at_failure = refresh ? 0U : 12000U,
            .internal_8bit_largest_at_failure = refresh ? 0U : 1024U,
            .internal_8bit_minimum_at_failure = refresh ? 0U : 10000U,
            .internal_8bit_free_after = 49000U,
            .internal_8bit_largest_after = 29000U,
            .internal_8bit_minimum_after = 40000U,
        };
        s_live_revision++;
    }
    return s_fixture_result;
}
static esp_err_t athom_cloud_refresh(void *state) { return fixture_transport(state, true); }
static esp_err_t athom_cloud_fetch_user_homeys(void *state) { return fixture_transport(state, false); }

/* PATCH061_PRODUCTION_DIAGNOSTICS */

static atomic_int s_writer_done;

static athom_inventory_attempt_diagnostic_t sample_attempt(bool second)
{
    athom_inventory_attempt_diagnostic_t diagnostic = {0};
    diagnostic.valid = true;
    diagnostic.origin = second ? ATHOM_REFRESH_ORIGIN_PERIODIC
                               : ATHOM_REFRESH_ORIGIN_BOOT_AUTO;
    diagnostic.attempt = second ? 2U : 1U;
    diagnostic.completed_at_ms = second ? 202U : 101U;
    diagnostic.final_error = second ? 77 : 55;
    diagnostic.final_http_status = second ? 503 : 0;
    diagnostic.stage = second ? ATHOM_INVENTORY_STAGE_DEVICES
                               : ATHOM_INVENTORY_STAGE_CLOUD_USER_DISCOVERY;
    diagnostic.raw_transport_observed = true;
    diagnostic.raw_role = second ? ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE
                                 : ATHOM_TRANSPORT_ROLE_CLOUD;
    diagnostic.raw_perform_error = second ? 77 : ESP_ERR_HTTP_CONNECT;
    diagnostic.raw_classification = second ? ATHOM_TRANSPORT_HTTP_5XX
                                           : ATHOM_TRANSPORT_TCP_CONNECT_FAIL;
    diagnostic.raw_http_status = second ? 503 : 0;
    diagnostic.raw_tls_query = second ? 12 : 0;
    diagnostic.raw_tls_error = second ? 13 : 0;
    diagnostic.raw_tls_flags = second ? 14 : 0;
    diagnostic.raw_socket_errno = second ? 15 : 0;
    diagnostic.raw_request_elapsed_ms = second ? 2020U : 1010U;
    diagnostic.raw_connected_event_seen = second;
    diagnostic.raw_error_event_seen = !second;
    diagnostic.raw_disconnected_event_seen = second;
    diagnostic.raw_tls_memory_diagnostic = (athom_tls_memory_diagnostic_t){
        .capture_attempted = true,
        .hook_registered = true,
        .matching_failure_count = second ? 0U : 1U,
        .first_requested_size = second ? 0U : 2048U,
        .last_requested_size = second ? 0U : 2048U,
        .max_requested_size = second ? 0U : 2048U,
        .failure_caps = second ? 0U : 25U,
        .all_heap_caps_calloc = !second,
        .internal_8bit_free_before = second ? 52000U : 50000U,
        .internal_8bit_largest_before = second ? 31000U : 30000U,
        .internal_8bit_minimum_before = second ? 41000U : 40000U,
        .internal_8bit_free_at_failure = second ? 0U : 12000U,
        .internal_8bit_largest_at_failure = second ? 0U : 1024U,
        .internal_8bit_minimum_at_failure = second ? 0U : 10000U,
        .internal_8bit_free_after = second ? 51000U : 49000U,
        .internal_8bit_largest_after = second ? 30000U : 29000U,
        .internal_8bit_minimum_after = second ? 41000U : 40000U,
    };
    diagnostic.raw_pre_tls_diagnostic = (athom_pre_tls_diagnostic_t){
        .valid = !second, .request_sequence = second ? 0U : 101U,
        .connection_called = !second, .dns_started = !second,
        .dns_ok = !second, .socket_attempted = !second,
        .socket_created = !second, .connect_started = !second,
        .connect_pending = !second, .wait_observed = !second, .wait_timeout = !second,
        .handshake = {.valid = !second, .call_count = second ? 0U : 3U,
            .send_bytes = second ? 0U : 123U, .last_ret = second ? 0 : -0x6900},
    };
    diagnostic.deltas.cloud_request_count = second ? 20U : 10U;
    diagnostic.deltas.homey_request_count = second ? 21U : 11U;
    return diagnostic;
}

static void *writer_thread(void *unused)
{
    (void)unused;
    for (unsigned i = 0U; i < 50000U; ++i) {
        const athom_inventory_attempt_diagnostic_t candidate =
            sample_attempt((i & 1U) != 0U);
        athom_inventory_attempt_diagnostic_publish(&candidate);
    }
    atomic_store(&s_writer_done, 1);
    return NULL;
}

static void assert_correlated(const athom_inventory_attempt_diagnostic_t *copy)
{
    if (!copy->valid) return;
    if (copy->origin == ATHOM_REFRESH_ORIGIN_BOOT_AUTO) {
        assert(copy->attempt == 1U && copy->completed_at_ms == 101U);
        assert(copy->final_error == 55 && copy->stage == ATHOM_INVENTORY_STAGE_CLOUD_USER_DISCOVERY);
        assert(copy->raw_role == ATHOM_TRANSPORT_ROLE_CLOUD);
        assert(copy->raw_pre_tls_diagnostic.valid && copy->raw_pre_tls_diagnostic.request_sequence == 101U);
        assert(copy->raw_pre_tls_diagnostic.handshake.valid && copy->raw_pre_tls_diagnostic.handshake.call_count == 3U);
        assert(copy->raw_pre_tls_diagnostic.handshake.send_bytes == 123U && copy->raw_pre_tls_diagnostic.handshake.last_ret == -0x6900);
        assert(copy->raw_classification == ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
        assert(copy->deltas.cloud_request_count == 10U);
        assert(copy->deltas.homey_request_count == 11U);
        assert(copy->raw_tls_memory_diagnostic.capture_attempted);
        assert(copy->raw_tls_memory_diagnostic.matching_failure_count == 1U);
        assert(copy->raw_tls_memory_diagnostic.last_requested_size == 2048U);
        assert(copy->raw_tls_memory_diagnostic.internal_8bit_largest_at_failure == 1024U);
    } else {
        assert(copy->origin == ATHOM_REFRESH_ORIGIN_PERIODIC);
        assert(copy->attempt == 2U && copy->completed_at_ms == 202U);
        assert(copy->final_error == 77 && copy->stage == ATHOM_INVENTORY_STAGE_DEVICES);
        assert(copy->raw_role == ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE);
        assert(!copy->raw_pre_tls_diagnostic.valid);
        assert(!copy->raw_pre_tls_diagnostic.handshake.valid && copy->raw_pre_tls_diagnostic.handshake.send_bytes == 0U);
        assert(copy->raw_classification == ATHOM_TRANSPORT_HTTP_5XX);
        assert(copy->deltas.cloud_request_count == 20U);
        assert(copy->deltas.homey_request_count == 21U);
        assert(copy->raw_tls_memory_diagnostic.capture_attempted);
        assert(copy->raw_tls_memory_diagnostic.matching_failure_count == 0U);
    }
}

static void test_concurrent_publication_keeps_each_attempt_coherent(void)
{
    pthread_t writer;
    atomic_store(&s_writer_done, 0);
    assert(pthread_create(&writer, NULL, writer_thread, NULL) == 0);
    while (atomic_load(&s_writer_done) == 0) {
        athom_inventory_attempt_diagnostic_t copy = {0};
        athom_inventory_attempt_diagnostic_copy(&copy);
        assert_correlated(&copy);
    }
    assert(pthread_join(writer, NULL) == 0);
    athom_inventory_attempt_diagnostic_t copy = {0};
    athom_inventory_attempt_diagnostic_copy(&copy);
    assert(copy.sequence == 50000U);
    assert_correlated(&copy);
}

static void test_stage_and_role_allowlists(void)
{
    assert(athom_inventory_attempt_stage_classify("oauth_user_me_request") ==
           ATHOM_INVENTORY_STAGE_CLOUD_USER_DISCOVERY);
    assert(athom_inventory_attempt_stage_classify("delegation_parse") ==
           ATHOM_INVENTORY_STAGE_PARSE);
    assert(athom_inventory_attempt_stage_classify("oauth_user_me_parse") ==
           ATHOM_INVENTORY_STAGE_PARSE);
    assert(athom_inventory_attempt_stage_classify("homey_login_remote") ==
           ATHOM_INVENTORY_STAGE_HOMEY_REMOTE_LOGIN);
    assert(athom_inventory_attempt_stage_classify("homey_login_parse") ==
           ATHOM_INVENTORY_STAGE_PARSE);
    assert(athom_inventory_attempt_stage_classify("favorites_user_me") ==
           ATHOM_INVENTORY_STAGE_FAVORITES);
    assert(athom_inventory_attempt_stage_classify("inventory_zones") ==
           ATHOM_INVENTORY_STAGE_ZONES);
    assert(athom_inventory_attempt_stage_classify("inventory_devices") ==
           ATHOM_INVENTORY_STAGE_DEVICES);
    assert(athom_inventory_attempt_stage_classify("inventory_complete") ==
           ATHOM_INVENTORY_STAGE_INVENTORY_COMPLETE);
    assert(athom_inventory_attempt_stage_classify("cached_session_validation") ==
           ATHOM_INVENTORY_STAGE_CACHED_SESSION_VALIDATION);
    assert(athom_inventory_attempt_stage_classify("cached_alias_validation") ==
           ATHOM_INVENTORY_STAGE_CACHED_ALIAS_VALIDATION);
    assert(strcmp(athom_inventory_attempt_stage_name(
               ATHOM_INVENTORY_STAGE_CACHED_SESSION_VALIDATION),
               "cached_session_validation") == 0);
    assert(strcmp(athom_inventory_attempt_stage_name(
               ATHOM_INVENTORY_STAGE_CACHED_ALIAS_VALIDATION),
               "cached_alias_validation") == 0);
    assert(athom_inventory_attempt_stage_classify("private raw stage") ==
           ATHOM_INVENTORY_STAGE_UNKNOWN);
    assert(strcmp(athom_inventory_attempt_role_name(ATHOM_TRANSPORT_ROLE_CLOUD), "cloud") == 0);
    assert(strcmp(athom_inventory_attempt_role_name(ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE), "homey_remote") == 0);
    assert(strcmp(athom_inventory_attempt_role_name((athom_transport_role_t)99), "none") == 0);
    assert(strcmp(athom_inventory_attempt_stage_name((athom_inventory_attempt_stage_t)99), "unknown") == 0);
}

static void test_counter_deltas_and_wrap_guard(void)
{
    athom_transport_metrics_t before = {0};
    athom_transport_metrics_t after = {0};
    before.cloud_request_count = 10U;
    before.homey_request_count = 20U;
    before.cloud_client_init_count = 30U;
    before.cloud_client_reuse_count = 40U;
    before.cloud_client_cleanup_count = 50U;
    before.homey_client_init_count = 60U;
    before.homey_client_reuse_count = 70U;
    before.homey_client_cleanup_count = 80U;
    before.homey_session_create_count = 90U;
    before.remote_rebind_count = 100U;
    after = before;
    after.cloud_request_count += 1U;
    after.homey_request_count += 2U;
    after.cloud_client_init_count += 3U;
    after.cloud_client_reuse_count += 4U;
    after.cloud_client_cleanup_count += 5U;
    after.homey_client_init_count += 6U;
    after.homey_client_reuse_count += 7U;
    after.homey_client_cleanup_count += 8U;
    after.homey_session_create_count += 9U;
    after.remote_rebind_count += 10U;
    after.perform_count = 4U;
    after.last_classification = ATHOM_TRANSPORT_DEVICES_FAIL;
    after.last_perform_role = ATHOM_TRANSPORT_ROLE_CLOUD;
    after.last_perform_classification = ATHOM_TRANSPORT_TCP_CONNECT_FAIL;
    after.last_perform_http_status = 0;
    after.last_perform_err = ESP_ERR_HTTP_CONNECT;
    after.last_tls_query = ESP_OK;
    after.last_request_elapsed_ms = 1234U;
    after.last_connected_event_seen = true;
    after.last_error_event_seen = false;
    after.last_disconnected_event_seen = true;
    after.last_tls_memory_diagnostic = (athom_tls_memory_diagnostic_t){
        .capture_attempted = true,
        .hook_registered = true,
        .matching_failure_count = 1U,
        .first_requested_size = 2048U,
        .last_requested_size = 2048U,
        .max_requested_size = 2048U,
        .failure_caps = 25U,
        .all_heap_caps_calloc = true,
        .internal_8bit_free_before = 50000U,
        .internal_8bit_largest_before = 30000U,
        .internal_8bit_minimum_before = 40000U,
        .internal_8bit_free_at_failure = 12000U,
        .internal_8bit_largest_at_failure = 1024U,
        .internal_8bit_minimum_at_failure = 10000U,
        .internal_8bit_free_after = 49000U,
        .internal_8bit_largest_after = 29000U,
        .internal_8bit_minimum_after = 40000U,
    };

    const athom_inventory_attempt_diagnostic_t diagnostic =
        athom_inventory_attempt_build(
            ATHOM_REFRESH_ORIGIN_MANUAL, 3U, 456U,
            ESP_ERR_HTTP_CONNECT, 0, "oauth_user_me_request", &before, &after);
    assert(diagnostic.valid && diagnostic.raw_transport_observed);
    assert(diagnostic.raw_classification == ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
    assert(diagnostic.raw_perform_error == ESP_ERR_HTTP_CONNECT);
    assert(diagnostic.raw_connected_event_seen);
    assert(!diagnostic.raw_error_event_seen);
    assert(diagnostic.raw_disconnected_event_seen);
    assert(diagnostic.deltas.cloud_request_count == 1U);
    assert(diagnostic.deltas.homey_request_count == 2U);
    assert(diagnostic.deltas.cloud_client_init_count == 3U);
    assert(diagnostic.deltas.cloud_client_reuse_count == 4U);
    assert(diagnostic.deltas.cloud_client_cleanup_count == 5U);
    assert(diagnostic.deltas.homey_client_init_count == 6U);
    assert(diagnostic.deltas.homey_client_reuse_count == 7U);
    assert(diagnostic.deltas.homey_client_cleanup_count == 8U);
    assert(diagnostic.deltas.homey_session_create_count == 9U);
    assert(diagnostic.deltas.remote_rebind_count == 10U);
    char json[ATHOM_INVENTORY_ATTEMPT_DIAGNOSTIC_JSON_MAX];
    assert(athom_inventory_attempt_diagnostic_json(&diagnostic, json, sizeof(json)));
    assert(strstr(json, "\"role\":\"cloud\"") != NULL);
    assert(strstr(json, "\"classification\":\"TCP_CONNECT_FAIL\"") != NULL);
    assert(strstr(json, "\"stage\":\"cloud_user_discovery\"") != NULL);
    assert(strstr(json, "\"remote_rebind_count\":10") != NULL);
    assert(strstr(json, "\"connected_event_seen\":true") != NULL);
    assert(strstr(json, "\"error_event_seen\":false") != NULL);
    assert(strstr(json, "\"disconnected_event_seen\":true") != NULL);
    assert(strstr(json, "\"scope\":\"perform_window\"") != NULL);
    assert(strstr(json, "\"matching_failure_count\":1") != NULL);
    assert(strstr(json, "\"first_requested_size\":2048") != NULL);
    assert(strstr(json, "\"internal_8bit_largest\":1024") != NULL);
    printf("PATCH063_JSON_SAMPLE=%s\n", json);
    assert(athom_inventory_attempt_counter_delta(UINT32_MAX - 1U, 1U) == 3U);
    assert(athom_inventory_attempt_counter_delta(100U, 3U) == 0U);
}

static void test_no_perform_clears_stale_raw_values_and_json_is_sanitized(void)
{
    athom_transport_metrics_t before = {0};
    athom_transport_metrics_t after = {0};
    before.perform_count = 7U;
    after = before;
    after.last_perform_role = ATHOM_TRANSPORT_ROLE_HOMEY_REMOTE;
    after.last_perform_classification = ATHOM_TRANSPORT_TLS_FAIL;
    after.last_perform_err = 98765;
    after.last_tls_error = 87654;
    after.last_socket_errno = 76543;
    after.last_request_elapsed_ms = 65432U;
    after.last_connected_event_seen = true;
    after.last_error_event_seen = true;
    after.last_disconnected_event_seen = true;

    athom_inventory_attempt_diagnostic_t diagnostic =
        athom_inventory_attempt_build(
            ATHOM_REFRESH_ORIGIN_MANUAL, 4U, 789U,
            ESP_ERR_HTTP_CONNECT, 0, "unrecognized private-stage", &before, &after);
    assert(diagnostic.valid && !diagnostic.raw_transport_observed);
    assert(diagnostic.raw_role == ATHOM_TRANSPORT_ROLE_NONE);
    assert(diagnostic.raw_perform_error == 0 && diagnostic.raw_tls_error == 0);
    assert(diagnostic.raw_socket_errno == 0 && diagnostic.raw_request_elapsed_ms == 0U);
    assert(diagnostic.stage == ATHOM_INVENTORY_STAGE_UNKNOWN);
    assert(!diagnostic.raw_connected_event_seen);
    assert(!diagnostic.raw_error_event_seen);
    assert(!diagnostic.raw_disconnected_event_seen);
    assert(diagnostic.raw_tls_memory_diagnostic.matching_failure_count == 0U);
    assert(!diagnostic.raw_tls_memory_diagnostic.capture_attempted);

    char json[ATHOM_INVENTORY_ATTEMPT_DIAGNOSTIC_JSON_MAX];
    assert(athom_inventory_attempt_diagnostic_json(&diagnostic, json, sizeof(json)));
    assert(strstr(json, "\"raw_transport_observed\":false") != NULL);
    assert(strstr(json, "\"raw_transport\":null") != NULL);
    assert(strstr(json, "unrecognized private-stage") == NULL);
    assert(strstr(json, "87654") == NULL && strstr(json, "76543") == NULL);
    assert(strstr(json, "Authorization") == NULL && strstr(json, "Bearer") == NULL);
    assert(strstr(json, "remote_url") == NULL && strstr(json, "homey_id") == NULL);
    assert(!athom_inventory_attempt_diagnostic_json(&diagnostic, json, 8U));
}

static void test_maximum_numeric_json_fits_fixed_capacity(void)
{
    athom_inventory_attempt_diagnostic_t diagnostic = sample_attempt(true);
    diagnostic.sequence = UINT32_MAX;
    diagnostic.attempt = UINT32_MAX;
    diagnostic.completed_at_ms = UINT64_MAX;
    diagnostic.final_error = INT32_MIN;
    diagnostic.final_http_status = INT32_MAX;
    diagnostic.raw_perform_error = INT32_MIN;
    diagnostic.raw_http_status = INT32_MAX;
    diagnostic.raw_tls_query = INT32_MIN;
    diagnostic.raw_tls_error = INT32_MAX;
    diagnostic.raw_tls_flags = INT32_MIN;
    diagnostic.raw_socket_errno = INT32_MAX;
    diagnostic.raw_request_elapsed_ms = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.capture_attempted = true;
    diagnostic.raw_tls_memory_diagnostic.hook_registered = true;
    diagnostic.raw_tls_memory_diagnostic.matching_failure_count = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.first_requested_size = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.last_requested_size = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.max_requested_size = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.failure_caps = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.internal_8bit_free_before = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.internal_8bit_largest_before = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.internal_8bit_minimum_before = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.internal_8bit_free_at_failure = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.internal_8bit_largest_at_failure = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.internal_8bit_minimum_at_failure = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.internal_8bit_free_after = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.internal_8bit_largest_after = UINT32_MAX;
    diagnostic.raw_tls_memory_diagnostic.internal_8bit_minimum_after = UINT32_MAX;
    diagnostic.deltas.cloud_request_count = UINT32_MAX;
    diagnostic.deltas.homey_request_count = UINT32_MAX;
    diagnostic.deltas.cloud_client_init_count = UINT32_MAX;
    diagnostic.deltas.cloud_client_reuse_count = UINT32_MAX;
    diagnostic.deltas.cloud_client_cleanup_count = UINT32_MAX;
    diagnostic.deltas.homey_client_init_count = UINT32_MAX;
    diagnostic.deltas.homey_client_reuse_count = UINT32_MAX;
    diagnostic.deltas.homey_client_cleanup_count = UINT32_MAX;
    diagnostic.deltas.homey_session_create_count = UINT32_MAX;
    diagnostic.deltas.remote_rebind_count = UINT32_MAX;
    diagnostic.raw_pre_tls_diagnostic = (athom_pre_tls_diagnostic_t){
        .valid = true, .request_sequence = UINT32_MAX, .connection_called = true,
        .connection_count = UINT32_MAX, .wait_observed = true, .wait_timeout = true,
        .dns_result_count = UINT32_MAX, .dns_code = -4095,
        .socket_error = -4095, .connect_error = -4095, .so_error = -4095,
        .connection_result = INT32_MIN, .connect_elapsed_ms = UINT32_MAX,
        .handshake = { .valid = true, .tx_observed = true, .rx_observed = true, .state = 11,
            .call_count = UINT32_MAX,
            .want_read_count = UINT32_MAX,
            .want_write_count = UINT32_MAX,
            .send_calls = UINT32_MAX,
            .send_bytes = UINT32_MAX,
            .recv_calls = UINT32_MAX,
            .recv_bytes = UINT32_MAX,
            .first_tx_ms = UINT32_MAX,
            .first_rx_ms = UINT32_MAX,
            .last_progress_ms = UINT32_MAX,
            .last_ret = INT32_MIN,
            .send_last_ret = INT32_MIN,
            .recv_last_ret = INT32_MIN,
            .send_last_errno = -4095,
            .recv_last_errno = -4095,
        },
    };
    char json[ATHOM_INVENTORY_ATTEMPT_DIAGNOSTIC_JSON_MAX];
    assert(athom_inventory_attempt_diagnostic_json(&diagnostic, json, sizeof(json)));
    assert(strstr(json, "\"pre_tls\":{\"valid\":true") != NULL);
}

static void test_completion_rejects_previous_attempt_stage_and_status(void)
{
    athom_transport_metrics_t baseline = {0};
    uint32_t revision_baseline = 0U;
    s_live_metrics = (athom_transport_metrics_t){0};
    s_live_revision = 12U;
    s_test_time_us = 45678LL;
    athom_inventory_attempt_diagnostic_begin(&baseline, &revision_baseline);
    athom_inventory_attempt_diagnostic_complete(
        ATHOM_REFRESH_ORIGIN_MANUAL, 8U, ESP_FAIL, 401,
        "inventory_complete", &baseline, revision_baseline);

    athom_inventory_attempt_diagnostic_t copy = {0};
    athom_inventory_attempt_diagnostic_copy(&copy);
    assert(copy.valid && copy.final_error == ESP_FAIL);
    assert(copy.final_http_status == 0);
    assert(copy.stage == ATHOM_INVENTORY_STAGE_UNKNOWN);
    assert(!copy.raw_transport_observed);

    athom_inventory_attempt_diagnostic_begin(&baseline, &revision_baseline);
    s_live_revision++;
    athom_inventory_attempt_diagnostic_complete(
        ATHOM_REFRESH_ORIGIN_MANUAL, 9U, ESP_OK, 200,
        "inventory_devices", &baseline, revision_baseline);
    athom_inventory_attempt_diagnostic_copy(&copy);
    assert(copy.final_error == ESP_OK && copy.final_http_status == 200);
    assert(copy.stage == ATHOM_INVENTORY_STAGE_DEVICES);
    assert(copy.sequence != 0U);
}

static void test_preselection_observation_preserves_results_and_correlates_attempt(void)
{
    for (unsigned i = 0U; i < 4U; ++i) {
        s_fixture_refresh = (i == 1U);
        s_fixture_perform = (i != 2U);
        s_fixture_result = i == 3U ? ESP_OK : ESP_ERR_HTTP_CONNECT;
        s_fixture_status = i == 3U ? 200 : 0;
        s_fixture_stage = s_fixture_refresh ? "oauth_token_request" : "oauth_user_me_request";
        const uint32_t requests = s_live_metrics.cloud_request_count;
        assert(preselection_transport_observed(i + 1U, s_fixture_refresh) == s_fixture_result);
        athom_inventory_attempt_diagnostic_t copy = {0};
        athom_inventory_attempt_diagnostic_copy(&copy);
        assert(copy.origin == ATHOM_REFRESH_ORIGIN_PRESELECTION_RESTORE && copy.attempt == i + 1U);
        assert(copy.final_error == s_fixture_result);
        assert(copy.raw_transport_observed == s_fixture_perform);
        assert(copy.deltas.cloud_request_count == (s_fixture_perform ? 1U : 0U));
        if (s_fixture_perform) {
            assert(copy.raw_role == ATHOM_TRANSPORT_ROLE_CLOUD);
            assert(copy.stage == (s_fixture_refresh ? ATHOM_INVENTORY_STAGE_CLOUD_TOKEN
                : ATHOM_INVENTORY_STAGE_CLOUD_USER_DISCOVERY));
            assert(copy.raw_perform_error == s_fixture_result);
            assert(copy.raw_http_status == s_fixture_status);
            assert(copy.raw_connected_event_seen == s_fixture_refresh);
            assert(copy.raw_error_event_seen);
        } else {
            assert(copy.stage == ATHOM_INVENTORY_STAGE_UNKNOWN);
            assert(!copy.raw_connected_event_seen && !copy.raw_error_event_seen);
        }
        char json[ATHOM_INVENTORY_ATTEMPT_DIAGNOSTIC_JSON_MAX];
        assert(athom_inventory_attempt_diagnostic_json(&copy, json, sizeof(json)));
        assert(strstr(json, "\"origin\":\"preselection_restore\"") != NULL);
        assert(s_live_metrics.cloud_request_count == requests + (s_fixture_perform ? 1U : 0U));
    }
}

int main(void)
{
    test_concurrent_publication_keeps_each_attempt_coherent();
    test_stage_and_role_allowlists();
    test_counter_deltas_and_wrap_guard();
    test_no_perform_clears_stale_raw_values_and_json_is_sanitized();
    test_maximum_numeric_json_fits_fixed_capacity();
    test_completion_rejects_previous_attempt_stage_and_status();
    test_preselection_observation_preserves_results_and_correlates_attempt();
    puts("PATCH061_ATTEMPT_DIAGNOSTICS_HOST_TESTS PASS");
    return 0;
}
