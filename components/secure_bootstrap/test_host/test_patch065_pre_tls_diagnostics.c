#include "athom_pre_tls_diag.h"
#include "test_patch065_platform.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
static int task=1, mode, network_calls, setup_calls, readiness_calls;
static bool ready=true;
static int64_t clock_us;
TaskHandle_t xTaskGetCurrentTaskHandle(void) {errno=999;return (TaskHandle_t)(uintptr_t)task;}
int64_t esp_timer_get_time(void) {return clock_us;}
esp_netif_t *esp_netif_get_default_netif(void) {++readiness_calls;errno=888;return ready ? (esp_netif_t *)(uintptr_t)1 : NULL;}
bool esp_netif_is_netif_up(esp_netif_t *n) {(void)n;return ready;}
int esp_netif_get_ip_info(esp_netif_t *n, esp_netif_ip_info_t *ip) {(void)n;ip->ip.addr=ready;return 0;}
int esp_netif_get_dns_info(esp_netif_t *n, esp_netif_dns_type_t t, esp_netif_dns_info_t *d) {(void)n;(void)t;d->ip.addr=ready;return 0;}
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap) {memset(ap,0x55,sizeof(*ap));return ready ? 0 : -1;}
int __wrap_lwip_getaddrinfo(const char *,const char *,const struct addrinfo *,struct addrinfo **);
int __wrap_lwip_socket(int,int,int);
int __wrap_lwip_connect(int,const struct sockaddr *,socklen_t);
int __wrap_select(int,fd_set *,fd_set *,fd_set *,struct timeval *);
int __wrap_lwip_getsockopt(int,int,int,void *,socklen_t *);
int __wrap_mbedtls_ssl_setup(mbedtls_ssl_context *,const mbedtls_ssl_config *);
int __wrap_esp_tls_conn_new_sync(const char *,int,int,const esp_tls_cfg_t *,esp_tls_t *);
int __real_lwip_getaddrinfo(const char *h,const char *s,const struct addrinfo *a,struct addrinfo **r)
{
 (void)h;(void)s;(void)a; ++network_calls;errno=123;
 static struct addrinfo second, first; memset(&first,0,sizeof(first));memset(&second,0,sizeof(second));
 first.ai_family=AF_INET;first.ai_next=&second;
 *r=mode==0 ? NULL : &first;return mode==0 ? EAI_FAIL : 0;
}
int __real_lwip_socket(int a,int b,int c) {(void)a;(void)b;(void)c;++network_calls;errno=mode==1 ? EMFILE : 234;return mode==1 ? -1 : 7;}
int __real_lwip_connect(int fd,const struct sockaddr *a,socklen_t l)
{assert(fd==7);(void)a;(void)l;++network_calls;clock_us+=1000;errno=mode==2 ? ECONNREFUSED : EINPROGRESS;return mode==8 ? 0 : -1;}
int __real_select(int n,fd_set *r,fd_set *w,fd_set *e,struct timeval *t)
{assert(n==8);(void)r;(void)w;(void)e;(void)t;++network_calls;clock_us+=12000000;errno=mode==4 ? EBADF : 345;return mode==3 ? 0 : (mode==4 ? -1 : 1);}
int __real_lwip_getsockopt(int fd,int l,int o,void *v,socklen_t *len)
{assert(fd==7 && l==SOL_SOCKET && o==SO_ERROR && *len==sizeof(int));++network_calls;*(int *)v=mode==5 ? ECONNREFUSED : 0;errno=mode==6 ? EBADF : 456;return mode==6 ? -1 : 0;}
int __real_mbedtls_ssl_setup(mbedtls_ssl_context *s,const mbedtls_ssl_config *c)
{(void)s;(void)c;++setup_calls;errno=567;return mode==10 ? -99 : 0;}
int __real_esp_tls_conn_new_sync(const char *h,int l,int port,const esp_tls_cfg_t *cfg,esp_tls_t *tls)
{
 (void)l;(void)port;(void)cfg;(void)tls;
 struct addrinfo *a=NULL;int rc=__wrap_lwip_getaddrinfo(h,NULL,NULL,&a);assert(errno==123);
 if(rc) {errno=0;return -1;}
 int fd=__wrap_lwip_socket(AF_INET,SOCK_STREAM,0);assert(errno==(mode==1 ? EMFILE : 234));if(fd<0){errno=0;return -1;}
 rc=__wrap_lwip_connect(fd,NULL,0);assert(errno==(mode==2 ? ECONNREFUSED : EINPROGRESS));
 if(rc<0 && mode==2){errno=0;return -1;}
 if(rc<0){fd_set w;FD_ZERO(&w);FD_SET(fd,&w);rc=__wrap_select(fd+1,NULL,&w,NULL,NULL);assert(errno==(mode==4 ? EBADF : 345));if(rc<=0){errno=0;return -1;}
 int err=0;socklen_t len=sizeof(err);rc=__wrap_lwip_getsockopt(fd,SOL_SOCKET,SO_ERROR,&err,&len);assert(errno==(mode==6 ? EBADF : 456));if(rc<0||err){errno=0;return -1;}}
 rc=__wrap_mbedtls_ssl_setup(NULL,NULL);assert(errno==567);return rc==0 ? 1 : -1;
}
static void classification(const athom_pre_tls_diagnostic_t *d, const char *expected)
{
 char json[2048], fragment[100];
 snprintf(fragment,sizeof(fragment), "\"result\":\"%s\"", expected);
 assert(athom_pre_tls_diag_json(d,json,sizeof(json)) && strstr(json,fragment));
}
static athom_pre_tls_diagnostic_t run(int scenario)
{
 mode=scenario;network_calls=setup_calls=0;clock_us=0;errno=42;
 bool acquired=athom_pre_tls_diag_begin(true,71);assert(acquired && errno==42);
 int rc=__wrap_esp_tls_conn_new_sync("private.invalid",15,443,NULL,NULL);int saved=errno;
 athom_pre_tls_diagnostic_t d;athom_pre_tls_diag_finish(acquired,&d);assert(errno==saved);
 assert(d.valid && d.request_sequence==71 && d.connection_called);
 assert(d.connection_completed==(rc==1));assert(d.connection_result==rc);
 assert(d.default_route_present && d.netif_up && d.ip_assigned && d.dns_server_configured && d.wifi_associated);
 const char *classes[]={"dns_error","socket_error","connect_error","connect_wait_timeout","connect_wait_error","connect_error","so_error_query_failed","tls_connected","tls_connected","unknown","tls_setup_reached"};
 classification(&d,classes[scenario]);
 return d;
}
int main(void)
{
 athom_pre_tls_diagnostic_t d=run(0);assert(d.dns_started&&!d.dns_ok&&!d.socket_attempted&&network_calls==1);
 d=run(1);assert(d.dns_ok&&d.dns_result_count==2&&d.address_family==4&&d.socket_attempted&&!d.socket_created&&d.socket_error==EMFILE&&network_calls==2);
 d=run(2);assert(d.connect_started&&!d.connect_pending&&!d.wait_observed&&d.connect_error==ECONNREFUSED&&network_calls==3);
 d=run(3);assert(d.wait_observed&&d.wait_timeout&&!d.so_error_observed&&!d.tls_setup_started&&d.connect_error==EINPROGRESS&&d.connect_elapsed_ms==12001&&network_calls==4);
 d=run(4);assert(d.wait_error==EBADF&&!d.tcp_connected);
 d=run(5);assert(d.so_error_observed&&d.so_error==ECONNREFUSED&&!d.tcp_connected&&!d.tls_setup_started&&network_calls==5);
 d=run(6);assert(!d.so_error_observed&&d.so_query_error==EBADF);
 d=run(7);assert(d.tcp_connected&&d.tls_setup_started&&d.connection_completed&&network_calls==5&&setup_calls==1);
 d=run(8);assert(d.tcp_connected&&!d.wait_observed&&d.tls_setup_started&&network_calls==3);
 d=run(10);assert(d.tcp_connected&&d.tls_setup_started&&!d.connection_completed);
 /* No connection needed (keep-alive), remote role and rejected overlap. */
 bool a=athom_pre_tls_diag_begin(true,81);assert(a);
 mode=7;__wrap_esp_tls_conn_new_sync("private.invalid",15,443,NULL,NULL);
 mode=0;__wrap_esp_tls_conn_new_sync("private.invalid",15,443,NULL,NULL);
 athom_pre_tls_diag_finish(a,&d);assert(d.connection_count==2 && !d.tcp_connected && !d.tls_setup_started && !d.socket_created);classification(&d,"dns_error");
 a=athom_pre_tls_diag_begin(true,81);assert(a);assert(!athom_pre_tls_diag_begin(true,82));
 task=2;mode=7;__wrap_esp_tls_conn_new_sync("private.invalid",15,443,NULL,NULL);
 athom_pre_tls_diag_finish(false,&d);assert(!d.valid);task=1;
 athom_pre_tls_diag_finish(a,&d);assert(d.valid&&d.request_sequence==81&&!d.connection_called);
 int before=readiness_calls;assert(!athom_pre_tls_diag_begin(false,90));mode=7;
 __wrap_esp_tls_conn_new_sync("private.invalid",15,443,NULL,NULL);athom_pre_tls_diag_finish(false,&d);
 assert(!d.valid&&readiness_calls==before);
 memset(&d,0x7f,sizeof(d));d.valid=false;char json[2048];
 assert(athom_pre_tls_diag_json(&d,json,sizeof(json)));assert(strcmp(json,"{\"valid\":false}")==0);
 ready=false;a=athom_pre_tls_diag_begin(true,91);athom_pre_tls_diag_finish(a,&d);assert(d.readiness_observed&&!d.default_route_present&&!d.wifi_associated);ready=true;
 memset(&d,0,sizeof(d));d.valid=true;d.connection_called=true;classification(&d,"unknown");
 d.connect_started=true;d.connect_pending=true;classification(&d,"connect_pending");
 d=run(7);athom_pre_tls_diagnostic_t original=d;before=network_calls;
 assert(athom_pre_tls_diag_json(&d,json,sizeof(json)));assert(memcmp(&d,&original,sizeof(d))==0&&network_calls==before);
 assert(strstr(json,"private.invalid")==NULL && strstr(json,"\"fd\"")==NULL);
 printf("JSON=%s\n",json);
 assert(!athom_pre_tls_diag_json(&d,json,2)&&json[0]==0);
 puts("PATCH065_FOCUSED_TESTS=PASS");
}
