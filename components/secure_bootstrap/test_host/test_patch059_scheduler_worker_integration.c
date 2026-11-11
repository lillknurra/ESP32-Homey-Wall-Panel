#include <setjmp.h>
#include <math.h>
#include "panel_first_failure_diag.h"

typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);
#define eSetBits 0

static uint32_t s_runtime_id;

void runtime_event_ring_record_now(uint16_t event_id, uint8_t stage,
                                   int32_t result, uint32_t value_a,
                                   uint32_t value_b)
{
    (void)runtime_event_ring_record_at(0U, event_id, stage, result,
                                       value_a, value_b);
}

void runtime_event_ring_record_resources(uint16_t event_id, uint8_t stage)
{
    (void)runtime_event_ring_record_at(0U, event_id, stage, 0, 0U, 0U);
}

#ifndef PATCH085_AWNING_2_WRITE_ARMED
#define PATCH085_AWNING_2_WRITE_ARMED 1
#endif
#ifndef PATCH085_AWNING_3_WRITE_ARMED
#define PATCH085_AWNING_3_WRITE_ARMED 1
#endif
#ifndef TAG
#define TAG "PATCH059_TEST"
#endif
#ifndef ESP_ERR_HTTP_INCOMPLETE_DATA
#define ESP_ERR_HTTP_INCOMPLETE_DATA (-28)
#endif
#ifndef ESP_ERR_HTTP_EAGAIN
#define ESP_ERR_HTTP_EAGAIN (-20)
#endif

#ifndef ESP_LOGE
#define ESP_LOGE(tag, ...) test_log((tag), __VA_ARGS__)
#endif

typedef enum {
    ATHOM_REFRESH_ORIGIN_BOOT_AUTO = 0,
    ATHOM_REFRESH_ORIGIN_MANUAL,
    ATHOM_REFRESH_ORIGIN_PERIODIC,
    ATHOM_REFRESH_ORIGIN_LIGHT_RECONCILIATION,
    ATHOM_REFRESH_ORIGIN_PRESELECTION_RESTORE,
} athom_refresh_origin_t;

typedef struct {
    uint32_t opaque;
} athom_transport_metrics_t;

static uint32_t s_runtime_id;
static uint32_t s_test_attempt_sequence;

static uint32_t athom_inventory_attempt_diagnostic_next_sequence(void)
{
    return ++s_test_attempt_sequence;
}

static void first_boot_capture_attempt(
    athom_refresh_origin_t origin, uint32_t attempt, uint32_t inventory_sequence,
    uint64_t started_us, bool verified, esp_err_t final_error,
    int final_http_status, const char *stage,
    const athom_transport_metrics_t *before)
{
    (void)origin; (void)attempt; (void)inventory_sequence; (void)started_us;
    (void)verified; (void)final_error; (void)final_http_status;
    (void)stage; (void)before;
}

static void athom_inventory_attempt_diagnostic_begin(
    athom_transport_metrics_t *baseline,
    uint32_t *diagnostic_revision_baseline)
{
    if (baseline != NULL) baseline->opaque = 0U;
    if (diagnostic_revision_baseline != NULL) *diagnostic_revision_baseline = 0U;
}

static void athom_inventory_attempt_diagnostic_complete(
    athom_refresh_origin_t origin,
    uint32_t attempt,
    esp_err_t error,
    int status,
    const char *stage,
    const athom_transport_metrics_t *baseline,
    uint32_t diagnostic_revision_baseline)
{
    (void)origin; (void)attempt; (void)error; (void)status;
    (void)stage; (void)baseline; (void)diagnostic_revision_baseline;
}

typedef enum {
    ATHOM_REFRESH_QUEUE_OK = 0,
    ATHOM_REFRESH_QUEUE_NOT_READY,
    ATHOM_REFRESH_QUEUE_BUSY,
    ATHOM_REFRESH_QUEUE_FAILED,
} athom_refresh_queue_result_t;

typedef enum {
    ATHOM_AWNING_QUEUE_QUEUED = 0,
    ATHOM_AWNING_QUEUE_QUEUED_NEXT,
    ATHOM_AWNING_QUEUE_INVALID_WIDGET,
    ATHOM_AWNING_QUEUE_NOT_READY,
    ATHOM_AWNING_QUEUE_BUSY,
    ATHOM_AWNING_QUEUE_OPPOSITE_REJECTED,
    ATHOM_AWNING_QUEUE_DEBOUNCED,
    ATHOM_AWNING_QUEUE_LIMIT_REACHED,
    ATHOM_AWNING_QUEUE_FAILED,
} athom_awning_queue_result_t;

typedef struct {
    bool active;
    uint8_t pending_bar_mask;
    uint8_t queued_count;
    bool failed;
    uint8_t guard_state;
    int8_t active_direction;
} athom_awning_interaction_snapshot_t;

typedef enum {
    ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA = 1,
    ATHOM_HOMEY_COMMAND_LIGHT_TOGGLE,
    ATHOM_HOMEY_COMMAND_AWNING_POSITION,
    ATHOM_HOMEY_COMMAND_AWNING_GUARD_COMMISSION,
} athom_homey_command_kind_t;

typedef struct {
    uint8_t selected_homey_digest[32];
    uint32_t alias_generation;
    uint32_t snapshot_generation;
    double current_position;
    double minimum;
    double maximum;
    double step;
    bool step_present;
} athom_cloud_awning_command_context_t;

static athom_cloud_awning_command_context_t s_mock_awning_context;
static bool s_mock_awning_widget_position_override;
static double s_mock_awning_widget_position[3];

typedef struct {
    athom_homey_command_kind_t kind;
    athom_refresh_origin_t origin;
    size_t widget_index;
    bool value;
    int8_t direction;
    double numeric_value;
    athom_cloud_awning_command_context_t awning_context;
    athom_cloud_awning_command_context_t commission_context[2];
} athom_homey_command_t;

typedef enum {
    PANEL_AWNING_GUARD_UNINITIALIZED = 0,
    PANEL_AWNING_GUARD_READY,
    PANEL_AWNING_GUARD_WRITE_PENDING,
    PANEL_AWNING_GUARD_QUARANTINED,
} panel_awning_guard_state_t;
#define PANEL_AWNING_GUARD_IDENTITY_SIZE 32U

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version, size;
    uint8_t state, reserved[3];
    uint32_t sequence;
    uint8_t homey_identity[32];
    uint32_t alias_generation;
    uint32_t crc32;
} panel_awning_guard_record_t;

static panel_awning_guard_state_t s_awning_guard_state;
static panel_awning_guard_record_t s_awning_guard_record;
static bool s_awning_guard_loaded;
static bool s_awning_guard_resolution_pending;
static bool s_awning_commission_review_valid;
static bool s_awning_commission_reserved;
static uint64_t s_awning_commission_review_expires_ms;
static athom_cloud_awning_command_context_t s_awning_commission_review_context[2];
static uint32_t s_awning_guard_resolution_sequence;
static uint8_t s_awning_guard_resolution_step;
static uint32_t s_awning_guard_resolution_generation;
/* This is the simulated durable NVS blob used by the real production adapter.
 * The staged copy changes only at nvs_set_blob and reaches durable storage at
 * nvs_commit. Runtime reset intentionally leaves these bytes intact. */
static panel_awning_guard_record_t s_mock_guard_durable_record;
static bool s_mock_guard_durable_present;
static panel_awning_guard_record_t s_mock_guard_staged_record;
static bool s_mock_guard_staged_present;
static int s_mock_guard_nvs_fault;
static bool s_mock_guard_corrupt_readback;
static unsigned s_mock_guard_nvs_commit_count;
static unsigned s_mock_guard_nvs_write_count;
static unsigned s_total_awning_homey_writes;
static unsigned s_power_loss_context_reads_to_cut;
static bool s_power_loss_during_http_attempt;
static bool s_power_loss_after_http_response;
static bool s_mock_change_alias_on_guard_commit;

enum {
    GUARD_NVS_FAULT_NONE = 0,
    GUARD_NVS_FAULT_OPEN,
    GUARD_NVS_FAULT_GET,
    GUARD_NVS_FAULT_SET,
    GUARD_NVS_FAULT_COMMIT_BEFORE_DURABLE,
    GUARD_NVS_FAULT_COMMIT_AFTER_DURABLE,
    GUARD_NVS_FAULT_READBACK_CORRUPT,
};

typedef uint32_t nvs_handle_t;
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define ESP_ERR_NVS_NOT_FOUND 0x1102

esp_err_t panel_awning_write_guard_load(panel_awning_guard_record_t *out);
esp_err_t panel_awning_write_guard_initialize_absent(
    const uint8_t homey_identity[PANEL_AWNING_GUARD_IDENTITY_SIZE],
    uint32_t alias_generation, panel_awning_guard_record_t *ready_out);
esp_err_t panel_awning_write_guard_begin(
    const panel_awning_guard_record_t *expected_ready,
    panel_awning_guard_record_t *pending_out);
esp_err_t panel_awning_write_guard_resolve(
    const panel_awning_guard_record_t *expected_pending,
    panel_awning_guard_record_t *ready_out);

esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle)
{
    assert(strcmp(name, "hpanel_awning") == 0);
    if (s_mock_guard_nvs_fault == GUARD_NVS_FAULT_OPEN) return ESP_FAIL;
    *handle = (nvs_handle_t)mode;
    if (mode == NVS_READWRITE) {
        s_mock_guard_staged_record = s_mock_guard_durable_record;
        s_mock_guard_staged_present = s_mock_guard_durable_present;
    }
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) { (void)handle; }

esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out,
                       size_t *size)
{
    (void)handle;
    assert(strcmp(key, "guard_v1") == 0 && out != NULL && size != NULL);
    if (s_mock_guard_nvs_fault == GUARD_NVS_FAULT_GET) return ESP_FAIL;
    if (!s_mock_guard_durable_present) return ESP_ERR_NVS_NOT_FOUND;
    if (*size < sizeof(s_mock_guard_durable_record)) return ESP_ERR_INVALID_SIZE;
    memcpy(out, &s_mock_guard_durable_record,
           sizeof(s_mock_guard_durable_record));
    *size = sizeof(s_mock_guard_durable_record);
    if (s_mock_guard_corrupt_readback) {
        ((uint8_t *)out)[0] ^= 0x01U;
        s_mock_guard_corrupt_readback = false;
    }
    return ESP_OK;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key,
                       const void *data, size_t size)
{
    (void)handle;
    assert(strcmp(key, "guard_v1") == 0 && data != NULL);
    if (s_mock_guard_nvs_fault == GUARD_NVS_FAULT_SET) return ESP_FAIL;
    assert(size == sizeof(s_mock_guard_staged_record));
    memcpy(&s_mock_guard_staged_record, data, size);
    s_mock_guard_staged_present = true;
    s_mock_guard_nvs_write_count++;
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    s_mock_guard_nvs_commit_count++;
    if (s_mock_guard_nvs_fault == GUARD_NVS_FAULT_COMMIT_BEFORE_DURABLE)
        return ESP_FAIL;
    if (s_mock_guard_staged_present) {
        s_mock_guard_durable_record = s_mock_guard_staged_record;
        s_mock_guard_durable_present = true;
    }
    if (s_mock_change_alias_on_guard_commit) {
        s_mock_awning_context.alias_generation++;
        s_mock_change_alias_on_guard_commit = false;
    }
    if (s_mock_guard_nvs_fault == GUARD_NVS_FAULT_COMMIT_AFTER_DURABLE)
        return ESP_FAIL;
    if (s_mock_guard_nvs_fault == GUARD_NVS_FAULT_READBACK_CORRUPT)
        s_mock_guard_corrupt_readback = true;
    return ESP_OK;
}

static uint32_t test_guard_crc32(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0U; i < size; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0U; bit < 8U; ++bit)
            crc = (crc >> 1U) ^ ((0U - (crc & 1U)) & 0xedb88320U);
    }
    return ~crc;
}

static void seed_durable_guard_ready(
    const athom_cloud_awning_command_context_t *context)
{
    memset(&s_mock_guard_durable_record, 0,
           sizeof(s_mock_guard_durable_record));
    s_mock_guard_durable_record.magic = 0x41574731U;
    s_mock_guard_durable_record.version = 1U;
    s_mock_guard_durable_record.size =
        (uint16_t)sizeof(s_mock_guard_durable_record);
    s_mock_guard_durable_record.state = PANEL_AWNING_GUARD_READY;
    s_mock_guard_durable_record.sequence = 1U;
    memcpy(s_mock_guard_durable_record.homey_identity,
           context->selected_homey_digest,
           sizeof(s_mock_guard_durable_record.homey_identity));
    s_mock_guard_durable_record.alias_generation = context->alias_generation;
    s_mock_guard_durable_record.crc32 = 0U;
    s_mock_guard_durable_record.crc32 = test_guard_crc32(
        &s_mock_guard_durable_record, sizeof(s_mock_guard_durable_record));
    s_mock_guard_durable_present = true;
}

typedef enum {
    ATHOM_HOMEY_AWNING_WRITE_ACCEPTED = 0,
    ATHOM_HOMEY_AWNING_WRITE_INVALID_ARGUMENT,
    ATHOM_HOMEY_AWNING_WRITE_NOT_READY,
    ATHOM_HOMEY_AWNING_WRITE_TARGET_INVALID,
    ATHOM_HOMEY_AWNING_WRITE_UNAUTHORIZED,
    ATHOM_HOMEY_AWNING_WRITE_REJECTED,
    ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS,
    ATHOM_HOMEY_AWNING_WRITE_INTERNAL_ERROR,
} athom_homey_awning_write_result_t;

static athom_homey_awning_write_result_t patch085_classify_awning_write(
    bool write_attempted, bool complete_response_received, int http_status,
    esp_err_t perform_error);

#ifndef PANEL_HOMEY_SNAPSHOT_INSPECTION_DEFINED
#define PANEL_HOMEY_SNAPSHOT_INSPECTION_DEFINED
typedef struct {
    bool present;
    panel_homey_read_result_t result;
    panel_homey_read_snapshot_t snapshot;
    uint64_t age_ms;
} panel_homey_snapshot_inspection_t;
#endif

typedef struct {
    panel_homey_read_result_t result;
    bool present;
    bool fresh;
    uint64_t age_ms;
    uint64_t captured_at_ms;
    uint32_t generation;
} panel_homey_snapshot_freshness_t;

typedef struct {
    bool has_command;
    athom_homey_command_t command;
    unsigned queued_count;
} patch059_runtime_queue_t;

typedef enum {
    ATHOM_HOMEY_DATA_LOADING = 0,
    ATHOM_HOMEY_DATA_READY,
    ATHOM_HOMEY_DATA_ERROR,
    ATHOM_HOMEY_DATA_RETRYING,
} athom_homey_data_state_t;

typedef enum {
    ATHOM_LIGHT_TOGGLE_DISPATCH_OK = 0,
    ATHOM_LIGHT_TOGGLE_DISPATCH_INVALID_WIDGET,
    ATHOM_LIGHT_TOGGLE_DISPATCH_NOT_READY,
} athom_light_toggle_dispatch_result_t;

typedef enum {
    ATHOM_LIGHT_WRITE_OK = 0,
} athom_homey_light_write_result_t;

