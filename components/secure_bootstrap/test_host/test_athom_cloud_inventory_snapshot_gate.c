#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int esp_err_t;
/* PATCH069_FAVORITES_TYPE */

#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_SIZE 0x103
#define ESP_ERR_INVALID_RESPONSE 0x104
#define ESP_ERR_NOT_FOUND 0x105
#define ESP_ERR_INVALID_STATE 0x106
#define ESP_ERR_HTTP_CONNECT 0x7002
#define ESP_ERR_HTTP_FETCH_HEADER 0x7004
#define ESP_ERR_TIMEOUT 0x107
#define HTTP_METHOD_GET 0
#define HTTP_BODY_MAX 4096U
#define HTTP_INVENTORY_BODY_MAX 4096U
#define PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS 120000ULL
#define ATHOM_HOMEY_URL_MAX 128U
#define ATHOM_HOMEY_ID_MAX 64U
#define ATHOM_HOMEY_MAX 8U
#define ATHOM_TOKEN_MAX 128U
#define ATHOM_TRANSPORT_NO_VALID_ENDPOINT 1
#define ATHOM_TRANSPORT_FAVORITES_FAIL 2
#define ATHOM_TRANSPORT_ZONES_FAIL 3
#define ATHOM_TRANSPORT_DEVICES_FAIL 4
#define PANEL_HOMEY_FAVORITES_OK 0
#define PANEL_HOMEY_ALIAS_STORE_OK 0
#define TAG "patch058-test"

static void test_log(const char *tag, const char *format, ...)
{
    (void)tag;
    (void)format;
    va_list args;
    va_start(args, format);
    va_end(args);
}

#define ESP_LOGI(tag, ...) test_log((tag), __VA_ARGS__)
#define ESP_LOGW(tag, ...) test_log((tag), __VA_ARGS__)

typedef enum {
    PANEL_HOMEY_READ_OK = 0,
    PANEL_HOMEY_READ_INVALID = 1,
    PANEL_HOMEY_READ_NOT_CONFIGURED = 2,
    PANEL_HOMEY_READ_NOT_FOUND = 3,
    PANEL_HOMEY_READ_STALE = 4,
    PANEL_HOMEY_READ_PROVIDER_ERROR = 5,
} panel_homey_read_result_t;

typedef struct {
    uint8_t opaque;
} panel_homey_alias_snapshot_t;

typedef struct {
    void *context;
    panel_homey_read_result_t (*resolve)(
        void *, const char *, const char *, void *);
    panel_homey_read_result_t (*capture)(
        void *, panel_homey_alias_snapshot_t *);
} panel_homey_alias_provider_t;

typedef struct {
    uint32_t generation;
    uint64_t captured_at_ms;
    size_t item_count;
} panel_homey_read_snapshot_t;

typedef struct {
    bool active;
    panel_homey_read_snapshot_t snapshot;
    bool publish_attempted;
    panel_homey_read_result_t publish_result;
    uint64_t publish_at_ms;
} panel_homey_snapshot_store_t;

#ifndef PANEL_HOMEY_SNAPSHOT_INSPECTION_DEFINED
#define PANEL_HOMEY_SNAPSHOT_INSPECTION_DEFINED
typedef struct {
    panel_homey_read_result_t result;
    bool present;
    bool fresh;
    uint64_t age_ms;
    panel_homey_read_snapshot_t snapshot;
} panel_homey_snapshot_inspection_t;
#endif

typedef struct {
    uint16_t event_type;
    uint64_t monotonic_ms;
    uint8_t source;
    uint8_t origin;
    uint16_t attempt;
    uint16_t result;
    int32_t error_code;
    uint16_t http_status;
    uint8_t transport;
    uint8_t stage;
    uint32_t retry_delay_ms;
    uint32_t snapshot_generation;
    uint16_t reset_reason;
} runtime_diag_event_t;

