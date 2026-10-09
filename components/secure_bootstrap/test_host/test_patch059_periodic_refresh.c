#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define ATHOM_HOMEY_DATA_RETRY_1_MS 5000U
#define ATHOM_HOMEY_DATA_RETRY_2_MS 10000U
#define ATHOM_HOMEY_DATA_RETRY_3_MS 20000U
#define ATHOM_HOMEY_DATA_RETRY_MAX_MS 30000U
#define ATHOM_HOMEY_DATA_429_RETRY_1_MS 60000U
#define ATHOM_HOMEY_DATA_429_RETRY_2_MS 120000U
#define ATHOM_HOMEY_DATA_429_RETRY_3_MS 240000U
#define ATHOM_HOMEY_DATA_429_RETRY_MAX_MS 300000U
#include <string.h>

#define PERIODIC_REFRESH_INTERVAL_MS 60000ULL
#define PERIODIC_REFRESH_BUSY_DEFER_MS 5000ULL
#define PERIODIC_REFRESH_COOLDOWN_MS 30000ULL

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
    char id[16];
} selected_homey_t;

typedef struct {
    selected_homey_t selected_homey;
    char homey_session_token[16];
} athom_cloud_state_t;

typedef struct {
    bool snapshot_seen;
    uint64_t snapshot_captured_at_ms;
    uint64_t next_attempt_ms;
} periodic_refresh_scheduler_state_t;

typedef struct {
    unsigned sent;
    bool accept;
    athom_homey_command_t last;
} fake_queue_t;

static athom_cloud_state_t s_cloud;
static void *s_homey_command_queue;
static fake_queue_t s_fake_queue;
static bool s_wifi_online = true;
static bool s_worker_running;
static bool s_select_worker_running;
static bool s_restore_worker_running;
static bool s_preselection_restore_worker_running;
static bool s_refresh_job_reserved;
static bool s_light_toggle_job_reserved;
static bool s_diag_probe_active;
static bool s_network_reserved;
static int s_patch031_diag_probe_mux;
static int s_refresh_job_mux;

#define ATHOM_NETWORK_PHASE_INVENTORY_REFRESH 1
#define pdTRUE 1
#define portENTER_CRITICAL(mux) ((void)(mux))
#define portEXIT_CRITICAL(mux) ((void)(mux))

static bool phone_provisioning_wifi_online(void)
{
    return s_wifi_online;
}

static bool patch031_diag_probe_active_locked(void)
{
    return s_diag_probe_active;
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
    assert(queue == &s_fake_queue);
    if (!s_fake_queue.accept) return 0;
    s_fake_queue.last = *command;
    s_fake_queue.sent++;
    return pdTRUE;
}

/* PATCH059_PRODUCTION_FUNCTIONS */

static void reset_fixture(void)
{
    memset(&s_cloud, 0, sizeof(s_cloud));
    memset(&s_fake_queue, 0, sizeof(s_fake_queue));
    strcpy(s_cloud.selected_homey.id, "selected");
    strcpy(s_cloud.homey_session_token, "session");
    s_fake_queue.accept = true;
    s_homey_command_queue = &s_fake_queue;
    s_wifi_online = true;
    s_worker_running = false;
    s_select_worker_running = false;
    s_restore_worker_running = false;
    s_preselection_restore_worker_running = false;
    s_refresh_job_reserved = false;
    s_light_toggle_job_reserved = false;
    s_diag_probe_active = false;
    s_network_reserved = false;
}

static void test_periodic_deadline_and_snapshot_reset(void)
{
    periodic_refresh_scheduler_state_t state = {0};
    assert(!periodic_refresh_scheduler_should_attempt(
        &state, false, 0U, 0U, 0U));
    assert(!periodic_refresh_scheduler_should_attempt(
        &state, true, 1000U, 59999U, 60999U));
    assert(periodic_refresh_scheduler_should_attempt(
        &state, true, 1000U, 60000U, 61000U));

    periodic_refresh_scheduler_record_queue_result(
        &state, ATHOM_REFRESH_QUEUE_OK, 61000U);
    assert(!periodic_refresh_scheduler_should_attempt(
        &state, true, 1000U, 90000U, 90999U));
    assert(periodic_refresh_scheduler_should_attempt(
        &state, true, 1000U, 90000U, 91000U));

    periodic_refresh_scheduler_record_queue_result(
        &state, ATHOM_REFRESH_QUEUE_FAILED, 91000U);
    assert(!periodic_refresh_scheduler_should_attempt(
        &state, true, 1000U, 120001U, 120999U));
    assert(periodic_refresh_scheduler_should_attempt(
        &state, true, 1000U, 120001U, 121000U));

    assert(!periodic_refresh_scheduler_should_attempt(
        &state, true, 121001U, 0U, 121001U));
    assert(state.snapshot_captured_at_ms == 121001U);
    assert(state.next_attempt_ms == 0U);
}

