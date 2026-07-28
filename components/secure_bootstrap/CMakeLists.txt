idf_component_register(
 SRCS "secure_bootstrap_logic.c" "secure_bootstrap_esp.c" "phone_provisioning_logic.c" "phone_provisioning_store.c" "phone_provisioning_mock_provider.c" "athom_oauth_flow.c" "athom_oauth_config.c" "athom_oauth_runtime.c"
 "athom_cloud_client.c"
 "athom_auth_store.c"
 "athom_cloud_model.c" "homey_panel_font_22.c"
 INCLUDE_DIRS "include"
 REQUIRES esp_wifi esp_event esp_netif esp_http_server nvs_flash driver esp_timer mdns waveshare__esp32_s3_touch_lcd_4b esp_http_client esp-tls espressif__cjson
)