static patch059_runtime_queue_t s_patch059_queue;
static bool s_wifi_online;
static bool s_worker_running;
static bool s_select_worker_running;
static bool s_restore_worker_running;
static bool s_preselection_restore_worker_running;
static bool s_preselection_restore_pending;
static int s_preselection_restore_mux;
static bool s_refresh_job_reserved;
static bool s_no_snapshot_recovery_pending;
static uint32_t s_no_snapshot_recovery_sequence;
static uint32_t s_no_snapshot_recovery_delay_ms;
static bool s_awning_job_reserved;
static bool s_light_toggle_job_reserved;
static bool s_schema_refresh_running;
static bool s_diag_probe_active;
static athom_network_phase_owner_t s_network_phase_owner;
static int s_patch059_network_mux;
static bool s_dashboard_visible;
static bool s_provisioning_runtime_ready;
static unsigned s_worker_inventory_commands;
static unsigned s_periodic_inventory_attempts;
static unsigned s_discovery_attempts;
static unsigned s_select_connect_attempts;
static unsigned s_write_attempts;
static bool s_awning_motion_uncertain;
static bool s_awning_write_result_ready;
typedef enum {
    PATCH085_AWNING_OUTCOME_NOT_ATTEMPTED = 0,
    PATCH085_AWNING_OUTCOME_ACCEPTED,
    PATCH085_AWNING_OUTCOME_REJECTED,
    PATCH085_AWNING_OUTCOME_AMBIGUOUS,
} patch085_awning_outcome_t;
static patch085_awning_outcome_t s_awning_outcome;
static bool s_awning_initial_enqueue_pending;
static size_t s_awning_pending_widget;
static double s_awning_target_position;
static uint32_t s_awning_confirmation_after_generation;
static athom_cloud_awning_command_context_t s_awning_context;
static panel_roller_interaction_t s_awning_interaction[3];
static uint8_t s_awning_interaction_identity[3][PANEL_HOMEY_IDENTITY_DIGEST_SIZE];
static uint32_t s_awning_interaction_alias_generation[3];
static bool s_awning_next_dispatch_pending;
static size_t s_awning_next_dispatch_widget;
static bool s_mock_awning_context_valid;
static bool s_mock_confirm_snapshot_on_context_read;
static bool s_mock_run_worker_after_confirm_context_read;
static double s_mock_confirmed_snapshot_position;
static uint32_t s_mock_confirmed_snapshot_generation;
static bool s_mock_queue_other_widget_after_snapshot;
static athom_awning_queue_result_t s_mock_other_widget_queue_result;
static bool s_mock_queue_same_widget_after_snapshot;
static athom_awning_queue_result_t s_mock_same_widget_queue_result;
static unsigned s_mock_context_taps_remaining;
static bool s_mock_context_tap_in_progress;
static athom_awning_queue_result_t s_mock_context_tap_result;
static bool s_mock_awning_queue_fail;
static bool s_mock_awning_reenter_on_queue_failure;
static athom_awning_queue_result_t s_mock_awning_reenter_result;
static athom_homey_awning_write_result_t s_mock_awning_write_result;
static bool s_mock_awning_use_http_response;
static bool s_mock_awning_response_complete;
static int s_mock_awning_http_status;
static esp_err_t s_mock_awning_perform_error;
static unsigned s_mock_awning_put_calls;
static unsigned s_mock_awning_dispatch_calls;
static bool s_observe_snapshot_during_awning_put;
static bool s_mock_awning_tap_during_put;
static athom_awning_queue_result_t s_mock_awning_tap_result;
static bool s_mock_selected_homey_valid;
static bool s_mock_active_alias_valid;
static bool s_mock_active_nvs_binding_matches;
static unsigned s_scheduler_iterations;
static unsigned s_scheduler_iteration_limit;
static athom_refresh_origin_t s_current_refresh_origin;
static int s_mock_discovery_status;
static int s_mock_discovery_status_sequence[4];
static unsigned s_mock_discovery_status_sequence_count;
static unsigned s_mock_discovery_status_sequence_index;
static unsigned s_mock_cloud_refresh_calls;
static bool s_mock_cloud_refresh_succeeds;
static bool s_mock_refresh_token_available;
static bool s_worker_active;
static TaskHandle_t s_homey_command_worker_task;
static uint32_t s_worker_wake_bits;
static unsigned s_worker_notify_count;
static void (*s_notify_wait_interleave)(void);
static jmp_buf s_scheduler_exit;
static jmp_buf s_worker_exit;
static int s_patch059_refresh_mux;
static int s_patch059_diag_mux;

static athom_awning_queue_result_t athom_oauth_runtime_queue_awning_step(
    size_t widget_index, int8_t direction);
bool athom_oauth_runtime_awning_interaction_snapshot(
    size_t widget_index, athom_awning_interaction_snapshot_t *out);

static void homey_command_worker(void *arg);
static void homey_command_worker_run_deferred_work(void);
void athom_oauth_runtime_awning_snapshot_observed(void);

#define ATHOM_HOMEY_ID_MAX 64U
#define PERIODIC_REFRESH_INTERVAL_MS 60000ULL
#define PERIODIC_REFRESH_BUSY_DEFER_MS 5000ULL
#define PERIODIC_REFRESH_COOLDOWN_MS 30000ULL
#define PERIODIC_REFRESH_SCHEDULER_POLL_MS 1000U
#define ATHOM_BOOT_AUTO_READY_WAIT_MS 1000U
#define ATHOM_PRESELECT_RESTORE_RETRY_MS 2000U
#define ATHOM_HOMEY_DATA_RETRY_1_MS 1000U
#define ATHOM_HOMEY_DATA_RETRY_2_MS 2000U
#define ATHOM_HOMEY_DATA_RETRY_3_MS 4000U
#define ATHOM_HOMEY_DATA_RETRY_MAX_MS 8000U
#define ATHOM_BOOT_AUTO_TRANSIENT_ATTEMPT_MAX 3U
#define ATHOM_HOMEY_DATA_429_RETRY_1_MS 60000U
#define ATHOM_HOMEY_DATA_429_RETRY_2_MS 120000U
#define ATHOM_HOMEY_DATA_429_RETRY_3_MS 240000U
#define ATHOM_HOMEY_DATA_429_RETRY_MAX_MS 300000U
#define portMAX_DELAY UINT32_MAX
#define pdTRUE 1
#define pdPASS 1
#define pdMS_TO_TICKS(milliseconds) (milliseconds)
#define portENTER_CRITICAL(mux) ((void)(mux))
#define portEXIT_CRITICAL(mux) ((void)(mux))
#define s_cloud s_cloud_state
#define s_homey_command_queue (&s_patch059_queue)
#define s_refresh_job_mux s_patch059_refresh_mux
#define s_patch031_diag_probe_mux s_patch059_diag_mux
#define s_network_phase_mux s_patch059_network_mux

static athom_homey_data_state_t s_homey_data_state;
static const char *s_state_name;
typedef struct {
    uint32_t phase, completed_phase, failed_phase;
    int32_t init_error;
    uint16_t handler_count, handler_capacity;
    uint8_t auth_restore_started, command_worker_created;
    uint8_t periodic_scheduler_started, inventory_queued;
    uint8_t inventory_in_flight, recovery_pending, retry_budget_remaining;
    uint32_t last_progress_ms;
} homey_boot_progress_t;
static homey_boot_progress_t s_boot_progress;
static int s_boot_progress_mux;

static void runtime_diag_emit(uint16_t event_type, uint16_t result,
                              int32_t error_code, int http_status,
                              unsigned attempt, uint32_t retry_delay_ms,
                              uint32_t snapshot_generation, uint8_t origin,
                              uint8_t stage)
{
    (void)event_type;
    (void)result;
    (void)error_code;
    (void)http_status;
    (void)attempt;
    (void)retry_delay_ms;
    (void)snapshot_generation;
    (void)origin;
    (void)stage;
}

static void set_homey_data_state(athom_homey_data_state_t state)
{
    s_homey_data_state = state;
}

static bool patch031_diag_probe_active_locked(void)
{
    return s_diag_probe_active;
}

static bool phone_provisioning_wifi_online(void)
{
    return s_wifi_online;
}

static bool phone_provisioning_homey_runtime_ready(void)
{
    return s_provisioning_runtime_ready;
}

static bool athom_cloud_get_awning_command_context(
    const athom_cloud_state_t *state, size_t widget, athom_cloud_awning_command_context_t *out)
{
    (void)state;
    if ((widget != 1U && widget != 2U) || out == NULL ||
        !s_mock_awning_context_valid || !s_mock_selected_homey_valid ||
        !s_mock_active_alias_valid) return false;
    if (s_mock_confirm_snapshot_on_context_read) {
        s_mock_confirm_snapshot_on_context_read = false;
        s_mock_awning_context.snapshot_generation =
            s_mock_confirmed_snapshot_generation;
        s_mock_awning_context.current_position =
            s_mock_confirmed_snapshot_position;
        athom_oauth_runtime_awning_snapshot_observed();
        if (s_mock_run_worker_after_confirm_context_read) {
            s_mock_run_worker_after_confirm_context_read = false;
            homey_command_worker_run_deferred_work();
        }
        if (s_mock_queue_other_widget_after_snapshot) {
            s_mock_queue_other_widget_after_snapshot = false;
            s_mock_other_widget_queue_result = athom_oauth_runtime_queue_awning_step(
                widget == 1U ? 2U : 1U, PANEL_ROLLER_DIRECTION_UP);
        }
        if (s_mock_queue_same_widget_after_snapshot) {
            s_mock_queue_same_widget_after_snapshot = false;
            s_mock_same_widget_queue_result = athom_oauth_runtime_queue_awning_step(
                widget, PANEL_ROLLER_DIRECTION_UP);
        }
    }
    if (s_worker_active && s_power_loss_context_reads_to_cut > 0U &&
        --s_power_loss_context_reads_to_cut == 0U) {
        longjmp(s_worker_exit, 1);
    }
    if (s_mock_context_taps_remaining > 0U &&
        !s_mock_context_tap_in_progress) {
        s_mock_context_taps_remaining--;
        s_mock_context_tap_in_progress = true;
        s_now_us += 500000LL;
        s_mock_context_tap_result = athom_oauth_runtime_queue_awning_step(
            widget, PANEL_ROLLER_DIRECTION_UP);
        s_mock_context_tap_in_progress = false;
    }
    *out = s_mock_awning_context;
    if (s_mock_awning_widget_position_override) {
        out->current_position = s_mock_awning_widget_position[widget];
    }
    return true;
}

static bool athom_cloud_awning_write_is_armed(size_t widget)
{
    if (widget == 1U) return PATCH085_AWNING_2_WRITE_ARMED == 1;
    if (widget == 2U) return PATCH085_AWNING_3_WRITE_ARMED == 1;
    return false;
}

static athom_homey_awning_write_result_t athom_cloud_set_awning_position(
    athom_cloud_state_t *state, size_t widget, double target,
    const athom_cloud_awning_command_context_t *expected)
{
    (void)state;
    assert(expected != NULL && (widget == 1U || widget == 2U));
    assert(athom_cloud_awning_write_is_armed(widget));
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_WRITE_PENDING);
    assert(s_mock_guard_durable_present &&
           s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_WRITE_PENDING);
    s_mock_awning_dispatch_calls++;
    if (!s_mock_active_nvs_binding_matches) {
        return ATHOM_HOMEY_AWNING_WRITE_NOT_READY;
    }
    assert(fabs(target - (widget == 1U ? s_awning_target_position : s_awning_target_position)) < 1e-9);
    athom_awning_interaction_snapshot_t movement = {0};
    assert(athom_oauth_runtime_awning_interaction_snapshot(widget, &movement));
    assert(movement.active_direction == 0); /* HTTP result is not accepted yet */
    s_mock_awning_put_calls++;
    s_total_awning_homey_writes++;
    if (s_mock_awning_tap_during_put) {
        s_mock_awning_tap_during_put = false;
        s_now_us += 500000LL;
        s_mock_awning_tap_result = athom_oauth_runtime_queue_awning_step(
            widget, PANEL_ROLLER_DIRECTION_UP);
    }
    if (s_power_loss_during_http_attempt) {
        s_power_loss_during_http_attempt = false;
        longjmp(s_worker_exit, 1);
    }
    if (s_observe_snapshot_during_awning_put) {
        s_mock_awning_context.snapshot_generation++;
        s_mock_awning_context.current_position = target;
        athom_oauth_runtime_awning_snapshot_observed();
    }
    if (s_mock_awning_use_http_response) {
        return patch085_classify_awning_write(
            true, s_mock_awning_response_complete,
            s_mock_awning_http_status, s_mock_awning_perform_error);
    }
    return s_mock_awning_write_result;
}

static int xQueueSend(void *queue, const athom_homey_command_t *command, int wait)
{
    (void)wait;
    assert(queue == &s_patch059_queue);
    if (command->kind == ATHOM_HOMEY_COMMAND_AWNING_POSITION &&
        s_mock_awning_queue_fail) {
        if (s_mock_awning_reenter_on_queue_failure) {
            s_mock_awning_reenter_on_queue_failure = false;
            s_mock_awning_reenter_result = athom_oauth_runtime_queue_awning_step(
                command->widget_index, (int8_t)command->direction);
        }
        return 0;
    }
    if (s_patch059_queue.has_command) return 0;
    s_patch059_queue.has_command = true;
    s_patch059_queue.command = *command;
    s_patch059_queue.queued_count++;
    return pdTRUE;
}

static int xQueueReceive(
    void *queue, athom_homey_command_t *command, uint32_t wait)
{
    (void)wait;
    assert(queue == &s_patch059_queue);
    if (s_patch059_queue.has_command) {
        *command = s_patch059_queue.command;
        s_patch059_queue.has_command = false;
        if (command->kind == ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA) {
            s_worker_inventory_commands++;
            s_current_refresh_origin = command->origin;
        }
        return pdTRUE;
    }
    /* The production worker now polls the existing queue nonblocking before
     * waiting on its task notification; the wait mock owns idle exit. */
    return 0;
}

static unsigned uxQueueMessagesWaiting(void *queue)
{
    assert(queue == &s_patch059_queue);
    return s_patch059_queue.has_command ? 1U : 0U;
}

static TaskHandle_t xTaskGetCurrentTaskHandle(void)
{
    return s_worker_active ? s_homey_command_worker_task :
        (TaskHandle_t)(uintptr_t)2U;
}

static int xTaskNotify(TaskHandle_t task, uint32_t value, int action)
{
    assert(task == s_homey_command_worker_task);
    assert(action == eSetBits);
    s_worker_wake_bits |= value;
    s_worker_notify_count++;
    return pdPASS;
}

static int xTaskNotifyWait(uint32_t clear_on_entry, uint32_t clear_on_exit,
                           uint32_t *value, uint32_t wait_ticks)
{
    (void)wait_ticks;
    s_worker_wake_bits &= ~clear_on_entry;
    if (s_notify_wait_interleave != NULL) {
        void (*interleave)(void) = s_notify_wait_interleave;
        s_notify_wait_interleave = NULL;
        interleave();
    }
    if (s_worker_wake_bits != 0U) {
        if (value != NULL) *value = s_worker_wake_bits;
        s_worker_wake_bits &= ~clear_on_exit;
        return pdTRUE;
    }
    if (s_worker_active) longjmp(s_worker_exit, 1);
    return 0;
}

