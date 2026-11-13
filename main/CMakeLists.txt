idf_component_register(
    SRCS "main.c" "nvs_startup_gate.c" "panel_ota_first_boot_gate.c"
         "panel_ota_https_stage.c" "panel_ota_maintenance.c" "panel_ota_supervisor.c"
    INCLUDE_DIRS "."
    REQUIRES secure_bootstrap nvs_flash app_update esp_app_format esp_partition esp_psram heap esp_http_client bootloader_support mbedtls esp_timer esp_event esp_netif esp_wifi
)

if(CONFIG_PANEL_OTA_ENABLED)
 target_compile_definitions(${COMPONENT_LIB} PRIVATE
  PANEL_OTA_HTTPS_STAGE_ENABLED=1
  PANEL_OTA_FIRST_BOOT_ENABLED=1
  PANEL_OTA_MAX_IMAGE_BYTES=0x400000
  PANEL_OTA_MAX_TRANSFER_MS=600000
  PANEL_OTA_MIN_INTERNAL_BLOCK_BYTES=${CONFIG_PANEL_OTA_MIN_INTERNAL_BLOCK_BYTES}
  PANEL_OTA_MIN_PSRAM_BLOCK_BYTES=${CONFIG_PANEL_OTA_MIN_PSRAM_BLOCK_BYTES})
endif()
