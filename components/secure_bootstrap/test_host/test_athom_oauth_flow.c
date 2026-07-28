#include "athom_oauth_flow.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void fill(uint8_t*s)
{
    for(size_t i=0;i<ATHOM_OAUTH_STATE_BYTES;i++)s[i]=(uint8_t)(i*7U+3U);
}

int main(void)
{
    uint8_t state[ATHOM_OAUTH_STATE_BYTES];
    uint8_t decoded[ATHOM_OAUTH_STATE_BYTES];
    uint8_t bad[ATHOM_OAUTH_STATE_BYTES];
    char text[ATHOM_OAUTH_STATE_TEXT_MAX];
    fill(state);

    assert(athom_oauth_state_encode(state,text));
    assert(strlen(text)==43U);
    assert(athom_oauth_state_decode(text,decoded));
    assert(memcmp(state,decoded,sizeof(state))==0);
    assert(!athom_oauth_state_decode("",decoded));
    assert(!athom_oauth_state_decode(NULL,decoded));

    athom_oauth_session_t session;
    athom_oauth_session_reset(&session);
    assert(athom_oauth_session_begin(&session,10,state)==ATHOM_OAUTH_OK);

    assert(
        athom_oauth_callback_consume(&session,11,NULL,true)==
        ATHOM_OAUTH_ERR_MISSING_STATE);

    memcpy(bad,state,sizeof(bad));
    bad[0]^=1U;
    char bad_text[ATHOM_OAUTH_STATE_TEXT_MAX];
    assert(athom_oauth_state_encode(bad,bad_text));
    assert(
        athom_oauth_callback_consume(&session,11,bad_text,true)==
        ATHOM_OAUTH_ERR_MISMATCH);

    assert(
        athom_oauth_callback_consume(&session,11,text,false)==
        ATHOM_OAUTH_ERR_MISSING_CODE);
    assert(
        athom_oauth_callback_consume(&session,11,text,true)==
        ATHOM_OAUTH_OK);
    assert(
        athom_oauth_callback_consume(&session,11,text,true)==
        ATHOM_OAUTH_ERR_CONSUMED);

    assert(athom_oauth_session_begin(&session,20,state)==ATHOM_OAUTH_OK);
    assert(
        athom_oauth_callback_consume(
            &session,
            20+ATHOM_OAUTH_SESSION_TTL_SECONDS,
            text,
            true)==ATHOM_OAUTH_ERR_EXPIRED);

    char status[192];
    assert(athom_oauth_status_json(
        status,
        sizeof(status),
        true,
        true,
        false,
        true,
        ATHOM_OAUTH_OK));
    assert(strstr(status,"\"mdns_ready\":true")!=NULL);
    assert(strstr(status,"\"client_configured\":true")!=NULL);
    assert(strstr(status,"client_secret")==NULL);
    assert(strstr(status,"authorization_code")==NULL);
    assert(strstr(status,"access_token")==NULL);
    assert(strstr(status,"refresh_token")==NULL);

    puts("ATHOM_OAUTH_FLOW_HOST_TEST PASS");
    return 0;
}