enum {
    RUNTIME_DIAG_EVENT_BOOT_START = 1,
    RUNTIME_DIAG_EVENT_WIFI_START,
    RUNTIME_DIAG_EVENT_WIFI_GOT_IP,
    RUNTIME_DIAG_EVENT_WIFI_ONLINE,
    RUNTIME_DIAG_EVENT_HTTP_SERVER_START_BEGIN,
    RUNTIME_DIAG_EVENT_HTTP_SERVER_START_RESULT,
    RUNTIME_DIAG_EVENT_AUTH_RESTORE_BEGIN,
    RUNTIME_DIAG_EVENT_AUTH_RESTORE_RESULT,
    RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_BEGIN,
    RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_ATTEMPT_RESULT,
    RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_RETRY,
    RUNTIME_DIAG_EVENT_PRESELECTION_RESTORE_RESULT,
    RUNTIME_DIAG_EVENT_BOOT_AUTO_SCHEDULER_BEGIN,
    RUNTIME_DIAG_EVENT_BOOT_AUTO_QUEUE_RESULT,
    RUNTIME_DIAG_EVENT_HOMEY_REFRESH_BEGIN,
    RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_BEGIN,
    RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_FAILURE,
    RUNTIME_DIAG_EVENT_HOMEY_RETRY_SCHEDULED,
    RUNTIME_DIAG_EVENT_HOMEY_ATTEMPT_SUCCESS,
    RUNTIME_DIAG_EVENT_SNAPSHOT_PUBLISH_SUCCESS,
    RUNTIME_DIAG_EVENT_SNAPSHOT_PUBLISH_FAILURE,
    RUNTIME_DIAG_EVENT_HOMEY_DATA_STATE_CHANGE,
    RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_QUEUE_RESULT,
    RUNTIME_DIAG_EVENT_PERIODIC_REFRESH_RESULT,
};

typedef struct {
    char id[ATHOM_HOMEY_ID_MAX];
    char remote_url[ATHOM_HOMEY_URL_MAX];
} athom_homey_t;

typedef struct {
    athom_homey_t items[ATHOM_HOMEY_MAX];
    size_t count;
} athom_homey_list_t;

typedef struct {
    char homey_session_token[ATHOM_TOKEN_MAX];
    athom_homey_list_t homeys;
    athom_homey_t selected_homey;
    size_t zone_count;
    size_t device_count;
} athom_cloud_state_t;

typedef struct cJSON {
    bool object;
    bool array;
    struct cJSON *child;
    struct cJSON *next;
} cJSON;

#define cJSON_ArrayForEach(element, array) \
    for ((element) = ((array) == NULL ? NULL : (array)->child); \
         (element) != NULL; (element) = (element)->next)

static athom_cloud_state_t s_cloud_state;
static panel_homey_snapshot_store_t s_device_snapshot_store;
static int s_alias_runtime;
static unsigned s_request_count;
static unsigned s_collection_parse_count;
static int s_favorites_parse_result;
static unsigned s_favorites_parse_count;
static unsigned s_favorites_clear_count;
static unsigned s_favorites_fetch_count;
static unsigned s_alias_activation_count;
static int s_alias_activation_result;
static unsigned s_snapshot_publish_count;
static unsigned s_response_zero_count;
static unsigned s_response_free_count;
static unsigned s_response_double_free_count;
static unsigned s_response_count;
static unsigned s_response_zeroed[4];
static size_t s_response_zero_length[4];
static size_t s_response_capacity[4];
static unsigned s_response_freed[4];
static char *s_response_buffers[4];
static int s_collection_kind;
static int s_http_status;
static int64_t s_now_us = 90000;
static panel_homey_read_result_t s_publish_result;
static const char *s_diagnostic_stage = "unknown";
static esp_err_t s_diagnostic_error = ESP_OK;
static int s_diagnostic_http_status;

static bool runtime_diag_journal_record(const runtime_diag_event_t *event)
{
    (void)event;
    return true;
}

