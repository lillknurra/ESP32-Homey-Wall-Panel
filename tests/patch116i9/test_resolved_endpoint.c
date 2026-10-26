#include "panel_update_i9_resolved_endpoint_policy.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_positive_channels(void)
{
    panel_update_i9_endpoint_t out;
    assert(panel_update_i9_build_resolved_ipv4_endpoint(
        "stable", "185.199.108.153", &out, sizeof(out)));
    assert(strcmp(out.url,
        "https://185.199.108.153/ESP32-Homey-Wall-Panel/updates/stable/"
        "waveshare-esp32-s3-touch-lcd-4b-n16r8.json") == 0);
    assert(strcmp(out.tls_common_name, "lillknurra.github.io") == 0);
    assert(strcmp(out.http_host_header, "lillknurra.github.io") == 0);
    assert(strcmp(out.manifest_path,
        "/ESP32-Homey-Wall-Panel/updates/stable/"
        "waveshare-esp32-s3-touch-lcd-4b-n16r8.json") == 0);
    assert(out.disable_auto_redirect);
    assert(!out.skip_cert_common_name_check);
    assert(out.require_certificate_bundle);
    assert(out.require_async_http_mode);

    assert(panel_update_i9_build_resolved_ipv4_endpoint(
        "beta", "185.199.111.153", &out, sizeof(out)));
    assert(strcmp(out.url,
        "https://185.199.111.153/ESP32-Homey-Wall-Panel/updates/beta/"
        "waveshare-esp32-s3-touch-lcd-4b-n16r8.json") == 0);
    assert(strcmp(out.tls_common_name, PANEL_UPDATE_I9_HOSTNAME) == 0);
    assert(strcmp(out.http_host_header, PANEL_UPDATE_I9_HOSTNAME) == 0);
}

static void test_address_classes(void)
{
    panel_update_i9_endpoint_t out;
    const char *denied[] = {
        "0.1.2.3", "10.1.2.3", "100.64.0.1", "100.127.255.254",
        "127.0.0.1", "169.254.1.1", "172.16.0.1", "172.31.255.254",
        "192.0.0.1", "192.0.2.1", "192.88.99.1", "192.168.1.1",
        "198.18.0.1", "198.19.255.254", "198.51.100.1", "203.0.113.1",
        "224.0.0.1", "239.255.255.255", "240.0.0.1", "255.255.255.255",
    };
    for (size_t i = 0U; i < sizeof(denied) / sizeof(denied[0]); ++i) {
        memset(&out, 0xA5, sizeof(out));
        assert(!panel_update_i9_build_resolved_ipv4_endpoint(
            "stable", denied[i], &out, sizeof(out)));
        assert(out.url[0] == '\0');
        assert(out.tls_common_name[0] == '\0');
        assert(!out.disable_auto_redirect);
    }
    assert(panel_update_i9_build_resolved_ipv4_endpoint(
        "stable", "8.8.8.8", &out, sizeof(out)));
}

static void test_malformed_addresses(void)
{
    panel_update_i9_endpoint_t out;
    const char *invalid[] = {
        "", "1.2.3", "1.2.3.4.5", ".1.2.3.4", "1..2.3.4",
        "1.2.3.", "256.1.1.1", "1.256.1.1", "1.1.1.256",
        "01.2.3.4", "1.02.3.4", "+1.2.3.4", " 1.2.3.4",
        "1.2.3.4 ", "1.2.3.4\r\nHost:evil", "1.2.3.4/path",
        "1.2.3.4:443", "::1", "[::1]", "example.com",
        "1.2.3.999999999999999999999999999999999999999999999999999",
    };
    for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        assert(!panel_update_i9_build_resolved_ipv4_endpoint(
            "stable", invalid[i], &out, sizeof(out)));
        assert(out.url[0] == '\0');
    }
}

static void test_channel_and_size_failures(void)
{
    panel_update_i9_endpoint_t out;
    const char *bad_channels[] = {
        NULL, "", "Stable", "BETA", "dev", "stable/beta", "stable?x=1",
        "stable\r\nHost:evil",
    };
    for (size_t i = 0U; i < sizeof(bad_channels) / sizeof(bad_channels[0]); ++i) {
        assert(!panel_update_i9_build_resolved_ipv4_endpoint(
            bad_channels[i], "185.199.108.153", &out, sizeof(out)));
        assert(out.url[0] == '\0');
    }
    assert(!panel_update_i9_build_resolved_ipv4_endpoint(
        "stable", "185.199.108.153", &out, sizeof(out) - 1U));
    assert(!panel_update_i9_build_resolved_ipv4_endpoint(
        "stable", "185.199.108.153", &out, 0U));
    assert(!panel_update_i9_build_resolved_ipv4_endpoint(
        "stable", "185.199.108.153", NULL, sizeof(out)));
    assert(out.url[0] == '\0');
}

static void test_outputs_are_fixed_and_unambiguous(void)
{
    panel_update_i9_endpoint_t out;
    assert(panel_update_i9_build_resolved_ipv4_endpoint(
        "beta", "185.199.109.153", &out, sizeof(out)));
    assert(strstr(out.url, "@") == NULL);
    assert(strstr(out.url, "#") == NULL);
    assert(strstr(out.url, "?") == NULL);
    assert(strchr(out.url, '\r') == NULL && strchr(out.url, '\n') == NULL);
    assert(strstr(out.manifest_path, "..") == NULL);
    assert(strcmp(out.http_host_header, out.tls_common_name) == 0);
    assert(strlen(out.url) < sizeof(out.url));
    assert(strlen(out.tls_common_name) < sizeof(out.tls_common_name));
    assert(strlen(out.http_host_header) < sizeof(out.http_host_header));
    assert(strlen(out.manifest_path) < sizeof(out.manifest_path));
}

int main(void)
{
    test_positive_channels();
    test_address_classes();
    test_malformed_addresses();
    test_channel_and_size_failures();
    test_outputs_are_fixed_and_unambiguous();
    puts("PASS: Patch116I9 resolved IPv4 endpoint policy; no DNS/TLS/HTTP performed");
    return 0;
}
