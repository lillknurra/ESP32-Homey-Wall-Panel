#include "secure_bootstrap.h"

#include <string.h>

static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

secure_bootstrap_status_t secure_bootstrap_code_generate(
    secure_bootstrap_code_t *state,
    int64_t now_s,
    const uint8_t *random_bytes,
    size_t random_len)
{
    if (state == NULL || random_bytes == NULL ||
        random_len < SECURE_BOOTSTRAP_CODE_LEN || now_s < 0) {
        return SECURE_BOOTSTRAP_ERR_ARGUMENT;
    }

    secure_bootstrap_code_wipe(state);
    for (size_t i = 0U; i < SECURE_BOOTSTRAP_CODE_LEN; ++i) {
        state->code[i] = alphabet[random_bytes[i] % (sizeof(alphabet) - 1U)];
    }
    state->code[SECURE_BOOTSTRAP_CODE_LEN] = '\0';
    state->issued_at_s = now_s;
    state->consumed = false;
    return SECURE_BOOTSTRAP_OK;
}

secure_bootstrap_status_t secure_bootstrap_code_verify_and_consume(
    secure_bootstrap_code_t *state,
    const char *candidate,
    int64_t now_s)
{
    if (state == NULL || candidate == NULL || now_s < 0) {
        return SECURE_BOOTSTRAP_ERR_ARGUMENT;
    }
    if (state->consumed) {
        return SECURE_BOOTSTRAP_ERR_CONSUMED;
    }
    if (now_s < state->issued_at_s ||
        now_s - state->issued_at_s >= SECURE_BOOTSTRAP_CODE_TTL_SECONDS) {
        secure_bootstrap_code_wipe(state);
        state->consumed = true;
        return SECURE_BOOTSTRAP_ERR_EXPIRED;
    }

    unsigned diff = 0U;
    size_t candidate_len = strlen(candidate);
    diff |= (unsigned)(candidate_len ^ SECURE_BOOTSTRAP_CODE_LEN);
    for (size_t i = 0U; i < SECURE_BOOTSTRAP_CODE_LEN; ++i) {
        unsigned char value =
            i < candidate_len ? (unsigned char)candidate[i] : 0U;
        diff |= (unsigned)(value ^ (unsigned char)state->code[i]);
    }
    if (diff != 0U) {
        return SECURE_BOOTSTRAP_ERR_MISMATCH;
    }

    state->consumed = true;
    return SECURE_BOOTSTRAP_OK;
}

bool secure_bootstrap_code_rotation_due(
    const secure_bootstrap_code_t *state,
    int64_t now_s)
{
    if (state == NULL || state->code[0] == '\0' || state->consumed ||
        now_s < state->issued_at_s) {
        return false;
    }
    return now_s - state->issued_at_s >= SECURE_BOOTSTRAP_CODE_TTL_SECONDS;
}

void secure_bootstrap_code_wipe(secure_bootstrap_code_t *state)
{
    if (state == NULL) {
        return;
    }

    volatile unsigned char *p = (volatile unsigned char *)state->code;
    for (size_t i = 0U; i < sizeof(state->code); ++i) {
        p[i] = 0U;
    }
    state->issued_at_s = 0;
    state->consumed = false;
}

bool secure_bootstrap_text_contains_code(
    const char *text,
    const secure_bootstrap_code_t *state)
{
    if (text == NULL || state == NULL || state->code[0] == '\0') {
        return false;
    }
    return strstr(text, state->code) != NULL;
}

void secure_bootstrap_wipe_tracker_reset(secure_bootstrap_wipe_tracker_t *tracker)
{
    if (tracker == NULL) {
        return;
    }
    tracker->pressed_since_ms = 0;
    tracker->triggered = false;
}

bool secure_bootstrap_wipe_tracker_update(
    secure_bootstrap_wipe_tracker_t *tracker,
    bool pressed,
    int64_t now_ms)
{
    if (tracker == NULL || now_ms < 0) {
        return false;
    }
    if (!pressed) {
        tracker->pressed_since_ms = 0;
        tracker->triggered = false;
        return false;
    }
    if (tracker->triggered) {
        return true;
    }
    if (tracker->pressed_since_ms == 0) {
        tracker->pressed_since_ms = now_ms;
        return false;
    }
    if (now_ms - tracker->pressed_since_ms >= SECURE_BOOTSTRAP_WIPE_HOLD_MS) {
        tracker->triggered = true;
    }
    return tracker->triggered;
}