static panel_homey_read_result_t panel_homey_snapshot_inspect(
    const panel_homey_snapshot_store_t *store,
    uint64_t now_ms,
    panel_homey_snapshot_inspection_t *inspection)
{
    if (store == NULL || inspection == NULL) return PANEL_HOMEY_READ_INVALID;
    memset(inspection, 0, sizeof(*inspection));
    inspection->present = store->active;
    inspection->fresh = store->active && now_ms >= store->snapshot.captured_at_ms;
    inspection->age_ms = inspection->fresh
        ? now_ms - store->snapshot.captured_at_ms : 0U;
    inspection->snapshot = store->snapshot;
    inspection->result = store->active ? PANEL_HOMEY_READ_OK : PANEL_HOMEY_READ_STALE;
    return inspection->result;
}

static void tracked_free(void *pointer);

static esp_err_t bearer_authorization(
    const char *token, char *out, size_t capacity)
{
    if (token == NULL || token[0] == '\0' || out == NULL || capacity < 2U)
        return ESP_ERR_INVALID_ARG;
    (void)snprintf(out, capacity, "Bearer synthetic");
    return ESP_OK;
}

static esp_err_t http_request_limited(
    const char *url,
    int method,
    const char *authorization,
    const void *body,
    const void *headers,
    char **response_out,
    int *status_out,
    size_t maximum,
    size_t *capacity_out)
{
    (void)method;
    (void)authorization;
    (void)body;
    (void)headers;
    (void)maximum;
    assert(url != NULL && response_out != NULL && status_out != NULL);
    assert(capacity_out != NULL);
    s_request_count++;
    s_collection_kind = strstr(url, "/zones/") != NULL ? 1 : 2;
    const size_t capacity = 96U;
    char *response = malloc(capacity);
    assert(response != NULL);
    strcpy(response, "{\"result\":{\"synthetic\":{},\"other\":{}}}");
    assert(s_response_count < 4U);
    s_response_buffers[s_response_count] = response;
    s_response_capacity[s_response_count] = capacity;
    s_response_count++;
    *response_out = response;
    *status_out = 200;
    *capacity_out = capacity;
    s_http_status = 200;
    return ESP_OK;
}

static void zero_secure(void *data, size_t length)
{
    if (data == NULL) return;
    unsigned char *bytes = data;
    for (size_t i = 0U; i < length; ++i) bytes[i] = 0U;
    for (unsigned i = s_response_count; i > 0U; --i) {
        const unsigned index = i - 1U;
        if (s_response_buffers[index] == data &&
            s_response_freed[index] == 0U) {
            s_response_zeroed[index]++;
            s_response_zero_length[index] = length;
            s_response_zero_count++;
            break;
        }
    }
}

static void tracked_free(void *pointer)
{
    for (unsigned i = s_response_count; i > 0U; --i) {
        const unsigned index = i - 1U;
        if (s_response_buffers[index] == pointer &&
            s_response_freed[index] == 0U) {
            s_response_freed[index]++;
            s_response_free_count++;
            free(pointer);
            return;
        }
    }
    for (unsigned i = 0U; i < s_response_count; ++i) {
        if (s_response_buffers[i] == pointer) {
            s_response_double_free_count++;
            return;
        }
    }
    free(pointer);
}

static void transport_memory_log(const char *phase, size_t size)
{
    (void)phase;
    (void)size;
}

static void ensure_device_snapshot_store(void) {}

static panel_homey_read_result_t panel_homey_alias_runtime_resolve(
    void *context, const char *device_alias, const char *capability_alias,
    void *out)
{
    (void)context;
    (void)device_alias;
    (void)capability_alias;
    (void)out;
    return PANEL_HOMEY_READ_OK;
}

static panel_homey_read_result_t alias_runtime_capture(
    void *context, panel_homey_alias_snapshot_t *out)
{
    (void)context;
    if (out == NULL) return PANEL_HOMEY_READ_INVALID;
    return PANEL_HOMEY_READ_OK;
}

static int64_t esp_timer_get_time(void) { return s_now_us; }

static panel_homey_read_result_t panel_homey_snapshot_publish_json(
    panel_homey_snapshot_store_t *store,
    const char *json,
    const panel_homey_alias_provider_t *provider,
    uint64_t now_ms)
{
    assert(store != NULL && json != NULL && provider != NULL);
    s_snapshot_publish_count++;
    store->publish_attempted = true;
    store->publish_result = s_publish_result;
    store->publish_at_ms = now_ms;
    if (s_publish_result != PANEL_HOMEY_READ_OK) return s_publish_result;
    store->snapshot.generation = store->active
        ? store->snapshot.generation + 1U : 1U;
    store->snapshot.captured_at_ms = now_ms;
    store->active = true;
    return PANEL_HOMEY_READ_OK;
}

