#include "athom_favorites_transport_diag.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define ESP_PLATFORM 1
#include "runtime_event_ring.h"
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_TIMEOUT 0x107
#define ESP_ERR_HTTP_CONNECT 0x7002
#define ESP_ERR_HTTP_EAGAIN 0x7007
#define ESP_ERR_HTTP_INCOMPLETE_DATA 0x700c
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_SIZE 0x104
#define ESP_ERR_INVALID_RESPONSE 0x108
#define ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME 0x8001
#define ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT 0x8006
#define ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET 0x8002
#define ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST 0x8004
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define HTTP_EVENT_ERROR 0
#define HTTP_EVENT_ON_CONNECTED 1
#define HTTP_EVENT_DISCONNECTED 2
#define HTTP_EVENT_ON_STATUS_CODE 3
#define HTTP_EVENT_ON_DATA 4
#define HTTP_EVENT_HEADERS_SENT 5
#define HTTP_EVENT_ON_HEADERS_COMPLETE 6
void athom_favorites_transport_diag_event(athom_favorites_transport_event_t event) { (void)event; }
#define ATHOM_HOMEY_LIVE_STATUS_JSON_MAX 6144U
#define PANEL_FIRST_BOOT_EVIDENCE_JSON_MAX 4096U

typedef struct { int event_id; void *user_data; void *data; int data_len; void *client; } esp_http_client_event_t;
typedef struct { const char *query; bool query_fail; } httpd_req_t;
typedef esp_err_t (*httpd_uri_handler_t)(httpd_req_t *);
typedef struct {
    const char *uri;
    int method;
    httpd_uri_handler_t handler;
    void *user_ctx;
} httpd_uri_t;
#define HTTP_GET 0
typedef struct { char id[96]; char name[128]; } athom_homey_t;
typedef struct { unsigned count; } athom_homey_list_t;
typedef struct { unsigned generation; } panel_homey_snapshot_value_t;
typedef struct { panel_homey_snapshot_value_t snapshot; } panel_homey_snapshot_inspection_t;
typedef struct { bool attempted; } panel_homey_snapshot_publish_inspection_t;
typedef struct { int unused; } panel_homey_alias_store_diagnostic_t;
typedef struct { bool attempted; int result; } athom_cloud_alias_activation_status_t;
typedef struct { bool valid; uint32_t sequence; bool inventory_snapshot_published; } athom_inventory_attempt_diagnostic_t;
typedef enum {
    PANEL_AWNING_GUARD_UNINITIALIZED = 0,
    PANEL_AWNING_GUARD_READY,
    PANEL_AWNING_GUARD_WRITE_PENDING,
    PANEL_AWNING_GUARD_QUARANTINED,
} panel_awning_guard_state_t;
typedef struct { uint8_t state; } panel_awning_guard_record_t;
typedef struct {
    bool loaded;
    panel_awning_guard_state_t state;
    bool runtime_ready;
} athom_awning_guard_diagnostic_snapshot_t;
typedef pthread_mutex_t portMUX_TYPE;
static portMUX_TYPE s_refresh_job_mux = PTHREAD_MUTEX_INITIALIZER;
static unsigned critical_entry_count;
#define portENTER_CRITICAL(mux) do { pthread_mutex_lock((mux)); critical_entry_count++; } while (0)
#define portEXIT_CRITICAL(mux) pthread_mutex_unlock((mux))
#define pdPASS 1
#define pdFAIL 0
#define portTRY_ENTER_CRITICAL(mux, timeout) \
    ((void)(timeout), pthread_mutex_trylock((mux)) == 0 ? (critical_entry_count++, pdPASS) : pdFAIL)
