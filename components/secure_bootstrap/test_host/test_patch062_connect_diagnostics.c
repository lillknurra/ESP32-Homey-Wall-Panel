#include "athom_favorites_transport_diag.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_TIMEOUT 0x107
#define ESP_ERR_HTTP_CONNECT 0x7002
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_INVALID_STATE 0x103
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

typedef struct { int event_id; void *user_data; void *data; int data_len; } esp_http_client_event_t;
typedef struct { const char *query; bool query_fail; } httpd_req_t;
typedef struct { char id[96]; char name[128]; } athom_homey_t;
typedef struct { unsigned count; } athom_homey_list_t;
typedef struct { unsigned generation; } panel_homey_snapshot_inspection_t;
typedef struct { bool attempted; } panel_homey_snapshot_publish_inspection_t;
typedef struct { int unused; } panel_homey_alias_store_diagnostic_t;
typedef struct { bool attempted; int result; } athom_cloud_alias_activation_status_t;
typedef struct { bool valid; } athom_inventory_attempt_diagnostic_t;
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
static const char *http_status;
static unsigned allocation_count, free_count, fail_allocation, passive_reads;
static bool serialization_fail;
static esp_err_t send_result;
static void *allocations[2];
static size_t allocation_sizes[2];
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
static void httpd_resp_set_type(httpd_req_t *r, const char *type) { (void)r; (void)type; }
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
{ assert(now == 123U); snapshot->generation = 7; publish->attempted = true; passive_reads++; return 0; }
static athom_cloud_alias_activation_status_t athom_cloud_alias_activation_status(void)
{ return (athom_cloud_alias_activation_status_t){true, 0}; }
static void athom_inventory_attempt_diagnostic_copy(athom_inventory_attempt_diagnostic_t *out)
{ out->valid = true; passive_reads++; }
static bool athom_inventory_attempt_diagnostic_json(const athom_inventory_attempt_diagnostic_t *d,
    char *out, size_t capacity)
{ assert(d->valid); return !serialization_fail && snprintf(out, capacity, "{\"valid\":true}") > 0; }
static int panel_homey_alias_store_inspect(const char *id, panel_homey_alias_store_diagnostic_t *out)
{ assert(strcmp(id, "PRIVATE_HOMEY_ID") == 0); out->unused = 1; passive_reads++; return 0; }
static bool athom_homey_awning_snapshot_json(char *out, size_t capacity,
    const panel_homey_snapshot_inspection_t *snapshot, const panel_homey_snapshot_publish_inspection_t *publish,
    bool attempted, int result, const panel_homey_alias_store_diagnostic_t *store)
{
    assert(snapshot->generation == 7 && publish->attempted && attempted && result == 0 && store->unused == 1);
    return snprintf(out, capacity, "{\"snapshot\":{\"generation\":7}}") > 0;
}
static const char *athom_cloud_diagnostic_stage(void) { return "inventory_devices"; }
static int athom_cloud_diagnostic_error(void) { return ESP_ERR_HTTP_CONNECT; }
static int athom_cloud_diagnostic_http_status(void) { return 0; }
static const char *athom_oauth_runtime_homey_data_state_name(void) { return "retrying"; }
#define calloc test_calloc
#define free test_free
/* PATCH062_PRODUCTION */
#undef calloc
#undef free

static void reset(void)
{
    assert(allocations[0] == NULL && allocations[1] == NULL);
    allocation_count = free_count = fail_allocation = passive_reads = 0;
    serialization_fail = false; send_result = ESP_OK;
    response[0] = 0; http_status = "200 OK";
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
    assert(strstr(response, "PRIVATE") == NULL && strstr(response, "selected_homey") == NULL);
    assert(strstr(response, "awning_snapshot") && strstr(response, "last_inventory_attempt_transport"));
    assert(memcmp(before, &s_cloud, sizeof(s_cloud)) == 0);
    reset(); request.query = NULL;
    assert(status_get(&request) == ESP_OK);
    assert(strstr(response, "PRIVATE_DISPLAY_NAME") != NULL); /* portal preserved */
    assert(strstr(response, "PRIVATE_HOMEY_ID") == NULL);
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
static void test_events_and_classification(void)
{
    assert(event_handler(NULL) == ESP_OK);
    esp_http_client_event_t event = {HTTP_EVENT_ERROR, NULL, (void *)"PRIVATE_EVENT", 0};
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
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_ERR_INVALID_STATE, 0, 0, 0) == ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 0, ESP_FAIL, 0, 0, 0) == ATHOM_TRANSPORT_TCP_CONNECT_FAIL);
    assert(transport_classify(ESP_ERR_HTTP_CONNECT, 401, 0x8010, -1234, 4, 0) == ATHOM_TRANSPORT_HTTP_401);
    assert(transport_classify(ESP_OK, 200, ESP_OK, 0, 0, 0) == ATHOM_TRANSPORT_OK);
}
int main(void)
{
    test_events_and_classification(); test_query_and_real_handler();
    puts("PATCH062_CONNECT_DIAGNOSTICS_HOST_TESTS PASS"); return 0;
}
