#include "athom_oauth_flow.h"
#include <stdio.h>
#include <string.h>

static void secure_zero(void *p,size_t n){volatile unsigned char *q=(volatile unsigned char*)p;while(n--)*q++=0U;}
static const char B64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static int b64v(char c){if(c>='A'&&c<='Z')return c-'A';if(c>='a'&&c<='z')return c-'a'+26;if(c>='0'&&c<='9')return c-'0'+52;if(c=='-')return 62;if(c=='_')return 63;return -1;}

void athom_oauth_session_reset(athom_oauth_session_t *s){if(s){secure_zero(s,sizeof(*s));}}

athom_oauth_result_t athom_oauth_session_begin(
    athom_oauth_session_t *s,
    int64_t now,
    const uint8_t r[ATHOM_OAUTH_STATE_BYTES])
{
    if(!s||!r||now<0)return ATHOM_OAUTH_ERR_ARGUMENT;
    athom_oauth_session_reset(s);
    memcpy(s->state,r,ATHOM_OAUTH_STATE_BYTES);
    s->issued_at_s=now;
    s->active=true;
    return ATHOM_OAUTH_OK;
}

athom_oauth_result_t athom_oauth_session_consume(
    athom_oauth_session_t *s,
    int64_t now,
    const uint8_t c[ATHOM_OAUTH_STATE_BYTES],
    bool code_present)
{
    if(!s||!c||now<0)return ATHOM_OAUTH_ERR_ARGUMENT;
    if(!code_present)return ATHOM_OAUTH_ERR_MISSING_CODE;
    if(!s->active)return ATHOM_OAUTH_ERR_STATE;
    if(s->consumed)return ATHOM_OAUTH_ERR_CONSUMED;
    if(now<s->issued_at_s||now-s->issued_at_s>=ATHOM_OAUTH_SESSION_TTL_SECONDS){
        s->consumed=true;
        secure_zero(s->state,sizeof(s->state));
        return ATHOM_OAUTH_ERR_EXPIRED;
    }
    unsigned diff=0;
    for(size_t i=0;i<ATHOM_OAUTH_STATE_BYTES;i++)diff|=(unsigned)(s->state[i]^c[i]);
    if(diff)return ATHOM_OAUTH_ERR_MISMATCH;
    s->consumed=true;
    secure_zero(s->state,sizeof(s->state));
    return ATHOM_OAUTH_OK;
}

athom_oauth_result_t athom_oauth_callback_consume(
    athom_oauth_session_t *s,
    int64_t now,
    const char *state_text,
    bool code_present)
{
    if(!s||now<0)return ATHOM_OAUTH_ERR_ARGUMENT;
    if(!state_text||state_text[0]=='\0')return ATHOM_OAUTH_ERR_MISSING_STATE;
    uint8_t candidate[ATHOM_OAUTH_STATE_BYTES]={0};
    if(!athom_oauth_state_decode(state_text,candidate)){
        secure_zero(candidate,sizeof(candidate));
        return ATHOM_OAUTH_ERR_MISMATCH;
    }
    athom_oauth_result_t result=athom_oauth_session_consume(s,now,candidate,code_present);
    secure_zero(candidate,sizeof(candidate));
    return result;
}

bool athom_oauth_state_encode(
    const uint8_t in[ATHOM_OAUTH_STATE_BYTES],
    char out[ATHOM_OAUTH_STATE_TEXT_MAX])
{
    if(!in||!out)return false;
    size_t i=0,o=0;
    while(i+3<=ATHOM_OAUTH_STATE_BYTES){
        uint32_t v=((uint32_t)in[i]<<16)|((uint32_t)in[i+1]<<8)|in[i+2];
        out[o++]=B64[(v>>18)&63];
        out[o++]=B64[(v>>12)&63];
        out[o++]=B64[(v>>6)&63];
        out[o++]=B64[v&63];
        i+=3;
    }
    if(i<ATHOM_OAUTH_STATE_BYTES){
        uint32_t v=(uint32_t)in[i]<<16;
        out[o++]=B64[(v>>18)&63];
        if(i+1<ATHOM_OAUTH_STATE_BYTES){
            v|=(uint32_t)in[i+1]<<8;
            out[o++]=B64[(v>>12)&63];
            out[o++]=B64[(v>>6)&63];
        }else{
            out[o++]=B64[(v>>12)&63];
        }
    }
    out[o]='\0';
    return o==43U;
}

bool athom_oauth_state_decode(
    const char *t,
    uint8_t out[ATHOM_OAUTH_STATE_BYTES])
{
    if(!t||!out||strlen(t)!=43U)return false;
    size_t i=0,o=0;
    while(i+4<=40U){
        int a=b64v(t[i]),b=b64v(t[i+1]),c=b64v(t[i+2]),d=b64v(t[i+3]);
        if(a<0||b<0||c<0||d<0)return false;
        uint32_t v=((uint32_t)a<<18)|((uint32_t)b<<12)|((uint32_t)c<<6)|(uint32_t)d;
        out[o++]=(uint8_t)(v>>16);
        out[o++]=(uint8_t)(v>>8);
        out[o++]=(uint8_t)v;
        i+=4;
    }
    int a=b64v(t[40]),b=b64v(t[41]),c=b64v(t[42]);
    if(a<0||b<0||c<0)return false;
    uint32_t v=((uint32_t)a<<18)|((uint32_t)b<<12)|((uint32_t)c<<6);
    out[o++]=(uint8_t)(v>>16);
    out[o++]=(uint8_t)(v>>8);
    return o==ATHOM_OAUTH_STATE_BYTES;
}

bool athom_oauth_status_json(
    char *out,
    size_t capacity,
    bool mdns_ready,
    bool client_configured,
    bool oauth_pending,
    bool callback_received,
    athom_oauth_result_t last_result)
{
    if(!out||capacity==0U)return false;
    int written=snprintf(
        out,
        capacity,
        "{\"mdns_ready\":%s,\"client_configured\":%s,"
        "\"oauth_pending\":%s,\"callback_received\":%s,\"last_result\":%d}",
        mdns_ready?"true":"false",
        client_configured?"true":"false",
        oauth_pending?"true":"false",
        callback_received?"true":"false",
        (int)last_result);
    return written>0&&(size_t)written<capacity;
}