static void panel_homey_favorites_clear(void)
{
    s_favorites_clear_count++;
}

static int panel_homey_favorites_parse_and_publish_with_alias_provider(
    const char *favorites_json,
    const char *device_json,
    const panel_homey_alias_provider_t *provider)
{
    assert(favorites_json != NULL && device_json != NULL && provider != NULL);
    s_favorites_parse_count++;
    return s_favorites_parse_result;
}

static const char *panel_homey_favorites_state_name(int ignored)
{
    (void)ignored;
    return "ready";
}

static int panel_homey_favorites_get_state(void) { return 0; }
static void homey_schema_log_inventory(const char *json) { (void)json; }

static struct {
    uint32_t inventory_read_count, perform_count, homey_client_reuse_count;
    bool inventory_snapshot_published, last_body_complete;
    int last_perform_err, last_perform_http_status, last_tls_error, last_socket_errno;
    uint32_t last_request_elapsed_ms;
    bool last_connected_event_seen, last_error_event_seen, last_disconnected_event_seen;
    athom_favorites_read_diagnostic_t favorites_read;
} s_transport_metrics;
static esp_err_t s_favorite_error;
static int s_favorite_status = 200;

static esp_err_t favorites_fetch_user_me(
    const char *base_url, const char *session_token, char **json_out,
    size_t *capacity_out, int *status_out)
{
    assert(base_url != NULL && session_token != NULL);
    assert(json_out != NULL && capacity_out != NULL && status_out != NULL);
    s_favorites_fetch_count++;
    const size_t capacity = 32U;
    char *json = malloc(capacity);
    assert(json != NULL);
    strcpy(json, "{}");
    *json_out = json;
    *capacity_out = capacity;
    *status_out = s_favorite_status;
    s_http_status = s_favorite_status;
    s_transport_metrics.perform_count++;
    s_transport_metrics.homey_client_reuse_count++;
    s_transport_metrics.last_perform_err = s_favorite_error;
    s_transport_metrics.last_perform_http_status = s_favorite_status;
    s_transport_metrics.last_body_complete = s_favorite_error == ESP_OK;
    return s_favorite_error;
}

static void diagnostic_set(const char *stage, esp_err_t error)
{
    s_diagnostic_stage = stage == NULL ? "unknown" : stage;
    s_diagnostic_error = error;
    s_diagnostic_http_status = 0;
}

static void diagnostic_set_http(const char *stage, esp_err_t error, int status)
{
    s_diagnostic_stage = stage == NULL ? "unknown" : stage;
    s_diagnostic_error = error;
    s_diagnostic_http_status = status;
}

static void transport_stage_failure(
    int category, const char *stage, esp_err_t error, int status)
{
    (void)category;
    s_diagnostic_stage = stage;
    s_diagnostic_error = error;
    s_diagnostic_http_status = status;
}

static int athom_cloud_diagnostic_http_status(void)
{
    return s_diagnostic_http_status != 0
        ? s_diagnostic_http_status : s_http_status;
}

static esp_err_t athom_cloud_diagnostic_error(void)
{
    return s_diagnostic_error;
}

static const char *athom_cloud_diagnostic_stage(void)
{
    return s_diagnostic_stage;
}

static int athom_cloud_alias_activate(const char *selected_homey_id)
{
    assert(selected_homey_id != NULL);
    s_alias_activation_count++;
    return s_alias_activation_result;
}

static const athom_homey_t *athom_homey_find_exact(
    const athom_homey_list_t *list, const char *homey_id)
{
    if (list == NULL || homey_id == NULL || homey_id[0] == '\0') return NULL;
    for (size_t i = 0U; i < list->count && i < ATHOM_HOMEY_MAX; ++i) {
        if (strcmp(list->items[i].id, homey_id) == 0) return &list->items[i];
    }
    return NULL;
}

