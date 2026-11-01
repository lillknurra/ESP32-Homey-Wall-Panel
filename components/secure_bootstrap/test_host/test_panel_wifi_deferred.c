#include "panel_wifi_deferred.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    panel_wifi_deferred_owner_t owner;
    panel_wifi_deferred_owner_init(&owner);

    assert(panel_wifi_deferred_schedule(&owner, 4U));
    assert(!panel_wifi_deferred_schedule(&owner, 4U));
    assert(!panel_wifi_deferred_begin(&owner, 4U, 5U, false));
    assert(!owner.queued && !owner.running);

    assert(panel_wifi_deferred_schedule(&owner, 6U));
    assert(!panel_wifi_deferred_begin(&owner, 6U, 6U, true));
    assert(!owner.queued && !owner.running);

    assert(panel_wifi_deferred_schedule(&owner, 7U));
    panel_wifi_deferred_invalidate(&owner);
    assert(!panel_wifi_deferred_begin(&owner, 7U, 8U, false));
    assert(!owner.queued && !owner.running);

    assert(panel_wifi_deferred_schedule(&owner, 9U));
    assert(panel_wifi_deferred_begin(&owner, 9U, 9U, false));
    assert(panel_wifi_deferred_still_current(&owner, 9U, 9U, false));
    assert(!panel_wifi_deferred_schedule(&owner, 10U));
    panel_wifi_deferred_invalidate(&owner);
    assert(!panel_wifi_deferred_still_current(&owner, 9U, 10U, false));
    panel_wifi_deferred_finish(&owner);
    assert(!owner.running);

    assert(panel_wifi_deferred_schedule(&owner, 11U));
    assert(panel_wifi_deferred_begin(&owner, 11U, 11U, false));
    panel_wifi_deferred_finish(&owner);
    assert(panel_wifi_deferred_schedule(&owner, 12U));
    puts("PANEL_WIFI_DEFERRED_OWNER_PASS");
    return 0;
}
