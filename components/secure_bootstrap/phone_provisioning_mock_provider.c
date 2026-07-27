#include "phone_provisioning.h"
#include <string.h>
static void wipe(void *p,size_t n){volatile unsigned char *q=p;while(n--)*q++=0;}
static phone_prov_result_t complete(phone_prov_oauth_result_t *o){ if(!o)return PHONE_PROV_ERR_ARGUMENT; memset(o,0,sizeof(*o)); strcpy(o->access_token,"synthetic-access-token-not-for-live-use"); strcpy(o->refresh_token,"synthetic-refresh-token-not-for-live-use"); o->expires_at=4102444800LL; o->homey_count=2; strcpy(o->homeys[0].id,"synthetic-homey-primary"); strcpy(o->homeys[0].name,"Hemmet"); strcpy(o->homeys[1].id,"synthetic-homey-secondary"); strcpy(o->homeys[1].name,"Sommarhuset"); return PHONE_PROV_OK; }
static void wipe_result(phone_prov_oauth_result_t *o){if(o)wipe(o,sizeof(*o));}
const phone_prov_provider_t *phone_prov_mock_provider(void){static const phone_prov_provider_t p={complete,wipe_result};return &p;}
