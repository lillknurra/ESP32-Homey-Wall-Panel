#include "panel_update_https_fetch.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    char buf[PANEL_UPDATE_MANIFEST_MAX_BYTES + 1U];
    size_t len = 10;
    assert(panel_update_https_fetch("stable", buf, sizeof(buf), &len) == PANEL_UPDATE_FETCH_DISABLED);
    assert(len == 0U);
    assert(panel_update_https_fetch("beta", buf, sizeof(buf), &len) == PANEL_UPDATE_FETCH_DISABLED);
    assert(len == 0U);
    puts("PATCH116I1_HTTPS_FETCH_DEFAULT_OFF=PASS");
    return 0;
}
