/* Script the existing calls; compile the production observer, not a model. */
#include "../athom_pre_tls_diag.c"
#include <assert.h>
static int task = 1, scenario, hs_calls, tx_calls, rx_calls, conn_calls;
static int64_t now_us;
static unsigned char private_payload[] = "PRIVATE_TLS_PAYLOAD_NEVER_EXPOSE";
TaskHandle_t xTaskGetCurrentTaskHandle(void) { errno = 999; return (void *)(uintptr_t)task; }
int64_t esp_timer_get_time(void) { errno = 888; return now_us; }
esp_netif_t *esp_netif_get_default_netif(void) { return NULL; }
bool esp_netif_is_netif_up(esp_netif_t *p) { (void)p; return false; }
int esp_netif_get_ip_info(esp_netif_t *p, esp_netif_ip_info_t *i) {(void)p;(void)i;return -1;}
int esp_netif_get_dns_info(esp_netif_t *p, esp_netif_dns_type_t t, esp_netif_dns_info_t *i) {(void)p;(void)t;(void)i;return -1;}
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *a) {(void)a;return -1;}
int __real_lwip_getaddrinfo(const char *a,const char *b,const struct addrinfo *c,struct addrinfo **d) {(void)a;(void)b;(void)c;(void)d;assert(0);return -1;}
int __real_lwip_socket(int a,int b,int c) {(void)a;(void)b;(void)c;assert(0);return -1;}
int __real_lwip_connect(int a,const struct sockaddr *b,socklen_t c) {(void)a;(void)b;(void)c;assert(0);return -1;}
int __real_select(int a,fd_set *b,fd_set *c,fd_set *d,struct timeval *e) {(void)a;(void)b;(void)c;(void)d;(void)e;assert(0);return -1;}
int __real_lwip_getsockopt(int a,int b,int c,void *d,socklen_t *e) {(void)a;(void)b;(void)c;(void)d;(void)e;assert(0);return -1;}
int __real_mbedtls_ssl_setup(mbedtls_ssl_context *s,const mbedtls_ssl_config *c) {(void)s;(void)c;return 0;}
int __real_mbedtls_net_send(void *ctx,const unsigned char *buf,size_t len)
{
    assert(ctx == (void *)(uintptr_t)77 && buf == private_payload && len == 17);
    assert(errno == 42); ++tx_calls; now_us += 2000;
    errno = scenario == 3 ? EAGAIN : (scenario == 5 ? EPIPE : 0);
    return scenario == 3 ? MBEDTLS_ERR_SSL_WANT_WRITE : (scenario == 5 ? -0x0050 : (scenario == 7 ? 0 : 17));
}
int __real_mbedtls_net_recv(void *ctx,unsigned char *buf,size_t len)
{
    assert(ctx == (void *)(uintptr_t)77 && buf == private_payload);
    assert(len == (scenario == 7 ? 0U : 9U) && errno == 43);
    ++rx_calls; now_us += 3000;
    errno = scenario == 1 || (scenario == 2 && hs_calls > 1) ? EAGAIN : 0;
    return scenario == 4 || scenario == 7 ? 0 : (errno ? MBEDTLS_ERR_SSL_WANT_READ : 9);
}
int __real_mbedtls_ssl_handshake(mbedtls_ssl_context *ssl)
{
    assert(errno == 41); ++hs_calls;
    ssl->MBEDTLS_PRIVATE(state) = scenario == 3 ? MBEDTLS_SSL_CLIENT_HELLO : MBEDTLS_SSL_SERVER_HELLO;
    if (scenario != 0 && (hs_calls == 1 || scenario == 3)) {
        errno = 42; int ret = __wrap_mbedtls_net_send((void *)(uintptr_t)77,private_payload,17);
        assert(ret == (scenario == 3 ? MBEDTLS_ERR_SSL_WANT_WRITE : (scenario == 5 ? -0x0050 : (scenario == 7 ? 0 : 17))));
        assert(errno == (scenario == 3 ? EAGAIN : (scenario == 5 ? EPIPE : 0)));
    }
    if (scenario != 0 && scenario != 3 && scenario != 5) {
        errno = 43; int ret = __wrap_mbedtls_net_recv((void *)(uintptr_t)77,private_payload,scenario == 7 ? 0 : 9);
        assert(ret == (scenario == 4 || scenario == 7 ? 0 : (scenario == 1 || (scenario == 2 && hs_calls > 1) ? MBEDTLS_ERR_SSL_WANT_READ : 9)));
        assert(errno == (scenario == 1 || (scenario == 2 && hs_calls > 1) ? EAGAIN : 0));
    }
    if (scenario == 6) ssl->MBEDTLS_PRIVATE(state) = MBEDTLS_SSL_HANDSHAKE_OVER;
    errno = 57;
    return scenario == 6 ? 0 : (scenario == 4 ? -0x0050 : (scenario == 5 ? -0x7200 : (scenario == 3 ? MBEDTLS_ERR_SSL_WANT_WRITE : MBEDTLS_ERR_SSL_WANT_READ)));
}
int __real_esp_tls_conn_new_sync(const char *host,int length,int port,const esp_tls_cfg_t *cfg,esp_tls_t *tls)
{
    assert(strcmp(host,"PRIVATE_HOST") == 0 && length == 12 && port == 443 && !cfg && !tls);
    ++conn_calls; mbedtls_ssl_context ssl = {0}; hs_calls = 0;
    int limit = scenario == 3 ? 3 : (scenario == 2 ? 2 : 1), ret = 0;
    for (int i=0;i<limit;++i) {
        errno=41; ret=__wrap_mbedtls_ssl_handshake(&ssl); assert(errno == 57);
    }
    /* Simulate later IDF timeout/cleanup overwriting errno. Captured BIO/HS
     * must survive unchanged and remain bound to this connection. */
    now_us += 12000000; errno=0;
    return ret == 0 ? 1 : ((ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) ? 0 : -1);
}
static athom_pre_tls_diagnostic_t capture(int which, bool cloud, uint32_t sequence)
{
    scenario=which;hs_calls=tx_calls=rx_calls=conn_calls=0;now_us=100000;
    errno=40;bool acquired=athom_pre_tls_diag_begin(cloud,sequence);assert(errno==40 && acquired==cloud);
    int ret=__wrap_esp_tls_conn_new_sync("PRIVATE_HOST",12,443,NULL,NULL);assert(errno==0);
    assert(ret==(which==6 ? 1 : (which==4 || which==5 ? -1 : 0)) && conn_calls==1);
    athom_pre_tls_diagnostic_t d;athom_pre_tls_diag_finish(acquired,&d);assert(errno==0);
    assert(d.valid==cloud);
    if (cloud) {
        assert(d.request_sequence==sequence && d.connection_count==1 && d.handshake.call_count==(unsigned)hs_calls);
        assert(d.handshake.send_calls==(unsigned)tx_calls && d.handshake.recv_calls==(unsigned)rx_calls);
    }
    return d;
}
int main(void)
{
    char json[2400];
    for (int i=0;i<8;++i) {
        athom_pre_tls_diagnostic_t d=capture(i,true,100U+(unsigned)i);
        athom_tls_handshake_diagnostic_t *h=&d.handshake;
        assert(h->valid && h->completed==(i==6));
        if (i==0) assert(h->send_bytes==0 && h->recv_bytes==0 && !h->tx_observed && !h->rx_observed);
        if (i==1) assert(h->send_bytes==17 && h->recv_bytes==0 && h->want_read_count==1 && h->recv_last_ret==MBEDTLS_ERR_SSL_WANT_READ && h->recv_last_errno==EAGAIN);
        if (i==2) assert(h->send_bytes==17 && h->recv_bytes==9 && h->want_read_count==2 && h->first_tx_ms==2 && h->first_rx_ms==5 && h->last_progress_ms==5);
        if (i==3) assert(h->send_bytes==0 && h->send_calls==3 && h->want_write_count==3 && h->send_last_errno==EAGAIN);
        if (i==4) assert(h->peer_closed && h->recv_last_ret==0 && h->last_ret==-0x0050);
        if (i==5) assert(h->last_ret==-0x7200 && h->send_last_ret==-0x0050 && h->send_last_errno==EPIPE && h->want_read_count==0);
        if (i==6) assert(h->completed && h->last_ret==0 && h->state==18 && h->send_bytes==17 && h->recv_bytes==9);
        if (i==7) assert(!h->peer_closed && !h->tx_observed && !h->rx_observed);
        athom_pre_tls_diagnostic_t before=d;int calls=conn_calls+hs_calls+tx_calls+rx_calls;
        assert(athom_pre_tls_diag_json(&d,json,sizeof(json)));
        assert(memcmp(&before,&d,sizeof(d))==0 && calls==conn_calls+hs_calls+tx_calls+rx_calls);
        assert(!strstr(json,"PRIVATE_") && strstr(json,"\"step_count\":null"));
        printf("CASE_%d=%s\n",i,json);
        assert(!athom_pre_tls_diag_json(&d,json,2) && !json[0]);
    }
    athom_pre_tls_diagnostic_t d=capture(2,false,999);
    assert(!d.handshake.valid && athom_pre_tls_diag_json(&d,json,sizeof(json)) && !strcmp(json,"{\"valid\":false}"));
    /* Other task cannot corrupt owning cloud attempt. */
    bool a=athom_pre_tls_diag_begin(true,700);assert(a);task=2;scenario=6;
    __wrap_esp_tls_conn_new_sync("PRIVATE_HOST",12,443,NULL,NULL);task=1;
    athom_pre_tls_diag_finish(a,&d);assert(d.valid && d.request_sequence==700 && !d.handshake.valid);
    /* Two connections in one perform: latest coherent connection only. */
    a=athom_pre_tls_diag_begin(true,701);scenario=6;__wrap_esp_tls_conn_new_sync("PRIVATE_HOST",12,443,NULL,NULL);
    scenario=0;__wrap_esp_tls_conn_new_sync("PRIVATE_HOST",12,443,NULL,NULL);
    athom_pre_tls_diag_finish(a,&d);assert(d.connection_count==2 && d.handshake.call_count==1 && !d.handshake.completed && !d.handshake.tx_observed && !d.handshake.recv_bytes);
    /* Saturation and invalid-state fallback, without generating traffic. */
    assert(saturated_add(UINT32_MAX-2,3)==UINT32_MAX);
    s_diag.handshake.send_calls=s_diag.handshake.send_bytes=UINT32_MAX;
    observe_bio(true,17,17,0);assert(s_diag.handshake.send_calls==UINT32_MAX && s_diag.handshake.send_bytes==UINT32_MAX);
    assert(!strcmp(handshake_state_name(UINT32_MAX),"unknown"));
    mbedtls_ssl_context ssl={.private_state=999};assert(handshake_state(&ssl)==0);
    assert(handshake_state(NULL)==0);
    d.handshake=(athom_tls_handshake_diagnostic_t){0};
    assert(athom_pre_tls_diag_json(&d,json,sizeof(json)) && strstr(json,"\"handshake\":{\"valid\":false}"));
    puts("PATCH066_HANDSHAKE_PROGRESS=PASS");
}
