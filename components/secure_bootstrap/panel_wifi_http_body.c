#include "panel_wifi_http_body.h"

#include <string.h>

static void clear_body(char *body, size_t capacity)
{
    volatile unsigned char *bytes = (volatile unsigned char *)body;
    while (capacity-- > 0U) *bytes++ = 0U;
}

bool panel_wifi_http_read_complete_body(
    char *body,
    size_t body_capacity,
    size_t content_length,
    int64_t timeout_us,
    panel_wifi_http_recv_fn receive,
    panel_wifi_http_now_us_fn now_us,
    void *context)
{
    if (body == NULL || body_capacity == 0U) return false;
    clear_body(body, body_capacity);
    if (content_length == 0U || content_length >= body_capacity ||
        timeout_us <= 0 || receive == NULL || now_us == NULL) return false;

    const int64_t started_us = now_us(context);
    if (started_us < 0) return false;
    size_t received = 0U;
    while (received < content_length) {
        const int64_t current_us = now_us(context);
        if (current_us < started_us || current_us - started_us >= timeout_us) {
            clear_body(body, body_capacity);
            return false;
        }
        const size_t remaining = content_length - received;
        const int count = receive(context, body + received, remaining);
        if (count <= 0 || (size_t)count > remaining) {
            clear_body(body, body_capacity);
            return false;
        }
        received += (size_t)count;
        body[received] = '\0';
    }
    const int64_t completed_us = now_us(context);
    if (completed_us < started_us || completed_us - started_us >= timeout_us) {
        clear_body(body, body_capacity);
        return false;
    }
    return true;
}
