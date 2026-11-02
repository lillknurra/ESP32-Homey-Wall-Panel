#include "homey_registration_sequence.h"

int homey_registration_sequence_run(
    size_t registration_count,
    homey_registration_step_fn register_step,
    homey_registration_complete_fn on_complete,
    void *context)
{
    if (register_step == 0 || on_complete == 0) return -1;
    for (size_t i = 0; i < registration_count; ++i) {
        const int result = register_step(context, i);
        if (result != 0) return result;
    }
    return on_complete(context);
}
