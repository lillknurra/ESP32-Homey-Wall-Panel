#include "panel_update_worker_policy.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned long checks;
#define CHECK(x) do { assert(x); checks++; } while (0)
#define T PANEL_UPDATE_WORKER_DEADLINE_MS

int main(void)
{
    panel_update_worker_policy_t p;
    panel_update_worker_policy_init(&p);
    CHECK(p.phase == PANEL_UPDATE_WORKER_IDLE);
    CHECK(!panel_update_worker_policy_queue(NULL, 1, 0, true, true, true));
    CHECK(!panel_update_worker_policy_queue(&p, 0, 0, true, true, true));
    CHECK(!panel_update_worker_policy_queue(&p, 1, 0, false, true, true));
    CHECK(!panel_update_worker_policy_queue(&p, 1, 0, true, false, true));
    CHECK(!panel_update_worker_policy_queue(&p, 1, 0, true, true, false));
    CHECK(!panel_update_worker_policy_queue(&p, 1, UINT64_MAX, true, true, true));
    CHECK(panel_update_worker_policy_queue(&p, 1, 0, true, true, true));
    CHECK(!panel_update_worker_policy_queue(&p, 2, 0, true, true, true));
    CHECK(!panel_update_worker_policy_worker_begin(&p, 2));
    CHECK(panel_update_worker_policy_worker_begin(&p, 1));
    CHECK(p.worker_owns_transport);
    CHECK(!panel_update_worker_policy_worker_begin(&p, 1));
    CHECK(!panel_update_worker_policy_tick(&p, T-1));
    CHECK(panel_update_worker_policy_tick(&p, T));
    CHECK(p.phase == PANEL_UPDATE_WORKER_QUARANTINED);
    CHECK(p.worker_owns_transport);
    CHECK(!panel_update_worker_policy_queue(&p, 2, T+1, true, true, true));
    CHECK(!panel_update_worker_policy_worker_finish(&p, 3, T+2, true));
    CHECK(p.worker_owns_transport);
    CHECK(!panel_update_worker_policy_worker_finish(&p, 1, T+2, true));
    CHECK(!p.worker_owns_transport);
    CHECK(p.phase == PANEL_UPDATE_WORKER_IDLE);
    CHECK(!panel_update_worker_policy_queue(&p, 1, T+3, true, true, true));
    CHECK(panel_update_worker_policy_queue(&p, 2, T+4, true, true, true));
    CHECK(panel_update_worker_policy_worker_begin(&p, 2));
    CHECK(panel_update_worker_policy_worker_finish(&p, 2, T+5, true));
    CHECK(p.phase == PANEL_UPDATE_WORKER_FINISHED_OK);
    bool outcome = false;
    CHECK(!panel_update_worker_policy_take_begin(&p, 1, T+6, &outcome));
    CHECK(panel_update_worker_policy_take_begin(&p, 2, T+6, &outcome));
    CHECK(outcome);
    CHECK(p.phase == PANEL_UPDATE_WORKER_READING);
    CHECK(!panel_update_worker_policy_queue(&p, 3, T+6, true, true, true));
    CHECK(!panel_update_worker_policy_take_end(&p, 1));
    CHECK(panel_update_worker_policy_take_end(&p, 2));
    CHECK(!panel_update_worker_policy_take_end(&p, 2));
    CHECK(panel_update_worker_policy_queue(&p, 3, T+7, true, true, true));
    CHECK(!panel_update_worker_policy_tick(&p, T+8));
    CHECK(panel_update_worker_policy_tick(&p, T+7+T));
    CHECK(p.phase == PANEL_UPDATE_WORKER_QUARANTINED);
    CHECK(!p.worker_owns_transport);
    CHECK(!panel_update_worker_policy_worker_begin(&p, 1));
    CHECK(!panel_update_worker_policy_worker_begin(&p, 3));
    CHECK(p.phase == PANEL_UPDATE_WORKER_IDLE);
    CHECK(panel_update_worker_policy_queue(&p, 4, T+T+9, true, true, true));
    CHECK(panel_update_worker_policy_worker_begin(&p, 4));
    CHECK(panel_update_worker_policy_worker_finish(&p, 4, T+T+10, false));
    CHECK(p.phase == PANEL_UPDATE_WORKER_FINISHED_ERROR);
    outcome = true;
    CHECK(panel_update_worker_policy_take_begin(&p, 4, T+T+11, &outcome));
    CHECK(!outcome);
    CHECK(panel_update_worker_policy_take_end(&p, 4));
    CHECK(p.phase == PANEL_UPDATE_WORKER_IDLE);
    CHECK(panel_update_worker_policy_queue(&p, 5, T+T+12, true, true, true));
    CHECK(panel_update_worker_policy_worker_begin(&p, 5));
    CHECK(!panel_update_worker_policy_worker_finish(&p, 5, T+T+12+T, true));
    CHECK(p.phase == PANEL_UPDATE_WORKER_IDLE);
    CHECK(panel_update_worker_policy_queue(&p, 6, T+T+13+T, true, true, true));
    CHECK(panel_update_worker_policy_worker_begin(&p, 6));
    CHECK(panel_update_worker_policy_worker_finish(&p, 6, T+T+14+T, true));
    CHECK(!panel_update_worker_policy_take_begin(&p, 6, T+T+13+T+T, &outcome));
    CHECK(p.phase == PANEL_UPDATE_WORKER_IDLE);
    /* A large deterministic soak: stale completions can never reclaim newer generations. */
    for (uint32_t i=7; i<22000; ++i) {
        uint64_t base = (uint64_t)i*UINT64_C(100000);
        CHECK(panel_update_worker_policy_queue(&p, i, base, true, true, true));
        CHECK(!panel_update_worker_policy_queue(&p, i+1, base, true, true, true));
        if (i%3==0) {
            CHECK(panel_update_worker_policy_tick(&p, base+T));
            CHECK(!panel_update_worker_policy_worker_begin(&p,i));
            CHECK(p.phase==PANEL_UPDATE_WORKER_IDLE);
        } else {
            CHECK(panel_update_worker_policy_worker_begin(&p, i));
            if (i%3==1) {
                CHECK(panel_update_worker_policy_tick(&p, base+T));
                CHECK(!panel_update_worker_policy_worker_finish(&p,i,base+T+1,true));
                CHECK(p.phase==PANEL_UPDATE_WORKER_IDLE);
            } else {
                CHECK(panel_update_worker_policy_worker_finish(&p,i,base+1,true));
                CHECK(panel_update_worker_policy_take_begin(&p,i,base+2,&outcome));
                CHECK(outcome);
                CHECK(panel_update_worker_policy_take_end(&p,i));
            }
        }
    }
    /* UINT64 overflow: never accept wrapped wall clock as fresh result. */
    panel_update_worker_policy_init(&p);
    CHECK(panel_update_worker_policy_queue(&p,1,UINT64_MAX-10,true,true,true));
    CHECK(panel_update_worker_policy_worker_begin(&p,1));
    CHECK(!panel_update_worker_policy_worker_finish(&p,1,3,true));
    CHECK(p.phase==PANEL_UPDATE_WORKER_IDLE);
    printf("PATCH116I2_WORKER_POLICY=PASS ASSERTIONS=%lu\n", checks);
    return 0;
}
