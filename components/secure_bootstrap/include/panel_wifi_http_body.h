#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int (*panel_wifi_http_recv_fn)(void *context, char *buffer, size_t length);
typedef int64_t (*panel_wifi_http_now_us_fn)(void *context);

bool panel_wifi_http_read_complete_body(
    char *body,
    size_t body_capacity,
    size_t content_length,
    int64_t timeout_us,
    panel_wifi_http_recv_fn receive,
    panel_wifi_http_now_us_fn now_us,
    void *context);
