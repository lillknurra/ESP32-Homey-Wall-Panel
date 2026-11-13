#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>

#include "freertos/task.h"
#include "panel_ota_handoff.h"
#include "panel_ota_supervisor.h"

struct test_task_control { unsigned notifications; };
static struct test_task_control task_control;
static TaskFunction_t captured_entry;
static TaskHandle_t captured_task;
static unsigned restart_calls, delete_calls, notify_calls;
static bool task_is_live;
static bool pause_before_notify;
static uint32_t take_script[4];
static unsigned take_script_count, take_script_index;
static unsigned deferred_wait_hook;
static jmp_buf terminal;
static int terminal_kind;
static void run_supervisor(void);

BaseType_t xTaskCreate(TaskFunction_t entry, const char *name, uint32_t stack,
                       void *arg, unsigned priority, TaskHandle_t *out)
{
    (void)name; (void)arg;
    assert(stack == 2048U && priority == 5U);
    captured_entry = entry;
    captured_task = &task_control;
    task_is_live = true;
    *out = captured_task;
    return pdPASS;
}

uint32_t ulTaskNotifyTake(BaseType_t clear, TickType_t wait)
{
    (void)clear;
    if (take_script_index < take_script_count)
        return take_script[take_script_index++];
    if (wait == portMAX_DELAY && deferred_wait_hook == 1U) {
        deferred_wait_hook = 0U;
        panel_ota_supervisor_native_operation_end();
        panel_ota_supervisor_flash_session_end();
        return 1U;
    }
    if (wait == portMAX_DELAY && deferred_wait_hook == 2U) {
        deferred_wait_hook = 0U;
        panel_ota_supervisor_boot_selection_complete(true);
        return 1U;
    }
    if (wait == portMAX_DELAY && captured_task->notifications != 0U) {
        const unsigned count = captured_task->notifications;
        captured_task->notifications = 0U;
        return count;
    }
    if (captured_task->notifications != 0U) {
        const unsigned count = captured_task->notifications;
        captured_task->notifications = 0U;
        return count;
    }
    return 0U;
}

void xTaskNotifyGive(TaskHandle_t task)
{
    assert(task == captured_task);
    assert(task_is_live);
    if (pause_before_notify) {
        pause_before_notify = false;
        /* Reproduce the worker having claimed completion and paused before
         * this notification while the supervisor reaches its deadline. */
        take_script[0] = 0U;
        take_script_count = 1U;
        take_script_index = 0U;
        run_supervisor();
        assert(terminal_kind == 3 && delete_calls == 0U && task_is_live);
    }
    ++task->notifications;
    ++notify_calls;
}

void vTaskDelete(TaskHandle_t task)
{
    assert(task == NULL);
    ++delete_calls;
    task_is_live = false;
    terminal_kind = 1;
    longjmp(terminal, 1);
}

void vTaskDelay(TickType_t ticks)
{
    (void)ticks;
    terminal_kind = 3;
    longjmp(terminal, 1);
}

void esp_restart(void)
{
    ++restart_calls;
    terminal_kind = 2;
    longjmp(terminal, 1);
}

static void reset_case(void)
{
    task_control.notifications = 0U;
    task_is_live = false;
    captured_entry = NULL;
    captured_task = NULL;
    restart_calls = delete_calls = notify_calls = 0U;
    take_script_count = take_script_index = 0U;
    deferred_wait_hook = 0U;
    terminal_kind = 0;
    pause_before_notify = false;
    assert(panel_ota_handoff_take_result() != (panel_ota_handoff_result_t)UINT32_MAX);
    assert(panel_ota_supervisor_start());
    assert(captured_entry != NULL && captured_task != NULL);
}

static void run_supervisor(void)
{
    if (setjmp(terminal) == 0) {
        captured_entry(NULL);
        assert(!"supervisor returned to the FreeRTOS task wrapper");
    }
}