static bool s_awning_guard_loaded;
static panel_awning_guard_state_t s_awning_guard_state;
static panel_awning_guard_record_t s_awning_guard_record;
typedef struct {
    panel_homey_snapshot_inspection_t snapshot;
    panel_homey_snapshot_publish_inspection_t publish;
    panel_homey_alias_store_diagnostic_t alias_store;
    char awning_json[2048];
    athom_inventory_attempt_diagnostic_t inventory_attempt;
    char inventory_attempt_json[2048];
} athom_live_status_diagnostic_workspace_t;
static struct {
    athom_homey_t selected_homey;
    athom_homey_list_t homeys;
    size_t zone_count, device_count;
} s_cloud;
static const char *s_state_name = "ready";
static uint32_t s_runtime_id = 1U, s_select_attempt = 2U;
static char response[ATHOM_HOMEY_LIVE_STATUS_JSON_MAX];
static size_t response_length;
static const char *response_type;
static const char *response_cache_control;
static const char *http_status;
static unsigned allocation_count, free_count, fail_allocation, passive_reads;
static bool serialization_fail;
static esp_err_t send_result;
static void *allocations[2];
static size_t allocation_sizes[2];
static const char *s_diagnostic_stage = "inventory_devices";
static uint8_t runtime_stage_id(const char *stage) { return stage != NULL ? 3U : 0U; }
void runtime_event_ring_record_now(uint16_t id, uint8_t stage, int32_t result,
                                   uint32_t a, uint32_t b)
{ (void)id; (void)stage; (void)result; (void)a; (void)b; }
void runtime_event_ring_record_resources(uint16_t id, uint8_t stage)
{ (void)id; (void)stage; }
static bool runtime_diag_uart_record_allowed(void) { return false; }
int64_t esp_http_client_get_content_length(void *client) { (void)client; return -1; }
int esp_http_client_get_status_code(void *client) { (void)client; return 0; }
static esp_err_t status_diag_finish(esp_err_t result, uint64_t started_us)
{ (void)started_us; return result; }
static bool panel_first_boot_evidence_append_json(char *body, size_t capacity, size_t *length)
{ (void)body; (void)capacity; (void)length; return false; }
static esp_err_t httpd_resp_set_hdr(httpd_req_t *r, const char *name, const char *value)
{ (void)r; if (strcmp(name, "Cache-Control") == 0) response_cache_control = value; return ESP_OK; }
static esp_err_t httpd_resp_send(httpd_req_t *r, const char *body, size_t length)
{
    (void)r;
    assert(length < sizeof(response));
    memcpy(response, body, length);
    response[length] = '\0';
    response_length = length;
    return send_result;
}
static void zero_secure(void *p, size_t n) { memset(p, 0, n); }
static void *test_calloc(size_t count, size_t size)
{
    allocation_count++;
    if (allocation_count == fail_allocation) return NULL;
    assert(allocation_count <= 2U);
    void *p = calloc(count, size);
    assert(p != NULL);
    allocations[allocation_count - 1U] = p;
    allocation_sizes[allocation_count - 1U] = count * size;
    return p;
}
static void test_free(void *p)
{
    for (unsigned i = 0; i < 2U; i++) {
        if (allocations[i] != p) continue;
        for (size_t j = 0; j < allocation_sizes[i]; j++) assert(((unsigned char *)p)[j] == 0);
        allocations[i] = NULL;
        free_count++;
        free(p);
        return;
    }
    assert(false);
}
static size_t httpd_req_get_url_query_len(httpd_req_t *r) { return r->query ? strlen(r->query) : 0U; }
static esp_err_t httpd_req_get_url_query_str(httpd_req_t *r, char *out, size_t capacity)
{
    if (r->query_fail || strlen(r->query) >= capacity) return ESP_FAIL;
    strcpy(out, r->query); return ESP_OK;
}
static void httpd_resp_set_status(httpd_req_t *r, const char *status) { (void)r; http_status = status; }
static void httpd_resp_set_type(httpd_req_t *r, const char *type) { (void)r; response_type = type; }
static esp_err_t httpd_resp_sendstr(httpd_req_t *r, const char *body)
{ (void)r; assert(strlen(body) < sizeof(response)); strcpy(response, body); return send_result; }
static int64_t esp_timer_get_time(void) { return 123000; }
static bool athom_homey_status_json(char *out, size_t capacity, const char *state,
    const athom_homey_list_t *list, const athom_homey_t *selected, size_t zones, size_t devices)
{
    (void)state; (void)list; (void)zones; (void)devices;
    assert(selected == &s_cloud.selected_homey);
    return snprintf(out, capacity, "{\"state\":\"ready\",\"selected_homey\":{\"name\":\"PRIVATE_DISPLAY_NAME\"}}") > 0;
}
static int athom_cloud_inspect_device_snapshot_with_publish(uint64_t now,
    panel_homey_snapshot_inspection_t *snapshot, panel_homey_snapshot_publish_inspection_t *publish)
{ assert(now == 123U); snapshot->snapshot.generation = 7; publish->attempted = true; passive_reads++; return 0; }
static athom_cloud_alias_activation_status_t athom_cloud_alias_activation_status(void)
{ return (athom_cloud_alias_activation_status_t){true, 0}; }
static void athom_inventory_attempt_diagnostic_copy(athom_inventory_attempt_diagnostic_t *out)
{ out->valid = true; out->sequence = 1U; out->inventory_snapshot_published = true; passive_reads++; }
static bool athom_inventory_attempt_diagnostic_json(const athom_inventory_attempt_diagnostic_t *d,
    char *out, size_t capacity)
{ assert(d->valid); return !serialization_fail && snprintf(out, capacity, "{\"valid\":true}") > 0; }
/* The production append-only capture module is tested separately. This
 * endpoint fixture isolates the handler's query/serialization behavior. */
