# Read-only update discovery; worker and network default-disabled, no runtime call sites.
idf_component_register(
    SRCS "panel_update_manifest.c" "panel_update_manifest_json.c"
         "panel_update_check.c" "panel_update_https_fetch.c"
         "panel_update_worker_policy.c" "panel_update_worker_freertos.c"
         "panel_update_i3_probe_policy.c" "panel_update_i3_probe_esp.c"
         "panel_update_i3a_web_diag.c"
         "panel_update_lvgl_psram_pool.c"
         "panel_update_lvgl_psram_pool_idf.c"
         "panel_update_i10_dns_bridge.c"
    INCLUDE_DIRS "include"
    REQUIRES espressif__cjson
    PRIV_REQUIRES esp_http_client esp_timer mbedtls freertos heap log esp_system esp_psram esp_hw_support lwip
)
