#include "athom_http_esp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int inventory_calls; int delegation_calls; int login_calls; int force_401_count; } mock_t;
static void response(athom_http_response_t*r,int code,const char*body){r->status_code=code;r->tls_verified=true;r->length=strlen(body);r->capacity=r->length+1;r->data=malloc(r->capacity);assert(r->data);memcpy(r->data,body,r->capacity);}
static athom_status_t mock_execute(void*opaque,const char*method,const char*url,const char*authorization,const char*content_type,const char*body,athom_http_response_t*out){
 mock_t*m=opaque;assert(strncmp(url,"https://",8)==0);
 if(strstr(url,"/oauth2/token")){assert(strcmp(method,"POST")==0);assert(authorization&&strncmp(authorization,"Basic ",6)==0);response(out,200,"{\"token_type\":\"bearer\",\"access_token\":\"a\",\"refresh_token\":\"r2\",\"expires_in\":3660}");}
 else if(strstr(url,"/user/me")){response(out,200,"{\"homeys\":[{\"_id\":\"h1\",\"name\":\"Home\",\"remoteUrl\":\"https://h1.homeypro.net\"}]}");}
 else if(strstr(url,"/delegation/token")){m->delegation_calls++;response(out,200,"\"delegation\"");}
 else if(strstr(url,"/api/manager/users/login")){m->login_calls++;assert(content_type&&strcmp(content_type,"application/json")==0);assert(body&&strcmp(body,"{\"token\":\"delegation\"}")==0);response(out,200,"\"session\"");}
 else {assert(authorization&&strcmp(authorization,"Bearer session")==0);m->inventory_calls++;if(m->force_401_count>0){m->force_401_count--;response(out,401,"{}");}else response(out,200,"{}");}
 return ATHOM_OK;
}
static void prepare(athom_http_transport_t*t,mock_t*m,athom_credentials_t*u){athom_credentials_t c={0};snprintf(c.client_id,sizeof(c.client_id),"client");snprintf(c.client_secret,sizeof(c.client_secret),"secret");assert(athom_http_esp_init(t)==ATHOM_OK);assert(athom_http_esp_set_executor(t,mock_execute,m)==ATHOM_OK);assert(t->vtable->exchange_authorization_code(t,&c,"code",u)==ATHOM_OK);athom_homey_list_t list;assert(t->vtable->list_homeys(t,u->access_token,&list)==ATHOM_OK);assert(list.count==1);}
int main(void){
 athom_http_transport_t t;mock_t m={0};athom_credentials_t u={0};prepare(&t,&m,&u);m.force_401_count=1;athom_inventory_summary_t inv;assert(t.vtable->read_inventory(&t,u.access_token,"h1",ATHOM_DISCOVERY_CLOUD,&inv)==ATHOM_OK);assert(m.delegation_calls==2&&m.login_calls==2);
 mock_t m2={0};athom_credentials_t u2={0};prepare(&t,&m2,&u2);m2.force_401_count=2;assert(t.vtable->read_inventory(&t,u2.access_token,"h1",ATHOM_DISCOVERY_CLOUD,&inv)==ATHOM_ERR_HTTP_STATUS);assert(m2.delegation_calls==2&&m2.login_calls==2);
 assert(!athom_http_url_is_official("https://evil.example/?x=.homeypro.net/"));assert(!athom_http_url_is_official("https://user@synthetic.homeypro.net/"));assert(!athom_http_url_is_official("https://synthetic.homeypro.net:443/"));puts("PASS: executable native Athom transport mock chain");return 0;
}
