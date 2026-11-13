#include "panel_ota_https_stage.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_crt_bundle.h"
#include "esp_timer.h"
#include "psa/crypto.h"
#include "freertos/task.h"
#include "panel_ota_handoff.h"
#include "panel_ota_supervisor.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_CAP 16384
#define PREAMBLE_SIZE (sizeof(esp_image_header_t)+sizeof(esp_image_segment_header_t)+sizeof(esp_app_desc_t))
#define ASSERT(expr,msg) do{++tests;if(!(expr)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,msg);exit(3);}}while(0)
#define RUN(result,msg) do{panel_ota_stage_result_t rv; \
 if((result)==PANEL_OTA_STAGE_STAGED_NO_REBOOT){ \
  stage_restart_catch=true; \
  if(setjmp(stage_restart_env)==0){(void)panel_ota_https_stage(&good_access,&manifest); \
   ASSERT(false,"successful selection must restart before teardown");} \
  rv=PANEL_OTA_STAGE_STAGED_NO_REBOOT;stage_restart_catch=false; \
 }else{rv=panel_ota_https_stage(&good_access,&manifest);} \
 ASSERT(rv==(result),msg);}while(0)
struct http_client_mock {unsigned tag;};
static struct http_client_mock client_obj={0x123};
static esp_partition_t parts[7]={
 {1,2,0x9000,0x6000,"nvs"}, {1,1,0xF000,0x1000,"phy_init"},
 {0,0,0x10000,0x600000,"factory"}, {1,6,0x610000,0x100000,"diaglog"},
 {1,0,0x710000,0x2000,"otadata"}, {0,0x10,0x720000,0x400000,"ota_0"},
 {0,0x11,0xB20000,0x400000,"ota_1"}
};
static int tests,auth_calls,maint_calls,init_calls,open_calls,fetch_calls,read_calls;
static int begin_calls,write_calls,end_calls,abort_calls,boot_calls,cleanup_calls;
static bool auth_ok,maint_ok,layout_missing,hide_running,hide_target,bad_boot_state;
static bool force_incomplete,force_chunked,hang_header,hang_preamble,hang_body,short_body;
static int auth_fail_on,maint_fail_on,init_error,open_error,fetch_error,read_error_at;
static int ota_begin_error,ota_write_error_at,ota_end_error,ota_setboot_error;
static int fetch_eagain_count,read_eagain_count,read_partial_size,header_after_bytes;
static int status_code,running_idx,target_idx,run_state_error,running_desc_error;
static int64_t now_us,clock_step_us,declared_length;
static size_t stream_length,stream_pos,bytes_written;
static uint8_t stream[IMAGE_CAP],written[IMAGE_CAP];
static uint32_t old_secure,new_secure;
static char remote_project[32],remote_version[32],current_project[32],current_version[32];
static panel_update_manifest_t manifest;
static bool wrong_chip,wrong_magic,wrong_segment,bad_desc_magic,unterminated_version;
static bool inspect_tls,ota_active;
static bool stage_restart_catch,cleanup_stall;
static int stage_restart_calls,cleanup_stall_would_block;
static jmp_buf stage_restart_env;
typedef enum { TIMEOUT_NONE, TIMEOUT_PRE_BEGIN, TIMEOUT_BEGIN, TIMEOUT_WRITE,
               TIMEOUT_BODY_WRITE,
               TIMEOUT_END, TIMEOUT_BEFORE_SELECT, TIMEOUT_DURING_SELECT,
               TIMEOUT_HEADERS } timeout_point_t;
static timeout_point_t timeout_point;
static bool timeout_fired;
static unsigned baseline_auth_calls;
static struct test_task_control { unsigned notifications; } task_control;
static TaskFunction_t captured_supervisor;

BaseType_t xTaskCreate(TaskFunction_t entry,const char *name,uint32_t stack,
                       void *arg,unsigned priority,TaskHandle_t *out){
 (void)name;(void)arg;ASSERT(stack==2048U&&priority==5U,"supervisor task shape");
 captured_supervisor=entry;*out=&task_control;return pdPASS;
}
uint32_t ulTaskNotifyTake(BaseType_t clear,TickType_t wait){(void)clear;(void)wait;return 0U;}
void xTaskNotifyGive(TaskHandle_t task){ASSERT(task==&task_control,"supervisor notification owner");++task_control.notifications;}
void vTaskDelete(TaskHandle_t task){(void)task;}
void vTaskDelay(TickType_t ticks){(void)ticks;}
void esp_restart(void){++stage_restart_calls;if(stage_restart_catch)longjmp(stage_restart_env,1);}