static void run_one_worker_command(void)
{
    if (!s_patch059_queue.has_command) return;
    s_worker_active = true;
    if (setjmp(s_worker_exit) == 0) {
        homey_command_worker(NULL);
    }
    s_worker_active = false;
}

static void vTaskDelay(uint32_t ticks)
{
    if (s_worker_active) return;
    s_now_us += (int64_t)ticks * 1000LL;
    run_one_worker_command();
    s_scheduler_iterations++;
    if (s_scheduler_iterations >= s_scheduler_iteration_limit) {
        longjmp(s_scheduler_exit, 1);
    }
}

static panel_homey_read_result_t athom_cloud_inspect_device_snapshot(
    uint64_t now_ms, panel_homey_snapshot_inspection_t *out)
{
    memset(out, 0, sizeof(*out));
    out->present = s_device_snapshot_store.active;
    if (!out->present) {
        out->result = PANEL_HOMEY_READ_NOT_FOUND;
        return out->result;
    }
    out->snapshot = s_device_snapshot_store.snapshot;
    if (now_ms < out->snapshot.captured_at_ms) {
        out->result = PANEL_HOMEY_READ_INVALID;
        return out->result;
    }
    out->age_ms = now_ms - out->snapshot.captured_at_ms;
    out->result = out->age_ms > PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS
        ? PANEL_HOMEY_READ_STALE : PANEL_HOMEY_READ_OK;
    return out->result;
}

static panel_homey_read_result_t athom_cloud_inspect_device_snapshot_freshness(
    uint64_t now_ms, panel_homey_snapshot_freshness_t *out)
{
    memset(out, 0, sizeof(*out));
    out->present = s_device_snapshot_store.active;
    if (!out->present) {
        out->result = PANEL_HOMEY_READ_NOT_FOUND;
        return out->result;
    }
    out->captured_at_ms = s_device_snapshot_store.snapshot.captured_at_ms;
    out->generation = s_device_snapshot_store.snapshot.generation;
    if (now_ms < out->captured_at_ms) {
        out->result = PANEL_HOMEY_READ_INVALID;
        return out->result;
    }
    out->age_ms = now_ms - out->captured_at_ms;
    out->fresh = out->age_ms <= PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS;
    out->result = out->fresh ? PANEL_HOMEY_READ_OK : PANEL_HOMEY_READ_STALE;
    return out->result;
}

static panel_homey_read_result_t athom_cloud_copy_device_snapshot(
    uint64_t now_ms, panel_homey_read_snapshot_t *out)
{
    panel_homey_snapshot_inspection_t inspection;
    const panel_homey_read_result_t result =
        athom_cloud_inspect_device_snapshot(now_ms, &inspection);
    if (result == PANEL_HOMEY_READ_OK) *out = inspection.snapshot;
    return result;
}

static esp_err_t athom_cloud_fetch_user_homeys(athom_cloud_state_t *state)
{
    assert(state != NULL);
    s_discovery_attempts++;
    if (s_current_refresh_origin == ATHOM_REFRESH_ORIGIN_PERIODIC) {
        s_periodic_inventory_attempts++;
    }
    int response_status = s_mock_discovery_status;
    if (s_mock_discovery_status_sequence_index <
        s_mock_discovery_status_sequence_count) {
        response_status = s_mock_discovery_status_sequence[
            s_mock_discovery_status_sequence_index++];
    }
    if (response_status != 200) {
        s_http_status = response_status;
        s_diagnostic_http_status = response_status;
        s_diagnostic_error = response_status == 401
            ? ESP_ERR_INVALID_STATE : ESP_FAIL;
        s_diagnostic_stage = "oauth_user_me_http";
        return s_diagnostic_error;
    }
    s_http_status = 200;
    s_diagnostic_http_status = 200;
    s_diagnostic_error = ESP_OK;
    s_diagnostic_stage = "homeys_complete";
    return ESP_OK;
}

static esp_err_t athom_cloud_refresh(athom_cloud_state_t *state)
{
    if (state == NULL || !s_mock_refresh_token_available) return ESP_FAIL;
    s_mock_cloud_refresh_calls++;
    return s_mock_cloud_refresh_succeeds ? ESP_OK : ESP_FAIL;
}

static esp_err_t athom_cloud_select_and_connect(
    athom_cloud_state_t *state, const char *homey_id)
{
    assert(state != NULL && homey_id != NULL);
    s_select_connect_attempts++;
    s_diagnostic_http_status = 200;
    return ESP_OK;
}

static int64_t elapsed_ms_since(int64_t started_at_us)
{
    return (esp_timer_get_time() - started_at_us) / 1000LL;
}

static void patch021_homey_phase_log(
    const char *phase, unsigned attempt, int64_t started_at_us,
    esp_err_t error, int status)
{
    (void)phase;
    (void)attempt;
    (void)started_at_us;
    (void)error;
    (void)status;
}

static void patch021_homey_remote_log(
    const char *phase, const char *origin, const char *result,
    unsigned attempt, uint32_t elapsed_ms, uint32_t delay_ms,
    esp_err_t error, int status, const char *stage, bool transient)
{
    (void)phase; (void)origin; (void)result; (void)attempt;
    (void)elapsed_ms; (void)delay_ms; (void)error; (void)status;
    (void)stage; (void)transient;
}

static void publish_cloud_state(void) {}

static bool patch038_refresh_authoritative_state_after_write(void)
{
    if (s_worker_active && s_power_loss_after_http_response) {
        s_power_loss_after_http_response = false;
        longjmp(s_worker_exit, 1);
    }
    return false;
}

static void phone_provisioning_show_live_ready(const char *name)
{
    (void)name;
}

static athom_light_toggle_dispatch_result_t
athom_oauth_runtime_dispatch_light_toggle(size_t widget_index, bool value);

static bool patch038_dispatch_result_requires_authoritative_refresh(
    athom_light_toggle_dispatch_result_t result)
{
    (void)result;
    return false;
}

static void patch038_complete_light_toggle_job(void) {}

static const char *patch037_light_toggle_dispatch_result_name(
    athom_light_toggle_dispatch_result_t result)
{
    (void)result;
    return "test";
}

static athom_homey_light_write_result_t athom_cloud_set_favorite_light_onoff(
    athom_cloud_state_t *state, size_t widget_index, bool value)
{
    (void)state; (void)widget_index; (void)value;
    s_write_attempts++;
    return ATHOM_LIGHT_WRITE_OK;
}

static bool panel_homey_favorites_light_toggle_execution_ready(
    size_t widget_index, bool runtime_ready)
{
    (void)widget_index;
    return runtime_ready;
}

static bool patch037_light_toggle_dispatch_gate(
    size_t widget_index, bool runtime_ready, bool execution_ready)
{
    (void)widget_index;
    return runtime_ready && execution_ready;
}

static athom_light_toggle_dispatch_result_t patch037_map_light_write_result(
    athom_homey_light_write_result_t result)
{
    (void)result;
    return ATHOM_LIGHT_TOGGLE_DISPATCH_OK;
}

static void preselection_restore_worker(void *arg)
{
    (void)arg;
}

static int xTaskCreate(TaskFunction_t function, const char *name,
                       uint32_t stack_bytes, void *arg, unsigned priority,
                       TaskHandle_t *task_handle)
{
    assert(function == preselection_restore_worker);
    assert(name != NULL && stack_bytes == 12288U && priority == 5U);
    (void)arg;
    if (task_handle != NULL) *task_handle = NULL;
    return pdPASS;
}

/* Diagnostic capture is tested in its production-linked focused runner. */
bool panel_first_failure_diag_rgb_snapshot(panel_first_failure_rgb_t *out)
{
    if (out != NULL) *out = (panel_first_failure_rgb_t){0};
    return false;
}
static void first_failure_capture_attempt(
    athom_refresh_origin_t origin, unsigned attempt, uint32_t inventory_sequence,
    uint64_t started_us, esp_err_t final_error, int final_http_status,
    const char *stage, const athom_transport_metrics_t *before,
    const panel_first_failure_rgb_t *rgb_before, bool rgb_before_valid)
{
    (void)origin; (void)attempt; (void)inventory_sequence; (void)started_us;
    (void)final_error; (void)final_http_status; (void)stage; (void)before;
    (void)rgb_before; (void)rgb_before_valid;
}

#define PATCH059_RUNTIME_PRODUCTION_FUNCTIONS

static void reset_patch059_integration_case(
    uint32_t generation, uint64_t captured_at_ms,
    athom_homey_data_state_t runtime_state)
{
    reset_case();
    memset(&s_patch059_queue, 0, sizeof(s_patch059_queue));
    s_wifi_online = true;
    s_worker_running = false;
    s_select_worker_running = false;
    s_restore_worker_running = false;
    s_preselection_restore_worker_running = false;
    s_preselection_restore_pending = false;
    s_refresh_job_reserved = false;
    s_no_snapshot_recovery_pending = false;
    s_no_snapshot_recovery_sequence = 0U;
    s_no_snapshot_recovery_delay_ms = PERIODIC_REFRESH_COOLDOWN_MS;
    s_light_toggle_job_reserved = false;
    s_schema_refresh_running = false;
    s_diag_probe_active = false;
    s_network_phase_owner = ATHOM_NETWORK_PHASE_NONE;
    s_dashboard_visible = true;
    s_provisioning_runtime_ready = true;
    s_worker_inventory_commands = 0U;
    s_periodic_inventory_attempts = 0U;
    s_discovery_attempts = 0U;
    s_select_connect_attempts = 0U;
    s_mock_discovery_status = 200;
    memset(s_mock_discovery_status_sequence, 0,
           sizeof(s_mock_discovery_status_sequence));
    s_mock_discovery_status_sequence_count = 0U;
    s_mock_discovery_status_sequence_index = 0U;
    s_mock_cloud_refresh_calls = 0U;
    s_mock_cloud_refresh_succeeds = false;
    s_mock_refresh_token_available = false;
    s_cloud_state.homeys.count = 1U;
    strcpy(s_cloud_state.homeys.items[0].id, "synthetic-homey");
    strcpy(s_cloud_state.homeys.items[0].remote_url, "https://synthetic.invalid");
    s_write_attempts = 0U;
    s_scheduler_iterations = 0U;
    s_scheduler_iteration_limit = 3U;
    s_worker_active = false;
    s_homey_command_worker_task = (TaskHandle_t)(uintptr_t)1U;
    s_worker_wake_bits = 0U;
    s_worker_notify_count = 0U;
    s_notify_wait_interleave = NULL;
    s_homey_data_state = runtime_state;
    s_state_name = "homey_connection_error";
    s_device_snapshot_store.active = true;
    s_device_snapshot_store.snapshot.generation = generation;
    s_device_snapshot_store.snapshot.captured_at_ms = captured_at_ms;
}

static void run_periodic_scheduler_for_bounded_ticks(void)
{
    if (setjmp(s_scheduler_exit) == 0) {
        periodic_inventory_refresh_scheduler(NULL);
    }
}

static void assert_single_successful_periodic_republication(
    uint32_t generation, uint64_t old_captured_at_ms,
    athom_homey_data_state_t runtime_state, bool stale_expected)
{
    const int64_t requested_now_us = s_now_us;
    reset_patch059_integration_case(
        generation, old_captured_at_ms, runtime_state);
    s_now_us = requested_now_us;
    const uint64_t start_ms = (uint64_t)(s_now_us / 1000LL);
    assert(start_ms >= old_captured_at_ms);
    const uint64_t age_ms = start_ms - old_captured_at_ms;
    assert(age_ms >= PERIODIC_REFRESH_INTERVAL_MS);
    panel_homey_snapshot_inspection_t before;
    const panel_homey_read_result_t before_result =
        athom_cloud_inspect_device_snapshot(start_ms, &before);
    assert(before.present && before.age_ms == age_ms);
    assert((before_result == PANEL_HOMEY_READ_STALE) == stale_expected);

    run_periodic_scheduler_for_bounded_ticks();

    assert(s_patch059_queue.queued_count == 1U);
    assert(!s_patch059_queue.has_command);
    assert(s_worker_inventory_commands == 1U);
    assert(s_periodic_inventory_attempts == 0U);
    assert(s_discovery_attempts == 0U);
    assert(s_select_connect_attempts == 0U);
    assert(s_alias_activation_count == 1U);
    assert(s_snapshot_publish_count == 1U);
    assert(s_device_snapshot_store.active);
    assert(s_device_snapshot_store.snapshot.generation == generation + 1U);
    assert(s_device_snapshot_store.snapshot.captured_at_ms > old_captured_at_ms);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(s_diagnostic_stage != NULL &&
           strcmp(s_diagnostic_stage, "inventory_complete") == 0);
    assert(s_scheduler_iterations == s_scheduler_iteration_limit);

    const uint64_t published_ms =
        s_device_snapshot_store.snapshot.captured_at_ms;
    panel_homey_snapshot_inspection_t after;
    assert(athom_cloud_inspect_device_snapshot(
        published_ms, &after) == PANEL_HOMEY_READ_OK);
    assert(after.present && after.age_ms == 0U);
    panel_homey_read_snapshot_t dashboard_snapshot = {0};
    assert(athom_cloud_copy_device_snapshot(
        published_ms, &dashboard_snapshot) == PANEL_HOMEY_READ_OK);
    assert(dashboard_snapshot.generation == generation + 1U);
    assert(s_patch059_queue.queued_count == 1U);
    assert(s_write_attempts == 0U);
}

static void test_fresh_due_scheduler_worker_publication_and_new_basis(void)
{
    s_now_us = 60000000LL;
    assert_single_successful_periodic_republication(
        40U, 0U, ATHOM_HOMEY_DATA_READY, false);
}

static void test_stale_snapshot_remains_eligible_and_recovers(void)
{
    s_now_us = 121000000LL;
    assert_single_successful_periodic_republication(
        51U, 0U, ATHOM_HOMEY_DATA_READY, true);
}

static void test_nonready_runtime_can_recover_without_opening_write_gate(void)
{
    s_now_us = 121000000LL;
    reset_patch059_integration_case(63U, 0U, ATHOM_HOMEY_DATA_ERROR);
    s_now_us = 121000000LL;
    assert(s_dashboard_visible);
    assert(athom_oauth_runtime_dispatch_light_toggle(4U, true) ==
           ATHOM_LIGHT_TOGGLE_DISPATCH_NOT_READY);
    assert(s_write_attempts == 0U);

    run_periodic_scheduler_for_bounded_ticks();

    assert(s_patch059_queue.queued_count == 1U);
    assert(s_worker_inventory_commands == 1U);
    assert(s_periodic_inventory_attempts == 0U);
    assert(s_discovery_attempts == 0U);
    assert(s_select_connect_attempts == 0U);
    assert(s_alias_activation_count == 1U);
    assert(s_snapshot_publish_count == 1U);
    assert(s_device_snapshot_store.snapshot.generation == 64U);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(s_write_attempts == 0U);
    panel_homey_read_snapshot_t snapshot = {0};
    assert(athom_cloud_copy_device_snapshot(
        s_device_snapshot_store.snapshot.captured_at_ms,
        &snapshot) == PANEL_HOMEY_READ_OK);
    assert(snapshot.generation == 64U);
}

