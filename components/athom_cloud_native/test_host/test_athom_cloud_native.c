#include "athom_auth.h"
#include "athom_homey_client.h"
#include "athom_provisioning.h"
#include "athom_redaction.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { athom_credentials_t value; bool present; bool fail_save; int save_count; int wipe_count; } memory_store_t;
static athom_status_t mem_load(athom_credential_store_t *store, athom_credentials_t *out) { memory_store_t *m=store->context; if(!m->present)return ATHOM_ERR_STORAGE; *out=m->value; return ATHOM_OK; }
static athom_status_t mem_save(athom_credential_store_t *store,const athom_credentials_t *value){memory_store_t*m=store->context;if(m->fail_save)return ATHOM_ERR_STORAGE;m->value=*value;m->present=true;m->save_count++;return ATHOM_OK;}
static athom_status_t mem_wipe(athom_credential_store_t *store){memory_store_t*m=store->context;athom_secure_zero(&m->value,sizeof(m->value));m->present=false;m->wipe_count++;return ATHOM_OK;}
static const athom_credential_store_vtable_t STORE_VTABLE={.load=mem_load,.save=mem_save,.wipe=mem_wipe};

typedef struct {int exchange_count,refresh_count,list_count,inventory_count,invalidate_count;bool fail_refresh;char observed_code[64];} mock_transport_t;
static athom_status_t mock_exchange(athom_http_transport_t*t,const athom_credentials_t*c,const char*code,athom_credentials_t*u){mock_transport_t*m=t->context;assert(strcmp(c->client_id,"client-id")==0);snprintf(m->observed_code,sizeof(m->observed_code),"%s",code);snprintf(u->access_token,sizeof(u->access_token),"%s","access-secret");snprintf(u->refresh_token,sizeof(u->refresh_token),"%s","refresh-secret");u->expires_at_epoch_s=2000;m->exchange_count++;return ATHOM_OK;}
static athom_status_t mock_refresh(athom_http_transport_t*t,const athom_credentials_t*c,athom_credentials_t*u){mock_transport_t*m=t->context;assert(c->refresh_token[0]);m->refresh_count++;if(m->fail_refresh)return ATHOM_ERR_TRANSPORT;snprintf(u->access_token,sizeof(u->access_token),"%s","new-access-secret");u->expires_at_epoch_s=4000;return ATHOM_OK;}
static athom_status_t mock_list(athom_http_transport_t*t,const char*a,athom_homey_list_t*out){mock_transport_t*m=t->context;assert(a[0]);memset(out,0,sizeof(*out));out->count=2;snprintf(out->items[0].id,sizeof(out->items[0].id),"%s","homey-a");snprintf(out->items[1].id,sizeof(out->items[1].id),"%s","homey-b");m->list_count++;return ATHOM_OK;}
static athom_status_t mock_inventory(athom_http_transport_t*t,const char*a,const char*h,athom_discovery_strategy_t s,athom_inventory_summary_t*out){mock_transport_t*m=t->context;assert(a[0]);assert(strcmp(h,"homey-b")==0);assert(s==ATHOM_DISCOVERY_CLOUD);*out=(athom_inventory_summary_t){1,2,3,4,5};m->inventory_count++;return ATHOM_OK;}
static void mock_invalidate(athom_http_transport_t*t){mock_transport_t*m=t->context;m->invalidate_count++;}
static const athom_http_transport_vtable_t HTTP_VTABLE={.exchange_authorization_code=mock_exchange,.refresh_access_token=mock_refresh,.list_homeys=mock_list,.read_inventory=mock_inventory,.invalidate_session=mock_invalidate};

int main(void){
 memory_store_t memory={0};athom_credential_store_t store={.vtable=&STORE_VTABLE,.context=&memory};mock_transport_t mock={0};athom_http_transport_t transport={.vtable=&HTTP_VTABLE,.context=&mock};athom_auth_context_t auth={.store=&store,.transport=&transport};athom_provisioning_t portal={.auth=&auth,.state=ATHOM_PROVISIONING_IDLE};
 char state[ATHOM_MAX_OAUTH_STATE]={0};assert(athom_provisioning_start(&portal,"client-id","client-secret","https://callback.invalid",1000,state,sizeof(state))==ATHOM_OK);portal.state=ATHOM_PROVISIONING_WAITING_FOR_CODE;assert(athom_provisioning_submit_code(&portal,state,"authorization-code",1001)==ATHOM_OK);
 athom_homey_list_t homeys={0};assert(athom_homey_list(&auth,1100,&homeys)==ATHOM_OK);assert(athom_provisioning_select_homey(&portal,"missing",&homeys,ATHOM_DISCOVERY_CLOUD)==ATHOM_ERR_HOMEY_SELECTION_STALE);
 portal.state=ATHOM_PROVISIONING_AUTHORIZED;assert(athom_provisioning_select_homey(&portal,"homey-b",&homeys,ATHOM_DISCOVERY_CLOUD)==ATHOM_OK);assert(mock.invalidate_count==1);
 portal.state=ATHOM_PROVISIONING_AUTHORIZED;assert(athom_provisioning_select_homey(&portal,"homey-b",&homeys,ATHOM_DISCOVERY_CLOUD)==ATHOM_OK);assert(mock.invalidate_count==1);
 athom_credentials_t before=memory.value;memory.fail_save=true;portal.state=ATHOM_PROVISIONING_AUTHORIZED;assert(athom_provisioning_select_homey(&portal,"homey-a",&homeys,ATHOM_DISCOVERY_CLOUD)==ATHOM_ERR_STORAGE);assert(memcmp(&before,&memory.value,sizeof(before))==0);assert(mock.invalidate_count==1);memory.fail_save=false;
 athom_inventory_summary_t summary={0};assert(athom_homey_read_inventory(&auth,1200,&summary)==ATHOM_OK);assert(summary.devices==2&&mock.inventory_count==1);
 memory.value.selected_homey_id[0]='\0';assert(athom_homey_read_inventory(&auth,1200,&summary)==ATHOM_ERR_HOMEY_SELECTION_REQUIRED);snprintf(memory.value.selected_homey_id,sizeof(memory.value.selected_homey_id),"%s","homey-b");
 assert(!athom_mutation_allowed("setCapabilityValue"));assert(athom_provisioning_wipe(&portal)==ATHOM_OK);puts("PASS: ESP32-native Athom Cloud host tests");return 0;
}