static void inject_timeout(timeout_point_t point){
 if(timeout_point==point&&!timeout_fired){
  timeout_fired=true;
  if(point==TIMEOUT_DURING_SELECT)
   ASSERT(!panel_ota_supervisor_watchdog_expired(),"reserved boot selection beats timeout");
  else
   ASSERT(panel_ota_supervisor_watchdog_expired(),"timeout owns before boot selection");
 }
}
static void rebuild_stream(void){
 memset(stream,0xA5,sizeof(stream));
 esp_image_header_t im={0};im.magic=wrong_magic?0:ESP_IMAGE_HEADER_MAGIC;
 im.chip_id=wrong_chip?0:ESP_CHIP_ID_ESP32S3;im.segment_count=1;
 esp_image_segment_header_t seg={0};seg.data_len=wrong_segment?1:(uint32_t)(stream_length - sizeof(im)-sizeof(seg));
 esp_app_desc_t desc={0};desc.magic_word=bad_desc_magic?0:ESP_APP_DESC_MAGIC_WORD;
 desc.secure_version=new_secure;
 strncpy(desc.project_name,remote_project,sizeof(desc.project_name)-1);
 strncpy(desc.version,remote_version,sizeof(desc.version)-1);
 if(unterminated_version)memset(desc.version,'X',sizeof(desc.version));
 memcpy(stream,&im,sizeof(im));
 memcpy(stream+sizeof(im),&seg,sizeof(seg));
 memcpy(stream+sizeof(im)+sizeof(seg),&desc,sizeof(desc));
}
static void reset(void){
 auth_calls=maint_calls=init_calls=open_calls=fetch_calls=read_calls=0;
 begin_calls=write_calls=end_calls=abort_calls=boot_calls=cleanup_calls=0;
 auth_ok=maint_ok=true;layout_missing=hide_running=hide_target=bad_boot_state=false;
 force_incomplete=force_chunked=hang_header=hang_preamble=hang_body=short_body=false;
 auth_fail_on=maint_fail_on=init_error=open_error=fetch_error=read_error_at=0;
 ota_begin_error=ota_write_error_at=ota_end_error=ota_setboot_error=0;
 fetch_eagain_count=read_eagain_count=0;read_partial_size=0;header_after_bytes=0;
 status_code=200;running_idx=2;target_idx=5;run_state_error=running_desc_error=0;
 now_us=0;clock_step_us=25000;declared_length=8192;
 stream_length=8192;stream_pos=bytes_written=0;ota_active=false;
 old_secure=1;new_secure=1;
 memset(remote_project,0,sizeof(remote_project));
 memset(remote_version,0,sizeof(remote_version));
 memset(current_project,0,sizeof(current_project));
 memset(current_version,0,sizeof(current_version));
 strcpy(remote_project,"esp32_homey_wall_panel");strcpy(remote_version,"1.1.0");
 strcpy(current_project,"esp32_homey_wall_panel");strcpy(current_version,"1.0.0");
 wrong_chip=wrong_magic=wrong_segment=bad_desc_magic=unterminated_version=false;
 inspect_tls=false;
 stage_restart_catch=cleanup_stall=false;stage_restart_calls=cleanup_stall_would_block=0;
 timeout_point=TIMEOUT_NONE;timeout_fired=false;
 task_control.notifications=0U;captured_supervisor=NULL;
 ASSERT(panel_ota_supervisor_start(),"supervisor state initialized");
 parts[0].address=0x9000;parts[2].address=0x10000;parts[3].address=0x610000;
 parts[5].size=0x400000;parts[5].address=0x720000;
 rebuild_stream();
 memset(&manifest,0,sizeof(manifest));
 manifest.schema_version=PANEL_UPDATE_MANIFEST_SCHEMA;
 strcpy(manifest.hardware_model,PANEL_UPDATE_MODEL_ID);
 strcpy(manifest.project,PANEL_UPDATE_PROJECT_ID);
 strcpy(manifest.channel,"stable");
 manifest.release_sequence=1001000U;
 strcpy(manifest.version,"1.1.0");
 manifest.image_size_bytes=(uint32_t)stream_length;
 snprintf(manifest.image_url,sizeof(manifest.image_url),
  "%supdates/releases/1.1.0/esp32_homey_wall_panel_%s_1.1.0_signed.bin",
  PANEL_UPDATE_ORIGIN,PANEL_UPDATE_MODEL_ID);
 psa_hash_operation_t hash=psa_hash_operation_init();size_t digest_len=0U;
 ASSERT(psa_crypto_init()==PSA_SUCCESS&&psa_hash_setup(&hash,PSA_ALG_SHA_256)==PSA_SUCCESS,
  "test digest setup");
 ASSERT(psa_hash_update(&hash,stream,stream_length)==PSA_SUCCESS&&
  psa_hash_finish(&hash,manifest.image_sha256,sizeof(manifest.image_sha256),&digest_len)==PSA_SUCCESS&&
  digest_len==sizeof(manifest.image_sha256),"test digest fixture");
}
static bool auth_fn(void *ctx){
 (void)ctx;++auth_calls;
 if(timeout_point==TIMEOUT_BEFORE_SELECT&&
    (unsigned)auth_calls==baseline_auth_calls)
  inject_timeout(TIMEOUT_BEFORE_SELECT);
 return auth_ok&&(!auth_fail_on||auth_calls<auth_fail_on)&&
        panel_ota_supervisor_worker_authorized();
}
static bool maint_fn(void *ctx){(void)ctx;++maint_calls;return maint_ok&&(!maint_fail_on||maint_calls<maint_fail_on);}
static const panel_ota_stage_access_t good_access={NULL,auth_fn,maint_fn};