static void test_manual_429_falls_back_to_valid_cached_session(void)
{
    reset_patch059_integration_case(80U, 0U, ATHOM_HOMEY_DATA_READY);
    s_now_us = 121000000LL;
    s_mock_discovery_status = 429;
    s_current_refresh_origin = ATHOM_REFRESH_ORIGIN_MANUAL;
    bool cloud_429_seen = false;
    const esp_err_t err = connect_and_fetch_inventory_for_origin(
        "synthetic-homey", ATHOM_REFRESH_ORIGIN_MANUAL, &cloud_429_seen, NULL);
    assert(err == ESP_OK);
    assert(cloud_429_seen);
    assert(s_discovery_attempts == 1U);
    assert(s_select_connect_attempts == 0U);
    assert(s_alias_activation_count == 1U);
    assert(s_snapshot_publish_count == 1U);
    assert(s_device_snapshot_store.snapshot.generation == 81U);
    assert(s_diagnostic_stage != NULL &&
           strcmp(s_diagnostic_stage, "inventory_complete") == 0);
}

static void test_cloud_auth_and_integrity_failures_do_not_fall_back(void)
{
    reset_patch059_integration_case(89U, 0U, ATHOM_HOMEY_DATA_READY);
    s_mock_discovery_status = 401;
    s_current_refresh_origin = ATHOM_REFRESH_ORIGIN_MANUAL;
    bool cloud_429_seen = false;
    assert(connect_and_fetch_inventory_for_origin(
        "synthetic-homey", ATHOM_REFRESH_ORIGIN_MANUAL,
        &cloud_429_seen, NULL) == ESP_FAIL);
    assert(!cloud_429_seen);
    assert(s_discovery_attempts == 1U);
    assert(s_alias_activation_count == 0U);
    assert(s_snapshot_publish_count == 0U);

    reset_patch059_integration_case(90U, 0U, ATHOM_HOMEY_DATA_READY);
    s_mock_discovery_status = 403;
    s_current_refresh_origin = ATHOM_REFRESH_ORIGIN_MANUAL;
    assert(connect_and_fetch_inventory_for_origin(
        "synthetic-homey", ATHOM_REFRESH_ORIGIN_MANUAL,
        &cloud_429_seen, NULL) == ESP_FAIL);
    assert(!cloud_429_seen);
    assert(s_alias_activation_count == 0U);
    assert(s_snapshot_publish_count == 0U);

    reset_patch059_integration_case(91U, 0U, ATHOM_HOMEY_DATA_READY);
    s_mock_discovery_status = 429;
    s_cloud_state.homey_session_token[0] = '\0';
    s_current_refresh_origin = ATHOM_REFRESH_ORIGIN_MANUAL;
    assert(connect_and_fetch_inventory_for_origin(
        "synthetic-homey", ATHOM_REFRESH_ORIGIN_MANUAL,
        &cloud_429_seen, NULL) ==
        ESP_ERR_INVALID_STATE);
    assert(cloud_429_seen);
    assert(s_alias_activation_count == 0U);
    assert(s_snapshot_publish_count == 0U);
    assert(s_device_snapshot_store.snapshot.generation == 91U);
}

static void test_boot_429_uses_valid_cache_or_preserves_discovery_failure(void)
{
    reset_patch059_integration_case(95U, 0U, ATHOM_HOMEY_DATA_READY);
    s_current_refresh_origin = ATHOM_REFRESH_ORIGIN_BOOT_AUTO;
    bool cloud_429_seen = false;
    s_mock_discovery_status = 429;
    assert(connect_and_fetch_inventory_for_origin(
        "synthetic-homey", ATHOM_REFRESH_ORIGIN_BOOT_AUTO,
        &cloud_429_seen, NULL) == ESP_OK);
    assert(!cloud_429_seen);
    /* A structurally valid selected-session cache avoids cloud discovery. */
    assert(s_discovery_attempts == 0U);
    assert(s_select_connect_attempts == 0U);
    assert(s_alias_activation_count == 1U);

    /* With an invalid cache, a real cloud 429 must remain visible and cannot
     * be converted into an inventory request using that invalid session. */
    reset_patch059_integration_case(96U, 0U, ATHOM_HOMEY_DATA_ERROR);
    s_current_refresh_origin = ATHOM_REFRESH_ORIGIN_BOOT_AUTO;
    s_mock_discovery_status = 429;
    s_cloud_state.homey_session_token[0] = '\0';
    cloud_429_seen = false;
    assert(connect_and_fetch_inventory_for_origin(
        "synthetic-homey", ATHOM_REFRESH_ORIGIN_BOOT_AUTO,
        &cloud_429_seen, NULL) != ESP_OK);
    assert(cloud_429_seen);
    assert(s_discovery_attempts == 1U);
    assert(s_select_connect_attempts == 0U);
    assert(s_alias_activation_count == 0U);
    assert(s_snapshot_publish_count == 0U);
    assert(s_diagnostic_http_status == 429);
}

static void test_boot_cache_validation_fallback_and_alias_fail_closed(void)
{
    reset_patch059_integration_case(96U, 0U, ATHOM_HOMEY_DATA_READY);
    s_current_refresh_origin = ATHOM_REFRESH_ORIGIN_BOOT_AUTO;
    s_cloud_state.homey_session_token[0] = '\0';
    bool cloud_429_seen = false;
    assert(connect_and_fetch_inventory_for_origin(
        "synthetic-homey", ATHOM_REFRESH_ORIGIN_BOOT_AUTO,
        &cloud_429_seen, NULL) != ESP_OK);
    assert(s_discovery_attempts == 1U);
    assert(s_select_connect_attempts == 1U);
    assert(s_snapshot_publish_count == 0U);

    reset_patch059_integration_case(97U, 0U, ATHOM_HOMEY_DATA_READY);
    s_current_refresh_origin = ATHOM_REFRESH_ORIGIN_BOOT_AUTO;
    s_alias_activation_result = ESP_FAIL;
    assert(connect_and_fetch_inventory_for_origin(
        "synthetic-homey", ATHOM_REFRESH_ORIGIN_BOOT_AUTO,
        &cloud_429_seen, NULL) != ESP_OK);
    assert(s_discovery_attempts == 0U);
    assert(s_select_connect_attempts == 0U);
    assert(s_snapshot_publish_count == 0U);

    reset_patch059_integration_case(98U, 0U, ATHOM_HOMEY_DATA_READY);
    s_current_refresh_origin = ATHOM_REFRESH_ORIGIN_PERIODIC;
    s_cloud_state.homey_session_token[0] = '\0';
    assert(connect_and_fetch_inventory_for_origin(
        "synthetic-homey", ATHOM_REFRESH_ORIGIN_PERIODIC,
        &cloud_429_seen, NULL) != ESP_OK);
    assert(s_discovery_attempts == 1U);
    assert(s_select_connect_attempts == 1U);
    assert(s_snapshot_publish_count == 0U);
}

static void test_periodic_path_has_no_cloud_discovery_retry_loop(void)
{
    reset_patch059_integration_case(100U, 0U, ATHOM_HOMEY_DATA_READY);
    s_now_us = 60000000LL;
    s_mock_discovery_status = 429;
    run_periodic_scheduler_for_bounded_ticks();
    assert(s_discovery_attempts == 0U);
    assert(s_periodic_inventory_attempts == 0U);
    assert(s_alias_activation_count == 1U);
    assert(s_snapshot_publish_count == 1U);
    assert(s_device_snapshot_store.snapshot.generation == 101U);
    assert(s_scheduler_iterations == s_scheduler_iteration_limit);
    assert(!inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_PERIODIC, true, 1U, false,
        "inventory_devices", 429));
    assert(inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, 1U, true,
        "inventory_devices", 429));
    assert(inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, 2U, true,
        "inventory_devices", 429));
    assert(!inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, 3U, true,
        "inventory_devices", 429));
    assert(inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, false, 1U, true,
        "cached_session_validation", 0));
    assert(inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, false, 1U, true,
        "cached_alias_validation", 0));
    assert(preselection_restore_retry_delay_ms(429) == 60000U);
    assert(preselection_restore_retry_delay_ms(503) == 2000U);
    assert(preselection_restore_retry_allowed(1U, 429, true));
    assert(!preselection_restore_retry_allowed(2U, 429, true));
    assert(preselection_restore_retry_allowed(2U, 503, true));
}

static void test_scheduler_recovers_without_initial_snapshot_after_cooldown(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_ERROR);
    s_device_snapshot_store.active = false;
    s_state_name = "homey_connection_error";
    s_homey_data_state = ATHOM_HOMEY_DATA_ERROR;
    s_no_snapshot_recovery_pending = true;
    s_no_snapshot_recovery_sequence = 1U;
    s_no_snapshot_recovery_delay_ms = PERIODIC_REFRESH_COOLDOWN_MS;
    s_scheduler_iterations = 0U;
    s_scheduler_iteration_limit = 35U;
    s_now_us = 0LL;

    run_periodic_scheduler_for_bounded_ticks();

    assert(s_worker_inventory_commands == 1U);
    assert(s_patch059_queue.queued_count == 1U);
    assert(s_current_refresh_origin == ATHOM_REFRESH_ORIGIN_PERIODIC);
    assert(s_snapshot_publish_count == 1U);
    assert(s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(!s_no_snapshot_recovery_pending);
    assert(s_scheduler_iterations >= 30U);
}

static void test_zone_eagain_after_boot_exhaustion_recovers_without_snapshot(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_ERROR);
    s_device_snapshot_store.active = false;
    s_collection_response_sequence_count = 3U;
    for (unsigned i = 0U; i < 3U; ++i) {
        s_collection_error_sequence[i] = ESP_ERR_HTTP_EAGAIN;
        s_collection_status_sequence[i] = 0;
    }

    s_patch059_queue.command = (athom_homey_command_t){
        .kind = ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA,
        .origin = ATHOM_REFRESH_ORIGIN_BOOT_AUTO,
    };
    s_patch059_queue.has_command = true;
    s_patch059_queue.queued_count = 1U;
    run_one_worker_command();

    assert(s_collection_response_sequence_index == 3U);
    assert(strcmp(s_diagnostic_stage, "inventory_zones") == 0);
    assert(s_diagnostic_error == ESP_ERR_HTTP_EAGAIN);
    assert(s_diagnostic_http_status == 0);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(!s_device_snapshot_store.active);
    assert(s_no_snapshot_recovery_pending);
    assert(s_no_snapshot_recovery_delay_ms == PERIODIC_REFRESH_COOLDOWN_MS);

    /* Later transport recovery is handled by the normal cooldown scheduler. */
    s_scheduler_iterations = 0U;
    s_scheduler_iteration_limit = 40U;
    s_now_us = 90000LL;
    run_periodic_scheduler_for_bounded_ticks();

    assert(s_worker_inventory_commands == 2U);
    assert(s_current_refresh_origin == ATHOM_REFRESH_ORIGIN_PERIODIC);
    assert(s_snapshot_publish_count == 1U);
    assert(s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(!s_no_snapshot_recovery_pending);
    assert(s_patch059_queue.queued_count == 2U);
    assert(!s_patch059_queue.has_command);
}

static void test_boot_429_cannot_leave_attempt_sequence_stuck_forever(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_ERROR);
    unsigned attempt = 1U;
    uint64_t simulated_now_ms = 0U;
    bool cloud_429_seen = true;
    for (unsigned failure = 0U; failure < 2U; ++failure) {
        assert(inventory_refresh_worker_should_retry_after_cloud_429(
            ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, attempt, cloud_429_seen,
            "inventory_devices", 429));
        const uint32_t delay = homey_data_retry_delay_ms_for_failure(
            attempt, 429, cloud_429_seen);
        assert(delay >= 60000U && delay <= 300000U);
        simulated_now_ms += delay;
        attempt++;
    }
    assert(attempt == 3U);
    assert(!inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, attempt, cloud_429_seen,
        "inventory_devices", 429));
    assert(simulated_now_ms == 180000U);
    assert(cloud_429_seen);
    assert(s_discovery_attempts == 0U);

    /* A missing restored cache fails closed, then arms delayed rediscovery. */
    assert(inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, false, 2U, cloud_429_seen,
        "cached_session_validation", 0));
    assert(inventory_refresh_retry_reenters_discovery(
        cloud_429_seen, "cached_session_validation"));
    assert(homey_data_retry_delay_ms_for_failure(attempt, 0, cloud_429_seen) ==
           ATHOM_HOMEY_DATA_429_RETRY_3_MS);
    assert(!inventory_refresh_retry_reenters_discovery(
        cloud_429_seen, "cached_alias_validation"));
    if (inventory_refresh_retry_reenters_discovery(
            cloud_429_seen, "cached_session_validation")) {
        cloud_429_seen = false;
    }
    assert(!cloud_429_seen); /* The next full attempt is deliberately delayed. */
    assert(homey_data_retry_delay_ms_for_failure(attempt, 0, cloud_429_seen) ==
           ATHOM_HOMEY_DATA_RETRY_3_MS);
}

static void test_favorites_transport_failure_keeps_worker_inventory_ready(void)
{
    reset_patch059_integration_case(40U, 0U, ATHOM_HOMEY_DATA_ERROR);
    s_favorite_error = ESP_ERR_HTTP_FETCH_HEADER;
    s_favorite_status = 0;
    s_patch059_queue.command = (athom_homey_command_t){
        .kind = ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA,
        .origin = ATHOM_REFRESH_ORIGIN_PERIODIC,
    };
    s_patch059_queue.has_command = true;
    run_one_worker_command();
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(s_device_snapshot_store.snapshot.generation == 41U);
    assert(s_favorites_clear_count > 0U);
    assert(!s_transport_metrics.favorites_read.data_verified);
    assert(s_transport_metrics.favorites_read.error == ESP_ERR_HTTP_FETCH_HEADER);
    assert(s_discovery_attempts == 0U && s_write_attempts == 0U);
}