static const char *esp_err_to_name(esp_err_t error)
{
    (void)error;
    return "synthetic";
}

static cJSON s_root;
static cJSON s_result;
static cJSON s_items[2];

static cJSON *cJSON_Parse(const char *json)
{
    assert(json != NULL);
    s_collection_parse_count++;
    memset(&s_root, 0, sizeof(s_root));
    memset(&s_result, 0, sizeof(s_result));
    memset(s_items, 0, sizeof(s_items));
    s_root.object = true;
    s_result.object = true;
    const unsigned count = s_collection_kind == 1 ? 1U : 2U;
    s_result.child = &s_items[0];
    s_items[0].next = count == 2U ? &s_items[1] : NULL;
    return &s_root;
}

static cJSON *cJSON_GetObjectItemCaseSensitive(cJSON *item, const char *name)
{
    assert(item == &s_root && name != NULL);
    return strcmp(name, "result") == 0 ? &s_result : NULL;
}

static bool cJSON_IsArray(const cJSON *item)
{
    return item != NULL && item->array;
}

static bool cJSON_IsObject(const cJSON *item)
{
    return item != NULL && item->object;
}

static int cJSON_GetArraySize(const cJSON *item)
{
    (void)item;
    return 0;
}

static void cJSON_Delete(cJSON *item) { (void)item; }

#define free tracked_free
/* PATCH058_PRODUCTION_FUNCTIONS */

static void reset_case(void)
{
    memset(&s_cloud_state, 0, sizeof(s_cloud_state));
    memset(&s_device_snapshot_store, 0, sizeof(s_device_snapshot_store));
    memset(s_response_zeroed, 0, sizeof(s_response_zeroed));
    memset(s_response_zero_length, 0, sizeof(s_response_zero_length));
    memset(s_response_capacity, 0, sizeof(s_response_capacity));
    memset(s_response_freed, 0, sizeof(s_response_freed));
    memset(s_response_buffers, 0, sizeof(s_response_buffers));
    s_request_count = 0U;
    s_collection_parse_count = 0U;
    s_favorites_parse_count = 0U;
    s_favorites_parse_result = PANEL_HOMEY_FAVORITES_OK;
    s_favorites_clear_count = 0U;
    s_favorites_fetch_count = 0U;
    memset(&s_transport_metrics, 0, sizeof(s_transport_metrics));
    s_favorite_error = ESP_OK;
    s_favorite_status = 200;
    s_snapshot_publish_count = 0U;
    s_response_zero_count = 0U;
    s_response_free_count = 0U;
    s_response_double_free_count = 0U;
    s_response_count = 0U;
    s_collection_kind = 0;
    s_http_status = 0;
    s_now_us = 90000;
    s_publish_result = PANEL_HOMEY_READ_OK;
    s_diagnostic_stage = "unknown";
    s_diagnostic_error = ESP_OK;
    s_diagnostic_http_status = 0;
    s_alias_activation_count = 0U;
    s_alias_activation_result = PANEL_HOMEY_ALIAS_STORE_OK;
    strcpy(s_cloud_state.homey_session_token, "synthetic-token");
    strcpy(s_cloud_state.selected_homey.id, "synthetic-homey");
    strcpy(s_cloud_state.selected_homey.remote_url, "https://synthetic.invalid");
    s_cloud_state.homeys.count = 1U;
    strcpy(s_cloud_state.homeys.items[0].id, "synthetic-homey");
    strcpy(s_cloud_state.homeys.items[0].remote_url, "https://synthetic.invalid");
}

static bool inventory_verified(esp_err_t result, esp_err_t *effective_error)
{
    int status = 0;
    const char *stage = NULL;
    return homey_inventory_result_verified(
        result, effective_error, &status, &stage);
}

