#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef ESP_PLATFORM
#include "esp_err.h"
#endif
#define ATHOM_CLIENT_ID_MAX 128U
#define ATHOM_CLIENT_SECRET_MAX 192U
#define ATHOM_REDIRECT_URI_MAX 128U
#define ATHOM_CLIENT_CONFIG_MAGIC 0x414f4346U
#define ATHOM_CLIENT_CONFIG_VERSION 1U
#define ATHOM_REDIRECT_URI "http://homey-panel.local/oauth/callback"
typedef struct {uint32_t magic;uint16_t version;uint16_t size;uint32_t crc32;char client_id[ATHOM_CLIENT_ID_MAX];char client_secret[ATHOM_CLIENT_SECRET_MAX];char redirect_uri[ATHOM_REDIRECT_URI_MAX];} athom_oauth_client_config_t;
bool athom_oauth_client_config_valid(const athom_oauth_client_config_t *config);
bool athom_oauth_client_config_prepare(athom_oauth_client_config_t *config,const char *client_id,const char *client_secret,const char *redirect_uri);
#ifdef ESP_PLATFORM
esp_err_t athom_oauth_client_config_load(athom_oauth_client_config_t *config,bool *present);
esp_err_t athom_oauth_client_config_save(const athom_oauth_client_config_t *config);
#endif
