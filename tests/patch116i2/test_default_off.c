#include "panel_update_worker.h"
#include <assert.h>
int main(void) {
    assert(!panel_update_worker_bootstrap());
    assert(!panel_update_worker_request(1, "stable", true, true));
    uint32_t gen = 4;
    char body[PANEL_UPDATE_MANIFEST_MAX_BYTES+1];
    size_t size = 10;
    assert(panel_update_worker_poll(&gen, body, sizeof(body), &size) == PANEL_UPDATE_POLL_DISABLED);
    assert(gen == 0U && size == 0U);
    return 0;
}