esp_err_t esp_crt_bundle_attach(void *p){(void)p;return ESP_OK;}
const esp_partition_t *esp_partition_find_first(esp_partition_type_t type,esp_partition_subtype_t subtype,const char *name){
 if(layout_missing)return NULL;
 for(int i=0;i<7;i++)if(parts[i].type==type&&parts[i].subtype==subtype&&strcmp(parts[i].label,name)==0)return &parts[i];
 return NULL;
}
const esp_partition_t *esp_ota_get_running_partition(void){return hide_running?NULL:&parts[running_idx];}
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *p){(void)p;return hide_target?NULL:&parts[target_idx];}
esp_err_t esp_ota_get_state_partition(const esp_partition_t *p,esp_ota_img_states_t *state){
 (void)p;*state=bad_boot_state?ESP_OTA_IMG_PENDING_VERIFY:ESP_OTA_IMG_VALID;return run_state_error;
}
const esp_app_desc_t *esp_app_get_description(void){
 static esp_app_desc_t desc;memset(&desc,0,sizeof(desc));
 strcpy(desc.project_name,"esp32_homey_wall_panel");strcpy(desc.version,"1.0.0");
 desc.secure_version=old_secure;return &desc;
}
esp_err_t esp_ota_get_partition_description(const esp_partition_t *p,esp_app_desc_t *d){
 (void)p;if(running_desc_error)return running_desc_error;
 memset(d,0,sizeof(*d));strncpy(d->project_name,current_project,31);
 strncpy(d->version,current_version,31);d->secure_version=old_secure;return ESP_OK;
}
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *c){
 ++init_calls;
 if(!c||!c->crt_bundle_attach||c->crt_bundle_attach!=esp_crt_bundle_attach||
  !c->disable_auto_redirect||c->keep_alive_enable||c->timeout_ms!=2000||
  c->max_authorization_retries!=-1||c->buffer_size!=2048||
  strcmp(c->url,manifest.image_url)!=0){
  fprintf(stderr,"TLS CONFIG INVALID\n");exit(5);
 }
 inspect_tls=true;return init_error?NULL:&client_obj;
}
esp_err_t esp_http_client_set_method(esp_http_client_handle_t c,esp_http_client_method_t m){
 ASSERT(c==&client_obj&&m==HTTP_METHOD_GET,"HTTPS-only GET method");return ESP_OK;
}
esp_err_t esp_http_client_open(esp_http_client_handle_t c,int n){
 ASSERT(c==&client_obj&&n==0,"GET open");++open_calls;return open_error?ESP_FAIL:ESP_OK;
}
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c){
 ASSERT(c==&client_obj,"fetch handle");++fetch_calls;
 inject_timeout(TIMEOUT_HEADERS);
 if(hang_header||fetch_eagain_count-->0)return -(int64_t)ESP_ERR_HTTP_EAGAIN;
 if(fetch_error)return ESP_FAIL;
 return declared_length;
}
int esp_http_client_get_status_code(esp_http_client_handle_t c){(void)c;return status_code;}
bool esp_http_client_is_chunked_response(esp_http_client_handle_t c){(void)c;return force_chunked;}
int esp_http_client_read(esp_http_client_handle_t c,char *out,int len){
 ASSERT(c==&client_obj&&len>0,"reader handle/size");++read_calls;
 if(read_error_at==read_calls)return ESP_FAIL;
 if((hang_preamble&&stream_pos<PREAMBLE_SIZE)||(hang_body&&stream_pos>=PREAMBLE_SIZE)||
    read_eagain_count-->0)return -ESP_ERR_HTTP_EAGAIN;
 if(short_body&&stream_pos>=4000)return 0;
 if(stream_pos>=stream_length)return 0;
 size_t count=stream_length-stream_pos;
 if(count>(size_t)len)count=(size_t)len;
 if(read_partial_size>0&&count>(size_t)read_partial_size)count=(size_t)read_partial_size;
 if(header_after_bytes>0&&stream_pos<PREAMBLE_SIZE&&count>(size_t)header_after_bytes)count=(size_t)header_after_bytes;
 memcpy(out,stream+stream_pos,count);stream_pos+=count;
 if(stream_pos>=PREAMBLE_SIZE)inject_timeout(TIMEOUT_PRE_BEGIN);
 return (int)count;
}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t c){
 (void)c;return !force_incomplete&&stream_pos==(size_t)declared_length;
}
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c){
 ASSERT(c==&client_obj,"cleanup handle");++cleanup_calls;
 if(cleanup_stall){
  ++cleanup_stall_would_block;
  ASSERT(panel_ota_supervisor_watchdog_expired(),
         "post-selection failure cleanup timeout owns recovery");
  ASSERT(panel_ota_supervisor_timeout_restart_ready(),
         "failed selection released native and flash gates before cleanup");
  return ESP_FAIL;
 }
 return ESP_OK;
}
esp_err_t esp_ota_begin(const esp_partition_t *part,size_t n,esp_ota_handle_t *handle){
 ++begin_calls;ASSERT(part==&parts[target_idx]&&n==(size_t)declared_length,"only target OTA slot");
 ASSERT(stream_pos>=PREAMBLE_SIZE,"no flash before full preamble");
 ASSERT(memcmp(stream+sizeof(esp_image_header_t)+sizeof(esp_image_segment_header_t)+16,
               "1.1.0",5)==0||remote_version[0]!= '1',"descriptor prefix is loaded");
 if(ota_begin_error)return ESP_FAIL;
 *handle=42;ota_active=true;inject_timeout(TIMEOUT_BEGIN);return ESP_OK;
}
esp_err_t esp_ota_write(esp_ota_handle_t handle,const void *data,size_t len){
 ++write_calls;ASSERT(ota_active&&handle==42&&data&&len>0,"valid OTA write handle");
 if(ota_write_error_at==write_calls)return ESP_FAIL;
 ASSERT(bytes_written+len<=sizeof(written),"mock write bound");
 memcpy(written+bytes_written,data,len);bytes_written+=len;
 if(write_calls==1)inject_timeout(TIMEOUT_WRITE);
 if(write_calls==2)inject_timeout(TIMEOUT_BODY_WRITE);
 return ESP_OK;
}
esp_err_t esp_ota_end(esp_ota_handle_t handle){
 ASSERT(handle==42&&ota_active,"end active handle");++end_calls;ota_active=false;
 ASSERT(bytes_written==(size_t)declared_length,"end only after complete byte count");
 ASSERT(memcmp(written,stream,bytes_written)==0,"single-connection image exact");
 inject_timeout(TIMEOUT_END);
 return ota_end_error?ESP_FAIL:ESP_OK;
}
esp_err_t esp_ota_abort(esp_ota_handle_t handle){
 ASSERT(handle==42&&ota_active,"abort active handle");++abort_calls;ota_active=false;return ESP_OK;
}
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *part){
 ASSERT(part==&parts[target_idx]&&!ota_active&&end_calls==1,"boot only after end");
 ++boot_calls;inject_timeout(TIMEOUT_DURING_SELECT);return ota_setboot_error?ESP_FAIL:ESP_OK;
}
int64_t esp_timer_get_time(void){now_us+=clock_step_us;return now_us;}

