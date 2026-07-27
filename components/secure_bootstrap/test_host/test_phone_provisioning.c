#include "phone_provisioning.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static void fill(uint8_t *p,uint8_t v){for(size_t i=0;i<PHONE_PROV_STATE_BYTES;i++)p[i]=(uint8_t)(v+i);}

static void test_exact_replay_reports_consumed(void){
 phone_prov_context_t c; phone_prov_init(&c); assert(phone_prov_open(&c)==PHONE_PROV_OK);
 uint8_t state[PHONE_PROV_STATE_BYTES]={0}; state[0]=7;
 assert(phone_prov_begin(&c,1,state)==PHONE_PROV_OK);
 assert(phone_prov_complete(&c,2,state,phone_prov_mock_provider())==PHONE_PROV_OK);
 assert(phone_prov_complete(&c,2,state,phone_prov_mock_provider())==PHONE_PROV_ERR_CONSUMED);
}


static void test_change_refresh_repopulates_candidates(void){
 phone_prov_context_t c; phone_prov_init(&c); c.state=PHONE_PROV_READY;
 assert(phone_prov_change_refresh(&c,phone_prov_mock_provider())==PHONE_PROV_OK);
 assert(c.state==PHONE_PROV_HOMEY_SELECTION_REQUIRED);
 assert(c.result.homey_count==2U);
 assert(strcmp(c.result.homeys[0].name,"Hemmet")==0);
 assert(strcmp(c.result.homeys[1].name,"Sommarhuset")==0);
}

static bool invalid_provider_wiped;
static phone_prov_result_t complete_zero(phone_prov_oauth_result_t *o){memset(o,0,sizeof(*o));o->homey_count=0U;return PHONE_PROV_OK;}
static phone_prov_result_t complete_overflow(phone_prov_oauth_result_t *o){memset(o,0,sizeof(*o));o->homey_count=PHONE_PROV_MAX_HOMEYS+1U;return PHONE_PROV_OK;}
static void wipe_invalid(phone_prov_oauth_result_t *o){invalid_provider_wiped=true;memset(o,0,sizeof(*o));}
static const phone_prov_provider_t zero_provider={complete_zero,wipe_invalid},overflow_provider={complete_overflow,wipe_invalid};
static void test_invalid_candidate_counts_fail_closed(void){phone_prov_context_t c;uint8_t state[PHONE_PROV_STATE_BYTES]={0};phone_prov_init(&c);assert(phone_prov_open(&c)==PHONE_PROV_OK);assert(phone_prov_begin(&c,1,state)==PHONE_PROV_OK);invalid_provider_wiped=false;assert(phone_prov_complete(&c,2,state,&zero_provider)==PHONE_PROV_ERR_STALE);assert(invalid_provider_wiped);phone_prov_init(&c);assert(phone_prov_open(&c)==PHONE_PROV_OK);assert(phone_prov_begin(&c,1,state)==PHONE_PROV_OK);invalid_provider_wiped=false;assert(phone_prov_complete(&c,2,state,&overflow_provider)==PHONE_PROV_ERR_STALE);assert(invalid_provider_wiped);}
static void test_publication_readback_invariant(void){phone_prov_context_t c;phone_prov_record_v1_t e,a;uint8_t state[PHONE_PROV_STATE_BYTES]={0};phone_prov_init(&c);assert(phone_prov_open(&c)==PHONE_PROV_OK);assert(phone_prov_begin(&c,1,state)==PHONE_PROV_OK);assert(phone_prov_complete(&c,2,state,phone_prov_mock_provider())==PHONE_PROV_OK);assert(phone_prov_select(&c,"synthetic-homey-primary",&e,7)==PHONE_PROV_OK);a=e;assert(phone_prov_publication_readback_valid(&e,&a,true));assert(!phone_prov_publication_readback_valid(&e,&a,false));a.homey_name[0]^=1;a.payload_crc32=0;a.payload_crc32=phone_prov_crc32(&a,sizeof(a));assert(phone_prov_record_valid(&a));assert(!phone_prov_publication_readback_valid(&e,&a,true));}

int main(void){
 test_invalid_candidate_counts_fail_closed();
 test_publication_readback_invariant();
 test_change_refresh_repopulates_candidates();
 test_exact_replay_reports_consumed();phone_prov_context_t c;phone_prov_init(&c);assert(c.state==PHONE_PROV_UNPROVISIONED);assert(phone_prov_open(&c)==PHONE_PROV_OK);uint8_t s[32],bad[32];fill(s,1);fill(bad,2);assert(phone_prov_begin(&c,10,s)==PHONE_PROV_OK);assert(phone_prov_complete(&c,11,bad,phone_prov_mock_provider())==PHONE_PROV_ERR_MISMATCH);assert(phone_prov_complete(&c,11,s,phone_prov_mock_provider())==PHONE_PROV_OK);assert(c.state==PHONE_PROV_HOMEY_SELECTION_REQUIRED);phone_prov_record_v1_t r;assert(phone_prov_select(&c,"missing",&r,1)==PHONE_PROV_ERR_STALE);assert(phone_prov_select(&c,"synthetic-homey-primary",&r,1)==PHONE_PROV_OK);assert(phone_prov_record_valid(&r));r.homey_name[0]^=1;assert(!phone_prov_record_valid(&r));phone_prov_init(&c);assert(phone_prov_open(&c)==PHONE_PROV_OK);assert(phone_prov_begin(&c,0,s)==PHONE_PROV_OK);assert(phone_prov_complete(&c,PHONE_PROV_SESSION_TTL_SECONDS,s,phone_prov_mock_provider())==PHONE_PROV_ERR_EXPIRED);phone_prov_init(&c);assert(phone_prov_open(&c)==PHONE_PROV_OK);assert(phone_prov_begin(&c,0,s)==PHONE_PROV_OK);assert(phone_prov_complete(&c,1,s,phone_prov_mock_provider())==PHONE_PROV_OK);assert(phone_prov_complete(&c,1,s,phone_prov_mock_provider())==PHONE_PROV_ERR_CONSUMED);puts("PHONE_PROV_HOST_TEST PASS");return 0;}
