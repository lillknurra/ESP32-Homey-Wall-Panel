#include "panel_ota_supervisor.h"

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stddef.h>

#define OTA_WATCHDOG_MS 900000U
#define OTA_SUPERVISOR_STACK_BYTES 2048U

typedef enum {
    DECISION_NONE = 0,
    DECISION_WORKER,
    DECISION_SUPERVISOR,
} terminal_decision_t;

static portMUX_TYPE s_state_mux = portMUX_INITIALIZER_UNLOCKED;
static terminal_decision_t s_decision;
static TaskHandle_t s_supervisor_task;
static bool s_flash_session;
static bool s_native_operation;
static bool s_boot_selection_reserved;
static bool s_timeout_pending;

static bool stop_is_ready(void)
{
    bool ready;
    portENTER_CRITICAL(&s_state_mux);
    ready = s_decision == DECISION_SUPERVISOR && !s_flash_session &&
            !s_native_operation;
    portEXIT_CRITICAL(&s_state_mux);
    return ready;
}

static void supervisor_task(void *arg)
{
    (void)arg;
    const uint32_t notified = ulTaskNotifyTake(
        pdTRUE, pdMS_TO_TICKS(OTA_WATCHDOG_MS));
    if (notified == 0U) (void)panel_ota_supervisor_watchdog_expired();
    for (;;) {
        portENTER_CRITICAL(&s_state_mux);
        const terminal_decision_t decision = s_decision;
        portEXIT_CRITICAL(&s_state_mux);
        if (decision == DECISION_WORKER) {
            /* The worker notifies using this handle after publishing its
             * result. Keep the handle valid through the immediately planned
             * restart; never self-delete in the notification race window. */
            for (;;) vTaskDelay(portMAX_DELAY);
        }
        if (decision == DECISION_SUPERVISOR && stop_is_ready()) {
            esp_restart();
            for (;;) vTaskDelay(portMAX_DELAY);
        }
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

bool panel_ota_supervisor_start(void)
{
    portENTER_CRITICAL(&s_state_mux);
    s_decision = DECISION_NONE;
    s_supervisor_task = NULL;
    s_flash_session = false;
    s_native_operation = false;
    s_boot_selection_reserved = false;
    s_timeout_pending = false;
    portEXIT_CRITICAL(&s_state_mux);
    return xTaskCreate(supervisor_task, "ota_supervisor",
                       OTA_SUPERVISOR_STACK_BYTES, NULL, 5U,
                       &s_supervisor_task) == pdPASS;
}

bool panel_ota_supervisor_worker_authorized(void)
{
    bool authorized;
    portENTER_CRITICAL(&s_state_mux);
    authorized = s_decision == DECISION_NONE;
    portEXIT_CRITICAL(&s_state_mux);
    return authorized;
}

bool panel_ota_supervisor_flash_session_begin(void)
{
    bool admitted;
    portENTER_CRITICAL(&s_state_mux);
    admitted = s_decision == DECISION_NONE && !s_flash_session &&
               !s_boot_selection_reserved;
    if (admitted) s_flash_session = true;
    portEXIT_CRITICAL(&s_state_mux);
    return admitted;
}

bool panel_ota_supervisor_flash_operation_begin(void)
{
    bool admitted;
    portENTER_CRITICAL(&s_state_mux);
    admitted = s_decision == DECISION_NONE && s_flash_session &&
               !s_native_operation && !s_boot_selection_reserved;
    if (admitted) s_native_operation = true;
    portEXIT_CRITICAL(&s_state_mux);
    return admitted;
}

bool panel_ota_supervisor_flash_cleanup_begin(void)
{
    bool admitted;
    portENTER_CRITICAL(&s_state_mux);
    /* Abort is admitted only before timeout revocation. After timeout, the
     * partial inactive slot is left unselected and reboot will discard its
     * volatile OTA handle state. */
    admitted = s_decision == DECISION_NONE && s_flash_session &&
               !s_native_operation && !s_boot_selection_reserved;
    if (admitted) s_native_operation = true;
    portEXIT_CRITICAL(&s_state_mux);
    return admitted;
}

bool panel_ota_supervisor_boot_selection_begin(void)
{
    bool admitted;
    portENTER_CRITICAL(&s_state_mux);
    admitted = s_decision == DECISION_NONE && s_flash_session &&
               !s_native_operation && !s_boot_selection_reserved;
    if (admitted) {
        /* This atomic reservation is the deadline boundary: after it wins,
         * the supervisor may not reset until the worker reports the native
         * selection result. */
        s_boot_selection_reserved = true;
        s_native_operation = true;
    }
    portEXIT_CRITICAL(&s_state_mux);
    return admitted;
}

void panel_ota_supervisor_native_operation_end(void)
{
    TaskHandle_t supervisor = NULL;
    portENTER_CRITICAL(&s_state_mux);
    s_native_operation = false;
    if (s_decision == DECISION_SUPERVISOR) supervisor = s_supervisor_task;
    portEXIT_CRITICAL(&s_state_mux);
    if (supervisor != NULL) xTaskNotifyGive(supervisor);
}

void panel_ota_supervisor_flash_session_end(void)
{
    TaskHandle_t supervisor = NULL;
    portENTER_CRITICAL(&s_state_mux);
    s_flash_session = false;
    if (s_decision == DECISION_SUPERVISOR) supervisor = s_supervisor_task;
    portEXIT_CRITICAL(&s_state_mux);
    if (supervisor != NULL) xTaskNotifyGive(supervisor);
}

void panel_ota_supervisor_boot_selection_complete(bool selected)
{
    TaskHandle_t supervisor = NULL;
    bool record_staged = false;
    bool record_timeout = false;
    portENTER_CRITICAL(&s_state_mux);
    /* The ESP-IDF operation has returned, so it is now safe to end the flash
     * session. A successful selection commits the worker-owned restart before
     * HTTP/TLS teardown. A failed selection releases the reservation; if the
     * deadline elapsed during the native call, timeout owns the outcome. */
    s_native_operation = false;
    s_boot_selection_reserved = false;
    s_flash_session = false;
    if (selected && s_decision == DECISION_NONE) {
        s_decision = DECISION_WORKER;
        record_staged = true;
        supervisor = s_supervisor_task;
    } else if (!selected && s_decision == DECISION_NONE && s_timeout_pending) {
        s_decision = DECISION_SUPERVISOR;
        record_timeout = true;
        supervisor = s_supervisor_task;
    }
    s_timeout_pending = false;
    portEXIT_CRITICAL(&s_state_mux);

    if (record_staged) {
        panel_ota_handoff_record_result(PANEL_OTA_HANDOFF_RESULT_STAGED);
    } else if (record_timeout) {
        panel_ota_handoff_record_result(
            PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT);
    }
    if (supervisor != NULL) xTaskNotifyGive(supervisor);
}

/* Called by the timer task and by deterministic integration tests at the
 * exact deadline boundary. Returns true only when timeout owns the outcome. */
bool panel_ota_supervisor_watchdog_expired(void)
{
    bool owns_timeout = false;
    portENTER_CRITICAL(&s_state_mux);
    if (s_decision == DECISION_NONE) {
        if (s_boot_selection_reserved) {
            /* Reservation admission won before the deadline. Let the native
             * API return, then resolve success vs timeout without resetting
             * across the boot-selection operation. */
            s_timeout_pending = true;
        } else {
            s_decision = DECISION_SUPERVISOR;
            owns_timeout = true;
        }
    }
    portEXIT_CRITICAL(&s_state_mux);
    if (owns_timeout) {
        panel_ota_handoff_record_result(
            PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT);
    }
    return owns_timeout;
}

bool panel_ota_supervisor_timeout_restart_ready(void)
{
    return stop_is_ready();
}

bool panel_ota_supervisor_worker_complete(panel_ota_handoff_result_t result)
{
    TaskHandle_t supervisor = NULL;
    bool owns_restart = false;
    portENTER_CRITICAL(&s_state_mux);
    if (s_decision == DECISION_NONE && !s_flash_session &&
        !s_native_operation) {
        s_decision = DECISION_WORKER;
        supervisor = s_supervisor_task;
        owns_restart = true;
    }
    portEXIT_CRITICAL(&s_state_mux);
    if (owns_restart) {
        panel_ota_handoff_record_result(result);
        if (supervisor != NULL) xTaskNotifyGive(supervisor);
    }
    return owns_restart;
}
