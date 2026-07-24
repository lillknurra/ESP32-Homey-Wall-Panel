idf_component_register(
  SRCS "secure_bootstrap_logic.c" "secure_bootstrap_esp.c"
  INCLUDE_DIRS "include"
  REQUIRES esp_wifi esp_event esp_netif esp_http_server nvs_flash driver esp_timer waveshare__esp32_s3_touch_lcd_4b
)
