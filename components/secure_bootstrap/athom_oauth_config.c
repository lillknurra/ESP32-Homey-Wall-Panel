#include "athom_oauth_config.h"
#include "phone_provisioning.h"
#include <stddef.h>
#include <string.h>
bool athom_oauth_client_config_valid(const athom_oauth_client_config_t *c){if(!c||c->magic!=ATHOM_CLIENT_CONFIG_MAGIC||c->version!=ATHOM_CLIENT_CONFIG_VERSION||c->size!=sizeof(*c)||!c->client_id[0]||!c->client_secret[0]||strcmp(c->redirect_uri,ATHOM_REDIRECT_URI)!=0)return false;athom_oauth_client_config_t t=*c;uint32_t expected=t.crc32;t.crc32=0;return expected==phone_prov_crc32(&t,sizeof(t));}
static void config_zero_secure(void *p,size_t n){volatile unsigned char*q=p;while(n--)*q++=0U;}
bool athom_oauth_client_config_prepare(athom_oauth_client_config_t *c,const char *id,const char *secret,const char *uri){if(!c||!id||!secret||!uri)return false;size_t a=strlen(id),b=strlen(secret),d=strlen(uri);if(a==0||a>=ATHOM_CLIENT_ID_MAX||b==0||b>=ATHOM_CLIENT_SECRET_MAX||d==0||d>=ATHOM_REDIRECT_URI_MAX||strcmp(uri,ATHOM_REDIRECT_URI)!=0)return false;memset(c,0,sizeof(*c));c->magic=ATHOM_CLIENT_CONFIG_MAGIC;c->version=ATHOM_CLIENT_CONFIG_VERSION;c->size=sizeof(*c);memcpy(c->client_id,id,a+1);memcpy(c->client_secret,secret,b+1);memcpy(c->redirect_uri,uri,d+1);c->crc32=phone_prov_crc32(c,sizeof(*c));return athom_oauth_client_config_valid(c);}

#ifdef ESP_PLATFORM
#include "nvs.h"
#define ATHOM_CFG_NS "athom_cli_cfg"
#define ATHOM_CFG_KEY "client_v1"
esp_err_t athom_oauth_client_config_load(athom_oauth_client_config_t *c,bool *present){if(!c||!present)return ESP_ERR_INVALID_ARG;*present=false;memset(c,0,sizeof(*c));nvs_handle_t h=0;esp_err_t e=nvs_open(ATHOM_CFG_NS,NVS_READONLY,&h);if(e==ESP_ERR_NVS_NOT_FOUND)return ESP_OK;if(e!=ESP_OK)return e;size_t n=sizeof(*c);e=nvs_get_blob(h,ATHOM_CFG_KEY,c,&n);nvs_close(h);if(e==ESP_ERR_NVS_NOT_FOUND)return ESP_OK;if(e!=ESP_OK)return e;if(n!=sizeof(*c)||!athom_oauth_client_config_valid(c)){memset(c,0,sizeof(*c));return ESP_ERR_INVALID_CRC;}*present=true;return ESP_OK;}
esp_err_t athom_oauth_client_config_save(const athom_oauth_client_config_t *c){if(!athom_oauth_client_config_valid(c))return ESP_ERR_INVALID_ARG;athom_oauth_client_config_t old;bool present=false;esp_err_t e=athom_oauth_client_config_load(&old,&present);config_zero_secure(&old,sizeof(old));if(e!=ESP_OK)return e;if(present)return ESP_ERR_INVALID_STATE;nvs_handle_t h=0;e=nvs_open(ATHOM_CFG_NS,NVS_READWRITE,&h);if(e!=ESP_OK)return e;e=nvs_set_blob(h,ATHOM_CFG_KEY,c,sizeof(*c));if(e==ESP_OK)e=nvs_commit(h);nvs_close(h);if(e!=ESP_OK)return e;athom_oauth_client_config_t v;bool p=false;e=athom_oauth_client_config_load(&v,&p);bool ok=e==ESP_OK&&p&&memcmp(&v,c,sizeof(v))==0;config_zero_secure(&v,sizeof(v));return ok?ESP_OK:ESP_FAIL;}

#endif
