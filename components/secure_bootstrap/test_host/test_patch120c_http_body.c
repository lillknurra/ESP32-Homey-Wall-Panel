#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "panel_wifi_http_body.h"

typedef struct {
    const char *input;
    size_t input_length;
    size_t offset;
    size_t chunk_limit;
    unsigned calls;
    unsigned eof_after;
    bool timeout;
    int64_t now;
} fake_request_t;

static int fake_recv(void *context, char *buffer, size_t length)
{
    fake_request_t *request = context;
    request->calls++;
    if (request->timeout) return -1;
    if (request->eof_after != 0U && request->calls >= request->eof_after) return 0;
    if (request->offset >= request->input_length) return 0;
    size_t count = request->input_length - request->offset;
    if (count > length) count = length;
    if (request->chunk_limit != 0U && count > request->chunk_limit)
        count = request->chunk_limit;
    memcpy(buffer, request->input + request->offset, count);
    request->offset += count;
    request->now += 100000;
    return (int)count;
}

static int64_t fake_now(void *context)
{
    return ((fake_request_t *)context)->now;
}

static void test_complete_and_fragmented(void)
{
    static const char form[] = "code=123456&ssid=Net&password=longsecret";
    char body[128];
    fake_request_t request = {
        .input = form, .input_length = sizeof(form) - 1U, .chunk_limit = 0U,
    };
    assert(panel_wifi_http_read_complete_body(body, sizeof(body),
        sizeof(form) - 1U, 10000000, fake_recv, fake_now, &request));
    assert(strcmp(body, form) == 0 && request.calls == 1U);

    memset(&request, 0, sizeof(request));
    request.input = form;
    request.input_length = sizeof(form) - 1U;
    request.chunk_limit = 3U;
    assert(panel_wifi_http_read_complete_body(body, sizeof(body),
        sizeof(form) - 1U, 10000000, fake_recv, fake_now, &request));
    assert(strcmp(body, form) == 0 && request.calls > 1U);
}

static void test_incomplete_eof_timeout_and_bounds(void)
{
    static const char form[] = "code=123456&ssid=Net&password=longsecret";
    char body[128];
    memset(body, 'X', sizeof(body));
    fake_request_t request = {
        .input = form, .input_length = sizeof(form) - 1U,
        .chunk_limit = 5U, .eof_after = 3U,
    };
    assert(!panel_wifi_http_read_complete_body(body, sizeof(body),
        sizeof(form) - 1U, 10000000, fake_recv, fake_now, &request));
    assert(body[0] == '\0' && body[sizeof(body) - 1U] == '\0');

    memset(&request, 0, sizeof(request));
    request.input = form;
    request.input_length = sizeof(form) - 1U;
    request.timeout = true;
    assert(!panel_wifi_http_read_complete_body(body, sizeof(body),
        sizeof(form) - 1U, 10000000, fake_recv, fake_now, &request));
    assert(body[0] == '\0');

    memset(&request, 0, sizeof(request));
    request.input = form;
    request.input_length = sizeof(form) - 1U;
    request.chunk_limit = 1U;
    request.now = 0;
    /* A slow fragmented peer crosses the total deadline and is rejected. */
    assert(!panel_wifi_http_read_complete_body(body, sizeof(body),
        sizeof(form) - 1U, 300000, fake_recv, fake_now, &request));
    assert(body[0] == '\0');

    memset(&request, 0, sizeof(request));
    request.input = form;
    request.input_length = sizeof(form) - 1U;
    assert(!panel_wifi_http_read_complete_body(body, sizeof(body),
        sizeof(form) - 1U, 50000, fake_recv, fake_now, &request));
    assert(body[0] == '\0');

    assert(!panel_wifi_http_read_complete_body(body, sizeof(body),
        sizeof(form) - 1U, 10000000, fake_recv, fake_now, &request));
    assert(!panel_wifi_http_read_complete_body(body, sizeof(body),
        sizeof(body), 10000000, fake_recv, fake_now, &request));
}

int main(void)
{
    test_complete_and_fragmented();
    test_incomplete_eof_timeout_and_bounds();
    puts("PASS: Patch120C bounded complete HTTP body reader");
    return 0;
}
