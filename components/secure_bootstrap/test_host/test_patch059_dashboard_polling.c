#include <assert.h>
#include <stdbool.h>
#include "panel_homey_dashboard_binding.h"

typedef enum {
    ATHOM_HOMEY_DATA_LOADING = 0,
    ATHOM_HOMEY_DATA_READY,
    ATHOM_HOMEY_DATA_ERROR,
    ATHOM_HOMEY_DATA_RETRYING,
} athom_homey_data_state_t;

/* PATCH059_PRODUCTION_FUNCTIONS */

int main(void)
{
    assert(dashboard_snapshot_poll_allowed(false, ATHOM_HOMEY_DATA_READY));
    assert(dashboard_snapshot_poll_allowed(true, ATHOM_HOMEY_DATA_ERROR));
    assert(dashboard_snapshot_poll_allowed(true, ATHOM_HOMEY_DATA_RETRYING));
    assert(!dashboard_snapshot_poll_allowed(false, ATHOM_HOMEY_DATA_ERROR));
    assert(!dashboard_snapshot_poll_allowed(false, ATHOM_HOMEY_DATA_LOADING));

    panel_homey_dashboard_state_t dashboard;
    panel_homey_dashboard_state_init(&dashboard);
    if (dashboard_snapshot_poll_allowed(true, ATHOM_HOMEY_DATA_ERROR)) {
        assert(panel_homey_dashboard_apply_snapshot(
            PANEL_HOMEY_READ_STALE, NULL, 0U, &dashboard) ==
            PANEL_HOMEY_DASHBOARD_APPLY_UPDATED);
    }
    assert(dashboard.widgets[0].status == PANEL_WIDGET_UNAVAILABLE);
    assert(dashboard.widgets[1].status == PANEL_WIDGET_UNAVAILABLE);
    assert(dashboard.widgets[2].status == PANEL_WIDGET_UNAVAILABLE);

    assert(dashboard_favorites_apply_allowed(ATHOM_HOMEY_DATA_READY));
    assert(!dashboard_favorites_apply_allowed(ATHOM_HOMEY_DATA_ERROR));
    assert(!dashboard_favorites_apply_allowed(ATHOM_HOMEY_DATA_RETRYING));

    assert(!dashboard_poll_requires_refresh(false, false));
    assert(dashboard_poll_requires_refresh(true, false));
    assert(dashboard_poll_requires_refresh(false, true));
    return 0;
}