static void assert_responses_cleaned_once(void)
{
    assert(s_response_count == 2U);
    assert(s_response_zero_count == 2U);
    assert(s_response_free_count == 2U);
    assert(s_response_double_free_count == 0U);
    for (unsigned i = 0U; i < s_response_count; ++i) {
        assert(s_response_zeroed[i] == 1U);
        assert(s_response_zero_length[i] == s_response_capacity[i]);
        assert(s_response_freed[i] == 1U);
    }
}

static void test_successful_publish_advances_existing_generation(void)
{
    reset_case();
    s_device_snapshot_store.active = true;
    s_device_snapshot_store.snapshot.generation = 7U;
    s_device_snapshot_store.snapshot.captured_at_ms = 1U;
    const esp_err_t result = athom_cloud_fetch_inventory(&s_cloud_state);
    assert(result == ESP_OK);
    assert(s_device_snapshot_store.active);
    assert(s_device_snapshot_store.snapshot.generation == 8U);
    assert(s_device_snapshot_store.snapshot.captured_at_ms == 90U);
    assert(s_device_snapshot_store.publish_attempted);
    assert(s_device_snapshot_store.publish_result == PANEL_HOMEY_READ_OK);
    assert(strcmp(s_diagnostic_stage, "inventory_complete") == 0);
    esp_err_t effective_error = ESP_FAIL;
    assert(inventory_verified(result, &effective_error));
    assert(effective_error == ESP_OK);
    assert(s_favorites_parse_count == 1U);
    assert(s_transport_metrics.favorites_read.data_verified);
    assert(s_favorites_clear_count == 0U);
    assert(s_request_count + s_favorites_fetch_count == 3U);
    assert(s_favorites_fetch_count == 1U);
    assert(s_snapshot_publish_count == 1U);
    assert(s_collection_parse_count == 2U);
    assert_responses_cleaned_once();
}

static void test_first_successful_publish_establishes_generation_one(void)
{
    reset_case();
    const esp_err_t result = athom_cloud_fetch_inventory(&s_cloud_state);
    assert(result == ESP_OK);
    assert(s_device_snapshot_store.active);
    assert(s_device_snapshot_store.snapshot.generation == 1U);
    assert(strcmp(s_diagnostic_stage, "inventory_complete") == 0);
    assert(s_request_count + s_favorites_fetch_count == 3U);
    assert_responses_cleaned_once();
}

static void test_cached_session_inventory_requires_matching_private_state(void)
{
    reset_case();
    s_device_snapshot_store.active = true;
    s_device_snapshot_store.snapshot.generation = 17U;
    s_device_snapshot_store.snapshot.captured_at_ms = 1U;
    assert(athom_cloud_fetch_inventory_from_cached_session(
        &s_cloud_state, "synthetic-homey") == ESP_OK);
    assert(s_device_snapshot_store.snapshot.generation == 18U);
    assert(s_snapshot_publish_count == 1U);
    assert(s_alias_activation_count == 1U);

    reset_case();
    /* Persisted selected/session state survives reboot; discovery list does not. */
    s_cloud_state.homeys.count = 0U;
    assert(athom_cloud_fetch_inventory_from_cached_session(
        &s_cloud_state, "synthetic-homey") == ESP_OK);
    assert(s_alias_activation_count == 1U);
    assert(s_snapshot_publish_count == 1U);
    assert(s_device_snapshot_store.snapshot.generation == 1U);

    reset_case();
    s_cloud_state.homey_session_token[0] = '\0';
    assert(athom_cloud_fetch_inventory_from_cached_session(
        &s_cloud_state, "synthetic-homey") == ESP_ERR_INVALID_STATE);
    assert(s_request_count == 0U && s_favorites_fetch_count == 0U);
    assert(s_snapshot_publish_count == 0U);
    assert(s_alias_activation_count == 0U);

    reset_case();
    assert(athom_cloud_fetch_inventory_from_cached_session(
        &s_cloud_state, "different-homey") == ESP_ERR_INVALID_STATE);
    assert(s_request_count == 0U && s_favorites_fetch_count == 0U);
    assert(s_snapshot_publish_count == 0U);

    reset_case();
    strcpy(s_cloud_state.homeys.items[0].remote_url, "https://other.invalid");
    assert(athom_cloud_fetch_inventory_from_cached_session(
        &s_cloud_state, "synthetic-homey") == ESP_ERR_INVALID_STATE);
    assert(s_request_count == 0U && s_favorites_fetch_count == 0U);
    assert(s_snapshot_publish_count == 0U);

    reset_case();
    s_cloud_state.homeys.count = ATHOM_HOMEY_MAX + 1U;
    assert(athom_cloud_fetch_inventory_from_cached_session(
        &s_cloud_state, "synthetic-homey") == ESP_ERR_INVALID_STATE);
    assert(s_request_count == 0U && s_favorites_fetch_count == 0U);
    assert(s_snapshot_publish_count == 0U);
}

