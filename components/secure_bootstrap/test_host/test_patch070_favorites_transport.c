#include "athom_favorites_transport_diag.h"
#include "test_patch070_platform.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

_Thread_local TaskHandle_t test_task = (TaskHandle_t)(uintptr_t)1U;
int64_t test_clock_us;
static int next_read, next_write = 32, next_connect, next_errno;
static int getter_value, connect_count, write_count, read_count, getter_count;
static const char *read_content;
static char transport_identity;
static int status_callback_count, last_parser_status;
typedef int esp_err_t;
typedef enum { HTTP_METHOD_GET } esp_http_client_method_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_SIZE 0x104
#define ATHOM_HOMEY_URL_MAX 256U
#define ATHOM_TOKEN_MAX 256U
#define HTTP_BODY_MAX 65536U
static struct { athom_favorites_fetch_diagnostic_t last_favorites_fetch; } s_transport_metrics;
static unsigned favorites_request_count;
static bool request_fixture_success;
static void zero_secure(void *p, size_t size) { memset(p, 0, size); }
static esp_err_t bearer_authorization(const char *token, char *out, size_t size)
{
    int n=snprintf(out,size,"Bearer %s",token);
    return n>0 && (size_t)n<size ? ESP_OK : ESP_ERR_INVALID_SIZE;
}
static esp_transport_handle_t transport(void) { return &transport_identity; }

int __wrap_esp_transport_connect(esp_transport_handle_t, const char *, int, int);
int __wrap_esp_transport_write(esp_transport_handle_t, const char *, int, int);
int __wrap_esp_transport_read(esp_transport_handle_t, char *, int, int);
int __wrap_esp_transport_get_errno(esp_transport_handle_t);
size_t __wrap_http_parser_execute(http_parser *, const http_parser_settings *, const char *, size_t);

int __real_esp_transport_connect(esp_transport_handle_t t, const char *host, int port, int timeout)
{
    assert(t == transport() && host != NULL && port == 443 && timeout == 8000);
    ++connect_count; errno = next_errno; return next_connect;
}
int __real_esp_transport_write(esp_transport_handle_t t, const char *data, int len, int timeout)
{
    assert(t == transport() && data != NULL && len > 0 && timeout == 8000);
    ++write_count; errno = next_errno; return next_write;
}
int __real_esp_transport_read(esp_transport_handle_t t, char *data, int len, int timeout)
{
    assert(t == transport() && data != NULL && len > 0 && timeout == 8000);
    ++read_count;
    if (next_read > 0) {
        assert(read_content != NULL && next_read <= len);
        memcpy(data, read_content, (size_t)next_read);
    }
    errno = next_errno; return next_read;
}
int __real_esp_transport_get_errno(esp_transport_handle_t t)
{
    assert(t == transport()); ++getter_count;
    int value = getter_value; getter_value = 0; return value;
}
size_t __real_http_parser_execute(http_parser *p, const http_parser_settings *s,
                                 const char *data, size_t size)
{
    return http_parser_execute(p, s, data, size);
}

/* PATCH070_PRODUCTION_JSON_HELPERS */

static int headers_complete(http_parser *parser)
{
    (void)parser;
    athom_favorites_transport_diag_event(ATHOM_FAVORITES_RESPONSE_HEADERS_COMPLETE);
    return 0;
}
static int status_seen(http_parser *parser, const char *data, size_t size)
{
    (void)parser; (void)data; (void)size;
    ++status_callback_count; return 0;
}

