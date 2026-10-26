option(PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED "Patch116I5 audit-only LVGL pool in PSRAM" OFF)
option(PANEL_UPDATE_I5_AUDIT_ONLY "Authorize Patch116I5 audit-only compilation" OFF)

if(PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED AND NOT PANEL_UPDATE_I5_AUDIT_ONLY)
    message(FATAL_ERROR "Patch116I5 pool relocation requires PANEL_UPDATE_I5_AUDIT_ONLY=ON")
endif()

if(PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED)
    idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
    idf_component_get_property(update_lib panel_update_service COMPONENT_LIB)
    idf_component_get_property(lvgl_dir lvgl__lvgl COMPONENT_DIR)
    if(NOT lvgl_lib OR NOT update_lib OR NOT lvgl_dir)
        message(FATAL_ERROR "Patch116I5 could not resolve the LVGL and project allocator components")
    endif()
    set(lvgl_allocator_source "${lvgl_dir}/src/stdlib/builtin/lv_mem_core_builtin.c")
    if(NOT EXISTS "${lvgl_allocator_source}")
        message(FATAL_ERROR "Patch116I5 expected the pinned LVGL built-in allocator source")
    endif()

    target_include_directories(${lvgl_lib} PRIVATE
        "${CMAKE_SOURCE_DIR}/components/panel_update_service/include")
    target_link_libraries(${lvgl_lib} PRIVATE ${update_lib})
    target_compile_definitions(${lvgl_lib} PRIVATE
        "PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED=1"
        "PANEL_UPDATE_I5_AUDIT_ONLY=1"
        "LV_MEM_POOL_INCLUDE=\"panel_update_lvgl_psram_pool.h\""
        "LV_MEM_POOL_ALLOC=panel_update_lvgl_psram_pool_alloc")
    target_compile_definitions(${update_lib} PRIVATE
        PANEL_UPDATE_I5_LVGL_PSRAM_POOL_ENABLED=1
        PANEL_UPDATE_I5_AUDIT_ONLY=1)
    message(STATUS "Patch116I5 audit-only LVGL 65536-byte TLSF pool uses explicit PSRAM allocation")
endif()
