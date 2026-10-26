#include "panel_update_lvgl_psram_pool.h"

#if PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_psram.h"
#include <stdlib.h>

static const char *TAG = "lvgl_psram_pool";

bool panel_update_i5_psram_ready(void)
{
    return esp_psram_is_initialized();
}

void *panel_update_i5_aligned_alloc(size_t alignment, size_t bytes, uint32_t capabilities)
{
    const uint32_t required = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    if (capabilities != required) return NULL;
    return heap_caps_aligned_alloc(alignment, bytes, required);
}

void panel_update_i5_free(void *ptr)
{
    heap_caps_free(ptr);
}

bool panel_update_i5_is_external_ram(const void *ptr)
{
    return esp_ptr_external_ram(ptr);
}

_Noreturn void panel_update_i5_fatal(const char *reason)
{
    ESP_LOGE(TAG, "audit pool initialization failed: %s", reason);
    abort();
}

#endif
