idf_component_register(
 SRCS "secure_bootstrap_logic.c" "secure_bootstrap_esp.c" "phone_provisioning_logic.c" "phone_provisioning_store.c" "phone_provisioning_mock_provider.c" "athom_oauth_flow.c" "athom_restore_policy.c" "athom_oauth_config.c" "athom_oauth_runtime.c" "runtime_diag_journal.c"
 "athom_cloud_client.c" "athom_pre_tls_diag.c"
 "athom_auth_store.c" "panel_homey_alias_store.c" "panel_homey_light_provisioning.c" "panel_homey_awning_provisioning.c" "panel_homey_favorites.c"
 "athom_cloud_model.c" "panel_homey_read_snapshot.c" "panel_homey_dashboard_binding.c" "panel_ui_model.c" "panel_ui_store.c" "panel_ui.c" "homey_panel_font_16.c" "homey_panel_font_18.c" "homey_panel_font_22.c"
 INCLUDE_DIRS "include"
 REQUIRES esp_wifi esp_event esp_netif esp_http_server nvs_flash driver esp_timer esp_partition esp_system freertos mdns waveshare__esp32_s3_touch_lcd_4b esp_http_client esp-tls espressif__cjson

  PRIV_REQUIRES mbedtls)

# Observe existing calls in the owning cloud perform only; no vendor source changes.
foreach(symbol esp_tls_conn_new_sync lwip_getaddrinfo lwip_socket lwip_connect select lwip_getsockopt mbedtls_ssl_setup)
 target_link_options(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=${symbol}")
endforeach()