static void test_cached_session_alias_failure_fails_closed(void)
{
    reset_case();
    s_alias_activation_result = 1;
    assert(athom_cloud_fetch_inventory_from_cached_session(
        &s_cloud_state, "synthetic-homey") == ESP_ERR_INVALID_STATE);
    assert(s_alias_activation_count == 1U);
    assert(s_request_count == 0U && s_favorites_fetch_count == 0U);
    assert(s_snapshot_publish_count == 0U);
}

static void assert_publication_failure(
    panel_homey_read_result_t failure, bool old_snapshot_present,
    uint32_t old_generation)
{
    reset_case();
    s_publish_result = failure;
    s_device_snapshot_store.active = old_snapshot_present;
    s_device_snapshot_store.snapshot.generation = old_generation;
    s_device_snapshot_store.snapshot.captured_at_ms = 1U;
    if (old_snapshot_present) s_now_us = 300000000LL;
    assert(!old_snapshot_present ||
        s_now_us / 1000LL -
            (int64_t)s_device_snapshot_store.snapshot.captured_at_ms >
                (int64_t)PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS);
    const esp_err_t result = athom_cloud_fetch_inventory(&s_cloud_state);
    assert(result == ESP_ERR_INVALID_RESPONSE);
    assert(s_device_snapshot_store.active == old_snapshot_present);
    assert(s_device_snapshot_store.snapshot.generation == old_generation);
    assert(s_device_snapshot_store.publish_attempted);
    assert(s_device_snapshot_store.publish_result == failure);
    assert(strcmp(s_diagnostic_stage, "inventory_devices") == 0);
    assert(s_diagnostic_error == ESP_ERR_INVALID_RESPONSE);
    esp_err_t effective_error = ESP_OK;
    assert(!inventory_verified(result, &effective_error));
    assert(effective_error == ESP_ERR_INVALID_RESPONSE);
    assert(!homey_data_failure_is_transient(
        effective_error, s_http_status));
    assert(s_favorites_parse_count == 0U);
    assert(s_collection_parse_count == 1U); /* zones only */
    assert(s_request_count + s_favorites_fetch_count == 3U);
    assert(s_snapshot_publish_count == 1U);
    assert_responses_cleaned_once();
}

static void test_failure_preserves_existing_snapshot_and_stops_downstream(void)
{
    assert_publication_failure(PANEL_HOMEY_READ_INVALID, true, 9U);
}

static void test_alias_provider_failure_fails_closed(void)
{
    assert_publication_failure(PANEL_HOMEY_READ_PROVIDER_ERROR, true, 4U);
}

static void test_first_publication_failure_cannot_verify_inventory(void)
{
    assert_publication_failure(PANEL_HOMEY_READ_INVALID, false, 0U);
}

static void test_stale_prior_snapshot_cannot_satisfy_refresh(void)
{
    assert_publication_failure(PANEL_HOMEY_READ_PROVIDER_ERROR, true, 12U);
}

