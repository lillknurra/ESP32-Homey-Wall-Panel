#pragma once

/* Called only after a valid single-use software-reset handoff and NVS startup.
 * Never returns to normal startup: it restarts into the staged image on success,
 * or restarts once into the prior image after a failed attempt. */
void panel_ota_maintenance_start(void);
_Noreturn void panel_ota_maintenance_run(void);