static bool athom_devices_http_diag_append_json(char *body, size_t capacity, size_t *length)
{
    static const char suffix[] = ",\"devices_http_capture\":{\"valid\":false}}";
    if (serialization_fail || body == NULL || length == NULL || *length == 0U ||
        body[*length - 1U] != '}' || sizeof(suffix) - 1U >= capacity - *length + 1U)
        return false;
    memcpy(body + *length - 1U, suffix, sizeof(suffix));
    *length += sizeof(suffix) - 2U;
    return true;
}
static bool panel_first_failure_diag_append_json(char *body, size_t capacity,
                                                 size_t *length)
{
    static const char suffix[] = ",\"first_failed_inventory\":null}";
    if (serialization_fail || body == NULL || length == NULL || *length == 0U ||
        body[*length - 1U] != '}' ||
        sizeof(suffix) - 1U >= capacity - *length + 1U) return false;
    memcpy(body + *length - 1U, suffix, sizeof(suffix));
    *length += sizeof(suffix) - 2U;
    return true;
}
static int panel_homey_alias_store_inspect(const char *id, panel_homey_alias_store_diagnostic_t *out)
{ assert(strcmp(id, "PRIVATE_HOMEY_ID") == 0); out->unused = 1; passive_reads++; return 0; }
static bool athom_homey_awning_snapshot_json(char *out, size_t capacity,
    const panel_homey_snapshot_inspection_t *snapshot, const panel_homey_snapshot_publish_inspection_t *publish,
    bool attempted, int result, const panel_homey_alias_store_diagnostic_t *store)
{
    assert(snapshot->snapshot.generation == 7 && publish->attempted && attempted && result == 0 && store->unused == 1);
    return snprintf(out, capacity, "{\"snapshot\":{\"generation\":7}}") > 0;
}
static const char *athom_cloud_diagnostic_stage(void) { return "inventory_devices"; }
static int athom_cloud_diagnostic_error(void) { return ESP_ERR_HTTP_CONNECT; }
static int athom_cloud_diagnostic_http_status(void) { return 0; }
static const char *athom_oauth_runtime_homey_data_state_name(void) { return "retrying"; }
#define calloc test_calloc
#define free test_free
/* PATCH062_PRODUCTION */
/* PATCH119_ROUTE */
#undef calloc
#undef free

