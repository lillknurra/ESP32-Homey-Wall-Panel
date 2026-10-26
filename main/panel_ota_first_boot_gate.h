#pragma once

#include <stdbool.h>

/* Patch116D: gated first-boot OTA confirmation. Call only AFTER NVS was
 * initialized and BEFORE bsp_display_start()/LVGL/RGB or Homey startup.
 * Default build remains a no-op; actual OTA activation is a separate release.
 */
bool panel_ota_first_boot_gate_after_nvs(void);