static esp_err_t http_request_limited(
    const char *url, esp_http_client_method_t method, const char *authorization,
    const char *content_type, const char *body, char **response_out,
    int *status_out, size_t maximum, size_t *capacity_out)
{
    assert(strcmp(url,"https://synthetic.invalid/selected/api/manager/users/user/me")==0);
    assert(method==HTTP_METHOD_GET && strcmp(authorization,"Bearer synthetic-secret")==0);
    assert(content_type==NULL && body==NULL && maximum==HTTP_BODY_MAX);
    ++favorites_request_count;
    athom_favorites_transport_diag_client_reused();
    next_errno=0; next_write=32;
    assert(__wrap_esp_transport_write(transport(),"synthetic-private-header",24,8000)==32);
    athom_favorites_transport_diag_event(ATHOM_FAVORITES_REQUEST_HEADERS_SENT);
    test_clock_us+=1000;
    char buffer[256];
    if (!request_fixture_success) {
        next_read=-0x7100; next_errno=0;
        assert(__wrap_esp_transport_read(transport(),buffer,sizeof(buffer),8000)==-0x7100);
        athom_favorites_transport_diag_close_result(0);
        return 0x7004;
    }
    read_content="HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    next_read=(int)strlen(read_content);
    assert(__wrap_esp_transport_read(transport(),buffer,sizeof(buffer),8000)==next_read);
    http_parser parser; http_parser_init(&parser,HTTP_RESPONSE);
    http_parser_settings settings={0}; settings.on_headers_complete=headers_complete;
    assert(__wrap_http_parser_execute(&parser,&settings,buffer,(size_t)next_read)==(size_t)next_read);
    *status_out=200; *capacity_out=3; *response_out=malloc(3); assert(*response_out!=NULL);
    memcpy(*response_out,"{}",3);
    return ESP_OK;
}

/* PATCH070_PRODUCTION_FAVORITES_REQUEST */

static athom_favorites_fetch_diagnostic_t run_case(
    bool reuse, const char *bytes, int terminal_result, int terminal_errno, int final_result)
{
    last_parser_status = 0;
    bool acquired = athom_favorites_transport_diag_begin(false);
    assert(acquired);
    next_errno = 0;
    if (reuse) athom_favorites_transport_diag_client_reused();
    else assert(__wrap_esp_transport_connect(transport(), "synthetic.invalid", 443, 8000) == 0);
    const int writes = write_count;
    assert(__wrap_esp_transport_write(transport(), "synthetic-private-header", 24, 8000) == 32);
    assert(write_count == writes + 1);
    athom_favorites_transport_diag_event(ATHOM_FAVORITES_REQUEST_HEADERS_SENT);
    test_clock_us += 1000;
    http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    http_parser_settings settings = {0};
    settings.on_headers_complete = headers_complete;
    settings.on_status = status_seen;
    char buffer[2048];
    if (bytes != NULL) {
        read_content = bytes; next_read = (int)strlen(bytes);
        const int reads = read_count;
        assert(__wrap_esp_transport_read(transport(), buffer, sizeof(buffer), 8000) == next_read);
        assert(read_count == reads + 1);
        size_t parsed = __wrap_http_parser_execute(&parser, &settings, buffer, (size_t)next_read);
        assert(parsed <= (size_t)next_read);
        last_parser_status = parser.status_code;
    }
    if (terminal_result <= 0) {
        next_read = terminal_result; next_errno = terminal_errno;
        assert(__wrap_esp_transport_read(transport(), buffer, sizeof(buffer), 8000) == terminal_result);
        assert(errno == terminal_errno);
        getter_value = terminal_errno;
        const int calls = getter_count;
        assert(__wrap_esp_transport_get_errno(transport()) == terminal_errno);
        assert(getter_value == 0 && getter_count == calls + 1);
        assert(__wrap_esp_transport_get_errno(transport()) == 0);
        athom_favorites_transport_diag_close_result(0);
    }
    test_clock_us += 44000;
    athom_favorites_fetch_diagnostic_t out;
    athom_favorites_transport_diag_finish(acquired, final_result, &out);
    assert(out.valid && out.request_result == final_result);
    return out;
}