static void reset_awning_worker_case(size_t widget, double position)
{
    (void)widget;
    reset_patch059_integration_case(200U, 0U, ATHOM_HOMEY_DATA_READY);
    s_wifi_online = true;
    s_provisioning_runtime_ready = true;
    s_network_phase_owner = ATHOM_NETWORK_PHASE_NONE;
    s_awning_job_reserved = false;
    s_awning_motion_uncertain = false;
    s_awning_write_result_ready = false;
    s_awning_outcome = PATCH085_AWNING_OUTCOME_NOT_ATTEMPTED;
    s_awning_initial_enqueue_pending = false;
    s_awning_pending_widget = 0U;
    s_awning_target_position = 0.0;
    s_awning_confirmation_after_generation = 0U;
    memset(&s_awning_context, 0, sizeof(s_awning_context));
    memset(s_awning_interaction, 0, sizeof(s_awning_interaction));
    memset(s_awning_interaction_identity, 0, sizeof(s_awning_interaction_identity));
    memset(s_awning_interaction_alias_generation, 0,
           sizeof(s_awning_interaction_alias_generation));
    s_awning_next_dispatch_pending = false;
    s_awning_next_dispatch_widget = 0U;
    memset(&s_mock_awning_context, 0, sizeof(s_mock_awning_context));
    memset(s_mock_awning_context.selected_homey_digest, 0x5A, 32U);
    s_mock_awning_context.alias_generation = 7U;
    s_mock_awning_context.snapshot_generation = 200U;
    s_mock_awning_context.current_position = position;
    s_mock_awning_widget_position_override = false;
    memset(s_mock_awning_widget_position, 0, sizeof(s_mock_awning_widget_position));
    s_mock_awning_context.minimum = 0.0;
    s_mock_awning_context.maximum = 1.0;
    s_mock_awning_context_valid = true;
    s_mock_confirm_snapshot_on_context_read = false;
    s_mock_run_worker_after_confirm_context_read = false;
    s_mock_confirmed_snapshot_position = 0.0;
    s_mock_confirmed_snapshot_generation = 0U;
    s_mock_queue_other_widget_after_snapshot = false;
    s_mock_other_widget_queue_result = ATHOM_AWNING_QUEUE_BUSY;
    s_mock_queue_same_widget_after_snapshot = false;
    s_mock_same_widget_queue_result = ATHOM_AWNING_QUEUE_BUSY;
    s_mock_context_taps_remaining = 0U;
    s_mock_context_tap_in_progress = false;
    s_mock_context_tap_result = ATHOM_AWNING_QUEUE_BUSY;
    s_mock_awning_queue_fail = false;
    s_mock_awning_reenter_on_queue_failure = false;
    s_mock_awning_reenter_result = ATHOM_AWNING_QUEUE_BUSY;
    s_mock_awning_write_result = ATHOM_HOMEY_AWNING_WRITE_ACCEPTED;
    s_mock_awning_use_http_response = false;
    s_mock_awning_response_complete = true;
    s_mock_awning_http_status = 200;
    s_mock_awning_perform_error = ESP_OK;
    s_mock_awning_put_calls = 0U;
    s_mock_awning_dispatch_calls = 0U;
    s_observe_snapshot_during_awning_put = false;
    s_mock_awning_tap_during_put = false;
    s_mock_awning_tap_result = ATHOM_AWNING_QUEUE_BUSY;
    s_mock_selected_homey_valid = true;
    s_mock_active_alias_valid = true;
    s_mock_active_nvs_binding_matches = true;
    seed_durable_guard_ready(&s_mock_awning_context);
    s_mock_guard_staged_present = false;
    memset(&s_mock_guard_staged_record, 0,
           sizeof(s_mock_guard_staged_record));
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_NONE;
    s_mock_guard_corrupt_readback = false;
    s_mock_guard_nvs_commit_count = 0U;
    s_mock_guard_nvs_write_count = 0U;
    s_power_loss_context_reads_to_cut = 0U;
    s_power_loss_during_http_attempt = false;
    s_power_loss_after_http_response = false;
    s_awning_guard_state = PANEL_AWNING_GUARD_UNINITIALIZED;
    memset(&s_awning_guard_record, 0, sizeof(s_awning_guard_record));
    s_awning_guard_loaded = false;
    s_awning_guard_resolution_pending = false;
    s_awning_commission_review_valid = false;
    s_awning_commission_reserved = false;
    s_awning_commission_review_expires_ms = 0U;
    memset(s_awning_commission_review_context, 0,
           sizeof(s_awning_commission_review_context));
    s_awning_guard_resolution_sequence = 0U;
    s_awning_guard_resolution_step = 0U;
    s_awning_guard_resolution_generation = 0U;
    awning_guard_load_on_worker_start();
}

#if defined(PATCH119E2B_ONLY)
#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
static void simulate_awning_runtime_restart(double position);
#endif

static void prepare_uninitialized_commission_case(void)
{
    reset_awning_worker_case(1U, 0.42);
    s_mock_guard_durable_present = false;
    memset(&s_mock_guard_durable_record, 0, sizeof(s_mock_guard_durable_record));
    s_awning_guard_loaded = true;
    s_awning_guard_state = PANEL_AWNING_GUARD_UNINITIALIZED;
    s_homey_data_state = ATHOM_HOMEY_DATA_READY;
    s_provisioning_runtime_ready = true;
    s_wifi_online = true;
    s_network_phase_owner = ATHOM_NETWORK_PHASE_NONE;
    s_mock_change_alias_on_guard_commit = false;
}

static void test_patch119e2b_commissioning_requires_local_review_and_worker(void)
{
    prepare_uninitialized_commission_case();
    assert(!athom_oauth_runtime_awning_commission_confirm());
    assert(!s_patch059_queue.has_command && !s_mock_guard_durable_present);
    assert(s_mock_guard_nvs_write_count == 0U && s_mock_awning_put_calls == 0U);
    athom_awning_interaction_snapshot_t blocked = {0};
    assert(athom_oauth_runtime_awning_interaction_snapshot(1U, &blocked));
    assert(blocked.guard_state == PANEL_AWNING_GUARD_UNINITIALIZED);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    athom_cloud_awning_command_context_t read_only_position = {0};
    assert(athom_cloud_get_awning_command_context(
        &s_cloud_state, 1U, &read_only_position));
    assert(read_only_position.current_position == s_mock_awning_context.current_position);

    char review[192] = {0};
    assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    assert(strstr(review, "Homey READY") != NULL);
    assert(strstr(review, "Solskydd") != NULL &&
           strstr(review, "Balkongmarkis") != NULL);
    /* Review is read-only. The final local confirmation only queues work. */
    assert(s_mock_guard_nvs_write_count == 0U && s_mock_guard_nvs_commit_count == 0U);
    assert(athom_oauth_runtime_awning_commission_confirm());
    assert(!athom_oauth_runtime_awning_commission_confirm());
    assert(s_patch059_queue.has_command &&
           s_patch059_queue.command.kind == ATHOM_HOMEY_COMMAND_AWNING_GUARD_COMMISSION);
    assert(!s_mock_guard_durable_present && s_mock_awning_put_calls == 0U);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_AWNING_GUARD_COMMISSION);
    run_one_worker_command();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_READY);
    assert(s_mock_guard_durable_present &&
           s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_READY &&
           s_mock_guard_durable_record.sequence == 1U);
    assert(s_mock_guard_nvs_write_count == 1U && s_mock_guard_nvs_commit_count == 1U);
    assert(s_mock_awning_put_calls == 0U &&
           s_network_phase_owner == ATHOM_NETWORK_PHASE_NONE);
    assert(!athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    assert(!athom_oauth_runtime_awning_commission_confirm());
#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
    simulate_awning_runtime_restart(0.42);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_READY);
    assert(!athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    assert(!athom_oauth_runtime_awning_commission_confirm());
    assert(s_mock_awning_put_calls == 0U);
#endif

    /* The first real command still crosses the ordinary durable pending barrier. */
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U);
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_WRITE_PENDING ||
           s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_READY);
}

static void test_patch119e2c_commissioning_review_formats_normalized_positions(void)
{
    static const struct {
        double position;
        const char *percent;
    } cases[] = {
        {0.0, "0%"},
        {0.53, "53%"},
        {1.0, "100%"},
    };
    char review[192] = {0};
    prepare_uninitialized_commission_case();
    s_mock_awning_widget_position_override = true;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        s_mock_awning_widget_position[1U] = cases[i].position;
        s_mock_awning_widget_position[2U] = cases[i].position;
        assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
        char expected[96] = {0};
        (void)snprintf(expected, sizeof(expected),
            "Solskydd %s - Balkongmarkis %s", cases[i].percent, cases[i].percent);
        assert(strstr(review, expected) != NULL);
        athom_oauth_runtime_awning_commission_cancel_review();
    }

    s_mock_awning_widget_position[1U] = 0.0;
    s_mock_awning_widget_position[2U] = 0.53;
    assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    assert(strstr(review, "Solskydd 0% - Balkongmarkis 53%") != NULL);
    athom_oauth_runtime_awning_commission_cancel_review();
    s_mock_awning_widget_position[1U] = 1.0;
    s_mock_awning_widget_position[2U] = 0.0;
    assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    assert(strstr(review, "Solskydd 100% - Balkongmarkis 0%") != NULL);
    athom_oauth_runtime_awning_commission_cancel_review();

    /* Missing authoritative context is not shown as a valid zero. */
    s_mock_awning_widget_position_override = false;
    s_mock_awning_context_valid = false;
    assert(!athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    assert(strstr(review, "Solskydd 0%") == NULL);
    s_mock_awning_context_valid = true;
}

static void test_patch119e2b_commissioning_fails_closed_on_mismatch_or_activity(void)
{
    char review[192] = {0};
    prepare_uninitialized_commission_case();
    assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    s_mock_awning_context.alias_generation++;
    assert(!athom_oauth_runtime_awning_commission_confirm());
    assert(!s_patch059_queue.has_command && !s_mock_guard_durable_present);

    prepare_uninitialized_commission_case();
    assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    s_mock_awning_context.selected_homey_digest[0] ^= 0x10U;
    assert(!athom_oauth_runtime_awning_commission_confirm());
    assert(!s_patch059_queue.has_command && !s_mock_guard_durable_present);

    prepare_uninitialized_commission_case();
    assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    s_mock_awning_context.snapshot_generation++;
    assert(!athom_oauth_runtime_awning_commission_confirm());
    assert(!s_patch059_queue.has_command && !s_mock_guard_durable_present);

    prepare_uninitialized_commission_case();
    s_awning_interaction[1U].active = true;
    assert(!athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    s_awning_interaction[1U].active = false;
    s_refresh_job_reserved = true;
    assert(!athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    s_refresh_job_reserved = false;

    prepare_uninitialized_commission_case();
    assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    s_mock_change_alias_on_guard_commit = true;
    assert(athom_oauth_runtime_awning_commission_confirm());
    run_one_worker_command();
    assert(s_mock_guard_durable_present &&
           s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_READY);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_mock_awning_put_calls == 0U);
#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
    const uint32_t persisted_alias_generation =
        s_mock_awning_context.alias_generation;
    simulate_awning_runtime_restart(0.42);
    /* Alias-generation storage survives the simulated application reset. */
    s_mock_awning_context.alias_generation = persisted_alias_generation;
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_READY);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_NOT_READY);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_mock_awning_put_calls == 0U);
#endif

    const int faults[] = {
        GUARD_NVS_FAULT_OPEN, GUARD_NVS_FAULT_GET, GUARD_NVS_FAULT_SET,
        GUARD_NVS_FAULT_COMMIT_BEFORE_DURABLE,
        GUARD_NVS_FAULT_COMMIT_AFTER_DURABLE,
        GUARD_NVS_FAULT_READBACK_CORRUPT,
    };
    for (size_t i = 0U; i < sizeof(faults) / sizeof(faults[0]); ++i) {
        prepare_uninitialized_commission_case();
        assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
        s_mock_guard_nvs_fault = faults[i];
        assert(athom_oauth_runtime_awning_commission_confirm());
        run_one_worker_command();
        assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
        assert(s_mock_awning_put_calls == 0U);
        athom_awning_interaction_snapshot_t blocked = {0};
        assert(athom_oauth_runtime_awning_interaction_snapshot(2U, &blocked));
        assert(blocked.guard_state == PANEL_AWNING_GUARD_QUARANTINED);
        assert(!athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    }
}

static void test_patch119e2b_missing_and_pending_records_are_distinct(void)
{
    char review[192] = {0};
    prepare_uninitialized_commission_case();
    assert(athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    athom_oauth_runtime_awning_commission_cancel_review();
    /* Simulated complete storage erase is indistinguishable from first absence
     * in NVS; no boot-time READY is created and local service review is required. */
    assert(!s_mock_guard_durable_present &&
           s_awning_guard_state == PANEL_AWNING_GUARD_UNINITIALIZED);
    assert(s_mock_awning_put_calls == 0U);

    seed_durable_guard_ready(&s_mock_awning_context);
    s_mock_guard_durable_record.state = PANEL_AWNING_GUARD_WRITE_PENDING;
    s_mock_guard_durable_record.crc32 = 0U;
    s_mock_guard_durable_record.crc32 = test_guard_crc32(
        &s_mock_guard_durable_record, sizeof(s_mock_guard_durable_record));
    s_awning_guard_loaded = false;
    awning_guard_load_on_worker_start();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(!athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_WRITE_PENDING);
    assert(s_mock_awning_put_calls == 0U);

    /* A CRC-invalid/unsupported record is not converted into first install. */
    s_mock_guard_durable_record.state = PANEL_AWNING_GUARD_READY;
    s_mock_guard_durable_record.version = 99U;
    s_mock_guard_durable_record.crc32 = 0U;
    s_mock_guard_durable_record.crc32 = test_guard_crc32(
        &s_mock_guard_durable_record, sizeof(s_mock_guard_durable_record));
    s_awning_guard_loaded = false;
    awning_guard_load_on_worker_start();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(!athom_oauth_runtime_awning_commission_review(review, sizeof(review)));
}
#endif

#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
static void simulate_awning_runtime_restart(double position)
{
    const panel_awning_guard_record_t durable = s_mock_guard_durable_record;
    const bool durable_present = s_mock_guard_durable_present;
    const unsigned total_writes = s_total_awning_homey_writes;
    reset_awning_worker_case(1U, position);
    s_mock_guard_durable_record = durable;
    s_mock_guard_durable_present = durable_present;
    s_total_awning_homey_writes = total_writes;
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_NONE;
    s_mock_guard_corrupt_readback = false;
    s_awning_guard_loaded = false;
    s_awning_guard_state = PANEL_AWNING_GUARD_UNINITIALIZED;
    memset(&s_awning_guard_record, 0, sizeof(s_awning_guard_record));
    awning_guard_load_on_worker_start();
}
#endif

static void test_patch119e_homey_ready_is_independent_of_awning_guard(void)
{
    reset_awning_worker_case(1U, 0.50);
    s_awning_guard_state = PANEL_AWNING_GUARD_UNINITIALIZED;
    s_awning_guard_loaded = true;
    s_mock_guard_durable_present = false;
    memset(&s_awning_guard_record, 0, sizeof(s_awning_guard_record));
    athom_awning_interaction_snapshot_t view = {0};
    assert(athom_oauth_runtime_awning_interaction_snapshot(1U, &view));
    assert(view.guard_state == PANEL_AWNING_GUARD_UNINITIALIZED && view.failed);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    athom_cloud_awning_command_context_t post_restart_context = {0};
    assert(athom_cloud_get_awning_command_context(
        &s_cloud_state, 1U, &post_restart_context));
    assert(post_restart_context.snapshot_generation == s_mock_awning_context.snapshot_generation);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_NOT_READY);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(s_mock_awning_put_calls == 0U && !s_patch059_queue.has_command);
    if (!athom_cloud_awning_write_is_armed(1U)) return;

    reset_awning_worker_case(1U, 0.50);
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_SET;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 0U);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_awning_interaction[1].failed && !s_awning_interaction[1].active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);

    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_mock_awning_write_result = ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS;
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U);
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_WRITE_PENDING);
    memset(s_awning_interaction, 0, sizeof(s_awning_interaction));
    s_awning_guard_loaded = false;
    s_awning_guard_state = PANEL_AWNING_GUARD_UNINITIALIZED;
    awning_guard_load_on_worker_start();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_NOT_READY);
    assert(s_mock_awning_put_calls == 1U);
    s_awning_guard_loaded = false; /* A later reboot still sees WRITE_PENDING. */
    s_awning_guard_state = PANEL_AWNING_GUARD_UNINITIALIZED;
    awning_guard_load_on_worker_start();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY &&
           s_mock_awning_put_calls == 1U);

    /* A persistent-store read error and repeated restart both remain
     * quarantined, while Homey read-only state remains usable. */
    reset_awning_worker_case(1U, 0.50);
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_GET;
    s_awning_guard_loaded = false;
    awning_guard_load_on_worker_start();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    athom_cloud_awning_command_context_t read_context = {0};
    assert(athom_cloud_get_awning_command_context(
        &s_cloud_state, 1U, &read_context));
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_NOT_READY);
    assert(s_mock_awning_put_calls == 0U);
    s_awning_guard_loaded = false; /* Simulated second RAM reset. */
    s_awning_guard_state = PANEL_AWNING_GUARD_UNINITIALIZED;
    awning_guard_load_on_worker_start();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY &&
           s_mock_awning_put_calls == 0U);
}

