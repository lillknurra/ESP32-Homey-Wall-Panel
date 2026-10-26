/* Host mock. The actual guarded ESP-IDF adapter is only compiled; no socket. */
#include "panel_update_i3_probe.h"
#include "panel_update_i3a_web_diag.h"
#include "panel_update_worker.h"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

int esp_reset_reason(void) {return 1;}
static unsigned log_calls,request_calls,bootstrap_calls,worker_poll_calls;
static unsigned fake_internal_free = 128U * 1024U;
static unsigned fake_largest = 64U * 1024U;
static unsigned fake_stack = 8192U;
void test_log(const char *level,const char *tag,const char *format,...)
{
    (void)level;
    assert(strcmp(tag,"panel_update_observer")==0);
    assert(format);
    ++log_calls;
}
size_t heap_caps_get_free_size(unsigned caps){assert(caps==1U); return fake_internal_free;}
size_t heap_caps_get_largest_free_block(unsigned caps){assert(caps==1U); return fake_largest;}
size_t heap_caps_get_minimum_free_size(unsigned caps){assert(caps==1U); return 60U * 1024U;}
unsigned uxTaskGetStackHighWaterMark(void *task){assert(task==NULL);return fake_stack;}

bool panel_update_worker_bootstrap(void) {++bootstrap_calls;return true;}
bool panel_update_worker_request(uint32_t generation, const char *channel,
    bool wifi_ready, bool normal_boot_settled)
{
    assert(generation==1U);
    assert(strcmp(channel,"stable")==0);
    assert(wifi_ready && normal_boot_settled);
    ++request_calls;
    return true;
}
panel_update_poll_result_t panel_update_worker_poll(uint32_t *gen,
    char *body,size_t cap,size_t *length)
{
    (void)body;
    assert(cap==PANEL_UPDATE_MANIFEST_MAX_BYTES+1U);
    *gen=1U;
    *length=0U;
    ++worker_poll_calls;
    return worker_poll_calls < 40U ? PANEL_UPDATE_POLL_BUSY : PANEL_UPDATE_POLL_QUARANTINED;
}
int main(void)
{
    for(uint64_t sec=0;sec<=450;sec++){
        panel_update_i3_owner_input_t input = {
            .monotonic_ms=sec*1000U,
            .wifi_online=true,.ui_allocated=true,.homey_ready=true,
            .operator_trial_approved=true /* host mock only */
        };
        panel_update_i3_owner_active_tick(&input);
    }
    assert(log_calls>=7);
    panel_update_i3a_snapshot_t snapshot = {0};
    assert(panel_update_i3a_snapshot_copy(&snapshot));
    assert(snapshot.monotonic_ms==420000U);
    assert(snapshot.reset_reason==1U);
    assert(snapshot.internal_free_bytes==fake_internal_free);
    assert(snapshot.homey_ready && snapshot.wifi_online && snapshot.ui_allocated);
#if PANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED
    assert(request_calls==1);
    assert(bootstrap_calls==1);
    assert(worker_poll_calls==40);
#else
    assert(request_calls==0);
    assert(bootstrap_calls==0);
    assert(worker_poll_calls==0);
#endif
    puts("PATCH116I3 ADAPTER MOCK PASS");
}
