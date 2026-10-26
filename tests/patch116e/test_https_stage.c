#include "panel_ota_https_stage.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_crt_bundle.h"
#include "esp_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_CAP 16384
#define PREAMBLE_SIZE (sizeof(esp_image_header_t)+sizeof(esp_image_segment_header_t)+sizeof(esp_app_desc_t))
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
static bool wrong_chip,wrong_magic,wrong_segment,bad_desc_magic,unterminated_version;
static bool inspect_tls,ota_active;
#define ASSERT(expr,msg) do{++tests;if(!(expr)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,msg);exit(3);}}while(0)
#define RUN(result,msg) do{panel_ota_stage_result_t rv=panel_ota_https_stage(&good_access);ASSERT(rv==(result),msg);}while(0)

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
 old_secure=1;new_secure=2;
 memset(remote_project,0,sizeof(remote_project));
 memset(remote_version,0,sizeof(remote_version));
 memset(current_project,0,sizeof(current_project));
 memset(current_version,0,sizeof(current_version));
 strcpy(remote_project,"esp32_homey_wall_panel");strcpy(remote_version,"next-release");
 strcpy(current_project,"esp32_homey_wall_panel");strcpy(current_version,"current-release");
 wrong_chip=wrong_magic=wrong_segment=bad_desc_magic=unterminated_version=false;
 inspect_tls=false;
 parts[0].address=0x9000;parts[2].address=0x10000;parts[3].address=0x610000;
 parts[5].size=0x400000;parts[5].address=0x720000;
 rebuild_stream();
}
static bool auth_fn(void *ctx){(void)ctx;++auth_calls;return auth_ok&&(!auth_fail_on||auth_calls<auth_fail_on);}
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
 strcpy(desc.project_name,"esp32_homey_wall_panel");strcpy(desc.version,"current-release");
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
  strcmp(c->url,"https://updates.example.org/firmware.bin")!=0){
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
 return (int)count;
}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t c){
 (void)c;return !force_incomplete&&stream_pos==(size_t)declared_length;
}
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c){
 ASSERT(c==&client_obj,"cleanup handle");++cleanup_calls;return ESP_OK;
}
esp_err_t esp_ota_begin(const esp_partition_t *part,size_t n,esp_ota_handle_t *handle){
 ++begin_calls;ASSERT(part==&parts[target_idx]&&n==(size_t)declared_length,"only target OTA slot");
 ASSERT(stream_pos>=PREAMBLE_SIZE,"no flash before full preamble");
 ASSERT(memcmp(stream+sizeof(esp_image_header_t)+sizeof(esp_image_segment_header_t)+16,
               "next-release",12)==0||remote_version[0]!= 'n',"descriptor prefix is loaded");
 if(ota_begin_error)return ESP_FAIL;
 *handle=42;ota_active=true;return ESP_OK;
}
esp_err_t esp_ota_write(esp_ota_handle_t handle,const void *data,size_t len){
 ++write_calls;ASSERT(ota_active&&handle==42&&data&&len>0,"valid OTA write handle");
 if(ota_write_error_at==write_calls)return ESP_FAIL;
 ASSERT(bytes_written+len<=sizeof(written),"mock write bound");
 memcpy(written+bytes_written,data,len);bytes_written+=len;return ESP_OK;
}
esp_err_t esp_ota_end(esp_ota_handle_t handle){
 ASSERT(handle==42&&ota_active,"end active handle");++end_calls;ota_active=false;
 ASSERT(bytes_written==(size_t)declared_length,"end only after complete byte count");
 ASSERT(memcmp(written,stream,bytes_written)==0,"single-connection image exact");
 return ota_end_error?ESP_FAIL:ESP_OK;
}
esp_err_t esp_ota_abort(esp_ota_handle_t handle){
 ASSERT(handle==42&&ota_active,"abort active handle");++abort_calls;ota_active=false;return ESP_OK;
}
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *part){
 ASSERT(part==&parts[target_idx]&&!ota_active&&end_calls==1,"boot only after end");
 ++boot_calls;return ota_setboot_error?ESP_FAIL:ESP_OK;
}
int64_t esp_timer_get_time(void){now_us+=clock_step_us;return now_us;}

int main(void){
#ifdef PANEL_OTA_EXPECT_BAD_URL
 reset();RUN(PANEL_OTA_STAGE_DENIED,"unsafe URL denied");
 ASSERT(init_calls==0&&begin_calls==0,"bad URL cannot reach network or flash");
 printf("PATCH116F_BAD_URL_TESTS_PASS=%d\n",tests);return 0;
#else
 reset();RUN(PANEL_OTA_STAGE_STAGED_NO_REBOOT,"valid release streamed/staged");
 ASSERT(boot_calls==1&&end_calls==1&&begin_calls==1&&abort_calls==0&&cleanup_calls==1&&inspect_tls,"success ownership");

 reset();running_idx=5;target_idx=6;RUN(PANEL_OTA_STAGE_STAGED_NO_REBOOT,"running OTA0 to OTA1");
 reset();running_idx=5;target_idx=5;RUN(PANEL_OTA_STAGE_DENIED,"same running and target");ASSERT(init_calls==0,"no unauth network");
 reset();running_idx=5;target_idx=6;bad_boot_state=true;RUN(PANEL_OTA_STAGE_DENIED,"pending verify denied");
 reset();running_idx=5;target_idx=6;run_state_error=ESP_FAIL;RUN(PANEL_OTA_STAGE_DENIED,"unreadable state denied");
 reset();ASSERT(panel_ota_https_stage(NULL)==PANEL_OTA_STAGE_DENIED,"missing credentials denied");ASSERT(init_calls==0,"no network without access");
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
 reset();strcpy(remote_version,"current-release");rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"same version");
 reset();remote_version[0]='\0';rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"empty version");
 reset();unterminated_version=true;rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"unterminated version");
 reset();new_secure=1;rebuild_stream();RUN(PANEL_OTA_STAGE_RELEASE_REJECTED,"same secure version");
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
 reset();ota_setboot_error=1;RUN(PANEL_OTA_STAGE_FINALIZE_FAILED,"otadata setboot error");ASSERT(boot_calls==1&&end_calls==1&&abort_calls==0,"no double free after end");
 reset();clock_step_us=200000000;RUN(PANEL_OTA_STAGE_INCOMPLETE,"total deadline closes before writes");ASSERT(boot_calls==0,"no boot on deadline");
 printf("PATCH116F_TEST_ASSERTIONS=%d PASS\n",tests);return 0;
#endif
}
