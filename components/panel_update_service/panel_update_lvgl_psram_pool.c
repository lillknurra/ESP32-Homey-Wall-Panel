#include "panel_update_lvgl_psram_pool.h"

#if PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED

#include "panel_update_lvgl_psram_pool_port.h"

#define PANEL_UPDATE_I5_CAP_SPIRAM (1U << 10)
#define PANEL_UPDATE_I5_CAP_8BIT   (1U << 2)

_Static_assert(PANEL_UPDATE_I5_LVGL_POOL_BYTES == 65536U, "LVGL pool size changed");
_Static_assert(PANEL_UPDATE_I5_LVGL_POOL_ALIGNMENT >= 8U, "TLSF alignment is insufficient");

static void *s_pool;

void *panel_update_lvgl_psram_pool_alloc(size_t requested_bytes)
{
    if (requested_bytes != PANEL_UPDATE_I5_LVGL_POOL_BYTES) {
        panel_update_i5_fatal("unexpected LVGL pool size");
    }

    /* LVGL's lv_mem_deinit() destroys TLSF metadata but does not own/free the
     * backing store. Reuse the same stable allocation on any later init. */
    if (s_pool != NULL) {
        if (((uintptr_t)s_pool % PANEL_UPDATE_I5_LVGL_POOL_ALIGNMENT) != 0U ||
            !panel_update_i5_is_external_ram(s_pool)) {
            panel_update_i5_fatal("cached LVGL pool validation failed");
        }
        return s_pool;
    }

    if (!panel_update_i5_psram_ready()) {
        panel_update_i5_fatal("PSRAM is not initialized");
    }

    void *candidate = panel_update_i5_aligned_alloc(
        PANEL_UPDATE_I5_LVGL_POOL_ALIGNMENT,
        PANEL_UPDATE_I5_LVGL_POOL_BYTES,
        PANEL_UPDATE_I5_CAP_SPIRAM | PANEL_UPDATE_I5_CAP_8BIT);
    if (candidate == NULL) {
        panel_update_i5_fatal("LVGL PSRAM pool allocation failed");
    }
    if (((uintptr_t)candidate % PANEL_UPDATE_I5_LVGL_POOL_ALIGNMENT) != 0U) {
        panel_update_i5_free(candidate);
        panel_update_i5_fatal("LVGL PSRAM pool alignment invalid");
    }
    if (!panel_update_i5_is_external_ram(candidate)) {
        panel_update_i5_free(candidate);
        panel_update_i5_fatal("LVGL pool was not allocated in PSRAM");
    }

    s_pool = candidate;
    return s_pool;
}

#endif
