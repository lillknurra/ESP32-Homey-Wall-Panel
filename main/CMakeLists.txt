idf_component_register(
    SRCS "main.c" "nvs_startup_gate.c" "panel_ota_first_boot_gate.c" "panel_ota_https_stage.c"
    INCLUDE_DIRS "."
    REQUIRES secure_bootstrap nvs_flash app_update esp_app_format esp_partition esp_psram heap esp_http_client bootloader_support mbedtls esp_timer
)