static void reset(void)
{
    assert(allocations[0] == NULL && allocations[1] == NULL);
    allocation_count = free_count = fail_allocation = passive_reads = 0;
    serialization_fail = false; send_result = ESP_OK;
    response[0] = 0; response_length = 0U; response_type = NULL; response_cache_control = NULL; http_status = "200 OK";
    pthread_mutex_lock(&s_refresh_job_mux);
    s_awning_guard_loaded = true;
    s_awning_guard_state = PANEL_AWNING_GUARD_READY;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_READY;
    pthread_mutex_unlock(&s_refresh_job_mux);
    critical_entry_count = 0U;
    strcpy(s_cloud.selected_homey.id, "PRIVATE_HOMEY_ID");
    strcpy(s_cloud.selected_homey.name, "PRIVATE_DISPLAY_NAME");
    s_cloud.zone_count = 5; s_cloud.device_count = 12;
}
static void test_query_and_real_handler(void)
{
    reset(); httpd_req_t request = {"diagnostics=1", false};
    unsigned char before[sizeof(s_cloud)]; memcpy(before, &s_cloud, sizeof(s_cloud));
    assert(status_get(&request) == ESP_OK);
    assert(allocation_count == 2 && free_count == 2 && passive_reads == 3);
    assert(critical_entry_count == 0U);
    assert(strstr(response, "PRIVATE") == NULL && strstr(response, "selected_homey") == NULL);
    assert(strstr(response, "awning_snapshot") && strstr(response, "last_inventory_attempt_transport"));
    printf("LIVE_STATUS_JSON=%s\n", response);
    assert(memcmp(before, &s_cloud, sizeof(s_cloud)) == 0);
    reset(); request.query = NULL;
    assert(status_get(&request) == ESP_OK);
    assert(strstr(response, "PRIVATE_DISPLAY_NAME") != NULL); /* portal preserved */
    assert(strstr(response, "PRIVATE_HOMEY_ID") == NULL);
    assert(critical_entry_count == 0U);
    assert(free_count == 2);
    const char *bad[] = {"diagnostics=0", "diagnostics=2", "diagnostics=1&x=1",
                        "diagnostics=1&diagnostics=1", "diagnostics=%31", "x=1", "diagnostics=1x"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        reset(); request.query = bad[i];
        assert(status_get(&request) == ESP_OK);
        assert(strcmp(http_status, "400 Bad Request") == 0);
        assert(strcmp(response, "{\"error\":\"invalid_query\"}") == 0);
        assert(allocation_count == 0 && passive_reads == 0);
    }
    reset(); request.query = "diagnostics=1"; request.query_fail = true;
    assert(status_get(&request) == ESP_OK && strcmp(http_status, "400 Bad Request") == 0);
    assert(allocation_count == 0 && passive_reads == 0);
    request.query_fail = false;
    for (unsigned fail = 1; fail <= 2; fail++) {
        reset(); fail_allocation = fail;
        assert(status_get(&request) == ESP_ERR_NO_MEM);
        assert(free_count == fail - 1U && response[0] == 0 && passive_reads == 0);
    }
    reset(); serialization_fail = true;
    assert(status_get(&request) == ESP_ERR_INVALID_RESPONSE && free_count == 2 && response[0] == 0);
    reset(); send_result = ESP_FAIL;
    assert(status_get(&request) == ESP_FAIL && free_count == 2);
}