static void test_favorites_failure_is_scoped_and_periodic_snapshots_continue(void)
{
    reset_case();
    s_favorite_error = ESP_ERR_HTTP_FETCH_HEADER;
    s_favorite_status = 0;
    esp_err_t err = athom_cloud_fetch_inventory(&s_cloud_state);
    esp_err_t effective = ESP_FAIL;
    assert(err == ESP_OK && inventory_verified(err, &effective));
    assert(effective == ESP_OK);
    assert(s_snapshot_publish_count == 1U);
    assert(s_device_snapshot_store.snapshot.generation == 1U);
    assert(s_transport_metrics.inventory_snapshot_published);
    assert(s_transport_metrics.favorites_read.error == ESP_ERR_HTTP_FETCH_HEADER);
    assert(s_transport_metrics.favorites_read.http_status == 0);
    assert(s_transport_metrics.favorites_read.transport_observed);
    assert(s_transport_metrics.favorites_read.client_reused);
    assert(!s_transport_metrics.favorites_read.data_verified);
    assert(!s_transport_metrics.favorites_read.response_received);
    assert(!s_transport_metrics.favorites_read.body_complete);
    assert(s_favorites_parse_count == 0U && s_favorites_clear_count > 0U);
    /* A later cached/periodic read still publishes current data without cloud discovery. */
    s_request_count = 0U;
    err = athom_cloud_fetch_inventory_from_cached_session(&s_cloud_state, "synthetic-homey");
    assert(err == ESP_OK && inventory_verified(err, &effective));
    assert(s_device_snapshot_store.snapshot.generation == 2U);
    assert(s_transport_metrics.inventory_read_count == 2U);
}

static void test_favorites_auth_rejection_remains_fail_closed(void)
{
    const int statuses[] = {401, 403};
    for (size_t i = 0U; i < sizeof(statuses)/sizeof(statuses[0]); ++i) {
        reset_case();
        s_favorite_status = statuses[i];
        esp_err_t effective = ESP_OK;
        const esp_err_t result = athom_cloud_fetch_inventory(&s_cloud_state);
        assert(result != ESP_OK && !inventory_verified(result, &effective));
        assert(s_diagnostic_http_status == statuses[i]);
        assert(!s_transport_metrics.favorites_read.data_verified);
        assert(s_favorites_clear_count > 0U);
    }
}

static void test_favorites_schema_or_http_failure_does_not_poison_inventory(void)
{
    reset_case();
    s_favorites_parse_result = 1; /* Production parser validation failure. */
    esp_err_t effective = ESP_FAIL;
    assert(inventory_verified(athom_cloud_fetch_inventory(&s_cloud_state), &effective));
    assert(!s_transport_metrics.favorites_read.data_verified);
    assert(s_favorites_clear_count == 1U);
    reset_case();
    s_favorite_status = 503;
    assert(inventory_verified(athom_cloud_fetch_inventory(&s_cloud_state), &effective));
    assert(s_transport_metrics.favorites_read.http_status == 503);
    assert(!s_transport_metrics.favorites_read.data_verified);
}

static void test_favorites_preflight_does_not_copy_old_transport(void)
{
    reset_case();
    s_transport_metrics.perform_count = 7U;
    s_transport_metrics.last_perform_err = ESP_ERR_HTTP_FETCH_HEADER;
    s_transport_metrics.last_perform_http_status = 200;
    favorites_read_capture(ESP_ERR_INVALID_ARG, 0, 7U, 0U);
    assert(s_transport_metrics.favorites_read.attempted);
    assert(!s_transport_metrics.favorites_read.transport_observed);
    assert(s_transport_metrics.favorites_read.perform_error == 0);
    assert(!s_transport_metrics.favorites_read.response_received);
}

int main(void)
{
    test_favorites_failure_is_scoped_and_periodic_snapshots_continue();
    test_favorites_auth_rejection_remains_fail_closed();
    test_favorites_schema_or_http_failure_does_not_poison_inventory();
    test_favorites_preflight_does_not_copy_old_transport();
    test_successful_publish_advances_existing_generation();
    test_first_successful_publish_establishes_generation_one();
    test_cached_session_inventory_requires_matching_private_state();
    test_cached_session_alias_failure_fails_closed();
    test_failure_preserves_existing_snapshot_and_stops_downstream();
    test_alias_provider_failure_fails_closed();
    test_first_publication_failure_cannot_verify_inventory();
    test_stale_prior_snapshot_cannot_satisfy_refresh();
    puts("PATCH058_INVENTORY_SNAPSHOT_GATE=PASS");
    return 0;
}
