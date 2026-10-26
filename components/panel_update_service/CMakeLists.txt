# Patch116I1: read-only release-manifest and HTTPS fetch components.
idf_component_register(
    SRCS "panel_update_manifest.c" "panel_update_manifest_json.c"
         "panel_update_check.c" "panel_update_https_fetch.c"
         "panel_update_lvgl_psram_pool.c" "panel_update_lvgl_psram_pool_idf.c"
    INCLUDE_DIRS "include"
    REQUIRES espressif__cjson
    PRIV_REQUIRES esp_http_client esp_timer mbedtls freertos heap log esp_system esp_psram esp_hw_support
)