static void assert_guard_endpoint_state(const char *expected_state,
                                       bool expected_ready)
{
    httpd_req_t request = {NULL, false};
    const bool old_loaded = s_awning_guard_loaded;
    const panel_awning_guard_state_t old_state = s_awning_guard_state;
    const uint8_t old_record_state = s_awning_guard_record.state;
    assert(test_guard_route.handler(&request) == ESP_OK);
    assert(response_length > 0U && response_length < 160U);
    assert(response_type != NULL &&
           strcmp(response_type, "application/json; charset=utf-8") == 0);
    assert(response_cache_control != NULL &&
           strcmp(response_cache_control, "no-store") == 0);
    assert(strstr(response, expected_state) != NULL);
    assert(strstr(response, expected_ready
        ? "\"runtime_ready\":true" : "\"runtime_ready\":false") != NULL);
    assert(strstr(response, "PRIVATE") == NULL);
    assert(strstr(response, "homey_identity") == NULL);
    assert(strstr(response, "crc32") == NULL);
    assert(s_awning_guard_loaded == old_loaded &&
           s_awning_guard_state == old_state &&
           s_awning_guard_record.state == old_record_state);
    assert(passive_reads == 0U);
    printf("GUARD_ENDPOINT_JSON=%s\n", response);
}

static void test_guard_endpoint_states_and_fail_closed(void)
{
    assert(strcmp(test_guard_route.uri, "/homey/debug/awning-guard") == 0);
    assert(test_guard_route.method == HTTP_GET);
    assert(test_guard_route.handler == awning_guard_get);
    reset();
    assert(awning_guard_get(NULL) == ESP_ERR_INVALID_ARG);
    assert_guard_endpoint_state("\"state\":\"ready\"", true);
    char first_ready_response[160];
    strcpy(first_ready_response, response);
    assert_guard_endpoint_state("\"state\":\"ready\"", true);
    assert(strcmp(first_ready_response, response) == 0);

    s_awning_guard_state = PANEL_AWNING_GUARD_UNINITIALIZED;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_UNINITIALIZED;
    assert_guard_endpoint_state("\"state\":\"uninitialized\"", false);

    s_awning_guard_state = PANEL_AWNING_GUARD_WRITE_PENDING;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_WRITE_PENDING;
    assert_guard_endpoint_state("\"state\":\"write_pending\"", false);

    s_awning_guard_state = PANEL_AWNING_GUARD_QUARANTINED;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_QUARANTINED;
    assert_guard_endpoint_state("\"state\":\"quarantined\"", false);

    s_awning_guard_state = PANEL_AWNING_GUARD_READY;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_WRITE_PENDING;
    assert_guard_endpoint_state("\"state\":\"unknown\"", false);

    s_awning_guard_state = PANEL_AWNING_GUARD_READY;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_READY;
    s_awning_guard_loaded = false;
    assert_guard_endpoint_state("\"state\":\"unknown\"", false);

    s_awning_guard_loaded = true;
    pthread_mutex_lock(&s_refresh_job_mux);
    httpd_req_t request = {NULL, false};
    assert(test_guard_route.handler(&request) == ESP_OK);
    pthread_mutex_unlock(&s_refresh_job_mux);
    assert(strstr(response, "\"state\":\"unknown\"") != NULL);
    assert(strstr(response, "\"runtime_ready\":false") != NULL);
    assert(strstr(response, "\"provenance\":\"unknown\"") != NULL);
}

