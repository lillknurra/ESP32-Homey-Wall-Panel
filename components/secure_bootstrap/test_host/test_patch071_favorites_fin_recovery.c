#include "athom_favorites_transport_diag.h"
#include "test_patch070_platform.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

typedef int esp_err_t;
typedef enum { HTTP_METHOD_GET } esp_http_client_method_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_SIZE 0x104
#define ESP_ERR_HTTP_FETCH_HEADER 0x7004
#define ESP_ERR_HTTP_EAGAIN 0x7007
#define ESP_ERR_ESP_TLS_TCP_CLOSED_FIN 0x8008
#define ATHOM_HOMEY_URL_MAX 256U
#define ATHOM_TOKEN_MAX 256U
#define HTTP_BODY_MAX 65536U
_Thread_local TaskHandle_t test_task = (TaskHandle_t)(uintptr_t)1U;
int64_t test_clock_us;
static struct {
    athom_favorites_fetch_diagnostic_t last_favorites_fetch;
    athom_favorites_fin_recovery_diagnostic_t last_favorites_fin_recovery;
    int last_tls_query, last_tls_error, last_tls_flags, last_perform_http_status;
} s_transport_metrics;

typedef enum {
    SUCCESS, FIN, PARTIAL_FIN, GENERIC_HEADER_ERROR, TIMEOUT, CERT_ERROR,
    HTTP401, HTTP403, PARSER_ERROR, CLOSE_FAILURE, REDIRECT_FIN, WRITE_FAILURE,
    CONNECT_FAILURE
} scenario_t;
static scenario_t steps[2];
static unsigned request_count, connect_count, write_count, read_count;
static bool first_fresh, connection_closed;
static int raw_read_result, raw_write_result, raw_connect_result;
static const char *raw_read_bytes;
static char transport_marker;
static void zero_secure(void *p,size_t size) { memset(p,0,size); }
static esp_err_t bearer_authorization(const char *token,char *out,size_t size)
{
    if (strcmp(token,"invalid-auth-fixture")==0) return ESP_ERR_INVALID_ARG;
    int n=snprintf(out,size,"Bearer %s",token);
    return n>0 && (size_t)n<size ? ESP_OK : ESP_ERR_INVALID_SIZE;
}
int __wrap_esp_transport_connect(esp_transport_handle_t,const char *,int,int);
int __wrap_esp_transport_write(esp_transport_handle_t,const char *,int,int);
int __wrap_esp_transport_read(esp_transport_handle_t,char *,int,int);
size_t __wrap_http_parser_execute(http_parser *,const http_parser_settings *,const char *,size_t);
int __real_esp_transport_connect(esp_transport_handle_t t,const char *host,int port,int timeout)
{
    assert(t==&transport_marker && strcmp(host,"synthetic.invalid")==0);
    assert(port==443 && timeout==8000); ++connect_count;
    return raw_connect_result;
}
int __real_esp_transport_write(esp_transport_handle_t t,const char *data,int len,int timeout)
{
    assert(t==&transport_marker && data!=NULL && len==266 && timeout==8000);
    ++write_count; return raw_write_result;
}
int __real_esp_transport_read(esp_transport_handle_t t,char *data,int len,int timeout)
{
    assert(t==&transport_marker && len>=64 && timeout==8000); ++read_count;
    if(raw_read_result>0)memcpy(data,raw_read_bytes,(size_t)raw_read_result);
    return raw_read_result;
}
int __real_esp_transport_get_errno(esp_transport_handle_t t) { assert(t==&transport_marker); return 0; }
size_t __real_http_parser_execute(http_parser *p,const http_parser_settings *s,const char *b,size_t len)
{ return http_parser_execute(p,s,b,len); }
static int header_complete(http_parser *parser)
{ (void)parser; athom_favorites_transport_diag_event(ATHOM_FAVORITES_RESPONSE_HEADERS_COMPLETE);return 0; }

