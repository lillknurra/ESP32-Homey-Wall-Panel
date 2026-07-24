idf_component_register(
  SRCS
    "src/athom_auth.c"
    "src/athom_credential_store_nvs.c"
    "src/athom_homey_client.c"
    "src/athom_provisioning.c"
    "src/athom_redaction.c"
  INCLUDE_DIRS "include"
  REQUIRES nvs_flash esp_http_client esp_timer
)
