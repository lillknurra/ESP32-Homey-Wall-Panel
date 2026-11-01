#include <assert.h>
#include <stddef.h>
#include "homey_registration_sequence.h"

typedef struct { size_t used, capacity, completed, attempts; } fake_httpd_t;

static int fake_register(void *opaque, size_t index)
{
    fake_httpd_t *server = opaque;
    (void)index;
    server->attempts++;
    if (server->used == server->capacity) return 0xB001; /* ESP-IDF v6 HANDLERS_FULL */
    server->used++;
    return 0;
}

static int fake_complete(void *opaque)
{
    fake_httpd_t *server = opaque;
    server->completed++;
    return 0;
}

int main(void)
{
    fake_httpd_t full = {.used = 14U, .capacity = 24U};
    assert(homey_registration_sequence_run(11U, fake_register, fake_complete,
                                           &full) == 0xB001);
    assert(full.attempts == 11U);
    assert(full.used == 24U);
    assert(full.completed == 0U); /* no queue/worker/auth continuation */

    fake_httpd_t repaired = {.used = 14U, .capacity = 26U};
    assert(homey_registration_sequence_run(11U, fake_register, fake_complete,
                                           &repaired) == 0);
    assert(repaired.attempts == 11U && repaired.used == 25U);
    assert(repaired.completed == 1U); /* runtime startup continuation reached */

    fake_httpd_t failure = {.used = 14U, .capacity = 26U};
    assert(homey_registration_sequence_run(11U, fake_register, NULL,
                                           &failure) == -1);
    assert(failure.completed == 0U);
    return 0;
}