static void *other_task(void *ignored)
{
    (void)ignored; test_task = (TaskHandle_t)(uintptr_t)2U;
    assert(!athom_favorites_transport_diag_begin(true));
    char b[64]; next_read = -2; next_errno = ECONNRESET;
    assert(__wrap_esp_transport_read(transport(), b, sizeof(b), 8000) == -2);
    athom_favorites_transport_diag_event(ATHOM_FAVORITES_RESPONSE_HEADERS_COMPLETE);
    athom_favorites_fetch_diagnostic_t denied;
    athom_favorites_transport_diag_finish(false, -1, &denied);
    assert(!denied.valid);
    return NULL;
}

int main(void)
{
    const char *ok = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    athom_favorites_fetch_diagnostic_t d = run_case(false, ok, 1, 0, 0);
    assert(d.complete_header_observed && d.connect_calls == 1);
    assert(d.connection_reuse_known && !d.connection_reused && !d.client_reused);
    assert(d.response_bytes_observed == strlen(ok));
    assert(strcmp(athom_favorites_transport_diag_class(&d), "ok") == 0);
    assert(d.header_wait_elapsed_ms == 1);

    d = run_case(true, NULL, -0x7100, 0, 0x7004);
    assert(d.client_reused && d.connection_reused && d.connect_calls == 0);
    assert(d.header_read_calls == 1 && d.response_bytes_observed == 0);
    assert(d.last_header_read_result == -0x7100 && !d.complete_header_observed);
    assert(strcmp(athom_favorites_transport_diag_class(&d), "header_zero_bytes_error") == 0);
    char json[4096]; size_t offset = 0;
    assert(athom_favorites_fetch_diagnostic_json_append(&d, json, sizeof(json), &offset));
    assert(strstr(json, "synthetic-private-header") == NULL);
    assert(strstr(json, "synthetic.invalid") == NULL);
    printf("PATCH070_JSON_SAMPLE=%s\n", json);
    char before[4096]; strcpy(before,json); offset=0;
    assert(athom_favorites_fetch_diagnostic_json_append(&d,json,sizeof(json),&offset));
    assert(strcmp(before,json)==0); /* Serializing performs no wrapper call. */

    d = run_case(true, NULL, -1, 0, 0x7004);
    assert(d.fin_reported && d.response_bytes_observed == 0);
    assert(strcmp(athom_favorites_transport_diag_class(&d), "header_fin_reported") == 0);
    status_callback_count = 0;
    d = run_case(true, "HTTP/1.", -2, ECONNRESET, 0x7004);
    assert(d.response_bytes_observed == 7 && status_callback_count == 0);
    assert(d.transport_errno == ECONNRESET && d.transport_errno_calls == 2);
    assert(strcmp(athom_favorites_transport_diag_class(&d), "header_partial_error") == 0);
    status_callback_count = 0;
    d = run_case(true, "HTTP/1.1 200\r\nX-Test:", -2, 0, 0x7004);
    assert(last_parser_status == 200 && status_callback_count == 0);
    assert(d.response_bytes_observed > 0 && !d.complete_header_observed);
    /* Complete reason-less status line can exist without the status callback. */
    d = run_case(true, "HTTP/1.1 200 OK\r\nContent-Length:", 0, 0, 0x7007);
    assert(d.timeout_observed && !d.complete_header_observed);
    assert(status_callback_count > 0); /* Status exists before headers finish. */
    assert(strcmp(athom_favorites_transport_diag_class(&d), "header_timeout") == 0);
    d = run_case(true, "INVALID\r\n", -2, 0, 0x7004);
    assert(d.parser_error != 0 && !d.complete_header_observed);
    assert(strcmp(athom_favorites_transport_diag_class(&d), "header_parse_error") == 0);

    /* Real parser header overflow is retained even though IDF ignores execute's result. */
    bool oversized_acquired = athom_favorites_transport_diag_begin(true);
    athom_favorites_transport_diag_event(ATHOM_FAVORITES_REQUEST_HEADERS_SENT);
    http_parser large_parser; http_parser_init(&large_parser, HTTP_RESPONSE);
    http_parser_settings large_settings = {0};
    const char *prefix = "HTTP/1.1 200 OK\r\nX-Long: ";
    (void)__wrap_http_parser_execute(&large_parser, &large_settings, prefix, strlen(prefix));
    char chunk[1024]; memset(chunk, 'a', sizeof(chunk));
    for (size_t i = 0; i < HTTP_MAX_HEADER_SIZE / sizeof(chunk) + 1; ++i) {
        (void)__wrap_http_parser_execute(&large_parser, &large_settings, chunk, sizeof(chunk));
    }
    athom_favorites_transport_diag_finish(oversized_acquired, 0x7004, &d);
    assert(d.parser_error == HPE_HEADER_OVERFLOW && !d.complete_header_observed);

    /* New attempt clears every failure/close/parser field, even on same client. */
    d = run_case(true, ok, 1, 0, 0);
    assert(d.parser_error==0 && d.transport_errno==0 && !d.timeout_observed);
    assert(!d.fin_reported && !d.close_called && d.transport_errno_calls==0);

    bool acquired=athom_favorites_transport_diag_begin(false);
    next_connect=-1; next_errno=EHOSTUNREACH;
    assert(__wrap_esp_transport_connect(transport(),"synthetic.invalid",443,8000)==-1);
    athom_favorites_transport_diag_finish(acquired,0x7002,&d);
    assert(d.connect_calls==1 && d.connect_result==-1 && d.write_calls==0);
    assert(!d.connection_reuse_known);
    assert(strcmp(athom_favorites_transport_diag_class(&d),"before_headers_sent")==0);
    next_connect=0;

    acquired=athom_favorites_transport_diag_begin(true);
    athom_favorites_transport_diag_event(ATHOM_FAVORITES_REQUEST_HEADERS_SENT);
    pthread_t thread; assert(pthread_create(&thread,NULL,other_task,NULL)==0);
    assert(pthread_join(thread,NULL)==0);
    athom_favorites_transport_diag_finish(acquired,0x7004,&d);
    assert(d.valid && d.header_read_calls==0 && !d.complete_header_observed);
    assert(strcmp(athom_favorites_transport_diag_class(&d),"unclassified")==0);
    assert(strcmp(athom_favorites_transport_diag_class(NULL),"not_observed")==0);
    memset(&d,0,sizeof(d)); assert(strcmp(athom_favorites_transport_diag_class(&d),"not_observed")==0);
    d.valid=true; d.request_headers_sent=true; d.complete_header_observed=true; d.request_result=1;
    assert(strcmp(athom_favorites_transport_diag_class(&d),"after_headers_error")==0);
    offset=0; assert(!athom_favorites_fetch_diagnostic_json_append(&d,json,8,&offset));

    /* Actual production entry brackets exactly one request, resets early exits,
     * and does not inherit failed header evidence on the next success. */
    char *response=NULL; size_t capacity=0; int status=0;
    const unsigned requests=favorites_request_count;
    assert(favorites_fetch_user_me("https://synthetic.invalid/selected","synthetic-secret",
        &response,&capacity,&status)==0x7004);
    assert(favorites_request_count==requests+1 && response==NULL && status==0);
    assert(s_transport_metrics.last_favorites_fetch.valid);
    assert(s_transport_metrics.last_favorites_fetch.last_header_read_result==-0x7100);
    request_fixture_success=true;
    assert(favorites_fetch_user_me("https://synthetic.invalid/selected","synthetic-secret",
        &response,&capacity,&status)==0);
    assert(favorites_request_count==requests+2 && response!=NULL && status==200);
    assert(s_transport_metrics.last_favorites_fetch.complete_header_observed);
    assert(s_transport_metrics.last_favorites_fetch.request_result==0);
    assert(!s_transport_metrics.last_favorites_fetch.close_called);
    free(response);
    assert(favorites_fetch_user_me(NULL,"synthetic-secret",&response,&capacity,&status)==ESP_ERR_INVALID_ARG);
    assert(!s_transport_metrics.last_favorites_fetch.valid && favorites_request_count==requests+2);
    puts("PATCH070_FAVORITES_TRANSPORT_HOST_TESTS PASS");
    return 0;
}