#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
static void test_patch119e1a_rejected_http_cannot_clear_pending_guard(void)
{
    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_mock_awning_use_http_response = true;
    s_mock_awning_response_complete = true;
    s_mock_awning_http_status = 400;
    s_mock_awning_perform_error = ESP_OK;
    s_diagnostic_http_status = 400;
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U);
    assert(s_mock_guard_durable_record.state ==
           PANEL_AWNING_GUARD_WRITE_PENDING);

    /* This is read-only reconciliation: fresh unchanged position must not be
     * accepted as proof the actuator has no outstanding operation. */
    s_mock_awning_context.snapshot_generation++;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    printf("PATCH119E1A_REJECTED_UNCHANGED guard=%u durable=%u writes=%u\n",
           (unsigned)s_awning_guard_state,
           (unsigned)s_mock_guard_durable_record.state,
           s_mock_awning_put_calls);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_WRITE_PENDING);
    assert(s_mock_guard_durable_record.state ==
           PANEL_AWNING_GUARD_WRITE_PENDING);
    assert(s_mock_awning_put_calls == 1U);
}

static void test_patch119e1a_http_terminal_classifier_matrix(void)
{
    const int rejected_statuses[] = {400, 401, 403, 404, 408, 409, 429, 499};
    for (size_t i = 0U; i < sizeof(rejected_statuses) / sizeof(rejected_statuses[0]); ++i) {
        const athom_homey_awning_write_result_t result =
            patch085_classify_awning_write(true, true, rejected_statuses[i], ESP_OK);
        assert(result == (rejected_statuses[i] == 401 || rejected_statuses[i] == 403
            ? ATHOM_HOMEY_AWNING_WRITE_UNAUTHORIZED
            : ATHOM_HOMEY_AWNING_WRITE_REJECTED));

        /* Exercise each complete response through the actual worker plus the
         * production NVS adapter. Rejected status and unchanged position do
         * not establish terminal actuator state. */
        reset_awning_worker_case(1U, 0.50);
        s_total_awning_homey_writes = 0U;
        assert(athom_oauth_runtime_queue_awning_step(
            1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
        s_mock_awning_use_http_response = true;
        s_mock_awning_response_complete = true;
        s_mock_awning_http_status = rejected_statuses[i];
        s_mock_awning_perform_error = ESP_OK;
        run_one_worker_command();
        assert(s_total_awning_homey_writes == 1U);
        assert(s_mock_guard_durable_record.state ==
               PANEL_AWNING_GUARD_WRITE_PENDING);
        s_mock_awning_context.snapshot_generation++;
        athom_oauth_runtime_awning_snapshot_observed();
        homey_command_worker_run_deferred_work();
        assert(s_mock_guard_durable_record.state ==
               PANEL_AWNING_GUARD_WRITE_PENDING);
        simulate_awning_runtime_restart(0.50);
        assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
        assert(s_total_awning_homey_writes == 1U);
    }
    assert(patch085_classify_awning_write(true, true, 204, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_ACCEPTED);
    assert(patch085_classify_awning_write(true, false, 400, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch085_classify_awning_write(true, true, 400, ESP_FAIL) ==
           ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch085_classify_awning_write(true, true, 302, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS);
    assert(patch085_classify_awning_write(true, true, 503, ESP_OK) ==
           ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS);
}

static void test_patch119e1a_integrated_restart_boundaries(void)
{
    /* A: failure before durable pending commit proves no write was attempted;
     * the durable READY record can safely be loaded after reboot. */
    reset_awning_worker_case(1U, 0.50);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_COMMIT_BEFORE_DURABLE;
    run_one_worker_command();
    assert(s_total_awning_homey_writes == 0U);
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_READY);
    simulate_awning_runtime_restart(0.50);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_READY);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_total_awning_homey_writes == 0U);

    /* B: commit reports an error after the durable bytes changed. It blocks
     * the HTTP call now and the next boot sees WRITE_PENDING. */
    reset_awning_worker_case(1U, 0.50);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_COMMIT_AFTER_DURABLE;
    run_one_worker_command();
    assert(s_total_awning_homey_writes == 0U);
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_WRITE_PENDING);
    simulate_awning_runtime_restart(0.50);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) != ATHOM_AWNING_QUEUE_QUEUED);
    assert(athom_oauth_runtime_queue_awning_step(
        2U, PANEL_ROLLER_DIRECTION_UP) != ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    athom_cloud_awning_command_context_t read_context = {0};
    assert(athom_cloud_get_awning_command_context(
        &s_cloud_state, 1U, &read_context));

    /* A successful commit followed by corrupted readback must also block
     * dispatch even though the underlying mock has pending bytes durable. */
    reset_awning_worker_case(1U, 0.50);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_READBACK_CORRUPT;
    run_one_worker_command();
    assert(s_total_awning_homey_writes == 0U);
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_WRITE_PENDING);
    simulate_awning_runtime_restart(0.50);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_total_awning_homey_writes == 0U);

    /* C: cut power after the successful pending readback and before the HTTP
     * call. The durable intent blocks both widgets after restart. */
    reset_awning_worker_case(1U, 0.50);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_power_loss_context_reads_to_cut = 2U;
    run_one_worker_command();
    assert(s_total_awning_homey_writes == 0U);
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_WRITE_PENDING);
    simulate_awning_runtime_restart(0.50);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(!s_patch059_queue.has_command && s_total_awning_homey_writes == 0U);

    /* D: a possible HTTP transmission followed by power loss cannot replay. */
    reset_awning_worker_case(1U, 0.50);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_power_loss_during_http_attempt = true;
    run_one_worker_command();
    assert(s_total_awning_homey_writes == 1U);
    simulate_awning_runtime_restart(0.50);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) != ATHOM_AWNING_QUEUE_QUEUED);
    assert(athom_oauth_runtime_queue_awning_step(
        2U, PANEL_ROLLER_DIRECTION_UP) != ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_total_awning_homey_writes == 1U);

    /* E: accepted HTTP with no authoritative confirmation persists pending. */
    reset_awning_worker_case(1U, 0.50);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_power_loss_after_http_response = true;
    run_one_worker_command();
    assert(s_total_awning_homey_writes == 1U);
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_WRITE_PENDING);
    simulate_awning_runtime_restart(0.75);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_total_awning_homey_writes == 1U);

    /* F: authoritative confirmation exists but READY commit did not become
     * durable. Restart must still observe pending and suppress another write. */
    reset_awning_worker_case(1U, 0.50);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    s_mock_awning_context.snapshot_generation++;
    s_mock_awning_context.current_position = 0.75;
    athom_oauth_runtime_awning_snapshot_observed();
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_COMMIT_BEFORE_DURABLE;
    homey_command_worker_run_deferred_work();
    assert(s_total_awning_homey_writes == 1U);
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_WRITE_PENDING);
    simulate_awning_runtime_restart(0.75);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_total_awning_homey_writes == 1U);

    /* G: READY bytes became durable but the commit API returned an error.
     * Runtime quarantines this boot; reboot may trust only the validated READY
     * record that followed accepted HTTP and authoritative target confirmation. */
    reset_awning_worker_case(1U, 0.50);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    s_mock_awning_context.snapshot_generation++;
    s_mock_awning_context.current_position = 0.75;
    athom_oauth_runtime_awning_snapshot_observed();
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_COMMIT_AFTER_DURABLE;
    homey_command_worker_run_deferred_work();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_READY);
    simulate_awning_runtime_restart(0.75);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_READY);
    assert(s_total_awning_homey_writes == 1U);

    /* H: READY commit precedes local continuation; reboot discards the
     * volatile second intention instead of replaying it. */
    reset_awning_worker_case(1U, 0.0);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    run_one_worker_command();
    s_mock_awning_context.snapshot_generation++;
    s_mock_awning_context.current_position = 0.25;
    assert(network_phase_try_reserve(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH));
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(s_mock_guard_durable_record.state == PANEL_AWNING_GUARD_READY);
    assert(s_awning_next_dispatch_pending && s_total_awning_homey_writes == 1U);
    simulate_awning_runtime_restart(0.25);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_READY);
    assert(!s_awning_next_dispatch_pending && !s_patch059_queue.has_command);
    assert(s_total_awning_homey_writes == 1U);

    /* I: the second intention must pass its own durable barrier. A commit
     * error after durable pending bytes blocks that second HTTP call/reboot. */
    reset_awning_worker_case(1U, 0.0);
    s_total_awning_homey_writes = 0U;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    run_one_worker_command();
    s_mock_awning_context.snapshot_generation++;
    s_mock_awning_context.current_position = 0.25;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);
    s_mock_guard_nvs_fault = GUARD_NVS_FAULT_COMMIT_AFTER_DURABLE;
    run_one_worker_command();
    assert(s_total_awning_homey_writes == 1U);
    simulate_awning_runtime_restart(0.25);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(s_total_awning_homey_writes == 1U);

    /* J: absent, corrupt and binding-stale durable records never become a
     * writable first-install default. Read-only Homey remains READY. */
    reset_awning_worker_case(1U, 0.50);
    s_mock_guard_durable_present = false;
    s_awning_guard_loaded = false;
    awning_guard_load_on_worker_start();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_UNINITIALIZED);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) != ATHOM_AWNING_QUEUE_QUEUED);

    reset_awning_worker_case(1U, 0.50);
    ((uint8_t *)&s_mock_guard_durable_record)[sizeof(uint32_t)] ^= 0x40U;
    s_awning_guard_loaded = false;
    awning_guard_load_on_worker_start();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_QUARANTINED);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) != ATHOM_AWNING_QUEUE_QUEUED);

    reset_awning_worker_case(1U, 0.50);
    s_mock_guard_durable_record.homey_identity[0] ^= 0x01U;
    s_mock_guard_durable_record.crc32 = 0U;
    s_mock_guard_durable_record.crc32 = test_guard_crc32(
        &s_mock_guard_durable_record, sizeof(s_mock_guard_durable_record));
    s_awning_guard_loaded = false;
    awning_guard_load_on_worker_start();
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_READY);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) != ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_total_awning_homey_writes == 1U);
}
#endif

#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
static void run_worker_deferred_pass(void)
{
    const bool was_worker_active = s_worker_active;
    s_worker_active = true;
    homey_command_worker_run_deferred_work();
    s_worker_active = was_worker_active;
}

static void setup_confirmed_pending_awning_continuation(void)
{
    reset_awning_worker_case(1U, 0.0);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U && s_awning_motion_uncertain);
    s_mock_awning_context.snapshot_generation = 201U;
    s_mock_awning_context.current_position = 0.25;
    assert(network_phase_try_reserve(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH));
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(s_awning_next_dispatch_pending && s_awning_interaction[1].active);
}

static void release_inventory_during_worker_wait(void)
{
    const bool was_worker_active = s_worker_active;
    s_worker_active = false;
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    s_worker_active = was_worker_active;
}

static void test_patch119d_worker_idle_notification_race(void)
{
    setup_confirmed_pending_awning_continuation();
    s_notify_wait_interleave = release_inventory_during_worker_wait;
    s_worker_active = true;
    if (setjmp(s_worker_exit) == 0) homey_command_worker(NULL);
    s_worker_active = false;
    assert(s_worker_notify_count > 0U);
    assert(!s_awning_next_dispatch_pending);
    assert(s_mock_awning_put_calls == 2U);
    assert(s_awning_motion_uncertain);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_NONE);
}

static void test_patch119d_preselection_serializes_continuation(void)
{
    setup_confirmed_pending_awning_continuation();
    s_preselection_restore_pending = true;
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    run_worker_deferred_pass();
    assert(!s_preselection_restore_pending);
    assert(s_preselection_restore_worker_running);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE);
    assert(s_awning_next_dispatch_pending && !s_awning_job_reserved);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);

    s_preselection_restore_worker_running = false;
    network_phase_release(ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE);
    run_worker_deferred_pass();
    assert(!s_awning_next_dispatch_pending && s_awning_job_reserved);
    assert(s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_LIGHT_TOGGLE);
}

static void test_patch119d_identity_timeout_and_worker_failure(void)
{
    setup_confirmed_pending_awning_continuation();
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    s_mock_awning_queue_fail = true;
    run_worker_deferred_pass();
    assert(!s_awning_next_dispatch_pending && !s_awning_job_reserved);
    assert(s_awning_interaction[1].failed && !s_awning_interaction[1].active);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_NONE);

    setup_confirmed_pending_awning_continuation();
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    s_mock_awning_context_valid = false;
    run_worker_deferred_pass();
    assert(!s_awning_next_dispatch_pending &&
           s_awning_interaction[1].failed && !s_awning_interaction[1].active);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);

    setup_confirmed_pending_awning_continuation();
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    athom_oauth_runtime_awning_tick(
        s_awning_interaction[1].deadline_ms + 1U);
    run_worker_deferred_pass();
    assert(!s_awning_next_dispatch_pending &&
           s_awning_interaction[1].failed && !s_awning_interaction[1].active);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);

    setup_confirmed_pending_awning_continuation();
    s_homey_command_worker_task = NULL;
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    assert(!s_awning_next_dispatch_pending &&
           s_awning_interaction[1].failed && !s_awning_interaction[1].active);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);
}
#endif

