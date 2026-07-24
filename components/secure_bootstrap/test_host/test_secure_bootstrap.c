#include "secure_bootstrap.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void fill_random(uint8_t *buffer, uint8_t start)
{
    for (size_t i = 0U; i < SECURE_BOOTSTRAP_CODE_LEN; ++i) {
        buffer[i] = (uint8_t)(start + i);
    }
}

static void test_code_lifecycle(void)
{
    uint8_t first_random[SECURE_BOOTSTRAP_CODE_LEN];
    fill_random(first_random, 1U);
    secure_bootstrap_code_t state = {0};

    assert(secure_bootstrap_code_generate(
               &state,
               100,
               first_random,
               sizeof(first_random)) == SECURE_BOOTSTRAP_OK);
    assert(strlen(state.code) == SECURE_BOOTSTRAP_CODE_LEN);
    char first_code[SECURE_BOOTSTRAP_CODE_LEN + 1];
    memcpy(first_code, state.code, sizeof(first_code));

    assert(secure_bootstrap_code_verify_and_consume(
               &state,
               "WRONGCODE234",
               101) == SECURE_BOOTSTRAP_ERR_MISMATCH);
    assert(secure_bootstrap_code_verify_and_consume(
               &state,
               first_code,
               100 + SECURE_BOOTSTRAP_CODE_TTL_SECONDS - 1) == SECURE_BOOTSTRAP_OK);
    assert(secure_bootstrap_code_verify_and_consume(
               &state,
               first_code,
               101) == SECURE_BOOTSTRAP_ERR_CONSUMED);
}

static void test_expiry_and_runtime_rotation(void)
{
    uint8_t first_random[SECURE_BOOTSTRAP_CODE_LEN];
    uint8_t second_random[SECURE_BOOTSTRAP_CODE_LEN];
    fill_random(first_random, 3U);
    fill_random(second_random, 97U);
    secure_bootstrap_code_t state = {0};

    assert(secure_bootstrap_code_generate(
               &state,
               200,
               first_random,
               sizeof(first_random)) == SECURE_BOOTSTRAP_OK);
    char old_code[SECURE_BOOTSTRAP_CODE_LEN + 1];
    memcpy(old_code, state.code, sizeof(old_code));

    assert(!secure_bootstrap_code_rotation_due(
        &state,
        200 + SECURE_BOOTSTRAP_CODE_TTL_SECONDS - 1));
    assert(secure_bootstrap_code_rotation_due(
        &state,
        200 + SECURE_BOOTSTRAP_CODE_TTL_SECONDS));
    assert(secure_bootstrap_code_rotation_due(
        &state,
        200 + SECURE_BOOTSTRAP_CODE_TTL_SECONDS + 1));

    assert(secure_bootstrap_code_generate(
               &state,
               200 + SECURE_BOOTSTRAP_CODE_TTL_SECONDS + 1,
               second_random,
               sizeof(second_random)) == SECURE_BOOTSTRAP_OK);
    assert(strcmp(old_code, state.code) != 0);
    assert(secure_bootstrap_code_verify_and_consume(
               &state,
               old_code,
               802) == SECURE_BOOTSTRAP_ERR_MISMATCH);

    char new_code[SECURE_BOOTSTRAP_CODE_LEN + 1];
    memcpy(new_code, state.code, sizeof(new_code));
    assert(secure_bootstrap_code_verify_and_consume(
               &state,
               new_code,
               802) == SECURE_BOOTSTRAP_OK);

    assert(secure_bootstrap_code_generate(
               &state,
               1000,
               first_random,
               sizeof(first_random)) == SECURE_BOOTSTRAP_OK);
    assert(secure_bootstrap_code_verify_and_consume(
               &state,
               state.code,
               1000 + SECURE_BOOTSTRAP_CODE_TTL_SECONDS + 1) ==
           SECURE_BOOTSTRAP_ERR_EXPIRED);
    assert(state.code[0] == '\0');
}

static void test_wipe_during_rotation_window(void)
{
    uint8_t random_bytes[SECURE_BOOTSTRAP_CODE_LEN];
    fill_random(random_bytes, 11U);
    secure_bootstrap_code_t state = {0};
    assert(secure_bootstrap_code_generate(
               &state,
               10,
               random_bytes,
               sizeof(random_bytes)) == SECURE_BOOTSTRAP_OK);
    assert(secure_bootstrap_code_rotation_due(
        &state,
        10 + SECURE_BOOTSTRAP_CODE_TTL_SECONDS + 1));
    secure_bootstrap_code_wipe(&state);
    assert(state.code[0] == '\0');
    assert(!secure_bootstrap_code_rotation_due(
        &state,
        10 + SECURE_BOOTSTRAP_CODE_TTL_SECONDS + 2));
}

static void test_wipe_tracker(void)
{
    secure_bootstrap_wipe_tracker_t tracker = {0};
    secure_bootstrap_wipe_tracker_reset(&tracker);
    assert(!secure_bootstrap_wipe_tracker_update(&tracker, true, 1000));
    assert(!secure_bootstrap_wipe_tracker_update(&tracker, true, 5999));
    assert(secure_bootstrap_wipe_tracker_update(&tracker, true, 6000));

    secure_bootstrap_wipe_tracker_reset(&tracker);
    assert(!secure_bootstrap_wipe_tracker_update(&tracker, true, 1000));
    assert(!secure_bootstrap_wipe_tracker_update(&tracker, false, 1200));
    assert(!secure_bootstrap_wipe_tracker_update(&tracker, true, 1300));
    assert(!secure_bootstrap_wipe_tracker_update(&tracker, true, 6299));
    assert(secure_bootstrap_wipe_tracker_update(&tracker, true, 6300));
}

static void test_secret_detection(void)
{
    uint8_t random_bytes[SECURE_BOOTSTRAP_CODE_LEN];
    fill_random(random_bytes, 7U);
    secure_bootstrap_code_t state = {0};
    assert(secure_bootstrap_code_generate(
               &state,
               1,
               random_bytes,
               sizeof(random_bytes)) == SECURE_BOOTSTRAP_OK);
    assert(!secure_bootstrap_text_contains_code("sanitized evidence", &state));

    char leaked[64];
    int written = snprintf(leaked, sizeof(leaked), "secret=%s", state.code);
    assert(written > 0 && (size_t)written < sizeof(leaked));
    assert(secure_bootstrap_text_contains_code(leaked, &state));
}

int main(void)
{
    test_code_lifecycle();
    test_expiry_and_runtime_rotation();
    test_wipe_during_rotation_window();
    test_wipe_tracker();
    test_secret_detection();
    puts("PASS: secure local bootstrap runtime-closure host tests");
    return 0;
}
