#include "phone_provisioning.h"
#include "esp_check.h"
#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define NS "hpanel_auth"
#define STAGING "staging_v1"
#define ACTIVE "active_v1"
static const char *TAG="phone_prov";
static phone_prov_context_t s_ctx;
static phone_prov_record_v1_t s_active;
static phone_prov_display_cb_t s_display;
static bool s_wifi_online;
static bool s_display_ready_rendered;
static bool s_display_homey_name_present;
static const char *s_portal_css="";
static bool s_wrong_state_pass;
static bool s_replay_pass;
static bool s_persistence_restored;
static bool s_wipe_complete;
static bool s_wifi_preserved=true;
static bool s_change_flow;
static uint32_t s_generation=1;
static int64_t now_s(void){return esp_timer_get_time()/1000000LL;}
static void zero_secure(void *p,size_t n){volatile unsigned char*q=p;while(n--)*q++=0;}
void phone_provisioning_set_display_callback(phone_prov_display_cb_t cb){s_display=cb;}
void phone_provisioning_set_portal_css(const char *css){s_portal_css=css?css:"";}
static void show(const char*t,const char*d){if(s_display)s_display(t,d,NULL);}
static void render_homey_display(void){
    s_display_ready_rendered=false;
    s_display_homey_name_present=false;
    if(!s_wifi_online)return;
    if(s_ctx.state==PHONE_PROV_READY&&s_active.homey_name[0]!='\0'){
        show(s_active.homey_name,"Status: READY");
        s_display_ready_rendered=true;
        s_display_homey_name_present=true;
        ESP_LOGI(TAG,"PHONE_PROV display_restore result=pass");
        ESP_LOGI(TAG,"PHONE_PROV display_ready rendered=true homey_name_present=true");
    }else{
        show("Homey-installation krävs","Öppna telefonportalen");
        ESP_LOGI(TAG,"PHONE_PROV display_ready rendered=false homey_name_present=false");
    }
}
void phone_provisioning_on_wifi_online(void){s_wifi_online=true;render_homey_display();}
bool phone_provisioning_display_ready_rendered(void){return s_display_ready_rendered;}
static esp_err_t erase_key(nvs_handle_t h,const char*k){esp_err_t e=nvs_erase_key(h,k);return e==ESP_ERR_NVS_NOT_FOUND?ESP_OK:e;}
static esp_err_t read_record(const char *key,phone_prov_record_v1_t *r,bool *present){*present=false;nvs_handle_t h;esp_err_t e=nvs_open(NS,NVS_READONLY,&h);if(e==ESP_ERR_NVS_NOT_FOUND)return ESP_OK;if(e!=ESP_OK)return e;size_t n=sizeof(*r);e=nvs_get_blob(h,key,r,&n);nvs_close(h);if(e==ESP_ERR_NVS_NOT_FOUND)return ESP_OK;if(e!=ESP_OK||n!=sizeof(*r)||!phone_prov_record_valid(r)){zero_secure(r,sizeof(*r));return ESP_ERR_INVALID_CRC;}*present=true;return ESP_OK;}
static esp_err_t write_record(const char *key,const phone_prov_record_v1_t *r){nvs_handle_t h;esp_err_t e=nvs_open(NS,NVS_READWRITE,&h);if(e==ESP_OK)e=nvs_set_blob(h,key,r,sizeof(*r));if(e==ESP_OK)e=nvs_commit(h);if(h)nvs_close(h);return e;}
static esp_err_t publish(phone_prov_record_v1_t *record){
 esp_err_t err=write_record(STAGING,record);if(err!=ESP_OK)return err;
 phone_prov_record_v1_t staged={0};bool staged_present=false;err=read_record(STAGING,&staged,&staged_present);
 if(err!=ESP_OK||!phone_prov_publication_readback_valid(record,&staged,staged_present)){zero_secure(&staged,sizeof(staged));return err!=ESP_OK?err:ESP_ERR_INVALID_CRC;}
 ESP_LOGI(TAG,"PHONE_PROV persist_stage result=pass");err=write_record(ACTIVE,&staged);if(err!=ESP_OK){zero_secure(&staged,sizeof(staged));return err;}
 phone_prov_record_v1_t active={0};bool active_present=false;err=read_record(ACTIVE,&active,&active_present);
 if(err!=ESP_OK||!phone_prov_publication_readback_valid(&staged,&active,active_present)){zero_secure(&active,sizeof(active));zero_secure(&staged,sizeof(staged));return err!=ESP_OK?err:ESP_ERR_INVALID_CRC;}
 nvs_handle_t h=0;err=nvs_open(NS,NVS_READWRITE,&h);if(err==ESP_OK)err=erase_key(h,STAGING);if(err==ESP_OK)err=nvs_commit(h);if(h)nvs_close(h);
 if(err!=ESP_OK){zero_secure(&active,sizeof(active));zero_secure(&staged,sizeof(staged));return err;}
 s_active=active;s_ctx.state=PHONE_PROV_READY;s_persistence_restored=true;ESP_LOGI(TAG,"PHONE_PROV persist_active result=pass");render_homey_display();zero_secure(&active,sizeof(active));zero_secure(&staged,sizeof(staged));return ESP_OK;
}
esp_err_t phone_provisioning_boot_restore(void){phone_prov_init(&s_ctx);phone_prov_record_v1_t r={0};bool p=false;esp_err_t e=read_record(ACTIVE,&r,&p);if(e==ESP_OK&&p){s_active=r;s_generation=r.generation+1;s_ctx.state=PHONE_PROV_READY;s_persistence_restored=true;ESP_LOGI(TAG,"PHONE_PROV boot_restore result=pass");render_homey_display();return ESP_OK;}if(e==ESP_OK){s_ctx.state=PHONE_PROV_UNPROVISIONED;s_persistence_restored=false;render_homey_display();return ESP_OK;}s_ctx.state=PHONE_PROV_REPROVISION_REQUIRED;show("Installation misslyckades","Radera och konfigurera igen");return e;}
static esp_err_t form(httpd_req_t *r,char *b,size_t n){int len=r->content_len;if(len<=0||(size_t)len>=n)return ESP_ERR_INVALID_SIZE;int got=httpd_req_recv(r,b,len);if(got!=len)return ESP_FAIL;b[len]=0;return ESP_OK;}
static const char *state_name(phone_prov_state_t state){switch(state){case PHONE_PROV_UNPROVISIONED:return "Inte konfigurerad";case PHONE_PROV_PORTAL_ACTIVE:return "Portal aktiv";case PHONE_PROV_AUTH_PENDING:return "Väntar på syntetisk inloggning";case PHONE_PROV_HOMEY_SELECTION_REQUIRED:return "Välj Homey";case PHONE_PROV_READY:return "Klar";case PHONE_PROV_REPROVISION_REQUIRED:return "Ny installation krävs";default:return "Okänd status";}}
static esp_err_t page_begin(httpd_req_t*r,const char*title){httpd_resp_set_type(r,"text/html; charset=utf-8");esp_err_t e=httpd_resp_sendstr_chunk(r,"<!doctype html><html lang=sv><head><meta charset=UTF-8><meta name=viewport content='width=device-width,initial-scale=1'><title>");if(e!=ESP_OK)return e;e=httpd_resp_sendstr_chunk(r,title);if(e!=ESP_OK)return e;e=httpd_resp_sendstr_chunk(r,"</title><style>");if(e!=ESP_OK)return e;e=httpd_resp_sendstr_chunk(r,s_portal_css);if(e!=ESP_OK)return e;return httpd_resp_sendstr_chunk(r,".steps{list-style:none;padding:0}.steps li{padding:.75rem;margin:.45rem 0;border-radius:.6rem;background:#eef2f5}.wait{color:#5b6670}.pass{color:#146c43;font-weight:700}.fail{color:#b02a37;font-weight:700}</style></head><body><main>");}
static esp_err_t test_mode(httpd_req_t*r){return httpd_resp_sendstr_chunk(r,"<div class=testmode><strong>TESTLÄGE – SYNTHETISK HOMEY</strong><br>Ingen kontakt med Athom.<br>Inga riktiga inloggningsuppgifter används.</div>");}
static esp_err_t page_end(httpd_req_t*r){esp_err_t e=httpd_resp_sendstr_chunk(r,"</main></body></html>");if(e!=ESP_OK)return e;return httpd_resp_sendstr_chunk(r,NULL);}
static esp_err_t friendly_error(httpd_req_t*r,const char*title,const char*detail){ESP_RETURN_ON_ERROR(page_begin(r,title),TAG,"page");ESP_RETURN_ON_ERROR(test_mode(r),TAG,"mode");ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"<h1>Testet kunde inte fortsätta</h1><div class=notice>"),TAG,"error");ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,detail),TAG,"detail");ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"</div><form method=post action=/homey/start><button>Försök igen</button></form><a href=/homey>Tillbaka till testöversikt</a>"),TAG,"actions");return page_end(r);}
static esp_err_t homey_get(httpd_req_t*r){
ESP_RETURN_ON_ERROR(page_begin(r,"Homey Panel – testöversikt"),TAG,"page");
ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"<h1>Homey Panel</h1>"),TAG,"head");
ESP_RETURN_ON_ERROR(test_mode(r),TAG,"mode");
ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,
"<section class=notice id=nextbox><strong id=nexttitle>NÄSTA STEG</strong><p id=nexttext>Tryck på &quot;Starta automatiskt syntetiskt test&quot;.</p></section>"),TAG,"next");
if(!(s_wrong_state_pass||s_replay_pass) && s_ctx.state!=PHONE_PROV_READY){
ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"<form method=post action=/homey/start><button>Starta automatiskt syntetiskt test</button></form>"),TAG,"start");
}
ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,
"<ul class=steps><li id=wifi>Wi-Fi anslutet: <span>väntar</span></li><li id=wrong>Fel state avvisades: <span>väntar</span></li><li id=replay>Replay avvisades: <span>väntar</span></li><li id=count>Två syntetiska Homey hittades: <span>väntar</span></li><li id=selected>Homey valt och sparat: <span>väntar</span></li><li id=ready>Panel READY: <span>väntar</span></li><li id=restore>Reboot restore: <span>väntar</span></li><li id=wipe>Homey-wipe: <span>väntar</span></li><li id=wifikept>Wi-Fi bevarad: <span>väntar</span></li></ul>"),TAG,"steps");
if(s_ctx.state==PHONE_PROV_READY){
ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"<section class=notice><strong>Förväntat på panelen</strong><br>Homey: Hemmet eller Sommarhuset<br>Status: READY</section><form method=post action=/homey/change><button>Byt Homey</button></form><form method=post action=/homey/wipe onsubmit=\"return confirm('Radera Homey-konfigurationen? Wi-Fi bevaras.')\"><button class=danger>Radera Homey-konfiguration</button></form>"),TAG,"actions");
}else if(s_ctx.state==PHONE_PROV_HOMEY_SELECTION_REQUIRED){
ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"<a href=/homey/select><button>Välj Homey</button></a>"),TAG,"select");
}
ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,
"<script>function set(id,ok,pending){const e=document.querySelector('#'+id+' span');e.textContent=pending?'väntar':(ok?'PASS':'FAIL');e.className=pending?'wait':(ok?'pass':'fail')}function next(j){const t=document.getElementById('nexttitle'),p=document.getElementById('nexttext');if(j.wipe_complete){t.textContent='STEG 3 KLART';p.textContent='Homey-konfigurationen är raderad och Wi-Fi är bevarad.';}else if(j.persistence_restored){t.textContent='NÄSTA STEG';p.textContent='Byt till Sommarhuset och radera sedan Homey-konfigurationen.';}else if(j.selected_homey_present&&j.state==='Klar'){t.textContent='STEG 1 KLART';p.textContent='Säkerhetstesterna är klara. Panelen ska visa vald Homey och READY.';}else if(j.test_started&&(!j.wrong_state_pass||!j.replay_pass)){t.textContent='TESTET MISSLYCKADES';p.textContent=!j.replay_pass?'Replay-skyddet kunde inte verifieras. Tryck Försök igen.':'Fel state kunde inte verifieras. Tryck Försök igen.';}else if(j.candidate_count===2&&j.wrong_state_pass&&j.replay_pass){t.textContent='NÄSTA STEG';p.textContent='Tryck på Välj Homey och välj Hemmet.';}else if(j.test_started){t.textContent='NÄSTA STEG';p.textContent='Säkerhetstester körs automatiskt. Vänta tills båda visar PASS.';}else{t.textContent='NÄSTA STEG';p.textContent='Tryck på Starta automatiskt syntetiskt test.';}}async function poll(){try{const j=await(await fetch('/homey/status',{cache:'no-store'})).json();set('wifi',j.wifi_connected,false);set('wrong',j.wrong_state_pass,!j.test_started);set('replay',j.replay_pass,!j.test_started);set('count',j.candidate_count===2,!j.test_started);set('selected',j.selected_homey_present,!j.selected_homey_present);set('ready',j.state==='Klar',j.state!=='Klar');set('restore',j.persistence_restored,!j.persistence_restored);set('wipe',j.wipe_complete,!j.wipe_complete);set('wifikept',j.wifi_preserved,false);next(j);}catch(e){document.getElementById('nexttitle').textContent='VÄNTAR PÅ PANELEN';document.getElementById('nexttext').textContent='Anslutningen återupptas automatiskt.';}}poll();setInterval(poll,1500);</script>"),TAG,"poll");
return page_end(r);}
static esp_err_t status_get(httpd_req_t*r){char b[640];snprintf(b,sizeof(b),"{\"state\":\"%s\",\"synthetic\":true,\"test_started\":%s,\"wifi_connected\":%s,\"wifi_preserved\":%s,\"candidate_count\":%u,\"wrong_state_pass\":%s,\"replay_pass\":%s,\"selected_homey_present\":%s,\"persistence_restored\":%s,\"display_ready_rendered\":%s,\"display_homey_name_present\":%s,\"wipe_complete\":%s}",state_name(s_ctx.state),(s_wrong_state_pass||s_replay_pass)?"true":"false",s_wifi_online?"true":"false",s_wifi_preserved?"true":"false",(unsigned)s_ctx.result.homey_count,s_wrong_state_pass?"true":"false",s_replay_pass?"true":"false",(s_ctx.state==PHONE_PROV_READY&&s_active.homey_name[0]!='\0')?"true":"false",s_persistence_restored?"true":"false",s_display_ready_rendered?"true":"false",s_display_homey_name_present?"true":"false",(s_ctx.state==PHONE_PROV_UNPROVISIONED&&s_wifi_online)?"true":"false");httpd_resp_set_type(r,"application/json; charset=utf-8");return httpd_resp_sendstr(r,b);}
static esp_err_t start_post(httpd_req_t*r){s_change_flow=false;if(s_ctx.state==PHONE_PROV_READY)phone_prov_wipe_context(&s_ctx);if(s_ctx.state!=PHONE_PROV_PORTAL_ACTIVE&&phone_prov_open(&s_ctx)!=PHONE_PROV_OK)return friendly_error(r,"Testet kunde inte startas","Provisioneringsläget kunde inte öppnas. Tryck Försök igen.");uint8_t rnd[32],wrong[32];esp_fill_random(rnd,sizeof(rnd));if(phone_prov_begin(&s_ctx,now_s(),rnd)!=PHONE_PROV_OK){zero_secure(rnd,sizeof(rnd));return friendly_error(r,"Testet kunde inte startas","En ny säker testsession kunde inte skapas.");}memcpy(wrong,rnd,sizeof(wrong));wrong[0]^=0x5a;s_wrong_state_pass=phone_prov_complete(&s_ctx,now_s(),wrong,phone_prov_mock_provider())==PHONE_PROV_ERR_MISMATCH;phone_prov_result_t ok=phone_prov_complete(&s_ctx,now_s(),rnd,phone_prov_mock_provider());phone_prov_result_t replay=phone_prov_complete(&s_ctx,now_s(),rnd,phone_prov_mock_provider());s_replay_pass=replay==PHONE_PROV_ERR_CONSUMED;zero_secure(wrong,sizeof(wrong));zero_secure(rnd,sizeof(rnd));ESP_LOGI(TAG,"PHONE_PROV wrong_state_rejected=%s",s_wrong_state_pass?"true":"false");ESP_LOGI(TAG,"PHONE_PROV replay_rejected=%s",s_replay_pass?"true":"false");if(ok!=PHONE_PROV_OK||!s_wrong_state_pass||!s_replay_pass){ESP_LOGE(TAG,"PHONE_PROV self_test result=fail wrong_state=%s replay=%s completion=%d",s_wrong_state_pass?"pass":"fail",s_replay_pass?"pass":"fail",ok);return friendly_error(r,"Testet kunde inte fortsätta",!s_replay_pass?"Replay-skyddet kunde inte verifieras. Tryck Försök igen.":"Fel state kunde inte verifieras. Tryck Försök igen.");}ESP_LOGI(TAG,"PHONE_PROV auth_complete result=success synthetic=true");ESP_LOGI(TAG,"PHONE_PROV homey_candidates count=%u",(unsigned)s_ctx.result.homey_count);show("Välj Homey","Fortsätt på telefonen");httpd_resp_set_status(r,"303 See Other");httpd_resp_set_hdr(r,"Location","/homey/select");return httpd_resp_send(r,NULL,0);}
static esp_err_t complete_post(httpd_req_t*r){return friendly_error(r,"Inget manuellt steg behövs","Syntetisk completion hanteras automatiskt. Gå tillbaka till testöversikten.");}
static esp_err_t select_get(httpd_req_t*r){
    if(s_ctx.state!=PHONE_PROV_HOMEY_SELECTION_REQUIRED)
        return friendly_error(r,"Homey-val är inte tillgängligt","Starta testet från testöversikten först.");
    ESP_RETURN_ON_ERROR(page_begin(r,"Välj Homey"),TAG,"page");
    ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"<h1>Välj Homey</h1>"),TAG,"head");
    ESP_RETURN_ON_ERROR(test_mode(r),TAG,"mode");
    if(s_change_flow){
        ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"<div class=notice>Homey-alternativen har lästs in på nytt. Välj Hemmet eller Sommarhuset.</div>"),TAG,"change mode");
    }else{
        char test[320];
        snprintf(test,sizeof(test),"<div class=notice>Fel state avvisades: <span class=%s>%s</span><br>Replay avvisades: <span class=%s>%s</span></div>",s_wrong_state_pass?"pass":"fail",s_wrong_state_pass?"PASS":"FAIL",s_replay_pass?"pass":"fail",s_replay_pass?"PASS":"FAIL");
        ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,test),TAG,"tests");
    }
    ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"<form method=post action=/homey/select><label>Homey<select name=id required><option value='' selected disabled>Välj Homey</option>"),TAG,"form");
    for(size_t i=0;i<s_ctx.result.homey_count;i++){
        char option[220];
        snprintf(option,sizeof(option),"<option value='%s'>%s</option>",s_ctx.result.homeys[i].id,s_ctx.result.homeys[i].name);
        ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,option),TAG,"option");
    }
    ESP_RETURN_ON_ERROR(httpd_resp_sendstr_chunk(r,"</select></label><button>Välj Homey</button></form><a href=/homey>Tillbaka till testöversikt</a>"),TAG,"end form");
    return page_end(r);
}
static esp_err_t select_post(httpd_req_t*r){char b[256];if(form(r,b,sizeof(b))!=ESP_OK)return friendly_error(r,"Valet kunde inte läsas","Välj en Homey och försök igen.");char *id=strstr(b,"id=");if(!id)return friendly_error(r,"Inget Homey-val hittades","Välj Hemmet eller Sommarhuset och försök igen.");phone_prov_record_v1_t rec;phone_prov_result_t x=phone_prov_select(&s_ctx,id+3,&rec,s_generation++);zero_secure(b,sizeof(b));if(x!=PHONE_PROV_OK)return friendly_error(r,"Homey-valet avvisades","Valet är inte längre giltigt. Starta om testet.");esp_err_t e=publish(&rec);zero_secure(&rec,sizeof(rec));if(e!=ESP_OK)return friendly_error(r,"Konfigurationen kunde inte sparas","Försök igen. Wi-Fi påverkas inte.");s_wipe_complete=false;s_change_flow=false;ESP_LOGI(TAG,"PHONE_PROV selection result=pass");ESP_LOGI(TAG,"PHONE_PROV sensitive_values_logged=false");httpd_resp_set_status(r,"303 See Other");httpd_resp_set_hdr(r,"Location","/homey");return httpd_resp_send(r,NULL,0);}
static esp_err_t change_post(httpd_req_t*r){
    phone_prov_result_t result=phone_prov_change_refresh(&s_ctx,phone_prov_mock_provider());
    if(result!=PHONE_PROV_OK)
        return friendly_error(r,"Byte kunde inte startas","Homey-alternativen kunde inte läsas in. Den nuvarande Homey-konfigurationen är oförändrad.");
    s_change_flow=true;
    ESP_LOGI(TAG,"PHONE_PROV change_candidates count=%u",(unsigned)s_ctx.result.homey_count);
    show("Välj Homey","Fortsätt på telefonen");
    httpd_resp_set_status(r,"303 See Other");
    httpd_resp_set_hdr(r,"Location","/homey/select");
    return httpd_resp_send(r,NULL,0);
}
static esp_err_t wipe_post(httpd_req_t*r){nvs_handle_t h;esp_err_t e=nvs_open(NS,NVS_READWRITE,&h);if(e==ESP_OK){e=erase_key(h,STAGING);if(e==ESP_OK)e=erase_key(h,ACTIVE);if(e==ESP_OK)e=nvs_commit(h);nvs_close(h);}if(e!=ESP_OK)return friendly_error(r,"Homey-konfigurationen kunde inte raderas","Wi-Fi har inte ändrats. Försök igen.");zero_secure(&s_active,sizeof(s_active));phone_prov_wipe_context(&s_ctx);s_wrong_state_pass=false;s_replay_pass=false;s_change_flow=false;s_wipe_complete=true;s_wifi_preserved=true;ESP_LOGI(TAG,"PHONE_PROV wipe result=pass wifi_preserved=true");show("Homey-installation krävs","Öppna telefonportalen");httpd_resp_set_status(r,"303 See Other");httpd_resp_set_hdr(r,"Location","/homey");return httpd_resp_send(r,NULL,0);}
esp_err_t phone_provisioning_register_handlers(httpd_handle_t s){if(!s)return ESP_ERR_INVALID_ARG;(void)phone_provisioning_boot_restore();const httpd_uri_t u[]={ {"/homey",HTTP_GET,homey_get,NULL},{"/homey/status",HTTP_GET,status_get,NULL},{"/homey/start",HTTP_POST,start_post,NULL},{"/homey/mock/complete",HTTP_POST,complete_post,NULL},{"/homey/select",HTTP_GET,select_get,NULL},{"/homey/select",HTTP_POST,select_post,NULL},{"/homey/change",HTTP_POST,change_post,NULL},{"/homey/wipe",HTTP_POST,wipe_post,NULL}};for(size_t i=0;i<sizeof(u)/sizeof(u[0]);i++){esp_err_t e=httpd_register_uri_handler(s,&u[i]);if(e!=ESP_OK&&e!=ESP_ERR_HTTPD_HANDLER_EXISTS)return e;}ESP_LOGI(TAG,"PHONE_PROV portal active=true");return ESP_OK;}
#endif
