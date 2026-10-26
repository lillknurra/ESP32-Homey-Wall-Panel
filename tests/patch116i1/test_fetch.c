#include "panel_update_https_fetch.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_timer.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct fake_http_client { int placeholder; };
static struct fake_http_client h;
static int64_t clock_us;
static int64_t advance_open_us;
static int64_t advance_header_us;
static int64_t advance_read_us;
static int http_status;
static int64_t content_length;
static bool is_chunked;
static bool incomplete;
static bool init_failure;
static bool open_failure;
static bool method_failure;
static bool header_failure;
static int read_error;
static int eagain_left;
static size_t mock_total_read;
static size_t init_calls;
static size_t read_calls;
static size_t close_calls;
static size_t cleanup_calls;
static size_t complete_calls;
static bool last_verified;
static size_t last_body_size;
static char captured_url[256];

static void reset(void)
{
    clock_us = 1000000LL;
    advance_open_us = advance_header_us = advance_read_us = 0;
    http_status = 200;
    content_length = 2;
    is_chunked = incomplete = init_failure = open_failure = method_failure = header_failure = false;
    read_error = eagain_left = 0;
    mock_total_read = init_calls = read_calls = close_calls = cleanup_calls = complete_calls = 0;
    last_verified = false;
    last_body_size = 0;
    memset(captured_url, 0, sizeof(captured_url));
}

int64_t esp_timer_get_time(void) { return clock_us; }
esp_err_t esp_crt_bundle_attach(void *c) { (void)c; return ESP_OK; }

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *cfg)
{
    init_calls++;
    assert(cfg && cfg->url && cfg->crt_bundle_attach == esp_crt_bundle_attach);
    assert(cfg->disable_auto_redirect && !cfg->keep_alive_enable);
    assert(cfg->buffer_size == 512 && cfg->timeout_ms == 1500);
    strncpy(captured_url, cfg->url, sizeof(captured_url)-1);
    return init_failure ? NULL : &h;
}
esp_err_t esp_http_client_set_method(esp_http_client_handle_t c, esp_http_client_method_t method)
{ assert(c == &h && method == HTTP_METHOD_GET); return method_failure ? ESP_FAIL : ESP_OK; }
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c,const char *n,const char *v)
{ assert(c == &h && n && v); return header_failure ? ESP_FAIL : ESP_OK; }
esp_err_t esp_http_client_open(esp_http_client_handle_t c,int write_len)
{ assert(c == &h && write_len == 0); clock_us += advance_open_us; return open_failure ? ESP_FAIL : ESP_OK; }
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c)
{ assert(c == &h); clock_us += advance_header_us; return content_length; }
int esp_http_client_get_status_code(esp_http_client_handle_t c)
{ assert(c == &h); return http_status; }
bool esp_http_client_is_chunked_response(esp_http_client_handle_t c)
{ assert(c == &h); return is_chunked; }
int esp_http_client_read(esp_http_client_handle_t c, char *buffer,int len)
{
    assert(c == &h && buffer && len > 0 && len <= 256);
    read_calls++;
    clock_us += advance_read_us;
    if (eagain_left > 0) { eagain_left--; return -ESP_ERR_HTTP_EAGAIN; }
    if (read_error) return read_error;
    int bytes = len > 1 ? 1 : len;
    memset(buffer, (mock_total_read++ % 2) == 0 ? '{' : '}', (size_t)bytes);
    return bytes;
}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t c)
{ assert(c == &h); return !incomplete; }
esp_err_t esp_http_client_close(esp_http_client_handle_t c)
{ assert(c == &h); close_calls++; return ESP_OK; }
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c)
{ assert(c == &h); cleanup_calls++; return ESP_OK; }

bool panel_update_check_complete(panel_update_check_t *s,uint32_t generation,uint64_t now_ms,bool verified,const char *body,size_t length,const panel_update_local_t *local)
{
    assert(s && local && generation == s->generation && now_ms >= 1000);
    complete_calls++;
    last_verified = verified;
    last_body_size = length;
    assert((verified && body) || (!verified && !body));
    s->in_flight = false;
    return true;
}

static void assert_cleanup(void)
{ assert(init_calls == 1 && cleanup_calls == 1); assert(close_calls <= 1); }

