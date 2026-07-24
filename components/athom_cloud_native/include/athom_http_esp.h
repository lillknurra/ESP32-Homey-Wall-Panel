#pragma once
#include "athom_cloud_types.h"
#include "athom_http_transport.h"
#include <stdbool.h>
#include <stddef.h>

#define ATHOM_HTTP_MAX_RESPONSE_BYTES (64U * 1024U)
#define ATHOM_HTTP_MAX_URL_BYTES 512U
#define ATHOM_HTTP_MAX_FORM_BYTES 4096U

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    int status_code;
    bool tls_verified;
} athom_http_response_t;

typedef athom_status_t (*athom_http_executor_fn)(
    void *executor_context,
    const char *method,
    const char *url,
    const char *authorization,
    const char *content_type,
    const char *body,
    athom_http_response_t *response);

athom_status_t athom_http_esp_init(athom_http_transport_t *transport);
athom_status_t athom_http_esp_set_executor(
    athom_http_transport_t *transport,
    athom_http_executor_fn executor,
    void *executor_context);
athom_status_t athom_url_encode(
    const char *input, char *output, size_t output_size);
athom_status_t athom_http_validate_response_size(size_t current, size_t incoming);
bool athom_http_url_is_official(const char *url);
void athom_http_response_free(athom_http_response_t *response);
