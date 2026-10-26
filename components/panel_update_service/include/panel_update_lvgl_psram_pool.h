#pragma once

#include <stddef.h>

#ifndef PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED
#define PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED 0
#endif

#ifndef PANEL_UPDATE_I5_AUDIT_ONLY
#define PANEL_UPDATE_I5_AUDIT_ONLY 0
#endif

#if (PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED != 0 && PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED != 1) || \
    (PANEL_UPDATE_I5_AUDIT_ONLY != 0 && PANEL_UPDATE_I5_AUDIT_ONLY != 1)
#error "Patch116I5 feature gates must be 0 or 1"
#endif

#if PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED && !PANEL_UPDATE_I5_AUDIT_ONLY
#error "Patch116I5 PSRAM pool is permitted only in an audit-only build"
#endif

#define PANEL_UPDATE_I5_LVGL_POOL_BYTES 65536U
#define PANEL_UPDATE_I5_LVGL_POOL_ALIGNMENT 16U

#if PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED
/* Called by LVGL's built-in allocator before TLSF is constructed. This function
 * never returns NULL: any unmet precondition terminates before TLSF sees it. */
void *panel_update_lvgl_psram_pool_alloc(size_t requested_bytes);
#endif
