#pragma once

#include <stdbool.h>
#include "panel_ota_handoff.h"

/* Owns the maintenance completion-vs-timeout decision. */
bool panel_ota_supervisor_start(void);
bool panel_ota_supervisor_worker_authorized(void);
bool panel_ota_supervisor_flash_session_begin(void);
bool panel_ota_supervisor_flash_operation_begin(void);
bool panel_ota_supervisor_flash_cleanup_begin(void);
bool panel_ota_supervisor_boot_selection_begin(void);
void panel_ota_supervisor_native_operation_end(void);
void panel_ota_supervisor_flash_session_end(void);
/* Resolves the admitted set-boot call. Success records STAGED and transfers
 * the immediate restart to the maintenance worker; failure releases the
 * reservation and resolves a deadline that arrived during the call. */
void panel_ota_supervisor_boot_selection_complete(bool selected);
bool panel_ota_supervisor_watchdog_expired(void);
bool panel_ota_supervisor_timeout_restart_ready(void);
/* Returns true only when the worker won and owns the planned restart. */
bool panel_ota_supervisor_worker_complete(panel_ota_handoff_result_t result);
