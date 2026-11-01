#pragma once

#include <stddef.h>

typedef int (*homey_registration_step_fn)(void *context, size_t index);
typedef int (*homey_registration_complete_fn)(void *context);

/* Runs the ordered registration path used by the Homey runtime. */
int homey_registration_sequence_run(
    size_t registration_count,
    homey_registration_step_fn register_step,
    homey_registration_complete_fn on_complete,
    void *context);