static esp_err_t http_request_limited(const char *url,esp_http_client_method_t method,
    const char *auth,const char *content_type,const char *body,char **response,
    int *status,size_t maximum,size_t *capacity)
{
    assert(strcmp(url,"https://synthetic.invalid/selected/api/manager/users/user/me")==0);
    assert(method==HTTP_METHOD_GET && content_type==NULL && body==NULL);
    assert(strcmp(auth,"Bearer synthetic-secret")==0 && maximum==HTTP_BODY_MAX);
    assert(request_count<2U); scenario_t step=steps[request_count++];
    bool fresh=request_count==1U ? first_fresh : true;
    if(request_count==2U)assert(connection_closed); /* Existing close must precede replay. */
    s_transport_metrics.last_tls_query=0; s_transport_metrics.last_tls_error=0;
    s_transport_metrics.last_tls_flags=0; s_transport_metrics.last_perform_http_status=0;
    athom_favorites_transport_diag_client_reused();
    if(fresh){
        raw_connect_result=step==CONNECT_FAILURE ? -1 : 0;
        if(__wrap_esp_transport_connect(&transport_marker,"synthetic.invalid",443,8000)<0)return 0x7002;
        connection_closed=false;
    }
    raw_write_result=step==WRITE_FAILURE ? -1 : 266;
    char request[266]={0};
    if(__wrap_esp_transport_write(&transport_marker,request,sizeof(request),8000)<0)return 0x7003;
    athom_favorites_transport_diag_event(ATHOM_FAVORITES_REQUEST_HEADERS_SENT);
    if(step==REDIRECT_FIN)athom_favorites_transport_diag_event(ATHOM_FAVORITES_REQUEST_HEADERS_SENT);
    test_clock_us+=45000;
    char bytes[256];
    http_parser parser;http_parser_init(&parser,HTTP_RESPONSE);
    http_parser_settings settings={0};settings.on_headers_complete=header_complete;
    if(step==SUCCESS || step==HTTP401 || step==HTTP403){
        raw_read_bytes="HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\n{}";
        raw_read_result=(int)strlen(raw_read_bytes);
        assert(__wrap_esp_transport_read(&transport_marker,bytes,sizeof(bytes),8000)==raw_read_result);
        (void)__wrap_http_parser_execute(&parser,&settings,bytes,(size_t)raw_read_result);
        *status=step==HTTP401 ? 401 : step==HTTP403 ? 403 : 200;
        s_transport_metrics.last_perform_http_status=*status;
        *capacity=3;*response=malloc(3);assert(*response!=NULL);memcpy(*response,"{}",3);
        return ESP_OK;
    }
    if(step==PARTIAL_FIN || step==PARSER_ERROR){
        raw_read_bytes=step==PARTIAL_FIN ? "HTTP/1." : "BAD\r\n";
        raw_read_result=(int)strlen(raw_read_bytes);
        assert(__wrap_esp_transport_read(&transport_marker,bytes,sizeof(bytes),8000)==raw_read_result);
        (void)__wrap_http_parser_execute(&parser,&settings,bytes,(size_t)raw_read_result);
    }
    raw_read_result=step==TIMEOUT ? 0 : step==GENERIC_HEADER_ERROR ? -0x7100 : -1;
    assert(__wrap_esp_transport_read(&transport_marker,bytes,sizeof(bytes),8000)==raw_read_result);
    s_transport_metrics.last_tls_query=step==GENERIC_HEADER_ERROR || step==TIMEOUT ? 0 : 0x8008;
    if(step==CERT_ERROR){s_transport_metrics.last_tls_error=-0x2700;s_transport_metrics.last_tls_flags=4;}
    int close_result=step==CLOSE_FAILURE ? -1 : 0;
    athom_favorites_transport_diag_close_result(close_result);connection_closed=close_result==0;
    return step==TIMEOUT ? ESP_ERR_HTTP_EAGAIN : ESP_ERR_HTTP_FETCH_HEADER;
}
/* PATCH071_PRODUCTION_FUNCTIONS */

