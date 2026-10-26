# Patch116I5: project-owned audit-only LVGL PSRAM pool adapter.
idf_component_register(
    SRCS "panel_update_lvgl_psram_pool.c" "panel_update_lvgl_psram_pool_idf.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES heap esp_psram esp_hw_support log
)