static void test_guard_snapshot_state_and_immutability(void)
{
    athom_awning_guard_diagnostic_snapshot_t snapshot;
    reset();
    assert(athom_oauth_runtime_awning_guard_diagnostic_snapshot(&snapshot));
    assert(snapshot.loaded && snapshot.state == PANEL_AWNING_GUARD_READY && snapshot.runtime_ready);

    s_awning_guard_state = PANEL_AWNING_GUARD_UNINITIALIZED;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_UNINITIALIZED;
    assert(athom_oauth_runtime_awning_guard_diagnostic_snapshot(&snapshot));
    assert(snapshot.loaded && snapshot.state == PANEL_AWNING_GUARD_UNINITIALIZED && !snapshot.runtime_ready);

    s_awning_guard_state = PANEL_AWNING_GUARD_WRITE_PENDING;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_WRITE_PENDING;
    assert(athom_oauth_runtime_awning_guard_diagnostic_snapshot(&snapshot));
    assert(snapshot.state == PANEL_AWNING_GUARD_WRITE_PENDING && !snapshot.runtime_ready);

    s_awning_guard_state = PANEL_AWNING_GUARD_QUARANTINED;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_QUARANTINED;
    assert(athom_oauth_runtime_awning_guard_diagnostic_snapshot(&snapshot));
    assert(snapshot.state == PANEL_AWNING_GUARD_QUARANTINED && !snapshot.runtime_ready);

    s_awning_guard_state = PANEL_AWNING_GUARD_READY;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_WRITE_PENDING;
    assert(athom_oauth_runtime_awning_guard_diagnostic_snapshot(&snapshot));
    assert(snapshot.state == PANEL_AWNING_GUARD_READY && !snapshot.runtime_ready);

    s_awning_guard_loaded = false;
    s_awning_guard_record.state = PANEL_AWNING_GUARD_READY;
    assert(athom_oauth_runtime_awning_guard_diagnostic_snapshot(&snapshot));
    assert(!snapshot.loaded && !snapshot.runtime_ready);

    pthread_mutex_lock(&s_refresh_job_mux);
    assert(!athom_oauth_runtime_awning_guard_diagnostic_snapshot(&snapshot));
    assert(!snapshot.loaded && !snapshot.runtime_ready);
    pthread_mutex_unlock(&s_refresh_job_mux);
}

static void *guard_transition_writer(void *unused)
{
    (void)unused;
    for (unsigned i = 0U; i < 20000U; ++i) {
        portENTER_CRITICAL(&s_refresh_job_mux);
        s_awning_guard_record.state = PANEL_AWNING_GUARD_WRITE_PENDING;
        s_awning_guard_state = PANEL_AWNING_GUARD_WRITE_PENDING;
        portEXIT_CRITICAL(&s_refresh_job_mux);
        portENTER_CRITICAL(&s_refresh_job_mux);
        s_awning_guard_record.state = PANEL_AWNING_GUARD_READY;
        s_awning_guard_state = PANEL_AWNING_GUARD_READY;
        portEXIT_CRITICAL(&s_refresh_job_mux);
    }
    return NULL;
}