#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
static void test_patch119a_f1_multitap_during_write_and_readback(void)
{
    reset_awning_worker_case(1U, 0.25);
    s_mock_awning_tap_during_put = true;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    athom_awning_interaction_snapshot_t movement = {0};
    assert(athom_oauth_runtime_awning_interaction_snapshot(1U, &movement));
    assert(movement.active && movement.active_direction == 0); /* only queued */
    run_one_worker_command();
    assert(athom_oauth_runtime_awning_interaction_snapshot(1U, &movement));
    assert(movement.active_direction == PANEL_ROLLER_DIRECTION_UP);
    assert(s_mock_awning_put_calls == 1U);
    assert(s_mock_awning_tap_result == ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    assert(s_awning_interaction[1].queued_count == 1U);
    assert(!s_patch059_queue.has_command);

    s_now_us += 500000LL;
    const athom_awning_queue_result_t after_write_tap =
        athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP);
    assert(after_write_tap == ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    const uint8_t queued = s_awning_interaction[1].queued_count;
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_DOWN) ==
           ATHOM_AWNING_QUEUE_OPPOSITE_REJECTED);
    assert(s_awning_interaction[1].queued_count == queued);
    assert(s_mock_awning_put_calls == 1U && !s_patch059_queue.has_command);

    s_mock_awning_context.snapshot_generation = 201U;
    s_mock_awning_context.current_position = 0.53;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(s_patch059_queue.has_command);
    assert(s_patch059_queue.command.numeric_value == 0.75);
    assert(s_mock_awning_put_calls == 1U);
}

static void test_patch119a_f2_worker_prewrite_rejection_cancels_intents(void)
{
    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    s_mock_awning_context.snapshot_generation++;
    run_one_worker_command();
    assert(s_mock_awning_dispatch_calls == 0U && s_mock_awning_put_calls == 0U);
    assert(s_awning_interaction[1].failed);
    assert(!s_awning_interaction[1].active && s_awning_interaction[1].queued_count == 0U);
    athom_awning_interaction_snapshot_t snapshot = {0};
    assert(athom_oauth_runtime_awning_interaction_snapshot(1U, &snapshot));
    assert(snapshot.failed && !snapshot.active && snapshot.pending_bar_mask == 0U);
    assert(!s_awning_motion_uncertain && !s_awning_job_reserved);
}

static void test_patch119a_f3_nonaccepted_result_never_continues(void)
{
    const athom_homey_awning_write_result_t failures[] = {
        ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS,
        ATHOM_HOMEY_AWNING_WRITE_REJECTED,
    };
    for (size_t i = 0U; i < sizeof(failures) / sizeof(failures[0]); ++i) {
        reset_awning_worker_case(1U, 0.50);
        s_mock_awning_write_result = failures[i];
        if (failures[i] == ATHOM_HOMEY_AWNING_WRITE_REJECTED)
            s_diagnostic_http_status = 403;
        assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
               ATHOM_AWNING_QUEUE_QUEUED);
        s_now_us += 500000LL;
        assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
               ATHOM_AWNING_QUEUE_QUEUED_NEXT);
        run_one_worker_command();
        assert(s_mock_awning_put_calls == 1U);
        assert(s_awning_interaction[1].failed &&
               s_awning_interaction[1].queued_count == 0U);
        s_mock_awning_context.snapshot_generation = 201U;
        s_mock_awning_context.current_position = 0.75;
        athom_oauth_runtime_awning_snapshot_observed();
        assert(s_mock_awning_put_calls == 1U);
        assert(!s_patch059_queue.has_command);
        assert(!s_awning_next_dispatch_pending);
        assert(s_awning_motion_uncertain);
        assert(athom_oauth_runtime_queue_awning_step(
            1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_BUSY);
        if (failures[i] == ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS) {
            memset(s_mock_awning_context.selected_homey_digest, 0x6B,
                   sizeof(s_mock_awning_context.selected_homey_digest));
            s_mock_awning_context.alias_generation++;
            s_mock_awning_context.snapshot_generation = 202U;
            athom_oauth_runtime_awning_snapshot_observed();
            assert(s_awning_motion_uncertain);
            assert(athom_oauth_runtime_queue_awning_step(
                1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_NOT_READY);
        }
    }
}

static void test_patch119a_f4_queue_failure_cannot_erase_later_acceptance(void)
{
    reset_awning_worker_case(1U, 0.50);
    const panel_roller_interaction_t unrelated_before = s_awning_interaction[2];
    s_mock_awning_queue_fail = true;
    s_mock_awning_reenter_on_queue_failure = true;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_FAILED);
    assert(s_mock_awning_reenter_result == ATHOM_AWNING_QUEUE_BUSY);
    assert(!s_awning_interaction[1].active && !s_awning_interaction[1].failed);
    assert(s_awning_interaction[1].queued_count == 0U);
    assert(!s_awning_initial_enqueue_pending && !s_awning_job_reserved);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 0U);
    assert(memcmp(&unrelated_before, &s_awning_interaction[2],
                  sizeof(unrelated_before)) == 0);

    /* While the nonblocking enqueue is in progress, the pending flag rejects
     * reentrant admission. Once failed cleanup has completed, a later valid
     * request must be accepted and own a fresh queue reservation. */
    s_mock_awning_queue_fail = false;
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_awning_interaction[1].active && !s_awning_interaction[1].failed);
    assert(s_awning_job_reserved && s_awning_pending_widget == 1U);
    assert(s_patch059_queue.has_command && s_mock_awning_put_calls == 0U);
    assert(memcmp(&unrelated_before, &s_awning_interaction[2],
                  sizeof(unrelated_before)) == 0);
}

static void test_patch119b_f5_pending_intent_waits_for_network_phase_release(void)
{
    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED_NEXT);

    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U);
    assert(s_awning_outcome == PATCH085_AWNING_OUTCOME_ACCEPTED);
    assert(s_awning_interaction[1].active &&
           s_awning_interaction[1].queued_count == 1U);

    assert(network_phase_try_reserve(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH));
    s_mock_awning_context.snapshot_generation = 201U;
    s_mock_awning_context.current_position = 0.75;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(s_awning_motion_uncertain == false);
    assert(s_awning_next_dispatch_pending);
    assert(s_awning_interaction[1].active &&
           s_awning_interaction[1].queued_count == 0U &&
           !s_awning_interaction[1].failed);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);

    /* Repeated callbacks while the unrelated network phase owns transport
     * must retain the same unattempted intent without dispatching it. */
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_next_dispatch_pending);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_DOWN) ==
           ATHOM_AWNING_QUEUE_OPPOSITE_REJECTED);

    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    assert(s_awning_next_dispatch_pending && !s_awning_job_reserved);
    assert(!s_patch059_queue.has_command);
    homey_command_worker_run_deferred_work();
    assert(!s_awning_next_dispatch_pending);
    assert(s_awning_job_reserved && s_awning_pending_widget == 1U);
    assert(s_patch059_queue.has_command &&
           fabs(s_patch059_queue.command.numeric_value - 1.0) < 1e-9);
    assert(s_mock_awning_put_calls == 1U);

    /* A duplicate or late callback cannot enqueue another write. */
    athom_oauth_runtime_awning_snapshot_observed();
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    assert(s_patch059_queue.has_command &&
           s_patch059_queue.queued_count == 2U);
    assert(s_mock_awning_put_calls == 1U);
}

static void test_patch119b_f6_timeout_and_verified_recovery_policy(void)
{
    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U &&
           s_awning_outcome == PATCH085_AWNING_OUTCOME_ACCEPTED &&
           s_awning_motion_uncertain);
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED_NEXT);

    const uint64_t expiry_ms = s_awning_interaction[1].deadline_ms + 1U;
    athom_oauth_runtime_awning_tick(expiry_ms);
    assert(s_awning_interaction[1].failed &&
           !s_awning_interaction[1].active &&
           s_awning_interaction[1].queued_count == 0U);
    assert(s_awning_motion_uncertain && s_mock_awning_put_calls == 1U);
    athom_awning_interaction_snapshot_t display = {0};
    assert(athom_oauth_runtime_awning_interaction_snapshot(1U, &display));
    assert(display.failed && !display.active && display.pending_bar_mask == 0U);

    /* Fresh unchanged readback alone does not authorize a retry or clear an
     * accepted-but-unconfirmed operation. A later matching snapshot can
     * reconcile the accepted write; it sends no automatic continuation. */
    s_mock_awning_context.snapshot_generation = 201U;
    s_mock_awning_context.current_position = 0.50;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain && s_mock_awning_put_calls == 1U);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_BUSY);
    s_mock_awning_context.snapshot_generation = 202U;
    s_mock_awning_context.current_position = 0.75;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(!s_awning_motion_uncertain && !s_awning_interaction[1].failed);
    assert(s_mock_awning_put_calls == 1U && !s_patch059_queue.has_command);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);

    /* A complete 4xx plus fresh unchanged position is not terminal proof.
     * Keep durable WRITE_PENDING even when a later readback matches. */
    reset_awning_worker_case(1U, 0.50);
    s_mock_awning_use_http_response = true;
    s_mock_awning_response_complete = true;
    s_mock_awning_http_status = 403;
    s_mock_awning_perform_error = ESP_OK;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    s_mock_awning_context.snapshot_generation = 201U;
    s_mock_awning_context.current_position = 0.50;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(s_awning_motion_uncertain && s_awning_interaction[1].failed);
    assert(s_awning_guard_state == PANEL_AWNING_GUARD_WRITE_PENDING);
    assert(s_mock_guard_durable_record.state ==
           PANEL_AWNING_GUARD_WRITE_PENDING);
    s_mock_awning_context.snapshot_generation = 202U;
    s_mock_awning_context.current_position = 0.75;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(s_awning_interaction[1].failed && s_awning_motion_uncertain);
    assert(s_mock_guard_durable_record.state ==
           PANEL_AWNING_GUARD_WRITE_PENDING);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) != ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_mock_awning_put_calls == 1U);
}

static void test_patch119c_f7_snapshot_completes_between_admission_reads(void)
{
    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U && s_awning_interaction[1].active &&
           s_awning_motion_uncertain);

    /* The second request samples local_active first. The mock context boundary
     * then publishes the real production snapshot observer, completing the
     * prior interaction before the request re-enters its local-intent branch. */
    s_now_us += 500000LL;
    s_mock_confirmed_snapshot_generation = 201U;
    s_mock_confirmed_snapshot_position = 0.75;
    s_mock_confirm_snapshot_on_context_read = true;
    s_mock_run_worker_after_confirm_context_read = true;
    const athom_awning_queue_result_t result =
        athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP);
    printf("PATCH119C_F7_OBSERVED result=%d active=%d failed=%d queue=%d pending=%d owner=%d writes=%u\n",
           (int)result, (int)s_awning_interaction[1].active,
           (int)s_awning_interaction[1].failed,
           (int)s_patch059_queue.has_command,
           (int)s_awning_next_dispatch_pending,
           (int)s_network_phase_owner, s_mock_awning_put_calls);
    assert(!s_awning_interaction[1].active);
    assert(!s_awning_interaction[1].failed);
    assert(!s_awning_next_dispatch_pending && !s_awning_job_reserved);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_NONE);

    /* A different widget may win a fresh initial admission after the snapshot
     * completes the sampled interaction. The stale same-widget request must
     * not create a second active interaction or erase that accepted command. */
    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    s_now_us += 500000LL;
    s_mock_confirmed_snapshot_generation = 201U;
    s_mock_confirmed_snapshot_position = 0.75;
    s_mock_confirm_snapshot_on_context_read = true;
    s_mock_run_worker_after_confirm_context_read = true;
    s_mock_queue_other_widget_after_snapshot = true;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_BUSY);
    assert(s_mock_other_widget_queue_result == ATHOM_AWNING_QUEUE_QUEUED);
    assert(!s_awning_interaction[1].active && s_awning_interaction[2].active);
    assert(s_patch059_queue.has_command &&
           s_patch059_queue.command.widget_index == 2U);
    assert(s_mock_awning_put_calls == 1U &&
           s_network_phase_owner == ATHOM_NETWORK_PHASE_LIGHT_TOGGLE);

    /* A same-widget new interaction may also be accepted after the old one
     * completes. The racing request must not silently append to that newer
     * generation through its stale local_active sample. */
    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    s_now_us += 500000LL;
    s_mock_confirmed_snapshot_generation = 201U;
    s_mock_confirmed_snapshot_position = 0.75;
    s_mock_confirm_snapshot_on_context_read = true;
    s_mock_run_worker_after_confirm_context_read = true;
    s_mock_queue_same_widget_after_snapshot = true;
    const athom_awning_queue_result_t stale_same_widget_result =
        athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP);
    printf("PATCH119C_F7_REPLACED_OBSERVED stale_result=%d new_result=%d active=%d base=%u queued=%u command=%d owner=%d\n",
           (int)stale_same_widget_result,
           (int)s_mock_same_widget_queue_result,
           (int)s_awning_interaction[1].active,
           (unsigned)s_awning_interaction[1].base_generation,
           (unsigned)s_awning_interaction[1].queued_count,
           (int)s_patch059_queue.has_command,
           (int)s_network_phase_owner);
    assert(stale_same_widget_result == ATHOM_AWNING_QUEUE_BUSY);
    assert(s_mock_same_widget_queue_result == ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_awning_interaction[1].active &&
           s_awning_interaction[1].base_generation == 201U &&
           s_awning_interaction[1].queued_count == 0U);
    assert(s_patch059_queue.has_command && s_patch059_queue.queued_count == 2U);
    assert(s_mock_awning_put_calls == 1U &&
           s_network_phase_owner == ATHOM_NETWORK_PHASE_LIGHT_TOGGLE);
}

static void test_patch119d_nonrecursive_release_handoff(void)
{
    reset_awning_worker_case(1U, 0.0);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U && s_awning_motion_uncertain);

    s_mock_awning_context.snapshot_generation = 201U;
    s_mock_awning_context.current_position = 0.25;
    assert(network_phase_try_reserve(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH));
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(s_awning_next_dispatch_pending && s_awning_interaction[1].active);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);

    /* Release from the simulated HTTP/event caller only publishes a worker
     * notification. It cannot enter the dispatcher on this caller's stack. */
    s_mock_context_taps_remaining = 2U;
    const unsigned notify_before = s_worker_notify_count;
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    assert(s_worker_notify_count == notify_before + 1U);
    assert(s_worker_wake_bits != 0U);
    assert(s_awning_next_dispatch_pending && !s_awning_job_reserved);
    assert(!s_patch059_queue.has_command && s_mock_awning_put_calls == 1U);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_NONE);

    /* Each changed queued-count plan is retried only on a later worker pass;
     * no synchronous release-to-dispatch nesting occurs. */
    homey_command_worker_run_deferred_work();
    assert(s_mock_context_tap_result == ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    assert(s_awning_next_dispatch_pending && !s_awning_job_reserved);
    assert(!s_patch059_queue.has_command);
    homey_command_worker_run_deferred_work();
    assert(s_awning_next_dispatch_pending && !s_awning_job_reserved);
    assert(!s_patch059_queue.has_command);
    homey_command_worker_run_deferred_work();
    assert(!s_awning_next_dispatch_pending && s_awning_job_reserved);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_LIGHT_TOGGLE);
    assert(s_patch059_queue.has_command &&
           s_patch059_queue.command.kind == ATHOM_HOMEY_COMMAND_AWNING_POSITION &&
           s_patch059_queue.command.widget_index == 1U &&
           fabs(s_patch059_queue.command.numeric_value - 0.53) < 1e-9);
    assert(s_awning_interaction[1].queued_count == 2U);
    assert(s_mock_awning_put_calls == 1U);

    /* Duplicate callbacks cannot dispatch another copy while the command owns
     * the single network phase and reservation. */
    athom_oauth_runtime_awning_snapshot_observed();
    network_phase_release(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH);
    assert(s_patch059_queue.queued_count == 2U && s_mock_awning_put_calls == 1U);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 2U && s_patch059_queue.queued_count == 2U);
    assert(s_network_phase_owner == ATHOM_NETWORK_PHASE_NONE);
}
#endif

