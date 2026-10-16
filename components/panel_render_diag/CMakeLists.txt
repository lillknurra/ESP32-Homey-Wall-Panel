idf_component_register(SRCS "panel_render_diag.c" "panel_lvgl_alloc_diag.c" INCLUDE_DIRS "include"
                       REQUIRES esp_timer freertos esp_system heap)
target_link_options(${COMPONENT_LIB} INTERFACE
    "-Wl,--wrap=lv_malloc_core" "-Wl,--wrap=lv_realloc_core")
