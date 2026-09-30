#include <setjmp.h>

#ifndef ESP_LOGE
#define ESP_LOGE(tag, ...) test_log((tag), __VA_ARGS__)
#endif

typedef enum {
    ATHOM_REFRESH_ORIGIN_BOOT_AUTO = 0,
    ATHOM_REFRESH_ORIGIN_MANUAL,
    ATHOM_REFRESH_ORIGIN_PERIODIC,
} athom_refresh_origin_t;

typedef enum {
    ATHOM_REFRESH_QUEUE_OK = 0,
    ATHOM_REFRESH_QUEUE_NOT_READY,
    ATHOM_REFRESH_QUEUE_BUSY,
    ATHOM_REFRESH_QUEUE_FAILED,
} athom_refresh_queue_result_t;

typedef enum {
    ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA = 1,
    ATHOM_HOMEY_COMMAND_LIGHT_TOGGLE,
} athom_homey_command_kind_t;

typedef struct {
    athom_homey_command_kind_t kind;
    athom_refresh_origin_t origin;
    size_t widget_index;
    bool value;
} athom_homey_command_t;

typedef struct {
    bool present;
    panel_homey_read_result_t result;
    panel_homey_read_snapshot_t snapshot;
    uint64_t age_ms;
} panel_homey_snapshot_inspection_t;

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
static bool s_refresh_job_reserved;
static bool s_light_toggle_job_reserved;
static bool s_schema_refresh_running;
static bool s_diag_probe_active;
static bool s_network_reserved;
static bool s_dashboard_visible;
static bool s_provisioning_runtime_ready;
static unsigned s_worker_inventory_commands;
static unsigned s_periodic_inventory_attempts;
static unsigned s_write_attempts;
static unsigned s_scheduler_iterations;
static unsigned s_scheduler_iteration_limit;
static athom_refresh_origin_t s_current_refresh_origin;
static bool s_worker_active;
static jmp_buf s_scheduler_exit;
static jmp_buf s_worker_exit;
static int s_patch059_refresh_mux;
static int s_patch059_diag_mux;

static void homey_command_worker(void *arg);

#define ATHOM_NETWORK_PHASE_INVENTORY_REFRESH 1
#define ATHOM_NETWORK_PHASE_LIGHT_TOGGLE 2
#define ATHOM_HOMEY_ID_MAX 64U
#define PERIODIC_REFRESH_INTERVAL_MS 60000ULL
#define PERIODIC_REFRESH_BUSY_DEFER_MS 5000ULL
#define PERIODIC_REFRESH_COOLDOWN_MS 30000ULL
#define PERIODIC_REFRESH_SCHEDULER_POLL_MS 1000U
#define ATHOM_BOOT_AUTO_READY_WAIT_MS 1000U
#define ATHOM_HOMEY_DATA_RETRY_1_MS 1000U
#define ATHOM_HOMEY_DATA_RETRY_2_MS 2000U
#define ATHOM_HOMEY_DATA_RETRY_3_MS 4000U
#define ATHOM_HOMEY_DATA_RETRY_MAX_MS 8000U
#define ESP_ERR_HTTP_EAGAIN (-20)
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

static athom_homey_data_state_t s_homey_data_state;
static const char *s_state_name;

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

static bool network_phase_try_reserve(int owner)
{
    (void)owner;
    if (s_network_reserved) return false;
    s_network_reserved = true;
    return true;
}

static void network_phase_release(int owner)
{
    (void)owner;
    s_network_reserved = false;
}

static int xQueueSend(void *queue, const athom_homey_command_t *command, int wait)
{
    (void)wait;
    assert(queue == &s_patch059_queue);
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
    if (s_current_refresh_origin == ATHOM_REFRESH_ORIGIN_PERIODIC) {
        s_periodic_inventory_attempts++;
    }
    s_http_status = 200;
    s_diagnostic_http_status = 200;
    s_diagnostic_error = ESP_OK;
    s_diagnostic_stage = "homeys_complete";
    return ESP_OK;
}

static esp_err_t athom_cloud_refresh(athom_cloud_state_t *state)
{
    (void)state;
    return ESP_FAIL;
}

static esp_err_t athom_cloud_select_and_connect(
    athom_cloud_state_t *state, const char *homey_id)
{
    assert(state != NULL && homey_id != NULL);
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
    s_refresh_job_reserved = false;
    s_light_toggle_job_reserved = false;
    s_schema_refresh_running = false;
    s_diag_probe_active = false;
    s_network_reserved = false;
    s_dashboard_visible = true;
    s_provisioning_runtime_ready = true;
    s_worker_inventory_commands = 0U;
    s_periodic_inventory_attempts = 0U;
    s_write_attempts = 0U;
    s_scheduler_iterations = 0U;
    s_scheduler_iteration_limit = 3U;
    s_worker_active = false;
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
    assert(s_periodic_inventory_attempts == 1U);
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
    assert(s_periodic_inventory_attempts == 1U);
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

int main(void)
{
    (void)patch058_main();
    test_fresh_due_scheduler_worker_publication_and_new_basis();
    test_stale_snapshot_remains_eligible_and_recovers();
    test_nonready_runtime_can_recover_without_opening_write_gate();
    puts("PATCH059_SCHEDULER_WORKER_INTEGRATION=PASS");
    return 0;
}