#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
static void test_awning_queue_worker_and_authoritative_confirmation(void)
{
    assert(PATCH085_AWNING_2_WRITE_ARMED == 1);
    assert(PATCH085_AWNING_3_WRITE_ARMED == 1);
    reset_awning_worker_case(1U, 0.50);
    s_observe_snapshot_during_awning_put = true;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_patch059_queue.queued_count == 1U);
    assert(s_patch059_queue.command.numeric_value == 0.75);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U);
    assert(!s_awning_job_reserved && s_awning_motion_uncertain);
    assert(s_awning_write_result_ready &&
           s_awning_confirmation_after_generation == 201U);
    /* A snapshot observed while PUT is active cannot unlock, and the same
     * snapshot cannot be reused after transport completion. */
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_DOWN) ==
           ATHOM_AWNING_QUEUE_OPPOSITE_REJECTED);
    s_mock_awning_context.snapshot_generation = 202U;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(!s_awning_motion_uncertain && !s_awning_write_result_ready);
    assert(s_mock_awning_put_calls == 1U);

    reset_awning_worker_case(1U, 0.50);
    s_now_us = 1000000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    s_now_us = 1500000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    assert(s_patch059_queue.queued_count == 1U &&
           s_awning_interaction[1].queued_count == 1U);
    s_now_us = 2000000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_DOWN) ==
           ATHOM_AWNING_QUEUE_OPPOSITE_REJECTED);
    assert(s_patch059_queue.queued_count == 1U &&
           s_awning_interaction[1].queued_count == 1U);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U && s_awning_motion_uncertain);
    s_mock_awning_context.snapshot_generation = 201U;
    s_mock_awning_context.current_position = 0.75;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(s_patch059_queue.has_command &&
           fabs(s_patch059_queue.command.numeric_value - 1.0) < 1e-9);
    assert(s_mock_awning_put_calls == 1U);
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_patch059_queue.queued_count == 2U);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 2U);
    s_mock_awning_context.snapshot_generation = 202U;
    s_mock_awning_context.current_position = 1.0;
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(!s_awning_motion_uncertain && !s_awning_interaction[1].active);

    /* A fresh authoritative snapshot that has passed an unsent queued target
     * reconciles that intent without issuing a duplicate PUT. */
    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    s_now_us += 500000LL;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    run_one_worker_command();
    s_mock_awning_context.snapshot_generation = 201U;
    s_mock_awning_context.current_position = 1.0; /* step 0, beyond queued step 0 */
    athom_oauth_runtime_awning_snapshot_observed();
    homey_command_worker_run_deferred_work();
    assert(!s_awning_motion_uncertain && !s_awning_interaction[1].active);
    assert(s_patch059_queue.queued_count == 1U && !s_patch059_queue.has_command);
    assert(s_mock_awning_put_calls == 1U);

    reset_awning_worker_case(2U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(2U, PANEL_ROLLER_DIRECTION_DOWN) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_patch059_queue.command.numeric_value == 0.25);
    s_mock_awning_context.alias_generation++;
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 0U);
    assert(!s_awning_job_reserved && !s_awning_motion_uncertain);

    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    s_mock_active_nvs_binding_matches = false;
    run_one_worker_command();
    assert(s_mock_awning_dispatch_calls == 1U && s_mock_awning_put_calls == 0U);
    assert(!s_awning_motion_uncertain);

    reset_awning_worker_case(2U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(2U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    s_mock_selected_homey_valid = false;
    run_one_worker_command();
    assert(s_mock_awning_dispatch_calls == 0U && s_mock_awning_put_calls == 0U);

    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    s_mock_awning_context.snapshot_generation++;
    run_one_worker_command();
    assert(s_mock_awning_dispatch_calls == 0U && s_mock_awning_put_calls == 0U);

    reset_awning_worker_case(1U, 0.50);
    s_mock_awning_write_result = ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    run_one_worker_command();
    assert(s_mock_awning_put_calls == 1U && s_awning_motion_uncertain);
    s_mock_awning_context.snapshot_generation = 201U;
    s_mock_awning_context.current_position = 0.50;
    athom_oauth_runtime_awning_snapshot_observed();
    assert(s_awning_motion_uncertain);
    assert(s_mock_awning_put_calls == 1U);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_BUSY);

    reset_awning_worker_case(3U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(3U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_INVALID_WIDGET);
    assert(s_mock_awning_put_calls == 0U);

    reset_awning_worker_case(1U, 0.50);
    s_mock_awning_queue_fail = true;
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_FAILED);
    assert(!s_awning_interaction[1].active &&
           s_awning_interaction[1].queued_count == 0U);
    s_mock_awning_queue_fail = false;
    reset_awning_worker_case(2U, 0.50);
    s_network_phase_owner = ATHOM_NETWORK_PHASE_INVENTORY_REFRESH;
    assert(athom_oauth_runtime_queue_awning_step(2U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_BUSY);
    assert(!s_awning_interaction[2].active);
    s_network_phase_owner = ATHOM_NETWORK_PHASE_NONE;

    reset_awning_worker_case(1U, 0.50);
    assert(athom_oauth_runtime_queue_awning_step(1U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_QUEUED);
    assert(athom_oauth_runtime_queue_awning_step(2U, PANEL_ROLLER_DIRECTION_UP) ==
           ATHOM_AWNING_QUEUE_BUSY);
    assert(!s_awning_interaction[2].active &&
           s_patch059_queue.queued_count == 1U);
}

static void test_awning_request_debounce_matches_450ms_boundary(void)
{
    reset_awning_worker_case(1U, 0.50);
    s_now_us = 1000000LL;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED);
    assert(s_awning_interaction[1].last_request_ms == 1000U);

    /* Release the synthetic queued job without running transport so the
     * next calls test only production admission/debounce timing. */
    s_patch059_queue.has_command = false;
    s_awning_job_reserved = false;
    network_phase_release(ATHOM_NETWORK_PHASE_LIGHT_TOGGLE);
    s_now_us = 1449000LL;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_DEBOUNCED);
    assert(s_patch059_queue.queued_count == 1U);

    s_now_us = 1450000LL;
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_QUEUED_NEXT);
    assert(s_patch059_queue.queued_count == 1U);
    assert(s_awning_interaction[1].last_request_ms == 1450U);
}
#endif

static void test_awning_compile_arms_gate_queue_and_worker_independently(void)
{
    for (size_t widget = 1U; widget <= 2U; ++widget) {
        reset_awning_worker_case(widget, 0.50);
        const bool armed = athom_cloud_awning_write_is_armed(widget);
        const athom_awning_queue_result_t queued =
            athom_oauth_runtime_queue_awning_step(widget, PANEL_ROLLER_DIRECTION_UP);
        assert(queued == (armed ? ATHOM_AWNING_QUEUE_QUEUED :
                                  ATHOM_AWNING_QUEUE_NOT_READY));
        if (armed) {
            s_patch059_queue.has_command = false;
            s_awning_job_reserved = false;
            network_phase_release(ATHOM_NETWORK_PHASE_LIGHT_TOGGLE);
        }

        /* Even an injected queued command cannot bypass a disabled worker gate. */
        if (!armed) {
            reset_awning_worker_case(widget, 0.50);
            s_patch059_queue.command = (athom_homey_command_t){
                .kind = ATHOM_HOMEY_COMMAND_AWNING_POSITION,
                .widget_index = widget,
                .direction = PANEL_ROLLER_DIRECTION_UP,
                .numeric_value = 0.75,
                .awning_context = s_mock_awning_context,
            };
            s_patch059_queue.has_command = true;
            run_one_worker_command();
            assert(s_mock_awning_put_calls == 0U);
        }
    }
}

#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
static void test_patch119p4i_snapshot_uses_worker_direction_only(void)
{
    const athom_homey_awning_write_result_t outcomes[] = {
        ATHOM_HOMEY_AWNING_WRITE_ACCEPTED, ATHOM_HOMEY_AWNING_WRITE_REJECTED,
        ATHOM_HOMEY_AWNING_WRITE_UNAUTHORIZED, ATHOM_HOMEY_AWNING_WRITE_TRANSPORT_AMBIGUOUS,
    };
    for(size_t widget=1U;widget<=2U;widget++) {
        for(int direction=-1;direction<=1;direction+=2) {
            for(size_t result=0;result<4U;result++) {
                reset_awning_worker_case(widget,0.50);
                s_mock_awning_write_result=outcomes[result];
                athom_awning_interaction_snapshot_t view={0};
                assert(athom_oauth_runtime_queue_awning_step(widget,(int8_t)direction)==ATHOM_AWNING_QUEUE_QUEUED);
                assert(athom_oauth_runtime_awning_interaction_snapshot(widget,&view));
                assert(view.active&&view.active_direction==0); /* accepted queue, not dispatched */
                run_one_worker_command();
                assert(s_mock_awning_put_calls==1U);
                assert(athom_oauth_runtime_awning_interaction_snapshot(widget,&view));
                assert(view.active_direction==(result==0U?direction:0));
                assert(athom_oauth_runtime_awning_interaction_snapshot(widget==1U?2U:1U,&view));
                assert(view.active_direction==0); /* another widget never inherits direction */
                if(result==0U) {
                    const uint64_t deadline=s_awning_interaction[widget].deadline_ms;
                    athom_oauth_runtime_awning_tick(deadline+1U);
                    assert(athom_oauth_runtime_awning_interaction_snapshot(widget,&view));
                    assert(!view.active&&view.failed&&view.active_direction==0);
                    assert(s_awning_guard_state==PANEL_AWNING_GUARD_WRITE_PENDING);
                }
            }
        }
    }
    puts("PATCH119P4I_PRODUCTION_WORKER_PRESENTATION_SNAPSHOT PASS cases=16");
}
#endif

int main(void)
{
#if defined(PATCH119A_ONLY_F1)
    test_patch119a_f1_multitap_during_write_and_readback();
    puts("PATCH119A_F1=PASS");
    return 0;
#elif defined(PATCH119A_ONLY_F2)
    test_patch119a_f2_worker_prewrite_rejection_cancels_intents();
    puts("PATCH119A_F2=PASS");
    return 0;
#elif defined(PATCH119A_ONLY_F3)
    test_patch119a_f3_nonaccepted_result_never_continues();
    puts("PATCH119A_F3=PASS");
    return 0;
#elif defined(PATCH119A_ONLY_F4)
    test_patch119a_f4_queue_failure_cannot_erase_later_acceptance();
    puts("PATCH119A_F4=PASS");
    return 0;
#elif defined(PATCH119B_ONLY_F5)
    test_patch119b_f5_pending_intent_waits_for_network_phase_release();
    puts("PATCH119B_F5=PASS");
    return 0;
#elif defined(PATCH119B_ONLY_F6)
    test_patch119b_f6_timeout_and_verified_recovery_policy();
    puts("PATCH119B_F6=PASS");
    return 0;
#elif defined(PATCH119C_ONLY_F7)
    test_patch119c_f7_snapshot_completes_between_admission_reads();
    puts("PATCH119C_F7=PASS");
    return 0;
#elif defined(PATCH119C_ONLY_F8)
    test_patch119d_nonrecursive_release_handoff();
    puts("PATCH119D_RELEASE_HANDOFF=PASS");
    return 0;
#elif defined(PATCH119D_ONLY)
    test_patch119d_nonrecursive_release_handoff();
    test_patch119d_worker_idle_notification_race();
    test_patch119d_preselection_serializes_continuation();
    test_patch119d_identity_timeout_and_worker_failure();
    puts("PATCH119D_DEFERRED_RELEASE_SAFETY=PASS");
    return 0;
#elif defined(PATCH119E1A_ONLY)
    test_patch119e_homey_ready_is_independent_of_awning_guard();
    test_patch119e1a_http_terminal_classifier_matrix();
    test_patch119e1a_rejected_http_cannot_clear_pending_guard();
    test_patch119e1a_integrated_restart_boundaries();
    puts("PATCH119E1A_INTEGRATED_GUARD_TERMINAL_SAFETY=PASS");
    return 0;
#elif defined(PATCH119E2B_ONLY)
    test_patch119e2b_commissioning_requires_local_review_and_worker();
    test_patch119e2c_commissioning_review_formats_normalized_positions();
    test_patch119e2b_commissioning_fails_closed_on_mismatch_or_activity();
    test_patch119e2b_missing_and_pending_records_are_distinct();
    puts("PATCH119E2B_CONTROLLED_COMMISSIONING=PASS");
    return 0;
#endif
    test_favorites_transport_failure_keeps_worker_inventory_ready();
    (void)patch058_main();
    test_fresh_due_scheduler_worker_publication_and_new_basis();
    test_stale_snapshot_remains_eligible_and_recovers();
    test_nonready_runtime_can_recover_without_opening_write_gate();
    test_manual_429_falls_back_to_valid_cached_session();
    test_cloud_auth_and_integrity_failures_do_not_fall_back();
    test_boot_429_uses_valid_cache_or_preserves_discovery_failure();
    test_boot_cache_validation_fallback_and_alias_fail_closed();
    test_boot_429_cannot_leave_attempt_sequence_stuck_forever();
    test_periodic_path_has_no_cloud_discovery_retry_loop();
    test_scheduler_recovers_without_initial_snapshot_after_cooldown();
    test_zone_eagain_after_boot_exhaustion_recovers_without_snapshot();
    test_patch119e_homey_ready_is_independent_of_awning_guard();
#if PATCH085_AWNING_2_WRITE_ARMED && PATCH085_AWNING_3_WRITE_ARMED
    test_patch119p4i_snapshot_uses_worker_direction_only();
    test_patch119e1a_http_terminal_classifier_matrix();
    test_patch119e1a_rejected_http_cannot_clear_pending_guard();
    test_patch119e1a_integrated_restart_boundaries();
    test_awning_queue_worker_and_authoritative_confirmation();
    test_awning_request_debounce_matches_450ms_boundary();
    test_patch119a_f1_multitap_during_write_and_readback();
    test_patch119a_f2_worker_prewrite_rejection_cancels_intents();
    test_patch119a_f3_nonaccepted_result_never_continues();
    test_patch119a_f4_queue_failure_cannot_erase_later_acceptance();
    test_patch119b_f5_pending_intent_waits_for_network_phase_release();
    test_patch119b_f6_timeout_and_verified_recovery_policy();
    test_patch119c_f7_snapshot_completes_between_admission_reads();
    test_patch119d_nonrecursive_release_handoff();
    test_patch119d_worker_idle_notification_race();
    test_patch119d_preselection_serializes_continuation();
    test_patch119d_identity_timeout_and_worker_failure();
#endif
    test_awning_compile_arms_gate_queue_and_worker_independently();
    puts("PATCH059_SCHEDULER_WORKER_INTEGRATION=PASS");
    return 0;
}