int main(void){
#ifdef PANEL_OTA_EXPECT_BAD_URL
 reset();strcpy(manifest.image_url,"https://updates.invalid/image.bin");
 RUN(PANEL_OTA_STAGE_DENIED,"unsafe URL denied");
 ASSERT(init_calls==0&&begin_calls==0,"bad URL cannot reach network or flash");
 printf("PATCH116F_BAD_URL_TESTS_PASS=%d\n",tests);return 0;
#else
 reset();RUN(PANEL_OTA_STAGE_STAGED_NO_REBOOT,"valid release streamed/staged");
 baseline_auth_calls=(unsigned)auth_calls;
 ASSERT(boot_calls==1&&end_calls==1&&begin_calls==1&&abort_calls==0&&cleanup_calls==0&&
        stage_restart_calls==1&&inspect_tls,"success restarts before TLS teardown");

 /* Successful boot selection must not wait for TLS cleanup. The cleanup mock
  * is configured as stalled; reaching the controlled restart with zero calls
  * proves the production path leaves teardown to the imminent reboot. */
 reset();cleanup_stall=true;RUN(PANEL_OTA_STAGE_STAGED_NO_REBOOT,
        "selected image restarts despite stalled cleanup");
 ASSERT(boot_calls==1&&stage_restart_calls==1&&cleanup_calls==0&&
        cleanup_stall_would_block==0,
        "successful selection bypasses potentially stalled client cleanup");

 reset();timeout_point=TIMEOUT_HEADERS;RUN(PANEL_OTA_STAGE_DENIED,"timeout during TLS/header work revokes later flash");
 ASSERT(timeout_fired&&begin_calls==0&&boot_calls==0&&panel_ota_supervisor_timeout_restart_ready(),
        "network timeout owns restart without OTA writes");
 reset();timeout_point=TIMEOUT_PRE_BEGIN;RUN(PANEL_OTA_STAGE_DENIED,"timeout before OTA admission blocks begin");
 ASSERT(timeout_fired&&begin_calls==0&&write_calls==0&&boot_calls==0&&
        panel_ota_supervisor_timeout_restart_ready(),"timeout before begin performs no destructive call");
 reset();timeout_point=TIMEOUT_BEGIN;RUN(PANEL_OTA_STAGE_DENIED,"timeout during admitted begin waits for session close");
 ASSERT(timeout_fired&&begin_calls==1&&write_calls==0&&abort_calls==0&&boot_calls==0&&
        panel_ota_supervisor_timeout_restart_ready(),"begin completion then revoke prevents later operations");
 reset();timeout_point=TIMEOUT_WRITE;RUN(PANEL_OTA_STAGE_DENIED,"timeout during admitted write prevents later writes");
 ASSERT(timeout_fired&&begin_calls==1&&write_calls==1&&end_calls==0&&boot_calls==0&&abort_calls==0&&
        panel_ota_supervisor_timeout_restart_ready(),"write completes before deferred restart; partial slot not selected");
 ASSERT(!panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGED)&&
        panel_ota_handoff_take_result()==PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT,
        "late worker cannot replace timeout with staged success");
 reset();timeout_point=TIMEOUT_BODY_WRITE;RUN(PANEL_OTA_STAGE_DENIED,"timeout during body write prevents later writes");
 ASSERT(timeout_fired&&begin_calls==1&&write_calls==2&&end_calls==0&&boot_calls==0&&abort_calls==0&&
        panel_ota_supervisor_timeout_restart_ready(),"body write completion precedes timeout restart");
 reset();timeout_point=TIMEOUT_END;RUN(PANEL_OTA_STAGE_DENIED,"timeout during finalization prevents boot selection");
 ASSERT(timeout_fired&&begin_calls==1&&end_calls==1&&boot_calls==0&&
        panel_ota_supervisor_timeout_restart_ready(),"end completes then revoked worker cannot select boot");
 reset();timeout_point=TIMEOUT_BEFORE_SELECT;RUN(PANEL_OTA_STAGE_DENIED,"timeout wins immediately before boot selection");
 ASSERT(timeout_fired&&end_calls==1&&boot_calls==0&&panel_ota_supervisor_timeout_restart_ready(),
        "timeout claim atomically denies boot selection");
 reset();timeout_point=TIMEOUT_DURING_SELECT;RUN(PANEL_OTA_STAGE_STAGED_NO_REBOOT,
        "admitted boot selection wins deadline boundary");
 ASSERT(timeout_fired&&boot_calls==1&&stage_restart_calls==1&&
        !panel_ota_supervisor_timeout_restart_ready(),
        "admitted boot selection wins deadline and worker restarts once");
 ASSERT(!panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED),
        "selection completion prevents a competing worker terminal result");
 ASSERT(panel_ota_handoff_take_result()==PANEL_OTA_HANDOFF_RESULT_STAGED,
        "staged result is recorded before the worker restart");

 reset();manifest.image_sha256[0]^=1U;RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"complete image digest mismatch");
 ASSERT(begin_calls==1&&abort_calls==1&&end_calls==0&&boot_calls==0,"digest mismatch never selects boot");
 reset();manifest.release_sequence=1000000U;RUN(PANEL_OTA_STAGE_DENIED,"stale release sequence denied");
 ASSERT(init_calls==0&&begin_calls==0,"stale release no network or flash");
 reset();strcpy(manifest.hardware_model,"other-model");RUN(PANEL_OTA_STAGE_DENIED,"wrong hardware denied");
 ASSERT(init_calls==0,"wrong hardware no network");

 reset();running_idx=5;target_idx=6;RUN(PANEL_OTA_STAGE_STAGED_NO_REBOOT,"running OTA0 to OTA1");
 reset();running_idx=5;target_idx=5;RUN(PANEL_OTA_STAGE_DENIED,"same running and target");ASSERT(init_calls==0,"no unauth network");
 reset();running_idx=5;target_idx=6;bad_boot_state=true;RUN(PANEL_OTA_STAGE_DENIED,"pending verify denied");
 reset();running_idx=5;target_idx=6;run_state_error=ESP_FAIL;RUN(PANEL_OTA_STAGE_DENIED,"unreadable state denied");
 reset();ASSERT(panel_ota_https_stage(NULL,NULL)==PANEL_OTA_STAGE_DENIED,"missing credentials denied");ASSERT(init_calls==0,"no network without access");
 reset();auth_ok=false;RUN(PANEL_OTA_STAGE_DENIED,"unauthorized denied");ASSERT(init_calls==0,"no network auth denied");
 reset();maint_ok=false;RUN(PANEL_OTA_STAGE_DENIED,"RGB not quiescent denied");ASSERT(init_calls==0,"no network before RGB proof");
 reset();layout_missing=true;RUN(PANEL_OTA_STAGE_DENIED,"layout required");
 reset();parts[0].address=0xa000;RUN(PANEL_OTA_STAGE_DENIED,"NVS layout changed");
 reset();parts[2].address=0x20000;RUN(PANEL_OTA_STAGE_DENIED,"factory changed");
 reset();parts[3].address=0x600000;RUN(PANEL_OTA_STAGE_DENIED,"diaglog changed");
 reset();parts[5].size=0x200000;RUN(PANEL_OTA_STAGE_DENIED,"OTA slot insufficient");
 reset();hide_running=true;RUN(PANEL_OTA_STAGE_DENIED,"running absent");
 reset();hide_target=true;RUN(PANEL_OTA_STAGE_DENIED,"target absent");
 reset();running_desc_error=ESP_FAIL;RUN(PANEL_OTA_STAGE_DENIED,"running description absent");
 reset();strcpy(current_project,"wrong");RUN(PANEL_OTA_STAGE_DENIED,"running project mismatch");
 reset();strcpy(current_version,"wrong");RUN(PANEL_OTA_STAGE_DENIED,"running version mismatch");
 reset();init_error=1;RUN(PANEL_OTA_STAGE_TRANSPORT_ERROR,"client init failure");ASSERT(cleanup_calls==0,"no cleanup null");
 reset();open_error=1;RUN(PANEL_OTA_STAGE_TRANSPORT_ERROR,"TLS open failure");ASSERT(cleanup_calls==1&&begin_calls==0,"close no flash");
 reset();hang_header=true;clock_step_us=0;RUN(PANEL_OTA_STAGE_INCOMPLETE,"frozen clock EAGAIN header bounded");ASSERT(fetch_calls<=8&&begin_calls==0&&cleanup_calls==1,"header retries bounded, no flash");
 reset();fetch_eagain_count=3;RUN(PANEL_OTA_STAGE_STAGED_NO_REBOOT,"slow response headers recover");ASSERT(fetch_calls==4,"header count");
 reset();fetch_error=1;RUN(PANEL_OTA_STAGE_TRANSPORT_ERROR,"header read failure");ASSERT(begin_calls==0,"no flash header error");
 reset();status_code=302;RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"HTTP redirect rejected");ASSERT(begin_calls==0,"no flash redirect");
 reset();status_code=401;RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"HTTP auth response rejected");
 reset();force_chunked=true;RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"chunked body rejected");
 reset();declared_length=0;RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"unknown length rejected");
 reset();declared_length=3;RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"small length rejected");
 reset();declared_length=0x400001;RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"oversize rejected");
 reset();hang_preamble=true;clock_step_us=0;RUN(PANEL_OTA_STAGE_INCOMPLETE,"frozen clock preamble EAGAIN bounded");ASSERT(begin_calls==0&&read_calls<=9,"no flash header EAGAIN exhaustion");
 reset();read_eagain_count=3;RUN(PANEL_OTA_STAGE_STAGED_NO_REBOOT,"preamble EAGAIN recover");
 reset();read_partial_size=1;clock_step_us=1000;RUN(PANEL_OTA_STAGE_STAGED_NO_REBOOT,"slow bytewise preamble and body works");
 reset();read_partial_size=1;clock_step_us=2000000;RUN(PANEL_OTA_STAGE_INCOMPLETE,"aggregate timeout on slow header");ASSERT(begin_calls==0&&boot_calls==0,"slow header no writes");
 reset();read_error_at=1;RUN(PANEL_OTA_STAGE_TRANSPORT_ERROR,"header transport error");ASSERT(begin_calls==0,"no flash header error");
 reset();stream_length=100;rebuild_stream();RUN(PANEL_OTA_STAGE_INCOMPLETE,"truncated prefix");ASSERT(begin_calls==0,"never write partial prefix");
 reset();wrong_magic=true;rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"bad image magic");ASSERT(begin_calls==0,"bad header no flash");
 reset();wrong_chip=true;rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"bad chip");ASSERT(begin_calls==0,"chip no flash");
 reset();wrong_segment=true;rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"bad segment length");ASSERT(begin_calls==0,"segment no flash");
 reset();bad_desc_magic=true;rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"bad descriptor magic");ASSERT(begin_calls==0,"descriptor no flash");
 reset();strcpy(remote_project,"other");rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"wrong project");ASSERT(begin_calls==0,"wrong project no flash");
 reset();strcpy(remote_version,"1.0.0");rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"same version");
 reset();strcpy(remote_version,"0.9.0");rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"downgrade version");
 reset();remote_version[0]='\0';rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"empty version");
 reset();unterminated_version=true;rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"unterminated version");
 reset();new_secure=2;rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"secure version change rejected");
 reset();new_secure=0;rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"old secure version");
 reset();auth_fail_on=2;RUN(PANEL_OTA_STAGE_DENIED,"auth revoked during headers");ASSERT(begin_calls==0,"no flash before auth");
 reset();maint_fail_on=2;RUN(PANEL_OTA_STAGE_DENIED,"maintenance revoked during headers");ASSERT(begin_calls==0,"no flash before maintenance");
 reset();auth_fail_on=291;RUN(PANEL_OTA_STAGE_DENIED,"auth revoked before OTA erase");ASSERT(begin_calls==0,"no erase when revoked before begin");
 reset();auth_fail_on=292;RUN(PANEL_OTA_STAGE_DENIED,"auth revoked after OTA begin");ASSERT(begin_calls==1&&write_calls==0&&abort_calls==1&&boot_calls==0,"abort before first write");
 reset();auth_fail_on=294;RUN(PANEL_OTA_STAGE_DENIED,"auth revoked during stream");ASSERT(begin_calls==1&&abort_calls==1&&boot_calls==0,"abort during stream");
 reset();auth_fail_on=297;RUN(PANEL_OTA_STAGE_DENIED,"auth revoked before signature check");ASSERT(end_calls==0&&abort_calls==1&&boot_calls==0,"abort before verify");
 reset();auth_fail_on=298;RUN(PANEL_OTA_STAGE_DENIED,"auth revoked after signature check");ASSERT(end_calls==1&&abort_calls==0&&boot_calls==0,"validated image without boot selection");
 reset();read_error_at=289;RUN(PANEL_OTA_STAGE_TRANSPORT_ERROR,"body transport failure");ASSERT(abort_calls==1&&boot_calls==0,"abort on body error");
 reset();ota_begin_error=1;RUN(PANEL_OTA_STAGE_TRANSPORT_ERROR,"flash begin fails");ASSERT(abort_calls==0&&boot_calls==0,"no abort without live handle");
 reset();ota_write_error_at=1;RUN(PANEL_OTA_STAGE_TRANSPORT_ERROR,"preamble flash fails");ASSERT(abort_calls==1&&boot_calls==0,"abort failed write");
 reset();ota_write_error_at=2;RUN(PANEL_OTA_STAGE_TRANSPORT_ERROR,"body flash fails");ASSERT(abort_calls==1&&boot_calls==0,"abort failed body write");
 reset();hang_body=true;clock_step_us=0;RUN(PANEL_OTA_STAGE_INCOMPLETE,"frozen clock body EAGAIN bounded");ASSERT(abort_calls==1&&boot_calls==0,"abort on stall");
 reset();short_body=true;RUN(PANEL_OTA_STAGE_INCOMPLETE,"premature FIN");ASSERT(abort_calls==1&&boot_calls==0,"abort on FIN");
 reset();force_incomplete=true;RUN(PANEL_OTA_STAGE_INCOMPLETE,"HTTP reports incomplete");ASSERT(abort_calls==1&&boot_calls==0,"no boot after incomplete");
 reset();ota_end_error=1;RUN(PANEL_OTA_STAGE_FINALIZE_FAILED,"native signature/image validation failure");ASSERT(abort_calls==0&&boot_calls==0&&end_calls==1,"end consumes handle even on failure");
 reset();ota_setboot_error=1;RUN(PANEL_OTA_STAGE_FINALIZE_FAILED,"otadata setboot error");
 ASSERT(boot_calls==1&&end_calls==1&&abort_calls==0&&cleanup_calls==1&&
        stage_restart_calls==0,"failed boot selection is not STAGED and still tears down");
 ASSERT(panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED),
        "failed selection remains a worker-owned failure after teardown");
 ASSERT(panel_ota_handoff_take_result()==PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED,
        "failed selection cannot publish a staged result");
 reset();ota_setboot_error=1;timeout_point=TIMEOUT_DURING_SELECT;
 RUN(PANEL_OTA_STAGE_FINALIZE_FAILED,"failed selection after deadline remains failure");
 ASSERT(timeout_fired&&boot_calls==1&&stage_restart_calls==0&&
        panel_ota_supervisor_timeout_restart_ready(),
        "failed reserved selection resolves timeout only after native call exits");
 ASSERT(!panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGED),
        "late maintenance completion cannot replace timeout with STAGED");
 ASSERT(panel_ota_handoff_take_result()==PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT,
        "failed selection at deadline records timeout, not staged success");
 reset();ota_setboot_error=1;cleanup_stall=true;
 RUN(PANEL_OTA_STAGE_FINALIZE_FAILED,"failed selection cleanup remains timeout-recoverable");
 ASSERT(cleanup_calls==1&&cleanup_stall_would_block==1&&boot_calls==1&&
        stage_restart_calls==0,
        "failure cleanup is outside the boot-selection critical gate");
 ASSERT(!panel_ota_supervisor_worker_complete(PANEL_OTA_HANDOFF_RESULT_STAGE_FAILED),
        "post-selection cleanup timeout keeps sole supervisor ownership");
 ASSERT(panel_ota_handoff_take_result()==PANEL_OTA_HANDOFF_RESULT_SUPERVISOR_TIMEOUT,
        "cleanup timeout cannot become a worker failure or staged outcome");
 reset();clock_step_us=200000000;RUN(PANEL_OTA_STAGE_INCOMPLETE,"total deadline closes before writes");ASSERT(boot_calls==0,"no boot on deadline");
 printf("PATCH116F_TEST_ASSERTIONS=%d PASS\n",tests);return 0;
#endif
}
