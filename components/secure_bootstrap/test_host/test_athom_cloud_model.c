#include "athom_cloud_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void fill_token(char *out, size_t length, char seed)
{
    for (size_t i = 0U; i < length; ++i) out[i] = (char)(seed + (i % 20U));
    out[length] = '\0';
}

int main(void)
{
    athom_token_set_t tokens = {0};
    fill_token(tokens.refresh_token, 700U, 'a');
    char access[1200];
    char rotated[900];
    fill_token(access, 1199U, 'A');
    fill_token(rotated, 899U, 'k');

    assert(athom_token_set_apply_refresh(&tokens, access, NULL, 3600U));
    assert(strlen(tokens.access_token) == 1199U);
    assert(strlen(tokens.refresh_token) == 700U);
    assert(athom_token_set_apply_refresh(&tokens, access, rotated, 7200U));
    assert(strlen(tokens.refresh_token) == 899U);

    athom_homey_list_t list = {0};
    list.count = 3U;
    strcpy(list.items[0].id, "homey-a");
    strcpy(list.items[0].name, "Mamma");
    strcpy(list.items[0].local_url_secure, "https://secure.local");
    strcpy(list.items[0].local_url, "http://local");
    strcpy(list.items[0].remote_url, "https://remote");

    strcpy(list.items[1].id, "homey-b");
    strcpy(list.items[1].name, "Sommarhus");
    strcpy(list.items[1].local_url, "http://local-b");
    strcpy(list.items[1].remote_url, "https://remote-b");

    strcpy(list.items[2].id, "homey-c");
    strcpy(list.items[2].name, "Reserv");
    strcpy(list.items[2].remote_url, "https://remote-c");

    assert(strcmp(athom_homey_preferred_url(&list.items[0]),
                  "https://secure.local") == 0);
    assert(strcmp(athom_homey_preferred_url(&list.items[1]),
                  "http://local-b") == 0);
    assert(strcmp(athom_homey_preferred_url(&list.items[2]),
                  "https://remote-c") == 0);

    athom_homey_t none = {0};
    assert(athom_homey_preferred_url(&none) == NULL);
    assert(athom_homey_find_exact(&list, "homey-b") == &list.items[1]);
    assert(athom_homey_find_exact(&list, "HOMEY-B") == NULL);

    char status[4096];
    assert(athom_homey_status_json(
        status, sizeof(status), "ready", &list, &list.items[0], 5U, 12U));
    assert(strstr(status, "\"name\":\"Mamma\"") != NULL);
    assert(strstr(status, "access_token") == NULL);
    assert(strstr(status, "refresh_token") == NULL);
    assert(strstr(status, "client_secret") == NULL);
    assert(strstr(status, "session_token") == NULL);

    puts("ATHOM_CLOUD_MODEL_HOST_TEST PASS");
    return 0;
}