static void run(scenario_t original,scenario_t replay,bool fresh,unsigned expected_requests,
                int expected_error,int expected_status)
{
    steps[0]=original;steps[1]=replay;first_fresh=fresh;connection_closed=false;
    request_count=connect_count=write_count=read_count=0;errno=0;
    char *response=NULL;size_t capacity=0;int status=0;
    int result=favorites_fetch_user_me("https://synthetic.invalid/selected","synthetic-secret",
        &response,&capacity,&status);
    assert(result==expected_error && status==expected_status && request_count==expected_requests);
    assert(request_count<=2U);
    const athom_favorites_fin_recovery_diagnostic_t *d=&s_transport_metrics.last_favorites_fin_recovery;
    assert(d->logical_request_attempt_count==expected_requests);
    assert(d->attempted==(expected_requests==2U));
    if(expected_requests==2U){
        assert(d->eligible && d->fresh_connection && connect_count==1U);
        assert(d->original_fetch.request_result==0x7004 && d->original_fetch.connection_reused);
        assert(d->original_fetch.response_bytes_observed==0 && d->original_fetch.fin_reported);
        assert(d->original_tls_query==0x8008 && d->original_http_status==0);
        assert(!s_transport_metrics.last_favorites_fetch.connection_reused);
        assert(d->error==expected_error);
    }else{
        assert(!d->eligible && !d->original_fetch.valid);
    }
    if(response!=NULL){assert(result==0 && status>=200);free(response);}
}
int main(void)
{
    run(SUCCESS,SUCCESS,true,1,0,200);
    run(SUCCESS,SUCCESS,false,1,0,200);
    run(FIN,SUCCESS,false,2,0,200);
    char json[4096];size_t offset=0;
    assert(athom_favorites_fin_recovery_json_append(
        &s_transport_metrics.last_favorites_fin_recovery,json,sizeof(json),&offset));
    assert(strstr(json,"synthetic")==NULL && strstr(json,"Bearer")==NULL);
    printf("PATCH071_RECOVERY_JSON_SAMPLE=%s\n",json);
    run(FIN,FIN,false,2,0x7004,0);
    run(FIN,SUCCESS,true,1,0x7004,0); /* Fresh FIN cannot trigger replay. */
    run(PARTIAL_FIN,SUCCESS,false,1,0x7004,0);
    run(GENERIC_HEADER_ERROR,SUCCESS,false,1,0x7004,0);
    run(TIMEOUT,SUCCESS,false,1,ESP_ERR_HTTP_EAGAIN,0);
    run(CERT_ERROR,SUCCESS,false,1,0x7004,0);
    run(HTTP401,SUCCESS,false,1,0,401);
    run(HTTP403,SUCCESS,false,1,0,403);
    run(PARSER_ERROR,SUCCESS,false,1,0x7004,0);
    run(CLOSE_FAILURE,SUCCESS,false,1,0x7004,0);
    run(REDIRECT_FIN,SUCCESS,false,1,0x7004,0);
    run(WRITE_FAILURE,SUCCESS,false,1,0x7003,0);
    run(CONNECT_FAILURE,SUCCESS,true,1,0x7002,0);
    /* Next operation resets original FIN and attempt counters. */
    run(SUCCESS,SUCCESS,false,1,0,200);
    assert(!s_transport_metrics.last_favorites_fin_recovery.original_fetch.valid);
    char *response=NULL;size_t cap=0;int status=0;unsigned before=request_count;
    assert(favorites_fetch_user_me("https://synthetic.invalid/selected","invalid-auth-fixture",
        &response,&cap,&status)==ESP_ERR_INVALID_ARG);
    assert(request_count==before && s_transport_metrics.last_favorites_fin_recovery.logical_request_attempt_count==0);
    assert(!s_transport_metrics.last_favorites_fin_recovery.attempted);
    puts("PATCH071_FAVORITES_FIN_RECOVERY_TESTS PASS");
}