int main(void)
{
    char body[PANEL_UPDATE_MANIFEST_MAX_BYTES+1] = {0};
    size_t n = 55;
    reset();
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_OK);
    assert(n == 2 && !memcmp(body,"{}",2));
    assert(strstr(captured_url,"/updates/stable/") != NULL);
    assert(close_calls == 1); assert_cleanup();

    reset();
    assert(panel_update_https_fetch("beta",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_OK);
    assert(strstr(captured_url,"/updates/beta/") != NULL); assert_cleanup();

    reset();
    assert(panel_update_https_fetch("preview",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(n == 0 && init_calls == 0);
    assert(panel_update_https_fetch(NULL,body,sizeof(body),&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(panel_update_https_fetch("stable",body,2,&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(panel_update_https_fetch("stable",NULL,sizeof(body),&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(panel_update_https_fetch("stable",body,sizeof(body),NULL) == PANEL_UPDATE_FETCH_REJECTED);
    assert(init_calls == 0);

    reset(); http_status=301;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(n == 0 && read_calls == 0); assert_cleanup();

    reset(); http_status=304;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(n == 0 && read_calls == 0); assert_cleanup();

    reset(); is_chunked=true;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(n == 0 && read_calls == 0); assert_cleanup();

    reset(); content_length=PANEL_UPDATE_MANIFEST_MAX_BYTES+1;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(n == 0 && read_calls == 0); assert_cleanup();

    reset(); content_length=0;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(n == 0 && read_calls == 0); assert_cleanup();

    reset(); content_length=-ESP_ERR_HTTP_EAGAIN;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_REJECTED);
    assert(n == 0); assert_cleanup();

    reset(); incomplete=true;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_INCOMPLETE);
    assert(n == 0 && body[0] == 0); assert_cleanup();

    reset(); read_error=-5;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_INCOMPLETE);
    assert(n == 0 && body[0] == 0); assert_cleanup();

    reset(); eagain_left=4;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_TIMEOUT);
    assert(n == 0 && read_calls == 4); assert_cleanup();

    reset(); eagain_left=3;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_OK);
    assert(n == 2 && read_calls == 5); assert_cleanup();

    reset(); advance_open_us=31000000LL;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_TIMEOUT);
    assert(n == 0 && read_calls == 0); assert_cleanup();

    reset(); advance_header_us=31000000LL;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_TIMEOUT);
    assert(n == 0 && read_calls == 0); assert_cleanup();

    reset(); advance_read_us=31000000LL;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_TIMEOUT);
    assert(n == 0 && body[0] == 0); assert_cleanup();

    reset(); init_failure=true;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_NETWORK_ERROR);
    assert(n == 0 && init_calls == 1 && cleanup_calls == 0);

    reset(); open_failure=true;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_NETWORK_ERROR);
    assert(n == 0 && close_calls == 0); assert_cleanup();

    reset(); method_failure=true;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_NETWORK_ERROR);
    assert(n == 0); assert_cleanup();

    reset(); header_failure=true;
    assert(panel_update_https_fetch("stable",body,sizeof(body),&n) == PANEL_UPDATE_FETCH_NETWORK_ERROR);
    assert(n == 0); assert_cleanup();

    reset();
    panel_update_check_t state = {.in_flight=true,.generation=8};
    panel_update_local_t local = {.channel="stable"};
    assert(panel_update_https_fetch_and_complete(&state,8,&local) == PANEL_UPDATE_FETCH_OK);
    assert(complete_calls == 1 && last_verified && last_body_size == 2 && !state.in_flight);
    assert_cleanup();

    reset(); state.in_flight = true; state.generation=9; http_status=404;
    assert(panel_update_https_fetch_and_complete(&state,9,&local) == PANEL_UPDATE_FETCH_REJECTED);
    assert(complete_calls == 1 && !last_verified && last_body_size == 0 && !state.in_flight);
    assert_cleanup();

    reset(); state.in_flight=true; state.generation=9;
    assert(panel_update_https_fetch_and_complete(&state,8,&local) == PANEL_UPDATE_FETCH_REJECTED);
    assert(complete_calls==0 && init_calls==0);

    puts("PATCH116I1_HTTPS_FETCH_HOST_MOCK=PASS");
    return 0;
}