static void test_defer_intervals_are_bounded(void)
{
    assert(periodic_refresh_scheduler_defer_ms(
        ATHOM_REFRESH_QUEUE_BUSY) == 5000U);
    assert(periodic_refresh_scheduler_defer_ms(
        ATHOM_REFRESH_QUEUE_NOT_READY) == 5000U);
    assert(periodic_refresh_scheduler_defer_ms(
        ATHOM_REFRESH_QUEUE_OK) == 30000U);
    assert(periodic_refresh_scheduler_defer_ms(
        ATHOM_REFRESH_QUEUE_FAILED) == 30000U);

    periodic_refresh_scheduler_state_t state = {
        .snapshot_seen = true,
        .snapshot_captured_at_ms = 10U,
    };
    periodic_refresh_scheduler_record_queue_result(
        &state, ATHOM_REFRESH_QUEUE_BUSY, 1000U);
    assert(state.next_attempt_ms == 6000U);
    periodic_refresh_scheduler_record_queue_result(
        &state, ATHOM_REFRESH_QUEUE_FAILED, 1000U);
    assert(state.next_attempt_ms == 31000U);
}

static void test_refresh_retry_policy_is_origin_specific(void)
{
    assert(inventory_refresh_worker_should_retry(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true));
    assert(!inventory_refresh_worker_should_retry(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, false));
    assert(!inventory_refresh_worker_should_retry(
        ATHOM_REFRESH_ORIGIN_MANUAL, true));
    assert(!inventory_refresh_worker_should_retry(
        ATHOM_REFRESH_ORIGIN_PERIODIC, true));
    assert(inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, 1U, true,
        "inventory_devices", 429));
    assert(inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, 8U, true,
        "inventory_devices", 429));
    assert(inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, false, 1U, true,
        "cached_session_validation", 0));
    assert(inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, false, 1U, true,
        "cached_alias_validation", 0));
    assert(!inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, 1U, true,
        "inventory_devices", 401));
    assert(!inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, 1U, true,
        "inventory_devices", 403));
    assert(!inventory_refresh_worker_should_retry_after_cloud_429(
        ATHOM_REFRESH_ORIGIN_PERIODIC, true, 1U, false,
        "inventory_devices", 429));
    assert(homey_data_retry_delay_ms_for_failure(1U, 429, false) == 60000U);
    assert(homey_data_retry_delay_ms_for_failure(2U, 0, true) == 120000U);
    assert(homey_data_retry_delay_ms_for_failure(3U, 429, true) == 240000U);
    assert(homey_data_retry_delay_ms_for_failure(4U, 429, true) == 300000U);
    assert(homey_data_retry_delay_ms_for_failure(9U, 503, false) == 30000U);
}

static void test_queue_serialization_and_periodic_origin(void)
{
    reset_fixture();
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_PERIODIC) == ATHOM_REFRESH_QUEUE_OK);
    assert(s_fake_queue.sent == 1U);
    assert(s_fake_queue.last.kind == ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA);
    assert(s_fake_queue.last.origin == ATHOM_REFRESH_ORIGIN_PERIODIC);
    assert(s_refresh_job_reserved && s_network_reserved);

    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_PERIODIC) == ATHOM_REFRESH_QUEUE_BUSY);
    assert(s_fake_queue.sent == 1U);

    reset_fixture();
    s_network_reserved = true;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_PERIODIC) == ATHOM_REFRESH_QUEUE_BUSY);
    assert(s_fake_queue.sent == 0U);

    reset_fixture();
    s_wifi_online = false;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_PERIODIC) == ATHOM_REFRESH_QUEUE_NOT_READY);
    assert(s_fake_queue.sent == 0U);

    reset_fixture();
    s_fake_queue.accept = false;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_PERIODIC) == ATHOM_REFRESH_QUEUE_FAILED);
    assert(s_fake_queue.sent == 0U);
    assert(!s_refresh_job_reserved && !s_network_reserved);

    reset_fixture();
    s_refresh_job_reserved = true;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_PERIODIC) == ATHOM_REFRESH_QUEUE_BUSY);
    assert(s_fake_queue.sent == 0U);
    assert(!s_network_reserved);
}

int main(void)
{
    test_periodic_deadline_and_snapshot_reset();
    test_defer_intervals_are_bounded();
    test_refresh_retry_policy_is_origin_specific();
    test_queue_serialization_and_periodic_origin();
    return 0;
}
