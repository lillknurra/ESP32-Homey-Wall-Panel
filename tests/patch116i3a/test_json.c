#include "panel_update_i3a_web_diag.h"
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t assertions;
#define CHECK(x) do { ++assertions; assert(x); } while (0)

static void expect_bad(char *body, size_t cap, size_t *len,
                       uint64_t now, bool have, const panel_update_i3a_snapshot_t *s)
{
    const size_t before_len = *len;
    char backup[1024];
    CHECK(cap <= sizeof(backup));
    memcpy(backup, body, cap);
    CHECK(!panel_update_i3a_append_json(body, cap, len, now, have, s));
    CHECK(*len == before_len);
    CHECK(memcmp(backup, body, cap) == 0);
}

int main(void)
{
    panel_update_i3a_snapshot_t s = {
        .monotonic_ms = 100000,
        .internal_free_bytes = 30499,
        .internal_largest_block_bytes = 15360,
        .internal_minimum_free_bytes = 6867,
        .rotation_stack_margin_bytes = 4096,
        .reset_reason = 1,
        .wifi_online = true,
        .homey_ready = true,
        .ui_allocated = true,
    };
    char text[1024];
    strcpy(text, "{\"state\":\"ready\"}");
    size_t len = strlen(text);
    CHECK(panel_update_i3a_append_json(text, sizeof(text), &len, 100300, true, &s));
    CHECK(len == strlen(text));
    CHECK(strstr(text, "\"valid\":true") != NULL);
    CHECK(strstr(text, "\"stale\":false") != NULL);
    CHECK(strstr(text, "\"age_ms\":300") != NULL);
    CHECK(strstr(text, "\"reset_reason\":1") != NULL);
    CHECK(strstr(text, "\"homey_ready\":true") != NULL);
    printf("%s\n", text); /* Parse as real JSON in Python driver. */

    strcpy(text, "{\"state\":\"ready\"}");len = strlen(text);
    CHECK(panel_update_i3a_append_json(text,sizeof(text),&len,280001,true,&s));
    CHECK(strstr(text, "\"stale\":true") != NULL);
    CHECK(strstr(text, "\"age_ms\":180001") != NULL);
    printf("%s\n", text);

    strcpy(text, "{\"state\":\"ready\"}");len = strlen(text);
    CHECK(panel_update_i3a_append_json(text,sizeof(text),&len,200000,false,&s));
    CHECK(strstr(text, "\"valid\":false") != NULL);
    CHECK(strstr(text, "internal_free_bytes") == NULL);
    printf("%s\n", text);

    strcpy(text, "{\"state\":\"ready\"}");len = strlen(text);
    CHECK(panel_update_i3a_append_json(text,sizeof(text),&len,90000,true,&s));
    CHECK(strstr(text, "\"valid\":false") != NULL); /* time reversal */

    strcpy(text, "{\"state\":\"ready\"}");len = strlen(text);
    s.monotonic_ms = 0;
    CHECK(panel_update_i3a_append_json(text,sizeof(text),&len,UINT64_MAX,false,&s));
    CHECK(strstr(text,"\"valid\":false") != NULL);

    strcpy(text,"{}");len=strlen(text);
    s.monotonic_ms = 0;
    s.internal_free_bytes = UINT32_MAX;
    s.internal_largest_block_bytes = UINT32_MAX;
    s.internal_minimum_free_bytes = UINT32_MAX;
    s.rotation_stack_margin_bytes = UINT32_MAX;
    s.reset_reason = UINT32_MAX;
    CHECK(panel_update_i3a_append_json(text,sizeof(text),&len,UINT64_MAX-1,true,&s));
    CHECK(strstr(text,"18446744073709551614") != NULL);
    CHECK(strstr(text,"4294967295") != NULL);
    printf("%s\n",text);

    /* For every insufficient output buffer, return false with no mutation. */
    for(size_t cap=20;cap<400;cap++) {
        memset(text, 0x5a, sizeof(text));
        const char *base = "{\"state\":\"ready\"}";
        len = strlen(base);
        memcpy(text, base, len + 1U);
        if(cap <= len)continue;
        const size_t oldlen=len;
        char backup[1024];memcpy(backup,text,cap);
        const bool result=panel_update_i3a_append_json(text,cap,&len,200000,true,&s);
        if(result) {CHECK(len>oldlen);CHECK(text[len]=='\0');}
        else {CHECK(len==oldlen);CHECK(memcmp(text,backup,cap)==0);}
    }

    memset(text,0,sizeof(text));strcpy(text,"{\"ok\":true}");len=strlen(text);
    expect_bad(text, len, &len, 200000,true,&s);
    text[len-1]='X';expect_bad(text,sizeof(text),&len,200000,true,&s);
    text[len-1]='}';text[len]='X';expect_bad(text,sizeof(text),&len,200000,true,&s);
    text[len]='\0';text[0]='X';expect_bad(text,sizeof(text),&len,200000,true,&s);
    text[0]='{';CHECK(panel_update_i3a_append_json(text,sizeof(text),&len,200000,true,NULL));
    CHECK(strstr(text,"\"valid\":false") != NULL);
    printf("PATCH116I3A JSON TEST PASS assertions=%" PRIu32 "\n", assertions);
    return 0;
}
