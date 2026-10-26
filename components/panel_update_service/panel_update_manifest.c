/* Patch116I: fail-closed, read-only metadata policy. */
#include "panel_update_manifest.h"

#include <stdio.h>
#include <string.h>

static bool safe_id(const char *s, size_t max)
{
    if (!s || max == 0U) return false;
    size_t i = 0U;
    for (; i < max && s[i]; i++) {
        const unsigned char c = (unsigned char)s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
    }
    return i > 0U && i < max;
}

static bool semver(const char *s)
{
    if (!s || !*s) return false;
    for (int group = 0; group < 3; group++) {
        size_t digits = 0U;
        const char *part = s;
        while (*s >= '0' && *s <= '9') {
            digits++;
            if (digits > 8U) return false;
            s++;
        }
        if (digits == 0U || (digits > 1U && *part == '0')) return false;
        if (group != 2) {
            if (*s != '.') return false;
            s++;
        }
    }
    return *s == '\0';
}

static bool release_url_valid(const panel_update_manifest_t *m)
{
    const char *u = m->image_url;
    const char *prefix = PANEL_UPDATE_RELEASE_PREFIX;
    const size_t prelen = strlen(prefix);
    size_t ulen = strnlen(u, PANEL_UPDATE_URL_SIZE);
    if (ulen >= PANEL_UPDATE_URL_SIZE || ulen <= prelen + 16U) return false;
    if (strncmp(u, prefix, prelen) != 0) return false;
    for (size_t i = prelen; i < ulen; i++) {
        const char c = u[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_' || c == '/')) return false;
    }
    /* One fixed GitHub release tag per semantic version; no redirects or userinfo in the manifest. */
    const char *tail = u + prelen;
    const char *tagprefix = strcmp(m->channel, "beta") == 0 ? "panel-beta-v" : "panel-v";
    const size_t taglen = strlen(tagprefix);
    if (strncmp(tail, tagprefix, taglen) != 0) return false;
    tail += taglen;
    const size_t vlen = strlen(m->version);
    if (strncmp(tail, m->version, vlen) != 0 || tail[vlen] != '/') return false;
    tail += vlen + 1U;
    const char *filename_prefix = "esp32_homey_wall_panel_";
    const size_t flen = strlen(filename_prefix);
    if (strncmp(tail, filename_prefix, flen) != 0) return false;
    if (strstr(tail, "..") || strchr(tail, '/')) return false;
    char expected_filename[190];
    const int n = snprintf(expected_filename, sizeof(expected_filename),
                           "esp32_homey_wall_panel_%s_%s_signed.bin",
                           m->hardware_model, m->version);
    return n > 0 && (size_t)n < sizeof(expected_filename) &&
           strcmp(tail, expected_filename) == 0;
}

bool panel_update_manifest_metadata_valid(const panel_update_manifest_t *m)
{
    if (!m || m->schema_version != PANEL_UPDATE_MANIFEST_SCHEMA) return false;
    if (!safe_id(m->hardware_model, sizeof(m->hardware_model)) ||
        !safe_id(m->project, sizeof(m->project)) ||
        !safe_id(m->channel, sizeof(m->channel))) return false;
    if (strcmp(m->channel, "stable") != 0 && strcmp(m->channel, "beta") != 0) return false;
    if (strnlen(m->version, sizeof(m->version)) >= sizeof(m->version) ||
        !semver(m->version)) return false;
    if (m->release_sequence == 0U || m->image_size_bytes < 288U ||
        m->image_size_bytes > PANEL_UPDATE_MAX_IMAGE_BYTES) return false;
    bool any_hash = false;
    for (size_t i = 0; i < sizeof(m->image_sha256); i++) any_hash |= (m->image_sha256[i] != 0U);
    if (!any_hash || !release_url_valid(m)) return false;
    /* Notes: UTF-8 permitted, but no C0 control characters; bounded and terminated. */
    bool terminated = false;
    for (size_t i = 0; i < sizeof(m->notes); i++) {
        if (m->notes[i] == '\0') { terminated = true; break; }
        if ((unsigned char)m->notes[i] < 32U || (unsigned char)m->notes[i] == 127U) return false;
    }
    return terminated;
}

panel_update_status_t panel_update_manifest_evaluate(const panel_update_manifest_t *m,
                                                      const panel_update_local_t *local)
{
    if (!panel_update_manifest_metadata_valid(m) || !local ||
        !local->hardware_model || !local->project || !local->channel ||
        !local->installed_version ||
        strcmp(m->hardware_model, local->hardware_model) != 0 ||
        strcmp(m->project, local->project) != 0 ||
        strcmp(m->channel, local->channel) != 0) return PANEL_UPDATE_REJECTED;

    /* Old hash-based app versions are intentionally not assumed to be semver. */
    if (local->installed_release_sequence == 0U || !semver(local->installed_version)) {
        return PANEL_UPDATE_UNVERSIONED;
    }
    if (m->release_sequence < local->installed_release_sequence) return PANEL_UPDATE_CURRENT;
    if (m->release_sequence == local->installed_release_sequence) {
        return strcmp(m->version, local->installed_version) == 0
             ? PANEL_UPDATE_CURRENT : PANEL_UPDATE_REJECTED;
    }
    if (!local->has_ota_partitions) return PANEL_UPDATE_NEWER_NEEDS_MIGRATION;
    if (local->inactive_ota_capacity == 0U || m->image_size_bytes > local->inactive_ota_capacity) {
        return PANEL_UPDATE_NEWER_TOO_LARGE;
    }
    /* IMPORTANT: Available INFORMATION ONLY. Never interprets untrusted metadata as install authority. */
    return PANEL_UPDATE_NEWER_AVAILABLE;
}
