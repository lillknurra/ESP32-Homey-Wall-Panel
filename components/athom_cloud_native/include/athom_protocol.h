#pragma once
#include "athom_cloud_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATHOM_PROTOCOL_MAX_JSON_BYTES (64U * 1024U)
#define ATHOM_MAX_REMOTE_URL 384
#define ATHOM_MAX_SESSION_TOKEN 1024
#define ATHOM_MAX_DELEGATION_TOKEN 2048

typedef struct {
    char remote_url[ATHOM_MAX_REMOTE_URL];
} athom_homey_connection_t;

athom_status_t athom_parse_token_json(
    const char *json, size_t length, int64_t now_epoch_s,
    athom_credentials_t *updated);
athom_status_t athom_parse_user_homeys_json(
    const char *json, size_t length, athom_homey_list_t *out,
    athom_homey_connection_t *connections, size_t connection_capacity);
athom_status_t athom_parse_json_string(
    const char *json, size_t length, char *out, size_t out_size);
athom_status_t athom_count_top_level_members(
    const char *json, size_t length, size_t *out_count);
athom_status_t athom_pseudonymize(
    const char *value, char *out, size_t out_size);
bool athom_text_is_sanitized(const char *text);