int main(void)
{
    reset_case();
    assert(panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGED));
    assert(!panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED));
    assert(notify_calls == 1U);
    run_supervisor();
    assert(terminal_kind == 3 && delete_calls == 0U && restart_calls == 0U && task_is_live);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_STAGED);

    reset_case();
    run_supervisor();
    assert(terminal_kind == 2 && restart_calls == 1U && delete_calls == 0U);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT);
    assert(!panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGED));
    assert(restart_calls == 1U && notify_calls == 0U);

    /* Worker claims completion at the timeout boundary. The supervisor's
     * first wait reports timeout, then it consumes the worker notification. */
    reset_case();
    assert(panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGED));
    take_script[0] = 0U;
    take_script[1] = 1U;
    take_script_count = 2U;
    run_supervisor();
    assert(terminal_kind == 3 && delete_calls == 0U && restart_calls == 0U && task_is_live);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_STAGED);

    /* Actual production worker-complete path pauses in xTaskNotifyGive after
     * claiming DECISION_WORKER. The timed supervisor must park, not delete
     * itself, and the worker must then safely notify its still-live handle. */
    reset_case();
    pause_before_notify = true;
    assert(panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGED));
    assert(terminal_kind == 3 && task_is_live && delete_calls == 0U);
    assert(notify_calls == 1U && task_control.notifications == 1U);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_STAGED);

    /* Timeout claims first: late worker completion cannot overwrite result,
     * notify a deleted task, or request a second restart. */
    reset_case();
    run_supervisor();
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT);
    assert(!panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGED));
    assert(restart_calls == 1U && notify_calls == 0U);

    /* Timeout before any flash admission revokes the entire OTA path. */
    reset_case();
    assert(panel_ota_supervisor_watchdog_expired());
    assert(!panel_ota_supervisor_worker_authorized());
    assert(!panel_ota_supervisor_flash_session_begin());
    assert(!panel_ota_supervisor_flash_operation_begin());
    assert(panel_ota_supervisor_timeout_restart_ready());

    /* Timeout during an admitted OTA write is terminal immediately, but the
     * supervisor cannot restart until the operation and flash session leave. */
    reset_case();
    assert(panel_ota_supervisor_flash_session_begin());
    assert(panel_ota_supervisor_flash_operation_begin());
    assert(panel_ota_supervisor_watchdog_expired());
    assert(!panel_ota_supervisor_worker_authorized());
    assert(!panel_ota_supervisor_flash_operation_begin());
    assert(!panel_ota_supervisor_timeout_restart_ready());

    panel_ota_supervisor_native_operation_end();
    assert(!panel_ota_supervisor_timeout_restart_ready());
    assert(!panel_ota_supervisor_flash_cleanup_begin());
    panel_ota_supervisor_flash_session_end();
    assert(panel_ota_supervisor_timeout_restart_ready());
    assert(!panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGED));
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT);

    /* Exercise the actual supervisor task while a native flash operation is
     * active at deadline: it must wait for the owner to leave before reset. */
    reset_case();
    assert(panel_ota_supervisor_flash_session_begin());
    assert(panel_ota_supervisor_flash_operation_begin());
    take_script[0] = 0U;
    take_script_count = 1U;
    deferred_wait_hook = 1U;
    run_supervisor();
    assert(terminal_kind == 2 && restart_calls == 1U && delete_calls == 0U);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT);

    /* Exercise the production supervisor task at an admitted boot-selection
     * boundary. It waits for worker result and does not claim timeout. */
    reset_case();
    assert(panel_ota_supervisor_flash_session_begin());
    assert(panel_ota_supervisor_flash_operation_begin());
    panel_ota_supervisor_native_operation_end();
    assert(panel_ota_supervisor_boot_selection_begin());
    take_script[0] = 0U;
    take_script_count = 1U;
    deferred_wait_hook = 2U;
    run_supervisor();
    assert(terminal_kind == 3 && delete_calls == 0U && restart_calls == 0U && task_is_live);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_STAGED);

    /* Timeout between session admission and esp_ota_begin prevents begin. */
    reset_case();
    assert(panel_ota_supervisor_flash_session_begin());
    assert(panel_ota_supervisor_watchdog_expired());
    assert(!panel_ota_supervisor_flash_operation_begin());
    panel_ota_supervisor_flash_session_end();
    assert(panel_ota_supervisor_timeout_restart_ready());

    /* Boot-selection admission wins atomically. A deadline during the native
     * API is deferred until the worker publishes its actual result. */
    reset_case();
    assert(panel_ota_supervisor_flash_session_begin());
    assert(panel_ota_supervisor_flash_operation_begin());
    panel_ota_supervisor_native_operation_end();
    assert(panel_ota_supervisor_boot_selection_begin());
    assert(!panel_ota_supervisor_watchdog_expired());
    assert(!panel_ota_supervisor_timeout_restart_ready());
    panel_ota_supervisor_boot_selection_complete(true);
    assert(!panel_ota_supervisor_timeout_restart_ready());
    assert(!panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED));
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_STAGED);

    /* Failed selection after the deadline resolves to timeout, never STAGED,
     * and the supervisor restarts exactly once once the native call returns. */
    reset_case();
    assert(panel_ota_supervisor_flash_session_begin());
    assert(panel_ota_supervisor_flash_operation_begin());
    panel_ota_supervisor_native_operation_end();
    assert(panel_ota_supervisor_boot_selection_begin());
    assert(!panel_ota_supervisor_watchdog_expired());
    panel_ota_supervisor_boot_selection_complete(false);
    run_supervisor();
    assert(terminal_kind == 2 && restart_calls == 1U && delete_calls == 0U);
    assert(panel_ota_handoff_take_result() == PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT);

    puts("PATCH120B_PRODUCTION_SUPERVISOR=PASS timeout-revocation=PASS flash-defer=PASS boot-select-reservation=PASS single-owner=PASS no-return=PASS");
    return 0;
}
