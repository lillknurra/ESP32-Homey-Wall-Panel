#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATHOM_OAUTH_STATE_BYTES 32U
#define ATHOM_OAUTH_STATE_TEXT_MAX 44U
#define ATHOM_OAUTH_CODE_MAX 512U
#define ATHOM_OAUTH_SESSION_TTL_SECONDS 600

typedef enum {
    ATHOM_OAUTH_OK = 0,
    ATHOM_OAUTH_ERR_ARGUMENT,
    ATHOM_OAUTH_ERR_STATE,
    ATHOM_OAUTH_ERR_EXPIRED,
    ATHOM_OAUTH_ERR_MISMATCH,
    ATHOM_OAUTH_ERR_CONSUMED,
    ATHOM_OAUTH_ERR_MISSING_CODE,
    ATHOM_OAUTH_ERR_MISSING_STATE,
} athom_oauth_result_t;

typedef struct {
    uint8_t state[ATHOM_OAUTH_STATE_BYTES];
    int64_t issued_at_s;
    bool active;
    bool consumed;
} athom_oauth_session_t;

void athom_oauth_session_reset(athom_oauth_session_t *session);
athom_oauth_result_t athom_oauth_session_begin(
    athom_oauth_session_t *session,
    int64_t now_s,
    const uint8_t random_state[ATHOM_OAUTH_STATE_BYTES]);
athom_oauth_result_t athom_oauth_session_consume(
    athom_oauth_session_t *session,
    int64_t now_s,
    const uint8_t candidate[ATHOM_OAUTH_STATE_BYTES],
    bool code_present);
athom_oauth_result_t athom_oauth_callback_consume(
    athom_oauth_session_t *session,
    int64_t now_s,
    const char *state_text,
    bool code_present);
bool athom_oauth_state_encode(
    const uint8_t state[ATHOM_OAUTH_STATE_BYTES],
    char out[ATHOM_OAUTH_STATE_TEXT_MAX]);
bool athom_oauth_state_decode(
    const char *text,
    uint8_t out[ATHOM_OAUTH_STATE_BYTES]);
bool athom_oauth_status_json(
    char *out,
    size_t capacity,
    bool mdns_ready,
    bool client_configured,
    bool oauth_pending,
    bool callback_received,
    athom_oauth_result_t last_result);
