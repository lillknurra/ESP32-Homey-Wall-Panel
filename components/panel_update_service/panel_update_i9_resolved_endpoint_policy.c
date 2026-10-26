/* Patch116I9: pure offline endpoint policy; intentionally not in native CMake. */
#include "panel_update_i9_resolved_endpoint_policy.h"

#include <stdio.h>
#include <string.h>

static const char stable_path[] =
    "/ESP32-Homey-Wall-Panel/updates/stable/"
    "waveshare-esp32-s3-touch-lcd-4b-n16r8.json";
static const char beta_path[] =
    "/ESP32-Homey-Wall-Panel/updates/beta/"
    "waveshare-esp32-s3-touch-lcd-4b-n16r8.json";

static bool parse_canonical_ipv4(const char *text, unsigned int octets[4])
{
    size_t length = 0U;
    size_t pos = 0U;

    if (text == NULL || octets == NULL) return false;
    while (length <= 15U && text[length] != '\0') length++;
    if (length == 0U || length > 15U) return false;

    for (size_t part = 0U; part < 4U; ++part) {
        unsigned int value = 0U;
        size_t digits = 0U;

        while (pos < length && text[pos] >= '0' && text[pos] <= '9') {
            if (digits == 0U && text[pos] == '0' &&
                pos + 1U < length && text[pos + 1U] >= '0' &&
                text[pos + 1U] <= '9') {
                return false; /* Reject ambiguous leading-zero forms. */
            }
            value = value * 10U + (unsigned int)(text[pos] - '0');
            if (value > 255U) return false;
            digits++;
            pos++;
        }
        if (digits == 0U) return false;
        octets[part] = value;

        if (part < 3U) {
            if (pos >= length || text[pos] != '.') return false;
            pos++;
        }
    }
    return pos == length;
}

static bool public_unicast_ipv4(const unsigned int ip[4])
{
    const unsigned int a = ip[0];
    const unsigned int b = ip[1];
    const unsigned int c = ip[2];

    /* IANA special-use, private, local, documentation, benchmarking,
     * multicast, limited-broadcast, and reserved ranges. */
    if (a == 0U || a == 10U || a == 127U || a >= 224U) return false;
    if (a == 100U && b >= 64U && b <= 127U) return false; /* shared */
    if (a == 169U && b == 254U) return false;             /* link-local */
    if (a == 172U && b >= 16U && b <= 31U) return false;  /* private */
    if (a == 192U && b == 168U) return false;             /* private */
    if (a == 192U && b == 0U && c == 0U) return false;    /* special-use */
    if (a == 192U && b == 0U && c == 2U) return false;     /* documentation */
    if (a == 192U && b == 88U && c == 99U) return false;   /* 6to4 relay */
    if (a == 198U && (b == 18U || b == 19U)) return false; /* benchmark */
    if (a == 198U && b == 51U && c == 100U) return false;  /* documentation */
    if (a == 203U && b == 0U && c == 113U) return false;   /* documentation */
    return true;
}

static const char *path_for_channel(const char *channel)
{
    if (channel == NULL) return NULL;
    if (strcmp(channel, "stable") == 0) return stable_path;
    if (strcmp(channel, "beta") == 0) return beta_path;
    return NULL;
}

bool panel_update_i9_build_resolved_ipv4_endpoint(
    const char *channel,
    const char *resolved_ipv4,
    panel_update_i9_endpoint_t *out,
    size_t out_size)
{
    unsigned int octets[4];
    panel_update_i9_endpoint_t result = {0};
    const char *path;
    int written;

    if (out == NULL || out_size < sizeof(*out)) return false;
    memset(out, 0, sizeof(*out));

    path = path_for_channel(channel);
    if (path == NULL || !parse_canonical_ipv4(resolved_ipv4, octets) ||
        !public_unicast_ipv4(octets)) {
        return false;
    }

    written = snprintf(result.url, sizeof(result.url), "https://%s%s",
                       resolved_ipv4, path);
    if (written < 0 || (size_t)written >= sizeof(result.url)) return false;
    written = snprintf(result.tls_common_name, sizeof(result.tls_common_name),
                       "%s", PANEL_UPDATE_I9_HOSTNAME);
    if (written < 0 || (size_t)written >= sizeof(result.tls_common_name)) return false;
    written = snprintf(result.http_host_header, sizeof(result.http_host_header),
                       "%s", PANEL_UPDATE_I9_HOSTNAME);
    if (written < 0 || (size_t)written >= sizeof(result.http_host_header)) return false;
    written = snprintf(result.manifest_path, sizeof(result.manifest_path), "%s", path);
    if (written < 0 || (size_t)written >= sizeof(result.manifest_path)) return false;

    result.disable_auto_redirect = true;
    result.skip_cert_common_name_check = false;
    result.require_certificate_bundle = true;
    result.require_async_http_mode = true;
    *out = result;
    return true;
}