static void test_guard_snapshot_concurrency(void)
{
    reset();
    pthread_t writer;
    assert(pthread_create(&writer, NULL, guard_transition_writer, NULL) == 0);
    for (unsigned i = 0U; i < 40000U; ++i) {
        athom_awning_guard_diagnostic_snapshot_t snapshot = {0};
        if (!athom_oauth_runtime_awning_guard_diagnostic_snapshot(&snapshot))
            continue;
        if (snapshot.runtime_ready)
            assert(snapshot.loaded && snapshot.state == PANEL_AWNING_GUARD_READY);
        if (snapshot.state == PANEL_AWNING_GUARD_WRITE_PENDING)
            assert(!snapshot.runtime_ready);
    }
    assert(pthread_join(writer, NULL) == 0);
}
static void test_events_and_classification(void)
{
    assert(event_handler(NULL) == ESP_OK);
    esp_http_client_event_t event = {HTTP_EVENT_ERROR, NULL, (void *)"PRIVATE_EVENT", 0, NULL};
    assert(event_handler(&event) == ESP_OK);
    response_buffer_t first = {0};
    event.user_data = &first;
    assert(event_handler(&event) == ESP_OK && first.error_event_seen);
    assert(!first.connected_event_seen && !first.disconnected_event_seen);
    event.event_id = HTTP_EVENT_ON_CONNECTED; assert(event_handler(&event) == ESP_OK);
    event.event_id = HTTP_EVENT_DISCONNECTED; assert(event_handler(&event) == ESP_OK);
    assert(first.connected_event_seen && first.disconnected_event_seen && first.data == NULL);
    response_buffer_t next = {0}; event.user_data = &next;
    event.event_id = HTTP_EVENT_ON_CONNECTED; assert(event_handler(&event) == ESP_OK);
    assert(next.connected_event_seen && !next.error_event_seen && !next.disconnected_event_seen);
    int status = 200; event.event_id = HTTP_EVENT_ON_STATUS_CODE; event.data = &status; event.data_len = sizeof(status);
    assert(event_handler(&event) == ESP_OK && next.fresh_status_received && next.fresh_http_status == 200);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, 0x8010, -1234, 4, 0) == ATHOM_TRANSPORT_TLS_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_OK, 0, 4, 0) == ATHOM_TRANSPORT_TLS_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME, 0, 0, 0) == ATHOM_TRANSPORT_DNS_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_OK, ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME, 0, 0) == ATHOM_TRANSPORT_DNS_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST, 0, 0, ECONNREFUSED) == ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET, 0, 0, EMFILE) == ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT, 0, 0, 0) == ATHOM_TRANSPORT_HTTP_TIMEOUT);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_OK, 0, 0, ETIMEDOUT) == ATHOM_TRANSPORT_HTTP_TIMEOUT);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_OK, 0, 0, 0) == ATHOM_TRANSPORT_TCP_CONNECT_FAIL); /* legacy fallback, not proof */
    assert(transport_classify(ESP_ERR_HTTP_EAGAIN, 0, ESP_OK, 0, 0, 0) == ATHOM_TRANSPORT_HTTP_TIMEOUT);
    assert(transport_classify(ESP_ERR_HTTP_EAGAIN, 0, ESP_OK, 0, 0, ECONNREFUSED) == ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_EAGAIN, 0, ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME, 0, 0, 0) == ATHOM_TRANSPORT_DNS_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_EAGAIN, 0, ESP_FAIL, 0x8010, 4, 0) == ATHOM_TRANSPORT_TLS_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_INCOMPLETE_DATA, 200, ESP_OK, 0, 0, EAGAIN) == ATHOM_TRANSPORT_HTTP_INCOMPLETE_DATA);
    assert(strcmp(athom_cloud_transport_class_name(ATHOM_TRANSPORT_HTTP_INCOMPLETE_DATA),
                  "HTTP_INCOMPLETE_DATA") == 0);
    assert(strcmp(athom_cloud_transport_class_name(ATHOM_TRANSPORT_HTTP_TIMEOUT),
                  "HTTP_TIMEOUT") == 0);
    assert(strcmp(athom_cloud_transport_class_name(ATHOM_TRANSPORT_TCP_CONNECT_FAIL),
                  "TCP_CONNECT_FAIL") == 0);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_ERR_INVALID_STATE, 0, 0, 0) == ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_FAIL, 0, 0, 0) == ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 401, 0x8010, -1234, 4, 0) == ATHOM_TRANSPORT_HTTP_401);
    assert(transport_classify(ESP_ERR_HTTP_EAGAIN, 403, 0x8010, -1234, 4, 0) == ATHOM_TRANSPORT_HTTP_403);
    assert(transport_classify(ESP_ERR_HTTP_EAGAIN, 408, ESP_OK, 0, 0, 0) == ATHOM_TRANSPORT_HTTP_408);
    assert(transport_classify(ESP_ERR_HTTP_EAGAIN, 429, ESP_OK, 0, 0, 0) == ATHOM_TRANSPORT_HTTP_429);
    assert(transport_classify(ESP_ERR_HTTP_EAGAIN, 503, ESP_OK, 0, 0, 0) == ATHOM_TRANSPORT_HTTP_5XX);
    assert(transport_classify(ESP_OK, 200, ESP_OK, 0, 0, 0) == ATHOM_TRANSPORT_OK);
}
int main(void)
{
    test_events_and_classification(); test_query_and_real_handler();
    test_guard_snapshot_state_and_immutability();
    test_guard_snapshot_concurrency();
    test_guard_endpoint_states_and_fail_closed();
    puts("PATCH062_CONNECT_DIAGNOSTICS_HOST_TESTS PASS"); return 0;
}
