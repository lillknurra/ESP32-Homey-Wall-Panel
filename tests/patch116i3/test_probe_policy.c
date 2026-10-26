#include "panel_update_i3_probe_policy.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned assertions;
#define VERIFY(x) do { ++assertions; if (!(x)) { \
    fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while(0)
static panel_update_i3_sample_t in(uint64_t now)
{
    panel_update_i3_sample_t s = {
        .now_ms=now,
        .internal_free_bytes=128U*1024U,
        .internal_largest_bytes=64U*1024U,
        .internal_minimum_free_bytes=100U*1024U,
        .owner_stack_margin_bytes=5000U,
        .wifi_online=true,.ui_allocated=true,.homey_ready=true,
        .operator_trial_approved=true,
    };
    return s;
}
static int basic(void)
{
    panel_update_i3_policy_t p; panel_update_i3_policy_init(&p);
    panel_update_i3_sample_t s=in(0);
    panel_update_i3_decision_t d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(!d.sampled && !d.one_shot_trial_request);
    VERIFY(p.initialized && p.last_sample_ms==0);
    s=in(59999); d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(!d.sampled);
    s=in(60000); d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(d.sampled && d.sample_healthy && !d.one_shot_trial_request);
    s=in(120000); d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(d.sampled && d.sample_healthy && !d.one_shot_trial_request);
    s=in(180000); d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(d.sampled && d.one_shot_trial_request && p.trial_claimed);
    s=in(240000); d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(d.sampled && !d.one_shot_trial_request && p.count==4);
    VERIFY(p.lowest_internal_free==128U*1024U);
    VERIFY(p.lowest_largest_block==64U*1024U);
    VERIFY(p.lowest_owner_stack_margin==5000U);
    return 0;
}
static int no_network_without_compiled_trial(void)
{
    panel_update_i3_policy_t p; panel_update_i3_policy_init(&p);
    for (uint64_t i=0;i<30;i++) {
        panel_update_i3_sample_t s=in(i*60000U);
        panel_update_i3_decision_t d=panel_update_i3_policy_step(&p,&s,false);
        VERIFY(!d.one_shot_trial_request);
    }
    VERIFY(!p.trial_claimed);
    return 0;
}
static int unhealthy_reset_and_recovery(void)
{
    panel_update_i3_policy_t p; panel_update_i3_policy_init(&p);
    for(uint64_t i=0;i<=8;i++) {
        panel_update_i3_sample_t s=in(i*60000U);
        if(i==2) s.internal_largest_bytes=PANEL_UPDATE_I3_MIN_LARGEST_BLOCK-1;
        if(i==3) s.homey_ready=false;
        if(i==4) s.ui_allocated=false;
        panel_update_i3_decision_t d=panel_update_i3_policy_step(&p,&s,true);
        if(i<=6) VERIFY(!d.one_shot_trial_request);
        if(i==7) VERIFY(d.one_shot_trial_request);
        if(i==8) VERIFY(!d.one_shot_trial_request);
    }
    VERIFY(p.lowest_largest_block==PANEL_UPDATE_I3_MIN_LARGEST_BLOCK-1);
    return 0;
}
static int thresholds(void)
{
    for(unsigned i=0;i<6;i++) {
        panel_update_i3_policy_t p;panel_update_i3_policy_init(&p);
        for(unsigned n=0;n<=8;n++) {
            panel_update_i3_sample_t s=in(n*60000U);
            if(i==0) s.internal_free_bytes=PANEL_UPDATE_I3_MIN_INTERNAL_FREE-1;
            if(i==1) s.internal_largest_bytes=PANEL_UPDATE_I3_MIN_LARGEST_BLOCK-1;
            if(i==2) s.owner_stack_margin_bytes=PANEL_UPDATE_I3_MIN_OWNER_STACK_MARGIN-1;
            if(i==3) s.wifi_online=false;
            if(i==4) s.ui_allocated=false;
            if(i==5) s.homey_ready=false;
            panel_update_i3_decision_t d=panel_update_i3_policy_step(&p,&s,true);
            VERIFY(!d.one_shot_trial_request);
            if(n>0) VERIFY(d.sampled && !d.sample_healthy);
        }
        VERIFY(!p.trial_claimed);
    }
    return 0;
}
static int no_operator_approval(void)
{
    panel_update_i3_policy_t p; panel_update_i3_policy_init(&p);
    for(uint64_t i=0;i<16;i++) {
        panel_update_i3_sample_t s=in(i*60000U);
        s.operator_trial_approved=false;
        panel_update_i3_decision_t d=panel_update_i3_policy_step(&p,&s,true);
        VERIFY(!d.one_shot_trial_request);
    }
    VERIFY(!p.trial_claimed);
    return 0;
}
static int clocks_and_boundaries(void)
{
    panel_update_i3_policy_t p;panel_update_i3_policy_init(&p);
    panel_update_i3_sample_t s=in(999);(void)panel_update_i3_policy_step(&p,&s,true);
    s=in(1000);(void)panel_update_i3_policy_step(&p,&s,true);
    s=in(900);
    panel_update_i3_decision_t d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(!d.sampled && p.stopped);
    s=in(999999);d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(!d.sampled && !d.one_shot_trial_request);
    panel_update_i3_policy_init(&p);
    s=in(UINT64_MAX);d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(!d.sampled && !p.initialized);
    panel_update_i3_policy_init(&p);
    s=in(0);(void)panel_update_i3_policy_step(&p,&s,true);
    p.count=UINT32_MAX;
    s=in(60000);d=panel_update_i3_policy_step(&p,&s,true);
    VERIFY(!d.sampled && p.stopped);
    VERIFY(!panel_update_i3_policy_step(NULL,&s,true).sampled);
    VERIFY(!panel_update_i3_policy_step(&p,NULL,true).sampled);
    return 0;
}
static int fuzz_invariants(void)
{
    for(uint32_t seed=0;seed<512;seed++) {
        panel_update_i3_policy_t p;panel_update_i3_policy_init(&p);
        uint32_t trials=0;
        for(uint32_t i=0;i<256;i++) {
            panel_update_i3_sample_t s=in(i*60000U);
            s.internal_free_bytes=(seed ^ (i*40503U)) % (256U*1024U);
            s.internal_largest_bytes=(seed*13U ^ (i*19037U)) % (96U*1024U);
            s.owner_stack_margin_bytes=(seed*19U ^ (i*7717U)) % 12000U;
            s.wifi_online=(i%5)!=0;
            s.homey_ready=(i%9)!=0;
            s.ui_allocated=(i%4)!=0;
            panel_update_i3_decision_t d=panel_update_i3_policy_step(&p,&s,true);
            if(d.one_shot_trial_request) {
                trials++;
                VERIFY(d.sampled && d.sample_healthy && p.trial_claimed);
                VERIFY(p.consecutive_healthy>=PANEL_UPDATE_I3_REQUIRED_HEALTHY_SAMPLES);
                VERIFY(s.now_ms>=PANEL_UPDATE_I3_BOOT_GRACE_MS);
            }
            VERIFY(trials<=1);
        }
    }
    return 0;
}
int main(void)
{
    if(basic()||no_network_without_compiled_trial()||unhealthy_reset_and_recovery()||
       thresholds()||no_operator_approval()||clocks_and_boundaries()||fuzz_invariants()) return 1;
    printf("PATCH116I3 POLICY PASS assertions=%u\n",assertions);
    return 0;
}
