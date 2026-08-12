#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_all(const char *path)
{
    FILE *f = fopen(path, "rb");
    assert(f != NULL);
    assert(fseek(f, 0, SEEK_END) == 0);
    long n = ftell(f);
    assert(n >= 0);
    rewind(f);
    char *buf = calloc(1U, (size_t)n + 1U);
    assert(buf != NULL);
    assert(fread(buf, 1U, (size_t)n, f) == (size_t)n);
    fclose(f);
    return buf;
}

static void require(const char *haystack, const char *needle)
{
    if (strstr(haystack, needle) == NULL) {
        fprintf(stderr, "missing policy marker: %s\n", needle);
        exit(2);
    }
}

static void forbid(const char *haystack, const char *needle)
{
    if (strstr(haystack, needle) != NULL) {
        fprintf(stderr, "forbidden policy marker: %s\n", needle);
        exit(3);
    }
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    char *src = read_all(argv[1]);
    char *hdr = read_all(argv[2]);

    require(src, "#define CLOUD_HTTP_TIMEOUT_MS 8000");
    require(src, "#define HOMEY_REMOTE_HTTP_TIMEOUT_MS 8000");
    require(src, "persistent_http_client_t s_cloud_http");
    require(src, "persistent_http_client_t s_homey_http");
    require(src, "esp_http_client_set_url(ctx->handle, url)");
    require(src, "esp_http_client_set_user_data(ctx->handle, &buffer)");
    require(src, "selected->remote_url");
    require(src, "state->selected_homey.remote_url");
    require(src, "state->selected_homey.local_url_secure[0] = 0");
    require(src, "state->selected_homey.local_url[0] = 0");
    require(src, "PATCH019A1_TRANSPORT");
    require(src, "PATCH019A1_MEMORY");
    require(src, "DEVICES_RESPONSE_RECEIVED");
    require(src, "privacy=sanitized");
    forbid(src, "getaddrinfo(");
    forbid(src, "dns_begin host=");
    forbid(src, "homey_login_local_secure");
    forbid(src, "homey_login_local\"");
    forbid(src, "athom_homey_preferred_url(&state->selected_homey)");
    forbid(src, "url=%s");

    require(hdr, "ATHOM_TRANSPORT_DNS_FAIL");
    require(hdr, "ATHOM_TRANSPORT_TCP_CONNECT_FAIL");
    require(hdr, "ATHOM_TRANSPORT_TLS_FAIL");
    require(hdr, "ATHOM_TRANSPORT_HTTP_TIMEOUT");
    require(hdr, "ATHOM_TRANSPORT_HTTP_401");
    require(hdr, "ATHOM_TRANSPORT_HTTP_403");
    require(hdr, "ATHOM_TRANSPORT_HTTP_408");
    require(hdr, "ATHOM_TRANSPORT_HTTP_429");
    require(hdr, "ATHOM_TRANSPORT_HTTP_5XX");
    require(hdr, "ATHOM_TRANSPORT_HOMEY_SESSION_FAIL");
    require(hdr, "ATHOM_TRANSPORT_FAVORITES_FAIL");
    require(hdr, "ATHOM_TRANSPORT_ZONES_FAIL");
    require(hdr, "ATHOM_TRANSPORT_DEVICES_FAIL");
    require(hdr, "ATHOM_TRANSPORT_PARSE_FAIL");
    require(hdr, "ATHOM_TRANSPORT_NO_VALID_ENDPOINT");

    free(src);
    free(hdr);
    puts("PATCH019A1_TRANSPORT_POLICY_HOST_TEST=PASS");
    return 0;
}
